### NOTE: support in upstream Buildroot
BeagleV Starlight and the StarFive JH7100 SoC has been merged into upstream buildroot.
[Instructions](https://github.com/buildroot/buildroot/blob/master/board/beaglev/readme.txt) are available in [buildroot master branch](https://github.com/buildroot/buildroot/blob/master/board/beaglev/). 


# Freelight U SDK #
This builds a complete RISC-V cross-compile toolchain for the StarFiveTech JH7100 SoC. It also builds U-boot and a flattened image tree (FIT)
image with a OpenSBI binary, linux kernel, device tree, ramdisk and rootdisk for the BeagleV development board.

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

## Build Instructions ##

Checkout this repository (default branch:starfive). Then you will need to checkout all of the linked
submodules using:

	$ git submodule update --init --recursive

This will take some time and require around 7GB of disk space. Some modules may
fail because certain dependencies don't have the best git hosting.

Once the submodules are initialized, 4 submodules `buildroot`, `HiFive_U-boot`,
`linux` and `opensbi` need checkout to starfive branch manually.

After update submodules, run `make` or `make -jx` and the complete toolchain and
fw_payload.bin.out & image.fit will be built. The completed build tree will consume about 18G of
disk space.

Copy files fw_payload.bin.out and image.fit to tftp installation path to use

	Path:
	freelight-u-sdk/work/image.fit
	freelight-u-sdk/work/opensbi/platform/starfive/vic7100/firmware/fw_payload.bin.out

## SDCard Image ##

If you don't already use a local tftp server, then you probably want to make
the sdcard target; the default size is 16 GBs. NOTE THIS WILL DESTROY ALL
EXISTING DATA on the target sdcard; make sure you know the correct disk,
eg, with the lsblk command::

    $ lsblk
    NAME        MAJ:MIN RM   SIZE RO TYPE MOUNTPOINT
    sda           8:0    0 465.8G  0 disk 
    ├─sda1        8:1    0     2M  0 part 
    ├─sda2        8:2    0   128M  0 part 
    ├─sda3        8:3    0  1000M  0 part [SWAP]
    └─sda4        8:4    0 464.7G  0 part /
    sr0          11:0    1  1024M  0 rom  
    mmcblk0     179:0    0   3.6G  0 disk 
    ├─mmcblk0p1 179:1    0   130M  0 part 
    ├─mmcblk0p2 179:2    0     1M  0 part 
    └─mmcblk0p3 179:3    0   3.5G  0 part 

you would use the `/dev/mmcblk0` (whole disk) target. Note the build output
above is only a small initramfs, so the rootfs partition on the card is not
formatted yet. You will need an sdcard at least 1G (or larger if you want
to add a full rootfs).  Make sure the sdcard is inserted and you have the
correct device before you run the `make` command below. Then run something
like:

    $ make DISK_IMAGE_SIZE=4 DISK=/dev/mmcblk0 format-nvdla-disk

to generate a 4 GB sdcard. Currently you need to boot it manually from the
u-boot prompt:

    BeagleV # fatload mmc 0 ${ramdisk_addr_r} hifiveu.fit
    (wait for loading)
    BeagleV # bootm ${ramdisk_addr_r}


## Running on BeagleV ##

After the BeagleV™ is properly connected to the serial port cable, network cable and power cord turn on the power from the wall power socket to power on the BeagleV™ and you will see the startup information as follows:

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
