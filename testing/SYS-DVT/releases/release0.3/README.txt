Purpose:
-------
To support manual and automation requirement in terms of providing an integrated software interface to ease testing and debugging.
System DVT support software is built around a python CLI framework.

Scope:
-----
The implementation provides a reliable and time efficient way to present the interested metrics to the manual and automation teams. 
The software package is expected to be used only for system DVT testing.

Folder structure:
----------------
patches/:
-------
Patch files to be applied to the tip-sdk before building uboot/kernel/rootfs.
WARNING: Apply patches first and build the uboot, kernel, rootfs before any of the below steps.

install.sh:
----------
- Generates lsm_rd_dvt.gz from given lsm_rd.gz
- MUST run the script with sudo
- It install dvt pakages, scripts, files from pkgs/ in to lsm_rd.gz file.
- install.sh <directory of lsm_rd.gz> accepts exactly 1 arguments which is path of lsm_rd.gz
  Ex: sudo ./install.sh /home/oc/tip-sdk/out

pkgs/:
All DVT related suppport packages

Linux Driver:
drivers/misc/dvt.ko - Currently supports receiving BB alert interrupts

Note: The newly build u-boot has to be flashed on the board to successfully load kernel and rootfs.

DVT test steps:
--------------
1. Flash the u-boot
2. Change the namedalloc env in u-boot (refer uboot_dts.patch)
3. Load the lsm_os.gz and lsm_rd.gz through TFTP boot
4. cd dvt/
5.
# python occmd.py help
./occmd.py <module_name> <get/set> <param/all> <value>

Example:

# python occmd.py bb get all
current : 1086 mA
bus_voltage : 12966 mV
temperature : 32500 mCelcius
temperature_alert : 80000 mCelcius
temperature_hyst : 75000 mCelcius
shunt_voltage : 2 mV
bb_alert_enable : 0  interrupt_flag
current_alert : 0 mA

# python occmd.py bb get current
current : 1080 mA

# python occmd.py bb set temperature_alert 70000
echo 70000 > /sys/devices/soc.0/1180000001000.i2c/i2c-0/0-0048/temp1_max

# python occmd.py reliability

Monitoring BB Current/Temperature interrupt

Running reliabilty ..

<In another terminal set the current alert and temp alert using the python scripti;below is o/p >
CURRENT ALERT: Limit 200 Actual input current 1010

CURRENT ALERT: Limit 200 Actual input current 1003

TEMPERATURE ALERT: Limit 20000 Actual board temp 32500

CURRENT ALERT: Limit 200 Actual input current 1003

TEMPERATURE ALERT: Limit 20000 Actual board temp 32500

