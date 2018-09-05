include ../../make/Makefile
ORG=linuxkit

linuxkit:
	git clone https://github.com/linuxkit/linuxkit \

patch:  linuxkit
	cd linuxkit/ \
       	&& git checkout 0278d74e4119a03dcc9dfaead4fe9ef6b34fddf4 \
	&& git am < ../0001-Patch-containerd-commit-version.patch \
	&& git am < ../0001-Add-containerd-restart-plugin-over-moby-services.patch \
	&& git am < ../0001-Update-Compose-to-API-2.3.patch

linuxkit/tools/alpine/hash: patch
	make -C linuxkit/tools/alpine build \
	&& cd linuxkit \
	&& git add tools \
	&& git commit --amend -m "Updating versions"
          
OLD_BUILD=1f8437a5927e96ffc42be730dd6da4d0d39314a4-amd64
patch-alpine-children: linuxkit/tools/alpine/hash 
	cd linuxkit/ \
	&& grep -rl $(OLD_BUILD) . | xargs  sed -i 's/$(OLD_BUILD)/$(shell cat linuxkit/tools/alpine/hash)/g' || exit 0 \
	&& git add pkg/ \
	&& git commit --amend -m "update alpine pkg hashes"

SRCS:= containerd init
$(SRCS): patch-alpine-children
	linuxkit pkg build $(OPTIONS) "linuxkit/pkg/$@" \
	&& linuxkit pkg show-tag linuxkit/pkg/$@ > $@

pkgs: $(SRCS)

compose:
	ORG=linuxkitprojects make -C linuxkit/projects/compose tag \
	&& ORG=linuxkitprojects make -C linuxkit/projects/compose hash | tail -n2 | head -n1 > $@ 
all: linuxkit/tools/alpine/hash pkgs compose

.PHONY: clean
clean: 
	rm -rf linuxkit containerd init compose
