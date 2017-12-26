obj-m+=aiocpy.o

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules
	$(CC) aiocpy_test.c -o aiocpy_test

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean
	rm -f aiocpy_test aiocpy_test.o
