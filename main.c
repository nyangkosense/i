/* See LICENSE file for copyright and license details. */
/* i - terminal system information utility */

#define _POSIX_C_SOURCE 200809L

#include <ifaddrs.h>
#include <locale.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statvfs.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>

#include "termbox2.h"

#include "config.h"

#define MAXSTRLEN 256

typedef struct {
	char timestr[MAXSTRLEN];
	char uptimestr[MAXSTRLEN];
	char memorystr[MAXSTRLEN];
	char cpustr[MAXSTRLEN];
	char networkstr[MAXSTRLEN];
	char batterystr[MAXSTRLEN];
	char vpnstr[MAXSTRLEN];
	char systemstr[MAXSTRLEN];
	char ipstr[MAXSTRLEN];
	char gatewaystr[MAXSTRLEN];
	char dnsstr[MAXSTRLEN];
} SysInfo;

static void usage(void);
static void die(const char *msg);
static void printcenteredin(const char *str, int x, int y, int width, uint16_t fg, uint16_t bg);
static void printat(const char *str, int x, int y, uint16_t fg, uint16_t bg);
static void getcurrenttime(char *buffer);
static void getuptime(char *buffer);
static void getmemoryinfo(char *buffer);
static void getcpuusage(char *buffer);
static void getnetworkstatus(char *buffer);
static void getbatterystatus(char *buffer);
static void getvpnstatus(char *buffer);
static void getipaddress(char *buffer);
static void getgateway(char *buffer);
static void getdns(char *buffer);
static void collectsysteminfo(SysInfo *info);
static void displayinfo(const SysInfo *info);
static void drawbox(int x, int y, int width, int height, const char *title, uint16_t fg, uint16_t bg);
static void drawseparator(int x, int y, int width, uint16_t fg, uint16_t bg);
static void drawhexbanner(int x, int y, int width, uint16_t fg, uint16_t bg);
static void drawhexbackground(int width, int height);
static void detectsystem(char *buffer);
static const char **getasciiart(const char *system);
static void drawasciiart(const char **art, int x, int y, int width, int height, uint16_t fg, uint16_t bg);

static char *argv0;

static void
usage(void)
{
	fprintf(stderr, "usage: %s\n", argv0);
	exit(1);
}

static void
die(const char *msg)
{
	fprintf(stderr, "%s: %s\n", argv0, msg);
	exit(1);
}


static void
printcenteredin(const char *str, int x, int y, int width, uint16_t fg, uint16_t bg)
{
	int len, center_x, i;

	len = strlen(str);
	center_x = x + (width - len) / 2;

	for (i = 0; str[i]; i++)
		tb_set_cell(center_x + i, y, str[i], fg, bg);
}

static void
printat(const char *str, int x, int y, uint16_t fg, uint16_t bg)
{
	int i;

	for (i = 0; str[i]; i++)
		tb_set_cell(x + i, y, str[i], fg, bg);
}


static void
getcurrenttime(char *buffer)
{
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, MAXSTRLEN, "%Y-%m-%d %H:%M:%S", timeinfo);
}

static void
getuptime(char *buffer)
{
	struct sysinfo info;
	int days, hours, minutes;

	if (sysinfo(&info) == 0) {
		days = info.uptime / 86400;
		hours = (info.uptime % 86400) / 3600;
		minutes = (info.uptime % 3600) / 60;
		snprintf(buffer, MAXSTRLEN, "%dd %dh %dm", days, hours, minutes);
	} else {
		strcpy(buffer, "Unknown");
	}
}

static void
getmemoryinfo(char *buffer)
{
	FILE *fp;
	unsigned long memtotal, memfree, memavailable, buffers, cached;
	unsigned long totalmb, availablemb, usedmb;
	int usagepercent;

	memtotal = memfree = memavailable = buffers = cached = 0;

	fp = fopen("/proc/meminfo", "r");
	if (!fp) {
		strcpy(buffer, "Unknown");
		return;
	}

	char line[256];
	while (fgets(line, sizeof(line), fp)) {
		if (sscanf(line, "MemTotal: %lu kB", &memtotal) == 1) continue;
		if (sscanf(line, "MemFree: %lu kB", &memfree) == 1) continue;
		if (sscanf(line, "MemAvailable: %lu kB", &memavailable) == 1) continue;
		if (sscanf(line, "Buffers: %lu kB", &buffers) == 1) continue;
		if (sscanf(line, "Cached: %lu kB", &cached) == 1) continue;
	}
	fclose(fp);

	if (memtotal > 0) {
		totalmb = memtotal / 1024;
		if (memavailable > 0) {
			availablemb = memavailable / 1024;
			usedmb = totalmb - availablemb;
		} else {
			availablemb = (memfree + buffers + cached) / 1024;
			usedmb = totalmb - availablemb;
		}
		usagepercent = (usedmb * 100) / totalmb;

		snprintf(buffer, MAXSTRLEN, "%lu MB / %lu MB (%d%%)",
		         usedmb, totalmb, usagepercent);
	} else {
		strcpy(buffer, "Unknown");
	}
}

