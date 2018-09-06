linuxkit:
	make -C tools all
	ln -s tools/linuxkit/bin/linuxkit .


.PHONY: boot
boot:
	cd boot/compose/osmo-nitb && ../../../linuxkit build -disable-content-trust -format iso-bios -dir ../../.. -name osmo-nitb ../../oc-minimal.yml ../../docker.yml build.yml

.PHONY: vm
vm:
	cd boot/compose/osmo-nitb && ../../../linuxkit build -disable-content-trust -format iso-bios -name vm -dir ../../.. ../../oc-minimal.yml ../../docker.yml build.yml ../../vm.yml


PKG:= osmo-build osmo-slim osmo-trx osmo-bts osmo-nitb
.PHONY: $(PKG)
$(PKG):
	make -C pkg/$@ docker-build
pkg: $(PKG)


all: linuxkit pkg boot vm
