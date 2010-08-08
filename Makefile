#Makefile for NSP

VERSION?="0.1"
PREFIX?=/usr/local

CFLAGS=-Wall -DVERSION=\"$(VERSION)\"
LDFLAGS=
COMMANDS=diskspace list load uptime

all: nsp nspd
.PHONY: all

install: all
	install -d $(PREFIX)/bin $(PREFIX)/sbin $(PREFIX)/bin/nspd
	install -m 0755 nsp $(PREFIX)/bin
	install -m 0755 nspd $(PREFIX)/sbin
	for cmd in $(COMMANDS); do \
		install -m 0755 commands/$$cmd $(PREFIX)/bin/nspd ; \
	done
	printf -- "--------------------------------\n\
	Now add a line like:\n\
	 nsp 198/tcp\n\
	to /etc/services, a line like:\n\
	 nsp stream tcp nowait nobody $(PREFIX)/sbin/nspd nspd -d $(PREFIX)/bin/nspd\n\
	to /etc/inetd.conf and send inetd a SIGHUP.\n"
.PHONY: install

clean:
	rm nsp nspd
.PHONY: clean

nsp: nsp.c
	$(CC) -o nsp nsp.c $(CFLAGS) $(LDFLAGS)

nspd: nspd.c
	$(CC) -o nspd nspd.c $(CFLAGS) $(LDFLAGS)