static void
getcpuusage(char *buffer)
{
	static long previdle = 0, prevtotal = 0;
	FILE *fp;
	long user, nice, system, idle, iowait, irq, softirq, steal;
	long total, diffidle, difftotal;
	int usage;

	fp = fopen("/proc/stat", "r");
	if (!fp) {
		strcpy(buffer, "Unknown");
		return;
	}

	fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
	       &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
	fclose(fp);

	total = user + nice + system + idle + iowait + irq + softirq + steal;
	diffidle = idle - previdle;
	difftotal = total - prevtotal;

	if (difftotal > 0) {
		usage = 100 - (diffidle * 100 / difftotal);
		snprintf(buffer, MAXSTRLEN, "%d%%", usage);
	} else {
		strcpy(buffer, "0%");
	}

	previdle = idle;
	prevtotal = total;
}

static void
getnetworkstatus(char *buffer)
{
	struct ifaddrs *ifaddr, *ifa;
	int connectedinterfaces;
	char interfacelist[MAXSTRLEN];

	connectedinterfaces = 0;
	interfacelist[0] = '\0';

	if (getifaddrs(&ifaddr) == -1) {
		strcpy(buffer, "Unknown");
		return;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		if (ifa->ifa_addr->sa_family == AF_INET &&
		    strcmp(ifa->ifa_name, "lo") != 0) {
			if (connectedinterfaces > 0)
				strcat(interfacelist, ", ");
			strcat(interfacelist, ifa->ifa_name);
			connectedinterfaces++;
		}
	}

	freeifaddrs(ifaddr);

	if (connectedinterfaces > 0)
		snprintf(buffer, MAXSTRLEN, "Connected: %s", interfacelist);
	else
		strcpy(buffer, "No network connection");
}

static void
getbatterystatus(char *buffer)
{
	FILE *fp, *statusfp;
	int capacity;
	char status[32];

	char path[MAXSTRLEN];
	
	snprintf(path, MAXSTRLEN, "%s/capacity", battery_path);
	fp = fopen(path, "r");
	if (fp) {
		fscanf(fp, "%d", &capacity);
		fclose(fp);

		strcpy(status, "Unknown");
		snprintf(path, MAXSTRLEN, "%s/status", battery_path);
		statusfp = fopen(path, "r");
		if (statusfp) {
			fscanf(statusfp, "%s", status);
			fclose(statusfp);
		}

		snprintf(buffer, MAXSTRLEN, "%d%% (%s)", capacity, status);
	} else {
		strcpy(buffer, "No battery detected");
	}
}

static void
getvpnstatus(char *buffer)
{
	FILE *fp;
	char line[256];

	fp = popen("ip route show | grep -E '(tun|tap|vpn)' | head -1", "r");
	if (fp) {
		if (fgets(line, sizeof(line), fp))
			strcpy(buffer, "VPN: Active");
		else
			strcpy(buffer, "VPN: Inactive");
		pclose(fp);
	} else {
		strcpy(buffer, "VPN: Unknown");
	}
}

static void
getipaddress(char *buffer)
{
	FILE *fp;
	char line[256];

	fp = popen("ip route get 8.8.8.8 2>/dev/null | awk '/src/ {print $7}' | head -1", "r");
	if (fp) {
		if (fgets(line, sizeof(line), fp)) {
			line[strcspn(line, "\n")] = 0;
			snprintf(buffer, MAXSTRLEN, "IP: %s", line);
		} else {
			strcpy(buffer, "IP: Unknown");
		}
		pclose(fp);
	} else {
		strcpy(buffer, "IP: Unknown");
	}
}

