LIBS = -lX11 -luuid
CFLAGS = -O2 -std=c11 -pedantic -Wextra -Wall 
PREFIX = /usr/local/bin
CACHE = $(shell if [ "$$XDG_CACHE_HOME" ]; then echo "$$XDG_CACHE_HOME"; else echo "$$HOME"/.cache; fi)

all: sst

clean:
	rm -f sst $(CACHE)/sst

sst: sst.c
	$(CC) sst.c -o sst $(CFLAGS) $(LIBS)
	strip sst

install: sst
	install ./sst $(PREFIX)/sst
