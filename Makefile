TITLE := KI1 Widescreen Test
VERSION := 01.00
TITLE_ID := KIPT00001
CONTENT_ID := IV0000-KIPT00001_00-KI1WIDETEST000001

TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
PKGTOOLS := $(TOOLCHAIN)/bin/linux
COMMONDIR := $(TOOLCHAIN)/samples/_common
BUILD_DIR := build
ELF := $(BUILD_DIR)/ki1_widescreen_test.elf
OELF := $(BUILD_DIR)/ki1_widescreen_test.oelf
PKG := $(CONTENT_ID).pkg

TARGET_FLAGS := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c \
    -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
CXXFLAGS := $(TARGET_FLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 \
    -I$(COMMONDIR) -Isrc -std=c++17 -O2 -Wall -Wextra
LIBS := -lc -lkernel -lc++ -lSceVideoOut -lScePad -lSceUserService
LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr \
    -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

ifeq ($(shell uname -s),Darwin)
CXX := /usr/local/opt/llvm/bin/clang++
LD := /usr/local/opt/lld/bin/ld.lld
else
CXX := clang++
LD := ld.lld
endif

OBJS := $(BUILD_DIR)/main.o $(BUILD_DIR)/graphics.o

.PHONY: all clean
all: $(PKG)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/main.o: src/main.cpp $(COMMONDIR)/graphics.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/graphics.o: $(COMMONDIR)/graphics.cpp $(COMMONDIR)/graphics.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(ELF): $(OBJS)
	$(LD) $(OBJS) -o $@ $(LDFLAGS)

eboot.bin: $(ELF)
	$(PKGTOOLS)/create-fself -in=$(ELF) -out=$(OELF) --eboot "eboot.bin" --paid 0x3100000000000001
	test -s eboot.bin

sce_sys/icon0.png: tools/make_icon.py
	mkdir -p sce_sys
	python3 tools/make_icon.py $@

sce_sys/param.sfo: Makefile
	mkdir -p sce_sys
	$(PKGTOOLS)/PkgTool.Core sfo_new $@
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gd'
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(PKGTOOLS)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

sce_module/libc.prx: | 
	mkdir -p sce_module
	cp $(TOOLCHAIN)/bin/data/modules/libc.prx $@

sce_module/libSceFios2.prx: |
	mkdir -p sce_module
	cp $(TOOLCHAIN)/bin/data/modules/libSceFios2.prx $@

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_module/libc.prx sce_module/libSceFios2.prx
	python3 tools/make_gp4.py --out $@ --content-id $(CONTENT_ID) $^

$(PKG): pkg.gp4
	$(PKGTOOLS)/PkgTool.Core pkg_build $< .
	test -s $@

clean:
	rm -rf $(BUILD_DIR) eboot.bin pkg.gp4 sce_sys/param.sfo sce_sys/icon0.png \
	    sce_module *.pkg
