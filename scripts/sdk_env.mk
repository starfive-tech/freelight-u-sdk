ISA ?= rv64imafdc
ABI ?= lp64d
TARGET_BOARD := U74
BOARD_FLAGS	:=

confdir := $(sdkdir)/conf
srcdir := $(sdkdir)/src
wrkdir := $(CURDIR)/work

buildroot_srcdir := $(srcdir)/distros/buildroot
buildroot_initramfs_wrkdir := $(wrkdir)/buildroot_initramfs

# TODO: make RISCV be able to be set to alternate toolchain path
RISCV ?= $(buildroot_initramfs_wrkdir)/host
RVPATH ?= $(RISCV)/bin:/usr/sbin:/sbin:$(PATH)
target ?= riscv64-buildroot-linux-gnu

CROSS_COMPILE ?= $(RISCV)/bin/$(target)-

buildroot_initramfs_tar := $(buildroot_initramfs_wrkdir)/images/rootfs.tar
buildroot_initramfs_config := $(confdir)/buildroot_initramfs_config
buildroot_initramfs_sysroot_stamp := $(wrkdir)/.buildroot_initramfs_sysroot
buildroot_initramfs_sysroot := $(wrkdir)/buildroot_initramfs_sysroot
buildroot_rootfs_wrkdir := $(wrkdir)/buildroot_rootfs
buildroot_rootfs_ext := $(buildroot_rootfs_wrkdir)/images/rootfs.ext4
buildroot_rootfs_config := $(confdir)/buildroot_rootfs_config

linux_srcdir := $(srcdir)/kernel/linux
linux_wrkdir := $(wrkdir)/linux
linux_defconfig := $(confdir)/beaglev_defconfig_513

vmlinux := $(linux_wrkdir)/vmlinux
vmlinux_stripped := $(linux_wrkdir)/vmlinux-stripped
vmlinux_bin := $(wrkdir)/vmlinux.bin

ifeq ($(TARGET_BOARD),U74)
export TARGET_BOARD
	BOARD_FLAGS += -DTARGET_BOARD_U74
	bbl_link_addr :=0x80700000
	its_file=$(confdir)/u74_nvdla-uboot-fit-image.its
else
	bbl_link_addr :=0x80000000
	its_file=$(confdir)/nvdla-uboot-fit-image.its
endif

flash_image := $(wrkdir)/hifive-unleashed-a00-YYYY-MM-DD.gpt
vfat_image := $(wrkdir)/hifive-unleashed-vfat.part
#ext_image := $(wrkdir)  # TODO

initramfs := $(wrkdir)/initramfs.cpio.gz

sbi_srcdir := $(srcdir)/firmware/opensbi
sbi_wrkdir := $(wrkdir)/opensbi
#sbi_bin := $(wrkdir)/opensbi/platform/starfive/vic7100/firmware/fw_payload.bin
sbi_bin := $(wrkdir)/opensbi/platform/generic/firmware/fw_payload.bin
fit := $(wrkdir)/image.fit

#fesvr_srcdir := $(srcdir)/riscv-fesvr
#fesvr_wrkdir := $(wrkdir)/riscv-fesvr
#libfesvr := $(fesvr_wrkdir)/prefix/lib/libfesvr.so

#spike_srcdir := $(srcdir)/riscv-isa-sim
#spike_wrkdir := $(wrkdir)/riscv-isa-sim
#spike := $(spike_wrkdir)/prefix/bin/spike

#qemu_srcdir := $(srcdir)/riscv-qemu
#qemu_wrkdir := $(wrkdir)/riscv-qemu
#qemu := $(qemu_wrkdir)/prefix/bin/qemu-system-riscv64

uboot_srcdir := $(srcdir)/firmware/u-boot
uboot_wrkdir := $(wrkdir)/u-boot
uboot_dtb_file := $(wrkdir)/u-boot/arch/riscv/dts/jh7100-beaglev-starlight.dtb
uboot := $(uboot_wrkdir)/u-boot.bin
uboot_config := HiFive-U540_regression_defconfig

ifeq ($(TARGET_BOARD),U74)
	uboot_config := starfive_jh7100_starlight_smode_defconfig
else
	uboot_config := HiFive-U540_nvdla_iofpga_defconfig
endif

uboot_defconfig := $(uboot_srcdir)/configs/$(uboot_config)
rootfs := $(wrkdir)/rootfs.bin

target_gcc ?= $(CROSS_COMPILE)gcc
