include ../../make/Makefile
ORG?=osmocom

PKG:= osmo-build osmo-slim osmo-trx osmo-bts osmo-nitb

.PHONY: $(PKG)
$(PKG):
	ORG=$(ORG) make -C $@ docker-build

.PHONY: push
push:
	 @set -e; for d in $(PKG); do docker push "$(ORG)/$$d:latest"; done

all: $(PKG)
