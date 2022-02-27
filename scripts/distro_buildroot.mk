# TODO: depracated for now
#ifneq ($(RISCV),$(buildroot_initramfs_wrkdir)/host)
#$(target_gcc):
#	$(error The RISCV environment variable was set, but is not pointing at a toolchain install tree)
#else
#$(target_gcc): $(buildroot_initramfs_tar)
#endif

$(buildroot_initramfs_wrkdir)/.config: $(buildroot_srcdir)
	rm -rf $(dir $@)
	mkdir -p $(dir $@)
	cp $(buildroot_initramfs_config) $@
	$(MAKE) -C $< RISCV=$(RISCV) O=$(buildroot_initramfs_wrkdir) olddefconfig

# buildroot_initramfs provides gcc
$(buildroot_initramfs_tar): $(buildroot_srcdir) $(buildroot_initramfs_wrkdir)/.config $(buildroot_initramfs_config)
	$(MAKE) -C $< RISCV=$(RISCV) O=$(buildroot_initramfs_wrkdir)

$(buildroot_initramfs_sysroot_stamp): $(buildroot_initramfs_tar)
	-rm -rf $(buildroot_initramfs_sysroot)
	mkdir -p $(buildroot_initramfs_sysroot)
	tar -xpf $< -C $(buildroot_initramfs_sysroot) --exclude ./dev --exclude ./usr/share/locale
	touch $@

$(buildroot_initramfs_sysroot): $(buildroot_initramfs_sysroot_stamp)

.PHONY: buildroot_initramfs-menuconfig
buildroot_initramfs-menuconfig: $(buildroot_initramfs_wrkdir)/.config $(buildroot_srcdir)
	$(MAKE) -C $(dir $<) O=$(buildroot_initramfs_wrkdir) menuconfig
	$(MAKE) -C $(dir $<) O=$(buildroot_initramfs_wrkdir) savedefconfig
	cp $(dir $<)defconfig conf/buildroot_initramfs_config

# vpu building depend on the $(vmlinux), $(vmlinux) depend on $(buildroot_initramfs_sysroot)
# so vpubuild should be built seperately
vpubuild: $(vmlinux) wave511-build wave521-build omxil-build gstomx-build vpudriver-build
wave511-build:
	$(MAKE) -C $(buildroot_initramfs_wrkdir) O=$(buildroot_initramfs_wrkdir) wave511-dirclean
	$(MAKE) -C $(buildroot_initramfs_wrkdir) O=$(buildroot_initramfs_wrkdir) wave511-rebuild
wave521-build:
	$(MAKE) -C $(buildroot_initramfs_wrkdir) O=$(buildroot_initramfs_wrkdir) wave521-dirclean
	$(MAKE) -C $(buildroot_initramfs_wrkdir) O=$(buildroot_initramfs_wrkdir) wave521-rebuild
omxil-build:
	$(MAKE) -C $(buildroot_initramfs_wrkdir) O=$(buildroot_initramfs_wrkdir) sf-omx-il-dirclean
	$(MAKE) -C $(buildroot_initramfs_wrkdir) O=$(buildroot_initramfs_wrkdir) sf-omx-il-rebuild
gstomx-build:
	$(MAKE) -C $(buildroot_initramfs_wrkdir) O=$(buildroot_initramfs_wrkdir) sf-gst-omx-dirclean
	$(MAKE) -C $(buildroot_initramfs_wrkdir) O=$(buildroot_initramfs_wrkdir) sf-gst-omx-rebuild
vpudriver-build:
	$(MAKE) -C $(buildroot_initramfs_wrkdir) O=$(buildroot_initramfs_wrkdir) wave511driver
	$(MAKE) -C $(buildroot_initramfs_wrkdir) O=$(buildroot_initramfs_wrkdir) wave521driver

.PHONY: initrd buildroot_initramfs
initrd: $(initramfs)

buildroot_initramfs: $(initramfs).d

$(initramfs).d: $(buildroot_initramfs_sysroot) $(buildroot_initramfs_tar)
	touch $@