static void
getgateway(char *buffer)
{
	FILE *fp;
	char line[256];

	fp = popen("ip route | awk '/default/ {print $3}' | head -1", "r");
	if (fp) {
		if (fgets(line, sizeof(line), fp)) {
			line[strcspn(line, "\n")] = 0;
			snprintf(buffer, MAXSTRLEN, "Gateway: %s", line);
		} else {
			strcpy(buffer, "Gateway: Unknown");
		}
		pclose(fp);
	} else {
		strcpy(buffer, "Gateway: Unknown");
	}
}

static void
getdns(char *buffer)
{
	FILE *fp;
	char line[256];

	fp = fopen("/etc/resolv.conf", "r");
	if (fp) {
		while (fgets(line, sizeof(line), fp)) {
			if (strncmp(line, "nameserver", 10) == 0) {
				char *dns = line + 10;
				while (*dns == ' ' || *dns == '\t') dns++;
				dns[strcspn(dns, "\n")] = 0;
				snprintf(buffer, MAXSTRLEN, "DNS: %s", dns);
				fclose(fp);
				return;
			}
		}
		fclose(fp);
	}
	strcpy(buffer, "DNS: Unknown");
}

static void
collectsysteminfo(SysInfo *info)
{
	getcurrenttime(info->timestr);
	getuptime(info->uptimestr);
	getmemoryinfo(info->memorystr);
	getcpuusage(info->cpustr);
	getnetworkstatus(info->networkstr);
	getbatterystatus(info->batterystr);
	getvpnstatus(info->vpnstr);
	detectsystem(info->systemstr);
	getipaddress(info->ipstr);
	getgateway(info->gatewaystr);
	getdns(info->dnsstr);
}


static void
drawseparator(int x, int y, int width, uint16_t fg, uint16_t bg)
{
	int i;

	tb_set_cell(x, y, 0x251C, fg, bg);
	for (i = 1; i < width - 1; i++)
		tb_set_cell(x + i, y, 0x2500, fg, bg);
	tb_set_cell(x + width - 1, y, 0x2524, fg, bg);
}


static void
drawhexbanner(int x, int y, int width, uint16_t fg, uint16_t bg)
{
	char hexline[512];
	char hexdata[128];
	char asciidata[64];
	int bytes_per_line, max_bytes;
	int i;
	
	/* Calculate how many bytes we can fit per line based on terminal width */
	max_bytes = (width - 15) / 4;
	if (max_bytes > 32) max_bytes = 32;
	if (max_bytes < 8) max_bytes = 8;
	bytes_per_line = (max_bytes / 8) * 8; /* Round to multiple of 8 */
	
	char banner_msg[] = "i v0.1";
	int banner_len = strlen(banner_msg);
	
	/* Generate hex data for the banner line */
	for (i = 0; i < bytes_per_line; i++) {
		unsigned char byte;
		if (i < banner_len) {
			byte = banner_msg[i];
		} else if (i == banner_len) {
			byte = 0x00; /* Null terminator */
		} else {
			byte = 0x20; /* Space padding */
		}
		
		sprintf(hexdata + (i * 3), "%02x ", byte);
		asciidata[i] = (byte >= 32 && byte <= 126) ? byte : '.';
	}
	hexdata[bytes_per_line * 3 - 1] = '\0'; /* Remove last space */
	asciidata[bytes_per_line] = '\0';
	
	int mid = (bytes_per_line / 2) * 3;
	memmove(hexdata + mid + 1, hexdata + mid, strlen(hexdata + mid) + 1);
	hexdata[mid] = ' ';
	
	snprintf(hexline, sizeof(hexline), "00000000  %s |%s|", hexdata, asciidata);
	
	int len = strlen(hexline);
	while (len < width - 1) {
		hexline[len++] = ' ';
	}
	hexline[width - 1] = '\0';
	
	printat(hexline, x, y, fg, bg);
}

