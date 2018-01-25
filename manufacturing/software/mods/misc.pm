# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

package mods::misc;
#
#	File: misc.pm
#	
#
use Exporter;
our (@ISA, @EXPORT);
@ISA		= qw/Exporter/;
@EXPORT_OK	= qw/	ss_cmd	keygen	check_project	await_prompt 
			login	ping_check	look_for_tmp	ssess
			shess	@Ptbl	poff	parallel_pon	ssh_login
			$P2fn	tsess	check_pon		set_pwr
			get_pwr 	power_off pon		fix_logfn	
			ping_safe	mk_net_ping	port_22_available/;
use strict;
use Cwd;
use Expect;
use Time::HiRes		qw/usleep/;
use POSIX		qw/:sys_wait_h/;
use Term::ANSIColor	qw/:constants/;
use mods::walker	qw/get_user pwr_this/;
use Net::Ping;
use IO::Socket::INET;
use v5.14;
no warnings 'experimental::smartmatch';
use Carp;
		
sub await_prompt	{
	my $eh		= shift;
	my %info	= @_;
	my $stdre	= qr/\S\$ $/;
	my $stdto	= 9;
	my $to		= $info{to} // $stdto;
	my $re		= $info{re} // $stdre;
	my @rets	= $eh->expect($to, -re => $re);
	return ($rets[1], $rets[2]);
}

sub fix_logfn	{
	my $lfn	= shift;
	my $dir	= shift;
	my $old	= shift;

        mkdir $dir unless (-d $dir);
        croak("Cannot write to $dir.\n") unless (-w -d $dir);
        unlink $old if (-e $old);
        rename $lfn, $old if (-e $lfn);
	return 1;
}

sub ssh_login   {
        my $hr  = shift;
        my $ip  = $hr->{ip};
        my $act = $hr->{acct};
        my $pw  = $hr->{pw};
        my $dir = $hr->{ldir};
        my $lfn = $hr->{lfn};
        my $old = "$lfn.old";

	# Given a log file name, make its path and rename the previous
	# log file, only if not already done.
	my $k		= 'ldone' . $lfn;
	$hr->{$k}	= fix_logfn($lfn, $dir, $old) 
		unless (defined $hr->{$k});
        ping_check($ip);
        return ssess($hr);      # Run logged in.
}

sub look_for_tmp	{
	my $hr	= shift;
	my $ip	= $hr->{ip} // '192.168.0.20';
	my $act	= $hr->{acct} // croak "Need acct defined.\n";
	my $pw	= $hr->{pw} // croak "Need pw defined.\n";
	my $lfn	= $hr->{lfn} // croak "look_for_tmp needs log file name.\n";
	my $arg	= [ "root\@$ip", qw"test -d /usr/local/tmp" ];

	my %cfg	= (	sscmd => '/usr/bin/ssh',
			args => $arg,
			acct => $act,
			pw => $pw,
			eok => 1, lfn => $lfn,
			tgtaddr => $ip);
	my $ret	= ss_cmd(%cfg);
	croak "No /usr/local/tmp dir found.\n" if ($ret);
}

sub ping_check	{
	my $ip		= shift;
	my $prog	= '/bin/ping';
	my @args	= (qw/-c 3 -i 0.5/, $ip);

	if (my $pid	= fork)	{	# Parent
		waitpid($pid, 0);
		croak "Cannot find ip addr ", RED, $ip, RESET, ".\n"
			if ($? >> 8);
	}
	else	{
		open(STDOUT, '>', '/dev/null');
		open(STDERR, '>', '/dev/null');
		exec($prog, @args);
	}
}

sub mk_net_ping	{
	my $lip	= shift;
	my $p	= Net::Ping->new();

	$p->hires();		# Use high resolution timer.
	$p->bind($lip);		# I only look through local nic to UUT.
	return $p;
}

# This version of ping simply returns true if $ip answers.
sub ping_safe	{
	my $p	= shift;
	my $ip	= shift;
	return $p->ping($ip, 1.0);
}

sub port_22_available	{
	my $ip	= shift;

	my $tpl	= "$ip:22";
	my $soc	= IO::Socket::INET->new(PeerAddr => $tpl, Timeout => 0.15);
	if ($soc)	{ $soc->close(); return 1; }
	else { return 0; }
}


