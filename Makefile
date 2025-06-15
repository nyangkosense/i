# i - terminal system information utility
# See LICENSE file for copyright and license details.

include config.mk

SRC = main.c termbox.c
OBJ = ${SRC:.c=.o}

all: options i

options:
	@echo i build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

i: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f i ${OBJ} config.h i-${VERSION}.tar.gz *.o

dist: clean
	mkdir -p i-${VERSION}
	cp -R LICENSE Makefile README config.mk config.def.h \
		${SRC} i-${VERSION}
	tar -cf i-${VERSION}.tar i-${VERSION}
	gzip i-${VERSION}.tar
	rm -rf i-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f i ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/i
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < i.1 > ${DESTDIR}${MANPREFIX}/man1/i.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/i.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/i \
		${DESTDIR}${MANPREFIX}/man1/i.1

.PHONY: all options clean dist install uninstall