static void
drawhexbackground(int width, int height)
{
	char hexline[512];
	char hexdata[128];
	char asciidata[64];
	int addr, i, j, bytes_per_line, max_bytes;
	unsigned char byte;
	
	struct timeval tv;
	gettimeofday(&tv, NULL);
	unsigned int seed = (unsigned int)(tv.tv_sec * 1000000 + tv.tv_usec) & 0xFFFFFF;
	
	/* Calculate how many bytes we can fit per line based on terminal width */
	/* Format: "00000000  xx xx xx xx  |ascii| " = ~10 + 3*bytes + 3 + bytes */
	max_bytes = (width - 15) / 4;
	if (max_bytes > 32) max_bytes = 32;
	if (max_bytes < 8) max_bytes = 8;
	bytes_per_line = (max_bytes / 8) * 8; /* Round to multiple of 8 */
	
	for (i = 0; i < height; i++) {
		addr = i * bytes_per_line;
		
		for (j = 0; j < bytes_per_line; j++) {
			seed = (seed * 1103515245 + 12345) & 0x7FFFFFFF;
			byte = (seed >> 16) & 0xFF;
			
			if (j % 4 == 0) byte &= 0xF0; /* Some aligned data */
			if (j == bytes_per_line/2) byte = 0x00; /* Null bytes */
			
			sprintf(hexdata + (j * 3), "%02x ", byte);
			asciidata[j] = (byte >= 32 && byte <= 126) ? byte : '.';
		}
		hexdata[bytes_per_line * 3 - 1] = '\0'; /* Remove last space */
		asciidata[bytes_per_line] = '\0';
		
		int mid = (bytes_per_line / 2) * 3;
		memmove(hexdata + mid + 1, hexdata + mid, strlen(hexdata + mid) + 1);
		hexdata[mid] = ' ';
		
		snprintf(hexline, sizeof(hexline), "%08x  %s |%s|", 
		         addr, hexdata, asciidata);
		
		int len = strlen(hexline);
		while (len < width - 1) {
			hexline[len++] = ' ';
		}
		hexline[width - 1] = '\0';
		
		printat(hexline, 0, i, TB_BLACK | TB_BRIGHT, TB_DEFAULT);
	}
}

static void
detectsystem(char *buffer)
{
	FILE *fp;
	char line[256];

	strcpy(buffer, "linux");

	fp = fopen("/etc/os-release", "r");
	if (!fp) {
		fp = fopen("/usr/lib/os-release", "r");
		if (!fp)
			return;
	}

	while (fgets(line, sizeof(line), fp)) {
		if (strncmp(line, "ID=", 3) == 0) {
			char *id;
			id = line + 3;
			
			if (*id == '"') {
				id++;
				id[strcspn(id, "\"")] = 0;
			} else {
				id[strcspn(id, "\n")] = 0;
			}
			
			strncpy(buffer, id, MAXSTRLEN - 1);
			buffer[MAXSTRLEN - 1] = '\0';
			break;
		}
	}
	fclose(fp);
}

static const char **
getasciiart(const char *system)
{
	if (!system)
		return ascii_linux;

	if (strcmp(system, "alpine") == 0)
		return ascii_alpine;
	if (strcmp(system, "android") == 0)
		return ascii_android;
	if (strcmp(system, "arch") == 0)
		return ascii_arch;
	if (strcmp(system, "arco") == 0)
		return ascii_arco;
	if (strcmp(system, "artix") == 0)
		return ascii_artix;
	if (strcmp(system, "centos") == 0)
		return ascii_centos;
	if (strcmp(system, "debian") == 0)
		return ascii_debian;
	if (strcmp(system, "endeavouros") == 0)
		return ascii_endeavour;
	if (strcmp(system, "fedora") == 0)
		return ascii_fedora;
	if (strcmp(system, "freebsd") == 0)
		return ascii_freebsd;
	if (strcmp(system, "gentoo") == 0)
		return ascii_gentoo;
	if (strcmp(system, "linuxmint") == 0)
		return ascii_linux_mint;
	if (strstr(system, "mint") != NULL)
		return ascii_linux_mint;
	if (strcmp(system, "darwin") == 0)
		return ascii_macos;
	if (strcmp(system, "manjaro") == 0)
		return ascii_manjaro;
	if (strcmp(system, "nixos") == 0)
		return ascii_nixos;
	if (strcmp(system, "opensuse") == 0)
		return ascii_opensuse;
	if (strcmp(system, "pop") == 0)
		return ascii_pop_os;
	if (strstr(system, "pop") != NULL)
		return ascii_pop_os;
	if (strcmp(system, "slackware") == 0)
		return ascii_slackware;
	if (strcmp(system, "solus") == 0)
		return ascii_solus;
	if (strcmp(system, "ubuntu") == 0)
		return ascii_ubuntu;
	if (strcmp(system, "void") == 0)
		return ascii_void;

	return ascii_linux;
}

