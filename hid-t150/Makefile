obj-m += hid-t150.o
KDIR ?= /lib/modules/$(shell uname -r)/build
KBUILD_ARGS := $(shell grep -q '^CONFIG_CC_IS_CLANG=y' $(KDIR)/include/config/auto.conf 2>/dev/null && printf '%s' 'LLVM=1')

all:
	$(MAKE) -C $(KDIR) M=$(CURDIR) $(KBUILD_ARGS) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURDIR) $(KBUILD_ARGS) clean
