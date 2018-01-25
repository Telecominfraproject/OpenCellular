# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

package mods::sock_sess;
#
#	File: sock_sess.pm
#	
#
use Exporter;
our (@ISA, @EXPORT);
@ISA	= qw/Exporter/;
@EXPORT	= qw/	detect_sock	run_sock	$Sname	/;
use strict;
use v5.14;
no warnings 'experimental::smartmatch';
use Cwd;
use Socket;
use IO::Handle;
use Time::HiRes		qw/usleep/;
use mods::walker	qw/ro_user w_user/;
use mods::fbt_config	qw/@Ttable/;
use mods::misc		qw/	shess check_pon pon power_off ping_safe 
				mk_net_ping ssh_login port_22_available/;
use mods::Logio;
use threads;
use Carp;
my $Sock	= undef;
my $Tsock	= undef;
our $Sname	= '/tmp/fbt_sock';
my $Lfh		= undef;
my $Lfn		= '/var/log/fbt/sock.log';
my $Tconf	= {};
my $Uhr		= {};
my $Wh		= undef;	# My walk handle.
my ($Written, $Wusr, $Wsno);

sub detect_sock	{
	return 0 unless -S $Sname;
	socket($Sock, PF_UNIX, SOCK_STREAM, 0)
		or croak("Cannot open socket stream: $!.\n");
	my $dest	= sockaddr_un($Sname);
	connect($Sock, $dest) or croak("Cannot connect to $Sname: $!.\n");
	$Sock->autoflush(1);

	$Lfh		= get_sock_log_file($Lfn);
	$Lfh->autoflush(1);

	# For some reason, strict barks at the bareword SH used in this call
	# to tie. However, I cannot use a scalar in its place or tie will
	# think it is tying a scalar, not a file handle.
	{
	no strict;
	tie(*SH, 'mods::Logio', $Lfh, $Sock);
	$Tsock	= *SH;
	}
	return $Tsock;
}

sub run_sock	{
	my $dir	= shift;
	my $v	= shift;
	state_1($dir, $v);
	state_2();
	state_3();
	state_4();
}

sub std_ok { print $Tsock "ok.\n"; }
sub std_exit { my $ec = shift; print $Tsock "ok.\n"; exit $ec; }
	
sub sock_pon	{
	if (!defined $Tconf->{pon})	{
		print $Tsock "na\n";
		return 'na';
	}

	my $dir		= $Tconf->{dir} // '/var/log/fbt';
	$Tconf->{plfn}	= $Tconf->{plfn} // "$dir/fbt.pwr.log";
	$Tconf->{ppoke}	= $Tconf->{ppoke} // qr/xyz\$ /;
	$Tconf->{peh}	= shess($Tconf);
	pon($Tconf);
	std_ok();
	return 'on';
}

sub sock_poff	{
	if (!defined $Tconf->{pon})	{
		print $Tsock "na\n";
		return 'na';
	}

	power_off('return');
	std_ok();
	return 'off';
}

sub mk_sess	{
	my $hr	= shift;
	my $tip	= $hr->{tip} // croak "Need test network defined.\n";
	my $p	= mk_net_ping($tip);
	my $ret;
	
	# usleep(10 * 1_000_000);
	unless (defined $hr->{useip})	{	# We want a local shell
		print "mk_sess() wants local shell.\n";
		$hr->{eh}	= shess($hr);
		for (my $i = 10; $i < 50; $i += 10)	{
			usleep(10_000_000);
			print $Tsock "waiting $i seconds of 100\n";
		}
		std_ok();
		return 't';
	}
	elsif (!defined $hr->{pon})	{ 
		print "mk_sess() wants shell with $hr->{ip}.\n";
		$ret	= ping_safe($p, $hr->{ip});
		do { report_error("ping", 2); return 'f'; } if ($ret);
		goto ssh_login; 
	}

	# This pathway needs to wait for the UUT to power up.
	my ($n, $s, $p22, $last)	= (0, 0, 0, 70);
	until ($n > $last)	{
		usleep(5_000_000);
		$n	+= 5;
		$ret	= ping_safe($p, $hr->{ip});
		$p22	= port_22_available($hr->{ip});
		if ($ret && $p22)	{
			$s++;
			last if ($s == 2);	# Ping and port 22 twice.
		}
		print $Tsock "waiting $n seconds of $last ($ret, $p22)\n";
	}
	$p->close();
	do { report_error(2, "ping"); return 'f'; } unless ($ret);
	do { report_error(2, "port 22"); return 'f'; } unless ($p22);

	ssh_login:
	$hr->{eh}	= ssh_login($hr);
	do { report_error(2, "ssh login"); return 'f';} 
		unless (defined $hr->{eh});
	std_ok();
	return 't';
}
	
