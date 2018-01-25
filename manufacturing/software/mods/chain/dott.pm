# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

package mods::chain::dott;
#
#	File: dott.pm
#
#	This file has the pon and tests sequences used by
#	mods::fbt_config.
#
use Exporter;
our (@ISA, @EXPORT_OK);
@ISA		= qw/Exporter/;
@EXPORT_OK	= qw/$Dottpwr $Dottests/;
my $Dottpath	= '/usr/local/sbin';
my $Prompt	= $ENV{LOGIN_PROMPT} // ':~\$ ';
my $Tsen	= qr/Temperature\ssensor/;
my $Isen	= qr/INA\ssensor/;

our $Dottpwr	= [
	{
		prog	=> '/usr/local/fbin/p1set_v',
		args	=> [30],
		xcode	=> 0,
		name	=> 'PS1 to 30 volts',
	},
	{
		prog	=> '/usr/local/fbin/p1set_i',
		args	=> [0.1],
		xcode	=> 0,
		name	=> 'PS1 to 0.1 amperes',
	},
	{
		prog	=> '/usr/local/fbin/p1out_on',
		xcode	=> 0,
		name	=> 'PS1 to on',
	},
	{
		prog	=> '/usr/local/fbin/p2set_v',
		args	=> [30],
		xcode	=> 0,
		name	=> 'PS2 to 30 volts',
	},
	{
		prog	=> '/usr/local/fbin/p2set_i',
		args	=> [0.2],
		xcode	=> 0,
		name	=> 'PS2 to 0.2 amperes',
	},
	{
		prog	=> '/usr/local/fbin/p2out_on',
		xcode	=> 0,
		name	=> 'PS2 to on',
	},
	{
		prog	=> '/usr/local/fbin/p1stat',
		res	=> [
			{
			name	=> 'start.test.p1v',
			re	=> qr/,\s([\.\d]+)\s+volts/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'start.test.p1i',
			re	=> qr/p1:\s([\.\d]+)\s+amperes/m,
			func	=> \&mods::walker::range_match,
			},
			],
		xcode	=> 0,
		name	=> 'ps1stat',
	},
	{
		prog	=> '/usr/local/fbin/p2stat',
		res	=> [
			{
			name	=> 'start.test.p2v',
			re	=> qr/,\s([\.\d]+)\s+volts/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'start.test.p2i',
			re	=> qr/p2:\s([\.\d]+)\s+amperes/m,
			func	=> \&mods::walker::range_match,
			},
			],
		xcode	=> 0,
		name	=> 'ps2stat',
	},
	{
		prog	=> '/usr/local/fbin/p1out_off',
		xcode	=> 0,
		name	=> 'PS1 to off',
	},
	{
		prog	=> '/usr/local/fbin/p2out_off',
		xcode	=> 0,
		name	=> 'PS2 to off',
	},
	{
		prog	=> '/usr/local/fbin/p1apply',
		args	=> [11, 0.54321],
		xcode	=> 0,
		name	=> 'PS1 to 11v, 0.5a, on',
	},
	{
		prog	=> '/usr/local/fbin/p2apply',
		args	=> [11, 0.54321],
		xcode	=> 0,
		name	=> 'PS2 to 11v, 0.5a, on',
	},
	{
		# wait	=> 10,
		prog	=> '/usr/local/fbin/p1out_off',
		xcode	=> 0,
		name	=> 'PS1 to off, again',
	},
	{
		# wait	=> 10,
		prog	=> '/usr/local/fbin/p2out_off',
		xcode	=> 0,
		name	=> 'PS2 to off, again',
	},
	];

