# Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

TARGETS	= cbootimage bct_dump
CC		= gcc
CFLAGS	= -Wall -O -lm

all: $(TARGETS)

#
# Build the cbootimage tool.
#
CBOOTIMAGE_C_FILES := cbootimage.c
CBOOTIMAGE_C_FILES += data_layout.c
CBOOTIMAGE_C_FILES += set.c
CBOOTIMAGE_C_FILES += crypto.c
CBOOTIMAGE_C_FILES += aes_ref.c
CBOOTIMAGE_C_FILES += context.c
CBOOTIMAGE_C_FILES += parse.c
CBOOTIMAGE_C_FILES += t30/parse_t30.c
CBOOTIMAGE_C_FILES += t20/parse_t20.c
CBOOTIMAGE_C_FILES += t30/nvbctlib_t30.c
CBOOTIMAGE_C_FILES += t20/nvbctlib_t20.c

CBOOTIMAGE_OBJS	:= $(patsubst %.c,%.o,$(CBOOTIMAGE_C_FILES))

cbootimage: $(CBOOTIMAGE_OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

#
# Build the bct_dump tool.  This tool generates a human readable version of
# the given BCT file.
#
BCT_DUMP_C_FILES := bct_dump.c
BCT_DUMP_C_FILES += data_layout.c
BCT_DUMP_C_FILES += set.c
BCT_DUMP_C_FILES += crypto.c
BCT_DUMP_C_FILES += aes_ref.c
BCT_DUMP_C_FILES += context.c
BCT_DUMP_C_FILES += parse.c
BCT_DUMP_C_FILES += t30/parse_t30.c
BCT_DUMP_C_FILES += t20/parse_t20.c
BCT_DUMP_C_FILES += t30/nvbctlib_t30.c
BCT_DUMP_C_FILES += t20/nvbctlib_t20.c

BCT_DUMP_OBJS	:= $(patsubst %.c,%.o,$(BCT_DUMP_C_FILES))

bct_dump: $(BCT_DUMP_OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

#
# Remove built targets, object files and temporary editor files.
#
clean:
	rm -f $(CBOOTIMAGE_OBJS) $(BCT_DUMP_OBJS) *~ $(TARGETS)
