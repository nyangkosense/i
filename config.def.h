/* See LICENSE file for copyright and license details. */

/* refresh interval in seconds */
static const int refresh_interval = 1;

/* hex background refresh interval in seconds (can be fractional) */
static const double hex_refresh_interval = 2;

/* appearance */
static const struct {
	uint16_t fg;
	uint16_t bg;
} colors[] = {
	/* element           foreground            background */
	[0] = { TB_YELLOW,    TB_DEFAULT }, /* time */
	[1] = { TB_GREEN,     TB_DEFAULT }, /* uptime */
	[2] = { TB_BLUE,      TB_DEFAULT }, /* memory */
	[3] = { TB_RED,       TB_DEFAULT }, /* cpu */
	[4] = { TB_CYAN,      TB_DEFAULT }, /* network */
	[5] = { TB_MAGENTA,   TB_DEFAULT }, /* battery */
	[6] = { TB_WHITE,     TB_DEFAULT }, /* vpn */
};

/* battery path - adjust for your system */
static const char *battery_path = "/sys/class/power_supply/BAT0";

/* commands for power management (adjust for your system) */
/* Common alternatives:
 * sudo systems: "sudo reboot", "sudo shutdown -h now"
 * doas systems: "doas reboot", "doas shutdown -h now" 
 * systemd: "systemctl reboot", "systemctl poweroff"
 * OpenBSD: "doas /sbin/reboot", "doas /sbin/shutdown -h now"
 * FreeBSD: "sudo /sbin/reboot", "sudo /sbin/shutdown -h now"
 */
static const char *reboot_cmd = "PATH=/usr/bin:/bin:/sbin:/usr/sbin doas reboot";
static const char *shutdown_cmd = "PATH=/usr/bin:/bin:/sbin:/usr/sbin doas shutdown -h now";

/* ASCII art definitions for different operating systems */
static const char *ascii_alpine[] = {
	"   /\\ /\\",
	"  /./ \\  \\",
	" /./   \\  \\",
	"/./    \\  \\",
	"//      \\  \\",
	"         \\",
	NULL
};

static const char *ascii_android[] = {
	"  ;,           ,;",
	"   ';,.-----.,;'",
	"  ,'           ',",
	" /    O     O    \\",
	"|                 |",
	"'-----------------'",
	NULL
};

static const char *ascii_arch[] = {
	"       /\\",
	"      /  \\",
	"     /\\   \\",
	"    /      \\",
	"   /   ,,   \\",
	"  /   |  |  -\\",
	" /_-''    ''-_\\",
	NULL
};

static const char *ascii_arco[] = {
	"      /\\",
	"     /  \\",
	"    / /\\ \\",
	"   / /  \\ \\",
	"  / /    \\ \\",
	" / / _____\\ \\",
	"/_/  `----.\\_\\",
	NULL
};

static const char *ascii_artix[] = {
	"      /\\",
	"     /  \\",
	"    /`'.,\\",
	"   /     ',",
	"  /      ,`\\",
	" /   ,.'`.  \\",
	"/.,`'     `'.\\",
	NULL
};

static const char *ascii_centos[] = {
	" ____^____",
	" |\\  |  /|",
	" | \\ | / |",
	"<---- ---->",
	" | / | \\ |",
	" |/__| __\\|",
	"     v",
	NULL
};

static const char *ascii_debian[] = {
	"  _____",
	" /  __ \\",
	"|  /    |",
	"|  \\___-",
	"-_",
	"  --_",
	NULL
};

static const char *ascii_endeavour[] = {
	"      /\\",
	"    //  \\\\",
	"   //    \\ \\",
	" / //     _) )",
	"/_/___-- __-",
	" /____--",
	NULL
};

static const char *ascii_fedora[] = {
	"        ,'''''.    ",
	"       |   ,.  |   ",
	"       |  |  '_'   ",
	"  ,....|  |..      ",
	".'  ,_;|   ..'     ",
	"|  |   |  |        ",
	"|  ',_,'  |        ",
	" '.     ,'         ",
	"   '''''           ",
	NULL
};

static const char *ascii_freebsd[] = {
	"/\\,-'''''-,/\\",
	"\\_)       (_/",
	"|           |",
	"|           |",
	" ;         ;",
	"  '-_____-'",
	NULL
};

static const char *ascii_gentoo[] = {
	" _-----_",
	"(       \\",
	"\\    0   \\",
	" \\        )",
	" /      _/",
	"(     _-",
	"\\____-",
	NULL
};

static const char *ascii_linux[] = {
	"    ___",
	"   (.. |",
	"   (<> |",
	"  / __  \\",
	" ( /  \\ /|",
	"_/\\ __)/_)",
	"\\/----\\/",
	NULL
};

static const char *ascii_linux_mint[] = {
	" ___________",
	"|_          \\",
	"  | | _____ |",
	"  | | | | | |",
	"  | | | | | |",
	"  | \\__ ___/ |",
	"  \\_________/",
	NULL
};

static const char *ascii_macos[] = {
	"       .:'",
	"    _ :'_",
	" .'`_`-'_`'.",
	":________.-'",
	":_______:",
	" :_______`-;",
	"  `._.-._.'",
	NULL
};

static const char *ascii_manjaro[] = {
	"||||||||| ||||",
	"||||||||| ||||",
	"||||      ||||",
	"|||| |||| ||||",
	"|||| |||| ||||",
	"|||| |||| ||||",
	"|||| |||| ||||",
	NULL
};

static const char *ascii_nixos[] = {
	"  \\\\  \\\\ //",
	" ==\\\\__\\\\/ //",
	"   //   \\\\//",
	"==//     //==",
	" //\\\\___//",
	"// /\\\\  \\\\==",
	"  // \\\\  \\\\",
	NULL
};

static const char *ascii_opensuse[] = {
	"  _______",
	"__|   __ \\",
	"     / .\\ \\",
	"     \\__/ |",
	"   _______|",
	"   \\_______",
	"__________/",
	NULL
};

static const char *ascii_pop_os[] = {
	"______",
	"\\   _ \\        __",
	" \\ \\ \\ \\      / /",
	"  \\ \\_\\ \\    / /",
	"   \\  ___\\  /_/",
	"    \\ \\    _",
	"   __\\_\\__(_)_",
	"  (___________)",
	NULL
};

static const char *ascii_slackware[] = {
	"   ________",
	"  /  ______|",
	"  | |______",
	"  \\______  \\",
	"   ______| |",
	"| |________/",
	"|____________",
	NULL
};

static const char *ascii_solus[] = {
	"     /|",
	"    / |\\",
	"   /  | \\ _",
	"  /___|__\\_\\",
	" \\         /",
	"  `-------´",
	NULL
};

static const char *ascii_ubuntu[] = {
	"         _",
	"     ---(_)",
	" _/  ---  \\",
	"(_) |   |",
	"  \\  --- _/",
	"     ---(_)",
	NULL
};

static const char *ascii_void[] = {
	"    _______",
	" _ \\______ -",
	"| \\  ___  \\ |",
	"| | /   \\ | |",
	"| | \\___/ | |",
	"| \\______ \\_|",
	" -_______\\",
	NULL
};
