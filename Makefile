#Makefile for NSP

CFLAGS=-Wall
LDFLAGS=

all: nsp nspd

clean:
	rm nsp nspd

nsp: nsp.c
	$(CC) -o nsp nsp.c $(CFLAGS) $(LDFLAGS)

nspd: nspd.c
	$(CC) -o nspd nspd.c $(CFLAGS) $(LDFLAGS)