sub pull_from_w_	{
	my $dir = shift;
	my $txt	= shift;
	chomp $txt;
	my ($cmd, $ret)	= $txt	=~ m/^w_(\w+):\s([^\n]*)$/;
	
	if (defined $ret)	{
		$Wusr	= $ret if ($cmd eq 'uname');
		$Wsno	= $ret if ($cmd eq 'serno');
		if (defined $Wusr and defined $Wsno)	{
			$Uhr->{test_info}->{sn}		= $Wsno;
			$Uhr->{test_info}->{user}	= $Wusr;
			w_user($dir, $Uhr);
			undef $Wsno;
			undef $Wusr;
			$Written++;
		}
		std_ok(); 
	}
	else { print $Tsock "Error, no text for w_*: command.\n"; }
}

sub set_test_mode	{
	my $txt	= shift;

	$Tconf->{tmode}++;
	my ($ret)	= $txt =~ m/^test_mode:\s(\d+)/;
	if (defined $ret)	{ std_ok(); }
	else { print $Tsock "Error, test_mode: command had no value?\n"; }
	return $ret // 0;
}

sub report_error	{
	my $state	= shift;
	my $cmd		= shift;
	chomp($cmd);
	print $Tsock "Error, cannot $cmd in state $state.\n";
}

sub send_list	{
	my $i	= shift;
	my $maj	= 1;
	my $min	= 0;
	my $hr	= $Ttable[$i];
	# print "send_list() I'm looking to get Ttable[$i].\n";

	my @tst	= @{$hr->{tests}};

	# This is a check to see if there are really power suppies out
	# there. If not, we just undef the pon key.
	$hr->{pon}	= check_pon($hr->{pon}) if (defined $hr->{pon});
	for my $thr (@tst)	{
		print $Tsock "$maj.$min: $thr->{name}\n";
		if (defined $thr->{res})	{
			my @rtst	= @{$thr->{res}};
			for my $rhr (@rtst)	{
				$min++;
				print $Tsock "$maj.$min: $rhr->{name}\n";
			}
		}
		$min	= 0;
		$maj++;
	}
	$hr->{scomm}	= $Tsock;
	if (defined $hr->{pon})	{
		print $Tsock "test_list still needs power sequence.\n";
	}
	else	{
		print $Tsock "test_list end.\n";
		return $hr;
	}
	my @psq	= @{$hr->{pon}};
	$maj	= 1;
	$min	= 0;

	for my $phr (@psq)	{
		print $Tsock "$maj.$min: $phr->{name}\n";
		if (defined $phr->{res})	{
			my @rpwr	= @{$phr->{res}};
			for my $rhr (@rpwr)	{
				$min++;
				print $Tsock "$maj.$min: $rhr->{name}\n";
			}
		}
		$min	= 0;
		$maj++;
	}
		
	print $Tsock "test_list end.\n";
	return $hr;
}

