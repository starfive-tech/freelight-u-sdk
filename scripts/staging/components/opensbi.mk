# SPDX-License-Identifier: GPL-2.0

OPENSBI_NAME=opensbi

#OPENSBI_BUILD_DEPENDENT_PACKAGES=

OPENSBI_REPO_DIR=$(CONFIG_OPENSBI_REPO_PATH)

ifeq ($(CONFIG_OPENSBI_BUILD_IN_TMPFS),y)
OPENSBI_BUILD_IN_TMPFS=/tempfs
endif

OPENSBI_SRC_DIR=$(SDK_BUILD_DIR)$(OPENSBI_BUILD_IN_TMPFS)/firmware/$(OPENSBI_NAME)
OPENSBI_INSTALL_DIR=$(SDK_OUTPUT_DIR)/firmware/$(OPENSBI_NAME)

opensbi_update:
	@$(call fetch_remote, $(OPENSBI_REPO_DIR), $(CONFIG_OPENSBI_GIT_REMOTE))

#==============================================================================
# Build the U-Boot for some board.
#==============================================================================

_BOARD_OPENSBI_SRC_DIR=$(join $(OPENSBI_SRC_DIR), $(CONFIG_BOARD_SUBFIX))
_BOARD_OPENSBI_INSTALL_DIR=$(join $(OPENSBI_INSTALL_DIR), $(CONFIG_BOARD_SUBFIX))/$(SDK_TIMESTAMP_TEMPLATE_1)

#_BOARD_OPENSBI_BIN_PATH=$(_BOARD_OPENSBI_INSTALL_DIR)/u-boot.bin
#_BOARD_OPENSBI_DTB_PATH=$(_BOARD_OPENSBI_INSTALL_DIR)/u-boot.dtb
#				  FW_PAYLOAD_PATH=$(CONFIG_OPENSBI_PREBUILD_UBOOT_BIN) \
#				  FW_FDT_PATH=$(CONFIG_OPENSBI_PREBUILD_DTB)
#_board_opensbi_update:
#	@$(call fetch_remote, $(OPENSBI_REPO_DIR), $(CONFIG_OPENSBI_GIT_REMOTE))

ifneq ($(CONFIG_OPENSBI_BUILD_ARGS),"")
_BOARD_OPENSBI_BUILD_ARGS=$(shell echo $(CONFIG_OPENSBI_BUILD_ARGS))
endif

ifneq ($(CONFIG_OPENSBI_PREBUILD_UBOOT_BIN),"")
_BOARD_OPENSBI_BUILD_ARGS += FW_PAYLOAD_PATH=$(shell echo $(CONFIG_OPENSBI_PREBUILD_UBOOT_BIN))
endif

ifneq ($(CONFIG_OPENSBI_PREBUILD_DTB),"")
_BOARD_OPENSBI_BUILD_ARGS += FW_FDT_PATH=$(shell echo $(CONFIG_OPENSBI_PREBUILD_DTB))
else

ifeq ($(CONFIG_OPENSBI_PLATFORM_GENERIC),y)
$(warning Missing dtb file for building generic platform)
endif

endif

_board_opensbi_checkout_with_check:
	OVERWRITED_CHK=Y;\
	$(call checkout_src_full, $(OPENSBI_REPO_DIR),\
				  $(CONFIG_OPENSBI_GIT_REMOTE) \
				  $(CONFIG_OPENSBI_GIT_RBRANCH), \
				  $(CONFIG_OPENSBI_GIT_LBRANCH), \
				  $(_BOARD_OPENSBI_SRC_DIR),)

_board_opensbi_checkout:
	$(call checkout_src_full, $(OPENSBI_REPO_DIR),\
				  $(CONFIG_OPENSBI_GIT_REMOTE) \
				  $(CONFIG_OPENSBI_GIT_RBRANCH), \
				  $(CONFIG_OPENSBI_GIT_LBRANCH), \
				  $(_BOARD_OPENSBI_SRC_DIR),)

_board_opensbi_devel:
	$(call devel_init, $(_BOARD_OPENSBI_SRC_DIR))

_board_opensbi_build_silent:
	SILENT_BUILD=Y; \
	$(call build_target_log, $(_BOARD_OPENSBI_SRC_DIR), \
				  $(CROSS_COMPILE_RV64_ENV) \
				  PLATFORM=$(shell echo $(CONFIG_OPENSBI_BUILD_PLATFORM)) \
				  $(_BOARD_OPENSBI_BUILD_ARGS), )
				  
_board_opensbi_build:
	$(call build_target_log, $(_BOARD_OPENSBI_SRC_DIR), \
				  $(CROSS_COMPILE_RV64_ENV) \
				  PLATFORM=$(shell echo $(CONFIG_OPENSBI_BUILD_PLATFORM)) \
				  $(_BOARD_OPENSBI_BUILD_ARGS), )

_board_opensbi_install:
	$(call install_bin, \
		$(_BOARD_OPENSBI_SRC_DIR)/build/platform/$(CONFIG_OPENSBI_BUILD_PLATFORM)/firmware/fw_*.bin, \
		$(_BOARD_OPENSBI_INSTALL_DIR))

_board_opensbi_all: _board_opensbi_checkout _board_opensbi_build _board_opensbi_install
