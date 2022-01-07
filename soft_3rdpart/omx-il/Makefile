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
SOURCES_COMMON += SF_OMX_Vdec_decoder.c SF_OMX_Venc_encoder.c

VPATH  = component/video/common:
VPATH += component/video/enc:
VPATH += component/video/dec:
VPATH += core:
VPATH += tests:

OBJECTNAMES_COMMON=$(patsubst %.c,%.o,$(SOURCES_COMMON))
OBJECTPATHS_COMMON=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES_COMMON)))


all: CREATE_DIR $(OBJECTPATHS_COMMON)
	$(CC) -fPIC -shared -o libsf-omx-il.so $(LDFLAGS) $(OBJECTPATHS_COMMON)

clean:

CREATE_DIR:
	-mkdir -p $(OBJDIR)

obj/%.o: %.c $(MAKEFILE)
	$(CC) $(CFLAGS) -Wall -c $< -o $@ -MD -MF $(@:.o=.dep)


