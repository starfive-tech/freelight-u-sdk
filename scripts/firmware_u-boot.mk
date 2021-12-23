.PHONY: uboot
uboot: $(uboot)

$(uboot_wrkdir)/.config: $(uboot_defconfig)
	mkdir -p $(dir $@)
	cp -p $< $@

.PHONY: uboot-menuconfig
uboot-menuconfig: $(uboot_wrkdir)/.config
	$(MAKE) -C $(uboot_srcdir) O=$(dir $<) ARCH=riscv CROSS_COMPILE=$(CROSS_COMPILE) menuconfig
	cp $(uboot_wrkdir)/.config $(uboot_defconfig)

$(uboot): $(uboot_srcdir) $(target_gcc)
	rm -rf $(uboot_wrkdir)
	mkdir -p $(uboot_wrkdir)
	mkdir -p $(dir $@)
	$(MAKE) -C $(uboot_srcdir) O=$(uboot_wrkdir) CROSS_COMPILE=$(CROSS_COMPILE) $(uboot_config)
	$(MAKE) -C $(uboot_srcdir) O=$(uboot_wrkdir) CROSS_COMPILE=$(CROSS_COMPILE)
