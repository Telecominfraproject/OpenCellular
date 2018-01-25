# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

package mods::chain::c1;
#
#	File: c1.pm
#
#	This file has the pon and tests sequences used by
#	mods::fbt_config.
#
use Exporter;
our (@ISA, @EXPORT_OK);
@ISA		= qw/Exporter/;
@EXPORT_OK	= qw/$C1pwr $C1tests/;
my $Rpath	= '/home/oc/host/ocmw';
my $Upath1	= '/usr/local/osmo_built/lib/uhd/utils';
my $Prompt	= $ENV{LOGIN_PROMPT} // ':~\$ ';
my $Tsen	= qr/Temp\sSensor/;
my $Found	= qr/\s{2,}DEV\sFOUND/;
my $Miss	= qr/\s{2,}DEV\sMISSING/;
my $Cctl	= qr/Charge\s+Controller/;
my $Isen	= qr/Current\sSensor/;

our $C1pwr	= [	# Please turn on power.
	{
		prog	=> '/usr/local/fbin/acpwr_on',
		args	=> [4, 5],
		xcode	=> 0,
		name	=> 'Turn on IP load and fans',
	},
	{
		prog	=> '/usr/local/fbin/p1apply',
		args	=> [18, 3.5],
		xcode	=> 0,
		name	=> 'PS1 to 18 volts, 3.5 amps',
	},
	{
		prog	=> '/usr/local/fbin/p1stat',
		res	=> [
			{
			name	=> 'start.std.p1v',
			re	=> qr/,\s([\.\d]+)\s+volts/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'start.std.p1i',
			re	=> qr/p1:\s([\.\d]+)\s+amperes/m,
			func	=> \&mods::walker::range_match,
			},
			],
		xcode	=> 0,
		name	=> 'ps1stat',
	},
	];

