oc-minimal.yml:
	export INIT_BUILD=$(shell cat ../pkg/linuxkit/init) \
	&& export CONTAINERD_BUILD=$(shell cat ../pkg/linuxkit/containerd) \
	&& cat $@.tpl | envsubst > $@

docker.yml:
	export COMPOSE_BUILD=linuxkitprojects/compose:$(shell cat ../pkg/linuxkit/compose) \
	&& cat $@.tpl | envsubst > $@

compose/osmo-nitb/docker-compose.yml:
	export ORG=$(ORG) \
	&& cat $@.tpl | envsubst > $@

box.iso: oc-minimal.yml docker.yml compose/osmo-nitb/docker-compose.yml
	cd compose/osmo-nitb && linuxkit build -disable-content-trust -format iso-bios -dir ../../ -name box ../../oc-minimal.yml ../../docker.yml ../../box.yml build.yml

vm.iso: oc-minimal.yml docker.yml compose/osmo-nitb/docker-compose.yml
	cd compose/osmo-nitb && linuxkit build -disable-content-trust -format iso-bios -dir ../../ -name vm ../../oc-minimal.yml ../../docker.yml ../../vm.yml build.yml

clean: 
	rm -f *.iso oc-minimal.yml docker.yml compose/osmo-nitb/docker-compose.yml


all: vm.iso box.iso
