TARGETS	= cbootimage bct_dump
CC		= gcc
CFLAGS	= -Wall -O -lm

all: $(TARGETS)

#
# Build the cbootimage tool.
#
CBOOTIMAGE_C_FILES := cbootimage.c
CBOOTIMAGE_C_FILES += nvbctlib_ap20.c
CBOOTIMAGE_C_FILES += data_layout.c
CBOOTIMAGE_C_FILES += parse.c
CBOOTIMAGE_C_FILES += set.c
CBOOTIMAGE_C_FILES += crypto.c
CBOOTIMAGE_C_FILES += aes_ref.c
CBOOTIMAGE_C_FILES += context.c

CBOOTIMAGE_OBJS	:= $(patsubst %.c,%.o,$(notdir $(CBOOTIMAGE_C_FILES)))

cbootimage: $(CBOOTIMAGE_OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

#
# Build the bct_dump tool.  This tool generates a human readable version of
# the given BCT file.
#
BCT_DUMP_C_FILES := bct_dump.c
BCT_DUMP_C_FILES += nvbctlib_ap20.c
BCT_DUMP_C_FILES += data_layout.c
BCT_DUMP_C_FILES += parse.c
BCT_DUMP_C_FILES += set.c
BCT_DUMP_C_FILES += crypto.c
BCT_DUMP_C_FILES += aes_ref.c
BCT_DUMP_C_FILES += context.c

BCT_DUMP_OBJS	:= $(patsubst %.c,%.o,$(notdir $(BCT_DUMP_C_FILES)))

bct_dump: $(BCT_DUMP_OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

#
# Remove built targets, object files and temporary editor files.
#
clean:
	rm -rf *.o *~ $(TARGETS)
