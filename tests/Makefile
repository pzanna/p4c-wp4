obj-m+=wp4-p4.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
modules_install:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules_install
install:
	install -p -m 644 wp4-p4.ko /lib/modules/$(shell uname -r)/kernel/drivers/net/wireless/wp4/
	/sbin/depmod -a $(shell uname -r)
