COMPILER_PATH = /home/oc_lab/tools/gcc-linaro-5.5.0-2017.10-x86_64_arm-linux-gnueabihf/bin
CC = $(COMPILER_PATH)/arm-linux-gnueabihf-gcc
AR = $(COMPILER_PATH)/arm-linux-gnueabihf-ar
LD = $(COMPILER_PATH)/arm-linux-gnueabihf-ld
LDPATH = $(COMPILER_PATH)/../libc/usr/lib 
TARGET = ocpbsp.bin 
CFLAGS = -g 
LIBS = -lpthread
$(TARGET): main.o libocpbsp.a
	$(CC) -L$(LDPATH) $(LIBS) $^ -o $@ 

main.o: main.c
	$(CC) -c $(CFLAGS) $< -o $@

libocpbsp.a: bspMsgHandler.o ocps_bsp.o ocps_slip.o ocps_uart.o slip.o
	$(AR) rcs $@ $^
%.o: %.c
	$(CC) -c $(CFLAGS) $(LIBS) $< -o $@

clean:
	rm -f *.bin *.o *.a $(TARGET)
