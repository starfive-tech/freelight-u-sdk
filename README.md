### NOTE: support in upstream Buildroot
BeagleV Starlight and the StarFive JH7100 SoC has been merged into upstream buildroot.
[Instructions](https://github.com/buildroot/buildroot/blob/master/board/beaglev/readme.txt) are available in [buildroot master branch](https://github.com/buildroot/buildroot/blob/master/board/beaglev/). 


# Freelight U SDK #
This builds a complete RISC-V cross-compile toolchain for the StarFiveTech JH7100 SoC. It also builds a OpenSBI binary with U-boot as payload and a FIT(flattened image tree) image with linux kernel, device tree, ramdisk and rootdisk for the `Starlight` Dev board and `Visionfive` Dev board.

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
	patchutils python screen texinfo unzip zlib1g-dev device-tree-compiler libyaml-dev

## Fetch code Instructions ##

Checkout this repository (the multimedia branch: `JH7100_starlight_multimedia`). Then you will need to checkout all of the linked submodules using:

	$ git checkout --track origin/JH7100_starlight_multimedia
	$ git submodule update --init --recursive

This will take some time and require around 5GB of disk space. Some modules may
fail because certain dependencies don't have the best git hosting.

Once the submodules are initialized, 4 submodules `buildroot`, `HiFive_U-boot`,
`linux` and `opensbi` need checkout to corresponding branches manually, seeing `.gitmodule`

	$ cd buildroot && git checkout --track origin/starlight_multimedia && cd ..
	$ cd HiFive_U-Boot && git checkout --track origin/JH7100_Multimedia_V0.1.0 && cd ..
	$ cd linux && git checkout --track origin/visionfive-5.13.y-devel && cd ..
	$ cd opensbi && git checkout master && cd ..

## Build Instructions ##

After update submodules, run `make` or `make -jx` and the complete toolchain and
fw_payload.bin.out & image.fit will be built. The completed build tree will consume about 18G of
disk space.

U-SDK support different hardware boards, you should specify the board you use when build the project. Now U-SDK support fit image build for three starfive boards: `starlight`, `starlight-a1` or `visionfive`.
You can choose building your target board with `HWBOARD` variables.

> For starlight-a1 or a2 board:  `make HWBOARD=starlight-a1`, or `make starlight-a1`
>
> For starlight-a3 board:  `make HWBOARD=starlight`, or `make starlight`
>
> For visionfive board: `make HWBOARD=visionfive`, or `make visionfive`

Note that if not specify the  `HWBOARD` , the default `make`  is for `visionfive`, just equal to `make HWBOARD=visionfive`
Also you can specify the system environment variables `HWBOARD` to choose the board, 
eg. `export HWBOARD=visionfive`,  then run `make` as the below.

By default, the above generated image does not contain VPU driver module(wave511, the video hard decode driver and wave521, the video hard encode driver).  The following instructions will add VPU driver module according to your requirement:

	$ make -jx
	$ make vpudriver-build
	$ rm -rf work/buildroot_initramfs/images/rootfs.tar
	$ make -jx

Copy files fw_payload.bin.out and image.fit to tftp installation path to use

```
Path:
freelight-u-sdk/work/image.fit
freelight-u-sdk/work/opensbi/platform/generic/firmware/fw_payload.bin.out
```

Note the other make command to config uboot, linux, buildroot, busybox configuration:

```
make uboot-menuconfig      # uboot menuconfig
make linux-menuconfig      # Kernel menuconfig
make buildroot_initramfs-menuconfig   # initramfs menuconfig
make buildroot_rootfs-menuconfig      # rootfs menuconfig
make -C ./work/buildroot_initramfs/ O=./work/buildroot_initramfs busybox-menuconfig  # for initramfs busybox menuconfig 
make -C ./work/buildroot_rootfs/ O=./work/buildroot_rootfs busybox-menuconfig  # for rootfs busybox menuconfig
```

## Running on VisionFive/Starlight Board ##

After the `VisionFive` or `Starlight` is properly connected to the serial port cable, network cable and power cord, turn on the power from the wall power socket to power on the `VisionFive` or `Starlight` and you will see the startup information as follows:

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
	xxxxxxxxxxxFLASH PROGRAMMINGxxxxxxxxxxxxxxx
	xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	
	0:update boot
	1:quit
	select the function:

Type **"0"**  to update the uboot file fw_payload.bin.out via Xmodem mode,
and then Type **"1"** to exit Flash Programming.

After that, you will see the information `Starlight #` or `VisionFive #` when press the "f" button, select the installation path
and transfer image.fit through TFTP:

Step1: set enviroment parameter:

	setenv bootfile vmlinuz;setenv fdt_addr_r 0x88000000;setenv fdt_high 0xffffffffffffffff;setenv fdtcontroladdr 0xffffffffffffffff;setenv initrd_high 0xffffffffffffffff;setenv kernel_addr_r 0x84000000;setenv fileaddr a0000000;setenv ipaddr 192.168.xxx.xxx;setenv serverip 192.168.xxx.xxx

Step2: upload image file to ddr:

	tftpboot ${fileaddr} ${serverip}:image.fit;

Step3: load and excute:	

	bootm start ${fileaddr};bootm loados ${fileaddr};booti 0x80200000 0x86100000:${filesize} 0x86000000

When you see the `buildroot login:` message, then congratulations, the launch was successful

	buildroot login:root
	Password: starfive



## Appendix I: How to Switch Display Framework Between DRM and Framebuffer

The default display framework is `DRM` now.  Use `make linux-menuconfig`  follow below could change between `DRM` and `Framebuffer` framework

#### If switch to `Framebuffer` display framework with`hdmi` display device:

> 1. Disable the DRM feature:
>    CONFIG_DRM_I2C_NXP_TDA998X
>    CONFIG_DRM_I2C_NXP_TDA9950
>    CONFIG_DRM_STARFIVE
>
> 2. Enable the Framebuffer feature:
>    CONFIG_FB_STARFIVE
>    CONFIG_FB_STARFIVE_HDMI_TDA998X
>    CONFIG_FB_STARFIVE_VIDEO
>
> Note: Recommend disable the below:
>    CONFIG_NVDLA
>    CONFIG_FRAMEBUFFER_CONSOLE

#### If switch to `DRM` display framework with `hdmi` display device:

> 1. Disable the below kernel config
>    CONFIG_FB_STARFIVE
>    CONFIG_FB_STARFIVE_HDMI_TDA998X
>    CONFIG_FB_STARFIVE_VIDEO
>    CONFIG_NVDLA
>
> 2. Enable the below kernel config:
>    CONFIG_DRM_I2C_NXP_TDA998X
>    CONFIG_DRM_I2C_NXP_TDA9950
>    CONFIG_DRM_STARFIVE
>
> Note: when use DRM to hdmi pipeline, please make sure CONFIG_DRM_STARFIVE_MIPI_DSI is disable, or will cause color abnormal.

#### If switch to `DRM` display framework with`mipi dsi` display device:

> Based on the above "DRM with hdmi display device" config, enable the below kernel config:
> CONFIG_PHY_M31_DPHY_RX0
> CONFIG_DRM_STARFIVE_MIPI_DSI
>
> And also need to change HiFive_U-Boot/arch/riscv/dts/jh7100.dtsi.
> The default support HDMI, need to modify the "encoder-type" and "remote-endpoint" node to support mipi dsi, see the below:
>
> ```
> display-encoder {
> 	compatible = "starfive,display-encoder";
> 	encoder-type = <6>;	//2-TMDS, 3-LVDS, 6-DSI, 8-DPI
> 	status = "okay";
> 
> 	ports {
> 		port@0 {
> 			hdmi_out:endpoint {
> 				remote-endpoint = <&dsi_out_port>; // tda998x_0_input - HDMI, 
> 												   // dsi_out_port -MIPI DSI
> 			};
> 		};
> 
> 		port@1 {
> 			hdmi_input0:endpoint {
> 				remote-endpoint = <&crtc_0_out>;
> 			};
> 		};
> 
> 	};
> };
> ```



## Appendix II: How to Support WM8960 and AC108 Audio Board 

The visionfive and starlight board natively always support PWMDAC to audio-out (3.5mm mini-jack on the board), also support [WM8960 board](https://www.seeedstudio.com/ReSpeaker-2-Mics-Pi-HAT.html) to audio-in and audio-out, also support [AC108 board](https://www.seeedstudio.com/ReSpeaker-4-Mic-Array-for-Raspberry-Pi.html) to audio-in. Note that the WM8960 and AC108 could not be both supported.

 If support WM8960 board, need to modify the follow:

> HiFive_U-Boot/board/starfive/jh7100/jh7100.c:
>
> > #define STARFIVE_AUDIO_AC108    0
> > #define STARFIVE_AUDIO_WM8960    1
> > #define STARFIVE_AUDIO_VAD        0
> > #define STARFIVE_AUDIO_PWMDAC    1
> > #define STARFIVE_AUDIO_SPDIF    0
> > #define STARFIVE_AUDIO_PDM        0
>
> HiFive_U-Boot/arch/riscv/dts/jh7100-beaglev-starlight.dts:
>
> > /* #include "codecs/sf_pdm.dtsi" \*/
> > /\* #include "codecs/sf_spdif.dtsi" \*/
> > /\* #include "codecs/sf_ac108.dtsi" \*/
> > #include "codecs/sf_wm8960.dtsi"
> > /\* #include "codecs/sf_vad.dtsi" \*/

 If support AC108 board, need to modify the follow:

> HiFive_U-Boot/board/starfive/jh7100/jh7100.c:
>
> > #define STARFIVE_AUDIO_AC108    1
> > #define STARFIVE_AUDIO_WM8960    0
> > #define STARFIVE_AUDIO_VAD        0
> > #define STARFIVE_AUDIO_PWMDAC    1
> > #define STARFIVE_AUDIO_SPDIF    0
> > #define STARFIVE_AUDIO_PDM        0
>
> HiFive_U-Boot/arch/riscv/dts/jh7100-beaglev-starlight.dts:
>
> > /* #include "codecs/sf_pdm.dtsi" \*/
> > /\* #include "codecs/sf_spdif.dtsi" \*/
> > #include "codecs/sf_ac108.dtsi"
> > /\*#include "codecs/sf_wm8960.dtsi" \*/
> > /\* #include "codecs/sf_vad.dtsi" \*/
>
> Linux kernel config:
> run `make linux-menuconfig`, then enable `SND_SOC_AC108` 


## Appendix III: Build TF Card Booting Image

If you don't already use a local tftp server, then you probably want to make the TF card target; the default size is 16 GBs. NOTE THIS WILL DESTROY ALL EXISTING DATA on the target TF card; please modify the following file. The `GPT` Partition Table for the TF card is recommended. 

`HiFive_U-Boot/configs/starfive_jh7100_starlight_smode_defconfig` (for `starlight` or `starlight-a1` board)
or
`HiFive_U-Boot/configs/starfive_jh7100_visionfive_smode_defconfig`  (for `visionfive` board):

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

Please insert the TF card and run command `df -h` to check the device name `/dev/sdXX`, then run command `umount /dev/sdXX`",  then run the following instructions to build TF card image:

```
$ make HWBOARD=xxx -jx
$ make HWBOARD=xxx buildroot_rootfs -jx
$ make HWBOARD=xxx vpudriver-build-rootfs
$ rm -rf work/buildroot_rootfs/images/rootfs.ext*
$ make HWBOARD=xxx buildroot_rootfs -jx
$ make HWBOARD=xxx DISK=/dev/sdX format-nvdla-rootfs && sync
```
Note: please also do not forget to update the `fw_payload.bin.out` which is built by this step.

