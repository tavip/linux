KDIR=$(shell realpath $(PWD)/../..)

LABS ?= .

all: build_modules

include qemu/Makefile

build_modules: vmlinux $(shell find $(LABS) -name Kbuild | xargs dirname)
	for i in $^; do $(MAKE) -C $(KDIR) M=$(PWD)/$$i; done

clean_modules: $(shell find $(LABS) -name Kbuild | xargs dirname)
	for i in $^; do $(MAKE) -C $(KDIR) M=$(PWD)/$$i clean; done

docs:
	$(MAKE) -C $(KDIR) DOCBOOKS= SPHINXDIRS=labs htmldocs
