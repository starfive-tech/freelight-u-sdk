#==============================================================================
# Build the SD/flash image for StarFive VisionFive V1.
#==============================================================================

#visionfive_v1_uboot_all
#visionfive_v1_linux_all
#$(initramfs)

#visionfive_v1_uboot_build_tools
#make buildroot_rootfs -jx
#make DISK=/dev/sdX format-nvdla-rootfs && sync

#TF 卡分区信息：
VISIONFIVE_V1_BOOT_TYPE		= EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
VISIONFIVE_V1_ROOTFS_TYPE		= 0FC63DAF-8483-4772-8E79-3D69D8477DE4
VISIONFIVE_V1_BOOTLOADER_TYPE		= 5B193300-FC78-40CD-8002-E86C45580B47


#for debug
VISIONFIVE_V1_UENV_PATH=$(SDK_PREBUILD_DIR)/image$(CONFIG_BOARD_SUBFIX)/uEnv.txt
CONFIG_VISIONFIVE_V1_ROOTFS_IMAGE=$(SDK_PREBUILD_DIR)/image$(CONFIG_BOARD_SUBFIX)/test.ext4

CONFIG_VISIONFIVE_V1_BOOT_START=4096
CONFIG_VISIONFIVE_V1_BOOT_SECTORS=466943

CONFIG_VISIONFIVE_V1_BOOTLOADER_START=471040
CONFIG_VISIONFIVE_V1_BOOTLOADER_SECTORS=2047

CONFIG_VISIONFIVE_V1_ROOTFS_START=475136
CONFIG_VISIONFIVE_V1_ROOTFS_SECTORS=409600
#for debug

VISIONFIVE_V1_BOOT_END=$(shell expr \
	$(CONFIG_VISIONFIVE_V1_BOOT_START) + \
	$(CONFIG_VISIONFIVE_V1_BOOT_SECTORS))

VISIONFIVE_V1_BOOTLOADER_END=$(shell expr \
	$(CONFIG_VISIONFIVE_V1_BOOTLOADER_START) + \
	$(CONFIG_VISIONFIVE_V1_BOOTLOADER_SECTORS))

#VISIONFIVE_V1_ROOTFS_END=$(shell expr \
#	$(CONFIG_VISIONFIVE_V1_ROOTFS_START) + \
#	$(CONFIG_VISIONFIVE_V1_ROOTFS_SECTORS))

#0 means 'to the end of image'
VISIONFIVE_V1_ROOTFS_END=0

VISIONFIVE_V1_SD_IMAGE_SECTORS_NUM=$(shell expr \
	$(CONFIG_VISIONFIVE_V1_ROOTFS_START) + \
	$(CONFIG_VISIONFIVE_V1_ROOTFS_SECTORS))

#4K = 512 * 8
VISIONFIVE_V1_SD_IMAGE_BLK_SECTORS=8
VISIONFIVE_V1_SD_IMAGE_COUNT=$(shell expr \
	$(VISIONFIVE_V1_SD_IMAGE_SECTORS_NUM) / \
	$(VISIONFIVE_V1_SD_IMAGE_BLK_SECTORS) + \
	1)

VISIONFIVE_V1_SD_IMAGE_DIR=$(SDK_OUTPUT_DIR)/image$(CONFIG_BOARD_SUBFIX)
VISIONFIVE_V1_SD_IMAGE_FILE=flash$(CONFIG_BOARD_SUBFIX).img

visionfive_v1_create_image:
	mkdir -p $(VISIONFIVE_V1_SD_IMAGE_DIR)/
	dd iflag=fullblock bs=4K count=$(VISIONFIVE_V1_SD_IMAGE_COUNT) \
		if=/dev/zero \
		of=$(VISIONFIVE_V1_SD_IMAGE_DIR)/$(VISIONFIVE_V1_SD_IMAGE_FILE)
	tree -h --timefmt "%Y/%m/%d %H:%M:%S" $(VISIONFIVE_V1_SD_IMAGE_DIR)

visionfive_v1_format_image: visionfive_v1_create_image
	sgdisk --clear -g \
		--new=1:$(CONFIG_VISIONFIVE_V1_BOOT_START):$(VISIONFIVE_V1_BOOT_END) \
			--change-name=1:"Vfat Boot" \
			--typecode=1:$(VISIONFIVE_V1_BOOT_TYPE) \
		--new=2:$(CONFIG_VISIONFIVE_V1_BOOTLOADER_START):$(VISIONFIVE_V1_BOOTLOADER_END) \
			--change-name=2:uboot \
			--typecode=2:$(VISIONFIVE_V1_BOOTLOADER_TYPE) \
		--new=3:$(CONFIG_VISIONFIVE_V1_ROOTFS_START):$(VISIONFIVE_V1_ROOTFS_END) \
			--change-name=3:root \
			--typecode=3:$(VISIONFIVE_V1_ROOTFS_TYPE) \
		$(VISIONFIVE_V1_SD_IMAGE_DIR)/$(VISIONFIVE_V1_SD_IMAGE_FILE)

visionfive_v1_make_image:
	pushd  $(VISIONFIVE_V1_SD_IMAGE_DIR); \
	mkdir -p mnt;\
	LOOP_DEVS=`sudo kpartx -av $(VISIONFIVE_V1_SD_IMAGE_FILE) | awk '{print $$3}'`;\
	BOOT_LOOP_DEV=$${LOOP_DEVS%p1*}p1; \
	BL_LOOP_DEV=$${LOOP_DEVS%p1*}p2; \
	ROOT_LOOP_DEV=$${LOOP_DEVS%p1*}p3; \
	sudo mkfs.vfat /dev/mapper/$${BOOT_LOOP_DEV};\
	mkdir -p mnt/boot;\
	sudo mount /dev/mapper/$${BOOT_LOOP_DEV} mnt/boot; sleep 3; \
	sudo cp -rf $(SDK_OUTPUT_DIR)/$(CONFIG_CONFIG_FILE).fit \
	$(VISIONFIVE_V1_UENV_PATH) mnt/boot/ ;\
	tree -h --timefmt "%Y/%m/%d %H:%M:%S" mnt/boot/; \
	sync; sleep 3; sudo umount mnt/boot; \
	rm -rf mnt; \
	sudo dd if=$(join $(UBOOT_INSTALL_DIR), $(CONFIG_BOARD_SUBFIX))/u-boot.bin \
	of=/dev/mapper/$${BL_LOOP_DEV} bs=4K; sync; \
	sudo dd if=$(CONFIG_VISIONFIVE_V1_ROOTFS_IMAGE) \
	of=/dev/mapper/$${ROOT_LOOP_DEV} bs=4K; sync; \
	sudo resize2fs /dev/mapper/$${ROOT_LOOP_DEV}; \
	sudo fsck.ext4 /dev/mapper/$${ROOT_LOOP_DEV}; \
	sudo kpartx -d $(VISIONFIVE_V1_SD_IMAGE_FILE); sync; \
	sudo growpart $(VISIONFIVE_V1_SD_IMAGE_FILE) 3 ; \
	popd

