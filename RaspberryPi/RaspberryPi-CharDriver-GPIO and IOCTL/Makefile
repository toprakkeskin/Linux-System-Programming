MODULE_FORCE_UNLOAD=y

obj-m = RaspLedCharDev.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean :
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