sub keygen	{
	my $tgt		= shift;
	my $prog	= '/usr/bin/ssh-keygen';
	my @args	= (	'-f', "$ENV{HOME}/.ssh/known_hosts", 
				'-R', $tgt);

	if (my $pid	= fork)	{	# Parent
		waitpid($pid, 0);
		croak RED, "keygen error", RESET, ".\n" if ($? >> 8);
	}
	else {
		open(STDOUT, '>', '/dev/null');
		open(STDERR, '>', '/dev/null');
		exec($prog, @args);
	}
}

sub ssess	 {
	# In %{$hr} I need these keys: lfn, tgtaddr, acct, pw.
	# I return the expect handle.
	my $hr		= shift;
	my $lfn		= $hr->{lfn} // croak "Need lfn defined.\n";
	my $act		= $hr->{acct} // croak "Need acct defined.\n";
	my $pw		= $hr->{pw} // croak "Need password defined.\n";
	my $ip		= $hr->{ip} // croak "Need tgtaddr defined.\n";
	my $poke	= $hr->{poke} // croak "Need prompt defined.\n";
	my ($err, $mat, $eh, @args);

try_more:
	$eh		= Expect->new();
	@args		= ('/usr/bin/ssh', "$act\@$ip");
	# $eh->slave->clone_winsize_from(\*STDIN);
	$eh->log_stdout(0);
	$eh->log_file($lfn);
	$eh->spawn(@args);
	($err, $mat)	= await_prompt($eh, re => 'password: ');
	if (defined $err)	{
		# carp "Got $err, loop $loop.\n"; $loop++;
		my $bef	= $eh->before();
		# carp "My bef is $bef.\n";
		if ($err =~ m/3:/ && $bef =~ m/NASTY/)	{
			# OS has been updated. We need to keygen.
			$eh->hard_close();
			keygen($ip);
			goto try_more;
		}
		elsif ($err =~ m/2:/ && $bef =~ m{remove with: ssh-keygen}) {
			# Quit because of known_host problem.
			$eh->hard_close();
			keygen($ip);
			goto try_more;
		}
		elsif ($err =~ m/1:/ && $bef =~ m{yes/no\)})	{
			$eh->print("yes\n");
			($err, $mat)	= await_prompt($eh, re => 'password: ');
			croak "Could not get password prompt.\n" 
				if (defined $err);
		}
		else	{
			croak "Error from scp: $err.\n"; 
		}
	}
	$eh->print("$pw\n");
	($err, $mat)	= await_prompt($eh, re => $poke);
	croak "Never got shell prompt: $err.\n" if (defined $err);
	$eh->print_log_file("\nssh session established.\n");
	return $eh;
}

sub shess	{
	# shess opens an Expect handle to the local shell.
	# In %{$hr} I need these keys: lfn, poke.
	# I return the expect handle.
	my $hr		= shift;
	my $lfn		= $hr->{plfn} // $hr->{lfn} 
			// croak "Need [p]lfn defined.\n";
	my $dir		= $hr->{ldir} // croak "Need log dir.\n";
	my $fn		= $hr->{rcfn} // croak "Need the rcfn defined.\n";
	# Take ppoke if defined.
	my $poke	= 	$hr->{ppoke} //
				$hr->{poke} // 
				croak "Need prompt defined.\n";
        my $old 	= "$lfn.old";
	croak("Cannot find $fn.\n") unless (-e $fn);
	my ($err, $mat, $eh, @args);

	# Given a log file name, make its path and rename the previous
	# log file, only if not already done.
	# print "From shess(), my log file name is $lfn.\n";
	my $k		= 'ldone' . $lfn;
	$hr->{$k}	= fix_logfn($lfn, $dir, $old) 
		unless (defined $hr->{$k});

	$eh		= Expect->new();
	@args		= ('/bin/bash', '--rcfile', $fn);
	$eh->log_stdout(0);
	$eh->log_file($lfn);
	$eh->spawn(@args);
	# print "My poke is $poke.\n";
	($err, $mat)	= await_prompt($eh, re => $poke);
	croak "shess() never got shell prompt: $err.\n" if (defined $err);
	$eh->print('echo $$' . "\n");
	($err, $mat)	= await_prompt($eh, re => $poke);
	if (defined $hr->{peh})	{
		my $pid	= $hr->{peh}->pid();
		# print "In shess, first pid is $pid.\n";
		$pid	= $eh->pid();
		# print "In shess, second pid is $pid.\n";
	}
	return $eh;
}

