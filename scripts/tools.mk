# SPDX-License-Identifier: GPL-2.0

#$(libfesvr): $(fesvr_srcdir)
#	rm -rf $(fesvr_wrkdir)
#	mkdir -p $(fesvr_wrkdir)
#	mkdir -p $(dir $@)
#	cd $(fesvr_wrkdir) && $</configure \
#		--prefix=$(dir $(abspath $(dir $@)))
#	$(MAKE) -C $(fesvr_wrkdir)
#	$(MAKE) -C $(fesvr_wrkdir) install
#	touch -c $@

#$(spike): $(spike_srcdir) $(libfesvr)
#	rm -rf $(spike_wrkdir)
#	mkdir -p $(spike_wrkdir)
#	mkdir -p $(dir $@)
#	cd $(spike_wrkdir) && PATH=$(RVPATH) $</configure \
#		--prefix=$(dir $(abspath $(dir $@))) \
#		--with-fesvr=$(dir $(abspath $(dir $(libfesvr))))
#	$(MAKE) PATH=$(RVPATH) -C $(spike_wrkdir)
#	$(MAKE) -C $(spike_wrkdir) install
#	touch -c $@

#$(qemu): $(qemu_srcdir)
#	rm -rf $(qemu_wrkdir)
#	mkdir -p $(qemu_wrkdir)
#	mkdir -p $(dir $@)
#	which pkg-config
#	# pkg-config from buildroot blows up qemu configure
#	cd $(qemu_wrkdir) && $</configure \
#		--prefix=$(dir $(abspath $(dir $@))) \
#		--target-list=riscv64-softmmu
#	$(MAKE) -C $(qemu_wrkdir)
#	$(MAKE) -C $(qemu_wrkdir) install
#	touch -c $@

