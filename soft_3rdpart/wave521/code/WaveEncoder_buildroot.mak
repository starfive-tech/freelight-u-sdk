# ----------------------------------------------------------------------
#
# Project: C&M Video encoder sample
#
# ----------------------------------------------------------------------
.PHONY: CREATE_DIR clean all

PRODUCT := WAVE521C

$(shell cp sample_v2/component_list_encoder.h sample_v2/component/component_list.h)

USE_FFMPEG  ?= no
USE_PTHREAD = yes
USE_RTL_SIM = no
LINT_HOME   = etc/lint
TARGET		= libsfenc.so

UNAME = $(shell uname -a)
ifneq (,$(findstring i386, $(UNAME)))
    USE_32BIT = yes
endif

ifeq ($(RTL_SIM), 1)
USE_RTL_SIM = yes
endif


ifeq ($(USE_32BIT), yes)
PLATFORM    = nativelinux
else
PLATFORM    = riscvlinux
endif

CROSS_CC_PREFIX = riscv64-buildroot-linux-gnu-
VDI_C           = vdi/linux/vdi.c
VDI_OSAL_C      = vdi/linux/vdi_osal.c
MM_C            =
PLATFORM_FLAGS  =

VDI_VPATH       = vdi/linux
ifeq ("$(BUILD_CONFIGURATION)", "NonOS")
    CROSS_CC_PREFIX = arm-none-eabi-
    VDI_C           = vdi/nonos/vdi.c
    VDI_OSAL_C      = vdi/nonos/vdi_osal.c
    MM_C            = vdi/mm.c
    USE_FFMPEG      = no
    USE_PTHREAD     = no
    PLATFORM        = none
    DEFINES         = -DLIB_C_STUB
    PLATFORM_FLAGS  =
    VDI_VPATH       = vdi/nonos
    NONOS_RULE      = options_nonos.lnt
endif
ifeq ("$(BUILD_CONFIGURATION)", "EmbeddedLinux")
    CROSS_CC_PREFIX = riscv64-buildroot-linux-gnu-
    PLATFORM        = riscvlinux
endif

CC  = $(CROSS_CC_PREFIX)gcc
CXX = $(CROSS_CC_PREFIX)g++
LINKER=$(CC)
AR  = $(CROSS_CC_PREFIX)ar

INCLUDES = -I./vpuapi -I./ffmpeg/include -I./sample_v2/helper -I./sample_v2/helper/misc -I./sample_v2/component -I./vdi
INCLUDES += -I./sample_v2/component_encoder
DEFINES += -D$(PRODUCT)

ifeq ($(USE_RTL_SIM), yes)
DEFINES += -DCNM_SIM_PLATFORM -DCNM_SIM_DPI_INTERFACE -DSUPPORT_ENCODER
MM_C            = vdi/mm.c
else
endif	#USE_RTL_SIM
DEFINES += $(USER_DEFINES)
DEFINES += -DUSE_FEEDING_METHOD_BUFFER
CFLAGS  += -g -I. -Wl,--fatal-warning $(INCLUDES) $(DEFINES) $(PLATFORM_FLAGS)
ifeq ($(USE_RTL_SIM), yes)
ifeq ($(IUS), 1)
CFLAGS  += -fPIC # ncverilog is 64bit version
endif
endif
ARFLAGS += cru

LDFLAGS  += $(PLATFORM_FLAGS)


ifeq ($(USE_FFMPEG), yes)
CFLAGS  += -DSUPPORT_FFMPEG_DEMUX
INCLUDES += -I./ffmpeg/include
LDLIBS  += -lavformat -lavcodec -lavutil
LDFLAGS += -L./ffmpeg/lib/$(PLATFORM)
TARGET	= libsfenc.so

ifneq ($(USE_32BIT), yes)
LDLIBS  += -lz
endif #USE_32BIT
endif #USE_FFMPEG

ifeq ($(USE_PTHREAD), yes)
LDLIBS  += -lpthread
endif
LDLIBS  += -lm

