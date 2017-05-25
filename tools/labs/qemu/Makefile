QEMU_DISPLAY ?= none
ARCH ?= x86
ifeq ($(ARCH),x86)
b = b
endif

YOCTO_URL=http://downloads.yoctoproject.org/releases/yocto/yocto-2.3/machines/qemu/qemu$(ARCH)/
YOCTO_IMAGE=core-image-minimal-qemu$(ARCH).ext4
#YOCTO_IMAGE=core-image-minimal-dev-qemu$(ARCH).ext4
#YOCTO_IMAGE=core-image-sato-dev-qemu$(ARCH).ext4
#YOCTO_IMAGE=core-image-sato-qemu$(ARCH).ext4
#YOCTO_IMAGE=core-image-sato-sdk-qemu$(ARCH).ext4

QEMU_OPTS = -kernel zImage \
	-device virtio-serial \
	-chardev pty,id=virtiocon0 -device virtconsole,chardev=virtiocon0 \
	-net nic,model=virtio,vlan=0 -net tap,ifname=tap0,vlan=0,script=no,downscript=no \
	-drive file=rootfs.img,if=virtio,format=raw \
	--append "root=/dev/vda console=hvc0" \
	--display $(QEMU_DISPLAY) -s

boot: rootfs.img zImage tap0
	ARCH=$(ARCH) qemu/qemu.sh $(QEMU_OPTS)

TEMPDIR := $(shell mktemp -u)

zImage:
	cp qemu/kernel_config.x86 $(KDIR)/.config
	$(MAKE) -C $(KDIR) oldnoconfig && $(MAKE) -C $(KDIR)
	ln -s $(KDIR)/arch/$(ARCH)/boot/$(b)zImage zImage

rootfs.img: $(YOCTO_IMAGE) zImage
	$(MAKE) -C $(KDIR) modules
	mkdir $(TEMPDIR)
	sudo mount -t ext4 -o loop $(YOCTO_IMAGE) $(TEMPDIR)
	sudo $(MAKE) -C $(KDIR) modules_install INSTALL_MOD_PATH=$(TEMPDIR)
	sudo umount $(YOCTO_IMAGE)
	rmdir $(TEMPDIR)
	ln -f $(YOCTO_IMAGE) rootfs.img

gdb: vmlinux
	gdb -ex "target remote localhost:1234" vmlinux

vmlinux: zImage
	ln -fs $(KDIR)/vmlinux vmlinux

$(YOCTO_IMAGE):
	wget $(YOCTO_URL)/$(YOCTO_IMAGE)
	sudo qemu/prepare-image.sh $(YOCTO_IMAGE)

tap0:
	qemu/create_net.sh $@

.PHONY: run clean tap0

clean:
	rm -f zImage.* vmlinux.* rootfs.*.img $(KDIR)/.config

clean-images:
	rm -f core-image-*.ext4