static void
drawasciiart(const char **art, int x, int y, int width, int height, uint16_t fg, uint16_t bg)
{
	int i, j, artlines, start_y, start_x;

	if (!art)
		return;

	for (artlines = 0; art[artlines] != NULL; artlines++)
		;

	start_y = y + 1 + (height - 2 - artlines) / 2;
	if (start_y < y + 1)
		start_y = y + 1;

	for (i = 0; i < artlines && start_y + i < y + height - 1; i++) {
		start_x = x + 1;
		
		for (j = 0; art[i][j] && start_x + j < x + width - 1; j++) {
			tb_set_cell(start_x + j, start_y + i, art[i][j], fg, TB_BLACK);
		}
	}
}

static void
drawbox(int x, int y, int width, int height, const char *title, uint16_t fg, uint16_t bg)
{
	int i, j;
	
	(void)bg; /* Intentionally unused - we always use TB_BLACK for background */

	/* Fill the box with solid background to cover hex dump */
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			tb_set_cell(x + j, y + i, ' ', fg, TB_BLACK);
		}
	}

	/* Draw borders */
	tb_set_cell(x, y, 0x250C, fg, TB_BLACK);
	tb_set_cell(x + width - 1, y, 0x2510, fg, TB_BLACK);
	tb_set_cell(x, y + height - 1, 0x2514, fg, TB_BLACK);
	tb_set_cell(x + width - 1, y + height - 1, 0x2518, fg, TB_BLACK);

	for (i = 1; i < width - 1; i++) {
		tb_set_cell(x + i, y, 0x2500, fg, TB_BLACK);
		tb_set_cell(x + i, y + height - 1, 0x2500, fg, TB_BLACK);
	}

	for (i = 1; i < height - 1; i++) {
		tb_set_cell(x, y + i, 0x2502, fg, TB_BLACK);
		tb_set_cell(x + width - 1, y + i, 0x2502, fg, TB_BLACK);
	}

	if (title && strlen(title) > 0) {
		int title_x = x + (width - strlen(title) - 2) / 2;
		tb_set_cell(title_x, y, 0x251C, fg, TB_BLACK);
		printat(title, title_x + 1, y, fg | TB_BOLD, TB_BLACK);
		tb_set_cell(title_x + strlen(title) + 1, y, 0x2524, fg, TB_BLACK);
	}
}

