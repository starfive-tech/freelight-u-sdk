# SPDX-License-Identifier: GPL-2.0

#CROSS_COMPILE_RV64_ENV_STARFIVE=$(CROSS_COMPILE_RV64_ENV)

#BOARD_BUILD_DEPENDENT_PACKAGES=

include $(MK_BOARDS_DIR)/$(BOARD_NAME)/u-boot.mk
include $(MK_BOARDS_DIR)/$(BOARD_NAME)/opensbi.mk
include $(MK_BOARDS_DIR)/$(BOARD_NAME)/linux.mk

include $(MK_BOARDS_DIR)/$(BOARD_NAME)/fit.mk

include $(MK_BOARDS_DIR)/$(BOARD_NAME)/image.mk

visionfive_v1_all:
	@_EXISTED_DIR=''; \
	if [ -d $(_BOARD_OPENSBI_SRC_DIR) ]; then \
		_EXISTED_DIR+=$(_BOARD_OPENSBI_SRC_DIR); \
		_EXISTED_DIR+="\n"; \
	fi; \
	if [ -d $(_BOARD_UBOOT_SRC_DIR) ]; then \
		_EXISTED_DIR+=$(_BOARD_UBOOT_SRC_DIR); \
		_EXISTED_DIR+="\n"; \
	fi; \
	if [ -d $(_BOARD_LINUX_SRC_DIR) ]; then \
		_EXISTED_DIR+=$(_BOARD_LINUX_SRC_DIR); \
		_EXISTED_DIR+="\n"; \
	fi; \
	if [ "X$$_EXISTED_DIR" != "X" ]; then \
		whiptail \
		--title "!!!Warning: Work dir will be OVERWRITED!!!" \
		--yesno "Work dir existed: $${_EXISTED_DIR} \n\
		\nDo you really want to OVERWRITED it/them?\
		\nPlease make sure that you have backed up your work!" \
		20 60 || exit 0;\
	fi; \
	make visionfive_v1_all_silent

visionfive_v1_all_silent: visionfive_v1_uboot_all_silent visionfive_v1_linux_all_silent visionfive_v1_fit
