# SPDX-License-Identifier: GPL-2.0
#==============================================================================
# Build the SD/flash image for StarFive VisionFive V1.
#==============================================================================
#TF 卡分区信息：
VISIONFIVE_V1_BOOT_TYPE			= EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
VISIONFIVE_V1_ROOTFS_TYPE		= 0FC63DAF-8483-4772-8E79-3D69D8477DE4
VISIONFIVE_V1_BOOTLOADER_TYPE		= 5B193300-FC78-40CD-8002-E86C45580B47

VISIONFIVE_V1_SD_IMAGE_DIR=$(SDK_OUTPUT_DIR)/image$(CONFIG_BOARD_SUBFIX)
VISIONFIVE_V1_SD_IMAGE_FILE=flash$(CONFIG_BOARD_SUBFIX).img

#for debug
#CONFIG_VISIONFIVE_V1_UENV_PATH=$(SDK_PREBUILD_DIR)/image$(CONFIG_BOARD_SUBFIX)/uEnv.txt
#CONFIG_VISIONFIVE_V1_ROOTFS_IMAGE_PATH=$(SDK_PREBUILD_DIR)/image$(CONFIG_BOARD_SUBFIX)/test.ext4

#CONFIG_VISIONFIVE_V1_BOOT_START=4096
#CONFIG_VISIONFIVE_V1_BOOT_SECTORS=466943

#CONFIG_VISIONFIVE_V1_BOOTLOADER_START=471040
#CONFIG_VISIONFIVE_V1_BOOTLOADER_SECTORS=2047

#CONFIG_VISIONFIVE_V1_ROOTFS_START=475136
#CONFIG_VISIONFIVE_V1_ROOTFS_SECTORS=409600
#for debug

__VISIONFIVE_V1_ROOTFS_IMAGE_PATH=$(shell cd $(VISIONFIVE_V1_SD_IMAGE_DIR); realpath $(CONFIG_VISIONFIVE_V1_ROOTFS_IMAGE_PATH))

VISIONFIVE_V1_ROOTFS_IMAGE_SIZE=$(shell \
	expr `wc -c $(__VISIONFIVE_V1_ROOTFS_IMAGE_PATH) | awk '{print $$1}'` + 4096)

VISIONFIVE_V1_ROOTFS_SECTORS=$(shell expr $(VISIONFIVE_V1_ROOTFS_IMAGE_SIZE) / 4096 \* 8)

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
VISIONFIVE_V1_SD_ROOTFS_PART_NUM=3

VISIONFIVE_V1_SD_IMAGE_SECTORS_NUM=$(shell expr \
	$(CONFIG_VISIONFIVE_V1_ROOTFS_START) + \
	$(VISIONFIVE_V1_ROOTFS_SECTORS))

#4K = 512 * 8
VISIONFIVE_V1_SD_IMAGE_BLK_SECTORS=8
VISIONFIVE_V1_SD_IMAGE_COUNT=$(shell expr \
	$(VISIONFIVE_V1_SD_IMAGE_SECTORS_NUM) / \
	$(VISIONFIVE_V1_SD_IMAGE_BLK_SECTORS) + \
	5)

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

visionfive_v1_make_image: visionfive_v1_format_image
	pushd  $(VISIONFIVE_V1_SD_IMAGE_DIR); \
	mkdir -p mnt;\
	LOOP_DEVS=`sudo kpartx -av $(VISIONFIVE_V1_SD_IMAGE_FILE) | awk 'NR==1' | awk '{print $$3}'`;\
	BOOT_LOOP_DEV=$${LOOP_DEVS%p1*}p1; \
	BL_LOOP_DEV=$${LOOP_DEVS%p1*}p2; \
	ROOT_LOOP_DEV=$${LOOP_DEVS%p1*}p3; \
	sudo mkfs.vfat /dev/mapper/$${BOOT_LOOP_DEV};\
	mkdir -p mnt/boot;\
	sudo mount /dev/mapper/$${BOOT_LOOP_DEV} mnt/boot; sleep 3; \
	sudo cp -rf $(SDK_OUTPUT_DIR)/$(CONFIG_CONFIG_FILE).fit \
	$(CONFIG_VISIONFIVE_V1_UENV_PATH) mnt/boot/ ;\
	tree -h --timefmt "%Y/%m/%d %H:%M:%S" mnt/boot/; \
	sync; sleep 3; sudo umount mnt/boot; rm -rf mnt; \
	sudo dd if=$(join $(UBOOT_INSTALL_DIR), $(CONFIG_BOARD_SUBFIX))/u-boot.bin \
	of=/dev/mapper/$${BL_LOOP_DEV} bs=4K; sync; \
	sudo dd if=$(__VISIONFIVE_V1_ROOTFS_IMAGE_PATH) \
	of=/dev/mapper/$${ROOT_LOOP_DEV} bs=4K; sync; sleep 3;\
	sudo kpartx -d $(VISIONFIVE_V1_SD_IMAGE_DIR)/$(VISIONFIVE_V1_SD_IMAGE_FILE); sync; \
	popd

#CONFIG_VISIONFIVE_V1_SD_DEV=sdg
visionfive_v1_flash_sd:
	@if [ -b /dev/$(CONFIG_VISIONFIVE_V1_SD_DEV) ]; then \
		whiptail \
		--title "!!!Warning: DISK data will be OVERWRITED!!!" \
		--yesno "Warning:\n\
		\nDo you really want to OVERWRITED /dev/$(CONFIG_VISIONFIVE_V1_SD_DEV)?\n\
		\nPlease make sure that you have backed up your data!" \
		12 60 || exit 0;\
	else \
		echo /dev/$(CONFIG_VISIONFIVE_V1_SD_DEV) is not a block device.; exit 1;\
	fi;\
	sudo dd if=$(VISIONFIVE_V1_SD_IMAGE_DIR)/$(VISIONFIVE_V1_SD_IMAGE_FILE) \
		of=/dev/$(CONFIG_VISIONFIVE_V1_SD_DEV) bs=4M; sync;\
	sudo umount /dev/$(CONFIG_VISIONFIVE_V1_SD_DEV)$(VISIONFIVE_V1_SD_ROOTFS_PART_NUM);\
	sudo growpart /dev/$(CONFIG_VISIONFIVE_V1_SD_DEV) $(VISIONFIVE_V1_SD_ROOTFS_PART_NUM);\
	sudo e2fsck -f /dev/$(CONFIG_VISIONFIVE_V1_SD_DEV)$(VISIONFIVE_V1_SD_ROOTFS_PART_NUM);\
	sudo resize2fs /dev/$(CONFIG_VISIONFIVE_V1_SD_DEV)$(VISIONFIVE_V1_SD_ROOTFS_PART_NUM);\
	sudo fsck.ext4 /dev/$(CONFIG_VISIONFIVE_V1_SD_DEV)$(VISIONFIVE_V1_SD_ROOTFS_PART_NUM)