static void
displayinfo(const SysInfo *info)
{
	struct passwd *pw;
	struct utsname buf;
	char displayline[MAXSTRLEN];
	char temp[64];
	int width, height, memperc, cpuperc, battperc;
	int hex_width, max_bytes, bytes_per_line;
	int ascii_box_width, system_box_width, system_box_x;
	uint16_t memcolor, cpucolor, battcolor;
	FILE *hostname_fp;
	char hostname[64];
	const char **art;
	uid_t uid;

	width = tb_width();
	height = tb_height();

	max_bytes = (width - 15) / 4;
	if (max_bytes > 32) max_bytes = 32;
	if (max_bytes < 8) max_bytes = 8;
	bytes_per_line = (max_bytes / 8) * 8; /* Round to multiple of 8 */
	hex_width = 10 + (bytes_per_line * 3) + 1 + bytes_per_line + 1; /* addr + hex + space + ascii + | */

	ascii_box_width = (hex_width - 8) / 6;
	if (ascii_box_width < 20) ascii_box_width = 20;
	system_box_width = hex_width - ascii_box_width - 6;
	system_box_x = 2 + ascii_box_width + 2;

	tb_clear();
	
	drawhexbackground(width, height);
	
	drawhexbanner(0, 1, width, TB_GREEN | TB_BOLD, TB_BLACK);

	/* Draw ASCII art box */
	art = getasciiart(info->systemstr);
	drawbox(2, 6, ascii_box_width, 12, " OS ", TB_CYAN, TB_BLACK);
	drawasciiart(art, 2, 6, ascii_box_width, 12, TB_CYAN | TB_BOLD, TB_BLACK);
	
	uid = getuid();
	pw = getpwuid(uid);
	
	/* Get system information */
	if (uname(&buf) != 0) {
		strcpy(buf.sysname, "Unknown");
		strcpy(buf.release, "Unknown");
		strcpy(buf.machine, "Unknown");
	}

	strcpy(hostname, "Unknown");
	hostname_fp = fopen("/etc/hostname", "r");
	if (hostname_fp) {
		if (fgets(hostname, sizeof(hostname), hostname_fp))
			hostname[strcspn(hostname, "\n")] = 0;
		fclose(hostname_fp);
	}

	drawbox(system_box_x, 6, system_box_width, 8, " SYSTEM ", TB_GREEN, TB_BLACK);
	snprintf(displayline, MAXSTRLEN, "%s", info->timestr);
	printcenteredin(displayline, system_box_x, 8, system_box_width, TB_YELLOW, TB_BLACK);
	snprintf(displayline, MAXSTRLEN, "%s", info->uptimestr);
	printcenteredin(displayline, system_box_x, 9, system_box_width, TB_GREEN, TB_BLACK);
	
	drawseparator(system_box_x + 2, 10, system_box_width - 4, TB_GREEN, TB_BLACK);
	
	snprintf(displayline, MAXSTRLEN, "Host: %s@%s", pw ? pw->pw_name : "Unknown", hostname);
	printcenteredin(displayline, system_box_x, 11, system_box_width, TB_CYAN, TB_BLACK);

	snprintf(displayline, MAXSTRLEN, "System: %s %s %s", buf.sysname, buf.release, buf.machine);
	printcenteredin(displayline, system_box_x, 12, system_box_width, TB_CYAN, TB_BLACK);

	drawbox(2, 19, (hex_width - 6) / 2, 9, " RESOURCES ", TB_YELLOW, TB_BLACK);
	
	if (sscanf(info->memorystr, "%*d MB / %*d MB (%d%%)", &memperc) != 1)
		memperc = 0;
	memcolor = memperc > 80 ? TB_RED : memperc > 60 ? TB_YELLOW : TB_GREEN;
	
	printcenteredin("Memory:", 2, 21, (hex_width - 6) / 2, TB_WHITE | TB_BOLD, TB_BLACK);
	snprintf(temp, sizeof(temp), "%d%%", memperc);
	printcenteredin(temp, 2, 22, (hex_width - 6) / 2, memcolor, TB_BLACK);
	printcenteredin(info->memorystr, 2, 23, (hex_width - 6) / 2, TB_BLUE, TB_BLACK);

	if (sscanf(info->cpustr, "%d%%", &cpuperc) != 1)
		cpuperc = 0;
	cpucolor = cpuperc > 80 ? TB_RED : cpuperc > 60 ? TB_YELLOW : TB_GREEN;
	
	printcenteredin("CPU:", 2, 25, (hex_width - 6) / 2, TB_WHITE | TB_BOLD, TB_BLACK);
	snprintf(temp, sizeof(temp), "%d%%", cpuperc);
	printcenteredin(temp, 2, 26, (hex_width - 6) / 2, cpucolor, TB_BLACK);

	drawbox(2 + (hex_width - 6) / 2 + 2, 19, (hex_width - 6) / 2, 15, " CONNECTIVITY ", TB_BLUE, TB_BLACK);
	
	printcenteredin("Network:", 2 + (hex_width - 6) / 2 + 2, 21, (hex_width - 6) / 2, TB_WHITE | TB_BOLD, TB_BLACK);
	if (strstr(info->networkstr, "Connected") != NULL) {
		printcenteredin(info->networkstr, 2 + (hex_width - 6) / 2 + 2, 22, (hex_width - 6) / 2, TB_GREEN, TB_BLACK);
	} else {
		printcenteredin(info->networkstr, 2 + (hex_width - 6) / 2 + 2, 22, (hex_width - 6) / 2, TB_RED, TB_BLACK);
	}
	
	printcenteredin(info->ipstr, 2 + (hex_width - 6) / 2 + 2, 24, (hex_width - 6) / 2, TB_CYAN, TB_BLACK);
	printcenteredin(info->gatewaystr, 2 + (hex_width - 6) / 2 + 2, 25, (hex_width - 6) / 2, TB_CYAN, TB_BLACK);
	printcenteredin(info->dnsstr, 2 + (hex_width - 6) / 2 + 2, 26, (hex_width - 6) / 2, TB_CYAN, TB_BLACK);
	
	printcenteredin("Security:", 2 + (hex_width - 6) / 2 + 2, 28, (hex_width - 6) / 2, TB_WHITE | TB_BOLD, TB_BLACK);
	if (strstr(info->vpnstr, "Active") != NULL) {
		printcenteredin(info->vpnstr, 2 + (hex_width - 6) / 2 + 2, 29, (hex_width - 6) / 2, TB_GREEN, TB_BLACK);
	} else {
		printcenteredin(info->vpnstr, 2 + (hex_width - 6) / 2 + 2, 29, (hex_width - 6) / 2, TB_WHITE, TB_BLACK);
	}
	
	drawbox(2, 35, hex_width - 4, 6, " POWER ", TB_MAGENTA, TB_BLACK);
	
	if (strstr(info->batterystr, "No battery") == NULL) {
		if (sscanf(info->batterystr, "%d%%", &battperc) != 1)
			battperc = 0;
		battcolor = battperc < 20 ? TB_RED : battperc < 50 ? TB_YELLOW : TB_GREEN;
		
		printcenteredin("Battery:", 2, 37, hex_width - 4, TB_WHITE | TB_BOLD, TB_BLACK);
		snprintf(temp, sizeof(temp), "%d%%", battperc);
		printcenteredin(temp, 2, 38, hex_width - 4, battcolor, TB_BLACK);
		printcenteredin(info->batterystr, 2, 39, hex_width - 4, TB_MAGENTA, TB_BLACK);
	} else {
		printcenteredin("AC Power Only", 2, 37, hex_width - 4, TB_MAGENTA, TB_BLACK);
		printcenteredin("No battery detected", 2, 38, hex_width - 4, TB_CYAN, TB_BLACK);
	}

	/* Create a footer box */
	drawbox(2, height - 4, hex_width - 4, 3, "", TB_WHITE, TB_BLACK);
	snprintf(displayline, MAXSTRLEN, "'q' Quit  *  'r' Reboot  *  's' Shutdown  *  Refreshes every %ds", refresh_interval);
	printcenteredin(displayline, 2, height - 2, hex_width - 4,
	              TB_WHITE | TB_BOLD, TB_BLACK);

	tb_present();
}