sub state_1	{
	my $dir		= shift;
	my $v		= shift;
	my $smsg	= "state: 1, pwr: off, sess: f";
	my $ti		= $ENV{FBT_TEST_TABLE_INDEX} // 0;
	$Written	= undef;
	$Wsno		= undef;
	$Wusr		= undef;
	my $cmd;

	$Uhr		= ro_user($dir, $v);
	my $sno		= $Uhr->{test_info}->{sn};
	my $usr		= $Uhr->{test_info}->{user};
	
	until (defined $Tconf->{tests} and defined $Written)	{
		return unless (defined($cmd = <$Tsock>));

		for ($cmd)	{
			print $Tsock "$v\n"		when /^id$/;
			print $Tsock "$usr\n"		when /^uname$/;
			print $Tsock "$sno\n"		when /^serno$/;
			print $Tsock "$smsg\n"		when /^status$/;
			pull_from_w_($dir, $cmd)	when /^w_uname:\s/;
			pull_from_w_($dir, $cmd)	when /^w_serno:\s/;
			$Tconf = send_list($ti)		when /^test_list$/;
			$ti = set_test_mode($cmd)	when /^test_mode:\s/;
			std_exit(1)			when /^exit$/;
			do { 	undef $Written;
				undef $Wusr;
				undef $Wsno; 
				std_ok(); }		when /^clr_info$/;
			default { report_error(1, $cmd); }
		}
	}
}

sub print_status_2	{
	my ($pwr, $sess)	= @_;
	print $Tsock "state: 2, pwr: $pwr, sess: $sess\n";
}

sub state_2	{
	my $cmd;

	my $pwr		= 'off';
	my $sess	= 'f';
	my $status	= 0;
	until ($status)	{
		return unless (defined($cmd = <$Tsock>));

		for ($cmd)	{
			$pwr	= sock_pon()		when /^pon$/;
			$sess	= mk_sess($Tconf)	when /^mk_sess$/;
			std_exit(2)			when /^exit$/;
			do { 	print_status_2($pwr, $sess);
				$status++;}		when /^status$/;
			default { report_error(2, $cmd); }
		}
	}
}

sub report_nops	{
	my $hr	= shift;
	my $tn	= $hr->{tnum} . '.0';

	print $Tsock "$tn: pass_NOPS\n";

	return unless defined $hr->{res};
	for my $rhr (@{$hr->{res}})	{
		$tn	+= 0.1;
		print $Tsock "$tn: pass_NOPS\n";
	}
}

sub walk_all    { # This is an automated test.
        my $uhr         = shift;
        my $hr          = shift;
        my $wh          = mods::walker->new($uhr, $hr);

        $wh->_report_ok();      # Enable reporting.
        my $n           = 1;
        foreach my $th (@{$hr->{tests}})        {
                $th->{tnum}     = $n++;
		if ((!defined $hr->{peh}) 
			&& $th->{prog} =~ m{^/usr/local/fbin})	{
			report_nops($th);
			next;
		}
                $wh->_walk_this($th);
        }
	print $Tsock "Tests complete. Total errors: $wh->{nerr}.\n";
        return $wh;
}


sub state_3	{
	my $cmd;

	until (defined $Wh)	{
		return unless (defined($cmd = <$Tsock>));

		for ($cmd)	{
			$Wh	= walk_all($Uhr,$Tconf)	when /^run_tests$/;
			std_exit(3)			when /^exit$/;
			default { report_error(3, $cmd); }
		}
	}
}

sub state_4	{
	my $cmd;
	my ($off, $report)	= (0, 0);

	until ($off and $report)	{
		return unless (defined($cmd = <$Tsock>));

		for ($cmd)	{
			do { sock_poff(); $off++; }		when /^poff$/;
			do {	$Wh->_report_ok();
				$report++;
				std_ok();
			}					when /^report$/;
			std_exit(0)				when /^exit$/;
			default { report_error(4, $cmd); }
		}
	}
}

sub get_sock_log_file	{
	my $fn	= shift;
	my $ofn	= "$fn.old";

	unlink $ofn if (-e $ofn);
	rename $fn, $ofn if (-e $fn);
	open my $fh, '>', $fn or croak "Cannot open $fn for write: $!.\n";
	return $fh;
}

1;
