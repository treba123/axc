### toolchain
#
CC ?= gcc
AR ?= ar
PKG_CONFIG ?= pkg-config
NSS_CONFIG ?= nss-config
MKDIR = mkdir
MKDIR_P = mkdir -p
CMAKE ?= cmake
CMAKE_FLAGS = -DCMAKE_BUILD_TYPE=Debug -DCMAKE_C_FLAGS=-fPIC


SDIR = src
LDIR = lib
BDIR = build
TDIR = test
TFN = test_all
CDIR = coverage

AX_DIR=./lib/libsignal-protocol-c
AX_BDIR=$(AX_DIR)/build/src
AX_PATH=$(AX_BDIR)/libsignal-protocol-c.a

LGCRYPT_DIR=$(LDIR)/libgcrypt
LGCRYPT_SRC=$(LGCRYPT_DIR)/src
LGCRYPT_BUILD=$(LGCRYPT_DIR)/build
LGCRYPT_PATH=$(LGCRYPT_BUILD)/libgcrypt.a

PKGCFG_C=$(shell $(PKG_CONFIG) --cflags sqlite3 glib-2.0)
PKGCFG_L=$(shell $(PKG_CONFIG) --libs sqlite3 glib-2.0) \
		 $(shell $(NSS_CONFIG) --libs)

HEADERS=-I$(AX_DIR)/src -I$(LGCRYPT_DIR)/src
CFLAGS += $(HEADERS) $(PKGCFG_C) -std=c11 -Wall -Wextra -Wpedantic \
		  -Wstrict-overflow -fno-strict-aliasing -funsigned-char \
		  -fno-builtin-memset
CPPFLAGS += -D_XOPEN_SOURCE=700 -D_BSD_SOURCE -D_POSIX_SOURCE -D_GNU_SOURCE
TESTFLAGS=$(HEADERS) $(PKGCFG_C) -g -O0 --coverage
PICFLAGS=-fPIC $(CFLAGS)
LDFLAGS += -pthread -ldl $(PKGCFG_L) $(AX_PATH) -lm
LDFLAGS_T= -lcmocka $(LDFLAGS)

all: $(BDIR)/libaxc.a

$(BDIR):
	$(MKDIR_P) $@

client: $(SDIR)/message_client.c $(BDIR)/axc_store.o $(BDIR)/axc_crypto.o $(BDIR)/axc.o $(AX_PATH)
	$(MKDIR_P) $@
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@/$@.o $(LDFLAGS)

$(BDIR)/axc.o: $(SDIR)/axc.c $(BDIR)
	$(CC) $(PICFLAGS) $(CPPFLAGS) -c $< -o $@

$(BDIR)/axc-nt.o: $(SDIR)/axc.c $(BDIR)
	$(CC) $(PICFLAGS) $(CPPFLAGS) -DNO_THREADS -c $< -o $@

$(BDIR)/axc_crypto.o: $(SDIR)/axc_crypto.c $(BDIR)
	$(CC) $(PICFLAGS) $(CPPFLAGS) -c $< -o $@

$(BDIR)/axc_store.o: $(SDIR)/axc_store.c $(BDIR)
	$(CC) $(PICFLAGS) $(CPPFLAGS) -c $< -o $@

$(BDIR)/libaxc.a: $(BDIR)/axc.o $(BDIR)/axc_crypto.o $(BDIR)/axc_store.o $(LGCRYPT_PATH)
	$(AR) rcs $@ $^

$(BDIR)/libaxc-nt.a: $(BDIR)/axc-nt.o $(BDIR)/axc_crypto.o $(BDIR)/axc_store.o $(LGCRYPT_PATH)
	$(AR) rcs $@ $^

$(AX_PATH):
	cd $(AX_DIR) && \
		$(MKDIR_P) build && \
		cd build && \
		$(CMAKE) $(CMAKE_FLAGS) ..  && \
		$(MAKE)

$(LGCRYPT_PATH):
	$(MAKE) -C "$(LGCRYPT_DIR)" build/libgcrypt.a

.PHONY: test
test: $(AX_PATH) test_store test_client

.PHONY: test_store
test_store: $(SDIR)/axc_store.c $(SDIR)/axc_crypto.c $(TDIR)/test_store.c
	$(CC) $(TESTFLAGS) -o $(TDIR)/$@.o  $(TDIR)/test_store.c $(SDIR)/axc_crypto.c $(LDFLAGS_T)
	-$(TDIR)/$@.o
	mv *.g* $(TDIR)

.PHONY: test_client
test_client: $(SDIR)/axc.c $(SDIR)/axc_crypto.c  $(SDIR)/axc_store.c $(TDIR)/test_client.c
	$(CC) $(TESTFLAGS) -g $(HEADERS) -o $(TDIR)/$@.o $(SDIR)/axc_crypto.c $(TDIR)/test_client.c $(LDFLAGS_T)
	-$(TDIR)/$@.o
	mv *.g* $(TDIR)

.PHONY: coverage
coverage: test
	gcovr -r . --html --html-details -o $@.html
	gcovr -r . -s
	$(MKDIR_P) $@
	mv $@.* $@

.PHONY: clean
clean:
	rm -f $(TDIR)/*.o
	rm -f $(TDIR)/*.gcno $(TDIR)/*.gcda $(TDIR)/*.sqlite
	
.PHONY: clean-all
clean-all: clean
	rm -rf client $(BDIR) $(CDIR) $(AX_DIR)/build