int
main(int argc, char *argv[])
{
	SysInfo info;
	struct tb_event ev;
	struct timeval lastupdate, lasthexupdate, currenttime;
	int ret;

	argv0 = argv[0];

	if (argc != 1)
		usage();

	setlocale(LC_ALL, "");

	ret = tb_init();
	if (ret)
		die("tb_init() failed");

	tb_set_input_mode(TB_INPUT_ESC);

	gettimeofday(&lastupdate, NULL);
	gettimeofday(&lasthexupdate, NULL);

	for (;;) {
		gettimeofday(&currenttime, NULL);

		double elapsed_update = (currenttime.tv_sec - lastupdate.tv_sec) + 
		                       (currenttime.tv_usec - lastupdate.tv_usec) / 1000000.0;
		double elapsed_hex = (currenttime.tv_sec - lasthexupdate.tv_sec) + 
		                    (currenttime.tv_usec - lasthexupdate.tv_usec) / 1000000.0;

		if (elapsed_update >= refresh_interval) {
			collectsysteminfo(&info);
			displayinfo(&info);
			lastupdate = currenttime;
			lasthexupdate = currenttime;
		} else if (elapsed_hex >= hex_refresh_interval) {
			displayinfo(&info);
			lasthexupdate = currenttime;
		}

		ret = tb_peek_event(&ev, 100);

		if (ret == TB_OK) {
			if (ev.type == TB_EVENT_KEY) {
				if (ev.ch == 'q' || ev.key == TB_KEY_ESC) {
					break;
				} else if (ev.ch == 'r') {
					tb_shutdown();
					printf("Rebooting system...\n");
					fflush(stdout);
					int ret_code = system(reboot_cmd);
					if (ret_code != 0) {
						printf("Reboot command failed with exit code: %d\n", ret_code);
						printf("Command was: %s\n", reboot_cmd);
						exit(1);
					}
					return 0;
				} else if (ev.ch == 's') {
					tb_shutdown();
					printf("Shutting down system...\n");
					fflush(stdout);
					int ret_code = system(shutdown_cmd);
					if (ret_code != 0) {
						printf("Shutdown command failed with exit code: %d\n", ret_code);
						printf("Command was: %s\n", shutdown_cmd);
						exit(1);
					}
					return 0;
				} else {
					/* Debug: log unhandled keys */
					// Uncomment for debugging: printf("Unhandled key: ch=%c (%d), key=%d\n", ev.ch, ev.ch, ev.key);
				}
			} else if (ev.type == TB_EVENT_RESIZE) {
				displayinfo(&info);
			}
		}
	}

	tb_shutdown();
	return 0;
}
