
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

SRCS=dwn.c snd.c file_watcher.c notify.c
HEADERS=dwn.h snd.h file_watcher.h module.h
MODS=backlight.c volume.c
CFLAGS=-g -ggdb -std=gnu99 -Wall -Werror -Wl,-Tlink.ld -fuse-ld=bfd

all: $(SRCS) $(HEADERS) link.ld
	# $(CC) $(CFLAGS) snd.c -c snd.o
	# $(CC) $(CFLAGS) file_watcher.c -c file_watcher.o
	# $(CC) $(CFLAGS) dwn.c $(shell pkg-config --cflags libnotify) -c dwn.o
	# $(CC) -o dwn dwn.o snd.o file_watcher.o \
	#     $(shell pkg-config --libs alsa) \
	#     $(shell pkg-config --libs libnotify)
	$(CC) $(CFLAGS) $(SRCS) $(MODS) $(shell pkg-config --cflags libnotify) \
		-o dwn \
	    $(shell pkg-config --libs alsa) \
	    $(shell pkg-config --libs libnotify)

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dwn ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwn

uninstall:
	rm -rf ${DESTDIR}${PREFIX}/bin/dwn

clean:
	rm -rf *.o

.PHONY: all install uninstall clean

