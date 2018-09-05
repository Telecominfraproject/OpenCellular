include ../make/Makefile

#GOPATH=$(CURDIR)/go

.PHONY: linuxkit-bin
linuxkit-bin: 
	make -C linuxkit all 
	export PATH=$(CURDIR)/linuxkit/bin:$(PATH)

alpine:
	ORG=$(ORG) make -C linuxkit/tools/alpine build 

PKGS:=init containerd
.PHONY: $(PKGS)
$(PKGS):
	cd ./linuxkit && ./bin/linuxkit pkg build -org $(ORG) pkg/$@
pkg: $(PKGS)

.PHONY: compose
compose:
	ORG=$(ORG) make -C linuxkit/projects/compose/ tag

linuxkit-sub: alpine $(PKGS) compose


all: linuxkit-bin linuxkit-sub
