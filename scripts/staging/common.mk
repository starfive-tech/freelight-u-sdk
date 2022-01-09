# The rlue of definition:
# <component>_<operation>_<board>_<chip>_<vendor>

dep_packages_install: system_data_update sdk_deps_install
	sudo $(PKG_CMD) install $(CONFIG_DEPENDENT_PACKAGES)
	sudo $(PKG_CMD) install $(TOOLCHAIN_DEPENDENT_PACKAGES)
	#for Flat Device Tree
	sudo $(PKG_CMD) install $(DTB_DEPENDENT_PACKAGES)
	#for Linux Kernel development
	sudo $(PKG_CMD) install $(LINUX_DEPENDENT_PACKAGES)

#qemu_build_deps_install:
#	sudo $(PKG_CMD) install $(QEMU_DEPENDENT_PACKAGES)

system_data_update:
	sudo $(PKG_CMD) update

sdk_deps_install:
	sudo $(PKG_CMD) install $(SDK_DEPENDENT_PACKAGES)

