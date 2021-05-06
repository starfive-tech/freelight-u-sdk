# Freelight U SDK #

This builds a complete RISC-V cross-compile toolchain for the StarFiveTech JH7100 SoC. It also builds U-boot and a flattened image tree (FIT)
image with a OpenSBI binary, linux kernel, device tree, ramdisk and rootdisk for the BeagleV development board.

## Prerequisites ##

Recommend OS: Ubuntu 16.04/18.04
Tested on: Gentoo, Ubuntu 18.04

After installing the operating system.
Do not forget updating all packages

    $ sudo apt update
    $ sudo apt upgrade

Install required additional packages.

    $ sudo apt-get install autoconf automake autotools-dev bc bison xxd \
    build-essential curl flex gawk gdisk git gperf libgmp-dev \
    libmpc-dev libmpfr-dev libncurses-dev libssl-dev libtool \
    patchutils python screen texinfo unzip zlib1g-dev device-tree-compiler

**Note: On Gentoo, if you don't have vim installed, you'll need it for the `xxd` tool.

## Download Sources ##

**Note: You only need this if you do not have an existing build environment.**

Make sure to [install the `repo` command by Google](https://source.android.com/setup/downloading#installing-repo) first.

### Create workspace

Repo initialization:

    $ mkdir freelight-sdk && cd freelight-sdk
    $ repo init -u https://github.com/starfive-tech/freelight-u-sdk.git -m tools/manifests/riscv-sdk.xml

The above uses default branch for the manifest file; add `-b <branch_name>` to use a different branch.

sync repo:

    $ repo sync   # this will check out branch HEAD commits

To make changes you want to keep, after sync use the following:

    $ repo start <newbranchname> [--all | <project>...]

### Update existing workspace

In order to bring all the repositories up to date with upstream

    $ cd freelight-sdk
    $ repo sync

If you have local changes, you might also need:

    $ repo rebase

### Repo tips

Some info on how to customize your sync:

    -j JOBS, --jobs=JOBS  projects to fetch simultaneously (default 4)

    --no-clone-bundle     disable use of /clone.bundle on HTTP/HTTPS

    --no-tags             don't fetch tags

Fastest full sync:

    $ repo sync --no-tags --no-clone-bundle

Smallest/fastest sync:

    $ repo init -u https://github.com/starfive-tech/freelight-u-sdk.git -b starfive -m tools/manifests/riscv-sdk.xml --no-clone-bundle --depth=1
    $ repo sync --no-tags --no-clone-bundle --current-branch

Note: we already define -c in riscv-sdk.xml, so no need to add it.


## Build Instructions ##

After syncing repositories, cd into the top-level SDK directory:

    $ cd freelight-u-sdk/

Then run `make` or `make -jx` and the complete toolchain, initramfs,
fw_payload.bin.out & image.fit will be built. The completed build tree
will consume about 18G of disk space.

Copy files fw_payload.bin.out and image.fit to tftp installation path to use

	Path:
	freelight-u-sdk/work/image.fit
	freelight-u-sdk/work/opensbi/platform/starfive/vic7100/firmware/fw_payload.bin.out

## SDCard Image ##

If you don't already use a local tftp server, then you probably want to make
the sdcard target; the default size is 16 GBs. NOTE THIS WILL DESTROY ALL
EXISTING DATA on the target sdcard; make sure you know the correct disk,
eg, with the lsblk command:

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

### Deploy a boot script

One way to automate the boot process would be to use u-boot's handy-dandy
`boot.scr` method (see the modified example from the meta-riscv repo in
`conf/beaglev-uEnv.txt`). To deploy it on your sdcard:

* open it in your favorite editor, and (optionally) edit the boot args
  and/or kernel image name (the defaults match the current SDK build)
* compile it using u-boot's' `mkimage` command:

    $ mkimage -C none -A riscv -T script -d conf/beaglev-uEnv.txt boot.scr

* copy the resulting `boot.scr` to the first (vfat) partition on the sdcard
* insert the sdcard and power it up
* wait for the console login prompt


### Local toolchain override

If you already built (or downloaded) a proper toolchain, for now you can override the
following make variables:

    target=riscv64-unknown-linux-gnu RVPATH=$PATH target_gcc=/usr/bin/riscv64-unknown-linux-gnu-gcc CROSS_COMPILE=riscv64-unknown-linux-gnu- make uboot

**Note:** with a crossdev toolchain, this only works for individual targets: uboot, vmlinux, sbi, etc.
        
## TFTP boot on BeagleV ##

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


If dhcp fails on boot, try logging in as root and run:

    # ifup eth0