sub ss_cmd	{
	# In %cfg I need these keys: sscmd, args, tgtaddr, acct, pw.
	# The keys lfn and eok are optional.
	my %cfg		= @_;
	my $act		= $cfg{acct};
	my $pw		= $cfg{pw};
	my ($err, $mat, $eh, @args);

try_again:
	$eh		= Expect->new();
	@args		= ($cfg{sscmd}, @{$cfg{args}});
	$eh->slave->clone_winsize_from(\*STDIN);
	$eh->log_stdout(0);
	$eh->log_file($cfg{lfn}) if (defined $cfg{lfn});
	$eh->spawn(@args);
	($err, $mat)	= await_prompt($eh, re => 'password: ');
	if (defined $err)	{
		# carp "Got $err, loop $loop.\n"; $loop++;
		my $bef	= $eh->before();
		# carp "My bef is $bef.\n";
		if ($err =~ m/3:/ && $bef =~ m/NASTY/)	{
			# OS has been updated. We need to keygen.
			$eh->hard_close();
			keygen($cfg{tgtaddr});
			goto try_again;
		}
		elsif ($err =~ m/2:/ && $bef =~ m{remove with: ssh-keygen}) {
			# Quit because of known_host problem.
			$eh->hard_close();
			keygen($cfg{tgtaddr});
			goto try_again;
		}
		elsif ($err =~ m/1:/ && $bef =~ m{yes/no\)})	{
			$eh->print("yes\n");
			($err, $mat)	= await_prompt($eh, re => 'password: ');
			croak "Could not get password prompt.\n" 
				if (defined $err);
		}
		else	{
			croak "Error from scp: $err.\n"; 
		}
	}
	$eh->print("$pw\n");
	if (defined $cfg{wpid})	{
		$eh->log_stdout(1);
		$eh->log_file(undef);
		my $pid	= $eh->pid();
		$eh->interact();
		waitpid($pid, 0);
		return $? >> 8;
	}
	($err, $mat)	= await_prompt($eh, re => 'wait for death', to => 90);
	croak "Did not finish ss_cmd: $err.\n" unless ($err =~ m/(2|3):/);
	# Errors are ok for this command if $cfg{eok} defined.
	croak "Bad return from $cfg{sscmd}: $!.\n" 
		if (($err = $eh->exitstatus()) && (!defined $cfg{eok}));
	$eh->hard_close();
	return $err;
}

sub check_project	{
	my $dir	= shift // getcwd;
	my $re	= qr/	build	| components	| hardware	|
			hw-description		| images	|
			pre-built		| subsystems/x;
	chdir $dir or croak "Cannot cd to $dir: $!.\n";
	opendir my $dh, $dir or croak "Cannot read dir $dir:$!.\n";
	my @pdirs	= grep { /$re/ && -d $_ } readdir($dh);
	closedir $dh;

	croak "$dir does not appear to be PetaLinux project dir.\n"
		unless(@pdirs == 7);
	return {pdir => $dir, hdfdir => "$dir/../hdf", 
		imagedir => "$dir/images/linux"};
}
	
