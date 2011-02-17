C_FILES :=
C_FILES += nvbctlib_ap20.c
C_FILES += cbootimage.c
C_FILES += data_layout.c
C_FILES += parse.c
C_FILES += set.c
C_FILES += crypto.c
C_FILES += aes_ref.c
C_FILES += context.c

OBJS := $(patsubst %.c,%.o,$(notdir $(C_FILES)))

TARGET = cbootimage
CC = gcc
CFLAGS=-Wall -O

$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(CFLAGS)

clean:
	rm -rf *.o $(TARGET)
