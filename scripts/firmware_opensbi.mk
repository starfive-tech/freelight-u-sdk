sbi: $(uboot) $(vmlinux)
	rm -rf $(sbi_wrkdir)
	mkdir -p $(sbi_wrkdir)
	cd $(sbi_wrkdir) && O=$(sbi_wrkdir) CFLAGS="-mabi=$(ABI) -march=$(ISA)" ${MAKE} -C $(sbi_srcdir) CROSS_COMPILE=$(CROSS_COMPILE) \
		PLATFORM=generic FW_PAYLOAD_PATH=$(uboot) FW_FDT_PATH=$(uboot_dtb_file)
