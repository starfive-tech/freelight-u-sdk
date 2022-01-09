# SPDX-License-Identifier: GPL-2.0

SDK_INSTALL_DIR=$(sdkdir)
REPO_DIR=$(sdkdir)/src
 
SDK_BUILD_DIR=$(SDK_INSTALL_DIR)/work
#SDK_BIN_DIR=$(SDK_INSTALL_DIR)/bin
SDK_ETC_DIR=$(SDK_INSTALL_DIR)/conf
SDK_LOG_DIR=$(SDK_INSTALL_DIR)/logs
SDK_OUTPUT_DIR=$(SDK_INSTALL_DIR)/output
SDK_PREBUILD_DIR=$(SDK_INSTALL_DIR)/prebuild

#SDK_IMAGES_DIR=$(SDK_PREBUILD_DIR)/images
#SDK_TEMPLATES_DIR=$(SDK_INSTALL_DIR)/templates

MK_DIR=$(MAKEFILE_DIR)/staging

MK_ENV_DIR=$(MK_DIR)/env
MK_COMPONENTS_DIR=$(MK_DIR)/components
MK_BOARDS_DIR=$(MK_DIR)/boards
MK_DISTROS_DIR=$(MK_DIR)/distros

KCONDIFLIB_DIR=$(MK_DIR)/Kconfiglib

SDK_STAGING_CONFIGS_DIR=$(SDK_ETC_DIR)/staging

MAIN_CONFIG=$(SDK_STAGING_CONFIGS_DIR)/main.config

## main config file
-include $(MAIN_CONFIG)
## make sure we are using bash for "make"
SHELL = /bin/bash

## RISC-V dev boards
ifeq ($(CONFIG_VISIONFIVE_V1),y)
BOARD_NAME=visionfive_v1
endif
#ifeq ($(CONFIG_VISIONFIVE_Vx),y)
#BOARD_NAME=visionfive_vx
#endif

## load config file for a specific  development board
CONFIG_FILE=$(SDK_STAGING_CONFIGS_DIR)/$(shell echo $(CONFIG_CONFIG_FILE).config)
-include $(CONFIG_FILE)

include $(MK_DIR)/Rlue.mk
include $(MK_DIR)/common.mk

## Components
include $(MK_COMPONENTS_DIR)/opensbi.mk
include $(MK_COMPONENTS_DIR)/u-boot.mk
include $(MK_COMPONENTS_DIR)/linux.mk
include $(MK_COMPONENTS_DIR)/fit.mk

main_setup:
	export KCONFIG_CONFIG=$(MAIN_CONFIG); \
	export MENUCONFIG_STYLE="aquatic selection=fg:red,bg:green"; \
	$(KCONDIFLIB_DIR)/menuconfig.py $(MK_DIR)/Kconfig

ifneq ($(BOARD_NAME),"")
include $(MK_BOARDS_DIR)/$(BOARD_NAME)/Makefile
else
$(warning Missing board configuration, please run 'make main_setup' first!)
endif

$(BOARD_NAME)_config:
	export KCONFIG_CONFIG=$(CONFIG_FILE); \
	$(KCONDIFLIB_DIR)/menuconfig.py $(MK_BOARDS_DIR)/$(BOARD_NAME)/Kconfig

PHONY += FORCE
FORCE:

# Declare the contents of the .PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
.PHONY: $(PHONY)
