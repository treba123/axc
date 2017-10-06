### toolchain
#
CC ?= gcc
AR ?= ar
LIBTOOL ?= libtool
PKG_CONFIG ?= pkg-config
MKDIR = mkdir
MKDIR_P = mkdir -p


SDIR = src
BDIR = build
TDIR = test
LDIR = lib

FILES =
PKGCFG_C=$(shell $(PKG_CONFIG) --cflags nss mozjs-45)
PKGCFG_L=-ldl -lgpg-error

CFLAGS += -std=c11 -w -fno-strict-aliasing -funsigned-char \
		-fno-builtin-memset $(PKGCFG_C)
CPPFLAGS += -D_XOPEN_SOURCE=700 -D_BSD_SOURCE
COVFLAGS = --coverage -O0 -g $(CFLAGS)
LDFLAGS += -pthread -ldl -lm $(PKGCFG_L)
TESTFLAGS = -lcmocka $(LDFLAGS)

all: $(BDIR)/libgcrypt.a

$(BDIR):
	$(MKDIR_P) $@

$(BDIR)/libgcrypt.o: $(SDIR)/gcrypt.c $(BDIR)
	$(CC) -c $(SDIR)/gcrypt.c $(CFLAGS) $(CPPFLAGS) -fPIC -o $@

$(BDIR)/libgcrypt.a: $(BDIR)/libgcrypt.o
	$(AR) rcs $@ $^

clean:
	rm -rf build
