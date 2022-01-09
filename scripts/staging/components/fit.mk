#==============================================================================
# Build the FIT image for StarFive.
#==============================================================================

FIT_ITS_TEMPLATE=$(SDK_STAGING_CONFIGS_DIR)/uboot_fit_image$(CONFIG_BOARD_SUBFIX)_template.its
FIT_ITS_FILE=$(SDK_STAGING_CONFIGS_DIR)/uboot_fit_image$(CONFIG_BOARD_SUBFIX).its

_board_fit_its:
	$(call fit_its_gen, $(CONFIG_FILE), \
			     $(FIT_ITS_TEMPLATE), \
			     $(FIT_ITS_FILE))

#_board_uboot_build_tools
_board_fit: _board_fit_its
	$(_BOARD_UBOOT_SRC_DIR)/tools/mkimage \
		-f $(FIT_ITS_FILE) \
		-A riscv \
		-O linux \
		-T flat_dt \
		$(SDK_OUTPUT_DIR)/$(CONFIG_CONFIG_FILE).fit

