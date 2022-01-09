## Host info
BUILD_HOST_OS = $(shell lsb_release -i | awk '{print $$3}')

ifeq ($(BUILD_HOST_OS), Fedora)
	PKG_CMD = yum
	CONFIG_DEPENDENT_PACKAGES = newt
	TOOLCHAIN_DEPENDENT_PACKAGES = 
	ACPI_DEPENDENT_PACKAGES = acpica-tools
	UEFI_DEPENDENT_PACKAGES = libuuid-devel
	DTB_DEPENDENT_PACKAGES = dtc
	LINUX_DEPENDENT_PACKAGES = ncurses-devel expect sparse
else
$(warning  Warning: you are running the SDK on a unverified OS)
$(warning  Please run this on a Fedora/CentOS/RHEL)
$(warning  TODO: Please fix it here(strips/staging/env/host.mk) for more distros)
endif

ifeq ($(BUILD_HOST_OS), Ubuntu)
	PKG_CMD = apt-get
	TOOLCHAIN_DEPENDENT_PACKAGES = lib32z1 lib32ncurses5 lib32bz2-1.0 lib32stdc++6
	ACPI_DEPENDENT_PACKAGES=acpica-tools
	UEFI_DEPENDENT_PACKAGES=uuid-dev
	DTB_DEPENDENT_PACKAGES=device-tree-compiler
	LINUXKERNEL_DEPENDENT_PACKAGES=libncurses5-dev expect
endif
#lzop
#for ubuntu
#zlib-devel.i686 gcc-c++.i686
SDK_DEPENDENT_PACKAGES=terminix tmux

BUILD_HOST=$(shell uname -m)

BUILD_HOST_CPU_N=$(shell nproc)

#we may need this for debug build
ifeq ($(CONFIG_CUSTOMIZE_BUILD_JOBS),y)
BUILD_JOBS_N=$(CONFIG_BUILD_JOBS_N)
else
BUILD_JOBS_N=$(BUILD_HOST_CPU_N)
endif