MAKEFILE=Wave5xxEncV2.mak
OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p

SOURCES = component_enc_encoder.c      \
          component_enc_feeder.c       \
          component_enc_reader.c       \
          encoder_listener.c           \
          component_dec_decoder.c      \
          component_dec_feeder.c       \
          component_dec_renderer.c     \
          decoder_listener.c           \
          cnm_app.c                    \
          cnm_task.c                   \
          component.c                  \
          main_helper.c                \
          vpuhelper.c                  \
          bitstreamfeeder.c            \
          bsfeeder_fixedsize_impl.c    \
          bsfeeder_framesize_impl.c    \
          bsfeeder_size_plus_es_impl.c \
          bitstreamreader.c            \
          bin_comparator_impl.c        \
          comparator.c                 \
          md5_comparator_impl.c        \
          yuv_comparator_impl.c        \
          cfgParser.c                  \
          cnm_video_helper.c           \
          container.c                  \
          datastructure.c              \
          debug.c                      \
          yuvfeeder.c                  \
          yuvLoaderfeeder.c            \
          yuvBufferFeeder.c            \
          yuvCfbcfeeder.c              \
          bw_monitor.c                 \
          pf_monitor.c
SOURCES += $(VDI_C)                                               \
          $(VDI_OSAL_C)                                           \
          $(MM_C)                                                 \
          vpuapi/product.c                                        \
          vpuapi/vpuapifunc.c                                     \
          vpuapi/vpuapi.c                                         \
          vpuapi/coda9/coda9.c                                    \
          vpuapi/wave/wave5.c

ifeq ($(USE_RTL_SIM), yes)
	SOURCES += sample/main_sim.c
endif

VPATH  = sample_v2:
VPATH += sample_v2/component_encoder:
VPATH += sample_v2/component_decoder:
VPATH += sample_v2/helper:
VPATH += sample_v2/helper/bitstream:
VPATH += sample_v2/helper/comparator:
VPATH += sample_v2/helper/display:sample_v2/helper/misc:sample_v2/helper/yuv:sample_v2/component:
VPATH += vdi:
VPATH += $(VDI_VPATH):vpuapi:vpuapi/coda9:vpuapi/wave

OBJECTNAMES=$(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))
OBJECTPATHS=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES)))

LINT_SRC_INCLUDES = -I./sample_v2 -I./sample_v2/component -I./sample_v2/component_decoder -I./sample_v2/component_encoder
LINT_SRC_INCLUDES += -I./sample_v2/helper -I./sample_v2/helper/bitstream -I./sample_v2/helper/comparator -I./sample_v2/helper/display
LINT_SRC_INCLUDES += -I./sample_v2/helper/misc -I./sample_v2/helper/yuv

ifeq ($(USE_RTL_SIM), yes)
all: CREATE_DIR $(OBJECTPATHS)
else
all: CREATE_DIR $(OBJECTPATHS)
	$(LINKER) -fPIC -shared -o $(TARGET) $(LDFLAGS) -Wl,-gc-section -Wl,--start-group $(OBJECTPATHS) $(LDLIBS) -Wl,--end-group
endif

-include $(OBJECTPATHS:.o=.dep)

clean:
	$(RM) $(TARGET)
	$(RM) $(OBJDIR)/$(ALLOBJS)
	$(RM) $(OBJDIR)/$(ALLDEPS)

CREATE_DIR:
	-mkdir -p $(OBJDIR)

obj/%.o: %.c $(MAKEFILE)
	$(CC) -fPIC -shared $(CFLAGS) -Wall -Werror -c $< -o $@ -MD -MF $(@:.o=.dep)


lint:
	"$(LINT_HOME)/flint" -i"$(LINT_HOME)" $(DEFINES) $(INCLUDES) $(LINT_SRC_INCLUDES) linux_std.lnt $(HAPS_RULE) $(NONOS_RULE)  $(SOURCES)