our $Dottests	= [
	{	# These HRs are individual tests. Each has all info needed.
		# The prog key points to the execuable.
		# A shell test -x will be run before its launch.
		prog	=> '/usr/bin/arch',
		# The res key is a pointer to an array of regular expressions.
		# Each re will be a test on the text matched before the prompt.
		res	=> [
			# The HRs within res have keys for test name, regular
			# expression, and a subroutine reference to handle 
			# the test.
			{
				name	=> 'x86',
				re	=> qr/(^x86)/m,
				func	=> \&mods::walker::std_match,
			},
			{
				name	=> '6_64',
				re	=> qr/(6_64$)/,
				func	=> \&mods::walker::std_match,
			},
		],
		# The xcode key is a flag to check the exit code of this
		# program.
		xcode	=> 0,
		name	=> 'Run arch',
		tid	=> 't_1',
		to	=> 3,
	},
	{	# These HRs are individual tests. Each has all info needed.
		# The prog key points to the execuable.
		# A shell test -x will be run before its launch.
		prog	=> '/bin/sleep',
		args	=> ['10'],
		# The res key is a pointer to an array of regular expressions.
		# Each re will be a test on the text matched before the prompt.
		res	=> [
			# The HRs within res have keys for test name, regular
			# expression, and a subroutine reference to handle 
			# the test.
			{
				name	=> 'sleep fields',
				re	=> qr/sl(ee)p\s+(\d+)/m,
				func	=> \&mods::walker::field_match,
				marry	=> ['ee', qr/^10$/],
			},
		],
		xcode	=> 0,
		name	=> 'sleep 10',
		tid	=> 't_2',
		to	=> 12,
	},
	{	
		prog	=> '/bin/stty',
		args	=> ['-a'],
		res	=> [
			{
				name	=> 'icanon',
				re	=> qr/(\sicanon\s)/,
				func	=> \&mods::walker::std_match,
			},
			{
				name	=> 'speed',
				re	=> qr/speed\s(\d+)/m,
				func	=> \&mods::walker::std_1_match,
			},
			{
				name	=> 'min',
				re	=> qr/(\smin\s=\s1;)/m,
				func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Run stty',
		tid	=> 't_3',
		to	=> 3,
	},
	{
		prog	=> '/usr/local/fbin/p1apply',
		args	=> [21, 0.654321],
		poke	=> qr/xyz\$ /,
		xcode	=> 0,
		tid	=> 't_4',
		name	=> 'PS1 to 21v, 0.6a, on',
	},
	{	
		prog	=> '/bin/echo',
		args	=> ['Something: Nov 2, 2016.'],
		poke	=> qr{$Prompt},
		res	=> [
			{
				name	=> 'min date',
				re	=> qr/^Something: ([^\.]+)/m,
				func	=> \&mods::walker::date_range,
			},
		],
		xcode	=> 0,
		name	=> 'Run echo date',
		tid	=> 't_5',
		to	=> 3,
	},
	{	
		prog	=> '/bin/echo',
		args	=> ['Something: Nov 2, 2016.'],
		res	=> [
			{
				name	=> 'min date',
				re	=> qr/^Something: ([^\.]+)/m,
				func	=> \&mods::walker::date_range,
			},
		],
		xcode	=> 0,
		name	=> 'Run echo date',
		tid	=> 't_6',
		to	=> 3,
	},
	{	
		prog	=> '/usr/bin/sudo',
		args	=> ['/usr/bin/pgrep', '-l', 'bash'],
		res	=> [
			{
				name	=> 'pgrep bash',
				re	=> qr/(?:^\d+\s+bash)+/m,
				func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Run echo date',
		tid	=> 't_7',
		to	=> 3,
	},
	{	
		prog	=> '/bin/bash',
		args	=> ['-c', q!'echo hello; exit 99'!],
		res	=> [
			{
				name	=> 'bash prints hello',
				re	=> qr/hello/m,
				func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 99,
		name	=> 'Run bash, exit 99',
		tid	=> 't_8',
		to	=> 3,
	},
	{	
		prog	=> '/usr/bin/perl',
		args	=> ['-e', q!'$x=int(rand(1000));print "hello $x.\n"'!],
		res	=> [
			{
				name	=> 'rand fields',
				re	=> qr/^(hello)\s+(.+)$/m,
				func	=> \&mods::walker::field_match,
				marry	=> ['hello', qr/\d+\./],
			},
		],
		xcode	=> 0,
		name	=> 'Run Perl rand script',
		tid	=> 't_9',
		to	=> 3,
	},
	{
		prog	=> "$Dottpath/sudocheck",
		to	=> 3,
		xcode	=> 0,
		tid	=> 't_10',
		name	=> 'Check the sudo prompt answer',
		res	=> [
			{
				name	=> 'sudo passwd ok',
				re	=> qr/^Password matches\./m,
				func	=> \&mods::walker::std_match,
			},
			],
	},
	{	
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
		tid	=> 't_11',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
		subcmd	=> 'get status',
		args	=> ['sdr.fpga.temperature'],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
				name	=> 'fpga temp',
				re	=> qr/temperature\s=\s(\d+)$/m,
				func	=> \&mods::walker::field_match,
				marry	=> [0,],
			},
		],
		xcode	=> 0,
		name	=> 'Check fpga temp',
		tid	=> 't_12',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
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
		name	=> 'Check fpga pwrs',
		tid	=> 't_13',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
		subcmd	=> 'get status',
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
		name	=> 'Check rf chans',
		tid	=> 't_14',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
		subcmd	=> 'get status',
		args	=> [
			'rf.ch1_sensor.temperature',
			'rf.ch2_sensor.temperature',
			],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'rf.ch1_sensor.temperature',
			re	=> qr/ch1_sensor\.temperature\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
			{
			name	=> 'rf.ch2_sensor.temperature',
			re	=> qr/ch2_sensor\.temperature\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
		],
		xcode	=> 0,
		name	=> 'rf temps',
		tid	=> 't_15',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
		subcmd	=> 'set config',
		args	=> [qw/ rf.ch1_fe.band 3
				rf.ch1_fe.arfcn 51
				rf.ch1_fe.txattenuation 20
				rf.ch1_fe.rxattenuation 20
				rffe.ch1
				rf.ch2_fe.band 3
				rf.ch2_fe.arfcn 51
				rf.ch2_fe.txattenuation 15
				rf.ch2_fe.rxattenuation 20
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
		tid	=> 't_16_TC_RF_03',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
		subcmd	=> 'get status',
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
		tid	=> 't_17_TC_RF_04',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
		subcmd	=> 'get status',
		args	=> [qw/ ethernet.port1.status.speed
				ethernet.port1.status.link
				ethernet.port1.status.duplex
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'eth.port1.speed',
			re	=> qr/port1\.status\.speed : Invalid/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'eth.port1.link',
			re	=> qr/port1\.status\.link : Invalid/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'eth.port1.duplex',
			re	=> qr/port1\.status\.duplex : Invalid/m,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check port1 eth',
		tid	=> 't_18_TC_GBC_01',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
		subcmd	=> 'get status',
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
				powersource\.intbat \s=\s Available
				/xsm,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check powersource',
		tid	=> 't_19_TC_GBC_02',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
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
		xcode	=> 0,
		name	=> 'UART Intel to TIVA',
		tid	=> 't_20_TC_GBC_05',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
		subcmd	=> 'get status',
		args	=> [qw/ ethernet.port2.status.speed
				ethernet.port2.status.link
				ethernet.port2.status.duplex
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'eth.port2.speed',
			re	=> qr/port2\.status\.speed : Invalid/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'eth.port2.link',
			re	=> qr/port2\.status\.link : Invalid/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'eth.port2.duplex',
			re	=> qr/port2\.status\.duplex : Invalid/m,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check port2 eth',
		tid	=> 't_21_TC_GBC_06',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
		subcmd	=> 'post',
		args	=> [qw/ enable results
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'post enable',
			re	=> qr/^post.enable : Success/m,
			func	=> \&mods::walker::std_match,
			},
			{
			name	=> 'all devices',
			re	=> qr/ 
			.+?^	# Drink non-greedily until a new line and P.
			POWER\s{2,} Li-ion\sBattery\s{2,} DEV\sFOUND 
			.+?^
			POWER\s{2,} Lead-acid\sBattery\s{2,} DEV\sFOUND 
			.+?^
			POWER\s{2,} Li-ion\sB\w+\s$Tsen\s{2,} DEV\sFOUND 
			.+?^
			POWER\s{2,} Lead-acid\sB\w+\s$Tsen\s{2,} DEV\sFOUND 
			.+?^
			POWER\s{2,} PSE\s{2,} DEV\sMISSING 
			.+?^
			POWER\s{2,} PD\s{2,} DEV\sMISSING
			.+?^
			BMS\s{2,} TIVA\s-\s$Tsen\s{2,} DEV\sFOUND
			.+?^
			BMS\s{2,} TIVA\s-\sINA\ssensor\s{2,} DEV\sFOUND
			.+?^
			HCI\s{2,} Left\sLED\s{2,} FAULT
			.+?^
			HCI\s{2,} Right\sLED\s{2,} FAULT
			.+?^
			ETHERNET\s{2,} Switch\s{2,} DEV\sMISSING
			.+?^
			GPP\s{2,} x86\s$Tsen\s{2,} DEV\sFOUND
			.+?^
			GPP\s{2,} PMIC\s$Tsen\s{2,} DEV\sFOUND
			.+?^
			GPP\s{2,} x86\s$Isen\s{2,} DEV\sFOUND
			.+?^
			GPP\s{2,} mSATA\s$Isen\s{2,} DEV\sFOUND
			.+?^
			SDR\s{2,} FPGA\s$Tsen\s{2,} DEV\sFOUND
			.+?^
			SDR\s{2,} FPGA\s$Isen\s{2,} DEV\sFOUND
			.+?^
			RF\s{2,} CH1\s$Tsen\s{2,} DEV\sFOUND
			.+?^
			RF\s{2,} CH1\s$Isen\s{2,} DEV\sFOUND
			.+?^
			RF\s{2,} CH2\s$Tsen\s{2,} DEV\sFOUND
			.+?^
			RF\s{2,} CH2\s$Isen\s{2,} DEV\sFOUND
			.+?^
			SYNC\s{2,} IO\sExpander\s{2,} DEV\sFOUND
			.+?^
			TEST_MOD\s{2,} 2G\sModule\s{2,} DEV\sMISSING
			.+?^
			IRIDIUM\s{2,} -\s{2,} DEV\sFOUND
			.+?^
			EEPROM\s{2,} Inventory\s{2,} DEV\sMISSING
			.+?^
			EEPROM\s{2,} Serial\snumber\s{2,} DEV\sMISSING
			/xsm,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'Check device presence',
		tid	=> 't_22_TC_GBC_12',
		to	=> 9,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
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
			{
			name	=> 'power.lion.ts.temperature',
			re	=> qr/lion\.ts\.temperature\s=\s(\d+)/m,
			func	=> \&mods::walker::range_match,
			},
		],
		xcode	=> 0,
		tid	=> 't_23_TC_GBC_14',
		name	=> 'GBC temps',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
		subcmd	=> 'get status',
		args	=> [
			'bms.tiva.current',
			'bms.tiva.busvoltage',
			'bms.tiva.shuntvoltage',
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
		tid	=> 't_24_TC_GBC_15',
		name	=> 'GBC temps',
		to	=> 3,
	},
	{	
		prog	=> "$Dottpath/occli",
		bgjob	=> "$Dottpath/ocmw_uart",
		subcmd	=> 'get status',
		args	=> [qw/ 
				sync.gpslock
				/],
		fgpoke	=> 'opencellular# ',
		res	=> [
			{
			name	=> 'sync.gpslock',
			re	=> qr/sync\.gpslock : Failed/m,
			func	=> \&mods::walker::std_match,
			},
		],
		xcode	=> 0,
		name	=> 'GPS sync',
		tid	=> 't_25_TC_SYNC_01',
		to	=> 3,
	},
	{
		# wait	=> 10,
		prog	=> '/usr/local/fbin/p1out_off',
		poke	=> qr/xyz\$ /,
		xcode	=> 0,
		tid	=> 't_26',
		name	=> 'PS1 to off, again',
	},
	];
1;
