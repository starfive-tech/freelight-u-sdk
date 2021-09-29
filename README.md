### NOTE: support in upstream Buildroot
BeagleV Starlight and the StarFive JH7100 SoC has been merged into upstream buildroot.
[Instructions](https://github.com/buildroot/buildroot/blob/master/board/beaglev/readme.txt) are available in [buildroot master branch](https://github.com/buildroot/buildroot/blob/master/board/beaglev/). 


# Freelight U SDK #
This builds a complete RISC-V cross-compile toolchain for the StarFiveTech JH7100 SoC. It also builds U-boot and a flattened image tree (FIT)
image with a OpenSBI binary, linux kernel, device tree, ramdisk and rootdisk for the Starlight development board.

## Prerequisites ##

Recommend OS: Ubuntu 16.04/18.04

After installing the operating system.
Do not forget updating all packages

	$sudo apt update
	$sudo apt upgrade

Install required additional packages.

	$ sudo apt-get install autoconf automake autotools-dev bc bison xxd \
	build-essential curl flex gawk gdisk git gperf libgmp-dev \
	libmpc-dev libmpfr-dev libncurses-dev libssl-dev libtool \
	patchutils python screen texinfo unzip zlib1g-dev device-tree-compiler

## Fetch code Instructions ##

Checkout this repository (the multimedia branch: `JH7100_starlight_multimedia`). Then you will need to checkout all of the linked
submodules using:

	$ git checkout --track origin/JH7100_starlight_multimedia
	$ git submodule update --init --recursive

This will take some time and require around 7GB of disk space. Some modules may
fail because certain dependencies don't have the best git hosting.

Once the submodules are initialized, 4 submodules `buildroot`, `HiFive_U-boot`,
`linux` and `opensbi` need checkout to corresponding branches manually, seeing `.gitmodule`

	$ cd buildroot && git checkout --track origin/starlight_multimedia && cd ..
	$ cd HiFive_U-Boot && git checkout --track origin/Fedora_JH7100_upstream_devel && cd ..
	$ cd linux && git checkout --track origin/beaglev-5.13.y_multimedia && cd ..
	$ cd opensbi && git checkout --track origin/master && cd ..

## Build Instructions ##

After update submodules, run `make` or `make -jx` and the complete toolchain and
fw_payload.bin.out & image.fit will be built. The completed build tree will consume about 18G of
disk space.

By default, the above generated image does not contain VPU module(wave511, the video hard decode driver and openmax-il framework library).  The following instructions will add VPU module according to your requirement:

	$ make -jx
	$ make vpubuild
	$ rm -rf work/buildroot_initramfs/images/rootfs.tar
	$ make -jx

Copy files fw_payload.bin.out and image.fit to tftp installation path to use

```
Path:
freelight-u-sdk/work/image.fit
freelight-u-sdk/work/opensbi/platform/generic/firmware/fw_payload.bin.out
```

The other make command:

```
make linux-menuconfig      # Kernel menuconfig
make uboot-menuconfig      # uboot menuconfig
make buildroot_initramfs-menuconfig   # initramfs menuconfig
make buildroot_rootfs-menuconfig      # rootfs menuconfig
```

## Display Framework Switch Instructions

The default display framework is `Framebuffer`. If switch to `DRM` display framework (`drm to hdmi` pipeline):

```
1. Disable the below kernel config
   CONFIG_FB_STARFIVE
   CONFIG_FB_STARFIVE_HDMI_TDA998X
   CONFIG_FB_STARFIVE_VIDEO
   CONFIG_NVDLA
   CONFIG_FRAMEBUFFER_CONSOLE

2. Enable the below kernel config:
   CONFIG_DRM_I2C_NXP_TDA998X
   CONFIG_DRM_I2C_NXP_TDA9950
   CONFIG_DRM_STARFIVE

Note: when use DRM to hdmi pipeline, please make sure CONFIG_DRM_STARFIVE_MIPI_DSI is disable, or will cause color abnormal.
```

If switch to `DRM` display framework (`drm to mipi` pipeline):

```
based on the above drm to hdmi pipeline config, enable the below kernel config:
CONFIG_PHY_M31_DPHY_RX0
CONFIG_DRM_STARFIVE_MIPI_DSI
```

## Build SD Card Booting Image

If want to generate the sd card booting image, please modify the following file.

conf/beaglev_defconfig_513:

```
change
CONFIG_CMDLINE="earlyprintk console=tty1 console=ttyS0,115200 debug rootwait stmmaceth=chain_mode:1"
to
CONFIG_CMDLINE="earlyprintk console=tty1 console=ttyS0,115200 debug rootwait stmmaceth=chain_mode:1 root=/dev/mmcblk0p3"
```

HiFive_U-Boot/configs/starfive_vic7100_evb_smode_defconfig:

```
change
CONFIG_USE_BOOTCOMMAND is not set
to
CONFIG_USE_BOOTCOMMAND=y

change
#CONFIG_BOOTCOMMAND="run mmcsetup; run fdtsetup; run fatenv; echo 'running boot2...'; run boot2"
to
CONFIG_BOOTCOMMAND="run mmcsetup; run fdtsetup; run fatenv; echo 'running boot2...'; run boot2"
```

Then insert the TF card, and run command `df -h` to check the TF card device name `/dev/sdXX`, then run command `umount /dev/sdXX`",  then run the below instruction:

```
make buildroot_rootfs -jx
make -jx
make vpubuild_rootfs
sudo make clean
make -jx
make buildroot_rootfs -jx
sudo make DISK=/dev/sdX format-nvdla-rootfs && sync
```

## Running on Starlight Board ##

After the Starlight is properly connected to the serial port cable, network cable and power cord turn on the power from the wall power socket to power on the Starlight and you will see the startup information as follows:

	bootloader version: 210209-4547a8d
	ddr 0x00000000, 1M test
	ddr 0x00100000, 2M test
	DDR clk 2133M,Version: 210302-5aea32f
	2
Press any key as soon as it starts up to enter the **upgrade menu**. In this menu, you can update uboot

	bootloader version: 210209-4547a8d
	ddr 0x00000000, 1M test
	ddr 0x00100000, 2M test
	DDR clk 2133M,Version: 210302-5aea32f
	0
	xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	xxxxxxxxxxxFLASH PROGRAMMINGxxxxxxxxx
	xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	
	0:update boot
	1: quit
	select the function:

Type **"0"**  to update the uboot file fw_payload.bin.out via Xmodem mode,
and then Type **"1"** to exit Flash Programming.

After you will see the information `StarFive #`,select the installation path
and install image.fit through TFTP:

	setenv fileaddr a0000000; setenv serverip 192.168.xxx.xxx;tftpboot ${fileaddr}  ${serverip}:image.fit;bootm start ${fileaddr};bootm loados ${fileaddr};booti 0x80200000 0x86100000:${filesize} 0x86000000

When you see the `buildroot login:` message, then congratulations, the launch was successful

	buildroot login:root
	Password: starfive

