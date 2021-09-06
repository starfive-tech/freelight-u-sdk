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
`linux` and `opensbi` need checkout to starfive branch manually, seeing `.gitmodule`

	$ cd buildroot && git checkout --track origin/starlight_multimedia && cd ..
	$ cd HiFive_U-Boot && git checkout --track origin/JH7100_starlight_multimedia && cd ..
	$ cd linux && git checkout --track origin/beaglev-5.13.y_multimedia && cd ..
	$ cd opensbi && git checkout --track origin/starfive && cd ..

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

	Path:
	freelight-u-sdk/work/image.fit
	freelight-u-sdk/work/opensbi/platform/starfive/vic7100/firmware/fw_payload.bin.out


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

## Known Issue ##
- [ ] The system freezes during the u-boot phase if OpenSBI was moved to generic platform;
- [ ] I2C and I2S have some errors during boot stage after code was merged into 5.13.y kernel(Maybe it was conflict with pinctrl)
