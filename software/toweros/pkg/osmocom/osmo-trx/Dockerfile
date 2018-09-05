FROM osmocom/osmo-build:latest AS build

RUN	apt-get update && \
        apt-get -y install --no-install-recommends \
		telnet \
		libosmocore-dev \
                python-mako \
                doxygen \
                python-docutils \
                python-numpy \
                python-requests \
                python-setuptools \
                python-dev \
                libboost-all-dev \
                cmake \
                build-essential \
		libboost-dev \
		libudev-dev && \
	apt-get clean

WORKDIR	/tmp

ARG     UHD_BRANCH="UHD-3.9.LTS"
RUN     git clone git://github.com/EttusResearch/uhd.git
COPY    uhd_ocr01.patch /tmp/uhd_ocr01.patch

RUN     cd uhd/host && \
#	git fetch && git checkout -f -B $UHD_BRANCH origin/$UHD_BRANCH && \
	git checkout 180d5f3ad9db94bc61688014c0a52eeb5b649ee0 && \
        git apply /tmp/uhd_ocr01.patch && \
        mkdir build && \
        cd build && \
        cmake -DUHD_LOG_CONSOLE_LEVEL=trace -DUHD_LOG_MIN_LEVEL=trace -DCMAKE_INSTALL_PREFIX=/opt/uhd ../ && \
        make -j8 install

ARG	OSMO_TRX_BRANCH="master"
ARG     UHD_INSTALL_PATH=/opt/uhd
ARG     OSMO_INSTALL_PATH=/opt/osmo

RUN	git clone git://git.osmocom.org/osmo-trx.git
ADD	http://git.osmocom.org/osmo-trx/patch?h=$OSMO_TRX_BRANCH /tmp/commit-osmo-trx
COPY    osmotrx_ocr01.patch /tmp/osmotrx_ocr01.patch

RUN	cd osmo-trx && \
	#git fetch && git checkout -f -B $OSMO_TRX_BRANCH origin/$OSMO_TRX_BRANCH && \
        git checkout 2dee3e996e777b67aa3185f7456c041765f0d71f && \
        git apply /tmp/osmotrx_ocr01.patch && \
	autoreconf -fi && \
	./configure --prefix=$OSMO_INSTALL_PATH PKG_CONFIG_PATH=$UHD_INSTALL_PATH/lib/pkgconfig && \
	make -j8 install

FROM osmocom/osmo-slim:latest
ENTRYPOINT []
CMD []
VOLUME /data
WORKDIR /data

RUN	apt-get update && \
	apt-get install -y --no-install-recommends \
                libboost-program-options1.55.0 \
                libboost-filesystem1.55.0 \
                libboost-date-time1.55.0 \
                libboost-system1.55.0 \
                libboost-regex1.55.0 \
                libboost-test1.55.0 \
                libboost-thread1.55.0 \
                libboost-chrono1.55.0 \
                libboost-atomic1.55.0 \
                libboost-serialization1.55.0 \
		libusb-1.0 \
                libfftw3-single3 \
                libosmocore \
                libsqlite3-0 \
                python \
		usbutils \
		udev \
                python-requests && \
	apt-get clean

COPY --from=build /opt/osmo/ /usr/local/
COPY --from=build /opt/uhd/ /usr/local/
RUN     ldconfig
#RUN     uhd_images_downloader
COPY    usrp_*    /opt/uhd/share/uhd/images/
COPY    uhd-usrp.rules  /etc/udev/rules.d

CMD	["osmo-trx"]
#CMD ["ls", "/bin"]
