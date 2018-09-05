include ../make/Makefile

SRCS:= osmocom linuxkit

.PHONY: $(SRCS)
$(SRCS):
	cd $@ && make all 
all: $(SRCS)


clean:
	make -C linuxkit clean
