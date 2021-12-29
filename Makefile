sdkdir := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))
sdkdir := $(sdkdir:/=)

MAKEFILE_DIR=$(sdkdir)/scripts

# For SDK build ENV, it should be the first.
include $(MAKEFILE_DIR)/sdk_env.mk

.PHONY: all nvdla-demo
nvdla-demo: $(fit) $(vfat_image)
	@echo "To completely erase, reformat, and program a disk sdX, run:"
	@echo "  make DISK=/dev/sdX format-nvdla-disk"
	@echo "  ... you will need gdisk and e2fsprogs installed"
	@echo "  Please note this will not currently format the SDcard ext4 partition"
	@echo "  This can be done manually if needed"
	@echo

all: $(fit) $(flash_image)
	@echo
	@echo "This image has been generated for an ISA of $(ISA) and an ABI of $(ABI)"
	@echo "Find the image in work/image.fit, which should be copied to an MSDOS boot partition 1"
	@echo
	@echo "To completely erase, reformat, and program a disk sdX, run:"
	@echo "  make DISK=/dev/sdX format-boot-loader"
	@echo "  ... you will need gdisk and e2fsprogs installed"
	@echo "  Please note this will not currently format the SDcard ext4 partition"
	@echo "  This can be done manually if needed"
	@echo

$(fit): sbi $(vmlinux_bin) $(uboot) $(its_file) $(initramfs)
	$(uboot_wrkdir)/tools/mkimage -f $(its_file) -A riscv -O linux -T flat_dt $@
	@if [ -f fsz.sh ]; then ./fsz.sh $(sbi_bin); fi

.PHONY: buildroot_initramfs_sysroot vmlinux bbl fit
buildroot_initramfs_sysroot: $(buildroot_initramfs_sysroot)
vmlinux: $(vmlinux)
fit: $(fit)

.PHONY: clean
clean:
	rm -rf work/u-boot
	rm -rf work/opensbi
	rm work/vmlinux.bin
	rm work/hifive-unleashed-vfat.part
	rm work/image.fit
	rm work/initramfs.cpio.gz
	rm work/linux/vmlinux
ifeq ($(buildroot_rootfs_ext),$(wildcard $(buildroot_rootfs_ext)))
	rm work/buildroot_rootfs/images/rootfs.ext4
endif
ifeq ($(buildroot_initramfs_tar),$(wildcard $(buildroot_initramfs_tar)))
	rm work/buildroot_initramfs/images/rootfs.tar
endif

.PHONY: distclean
distclean:
	rm -rf -- $(wrkdir) $(toolchain_dest)

include $(MAKEFILE_DIR)/distro_buildroot.mk
include $(MAKEFILE_DIR)/distro_buildroot_image.mk

include $(MAKEFILE_DIR)/firmware_opensbi.mk
include $(MAKEFILE_DIR)/firmware_u-boot.mk

include $(MAKEFILE_DIR)/kernel_linux.mk

-include $(initramfs).d

include $(MAKEFILE_DIR)/virtual_test.mk

#-include $(MAKEFILE_DIR)/debug.mk
