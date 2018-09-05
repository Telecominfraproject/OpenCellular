include make/Makefile

.PHONY: boot
boot:
	make -C boot all

.PHONY: pkg
pkg: 
	make -C pkg all

push:
	make -C pkg/osmocom push

all: pkg boot


clean:
	make -C pkg boot clean
