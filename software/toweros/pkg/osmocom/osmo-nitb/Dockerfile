FROM osmocom/osmo-slim:latest

RUN	apt-get update && \
	apt-get install -y --no-install-recommends \
		telnet \
		osmocom-nitb && \
	apt-get clean

WORKDIR	/tmp

VOLUME	/data
VOLUME	/var/state

COPY	openbsc.cfg /data/openbsc.cfg

WORKDIR	/data
CMD	["osmo-nitb", "-c", "/data/openbsc.cfg", "-l", "/var/state/hlr.sqlite3" ]

EXPOSE	3003 3002 4242
