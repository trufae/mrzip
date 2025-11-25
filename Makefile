CC?=gcc
CFLAGS?=-O2 -Wall -Wextra -std=c99
DESTDIR?=
PREFIX?=/usr/local
BINDIR?=$(PREFIX)/bin

all: otezip

otezip: src/main.c src/lib/otezip.c src/include/otezip.h src/include/config.h
	$(CC) $(CFLAGS) -I src/include -o otezip src/main.c src/lib/otezip.c

mall:
	meson build && ninja -C build
	# Ensure a convenient top-level binary exists for quick invocation
	cp -f build/otezip ./otezip

install: otezip
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -f otezip $(DESTDIR)$(BINDIR)/otezip

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/otezip

test:
	meson build && ninja -C build
	cp -f build/otezip ./otezip
	$(MAKE) -C test
	rm -rf build

test2:
	meson build -Db_sanitize=address && ninja -C build
	cp -f build/otezip ./otezip
	CFLAGS="-fsanitize=address $(CFLAGS)" $(MAKE) -C test
	rm -rf build

clean:
	rm -rf build otezip

fmt indent:
	find . -name "*.c" -exec clang-format-radare2 -i {} \;

.PHONY: all mall clean install uninstall test test2 fmt-indent
