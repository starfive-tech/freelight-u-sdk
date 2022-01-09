# SPDX-License-Identifier: GPL-2.0

LINUX_NAME=linux

LINUX_REPO_DIR=$(CONFIG_LINUX_REPO_PATH)

ifeq ($(CONFIG_LINUX_INSTALL_SUDO),y)
_INSTALL_SUDO=sudo
endif

ifeq ($(CONFIG_LINUX_BUILD_IN_TMPFS),y)
LINUX_BUILD_IN_TMPFS=/tempfs
endif

LINUX_SRC_DIR=$(SDK_BUILD_DIR)$(LINUX_BUILD_IN_TMPFS)/kernel/$(LINUX_NAME)
LINUX_INSTALL_DIR=$(SDK_OUTPUT_DIR)/kernel/$(LINUX_NAME)

linux_update:
	@$(call fetch_remote, $(LINUX_REPO_DIR), $(CONFIG_LINUX_GIT_REMOTE))

#==============================================================================
# Build the Linux kernel for some board.
#==============================================================================

_BOARD_LINUX_SRC_DIR=$(join $(LINUX_SRC_DIR), $(CONFIG_BOARD_SUBFIX))
_BOARD_LINUX_INSTALL_DIR=$(join $(LINUX_INSTALL_DIR), $(CONFIG_BOARD_SUBFIX))


#$(1) linux src dir
#$(2) build_env
#$(3) des rootfs
define linux_install_modules
	pushd $(1); \
	$(_INSTALL_SUDO) mkdir -p $(3)/; \
	$(_INSTALL_SUDO) $(2) $(MAKE) $(join INSTALL_MOD_PATH=, $(3)/) modules_install; \
	popd; \
	sync
endef

#$(1) linux src dir
#$(2) build_env
#$(3) des rootfs
define linux_install_zimage
	pushd $(1); \
	$(_INSTALL_SUDO) mkdir -p $(3)/boot; \
	$(_INSTALL_SUDO) $(2) $(MAKE) $(join INSTALL_PATH=, $(3)/boot) zinstall;  \
	popd; \
	sync
endef

#$(1) linux src dir
#$(2) build_env
#$(3) des rootfs
#$(4) kernel version string
define linux_install_dtbs
	pushd $(1); \
	$(_INSTALL_SUDO) mkdir -p $(3)/boot/dtb$(4); \
	$(_INSTALL_SUDO) $(2) $(MAKE) $(join INSTALL_DTBS_PATH=, $(3)/boot/dtb$(4)) dtbs_install;\
	popd; \
	sync
endef

_board_linux_checkout_with_check:
	OVERWRITED_CHK=Y;\
	$(call checkout_src_full, $(LINUX_REPO_DIR),\
				  $(CONFIG_LINUX_GIT_REMOTE) \
				  $(CONFIG_LINUX_GIT_RBRANCH), \
				  $(CONFIG_LINUX_GIT_LBRANCH), \
				  $(_BOARD_LINUX_SRC_DIR),)

_board_linux_checkout:
	$(call checkout_src_full, $(LINUX_REPO_DIR),\
				  $(CONFIG_LINUX_GIT_REMOTE) \
				  $(CONFIG_LINUX_GIT_RBRANCH), \
				  $(CONFIG_LINUX_GIT_LBRANCH), \
				  $(_BOARD_LINUX_SRC_DIR),)

_board_linux_devel:
	$(call devel_init, $(_BOARD_LINUX_SRC_DIR))

_board_linux_defconfig:
	$(call build_target, $(_BOARD_LINUX_SRC_DIR), \
			      $(CROSS_COMPILE_RV64_ENV), \
			      $(CONFIG_LINUX_BUILD_TARGET)_defconfig)

_board_linux_menuconfig:
	$(call build_target, $(_BOARD_LINUX_SRC_DIR), \
			      $(CROSS_COMPILE_RV64_ENV), \
			      menuconfig)

_board_linux_mkdefconfig:
	$(call build_target, $(_BOARD_LINUX_SRC_DIR), \
			      $(CROSS_COMPILE_RV64_ENV), \
			      savedefconfig)
#	cd $(_BOARD_LINUX_SRC_DIR); \
#	mv defconfig arch/riscv/configs/starfive_jh7100_visionfive_fedora_defconfig


_board_linux_build_dtbs:
	$(call build_target, $(_BOARD_LINUX_SRC_DIR), \
			      $(CROSS_COMPILE_RV64_ENV), \
			      dtbs)

_board_linux_install_dtbs:
	$(call linux_install_dtbs, \
		$(_BOARD_LINUX_SRC_DIR), \
		$(CROSS_COMPILE_RV64_ENV), \
		$(_BOARD_LINUX_INSTALL_DIR))

_board_linux_build_all_silent:
	SILENT_BUILD=Y; \
	$(call build_target_log, $(_BOARD_LINUX_SRC_DIR), \
				  $(CROSS_COMPILE_RV64_ENV),)

_board_linux_build_all:
	$(call build_target_log, $(_BOARD_LINUX_SRC_DIR), \
				  $(CROSS_COMPILE_RV64_ENV),)

_board_linux_clean:
	$(call build_target, $(_BOARD_LINUX_SRC_DIR), \
			       $(CROSS_COMPILE_RV64_ENV), \
			       clean)

_board_linux_install_zimage:
	$(call linux_install_zimage, \
		$(_BOARD_LINUX_SRC_DIR), \
		$(CROSS_COMPILE_RV64_ENV), \
		$(_BOARD_LINUX_INSTALL_DIR))

_board_linux_install_modules:
	$(call linux_install_modules, \
		$(_BOARD_LINUX_SRC_DIR), \
		$(CROSS_COMPILE_RV64_ENV), \
		$(_BOARD_LINUX_INSTALL_DIR))

_board_linux_install_all: _board_linux_install_zimage _board_linux_install_modules _board_linux_install_dtbs

_board_linux_all: _board_linux_checkout _board_linux_defconfig _board_linux_menuconfig _board_linux_build_all _board_linux_install_all


