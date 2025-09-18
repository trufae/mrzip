# Makefile for mzip

CC?=gcc
CFLAGS?=-O2 -Wall
DESTDIR?=
PREFIX?=/usr/local
BINDIR?=$(PREFIX)/bin

# Default compression algorithms to enable
#COMPRESSION_FLAGS = -DMZIP_IMPLEMENTATION \
#                     -DMZIP_ENABLE_STORE \
#                     -DMZIP_ENABLE_DEFLATE

# Uncomment to enable additional compression algorithms
#COMPRESSION_FLAGS += -DMZIP_ENABLE_ZSTD -DMZSTD_IMPLEMENTATION
#COMPRESSION_FLAGS += -DMZIP_ENABLE_LZMA
#COMPRESSION_FLAGS += -DMZIP_ENABLE_BROTLI
#COMPRESSION_FLAGS += -DMZIP_ENABLE_LZ4
#COMPRESSION_FLAGS += -DMZIP_ENABLE_LZFSE

# Libraries
LIBS = -lz

# Main targets
all: mzip

# Brotli is now self-contained in brotli.inc.c
mzip: mzip.c main.c config.h deflate.inc.c crc32.inc.c zstd.inc.c
	$(CC) $(CFLAGS) $(COMPRESSION_FLAGS) -o $@ main.c mzip.c $(LIBS)

install:
	mkdir -p $(DESTDIR)$(BINDIR)
	cp -f mzip $(DESTDIR)$(BINDIR)/mzip

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/mzip


# Enable all supported compression methods
all-compression: COMPRESSION_FLAGS += -DMZIP_ENABLE_ZSTD -DMZSTD_IMPLEMENTATION
all-compression: mzip

clean:
	rm -f mzip *.o

.PHONY: all clean all-compression