initramfs_cpio := $(wrkdir)/initramfs.cpio
# Generate initramfs.cpio.gz after buildroot and kernel had been compiled
$(initramfs): $(initramfs).d
	pushd $(_BOARD_LINUX_SRC_DIR); \
		$(linux_srcdir)/usr/gen_initramfs.sh \
		-l $(initramfs).d \
		-o $(initramfs_cpio) -u $(shell id -u) -g $(shell id -g) \
		$(confdir)/initramfs.txt \
		$(buildroot_initramfs_sysroot); \
	(cat $(initramfs_cpio) | gzip -n -9 -f - > $@) || (rm -f $@; echo "Error: Fail to compress $(initramfs_cpio)"; exit 1); \
	rm -f $(initramfs_cpio); \
	popd

# use buildroot_initramfs toolchain
# TODO: fix path and conf/buildroot_rootfs_config
$(buildroot_rootfs_wrkdir)/.config: $(buildroot_srcdir) $(buildroot_initramfs_tar)
#	rm -rf $(dir $@)
	mkdir -p $(dir $@)
	cp $(buildroot_rootfs_config) $@
	$(MAKE) -C $< RISCV=$(RISCV) PATH=$(RVPATH) O=$(buildroot_rootfs_wrkdir) olddefconfig

$(buildroot_rootfs_ext): $(buildroot_srcdir) $(buildroot_rootfs_wrkdir)/.config $(target_gcc) $(buildroot_rootfs_config)
	$(MAKE) -C $< RISCV=$(RISCV) PATH=$(RVPATH) O=$(buildroot_rootfs_wrkdir)

.PHONY: buildroot_rootfs
buildroot_rootfs: $(buildroot_rootfs_ext)
#	cp $< $@

$(rootfs): $(buildroot_rootfs_ext)
	cp $< $@

.PHONY: buildroot_rootfs-menuconfig
buildroot_rootfs-menuconfig: $(buildroot_rootfs_wrkdir)/.config $(buildroot_srcdir)
	$(MAKE) -C $(dir $<) O=$(buildroot_rootfs_wrkdir) menuconfig
	$(MAKE) -C $(dir $<) O=$(buildroot_rootfs_wrkdir) savedefconfig
	cp $(dir $<)defconfig conf/buildroot_rootfs_config

vpubuild_rootfs: $(vmlinux) wave511-build-rootfs wave521-build-rootfs omxil-build-rootfs gstomx-build-rootfs vpudriver-build-rootfs
wave511-build-rootfs:
	$(MAKE) -C $(buildroot_rootfs_wrkdir) O=$(buildroot_rootfs_wrkdir) wave511-dirclean
	$(MAKE) -C $(buildroot_rootfs_wrkdir) O=$(buildroot_rootfs_wrkdir) wave511-rebuild
wave521-build-rootfs:
	$(MAKE) -C $(buildroot_rootfs_wrkdir) O=$(buildroot_rootfs_wrkdir) wave521-dirclean
	$(MAKE) -C $(buildroot_rootfs_wrkdir) O=$(buildroot_rootfs_wrkdir) wave521-rebuild
omxil-build-rootfs:
	$(MAKE) -C $(buildroot_rootfs_wrkdir) O=$(buildroot_rootfs_wrkdir) sf-omx-il-dirclean
	$(MAKE) -C $(buildroot_rootfs_wrkdir) O=$(buildroot_rootfs_wrkdir) sf-omx-il-rebuild
gstomx-build-rootfs:
	$(MAKE) -C $(buildroot_rootfs_wrkdir) O=$(buildroot_rootfs_wrkdir) sf-gst-omx-dirclean
	$(MAKE) -C $(buildroot_rootfs_wrkdir) O=$(buildroot_rootfs_wrkdir) sf-gst-omx-rebuild
vpudriver-build-rootfs:
	$(MAKE) -C $(buildroot_rootfs_wrkdir) O=$(buildroot_rootfs_wrkdir) wave511driver
	$(MAKE) -C $(buildroot_rootfs_wrkdir) O=$(buildroot_rootfs_wrkdir) wave521driver