our $C1tests	= [
	{
		prog	=> '/usr/bin/sudo',
		args	=> ['/bin/pwd'],
		xcode	=> 0,
		name	=> 'Establish sudo',
		to	=> 3,
	},
	{	# These HRs (an HR is a Perl hash reference) define test cases.
		# Each HR has all info needed to run a test case.
		# The prog key points to the execuable.
		# A shell test -x will be run before its launch.
		prog	=> '/usr/bin/sudo',
		args	=> ['/bin/date'],
		timereq	=> 1,
		res	=> [
			{
				name	=> 'verify time',
				re	=> qr/^([MTWFS][a-z]{2}\s.+)$/m,
				func	=> \&mods::walker::tod_match,
			},
		],
		xcode	=> 0,
		name	=> 'Set time of day',
		to	=> 3,
	},
	{	
		prog	=> '/usr/bin/sudo',
		args	=> ['/usr/sbin/dmidecode', '-t', 'BIOS'],
		res	=> [
			{
				name	=> 'verify version 4.4',
				re	=> qr/Version: 4.4-/,
				func	=> \&mods::walker::std_match,
			},
			{
				name	=> 'dmidecode.coreboot.build',
				re	=> qr/Version: 4.4-(\d+)/,
				func	=> \&mods::walker::range_match,
			},
		],
		xcode	=> 0,
		tid	=> 'TC_GBC_01',
		name	=> 'Check core boot verion',
	},
	{	
		prog	=> '/usr/bin/sudo',
		args	=> ['/usr/sbin/smartctl', '-i', '/dev/sda'],
		to	=> 4,
		res	=> [
			{
				name	=> 'Find model',
				re	=> qr/Model:\s+(.+?)$/m,
				func	=> \&mods::walker::std_1_match,
			},
			{
				name	=> 'Find sn',
				re	=> qr/Number:\s+(.+?)$/m,
				func	=> \&mods::walker::std_1_match,
			},
			{
				name	=> 'Find size',
				re	=> qr/Capacity:\s+(.+?)$/m,
				func	=> \&mods::walker::std_1_match,
			},
			{
				name	=> 'msata.size',
				re	=> qr/Capacity:.+?\[(\S+)/m,
				func	=> \&mods::walker::range_match,
			},
			{
				name	=> 'msata.speed',
				re	=> qr/\(current:\s+(\d+\.\d+)\s+Gb/m,
				func	=> \&mods::walker::range_match,
			},
		],
		xcode	=> 0,
		# tid	=> 'TC_GBC_09',
		tid	=> 'TC_GBC_02',
		name	=> 'mSATA size, vendor, speed',
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'echo',
		args	=> [qw/ system
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'echo system',
			re	=> qr/^echo.system : Success/m,
			func	=> \&mods::walker::std_match,
			},
		],
		wait	=> 45,
		xcode	=> 0,
		name	=> 'UART Intel to TIVA',
		# tid	=> 'TC_GBC_05',
		tid	=> 'TC_GBC_03',
		to	=> 3,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		args	=> [qw/ ethernet.port2.speed
				ethernet.port2.link
				ethernet.port2.duplex
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'eth.port2.speed',
			re	=> qr/port2\.speed = (\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'eth.port2.link',
			re	=> qr/port2\.link = (\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'eth.port2.duplex',
			re	=> qr/port2\.duplex = (\d+)/m,
			func	=> \&mods::walker::range_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check port2 eth',
		tid	=> 'TC_GBC_04',
		to	=> 3,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		wait	=> 2,
		args	=> [qw/ ethernet.port1.speed
				ethernet.port1.link
				ethernet.port1.duplex
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'eth.port1.speed',
			re	=> qr/port1\.speed = (\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'eth.port1.link',
			re	=> qr/port1\.link = (\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'eth.port1.duplex',
			re	=> qr/port1\.duplex = (\d+)/m,
			func	=> \&mods::walker::range_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check port1 eth',
		# tid	=> 'TC_GBC_01',
		tid	=> 'TC_GBC_04',
		to	=> 6,
	},
	{
		prog	=> '/bin/ping',
		args	=> [qw/-c 4 -q/, '192.168.1.34'],
		xcode	=> 0,
		name	=> 'Check known ethernet device',
		tid	=> 'TC_GBC_05.a',
		to	=> 7,
	},
	{	
		prog	=> '/usr/bin/sudo',
		args	=> ['/sbin/modprobe', 'i2c-i801'],
		xcode	=> 0,
		# tid	=> 'TC_GBC_04',
		tid	=> 'TC_GBC_06',
		name	=> 'ID I2C devices',
	},
	{	
		prog	=> '/usr/bin/sudo',
		args	=> ['/usr/sbin/i2cdetect', '-y', '9'],
		to	=> 4,
		res	=> [
			{
				name	=> 'Find GBE-PHY-1.a',
				re	=> qr/^00:\s{9}(?:\s--){5}\s(08)/m,
				func	=> \&mods::walker::std_match,
			},
			{
				name	=> 'Find GBE-PHY-1.b',
				re	=> qr/^30:\s(30)/m,
				func	=> \&mods::walker::std_match,
			},
			{
				name	=> 'Find SE98ATP',
				re	=> qr/^10:(?:\s--){15}\s(1f)/m,
				func	=> \&mods::walker::std_match,
			},
			# {
			# 	name	=> 'Find ADT7481',
			# 	re	=> qr/^40:(?:\s--){12}\s(4c)/m,
			# 	func	=> \&mods::walker::std_match,
			# },
			{
				name	=> 'Find SPD memory',
				re	=> qr/^50:\s(50)/m,
				func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		# tid	=> 'TC_GBC_04.a',
		tid	=> 'TC_GBC_06.a',
		name	=> 'Detect I2C on bus 9',
	},
	{	
		prog	=> '/usr/bin/sudo',
		args	=> ['/usr/sbin/i2cdetect', '-y', '-r', '0'],
		to	=> 8,
		res	=> [
			{
				name	=> 'Find PMIC',
				re	=> qr/^50:(?:\s--){14}\s(5e)/m,
				# re	=> qr/^50:(?:\s--){14}\s(--)/m,
				func	=> \&mods::walker::std_match,
			},
		],
		wait	=> 6,
		xcode	=> 0,
		# tid	=> 'TC_GBC_04.b',
		tid	=> 'TC_GBC_06.b',
		name	=> 'Detect I2C on bus 0',
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		wait	=> 2,
		args	=> [qw/ powersource
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'powersources',
			re	=> qr/
				^powersource\.poe \s=\s Not\s Available .+?
				powersource\.aux \s=\s Primary .+?
				powersource\.extbat \s=\s Not\s Available .+?
				powersource\.intbat \s=\s Not\s Available
				/xsm,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check powersource PS1 only',
		# tid	=> 'TC_GBC_02',
		tid	=> 'TC_GBC_07',
		to	=> 6,
	},
	{
		prog	=> '/usr/local/fbin/p3on',
		poke	=> qr/xyz\$ /,
		xcode	=> 0,
		name	=> 'PS3 to on',
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		wait	=> 2,
		args	=> [qw/ powersource
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'powersources',
			re	=> qr/
				^powersource\.poe \s=\s Available .+?
				powersource\.aux \s=\s Primary .+?
				powersource\.extbat \s=\s Not\s Available .+?
				powersource\.intbat \s=\s Not\s Available
				/xsm,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check powersource P3 on',
		# tid	=> 'TC_GBC_02.a',
		tid	=> 'TC_GBC_07.a',
		to	=> 6,
	},
	{
		prog	=> '/usr/local/fbin/p1off',
		poke	=> qr/xyz\$ /,
		xcode	=> 0,
		name	=> 'PS1 to off',
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		wait	=> 2,
		args	=> [qw/ powersource
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'powersources',
			re	=> qr/
				^powersource\.poe \s=\s Available .+?
				powersource\.aux \s=\s Primary .+?
				powersource\.extbat \s=\s Not\s Available .+?
				powersource\.intbat \s=\s Not\s Available
				/xsm,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check powersource P3',
		# tid	=> 'TC_GBC_02.b',
		tid	=> 'TC_GBC_07.b',
		to	=> 6,
	},
	{
		prog	=> '/usr/local/fbin/p1on',
		poke	=> qr/xyz\$ /,
		xcode	=> 0,
		name	=> 'PS1 to on',
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		wait	=> 2,
		args	=> [qw/ powersource
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'powersources',
			re	=> qr/
				^powersource\.poe \s=\s Available .+?
				powersource\.aux \s=\s Primary .+?
				powersource\.extbat \s=\s Not\s Available .+?
				powersource\.intbat \s=\s Not\s Available
				/xsm,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check powersource PS1, PS3',
		# tid	=> 'TC_GBC_02.c',
		tid	=> 'TC_GBC_07.c',
		to	=> 6,
	},
	{
		prog	=> '/usr/local/fbin/p3off',
		poke	=> qr/xyz\$ /,
		xcode	=> 0,
		name	=> 'PS3 to off',
	},
	{
		prog	=> '/usr/local/fbin/p2apply',
		args	=> ['13', '4.0'],
		poke	=> qr/xyz\$ /,
		xcode	=> 0,
		name	=> 'PS2 to on',
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		wait	=> 2,
		args	=> [qw/ powersource
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'powersources',
			re	=> qr/
				^powersource\.poe \s=\s Not\s Available .+?
				powersource\.aux \s=\s Primary .+?
				powersource\.extbat \s=\s Available .+?
				powersource\.intbat \s=\s Not\s Available
				/xsm,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check powersource P2 on',
		# tid	=> 'TC_GBC_02.a',
		tid	=> 'TC_GBC_07.d',
		to	=> 6,
	},
	{
		prog	=> '/usr/local/fbin/p1off',
		poke	=> qr/xyz\$ /,
		xcode	=> 0,
		name	=> 'PS1 to off',
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		wait	=> 2,
		args	=> [qw/ powersource
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'powersources',
			re	=> qr/
				^powersource\.poe \s=\s Not\s Available .+?
				powersource\.aux \s=\s Not\s Available .+?
				powersource\.extbat \s=\s Primary .+?
				powersource\.intbat \s=\s Not\s Available
				/xsm,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check powersource P2',
		# tid	=> 'TC_GBC_02.b',
		tid	=> 'TC_GBC_07.e',
		to	=> 6,
	},
	{
		prog	=> '/usr/local/fbin/p1on',
		poke	=> qr/xyz\$ /,
		xcode	=> 0,
		name	=> 'PS1 to on',
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		wait	=> 2,
		args	=> [qw/ powersource
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'powersources',
			re	=> qr/
				^powersource\.poe \s=\s Not\s Available .+?
				powersource\.aux \s=\s Primary .+?
				powersource\.extbat \s=\s Available .+?
				powersource\.intbat \s=\s Not\s Available
				/xsm,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check powersource PS1, PS2',
		# tid	=> 'TC_GBC_02.c',
		tid	=> 'TC_GBC_07.f',
		to	=> 6,
	},
	{
		prog	=> '/usr/local/fbin/p2off',
		poke	=> qr/xyz\$ /,
		xcode	=> 0,
		name	=> 'PS2 to off',
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		args	=> [qw/ powersource
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'powersources',
			re	=> qr/
				^powersource\.poe \s=\s Not \s Available .+?
				powersource\.aux \s=\s Primary .+?
				powersource\.extbat \s=\s  Available .+?
				powersource\.intbat \s=\s Not\s Available
				/xsm,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check powersource PS1',
		# tid	=> 'TC_GBC_02.d',
		tid	=> 'TC_GBC_07.g',
		to	=> 6,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'post',
		wait	=> 10,
		args	=> [qw/ enable 
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'post enable',
			re	=> qr/^post.enable : Success/m,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check device presence',
		# tid	=> 'TC_GBC_12',
		tid	=> 'TC_GBC_08',
		to	=> 50,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'post',
		wait	=> 40,
		args	=> [qw/ results
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'EEPROM config',
			func	=> \&mods::walker::std_match,
			re	=> qr/ 
			.+?^	# Drink non-greedily until a new line and an S.
			System\s{2,} CFG\.\s+ EEPROM $Found
			/xsm,
			},
			{
			name	=> 'Inv. EEPROM',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			System\s{2,} INV\.\s+ EEPROM $Found
			/xsm,
			},
			{
			name	=> 'Flash',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			System\s{2,} Flash $Found
			/xsm,
			},
			{
			name	=> 'Lead Acid Temp Sensor',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			Power\s{2,} Lead\s+Acid\s+ $Tsen $Found
			/xsm,
			},
			{
			name	=> 'Lead Acid Charge Controller',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			Power\s{2,} Lead\s+Acid\s+ $Cctl $Found
			/xsm,
			},
			{
			name	=> 'Lithium Ion Charge Controller',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			Power\s{2,} Lithium\s+ Ion\s+ $Cctl $Found
			/xsm,
			},
			{
			name	=> 'PSE Device',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			Power\s{2,} PSE\s+ Device $Miss
			/xsm,
			},
			{
			name	=> 'PD Device',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			Power\s{2,} PD\s+ Device $Miss
			/xsm,
			},
			{
			name	=> 'BMS Temp Sensor',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			BMS\s{2,} Temp\s+ Sensor $Found
			/xsm,
			},
			{
			name	=> 'Current Sensor 12V',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			BMS\s{2,} $Isen \s+ 12V $Found
			/xsm,
			},
			{
			name	=> 'Current Sensor 3.3V',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			BMS\s{2,} $Isen \s+ 3.3V $Found
			/xsm,
			},
			{
			name	=> 'LED Board',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			HCI\s{2,} LED\s+ Board $Found
			/xsm,
			#### HCI\s{2,} LED\s+ Board $Miss
			},
			{
			name	=> 'HCI Temp Sensor',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			HCI\s{2,} $Tsen $Found
			/xsm,
			#### HCI\s{2,} $Tsen $Miss
			},
			{
			name	=> 'ETH Switch',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			ETH\s+ SWITCH\s{2,} NA $Found
			/xsm,
			},
			{
			name	=> 'OBC Iridium',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			OBC\s{2,} Iridium $Miss
			/xsm,
			},
			{
			name	=> 'Temp Sensor 1',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			GPP\s{2,} $Tsen\s+ 1 $Found
			/xsm,
			},
			{
			name	=> 'Temp Sensor 2',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			GPP\s{2,} $Tsen\s+ 2 $Found
			/xsm,
			},
			{
			name	=> 'Temp Sensor 3',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			GPP\s{2,} $Tsen\s+ 3 $Found
			/xsm,
			},
			{
			name	=> 'Current Sensor - Atom',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			GPP\s{2,} $Isen\s+ - \s+ Atom $Found
			/xsm,
			},
			{
			name	=> 'Current Sensor - Msata',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			GPP\s{2,} $Isen\s+ - \s+ Msata $Found
			/xsm,
			},
			{
			name	=> 'SDR Temp Sensor',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			SDR\s{2,} $Tsen $Found
			/xsm,
			},
			{
			name	=> 'Current Sensor - FPGA',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			SDR\s{2,} $Isen\s+ - \s+ FPGA $Found
			/xsm,
			},
			{
			name	=> 'SDR Current Sensor - RF&SDR',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			SDR\s{2,} $Isen \s+ -\s+ RF&SDR $Found
			/xsm,
			},
			{
			name	=> 'RF Temp Sensor - CH1',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			RF\s{2,} $Tsen \s+ -\s+ CH1 $Miss
			/xsm,
			},
			{
			name	=> 'RF Temp Sensor - CH2',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			RF\s{2,} $Tsen \s+ -\s+ CH2 $Found
			/xsm,
			},
			{
			name	=> 'RF Current Sensor - CH1',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			RF\s{2,} $Isen \s+ -\s+ CH1 $Found
			/xsm,
			},
			{
			name	=> 'RF Current Sensor - CH2',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			RF\s{2,} $Isen \s+ -\s+ CH2 $Found
			/xsm,
			},
			{
			name	=> 'Sync IO Exp',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			SYNC\s{2,} IO\s+ Exp $Found
			/xsm,
			},
			{
			name	=> 'Sync Temp Sensor',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			SYNC\s{2,} $Tsen $Found
			/xsm,
			},
			{
			name	=> 'Sync Test Module',
			func	=> \&mods::walker::std_match,
			re	=> qr/ .+?^
			Test\s+ Module\s{2,} Test\s+ Module $Miss
			/xsm,
			},
		],
		xcode	=> 0,
		name	=> 'Check device presence',
		# tid	=> 'TC_GBC_12',
		tid	=> 'TC_GBC_08.a',
		to	=> 50,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		args	=> [
			'bms.tiva.temperature',
			'gpp.intel.temperature1',
			'gpp.intel.temperature2',
			'power.leadacid.ts.temperature',
			'power.lion.ts.temperature',
			],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'bms.tiva.temperature',
			re	=> qr/bms\.tiva\.temperature\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'gpp.intel.temperature1',
			re	=> qr/intel\.temperature1\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'gpp.intel.temperature2',
			re	=> qr/intel\.temperature2\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'power.leadacid.ts.temperature',
			re	=> qr/acid\.ts\.temperature\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			# {
			# name	=> 'power.lion.ts.temperature',
			# re	=> qr/lion\.ts\.temperature\s=\s(\d+)/m,
			# func	=> \&mods::walker::range_match,
			# },
		],
		xcode	=> 0,
		# tid	=> 'TC_GBC_14',
		tid	=> 'TC_GBC_10',
		name	=> 'GBC temps',
		to	=> 3,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		args	=> [
			'bms.tiva.current',
			'bms.tiva.busvoltage',
			'bms.tiva.shuntvoltage',
			'bms.tiva.power',
			'bms.tiva.busvoltage2',
			'bms.tiva.shuntvoltage2',
			'bms.tiva.current2',
			'bms.tiva.power2',
			'gpp.intel.current',
			'gpp.intel.busvoltage',
			'gpp.intel.shuntvoltage',
			'gpp.intel.power',
			'gpp.msata.current',
			'gpp.msata.busvoltage',
			'gpp.msata.shuntvoltage',
			'gpp.msata.power',
			],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'bms.tiva.current',
			re	=> qr/bms\.tiva\.current\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'bms.tiva.busvoltage',
			re	=> qr/bms\.tiva\.busvoltage\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'bms.tiva.shuntvoltage',
			re	=> qr/bms\.tiva\.shuntvoltage\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'bms.tiva.power',
			re	=> qr/bms\.tiva\.power\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'bms.tiva.busvoltage2',
			re	=> qr/bms\.tiva\.busvoltage2\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'bms.tiva.shuntvoltage2',
			re	=> qr/bms\.tiva\.shuntvoltage2\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'bms.tiva.current2',
			re	=> qr/bms\.tiva\.current2\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'bms.tiva.power2',
			re	=> qr/bms\.tiva\.power2\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'gpp.intel.current',
			re	=> qr/intel\.current\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'gpp.intel.busvoltage',
			re	=> qr/intel\.busvoltage\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'gpp.intel.shuntvoltage',
			re	=> qr/intel\.shuntvoltage\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'gpp.intel.power',
			re	=> qr/intel\.power\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'gpp.msata.current',
			re	=> qr/msata\.current\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'gpp.msata.busvoltage',
			re	=> qr/msata\.busvoltage\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'gpp.msata.shuntvoltage',
			re	=> qr/msata\.shuntvoltage\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'gpp.msata.power',
			re	=> qr/msata\.power\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
		],
		xcode	=> 0,
		# tid	=> 'TC_GBC_15',
		tid	=> 'TC_GBC_11',
		name	=> 'GBC temps',
		to	=> 3,
	},
	{	# These HRs are individual tests. Each has all info needed.
		prog	=> '/usr/bin/sudo su',
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> "$Upath1/b2xx_fx3_utils",
		fgpoke	=> '/oc# ',
		args	=> [ 
			'--init-device ' . '--vid=0x2500 ' . '--pid=0x0020'],
		to	=> 9,
		# The res key is a pointer to an array of regular expressions.
		# Each re will be a test on the text matched before the prompt.
		res	=> [
			# The HRs within res have keys for test name, regular
			# expression, and a subroutine reference to handle 
			# the test.
			{
				name	=> 'Process complete',
				re	=> 
				qr/(Initialization Process Complete\.)/m,
				func	=> \&mods::walker::std_match,
			},
		],
		# The xcode key is a flag to check the exit code of this
		# program.
		xcode	=> 0,
		tid	=> 'TC_SDR_01',
		name	=> 'W/R FPGA EEPROM with VID, PID',
	},
	{	
		prog	=> '/usr/bin/lsusb',
		to	=> 4,
		res	=> [
			{
				name	=> 'Bus ID',
				re	=> qr/(ID\s+2500:0020)/m,
				func	=> \&mods::walker::std_match,
			},
		],
		wait	=> 4,
		xcode	=> 0,
		tid	=> 'TC_SDR_01.a',
		name	=> 'Run lsusb',
	},
	{	
		prog	=> '/usr/local/osmo_built/bin/uhd_usrp_probe',
		to	=> 30,
		res	=> [
			{
				name	=> 'Version info',
				re	=> qr/(UHD_3.11.0)/im,
				func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		tid	=> 'TC_SDR_02',
		name	=> 'Program FPGA and sanity check',
	},
	{	
		prog	=> '/usr/local/osmo_built/bin/uhd_find_devices',
		to	=> 8,
		res	=> [
			{
				name	=> 'Find devices name',
				re	=> qr/(name:\s+OCSDR)/im,
				func	=> \&mods::walker::std_match,
			},
			{
				name	=> 'Find devices product',
				re	=> qr/(product:\s+OCR01)/im,
				func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		tid	=> 'TC_SDR_03',
		name	=> 'USB enumeration with data speed',
	},
	{	
		prog	=> '/usr/bin/lsusb',
		to	=> 4,
		res	=> [
			{
				name	=> 'Bus ID',
				re	=> qr/(Bus 002.+?:\s+ID\s+2500:0020)/m,
				func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		tid	=> 'TC_SDR_03.a',
		name	=> 'USB enumeration',
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		wait	=> 19,
		subcmd	=> 'get status',
		args	=> ['sdr.fpga.temperature'],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
				name	=> 'sdr.fpga.temperature',
				re	=> qr/temperature\s=\s(\d+)$/m,
				func	=> \&mods::walker::range_match,
				marry	=> [0,],
			},
		],
		xcode	=> 0,
		tid	=> 'TC_SDR_04',
		name	=> 'SDR temp',
		to	=> 49,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		wait	=> 10,
		subcmd	=> 'get status',
		args	=> [qw/ sdr.fpga.current
				sdr.fpga.busvoltage
				sdr.fpga.shuntvoltage
				sdr.fpga.power
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
				name	=> 'sdr.fpga.current',
				re	=> qr/current\s=\s(\d+)$/m,
				func	=> \&mods::walker::range_match,
			},
			{
				name	=> 'sdr.fpga.busvoltage',
				re	=> qr/busvoltage\s=\s(\d+)$/m,
				func	=> \&mods::walker::range_match,
			},
			{
				name	=> 'sdr.fpga.shuntvoltage',
				re	=> qr/shuntvoltage\s=\s(\d+)$/m,
				func	=> \&mods::walker::range_match,
			},
			{
				name	=> 'sdr.fpga.power',
				re	=> qr/power\s=\s(\d+)$/m,
				func	=> \&mods::walker::range_match,
			},
		],
		xcode	=> 0,
		tid	=> 'TC_SDR_05',
		name	=> 'SDR FPGA v, i, w',
		to	=> 19,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		wait	=> 10,
		subcmd	=> 'get status',
		args	=> [qw/ sdr.comp_all.current
				sdr.comp_all.busvoltage
				sdr.comp_all.shuntvoltage
				sdr.comp_all.power
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
				name	=> 'sdr.comp_all.current',
				re	=> qr/current\s=\s(\d+)$/m,
				func	=> \&mods::walker::range_match,
			},
			{
				name	=> 'sdr.comp_all.busvoltage',
				re	=> qr/busvoltage\s=\s(\d+)$/m,
				func	=> \&mods::walker::range_match,
			},
			{
				name	=> 'sdr.comp_all.shuntvoltage',
				re	=> qr/shuntvoltage\s=\s(\d+)$/m,
				func	=> \&mods::walker::range_match,
			},
			{
				name	=> 'sdr.comp_all.power',
				re	=> qr/power\s=\s(\d+)$/m,
				func	=> \&mods::walker::range_match,
			},
		],
		xcode	=> 0,
		tid	=> 'TC_SDR_05.a',
		name	=> 'SDR comp_all v, i, w',
		to	=> 19,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		wait	=> 5,
		args	=> [qw/ rf.ch1_sensor.current
				rf.ch1_sensor.busvoltage
				rf.ch1_sensor.shuntvoltage
				rf.ch1_sensor.power
				rf.ch2_sensor.current
				rf.ch2_sensor.busvoltage
				rf.ch2_sensor.shuntvoltage
				rf.ch2_sensor.power
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'rf.ch1_sensor.current',
			re	=> qr/h1_sensor\.current\s=\s(\d+)$/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'rf.ch1_sensor.busvoltage',
			re	=> qr/h1_sensor\.busvoltage\s=\s(\d+)$/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'rf.ch1_sensor.shuntvoltage',
			re	=> qr/h1_sensor\.shuntvoltage\s=\s(\d+)$/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'rf.ch1_sensor.power',
			re	=> qr/h1_sensor\.power\s=\s(\d+)$/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'rf.ch2_sensor.current',
			re	=> qr/h2_sensor\.current\s=\s(\d+)$/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'rf.ch2_sensor.busvoltage',
			re	=> qr/h2_sensor\.busvoltage\s=\s(\d+)$/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'rf.ch2_sensor.shuntvoltage',
			re	=> qr/h2_sensor\.shuntvoltage\s=\s(\d+)$/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'rf.ch2_sensor.power',
			re	=> qr/h2_sensor\.power\s=\s(\d+)$/m,
			func	=> \&mods::walker::range_match,
			},
		],
		xcode	=> 0,
		name	=> 'RF v, i, power',
		tid	=> 'TC_RF_01',
		to	=> 9,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		wait	=> 2,
		args	=> [
			'rf.ch1_sensor.temperature',
			'rf.ch2_sensor.temperature',
			],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'rf.ch1_sensor.temperature',
			re	=> qr/temperature : Failed/m,
			func	=> \&mods::walker::std_match,
			# re	=> qr/ch1_sensor\.temperature\s=\s([+-]?\d+)/m,
			# func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'rf.ch2_sensor.temperature',
			re	=> qr/ch2_sensor\.temperature\s=\s([+-]?\d+)/m,
			func	=> \&mods::walker::range_match,
			},
		],
		xcode	=> 0,
		name	=> 'rf temps',
		tid	=> 'TC_RF_02',
		to	=> 9,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'set config',
		wait	=> 2,
		args	=> [qw/ rf.ch1_fe.band 3
				rf.ch1_fe.arfcn 51
				rf.ch1_fe.txattenuation 20
				rf.ch1_fe.rxattenuation 20
				rffe.ch1
				rf.ch2_fe.band 3
				rf.ch2_fe.arfcn 51
				rf.ch2_fe.txattenuation 15
				rf.ch2_fe.rxattenuation 25
				rffe.ch2
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'rf.ch1_fe.band',
			re	=> qr/^rf.ch1_fe.band : Success/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'rf.ch1_fe.arfcn',
			re	=> qr/^rf.ch1_fe.arfcn : Success/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'rf.ch1_fe.txattenuation',
			re	=> qr/^rf.ch1_fe.txattenuation : Success/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'rf.ch1_fe.rxattenuation',
			re	=> qr/^rf.ch1_fe.rxattenuation : Success/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'enable rffe.ch1',
			re	=> qr/^enable.rffe.ch1 : Success/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'rf.ch2_fe.band',
			re	=> qr/^rf.ch2_fe.band : Success/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'rf.ch2_fe.arfcn',
			re	=> qr/^rf.ch2_fe.arfcn : Success/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'rf.ch2_fe.txattenuation',
			re	=> qr/^rf.ch2_fe.txattenuation : Success/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'rf.ch2_fe.rxattenuation',
			re	=> qr/^rf.ch2_fe.rxattenuation : Success/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'enable rffe.ch2',
			re	=> qr/^enable.rffe.ch2 : Success/m,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Config RF',
		tid	=> 'TC_RF_03',
		to	=> 6,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		wait	=> 2,
		args	=> [qw/ rf.ch1_fe.fpower
				rf.ch1_fe.rpower
				rf.ch2_fe.fpower
				rf.ch2_fe.rpower
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'rf.ch1_fe.fpower',
			re	=> qr/h1_fe\.fpower\s=\s(\d+)$/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'rf.ch1_fe.rpower',
			re	=> qr/h1_fe\.rpower\s=\s(\d+)$/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'rf.ch2_fe.fpower',
			re	=> qr/h2_fe\.fpower\s=\s(\d+)$/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'rf.ch2_fe.rpower',
			re	=> qr/h2_fe\.rpower\s=\s(\d+)$/m,
			func	=> \&mods::walker::range_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check rf power',
		tid	=> 'TC_RF_04',
		to	=> 6,
	},
	{	
		prog	=> "$Rpath/occli",
		bgjob	=> "$Rpath/ocmw_usb_revc",
		subcmd	=> 'get status',
		args	=> [qw/ 
				sync.gpslock
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'sync.gpslock',
			re	=> qr/sync\.gpslock = \d+/m,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'GPS sync',
		tid	=> 'TC_SYNC_01',
		to	=> 3,
	},
	];

1;
