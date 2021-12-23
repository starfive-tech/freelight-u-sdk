
# disk tools
MKFS_VFAT ?= mkfs.vfat
MKFS_EXT4 ?= mkfs.ext4
PARTPROBE ?= partprobe
SGDISK ?= sgdisk

# Relevant partition type codes
BBL		= 2E54B353-1271-4842-806F-E436D6AF6985
VFAT            = EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
LINUX		= 0FC63DAF-8483-4772-8E79-3D69D8477DE4
#FSBL		= 5B193300-FC78-40CD-8002-E86C45580B47
UBOOT		= 5B193300-FC78-40CD-8002-E86C45580B47
UBOOTENV	= a09354ac-cd63-11e8-9aff-70b3d592f0fa
UBOOTDTB	= 070dd1a8-cd64-11e8-aa3d-70b3d592f0fa
UBOOTFIT	= 04ffcafa-cd65-11e8-b974-70b3d592f0fa

flash.gpt: $(flash_image)

ifeq ($(TARGET_BOARD),U74)
VFAT_START=4096
VFAT_END=270335
VFAT_SIZE=266239
UBOOT_START=270336
UBOOT_END=272383
UBOOT_SIZE=2047
UENV_START=272384
UENV_END=274431
$(vfat_image): $(fit) $(confdir)/u74_uEnv.txt
	@if [ `du --apparent-size --block-size=512 $(uboot) | cut -f 1` -ge $(UBOOT_SIZE) ]; then \
		echo "Uboot is too large for partition!!\nReduce uboot or increase partition size"; \
		rm $(flash_image); exit 1; fi
	dd if=/dev/zero of=$(vfat_image) bs=512 count=$(VFAT_SIZE)
	$(MKFS_VFAT) $(vfat_image)
	PATH=$(RVPATH) MTOOLS_SKIP_CHECK=1 mcopy -i $(vfat_image) $(fit) ::hifiveu.fit
	PATH=$(RVPATH) MTOOLS_SKIP_CHECK=1 mcopy -i $(vfat_image) $(confdir)/u74_uEnv.txt ::u74_uEnv.txt
else

VFAT_START=4096
VFAT_END=269502
VFAT_SIZE=263454
UBOOT_START=2048
UBOOT_END=4048
UBOOT_SIZE=2000
UENV_START=1024
UENV_END=1099

$(vfat_image): $(fit) $(confdir)/uEnv.txt
	@if [ `du --apparent-size --block-size=512 $(uboot) | cut -f 1` -ge $(UBOOT_SIZE) ]; then \
		echo "Uboot is too large for partition!!\nReduce uboot or increase partition size"; \
		rm $(flash_image); exit 1; fi
	dd if=/dev/zero of=$(vfat_image) bs=512 count=$(VFAT_SIZE)
	$(MKFS_VFAT) $(vfat_image)
	PATH=$(RVPATH) MTOOLS_SKIP_CHECK=1 mcopy -i $(vfat_image) $(fit) ::hifiveu.fit
	PATH=$(RVPATH) MTOOLS_SKIP_CHECK=1 mcopy -i $(vfat_image) $(confdir)/uEnv.txt ::uEnv.txt
endif
$(flash_image): $(uboot) $(fit) $(vfat_image)
	dd if=/dev/zero of=$(flash_image) bs=1M count=32
	$(SGDISK) --clear -g  \
		--new=1:$(VFAT_START):$(VFAT_END)  --change-name=1:"Vfat Boot"	--typecode=1:$(VFAT)   \
		--new=2:$(UBOOT_START):$(UBOOT_END)   --change-name=2:uboot	--typecode=2:$(UBOOT) \
		--new=3:$(UENV_START):$(UENV_END)   --change-name=3:uboot-env	--typecode=3:$(UBOOTENV) \
		$(flash_image)
	dd conv=notrunc if=$(vfat_image) of=$(flash_image) bs=512 seek=$(VFAT_START)
	dd conv=notrunc if=$(uboot) of=$(flash_image) bs=512 seek=$(UBOOT_START) count=$(UBOOT_SIZE)

DEMO_END=11718750

