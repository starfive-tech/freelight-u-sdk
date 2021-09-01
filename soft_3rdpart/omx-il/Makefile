# SPDX-License-Identifier: GPL-2.0
#
# Copyright (C) 2021 StarFive Technology Co., Ltd.
#
# Project: Starfive Openmax IL Shimlayer
#
.PHONY: CREATE_DIR
BUILD_CONFIGURATION := EmbeddedRiscvLinux

PRODUCT := OMX_IL

ifeq ("$(BUILD_CONFIGURATION)", "EmbeddedRiscvLinux")
    CROSS_CC_PREFIX = riscv64-buildroot-linux-gnu-
    PLATFORM        = riscvlinux
endif
CC  = $(CROSS_CC_PREFIX)gcc
CXX = $(CROSS_CC_PREFIX)g++
LINKER=$(CC)
AR  = $(CROSS_CC_PREFIX)ar

DEFINES = -DUSE_FEEDING_METHOD_BUFFER
INCLUDES = -I./include/khronos -I./core -I./component/video/common
$(warning "the value of LOCAL_PATH is$(STAGING_DIR)")
INCLUDES += -I$(STAGING_DIR)/usr/include/wave511/sample_v2/component/
INCLUDES += -I$(STAGING_DIR)/usr/include/wave511/sample_v2/component_encoder/
INCLUDES += -I$(STAGING_DIR)/usr/include/wave511/sample_v2/component_decoder/
INCLUDES += -I$(STAGING_DIR)/usr/include/wave511/
INCLUDES += -I$(STAGING_DIR)/usr/include/wave511/vpuapi/
INCLUDES += -I$(STAGING_DIR)/usr/include/wave511/sample_v2/helper/

CFLAGS  += -g -I. -Wno-implicit-function-declaration -Wno-format -Wl,--fatal-warning $(INCLUDES) $(DEFINES) $(PLATFORM_FLAGS)
CFLAGS  += -fpic
ARFLAGS += cru
LDFLAGS = -ldl -lpthread

OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p
SOURCES_COMMON = SF_OMX_Core.c SF_OMX_video_common.c 
SOURCES_COMMON += SF_OMX_Vdec_decoder.c

SOURCES_TESTS_WAVE521 = wave521test.c
SOURCES_TESTS_WAVE511 = wave511test.c

VPATH  = component/video/common:
VPATH += component/video/enc:
VPATH += component/video/dec:
VPATH += core:
VPATH += tests:

OBJECTNAMES_COMMON=$(patsubst %.c,%.o,$(SOURCES_COMMON))
OBJECTPATHS_COMMON=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES_COMMON)))
OBJECTNAMES_WAVE521=$(patsubst %.c,%.o,$(SOURCES_TESTS_WAVE521))
OBJECTPATHS_WAVE521=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES_WAVE521)))
OBJECTNAMES_WAVE511=$(patsubst %.c,%.o,$(SOURCES_TESTS_WAVE511))
OBJECTPATHS_WAVE511=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES_WAVE511)))

all: CREATE_DIR $(OBJECTPATHS_COMMON)
	$(CC) -fPIC -shared -o libsf-omx-il.so $(LDFLAGS) $(OBJECTPATHS_COMMON)

wave521-test: CREATE_DIR $(OBJECTPATHS_WAVE521)
	$(CC) -o wave521test $(LDFLAGS) $(OBJECTPATHS_WAVE521) -lsf-omx-il -L./

wave511-test: CREATE_DIR $(OBJECTPATHS_WAVE511)
	$(CC) -o wave511test $(LDFLAGS) $(OBJECTPATHS_WAVE511) -lsf-omx-il -L./

clean:

CREATE_DIR:
	-mkdir -p $(OBJDIR)

obj/%.o: %.c $(MAKEFILE)
	$(CC) $(CFLAGS) -Wall -c $< -o $@ -MD -MF $(@:.o=.dep)


