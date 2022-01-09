#==============================================================================
# Build the U-Boot for StarFive VisionFive V1.
#==============================================================================

visionfive_v1_uboot_update: uboot_update
visionfive_v1_uboot_checkout: _board_uboot_checkout_with_check
visionfive_v1_uboot_checkout_force: _board_uboot_checkout
visionfive_v1_uboot_devel: _board_uboot_devel
visionfive_v1_uboot_defconfig: _board_uboot_defconfig
visionfive_v1_uboot_menuconfig: _board_uboot_menuconfig
visionfive_v1_uboot_mkdefconfig: _board_uboot_mkdefconfig
visionfive_v1_uboot_build_dtb: _board_uboot_build_dtb
visionfive_v1_uboot_install_dtb: _board_uboot_install_dtb
visionfive_v1_uboot_build: _board_uboot_build
visionfive_v1_uboot_build_silent: _board_uboot_build_silent
visionfive_v1_uboot_build_tools: _board_uboot_build_tools
visionfive_v1_uboot_clean: _board_uboot_clean
visionfive_v1_uboot_install: _board_uboot_install
visionfive_v1_uboot_all: visionfive_v1_uboot_checkout visionfive_v1_uboot_defconfig visionfive_v1_uboot_menuconfig visionfive_v1_uboot_build visionfive_v1_uboot_install visionfive_v1_opensbi_checkout visionfive_v1_opensbi_build visionfive_v1_opensbi_install
visionfive_v1_uboot_all_silent: visionfive_v1_uboot_checkout_force visionfive_v1_uboot_defconfig visionfive_v1_uboot_build_silent visionfive_v1_uboot_install visionfive_v1_opensbi_all_silent

