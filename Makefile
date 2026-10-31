TITLE := Killer Instinct
VERSION := 01.00
TITLE_ID := KIPT00001
CONTENT_ID := IV0000-KIPT00001_00-KI1WIDE000000001

TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
PKGTOOLS := $(TOOLCHAIN)/bin/linux
FSELF_TOOL ?= tools/bin/create-fself
COMMONDIR ?= third_party/openorbis-sample-graphics
BUILD_DIR := build
ELF := $(BUILD_DIR)/killer_instinct_ps4.elf
OELF := $(BUILD_DIR)/killer_instinct_ps4.oelf
PKG := $(CONTENT_ID).pkg

TARGET_FLAGS := --target=x86_64-pc-freebsd12-elf -D_GNU_SOURCE -fPIC -funwind-tables -c \
    -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include
CXXFLAGS := $(TARGET_FLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 \
    -I$(COMMONDIR) -Isrc -std=c++17 -O2 -Wall -Wextra
LIBS := -lc -lkernel -lc++ -lSceVideoOut -lScePad -lSceUserService -lm
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

# Pinned UltraCombo machine core. It is cross-compiled as a standalone archive
# first, then linked into the app which uses the in-app game-folder browser.
ULTRACOMBO_DIR ?= deps/UltraCombo/src
ULTRACOMBO_CHDR_INC := -I$(ULTRACOMBO_DIR)/third_party/libchdr/include \
    -I$(ULTRACOMBO_DIR)/third_party/libchdr/deps/lzma-26.02/include \
    -I$(ULTRACOMBO_DIR)/third_party/libchdr/deps/miniz-3.1.2 \
    -I$(ULTRACOMBO_DIR)/third_party/libchdr/deps/zstd-1.5.7
ULTRACOMBO_NAMES := cpu disasm mem ide dcs adsp adsprec rec rec_none game game_ki1 game_ki2 state
ULTRACOMBO_OBJS := $(addprefix $(BUILD_DIR)/uc_,$(addsuffix .o,$(ULTRACOMBO_NAMES)))
ULTRACOMBO_OBJS += $(BUILD_DIR)/uc_libchdr.o $(BUILD_DIR)/ki1_core_port.o
ULTRACOMBO_CFLAGS := $(TARGET_FLAGS) -std=gnu11 -O2 -Wall -Wextra \
    -I$(ULTRACOMBO_DIR) $(ULTRACOMBO_CHDR_INC)

.PHONY: all clean core-check
all: core-check $(PKG)

core-check: $(BUILD_DIR)/libki1core.a

$(BUILD_DIR)/uc_%.o: $(ULTRACOMBO_DIR)/%.c | $(BUILD_DIR)
	clang $(ULTRACOMBO_CFLAGS) -o $@ $<

$(BUILD_DIR)/uc_libchdr.o: $(ULTRACOMBO_DIR)/third_party/libchdr_all.c | $(BUILD_DIR)
	clang $(ULTRACOMBO_CFLAGS) -w -o $@ $<

$(BUILD_DIR)/ki1_core_port.o: src/ki1_core_port.c src/ki1_core_port.h | $(BUILD_DIR)
	clang $(ULTRACOMBO_CFLAGS) -o $@ $<

$(BUILD_DIR)/libki1core.a: $(ULTRACOMBO_OBJS)
	ar rcs $@ $^

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/main.o: src/main.cpp $(COMMONDIR)/graphics.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/graphics.o: $(COMMONDIR)/graphics.cpp $(COMMONDIR)/graphics.h $(COMMONDIR)/log.h | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(ELF): $(OBJS) $(BUILD_DIR)/libki1core.a
	$(LD) $(OBJS) $(BUILD_DIR)/libki1core.a -o $@ $(LDFLAGS)

eboot.bin: $(ELF)
	$(FSELF_TOOL) -in=$(ELF) -out=$(OELF) --eboot "eboot.bin" --paid 0x3100000000000001
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

sce_module/libc.prx:
	mkdir -p sce_module
	cp $(TOOLCHAIN)/bin/data/modules/libc.prx $@

sce_module/libSceFios2.prx:
	mkdir -p sce_module
	cp $(TOOLCHAIN)/bin/data/modules/libSceFios2.prx $@

sce_sys/about/right.sprx:
	test -s $@

licenses/OpenOrbis-GPL-3.0.txt: third_party/openorbis-sample-graphics/GPL-3.0.txt
	mkdir -p licenses
	cp $< $@

pkg.gp4: eboot.bin sce_sys/param.sfo sce_sys/icon0.png sce_module/libc.prx sce_module/libSceFios2.prx sce_sys/about/right.sprx licenses/OpenOrbis-GPL-3.0.txt
	python3 tools/make_gp4.py --out $@ --content-id $(CONTENT_ID) $^

$(PKG): pkg.gp4
	$(PKGTOOLS)/PkgTool.Core pkg_build $< .
	test -s $@

clean:
	rm -rf $(BUILD_DIR) eboot.bin pkg.gp4 sce_sys/param.sfo sce_sys/icon0.png \
	    sce_module *.pkg