sub login	{
	my %cfg		= @_;
	my $eh		= $cfg{eh};
	my $acct	= $cfg{acct} // croak "Need acct defined.\n";
	my $pw		= $cfg{pw} // croak "Need pw defined.\n";
	my $prompt	= qr/(?:login: )|(?::~# )/;
	my $passw	= qr/Password: /;
	my $lfn		= $cfg{lfn};
	my ($err, $mat);

	$eh->print("\n");
	($err, $mat)	= await_prompt($eh, re => $prompt, to => 3);
	croak "Unexpected $err in login().1.\n" if (defined $err);
	return if ($mat =~ m/:~# /);
	$eh->print("$acct\n");
	($err, $mat)	= await_prompt($eh, re => $passw, to => 3);
	$eh->print("$pw\n");
	($err, $mat)	= await_prompt($eh, re => $prompt, to => 3);
	croak "Unexpected $err in login().2.\n" if (defined $err);
	croak "Could not login, check $lfn.\n" unless ($mat =~ m/:~# /);
	return;
}

# Here is a note on pon and poff. I want the fbt program to run
# even if there is no way to program the power supply automatically.
# If either pon or poff return an exit code of 22, that means that
# the device /dev/dc_ps_1 cannot be found. This is not a problem, we'll
# just assume that someone has provided power to the UUT another way.
sub poff	{	# Turn off power supply.
	my $n	= shift;
	my @pn	= ('/usr/local/fbin/p1off', '/usr/local/fbin/p2off');

	# A temp patch is to poff only one PS.
	pop @pn if (defined $n);
	foreach my $pn (@pn)	{
		croak "Cannot find $pn.\n" unless (-x $pn);
		my $pid	= fork;

		if ($pid)	{
			waitpid($pid, 0);
			my $err	= $? >> 8;
			return if ($err == 22);
			croak "Bad exit code from $pn: $err.\n" if ($err);
		}
		else	{
			open(STDOUT, '>', '/dev/null');
			open(STDERR, '>', '/dev/null');
			exec $pn;
		}
	}
}

sub pon	{	# Turn on power supply.
	my $hr		= shift;
	my $num		= 0;
	my $scomm	= $hr->{scomm};

	foreach my $pr (@{$hr->{pon}})	{
		$num++;
		$pr->{tnum}	= int($num);
		$pr->{tid}	= 'PWR';
		$pr->{scomm}	= $scomm;
		$pr->{peh}	= $hr->{peh};
		$pr->{ppoke}	= $hr->{ppoke};
		my $nam		= $pr->{name};
		pwr_this($pr);
		# print "pon() num is $num, tnum is $pr->{tnum}.\n";
		# use Data::Dump	qw/dump/;
		# print '$pr is ', dump($pr), ";\n";
		croak "Power sequence $num, $nam failure.\n" unless ($pr->{pf});
		my $ap	= $pr->{res};
		foreach my $rhr (@{$ap})	{
			$nam	= $rhr->{name};
			$num	= $rhr->{tnum};
			croak "Power residual $num, $nam failure.\n" 
				unless ($rhr->{pf});
		# 	my $fp	= $rhr->{func};
		# 	my $ret	= $fp->($pr->{bef}, $rhr);
		}
	}
}

sub rotate_in_sleep	{
	my $dur		= shift;
	my $ltime	= int($dur / 10);
	my $wait	= int(($ltime / 4.0) * 1_000_000);
	
	for (my $i = 0; $i < 10; $i++)	{
		syswrite STDOUT, "\b|";
		usleep($wait);
		syswrite STDOUT, "\b/";
		usleep($wait);
		syswrite STDOUT, "\b-";
		usleep($wait);
		syswrite STDOUT, "\b\\";
		usleep($wait);
	}
	syswrite STDOUT, "\b+";
}

sub parallel_pon	{
	my $bin	= shift;
	my $ver	= shift;
	my $hr	= shift;
	my $pon	= $hr->{pon} // 0;
	my $ip	= $hr->{ip} // 'na';
	my $tip	= $hr->{tip} // 'na';
	my $pw	= $hr->{pw};
	my $act	= $hr->{acct};
	my $dir	= $hr->{dir} // '/var/log/fbt';
	my ($uhr, $pthrd, $ret);

	if ($pon)	{
		$hr->{plfn}	= $hr->{plfn} // "$dir/fbt.pwr.log";
		$hr->{ppoke}	= $hr->{ppoke} // qr/xyz\$ /;
		$hr->{peh}	= shess($hr);
		my $ip		= $hr->{ip} // 'na';
		my $n		= 0;
		my $s		= 0;
		my $msg		= "Waiting network connection ";
		my $ehr;
		my $ret;
		my $pok;
		my $p22ok;
		pon($hr);
		# I want the message from pon to get printed first
		# if it finds a problem quickly.
		# usleep(1_500_000);
		$uhr	= get_user($bin, $ver);
		goto JOIN_ONLY if ($ip eq 'na');
		my $p	= mk_net_ping($tip);
		
		syswrite STDOUT, $msg;
		until ($n++ > 14)	{
			syswrite STDOUT, ' ';
			rotate_in_sleep(10);
			
			$pok	= ping_safe($p, $ip);
			$p22ok	= port_22_available($ip);
			$ret	= $pok && $p22ok;

			if ($ret)	{
				$s++;
				last if ($s == 2);	# Ping and 22 twice.
			}
		}
			
		$msg	= "\r\e[0K";
		syswrite STDOUT, $msg;
		$p->close();
		unless ($ret)	{
			if ($pok)	{
				croak "I can ping $ip, but no ssh answers.\n";
			}
			else	{
				croak "Could not ping $ip.\n" 
			}
		}
		JOIN_ONLY:
	}
	else	{
		$uhr	= get_user($bin, $ver);
	}
	return $uhr;
}

sub tsess { 
	my $hr		= shift;
	my $prog	= $hr->{prog} // croak "No prog defined in tsess().\n";
	my $ip		= $hr->{ip} // croak "No ip defined in tsess().\n";
	my $to		= $hr->{to} // croak "No to defined in tsess().\n";
	my $lfn		= $hr->{lfn} // croak "No log file in tsess().\n";
	my $poke	= $hr->{poke1} // croak "No prompt in tsess().\n";
	my $eh		= Expect->new();
	my @args	= ($prog, $ip);
	my ($mat, $err);
	$eh->log_stdout(0);
	unlink $lfn if (-e $lfn);
	$eh->log_file($lfn);
	$eh->spawn("@args");
	($err, $mat)	= await_prompt($eh, re => $poke, to => $to);
	croak "Could not get first prompt. Error: $err.\n" if (defined $err);
	$hr->{eh}	= $eh;
	return $eh; 
}

# The pwr_get and pwr_set routines report and maintain a hash used
# by the interrupt signal handler to turn off known on power supplies.
my %Pwr;

sub set_pwr	{
	my $prog	= shift;

	return if ($prog =~m{/acpwr_});
	my ($pnam)	= $prog =~ m{/([^/]+)$};
	croak("In set_pwr(), cannot find in $prog.\n") unless defined $pnam;
	my ($n, $oper)	= $pnam =~ m/^p(\d)(.+)$/;
	croak("In set_pwr(), cannot parse $pnam.\n") 
		unless(defined $n and defined $oper);
	
	for ($oper)	{
		when (/on$/)	{ $Pwr{$n}++; }
		when (/off$/)	{ delete $Pwr{$n} if (defined $Pwr{$n}); }
		when (/apply$/) { $Pwr{$n}++; }
	}
}

sub get_pwr	{ return \%Pwr; }

sub power_off	{
	my $ret	= shift;
        my $hr  = get_pwr();

        for my $k (keys %{$hr})      {
		usleep(200_000);
                my $prog        = '/usr/local/fbin/p' . $k . 'off';
                my $pid         = fork;
                if ($pid)       {
                        waitpid($pid, 0);       # I ignore exit codes.
                }
                else    {
			# warn("I want to run $prog.\n");
                        open(STDOUT, '>', '/dev/null');
                        open(STDERR, '>', '/dev/null');
                        exec($prog);
                }
        }
	return if (defined $ret and $ret eq 'return');
        exit 22;
}

# This routine checks to see if the two Keysight power supplies are
# connected and turned on. Both should return 0 from the stat program.
# If not, an undef is returned. On success, the original pon key is returned.
sub check_pon	{
	my $pf	= shift;
	return unless (defined $pf);

	my $pprog	= '/usr/local/fbin/pxstat';

	for my $n (1 .. 4)	{
		my $lprog	= $pprog;
		$lprog		=~ s/x/$n/;
		my $pid		= fork;

		if ($pid)	{
			waitpid($pid, 0);
			return if ($? >> 8);
		}
		else	{
			open(STDOUT, '>', '/dev/null');
			open(STDERR, '>', '/dev/null');
			exec($lprog);
		}
	}
	return $pf;
}

our $P2fn	= '/dev/dc_ps_2';
our @Ptbl	= (
		{
			dev	=> '/dev/dc_ps_1',
			del	=> 200_000,		# n useconds/cmd
			v	=> 18.0,
			i	=> 3.5,
			vtol	=> 10,			# +- %
			imin	=> 4,			# % of i
			range	=> "HIGH",
			type	=> qr/E3632A/,
			trmio	=> '9600,8,n,1',
			ldir	=> '/var/log/fbt',
			lfn	=> 'pon_poff.log',
			stty	=> [ qw/
				cs8 clocal -hupcl -icrnl -ixon -opost
				-isig -icanon -iexten -echo
				-echoe -echonl -echok
				/ ],
		},
		);
1;
