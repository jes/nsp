#Makefile for NSP

CFLAGS=-Wall
LDFLAGS=
PREFIX=/usr
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
	printf -- "---------\n\
	Now add a line like:\n\
	 198 stream tcp nowait nobody /usr/sbin/nspd nspd -d /usr/bin/nspd\n\
	to /etc/inetd.conf and send inetd a SIGHUP.\n"
.PHONY: install

clean:
	rm nsp nspd
.PHONY: clean

nsp: nsp.c
	$(CC) -o nsp nsp.c $(CFLAGS) $(LDFLAGS)

nspd: nspd.c
	$(CC) -o nspd nspd.c $(CFLAGS) $(LDFLAGS)
