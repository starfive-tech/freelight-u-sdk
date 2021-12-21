## 										WAVE521编译与使用

### 主目录结构说明

doc				  	文档，包括ip和api说明文档

code					源码，包含驱动和用户态示例程序。详细功能描述参考 “wave521代码结构”

firmware			固件

README.md 	 本文件

#### 【wave521代码结构】

wave521源码由两部分组成: 

​	code/vdi/linux/driver: 编译生成vpu.ko，为wave521的模块文件，主要功能是空间分配及映射、中断注册及平台无关功能实现。

​	code/下除vdi/linux/driver以外的目录: 编译生成sample app。主要功能是初始化vpu，设置vpu工作参数，启动vpu编码工作。

wave521使用freedom-u-sdk提供的交叉编译工具链进行编译，交叉编译采用的工具链前缀为riscv64-xxx-linux-gnu-，在编译前需要将交叉编译工具链路径添加至PATH环境变量中。

### 代码编译

编译程序需要进入wave521/code目录。

code目录主要文件说明：

set_env.sh						 配置环境变量文件

Wave5xxDriver.mak		编译linux模块驱动makefile文件

Wave5xxEncV2.mak		编译单路编码示例程序makefile

Wave5xxMultiV2.mak	 编译多路编码示例程序makefile

build_for_venc.sh			一键编译工具

#### 一键编译脚本

```bash
$./build_for_venc.sh #（可一键编译驱动、用户态程序，打包测试程序需要的文件,生成文件保存到venc_driver
```

#### 单独编译

编译前，需要配置交叉编译环境。

```bash
$ source set_env.sh 
```

##### 驱动模块编译

```bash
$make -f  Wave5xxDriver.mak
```

编译成功在当前目录下生成vpu.ko，在运行wave521 sample app前，需要先通过专用加载脚本加载该模块，后面章节会进一步描述。

##### sample app程序编译

```bash
$make -f  Wave5xxEncV2.mak 		#编译单路编码程序
$make -f  Wave5xxMultiV2.mak	#编译多路编码程序
```

wave521支持单路或至多4路编码，使用wave521/Wave5xxEncV2.mak编译生成单路编码sample app---venc_test，使用wave521/Wave5xxMultiV2.mak编译生成多路编码sample app---multi_instance_test。



目前暂时未使用FFMPEG，后续如果使用，可去掉USE_FFMPEG = no即可。

在wave521/下直接执行make进行编译。编译成功生成sample app，在Linux shell下调用该app即可进行视频编码工作。

### 3、wave521运行

#### 3.1、vpu.ko加载

上述章节提到编译生成的vpu.ko不能直接通过insmod命令加载，需要调用脚本进行加载，因为代码中没有在/dev目录下创建vpu对应的设备文件，是通过脚本在模块加载前进行设备文件创建。模块加载脚本路径为freedom-u-sdk/soft_3rdparty/wave521/root/load.sh，加载脚本与vpu.ko应在同一目录下。

不管wave521采用单路还是多路工作模式，均使用同一个vpu.ko，唯一区别是多路工作模式下，需要预留更多空间，关于空间预留后续章节会提及。

#### 3.1、sample app运行

运行脚本run.sh，开始编码。run.sh具体命令如下

```shell
./venc_test --codec=12 --cfgFileName=./cfg/hevc_fhd_inter_8b_11.cfg --output=./output/inter_8b_11.cfg.265 --input=./yuv/mix_1920x1080_8b_9frm.yuv
```

--codec=12: 设置编码格式，目前验证两种格式:HEVC和AVC，HEVC:12, AVC:0

--cfgFileName:编码需要用到的参数配置文件，可以根据实际情况修改该配置文件，目前默认使用的是C&M官方提供的配置文件，注意不同的编码格式需要传递不同的编码参数，编码参数使用可以参考官方提供的默认配置文件。

*注:官方代码也提供不使用配置文件方式，在代码中直接定义默认参数的方式，但是该使用该方式程序运行会报错。*

--output: 为编码输出文件

--input: 为编码输入文件

multi_instance_test运行脚本为multi-run.sh，具体命令如下

```shell
./multi_instance_test --instance-num=3 -e 1,1,1 --codec=12,0,12 --output=./output/bg_8b_01.cfg.265,./output/inter_8b_02.cfg.264,./output/inter_8b_11.cfg.265 --input=./cfg/hevc_bg_8b_01.cfg,./cfg/avc_inter_8b_02.cfg,./cfg/hevc_fhd_inter_8b_11.cfg
```

--instance-num: 进行编码工作的路数，如示例使用3路进行编码工作，则-e 1,1,1表示instanc0的-e为1，instance1的-e为1，instance2的-e为1。

-e: 编解码选择。decoder=0,  encoder=1

--codec:编码格式，同单路编码程序--codec一致

--output:为编码输出文件

--input: 编码需要使用的配置文件，同单路测试程序中的--cfgFileName一致

venc_test和multi_instance_test需在Linux shell命令行下调用，可以通过scp命令拷贝传输sample app及依赖文件，也可以使用将测试程序打包到initramfs中，系统启动后直接运行。为了方便目前采用将测试程序打包进initramfs的方式，只需将freedom-u-sdk/soft_3rdparty/wave521/root/替换freedom-u-sdk/work/buildroot_initramfs/target/root和freedom-u-sdk/work/buildroot_initramfs_sysroot/root下即可。

### 4、Tips

#### 4.1、DTS配置

freedom-u-sdk/HiFive_U-Boot/arch/riscv/dts/hifive_u74_nvdla_iofpga.dts

```dtd
		vpu_enc:vpu_enc@118E0000 {
			compatible = "cm,cm521-vpu";
			reg = <0x0 0x118E0000 0x0 0x4000>;
			interrupt-parent = <&plic>;
			interrupts = <26>;
			clocks = <&vpuclk>;
			clock-names = "vcodec";
			reg-names = "control";
		};
```

#### 4.2、预留内存方式

官方提供参考代码，提供两种内存预留方式

1)、直接划分一块物理空间作为wave521使用的内存空间，参考代码实现了一套内存管理API用于wave521的内存操作，代码中需要定义VPU_SUPPORT_RESERVED_VIDEO_MEMORY宏，该方式操作比较简单，但是必须保证划分的空间为wave521独占内存，不被其他模块使用，因此该方式会减少CPU使用的有效空间，不推荐使用。

2)、使用内核接口dma_alloc_coherent/dma_free_coherent动态分配内存，需要屏蔽VPU_SUPPORT_RESERVED_VIDEO_MEMORY宏定义，由于dma_alloc_coherent默认一次性只能分配最大4M空间，wave521单次内存存在分配10M及以上的内存大小。需要实现CMA框架，通过CMA机制，我们可以做到不预留内存，这些内存平时是可用的，只有当需要的时候才被分配给wave521，关于CMA目前已完成移植，并且验证通过，详情参见最新linux kernel代码。

#### 4.3、cache一致性问题

VIC中的cache一致性问题在wave521中也存在，暂时采用sysport地址代替memport地址方式规避该问题。具体修改方式:

在对dma buff空间进行mmap时，写入wave521寄存器地址使用memport地址，因为wave521寄存器只支持32位地址，但是在进行mmap操作时，传递给mmap接口的物理地址则使用memport对应的sysport地址。