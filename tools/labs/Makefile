KDIR=$(shell realpath $(PWD)/../..)

LABS ?= .

build_modules: $(shell find $(LABS) -name Kbuild | xargs dirname)
	for i in $^; do $(MAKE) -C $(KDIR) M=$(PWD)/$$i; done

clean_modules: $(shell find $(LABS) -name Kbuild | xargs dirname)
	for i in $^; do $(MAKE) -C $(KDIR) M=$(PWD)/$$i clean; done

include qemu/Makefile
