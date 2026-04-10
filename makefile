CC     = gcc
CFLAGS = -O2 -Wall
NAME   = prim

.PHONY: all windows linux clean

all: linux

windows:
	mkdir -p dist
	x86_64-w64-mingw32-gcc $(CFLAGS) -o dist/$(NAME).exe prim_win.c -lws2_32 -lm

linux:
	mkdir -p dist
	$(CC) $(CFLAGS) -o dist/$(NAME) prim_linux.c -lm

clean:
	rm -rf dist/$(NAME) dist/$(NAME).exe
