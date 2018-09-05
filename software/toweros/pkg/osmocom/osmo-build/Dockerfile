FROM debian:jessie

MAINTAINER Harald Welte <laforge@gnumonks.org>

RUN	apt-get update && \
	apt-get upgrade -y && \
	apt-get install -y --no-install-recommends \
		autoconf \
		autoconf-archive \
		autogen \
		automake \
		bison \
		build-essential \
		cppcheck \
		debhelper \
		devscripts \
		dh-autoreconf \
		dh-systemd \
		doxygen \
		flex \
		g++ \
		gawk \
		gcc \
		git \
		git-buildpackage \
		libc-ares-dev \
		libdbd-sqlite3 \
		libdbi-dev \
		libfftw3-dev \
		libgnutls28-dev \
		libgps-dev \
		libgsm1-dev \
		libncurses5-dev \
		libortp-dev \
		libpcap-dev \
		libpcsclite-dev \
		libtalloc-dev \
		libtool \
		libreadline-dev \
		libsctp-dev \
		libsofia-sip-ua-glib-dev \
		libsqlite3-dev \
		libssl-dev \
		libusb-dev \
		libusb-1.0-0-dev \
		make \
		pkg-config \
		sqlite3 \
		wget && \
	apt-get clean

ARG     OSMOCOM_REPO="http://download.opensuse.org/repositories/network:/osmocom:/nightly/Debian_8.0/"
COPY    Release.key /tmp/Release.key
RUN     apt-key add /tmp/Release.key && \
        rm /tmp/Release.key && \
        echo "deb " $OSMOCOM_REPO " ./" > /etc/apt/sources.list.d/osmocom-nightly.list
ADD     $OSMOCOM_REPO/Release /tmp/Release