#$(demo_image): $(uboot) $(fit) $(vfat_image) $(ext_image)
#	dd if=/dev/zero of=$(flash_image) bs=512 count=$(DEMO_END)
#	$(SGDISK) --clear -g \
#		--new=1:$(VFAT_START):$(VFAT_END)  --change-name=1:"Vfat Boot"	--typecode=1:$(VFAT)   \
#		--new=3:$(UBOOT_START):$(UBOOT_END)   --change-name=3:uboot	--typecode=3:$(UBOOT) \
#		--new=2:264192:$(DEMO_END) --change-name=2:root	--typecode=2:$(LINUX) \
#		--new=4:1024:1247   --change-name=4:uboot-env	--typecode=4:$(UBOOTENV) \
#		$(flash_image)
#	dd conv=notrunc if=$(vfat_image) of=$(flash_image) bs=512 seek=$(VFAT_START)
#	dd conv=notrunc if=$(uboot) of=$(flash_image) bs=512 seek=$(UBOOT_START) count=$(UBOOT_SIZE)

.PHONY: format-boot-loader
format-boot-loader: $(bbl_bin) $(uboot) $(fit) $(vfat_image)
	@test -b $(DISK) || (echo "$(DISK): is not a block device"; exit 1)
	$(SGDISK) --clear -g  \
		--new=1:$(VFAT_START):$(VFAT_END)  --change-name=1:"Vfat Boot"	--typecode=1:$(VFAT)   \
		--new=2:$(UBOOT_START):$(UBOOT_END)   --change-name=2:uboot	--typecode=2:$(UBOOT) \
		--new=3:$(UENV_START):$(UENV_END)  --change-name=3:uboot-env	--typecode=3:$(UBOOTENV) \
		--new=4:274432:0 --change-name=4:root	--typecode=4:$(LINUX) \
		$(DISK)
	-$(PARTPROBE)
	@sleep 1
ifeq ($(DISK)p1,$(wildcard $(DISK)p1))
	@$(eval PART1 := $(DISK)p1)
	@$(eval PART2 := $(DISK)p2)
	@$(eval PART3 := $(DISK)p3)
	@$(eval PART4 := $(DISK)p4)
else ifeq ($(DISK)s1,$(wildcard $(DISK)s1))
	@$(eval PART1 := $(DISK)s1)
	@$(eval PART2 := $(DISK)s2)
	@$(eval PART3 := $(DISK)s3)
	@$(eval PART4 := $(DISK)s4)
else ifeq ($(DISK)1,$(wildcard $(DISK)1))
	@$(eval PART1 := $(DISK)1)
	@$(eval PART2 := $(DISK)2)
	@$(eval PART3 := $(DISK)3)
	@$(eval PART4 := $(DISK)4)
else
	@echo Error: Could not find bootloader partition for $(DISK)
	@exit 1
endif
	dd if=$(uboot) of=$(PART2) bs=4096
	dd if=$(vfat_image) of=$(PART1) bs=4096

DEMO_IMAGE	:= sifive-debian-demo-mar7.tar.xz
DEMO_URL	:= https://github.com/tmagik/freedom-u-sdk/releases/download/hifiveu-2.0-alpha.1/

