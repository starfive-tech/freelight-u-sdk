UBOOT_NAME=u-boot

UBOOT_BUILD_DEPENDENT_PACKAGES=dtc

UBOOT_REPO_DIR=$(CONFIG_UBOOT_REPO_PATH)

ifeq ($(CONFIG_UBOOT_BUILD_IN_TMPFS),y)
UBOOT_BUILD_IN_TMPFS=/tempfs
endif

UBOOT_SRC_DIR=$(SDK_BUILD_DIR)$(UBOOT_BUILD_IN_TMPFS)/firmware/$(UBOOT_NAME)
UBOOT_INSTALL_DIR=$(SDK_OUTPUT_DIR)/firmware/$(UBOOT_NAME)

uboot_update:
	@$(call fetch_remote, $(UBOOT_REPO_DIR), $(CONFIG_UBOOT_GIT_REMOTE))

#==============================================================================
# Build the U-Boot for some board.
#==============================================================================

_BOARD_UBOOT_SRC_DIR=$(join $(UBOOT_SRC_DIR), $(CONFIG_BOARD_SUBFIX))
_BOARD_UBOOT_INSTALL_DIR=$(join $(UBOOT_INSTALL_DIR), $(CONFIG_BOARD_SUBFIX))

_BOARD_UBOOT_BIN_PATH=$(_BOARD_UBOOT_INSTALL_DIR)/u-boot.bin
_BOARD_UBOOT_DTB_PATH=$(_BOARD_UBOOT_INSTALL_DIR)/u-boot.dtb

#_board_uboot_update:
#	@$(call fetch_remote, $(UBOOT_REPO_DIR), $(CONFIG_UBOOT_GIT_REMOTE))

_board_uboot_checkout_with_check:
	OVERWRITED_CHK=Y;\
	$(call checkout_src_full, $(UBOOT_REPO_DIR),\
				  $(CONFIG_UBOOT_GIT_REMOTE) \
				  $(CONFIG_UBOOT_GIT_RBRANCH), \
				  $(CONFIG_UBOOT_GIT_LBRANCH), \
				  $(_BOARD_UBOOT_SRC_DIR),)

_board_uboot_checkout:
	$(call checkout_src_full, $(UBOOT_REPO_DIR),\
				  $(CONFIG_UBOOT_GIT_REMOTE) \
				  $(CONFIG_UBOOT_GIT_RBRANCH), \
				  $(CONFIG_UBOOT_GIT_LBRANCH), \
				  $(_BOARD_UBOOT_SRC_DIR),)

_board_uboot_devel:
	$(call devel_init, $(_BOARD_UBOOT_SRC_DIR))

_board_uboot_defconfig:
	$(call build_target, $(_BOARD_UBOOT_SRC_DIR), \
			      $(CROSS_COMPILE_RV64_ENV), \
			      $(CONFIG_UBOOT_BUILD_TARGET)_defconfig)

_board_uboot_menuconfig:
	$(call build_target, $(_BOARD_UBOOT_SRC_DIR), \
			      $(CROSS_COMPILE_RV64_ENV), \
			      menuconfig)

_board_uboot_mkdefconfig:
	$(call build_target, $(_BOARD_UBOOT_SRC_DIR), \
			      $(CROSS_COMPILE_RV64_ENV), \
			      savedefconfig)
#	cd $(_BOARD_UBOOT_SRC_DIR); \
#	mv defconfig arch/riscv/configs/starfive_jh7100_starlight_fedora_defconfig


_board_uboot_build_dtb:
	$(call build_target, $(_BOARD_UBOOT_SRC_DIR), \
			      $(CROSS_COMPILE_RV64_ENV), \
			      u-boot.dtb)

_board_uboot_install_dtb:
	$(call install_bin, \
		$(_BOARD_UBOOT_SRC_DIR)/u-boot.dtb, \
		$(_BOARD_UBOOT_INSTALL_DIR))

_board_uboot_build_silent:
	SILENT_BUILD=Y; \
	$(call build_target_log, $(_BOARD_UBOOT_SRC_DIR), \
				  $(CROSS_COMPILE_RV64_ENV), \
				  tools u-boot.bin u-boot.dtb)

_board_uboot_build:
	$(call build_target_log, $(_BOARD_UBOOT_SRC_DIR), \
				  $(CROSS_COMPILE_RV64_ENV), \
				  u-boot.bin u-boot.dtb)

_board_uboot_build_tools:
	$(call build_target_log, $(_BOARD_UBOOT_SRC_DIR), \
				  $(CROSS_COMPILE_RV64_ENV), \
				  tools)

_board_uboot_clean:
	$(call build_target, $(_BOARD_UBOOT_SRC_DIR), \
			       $(CROSS_COMPILE_RV64_ENV), \
			       clean)

_board_uboot_install:
	$(call install_bin, \
		$(_BOARD_UBOOT_SRC_DIR)/u-boot*, \
		$(_BOARD_UBOOT_INSTALL_DIR))

_board_uboot_all: _board_uboot_checkout _board_uboot_defconfig _board_uboot_menuconfig _board_uboot_build _board_uboot_install
# _board_opensbi_checkout _board_opensbi_build _board_opensbi_install


