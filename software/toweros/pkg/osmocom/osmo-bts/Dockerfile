FROM osmocom/osmo-build:latest AS build

RUN	apt-get update && \
	apt-get install -y --no-install-recommends \
		telnet \
		libosmocore-dev \
		libosmo-abis-dev \
		libosmo-netif-dev \
		libosmo-sccp-dev \
		libsmpp34-dev \
		libgtp-dev && \
	apt-get clean

WORKDIR	/tmp

ARG	OSMO_BTS_BRANCH="master"

RUN	git clone git://git.osmocom.org/osmo-bts.git
ADD	http://git.osmocom.org/osmo-bts/patch?h=$OSMO_BTS_BRANCH /tmp/commit-osmo-bts

RUN	cd osmo-bts && \
	git fetch && git checkout -f -B $OSMO_BTS_BRANCH origin/$OSMO_BTS_BRANCH && \
	autoreconf -fi && \
	./configure --enable-trx --prefix=/opt/osmo && \
	make -j8 install

FROM osmocom/osmo-slim:latest
RUN	apt-get update && \
	apt-get install -y --no-install-recommends \
		libosmo-abis \
		libosmocore && \
        apt-get clean

COPY --from=build /opt/osmo/ /usr/local/
VOLUME	/data

COPY	osmo-bts.cfg /data/osmo-bts.cfg

WORKDIR	/data
	# send GSMTAP data to .230 which is the ttcn3-sysinfo test
CMD	["osmo-bts-trx", "-c", "osmo-bts.cfg"]

#EXPOSE	