format-rootfs-image: format-boot-loader
	@echo "Done setting up basic initramfs boot. We will now try to install"
	@echo "a Debian snapshot to the Linux partition, which requires sudo"
	@echo "you can safely cancel here"
	$(MKFS_EXT4) $(PART4)
	-mkdir -p tmp-mnt
	-mkdir -p tmp-rootfs
	-sudo mount $(PART4) tmp-mnt && \
		sudo mount -o loop $(buildroot_rootfs_ext) tmp-rootfs&& \
		sudo cp -fr tmp-rootfs/* tmp-mnt/
	sudo umount tmp-mnt
	sudo umount tmp-rootfs
format-demo-image: format-boot-loader
	@echo "Done setting up basic initramfs boot. We will now try to install"
	@echo "a Debian snapshot to the Linux partition, which requires sudo"
	@echo "you can safely cancel here"
	$(MKFS_EXT4) $(PART4)
	-mkdir tmp-mnt
	-sudo mount $(PART4) tmp-mnt && cd tmp-mnt && \
		sudo wget $(DEMO_URL)$(DEMO_IMAGE) && \
		sudo tar -xvpf $(DEMO_IMAGE)
	sudo umount tmp-mnt

# default size: 16GB
DISK_IMAGE_SIZE ?= 16
ROOT_BEGIN=272384
ROOT_CLUSTER_NUM=$(shell echo $$(($(DISK_IMAGE_SIZE)*1024*1024*1024/512)))
ROOT_END=$(shell echo $$(($(ROOT_BEGIN)+$(ROOT_CLUSTER_NUM))))

format-nvdla-disk: $(bbl_bin) $(uboot) $(fit) $(vfat_image)
	@test -b $(DISK) || (echo "$(DISK): is not a block device"; exit 1)
	sudo $(SGDISK) --clear -g \
		--new=1:$(VFAT_START):$(VFAT_END)  --change-name=1:"Vfat Boot"  --typecode=1:$(VFAT)   \
		--new=2:$(UBOOT_START):$(UBOOT_END)   --change-name=2:uboot --typecode=2:$(UBOOT) \
		--new=3:$(ROOT_BEGIN):0 --change-name=3:root  --typecode=3:$(LINUX) \
		$(DISK)
	-$(PARTPROBE)
	@sleep 3
ifeq ($(DISK)p1,$(wildcard $(DISK)p1))
	@$(eval PART1 := $(DISK)p1)
	@$(eval PART2 := $(DISK)p2)
	@$(eval PART3 := $(DISK)p3)
else ifeq ($(DISK)s1,$(wildcard $(DISK)s1))
	@$(eval PART1 := $(DISK)s1)
	@$(eval PART2 := $(DISK)s2)
	@$(eval PART3 := $(DISK)s3)
else ifeq ($(DISK)1,$(wildcard $(DISK)1))
	@$(eval PART1 := $(DISK)1)
	@$(eval PART2 := $(DISK)2)
	@$(eval PART3 := $(DISK)3)
else
	@echo Error: Could not find bootloader partition for $(DISK)
	@exit 1
endif
	sudo dd if=$(uboot) of=$(PART2) bs=4096
	sudo dd if=$(vfat_image) of=$(PART1) bs=4096

#usb config

format-usb-disk: sbi $(uboot) $(fit) $(vfat_image)
	@test -b $(DISK) || (echo "$(DISK): is not a block device"; exit 1)
	sudo $(SGDISK) --clear -g \
	--new=1:0:+256M  --change-name=1:"Vfat Boot"  --typecode=1:$(VFAT)   \
	--new=2:0:+1G   --change-name=2:uboot --typecode=2:$(UBOOT) -g\
	$(DISK)
	-$(PARTPROBE)
	@sleep 3
ifeq ($(DISK)p1,$(wildcard $(DISK)p1))
	@$(eval PART1 := $(DISK)p1)
	@$(eval PART2 := $(DISK)p2)
#	@$(eval PART3 := $(DISK)p3)
else ifeq ($(DISK)s1,$(wildcard $(DISK)s1))
	@$(eval PART1 := $(DISK)s1)
	@$(eval PART2 := $(DISK)s2)
#	@$(eval PART3 := $(DISK)s3)
else ifeq ($(DISK)1,$(wildcard $(DISK)1))
	@$(eval PART1 := $(DISK)1)
	@$(eval PART2 := $(DISK)2)
#	@$(eval PART3 := $(DISK)3)
else
	@echo Error: Could not find bootloader partition for $(DISK)
	@exit 1
endif
	sudo dd if=$(uboot) of=$(PART2) bs=4096
	sudo dd if=$(vfat_image) of=$(PART1) bs=4096

DEB_IMAGE := debian_nvdla_20190506.tar.xz
DEB_URL := https://github.com/sifive/freedom-u-sdk/releases/download/nvdla-demo-0.1

format-nvdla-rootfs: format-nvdla-disk
	@echo "Done setting up basic initramfs boot. We will now try to install"
	@echo "a Debian snapshot to the Linux partition, which requires sudo"
	@echo "you can safely cancel here"
	sudo $(MKFS_EXT4) -F $(PART3)
	-mkdir -p tmp-mnt
	-mkdir -p tmp-rootfs
	-sudo mount $(PART3) tmp-mnt && \
		sudo mount -o loop $(buildroot_rootfs_ext) tmp-rootfs&& \
		sudo cp -fr tmp-rootfs/* tmp-mnt/
	sudo umount tmp-mnt
	sudo umount tmp-rootfs

format-nvdla-root: format-nvdla-disk
	@echo "Done setting up basic initramfs boot. We will now try to install"
	@echo "a Debian snapshot to the Linux partition, which requires sudo"
	@echo "you can safely cancel here"
	@test -e $(wrkdir)/$(DEB_IMAGE) || (wget -P $(wrkdir) $(DEB_URL)/$(DEB_IMAGE))
	sudo $(MKFS_EXT4) $(PART3)
	-mkdir -p tmp-mnt
	-sudo mount $(PART3) tmp-mnt && \
		echo "please wait until checkpoint reaches 489k" && \
		sudo tar xpf $(wrkdir)/$(DEB_IMAGE) -C tmp-mnt --checkpoint=1000
	sudo umount tmp-mnt
