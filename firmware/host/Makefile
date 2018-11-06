
#Makefile for ocmw and cli

CC=gcc
CFLAGS=  -I./ocwdg -I./include -I./../ec -I./../ec/common -I./../ec/platform/oc-sdr/schema -I./../ec/common/inc/global -Wall -Werror -g -fshort-enums
LIBS =   -lpthread -ledit -lm -lrt
src_ocmw = $(wildcard ocmw/*.c) $(wildcard ocwdg/*.c) $(wildcard util/*.c) $(wildcard ../ec/platform/oc-sdr/schema/*.c)
src_occli = $(wildcard occli/*.c) $(wildcard util/*.c) $(wildcard ../ec/platform/oc-sdr/schema/*.c)
src_ocware_stub = $(wildcard ocwarestub/*.c) $(wildcard ../ec/platform/oc-sdr/schema/*.c)
output_dir = bin

# --- Build  binary and executable files

default:
	@mkdir -p $(output_dir)
	make all
	
all:	get_cfg ocmw_uart ocmw_usb ocmw_eth occli ocmw_eth_stub ocware_stub

get_cfg:
	cd ../utilities/build; ./envDetect.sh
	
.PHONY:	occli

ocmw_uart:   $(OBJECTS)
	$(CC)  $(src_ocmw) -o $(output_dir)/ocmw_uart  $(LIBS) $(CFLAGS) -DOCWARE_HOST

ocmw_usb:   $(OBJECTS)
	$(CC)  $(src_ocmw) -o $(output_dir)/ocmw_usb  $(LIBS) $(CFLAGS) -DINTERFACE_USB -DOCWARE_HOST

ocmw_eth:   $(OBJECTS)
	$(CC) $(src_ocmw) -o $(output_dir)/ocmw_eth  $(LIBS) $(CFLAGS) -DINTERFACE_ETHERNET -DOCWARE_HOST
	
ocmw_eth_stub:   $(OBJECTS)
	$(CC) $(src_ocmw) -o $(output_dir)/ocmw_eth_stub  $(LIBS) $(CFLAGS) -DINTERFACE_STUB_EC -DOCWARE_HOST

ocware_stub:   $(OBJECTS)
	$(CC)  $(src_ocware_stub) -o $(output_dir)/ocware_stub  $(LIBS) $(CFLAGS) -DOCWARE_HOST

occli:   $(OBJECTS)
		$(CC)  $(src_occli) -o $(output_dir)/occli   $(LIBS) $(CFLAGS) -DOCWARE_HOST
		ln -sf occli $(output_dir)/occmd


# --- remove binary and executable files
clean:
	rm -fr $(output_dir)/ocmw_eth_stub $(output_dir)/ocmw_eth $(output_dir)/ocmw_usb $(output_dir)/ocmw_uart $(output_dir)/occli $(output_dir)/ocware_stub
