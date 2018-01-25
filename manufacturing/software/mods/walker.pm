# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

package mods::walker;
#
#	File: walker.pm
#	
#	This is a module that uses a ssh session to walk through
#	some pre-defined tests.
#	
#
use Exporter;
our (@ISA, @EXPORT);
@ISA	= qw/Exporter/;
@EXPORT	= qw/	get_user print_menu std_match date_range 
		field_match eval_version ro_user w_user
		tod_match tod_set range_match std_1_match 
		pwr_this led_inquire
	/;
use strict;
use Cwd;
use Expect;
use Time::HiRes		qw/usleep/;
use Term::ANSIColor	qw/:constants/;
use Regexp::Stringify	qw/stringify_regexp/;
use Time::Local;
use FindBin		qw/$Bin/;
use lib $Bin;
use mods::misc		qw/set_pwr power_off/;
use mods::tolerance	qw/%Tol/;
use File::Basename;
use Carp;
use JSON;
my $Stdto	= 5;	# Standard prompt timeout.

sub std_1_match	{
#	This function is given a match field within its re.
#	On success, the matched text becomes part of the test report.
	my $txt	= shift;
	my $hr	= shift;
	# print "My hr is $hr.\n";
	my $re	= $hr->{re};
	my $pf	= 0;
	my $mval;
	# print "std_match: txt, re: $txt, $re.\n" 
	#	if ($hr->{name} =~ m/^powerso/);
	my ($mat)	= $txt =~ $re;
	my $mtxt;
	$re = stringify_regexp(regexp => $re, with_qr => 1); 
	if (defined $mat)	{	# We matched.
		$mtxt	= "Matched $re.\n";
		$mval	= $mat;
		$pf++;
	}
	else	{
		$mtxt	= "No match of $re in $txt.\n";
		$mval	= "na";
	}
	return {pf => $pf, mtxt => $mtxt, mval => $mval};
}

sub std_match	{
#	All I have to do is run the provided re against the text passed.
	my $txt	= shift;
	my $hr	= shift;
	# print "My hr is $hr.\n";
	my $re	= $hr->{re};
	# print "std_match: txt, re: $txt, $re.\n" 
	#	if ($hr->{name} =~ m/^powerso/);
	my $mat	= $txt =~ $re;
	my $mtxt;
	$re = stringify_regexp(regexp => $re, with_qr => 1); 
	if ($mat)	{	# We matched.
		$mtxt	= "Matched $re.\n";
	}
	else	{
		$mtxt	= "No match of $re in $txt.\n";
	}
	return {pf => $mat, mtxt => $mtxt};
}

#	The global $Tod contains and hold the time information. It gets
#	set by tod_set and read by tod_match.
my $Tod	= 0;
sub tod_set	{
	$Tod	= time;
	my ($min, $hr, $date, $mo, $yr) = (localtime($Tod))[1 .. 5];
	$mo	+= 1;
	$yr	+= 1900;
	return sprintf("%02d%02d%02d%02d%04d", $mo, $date, $hr, $min, $yr);
}

#	
#	In tod_match I match two things. First, the re must be good.
#	Second, the $Tod must be within 120 seconds of the converted
#	time.
sub tod_match	{
	my $txt	= shift;
	my $hr	= shift;
	# print "My hr is $hr.\n";
	my $re	= $hr->{re};
	# print "std_match: txt, re: $txt, $re.\n";
	my $mat		= $txt =~ $re;
	my $mtxt	= $1 // 'Mon Jan  1 00:00:00 CST 1900';
	my $tod		= get_a_time($mtxt);
	my $diff	= abs($Tod - $tod) < 120;
	$re = stringify_regexp(regexp => $re, with_qr => 1); 
	if ($mat && $diff)	{	# We matched.
		$mtxt	= "Matched $re and time of day.\n";
	}
	elsif ($mat)	{
		$mtxt	= "No match of $re in $txt.\n";
	}
	else	{
		$mtxt	= "Date greater than 120 seconds of expected.\n";
	}
	return {pf => ($mat && $diff), mtxt => $mtxt};
}

sub field_match	{
#	field_match() matches fields within matched groups.
#	For each match group within the passed regular expression,
#	a cooresponding member of the marry array will be matched
#	against it.
	my $txt		= shift;
	my $hr		= shift;
	# print "My text in field_match is $txt.\n";
	my $re		= $hr->{re};
	my @marry	= @{$hr->{marry}};
	my $hoped	= @marry;
	my @res		= $txt =~ $re;
	my $saw		= @res;
	my $mtxt	= "Could not find $re in:\n$txt\n";
	# print "I see ", scalar @res, " matches: @res.\n";
	# print "I expect @marry.\n";
	my $rhr		= {pf => 0, expected => $hoped, 
			seen => $saw, matched => 0};
	return $rhr unless (@res == @marry);

	my $found	= 0;
	my $i		= 0;
	for (; $i < scalar(@res); $i++)	{
		my $mre	= $marry[$i];
		$found++ if ($res[$i] =~ m/$mre/);
		$marry[$i] = stringify_regexp(regexp => $mre, with_qr => 1) 
			if (ref($mre) eq 'Regexp');
	}
	$rhr->{matched}	= $found;
	return $rhr if ($found != $i);	# We failed.
	$rhr->{pf}	= 1;		# We passed.
	$rhr->{mtxt}	= [@marry];
	return $rhr;
}

sub in_range	{
	my $hr	= shift;

	my $n	= $hr->{actual};
	my $min	= $hr->{min};
	my $max	= $hr->{max};
	return (($n >= $min && $n <= $max) ? 1 : 0);
}

sub range_match	{
	my $txt		= shift;
	my $hr		= shift;
	# print "My text in field_match is $txt.\n";
	my $re		= $hr->{re};
	my $name	= $hr->{name};
	my ($act)	= $txt =~ $re;
	my $hoped	= stringify_regexp(regexp => $re, with_qr => 1);
	my $rhr		= {pf => 0, expected => $hoped, 
			name => $name, seen => $txt, matched => 0};
	return $rhr unless (defined $act);
	$rhr->{matched}++;
	my $thr		= $Tol{$name} // croak "Cannot find tol in $name.\n";
	$rhr->{min}	= $thr->{min} // croak "Cannot find min in $name.\n";
	$rhr->{max}	= $thr->{max} // croak "Cannot find max in $name.\n";
	$rhr->{actual}	= $act;

	$rhr->{pf}	= in_range($rhr);
	return $rhr;
}

sub waiter      {
        my $n   = shift;
	my $s	= shift;

        return unless $n;

        if ($n < 10)    {
                usleep(int($n * 1_000_000));
		return;
        }

	if ($s)	{		# Socket connection.
		my $nsp = 10;	# This many messages.
		my $inc	= int($n * 1_000_000 / $nsp);
		for (my $i = 1; $i <= $nsp; $i++)	{
			print $s "waiting $i part of $nsp\n";
			usleep($inc);
		}
	}
        else	{		# No socket connection, out to tty.
                my $nsp = 50;   # This many spaces.
                my $inc = int($n * 1_000_000 / $nsp);
                my $str = '.' x $nsp;
                syswrite STDOUT, "$str\r";
		my $cdelay	= int($inc / 4);
                for (my $i = 0; $i < $nsp; $i++)        {
                        usleep($cdelay);
                        syswrite STDOUT, "/\010";
                        usleep($cdelay);
                        syswrite STDOUT, "-\010";
                        usleep($cdelay);
                        syswrite STDOUT, "\\\010";
                        usleep($cdelay);
                        syswrite STDOUT, '|';
                }
                syswrite STDOUT, "\r\e[0K";     # Clear entire line.
        }
}

sub get_a_time	{
	my $txt	= shift;

	my ($sec, $min, $hr)	= (0, 0, 0);
	# I expect two formats:
	# Jan 1, 2004
	# or
	# Thu Mar  2 15:42:00 UTC 2017
	my @mos	= qw/jan feb mar apr may jun jul aug sep oct nov dec/;
	my ($tmo, $mday, $yr)	= $txt =~ m/(...)\s+(\d+),\s+(\d{4})/;
	unless(defined $yr)	{	# Try the other format
		($tmo, $mday, $hr, $min, $sec, $yr)	= 
		$txt =~ m/\w{3}\s(\w{3})\s+(\d+)\s(\d+):(\d+):(\d+)\D+(\d{4})/;
	
		croak "Cannot parse: $txt, get_a_time().\n" unless defined $yr;
	}

	my ($i, $mon)		= (0, undef);
	$tmo			= lc($tmo);
	foreach my $mo (@mos)	{
		if ($tmo eq $mo)	{
			$mon	= $i;
			last;
		}
		$i++;
	}

	croak "Cannot match month $tmo in get_a_time().\n" unless defined $mon;

	my $time	= timelocal($sec, $min, $hr, $mday, $mon, $yr);
	return $time;
}

sub date_range	{
	my $txt		= shift;
	my $hr		= shift;
	my $re		= $hr->{re};
	my ($date)	= $txt =~ m/$re/;
	$date		= $date // croak "date_range() err.\n$txt.\nre: $re.\n";
	my $min		= $hr->{min} // 'Nov 01, 2016';
	my $max		= $hr->{max} // 'na';
	my ($atime, $mtime, $mxtim, $pf);

	$atime	= get_a_time($date);
	$mtime	= get_a_time($min);
	if ($max eq 'na')	{
		$pf	= $atime > $mtime;
	}
	else	{
		$mxtim	= get_a_time($max);
		$pf	= ($atime > $mtime) && ($atime < $mxtim);
	}
	
	return { pf => $pf, min => $min, max => $max, actual => $date };
}

sub _prompt	{
	my $self	= shift;
	my $eh		= $self->{eh};
	my $to		= $self->{to};
	my $re		= $self->{poke};
	my $pw		= $self->{pw};
	#		If I have an sudo command, I need to send pw.
	my $sure	= [ 	qr{\[sudo\] password for \S+: },
				sub {	my $self = shift;
					$self->send("$pw\n");
					exp_continue; } ];
				
	my @rets	= $eh->expect($to, $sure, -re => $re);
	$rets[3]	=~ s/\r//g;	# Rid of \r for anchors to work.
	return ({err => $rets[1], mat => $rets[2], bef => $rets[3]});
}

sub _exit_code	{
	my $self	= shift;
	my $eh		= $self->{eh};
	my $cmd		= 'echo $?';
	$eh->print("$cmd\n");
	my $hr		= $self->_prompt();
	if (defined $hr->{err})	{
		carp "In _exit_code, return error: $hr->{err}.\n";
		return 22;
	}
	my $bef		= $hr->{bef};
	$bef		=~ s/\r//g;
	my ($n)		= $bef =~ m/^(\d+)$/m;
	unless (defined $n)	{
		croak "In _exit_code, cannot find in $bef.\n";
	}
	return $n;
}

# Find out if there is no executable.
sub _no_exe	{
	my $self	= shift;
	my $prog	= shift;
	my $tnum	= shift . ' prog_check';
	my $eh		= $self->{eh};
	my @progs	= split / /, $prog;
	my $cmd		= "test -x $progs[0]";

	$eh->print("$cmd\n");
	my $hr		= $self->_prompt();
	if (defined $hr->{err})	{
		croak "In _no_exe(), test of $prog returned $hr->{err}.\n"
		. "My eh is $eh and my poke is $self->{poke}.";
		return 1;
	}
	my $exitc	= $self->_exit_code();
	$self->_report_exc(0, $exitc, "Seeking $prog", $tnum, 0) if ($exitc);
	return $exitc;
}

sub _init	{
	my $self	= shift;
	my $hr		= shift;
	my $tconf	= shift;

	$self->{result}	= $hr;
	for my $k (qw/acct pw poke eh tdir ip/)	{
		$self->{$k}	= $tconf->{$k} // croak
				"_init cannot find key $k.\n";
	}
	# I want a copy of the power on expect handle, if used.
	$self->{oeh}	= $self->{eh};
	$self->{opoke}	= $self->{poke};
	$self->{peh}	= $tconf->{peh} if (defined $tconf->{peh});
	$self->{ppoke}	= $tconf->{ppoke} if (defined $tconf->{ppoke});

	if (defined $tconf->{pon})	{	# Copy in pon results.
		my @ap	= @{$tconf->{pon}};
		for my $pr (@ap)	{
			next unless defined $pr->{result}{pwr};
			# print "Found result pwr.\n";
			my $phr	= $pr->{result}{pwr};
			push @{$self->{result}{pwr}}, $phr;
			next unless defined $pr->{res};
			for my $rr (@{$pr->{res}})	{
				next unless defined $rr->{result}{pwr};
				my $rhr	= $rr->{result}{pwr};
				push @{$self->{result}{pwr}}, $rhr;
			}
		}
	}
	
	$self->{scomm}	= $tconf->{scomm};
	$self->{nerr}	= 0;
	$self->{to}	= 3;
	return;
}

sub new	{
	my $class	= shift;
	my $hr		= shift;
	my $tconf	= shift;
	my $self	= {};
	bless $self, $class;

	$self->_init($hr, $tconf);
	return $self;
}

sub _prune_results	{
	my $glob	= shift;
	my $days	= shift;

	my @fns		= glob $glob;
	my @unlinks	= ();

	for my $fn (@fns)	{
		push @unlinks, $fn if (-M $fn > $days);
	}
	my $n	= @unlinks;
	my $m	= (scalar @fns) - $n;
	carp "I'll remove $n files, leaving $m files.\n" if ($n);
	unlink @unlinks if ($n);
}

sub _write_results	{
	my $self	= shift;
	my $dir		= $self->{tdir};
	my %mos		= qw/	jan 1 feb 2 mar 3 
				apr 4 may 5 jun 6 
				jul 7 aug 8 sep 9
				oct 10 nov 11 dec 12/;

	unless (-d $dir)	{
		mkdir $dir or croak "Cannot create $dir: $!.\n";
	}
	croak "Cannot write to $dir.\n" unless (-w -d $dir);
	$self->{result}{test_info}{ip}	= $self->{ip} // '127.0.0.1';
	my $tt	= $self->{result}{test_info}{time} // scalar(localtime);
	my @ts	= $tt =~ m/\w{3}\s		# Fri
			(\w{3})\s+		# Dec
			(\d+)\s			# 9
			(\d\d):(\d\d):(\d\d)\s	# 09:27:37
			(\d{4})			# 2016
			/x;
	croak "Could not parse $tt.\n" unless (@ts == 6);
	my $mo	= $mos{lc($ts[0])};
	croak "I cannot find $ts[0].\n" unless (defined $mo);
	my $fn	= "fbt.$mo-$ts[1]-$ts[5]_$ts[2]-$ts[3]-$ts[4].json";
	my $ffn	= "$dir/$fn";
	$self->{full_log_file_name} = $ffn;
	my $hr	= $self->{result};
	open my $fh, '>', $ffn or croak "Cannot write to $ffn: $!.\n";
	my $js	= to_json($hr, {ascii => 1, pretty => 1});
	print $fh $js;
	close $fh;
	# $self->_prune_results("$dir/fbt.*.json", 10_000);
}

sub _add_result	{
	my $self	= shift;
	my $hr		= shift;

	if (defined $hr->{pwr})	{ push @{$self->{result}{pwr}}, $hr; }
	else			{ push @{$self->{result}{test}}, $hr; }
}

sub _report_exc	{
#	Here I report the results of an exit code test step.
#	My $ec argument is my exit code. If 0, ok. If != 0, fail.
	my $self	= shift;
	my $eec		= shift;	# Expected exit code.
	my $ac		= shift;	# Actual exit code value.
	my $pname	= shift;
	my $tnum	= shift;
	my $tid		= shift // 0;
	my $hr		= $self->{result};
	my $sock	= $self->{scomm};
	my $ecerr	= 0;
	if (defined $eec)	{	# We have an exit code to match.
		$ecerr	= ($eec == $ac) ? 0 : 1;
	}
	else 	{
		$eec	= $ac = 'na';
	}
	my $pf		= ($ecerr) ? 'fail' : 'pass';

	if (defined $sock)	{
		print $sock "$tnum: $pf.\n";
	}
	else	{ # Send to tty.
		my $res	= ($ecerr) ? (RED . 'fail') : (GREEN . 'ok');
		my $nam	= (($tnum =~ m/\.0$/) and $tid) 
			? ("$tnum " . BOLD . BLACK . ON_YELLOW  
			. "$tid" . RESET . ',') 
			: "$tnum,";

		$res	.= RESET . ".\n";
		print	 "Test $nam " . BLUE . $pname . RESET 
			. " exit $eec: $res";
	}
	$self->{nerr}++ if $ecerr;
	my $v		= {	name => $pname, expected_exit_code => $eec, 
				actual_exit_code => $ac, pf => $pf};
	$v->{tid}	= $tid if ($tid);
	$self->_add_result({$tnum, $v});
	# $hr->{test}{$tnum} = {name => $pname, exit_code => $ec, pf => $pf};
}

sub _report_pf	{
#	Here I report the results of a pass fail test.
#	If $pf is true, we pass. Otherwise, we fail.
	my $self	= shift;
	my $phr		= shift;
	my $pname	= shift;
	my $tnum	= shift;
	my $tid		= shift // 0;
	my $pf		= $phr->{pf};
	my $mtxt	= $phr->{mtxt} // 'na';
	my $hr		= $self->{result};
	my $sock	= $self->{scomm};
	my $pft		= ($pf) ? 'pass' : 'fail';

	if (defined $sock)	{
		print $sock "$tnum: $pft.\n";
	}
	else	{ # Send to tty.
		my $res		= ($pf) ? (GREEN . 'ok') : (RED . 'fail');
		$res		.= RESET . ".\n";
		print "Test $tnum, " . BLUE . $pname . RESET . " found: $res";
	}
	$self->{nerr}++ unless $pf;
	# $hr->{test}{$tnum} = {name => $pname, pf_val => $pf, pf => $pft};
	my $v	= {name => $pname, pf_val => $pf, pf => $pft, mtxt => $mtxt};
	$v->{mval}	= $phr->{mval} if (defined $phr->{mval});
	$v->{tid}	= $tid if ($tid);
	$self->_add_result({$tnum, $v});
}

#	This is the report of a parameter test. What is passed here is the
#	min and max values with the pass fail determination.
sub _report_param	{
	my $self	= shift;
	my $hr		= shift;
	my $pname	= shift;
	my $tnum	= shift;
	my $tid		= shift // 0;
	my $pf		= $hr->{pf};
	my $min		= $hr->{min} // 'unknown';
	my $max		= $hr->{max} // 'unknown';
	my $act		= $hr->{actual} // 'unknown';
	my $shr		= $self->{result};
	my $sock	= $self->{scomm};
	my $pft		= ($pf) ? 'pass' : 'fail';

	if (defined $sock)	{	# Send message to socket.
		print $sock "$tnum: $pft.\n";
	}
	else	{ # Send to tty.
		my $res		= ($pf) ? (GREEN . 'ok') : (RED . 'fail');
		$res		.= RESET . ".\n";
		print	"Test $tnum, " . BLUE . $pname . RESET . 
			" min, max, actual; $min, $max, $act: $res";
	}
	$self->{nerr}++ unless $pf;
	my $v	= {	name => $pname, min => $min, max=> $max, 
			actual => $act, pf => $pft};
	$v->{tid}	= $tid if ($tid);
	$self->_add_result({$tnum, $v});
}

sub _report_marry	{
#	This is the report of a multiple array match test. 
#	What is passed here is the expected number of matches,
#	the seen field matches from the re, and the actual
#	text matches.
	my $self	= shift;
	my $hr		= shift;
	my $pname	= shift;
	my $tnum	= shift;
	my $tid		= shift // 0;
	my $pf		= $hr->{pf};
	my $hoped	= $hr->{expected};
	my $saw		= $hr->{seen};
	my $match	= $hr->{matched};
	my $mtxt	= $hr->{mtxt};
	my $shr		= $self->{result};
	my $sock	= $self->{scomm};
	my $pft		= ($pf) ? 'pass' : 'fail';

	if (defined $sock)	{	# Send message to socket.
		print $sock "$tnum: $pft.\n";
	}
	else	{ # Send to tty.
		my $res		= ($pf) ? (GREEN . 'ok') : (RED . 'fail');
		$res		.= RESET . ".\n";
		print	"Test $tnum, " . BLUE . $pname . RESET . 
		" nfields, rematch, txtmatch; $hoped, $saw, $match: $res";
	}
	$self->{nerr}++ unless $pf;
	# $shr->{test}{$tnum} = {
	my $v	= {	name => $pname, nfield => $hoped, rematched => $saw, 
			txtmatch => $match, pf => $pft, mtxt => $mtxt};
	$v->{tid}	= $tid if ($tid);
	$self->_add_result({$tnum, $v});
}

sub _report_ok	{
	my $self	= shift;
	return ++$self->{rep_ok};
}

sub _nerr	{
	my $self	= shift;
	return $self->{nerr} // 0;
}

#
#	The residual tests are those concerned with the text that
#	was output from the previous cli command. These tests
#	reside in the array pointed to by $hr->{res}.
#
sub _residual_tests	{
	my $self	= shift;
	my $hr		= shift;
	my $ok		= $self->{rep_ok} // 0;
	my $beftxt	= $hr->{bef};
	my $minort	= 1;
	my $tnum	= $hr->{tnum};
	my $tid		= $hr->{tid} // 0;
	my $tname;

	foreach my $rhr (@{$hr->{res}})	{
		my $re	= $rhr->{re};
		$tname	= $rhr->{name};
		# print "Bef txt: $beftxt.\n" if ($tname =~ m/start\.test\.p/);
		my $fp	= $rhr->{func};
		my $ret	= $fp->($beftxt, $rhr);
		my $tst	= "$tnum.$minort";
		if (defined $ret->{min})	{
			# This test was for parametric data.
			$self->_report_param($ret, $tname, $tst, $tid) if $ok;
			
		}
		elsif (defined $ret->{marry})	{
			# This test was for a match array.
			$self->_report_marry($ret, $tname, $tst, $tid) if $ok;
		}
		else	{
			$self->_report_pf($ret, $tname, $tst, $tid) if $ok;
		}
		$minort++;
	}
}

sub _deamon_check	{
	my $self	= shift;
	my $hr		= shift;
	my $eh		= $self->{eh};
	my $bgjob	= $hr->{bgjob};
	my $bgnam	= basename($bgjob);
	my @pids;
 
	$eh->print("pgrep $bgnam\n");
	my $phr		= $self->_prompt();
	croak "From _deamon_check: $phr->{err}.\n" if (defined $phr->{err});
	# carp "From _deamon_check: $phr->{bef}.\n";
	@pids		= $phr->{bef} =~ m/^(\d+)$/smg;
	carp "Multiple copies of $bgjob are running.\n" if (@pids > 1);
	return $pids[0] if (@pids == 1);

	$self->_kill_deamon($bgnam) if (@pids > 1);
	# I need to launch my background job and get its pid.
	$eh->print("sudo $bgjob > /dev/null &\n");
	$phr		= $self->_prompt();
	croak "Prompt error running $bgjob: $phr->{err}.\n"
		if (defined $phr->{err});
	my ($pid)	= $phr->{bef} =~ m/^\[\d+\]\s(\d+)/m;
	croak "In _deamon_check(), cannot find pid from $phr->{bef}.\n"
		unless (defined $pid);
	return $pid;
}

sub _kill_deamon	{
	my $self	= shift;
	my $bgjob	= shift;
	my $eh		= $self->{eh};
	my $phr;
	my @pids;
	my $found	= 0;

	$eh->print("pgrep $bgjob\n");
	$phr		= $self->_prompt();
	croak "From _kill_deamon: $phr->{err}.\n" if (defined $phr->{err});
	@pids		= $phr->{bef} =~ m/^(\d+)$/smg;
	foreach my $pid (@pids)	{
		$found++;
		$eh->print("sudo kill -9 $pid\n");
		$phr		= $self->_prompt();
	}
	return $found;
}

#
#	The _walk_these method is similar to _walk_this. However,
#	_walk_these interfaces to an interactive program with a background
#	job. So, all of the commands and command responses get combined
#	for use with the residual tests.
#
#	We start by launching the background job and extracting its
#	pid.
sub _walk_these	{
	my $self	= shift;
	my $hr		= shift;

	my $tnum	= ($hr->{tnum} // 99) . '.0';
	my $pnam	= $hr->{prog} // croak "Must have prog defined.\n";
	my $tname	= $hr->{name} // 'unknown';
	my $tid		= $hr->{tid} // 0;
	my $ap		= $hr->{args};
	my $xcode	= $hr->{xcode};
	$self->{to}	= $hr->{to} // $Stdto;
	# For this test, we change to a new prompt, if defined.
	$self->{poke}	= $hr->{poke} // $self->{poke};
	my $eh		= $self->{eh} // croak "No expect handle walk_these.\n";
	my $bgjob	= $hr->{bgjob} // croak "No bgjob walk_these.\n";
	my $fgpoke	= $hr->{fgpoke} // croak "No cli interactive prompt.\n";
	my $scmd	= $hr->{subcmd} // croak "Sub command required.\n";
	my @args	= (defined $ap) ? @{$ap} : ();
	my $bef		= '';

	$self->_deamon_check($hr);
	waiter(($hr->{wait} // 0), ($self->{scomm} // 0));
	# my $wtime	= int(($hr->{wait} // 0) * 1_000_000);
	# usleep($wtime);
	
	# Get Expect ready for an interactive session.
	my $opoke	= $self->{poke};
	$self->{poke}	= $fgpoke;
	$eh->print("$pnam\n");
	my $phr		= $self->_prompt();
	croak "In _walk_these, error awaiting prompt $fgpoke: $phr->{err}.\n"
		if (defined $phr->{err});

	# The game plan is when my subcmd is set config, I have some args
	# coming in as cmd/param pairs. The exception is the rffe.chn
	# command. I use the @param array to store the previous command
	# in a set config scmd. I expect a numeric argument as the second
	# part of the command.
	my @param	= ();
	for my $cmd (@args)	{
		if ($scmd =~ m/^set/)	{	# Set commands have params
			if ($cmd =~ m/^rffe/)	{ # Enable rffe.chn
				$eh->print("enable $cmd\n");
			}
			else	{
				if ($cmd =~ m/^\d+/)	{
					croak "No cmd for $cmd.\n"
						unless (@param);
					$eh->print("$scmd $param[0] $cmd\n");
					@param	= ();
				}
				else	{
					push @param, $cmd;
					next;
				}
			}
		}
		else	{
			$eh->print("$scmd $cmd\n");
		}
		$phr	= $self->_prompt();
						
		croak "In _walk_these loop prompt $fgpoke: $phr->{err}.\n"
			if (defined $phr->{err});
		$bef	.= $phr->{bef};
	}

	if ($fgpoke =~ m/opencellular/)	{ # In occli
		$eh->print("quit\n");
	}
	else	{			# I must be in sudo su
		$eh->print("exit\n");
	}
	$self->{poke}	= $opoke;
	$phr		= $self->_prompt();
	croak "In _walk_these, after quit: $phr->{err}.\n"
		if (defined $phr->{err});
	my $exitc	= $self->_exit_code() if (defined $xcode);
	$self->_report_exc($xcode, $exitc, $tname, $tnum, $tid)
		if ($self->{rep_ok});
	$hr->{bef}	= $bef;

	return unless (defined $hr->{res});
	return $self->_residual_tests($hr);
}
	
# Here are the rules. If the program is in /usr/local/fbin, I assume
# I have a power-related request and need to use the Expect handle
# from peh for the next test step.
# Otherwise, use the original Expect handle from our oeh key.
# If I have a power-related request and peh is not defined,
# just return to the caller an undef.
sub _find_handle	{
	my $self	= shift;
	my $hr		= shift;
	my $prog	= $hr->{prog};
	my $eh;
	my $poke;

	if ($prog =~ m{^/usr/local/fbin/})	{	# Use local eh
		$eh	= $self->{peh} // do {
			carp("Expected peh not found in _find_handle().\n");
			return; };
		$poke	= $self->{ppoke} // 
			croak("Expected ppoke not found in _find_handle().\n");
		# Change my current expect handle to the power eh.
		$self->{eh}	= $eh;
		$self->{poke}	= $poke;
		mods::misc::set_pwr($prog);	# Keep abreast of PS situ.
	}
	else	{
		$eh	= $self->{oeh} //
			croak("Expected oeh not found in _find_handle().\n");
		$poke	= $self->{opoke} //
			croak("Expected opoke not found in _find_handle().\n");
		$self->{eh}	= $eh;
		$self->{poke}	= $poke;
	}
	return $eh;
}

#
#	The _walk_this method works on an open ssh session through
#	expect.
#	What is passed to walk_this is the object that has the expect
#	handle, prompt re, and time out values.
#	To get started, a shell test command is made to ensure that
#	the executable of interest is where it is supposed to be.
#	Next, the program is run. If the exit code needs to be tested,
#	and echo $? command is issued.
#	Finally, for as many members as are in the result array,
#	walk_this checks the output text from the program.
#
sub _walk_this	{
	my $self	= shift;
	my $hr		= shift;

	my $tnum	= ($hr->{tnum} // 99) . '.0';
	my $pnam	= $hr->{prog} // croak "Must have prog defined.\n";
	my $tname	= $hr->{name} // 'unknown';
	my $tid		= $hr->{tid} // 0;
	my $ap		= $hr->{args};
	my $xcode	= $hr->{xcode};
	my $serrs	= $self->{nerr};
	$self->{to}	= $hr->{to} // $Stdto;
	# For this test, we change to a new prompt, if defined.
	$self->{poke}	= $hr->{poke} // $self->{poke};
	# my $eh	= $self->{eh} // croak "No expect handle walk_this.\n";
	my $eh		= $self->_find_handle($hr);
	croak("No Expect handle for $tname!\n") unless (defined $eh);
	$eh->print_log_file("\n\ntid: $tid.\n") if ($tid);
	my @args	= (defined $ap) ? @{$ap} : ();
	return if ($self->_no_exe($pnam, $tnum)); # No executable by this name.
	
	return $self->_walk_these($hr) if ($pnam =~ m/occli$/);
	return $self->_walk_these($hr) if ($pnam =~ m/sudo su$/);
	push @args, tod_set() if (defined $hr->{timereq});
	waiter(($hr->{wait} // 0), ($self->{scomm} // 0));
	# my $wtime	= int(($hr->{wait} // 0) * 1_000_000);
	# usleep($wtime);
	$eh->print("$pnam @args\n");
	my $phr		= $self->_prompt();
	croak "Prompt error running $pnam: $phr->{err}.\n" 
		if (defined $phr->{err});

	my $exitc	= $self->_exit_code() if (defined $xcode);
	$self->_report_exc($xcode, $exitc, $tname, $tnum, $tid) 
		if ($self->{rep_ok});
	$hr->{bef}	= $phr->{bef};
	return if ($serrs != $self->{nerr});	
	return unless (defined $hr->{res});
	return $self->_residual_tests($hr);
}

sub pwr_exc	{
#	Here I report the results of an exit code during power setup.
#	My $ec argument is my exit code. If 0, ok. If != 0, fail.
	my $hr		= shift;
	my $eec		= $hr->{xcode};		# Expected exit code.
	my $ac		= $hr->{actual};	# Actual exit code value.
	my $pname	= $hr->{name};
	my $tnum	= $hr->{tnum} . '.0';
	my $tid		= $hr->{tid} // 0;
	my $sock	= $hr->{scomm};
	my $ecerr	= 0;

	if (defined $eec)	{	# We have an exit code to match.
		$ecerr	= ($eec == $ac) ? 0 : 1;
	}
	else 	{
		$eec	= $ac = 'na';
	}

	my $pf		= ($ecerr) ? 'fail' : 'pass';
	$hr->{pf}	= $pf;

	if (defined $sock)	{
		print $sock "$tnum: $pf.\n";
	}
	else	{ # Send to tty.
		my $res	= ($ecerr) ? (RED . 'fail') : (GREEN . 'ok');
		my $nam	= "$tnum,";

		$res	.= RESET . ".\n";
		print	 "Setup $nam " . BLUE . $pname . RESET 
			. " exit $eec: $res";
	}
	#	The power sequence at startup cannot have failure.
	#	Any detected failure kills the test and we attempt to shutoff.
	if ($ecerr)	{
		$hr->{nerr}++;
		mods::misc::power_off('return');
		croak("pwr_exc() setup failure, cannot continue.\n");
	}

	my $v		= {	name => $pname, expected_exit_code => $eec, 
				actual_exit_code => $ac, pf => $pf};
	$v->{tid}	= $tid if ($tid);
	$hr->{result}{pwr}	= {$tnum, $v};
}

#	This is for a residual power parameter test.
sub report_pwr_param	{
	my $hr		= shift;
	my $pname	= $hr->{name};
	my $tnum	= $hr->{tnum};
	my $tid		= $hr->{tid} // 0;
	my $pf		= $hr->{pf};
	my $min		= $hr->{min} // 'unknown';
	my $max		= $hr->{max} // 'unknown';
	my $act		= $hr->{actual} // 'unknown';
	my $sock	= $hr->{scomm};
	my $pft		= ($pf) ? 'pass' : 'fail';

	if (defined $sock)	{	# Send message to socket.
		print $sock "$tnum: $pft.\n";
	}
	else	{ # Send to tty.
		my $res		= ($pf) ? (GREEN . 'ok') : (RED . 'fail');
		$res		.= RESET . ".\n";
		print	"Setup $tnum, " . BLUE . $pname . RESET . 
			" min, max, actual; $min, $max, $act: $res";
	}
	#	The power sequence at startup cannot have failure.
	#	Any detected failure kills the test and we attempt to shutoff.
	unless ($pf)	{
		$hr->{nerr}++;
		mods::misc::power_off('return');
		croak("report_pwr_param() setup failure, cannot continue.\n");
	}

	my $v	= {	name => $pname, min => $min, max=> $max, 
			actual => $act, pf => $pft};
	$v->{tid}		= $tid if ($tid);
	$hr->{result}{pwr}	= {$tnum, $v};
}

#
#	This function handles the residual tests used in the power sequence.
#
sub res_pwr_tests	{
	my $hr		= shift;
	my $beftxt	= $hr->{bef};
	my $minort	= 1;
	my $tnum	= $hr->{tnum};
	my $tid		= $hr->{tid} // 0;
	my $tname;

	foreach my $rhr (@{$hr->{res}})	{
		my $re	= $rhr->{re};
		$tname	= $rhr->{name};
		# print "Bef txt: $beftxt.\n" if ($tname =~ m/start\.test\.p/);
		my $fp		= $rhr->{func};
		my $ret		= $fp->($beftxt, $rhr);
		$rhr->{tnum}	= "$tnum.$minort";
		$rhr->{scomm}	= $hr->{scomm};
		$rhr->{$_}	= $ret->{$_} for (keys %{$ret});
		if (defined $ret->{min})	{
			# This test was for parametric data.
			report_pwr_param($rhr);
		}
		else	{
			croak("res_pwr_tests() unexpected return from func.\n");
		}
		$minort++;
	}
}

sub pwr_prompt	{
	my $eh		= shift;
	my %info	= @_;
	my $stdre	= qr/xyz\$ /;
	my $to		= $info{to} // $Stdto;
	my $re		= $info{re} // $stdre;
	my @rets	= $eh->expect($to, -re => $re);
	return ($rets[1], $rets[3]);
}
#
#	This function is similar to the _walk_this method, but for
#	power supply setup only.
#	We don't use the expect handle for this step.
#
sub pwr_this	{
	my $hr		= shift;

	my $tnum	= ($hr->{tnum} // 'PWR') . '.0';
	my $pnam	= $hr->{prog} // croak "Must have prog defined.\n";
	my $tname	= $hr->{name} // 'unknown';
	my $tid		= $hr->{tid} // 0;
	my $ap		= $hr->{args} // [];
	my $xcode	= $hr->{xcode};
	my $to		= $hr->{to} // $Stdto;
	my $serrs	= $hr->{nerr} // 0;
	my $eh		= $hr->{peh} // croak "Power expect handle needed.\n";
	my $poke	= $hr->{ppoke} // croak "Need power prompt.\n";
	my ($bef, $err);

	my $wtime	= int(($hr->{wait} // 0) * 1_000_000);
	usleep($wtime);
	$eh->print("$pnam @{$ap}\n");
	($err, $bef)	= pwr_prompt($eh, to => $to, re => $poke);
	croak "pwr_this() prompt error 1 running $pnam: $err.\n" 
		if (defined $err);
	mods::misc::set_pwr($pnam);	# Keep power supply status up to date.
	$hr->{bef}	= $bef;
	# $hr->{bef}	= `$pnam @{$ap}`;
	# $hr->{actual}	= ($? >> 8);
	$eh->print('echo $?' . "\n");
	($err, $bef)	= pwr_prompt($eh, to => 3, re => $poke);
	croak "pwr_this() prompt error 2 running $pnam: $err.\n" 
		if (defined $err);
	$bef	=~ s/\r//g;
	($hr->{actual})	= $bef =~ m/^(\d+)$/m;
	pwr_exc($hr);
	return if ($serrs);	
	return unless (defined $hr->{res});
	return res_pwr_tests($hr);
}

sub nspaces	{
	my $txt	= shift;
	my $n	= shift;
	return $n - length($txt);
}

sub post_choice	{
	my ($k, $v)	= @_;

	my $txt		= " select ";
	my $postfx	= ": ";
	my $n		= nspaces(($v. $k . $txt. $postfx), 65);
	my $line	= ' ' x $n;
	$line		.= BLUE . $k . RESET;
	$line		.= $txt . CYAN . $v . RESET . $postfx;
	print "$line\n";
	if ($v == 99)	{
		my $prompt	= "Enter selection: ";
		$n		= nspaces($prompt, 65);
		print ' ' x $n, $prompt;
	}
}

sub user_prompt	{
	my ($k, $v)	= @_;
	my $prompt;
	my $txt;

	if ($k eq 'user')	{
		$prompt	= 'Enter test operator name';
	}
	elsif ($k eq 'sn')	{
		$prompt = 'UUT Serial Number';
	}
	else	{
		croak "Do not know key $k in user_prompt.\n";
	}
	while (1)	{
		my $n	= nspaces(($v . $prompt), 45);
		my $pln	= ' ' x $n;
		$pln	.= $prompt;
		$pln	.= ' [' . YELLOW . $v . RESET . ']: ';
		print $pln;
		$txt	= <>;
		chomp($txt);
		return $v if ($txt eq '');
		$v	= $txt;
	}
}

#	Get last user info from json config file.
#	Or if empty dir, return empty hash reference.
sub get_user	{
	my $dir	= shift;
	my $rev	= shift // '1.0';
	return {} if ($dir eq '');
	my $fn	= "$dir/config/fbt.config";
	my $hr;
	# print "I see a rev of $rev.\n";
	
	if (-e $fn)	{
		my $txt	= do {
				local $/;
				open my $fh, '<', $fn 
					or croak "Cannot read $fn: $!.\n";
				<$fh>;
			};
		$hr	= decode_json $txt;
	}
	else	{
		$hr	= {
			test_info	=> {
				user	=> '',
				sn	=> '',
			},
		};
	}
	$hr->{test_info}->{time}	= scalar(localtime);
	$hr->{test_info}->{rev}		= $rev;
	$hr->{test_info}->{agile}	= "T18-DOC-000038 Rev 003";

	for my $k (qw/user sn/)	{
		my $v	= $hr->{test_info}->{$k};
		$hr->{test_info}->{$k}	= user_prompt($k, $v);
	}
	my $js	= to_json($hr, {ascii => 1, pretty => 1});
	open my $fh, '>', $fn or croak "Cannot write in get_user $fn: $!.\n";
	print $fh $js;
	close $fh;
	return $hr;
}

# I want to read-only my config info.
sub ro_user	{
	my $dir	= shift;
	my $rev	= shift // '1.0';
	return {} if ($dir eq '');
	my $fn	= "$dir/config/fbt.config";
	my $hr;
	# print "I see a rev of $rev.\n";
	
	if (-e $fn)	{
		my $txt	= do {
				local $/;
				open my $fh, '<', $fn 
					or croak "Cannot read $fn: $!.\n";
				<$fh>;
			};
		$hr	= decode_json $txt;
	}
	else	{
		$hr	= {
			test_info	=> {
				user	=> '',
				sn	=> '',
			},
		};
	}
	$hr->{test_info}->{time}	= scalar(localtime);
	$hr->{test_info}->{rev}		= $rev;
	return $hr;
}

# Write out the new config info.
sub w_user	{
	my $dir	= shift;
	my $hr	= shift;
	my $fn	= "$dir/config/fbt.config";

	my $js	= to_json($hr, {ascii => 1, pretty => 1});
	open my $fh, '>', $fn or croak "Cannot write in get_user $fn: $!.\n";
	print $fh $js;
	close $fh;
}

sub nasty {
	my $v	= shift;
	my $txt	= "Cannot select " . UNDERLINE . RED . $v . RESET . ".\n";
	print $txt;
}

sub check_range	{
	my $n	= shift;
	my $txt	= shift;

	my ($s, $e)	= $txt =~ m/(\d+)-(\d+)/;
	croak "Could not parse $txt in check_range().\n" unless (defined $e);

	if ($s > 0 && $e <= $n)	{ return ($s .. $e); }
	return ();
}
	
sub print_menu	{
	my @ahr		= @_;
	my @rang	= ();
	my $ans;
	
again:
	print "\n";
	my $n	= 1;
	foreach my $hr (@ahr)	{
		my $name	= $hr->{name};
		post_choice($name, $n++);
	}
	post_choice("Exit program", 99);
	chomp($ans = <>);
	if ($ans =~ m/^99$/) { return (); }
	if ($ans !~ m/^\d+(?:-\d+)?$/)	{ nasty($ans); goto again; }
	if ($ans =~ m/-/)	{ 
			@rang	= check_range($n, $ans); 
			if (@rang) { return @rang; }
			else { nasty($ans); goto again; }
	}
	elsif ($ans > 0 && $ans < $n) { return ($ans); }
	else { nasty($ans); goto again; }
}

sub eval_version        {
        my $bin         = shift;
        my $fn          = "$bin/config/build_ver.conf";
        croak "Cannot find $fn.\n" unless (-f $fn);
        my $txt         = do    {
                                local $/;
                                open my $fh, '<', $fn
                                        or croak "Cannot read $fn: $!.\n";
                                <$fh>;
                        };
        my $v           = eval $txt;
        croak "Syntax problem:\n$@.\n" if ($@);
        return $v;
}

my %Hc	= (
	1 => ON_GREEN,
	2 => ON_GREEN,
	3 => ON_GREEN,
	4 => ON_GREEN,
	5 => ON_GREEN,
	6 => ON_GREEN,
	7 => ON_GREEN,
	8 => ON_GREEN,
	9 => ON_GREEN,
	10 => ON_GREEN,
	11 => ON_GREEN,
	12 => ON_GREEN,
	13 => ON_GREEN,
	14 => ON_GREEN,
	rs => RESET,
	);

sub led_inquire	{
	my $bef	= shift;
	my $hr	= shift;
	my $led	= $hr->{led};

	fill_in(\%Hc, $led);

	my $res;
	my $intest	= $hr->{intest} // 0;
	my $msg		= $led->{msg};
	my $tline	= get_lines(\%Hc);
	print $tline;

	ASK_AGAIN:
	print "Are $msg (y/n) ";
	if ($intest)	{	# I'll provide my own answer if I am in an 
		$res	= 'y';	# automated test.
		usleep(2_000_000);
		print "\n";
	}
	else	{
		$res	= <>;
		chomp $res;
	}
	syswrite(STDOUT, "\e[K");	# Clear to end of line.
	if ($res =~ m/y|n/i)	{
		for (my $i = 1; $i < 9; $i++)	{
			syswrite(STDOUT, "\e[1A");	# Up 1 line.	
			syswrite(STDOUT, "\e[K");	# Clear to end of line.
		}
		my $pf	= ($res eq 'y') ? 1 : 0;
		return {pf => $pf, mtxt => $res};
	}	
	print BOLD . RED . "Say what?" . RESET. " I need y or n.\n";
	syswrite(STDOUT, "\e[2A");	# Up 2 lines.	
	syswrite(STDOUT, "\e[K");	# Clear to end of line.
	goto ASK_AGAIN;
}

sub fill_in	{
	my $hr	= shift;
	my $thr	= shift;

	my $ap	= $thr->{range} // die "Range key is undef.\n";
	my @r	= @{$ap};
	my $c	= $thr->{color};

	for my $k (@r)	{
		$hr->{$k}	= $c;
	}
}

sub get_lines	{
	my $hr	= shift;
	my %Hc	= %{$hr};
my $tplt	= 
#      12       34      56       78      90       12      34       56
"$Hc{4} 4$Hc{rs}  $Hc{5} 5$Hc{rs}  $Hc{6} 6$Hc{rs}  $Hc{7} 7$Hc{rs}  " .
"$Hc{8} 8$Hc{rs}  $Hc{9} 9$Hc{rs}  $Hc{10}10$Hc{rs}  $Hc{11}11$Hc{rs}  " .
"\n\n" .
"$Hc{3} 3$Hc{rs}                          $Hc{12}12$Hc{rs}\n\n" .
"$Hc{2} 2$Hc{rs}                          $Hc{13}13$Hc{rs}\n\n" .
"$Hc{1} 1$Hc{rs}                          $Hc{14}14$Hc{rs}\n";
return $tplt;
}


1;

__END__

=head1 NAME

mods::walker - Utilities supporting OpenCellular Connect-1 testing.

=head1 SYNOPSIS

 use mods::walker qw/date_range eval_version field_match
                     get_user print_menu std_match std_1_match
                     tod_match range_match/;

 # Create mods::walker object
 my $wh = mods::walker->new(get_user($conf_dir[, $ver]), $hr);

The B<mods::walker> module is a group of routines that support 
the step by step walking through a list of tests that need 
to be performed.

The call on I<get_user()> returns a hash reference 
containing general test information. This hash reference 
from I<get_user> is stored by the B<mods::walker> object 
for later use. Among other things, this hash reference 
is where 
test results will be stored and an empty hash reference may be 
used instead of a call on I<get_user()>. 

 # Create mods::walker object without general test information
 my $wh = mods::walker->new({}, $hr);

The I<get_user()>  
function returns 
keys that add information about the test such as the operator 
name, the serial number of the UUT, the time of day of the test, 
and the software version.

The hash reference B<$hr> is stored within the 
B<< mods::walker->new() >> object. 
The B<$hr> argument 
passes a hash reference with the following required keys: 

=over

=item acct

The key B<acct> defines the account to use when logging into the UUT.

=item eh

The B<eh> key points to the Expect(3pm) handle for the open session 
that this object requires.

Note that an Expect(3pm) handle must be defined before a call to 
B<< mods::walker->new() >> is made.

=item ip

The B<ip> key is the network address of the UUT.

=item poke

The B<poke> points to a regular expression that defines 
the prompt to be used by the Expect(3pm) module.

=item pw

The B<pw> key is the password to be used with the B<acct> name 
when loggin into the UUT.

=item tdir

Test results will be stored in the directory pointed to by the B<tdir> 
key.

=back

=head1 EXPORTED FUNCTIONS

As of this writing, exported functions from B<mods::walker> 
are of two types, general and residual test.

The general functions are not called by a specific test. 
These functions may support test setup or 
how a particular test item is accessed. 
In the case of I<get_user()>, this routine inquires 
from the operator things such as user name and UUT 
serial number.

The residual test functions are called as tests 
on text provided by a previous major test step. These 
residual tests examine the text output from the 
major test step and can parse for ranges 
or exact textual matches.

=head3 GENERAL TEST FUNCTIONS

 $version = eval_version($bin_dir);

The B<eval_version()> function opens a file 
under the directory I<$bin_dir/config> by 
the name of I<build_ver.conf>. This file 
is created by the development station's 
B<mk_fb_apps> program. This program uses 
the git(1) repository tag for this project and combines 
that tag with the number of commits 
of that repository to form 
a version in the following format:

 major.minor-commit

The major and minor release numbers are from 
the repository's tag. The number 
of git(1) commits on this repository 
is the commit number. 

Note that the content of the I<$bin_dir/config/build_ver.conf> 
file is simply Perl code. That is why the function will 
I<eval> the file contents in order to return 
the I<$version>.

 $ret_hr = get_user($conf_dir, [$ver]);

The B<get_user()> function returns a hash reference with 
the key B<test_info> defined. General test information 
is returned by B<get_user()> under the B<test_info> key. This  
information includes the software version number, 
the serial number of the UUT, the time of day for this test, 
and the test operator's name.

Arguments to B<get_user()> is the directory where the 
I<config/fbt.config> file may be found and an optional 
version number. If no version number is given, 
the value 1.0 is used. 

After a dialog with the operator, the B<get_user()> function 
writes out new test information into the 
I<config/fbt.config> file. The prompts to the operator 
contain last used values from the previously written I<config/fbt.config> 
file. This speeds up data entry. When no new data need 
be typed, the operator simply uses the Enter key to use 
the existing data entry and move to the next field.

 print_menu($ahr);

The B<print_menu()> function is the function that allows 
interactive access to any test passed to it
through the B<$ahr> argument.

The B<$ahr> argument is an array of hash references 
that define major and residual tests. When the user
selects test number 99, the B<print_menu()> function 
returns. Otherwise, the B<print_menu()> function 
will continue to prompt the user for a test selection.

Use the B<print_menu()> function to allow random access 
to any defined test sequence.


=head3 RESIDUAL TEST FUNCTIONS

The residual test functions look for specific information from 
the output from each fbt(1) major test step. After each major step 
executes, its output text is preserved for examination 
by the residual test steps. The residual test steps 
combine regular expressions and residual test functions 
to form a hash reference with test results.

Each hash reference defining a residual test must have a 
key named B<func> whose value is a function reference. 
Each of the residual tests below will be called with two 
arguments. If B<$rhr> is the residual test hash reference, 
the call to the residual test function will look like 
this at run time:

 $rhr->{func}->($beftxt, $rhr);

The argument B<$beftxt> is all of the text output 
from the major test up to the prompt. The argument 
B<$rhr> is a copy of the residucal test hash 
reference. Therefore, each residual test function 
has access to all residual test keys.

=head4 date_range()

 $ret_hr = date_range($txt, $hr);

The B<date_range()> function returns a hash reference containing test results 
from the comparison of B<$txt> against the regular expression pointed to 
by the B<$hr> hash reference.

The hash reference must have an B<re> key. The general 
format of the date used with this function is:

 Nov 01, 2016

As a regular expression:

 m/([a-z]{3})\s+(\d{1,2}),\s+(\d{4})/i
 # Example presented text:
 # Aug 8, 1997.
 # After re match:
 #        Aug           8       1997
 # Capture $1          $2         $3

The capture field B<$1> is a three character abbreviation 
of the month. The B<$2> field is the day of the month and 
the B<$3> field is the year. The 'i' option 
allows case folding.

Two optional keys may be provided: B<min> and B<max>. If 
neither of these is defined, the extracted date 
text will be compared against November 1, 2016 as 
the minimal acceptable date value. If only a minimal 
value is given, the B<date_range> function assures 
that the minimal date is tested and the maximum 
ceiling is set to 'na'. 

When both B<min> and B<max> are provided, the extracted 
date must fall between these two dates in order to pass. 

=head4 date_range() Example

 {
  prog    => '/usr/local/bin/whatsystem',
  res     => [
             {
              name    => 'FPGA Date',
              re      => qr/^FPGA Build Date: ([^\.]+)/m,
              func    => \&date_range,
             },
            ],
  xcode   => 0,
  name    => 'Run whatsystem',
  to      => 3,
 },

In the example above, the program B<whatsystem> is the major 
test step. Its exit code will be checked and must be zero 
to pass. The residual test step passes a pointer to the function 
B<date_range()> using the B<func> key.

The B<re> specifies that the output from B<whatsystem> must 
have  
one line that starts with 'FPGA Build Date: ' followed by 
a date in the capture field. The regular expression 
that captures the date is simply one or more characters 
that are not periods. The B<whatsystem> command 
has its dates terminated with a period character.

=head4 tod_match()

 $ret_hr = tod_match($txt, $hr);

The B<tod_match()> function expects a regular expression to capture 
date and time information contained within the text of the 
previously executed command. Once the text string is extracted, 
the B<tod_match()> function converts this information to the number 
of non-leap seconds since the epoch and subtracts this information from the 
current time on the test system. B<tod_match()> will pass if 
both the regular expression matches and the time 
difference is within 
120 seconds of the current time.

The key B<timereq> being set in the enclosing test forces the 
time of day and date to be set in the UUT. Once set, the test 
system and UUT clocks should be in close agreement. Refer to 
the key B<timereq> above.


=head4 tod_match() Example:

 {
   prog    => '/usr/bin/sudo',
   args    => ['/bin/date'],
   timereq => 1,
   res     => [
              {
                name    => 'verify time',
                re      => qr/^([MTWFS][a-z]{2}\s.+)$/m,
                func    => \&mods::walker::tod_match,
               },
               ],
   xcode   => 0,
   name    => 'Set time of day',
   tid     => 't_11',
   to      => 3,
 },

The B<tod_match()> example above has the key B<timereq> set. Therefore, 
this test step calls date(1) with the test station's time information. 
It next checks the program's exit code, expecting a zero return.

The residual test on the output from date(1) looks for text matching
this regular expression:

 ^([MTWFS][a-z]{2}\s.+)$

This regular expression reads, "Find a new line that begins with M, T, 
W, F, or S. This first character is followed by two lower case characters 
followed by a space. At this point grab one or more characters that 
are not a new line character until the end of the line."

This text matches the regular expression:

 Wed Jun  7 09:24:00 CDT 2017

After the text is matched, the B<tod_match()> function compares the date 
and time provided with the test system's clock to verify that 
each is within 120 seconds of each other.

=head4 field_match()

 $ret_hr = field_match($txt, $hr);

The B<field_match()> function can match several fields 
in the text argument. The returned hash reference 
contains a pass or fail flag, the expected number 
of matches, the actual number 
of matches extracted, and the number 
matched.

The B<$hr> hash reference passed is required 
to contain at least these two keys: B<re> and B<marry>. 

The B<re> regular expression will contain a number 
of capture fields and these capture fields match 
the members within the B<marry> array. 

The key B<marry> must point to an 
array of text items or quoted regular expressions.

The number of capture fields within B<re> must match 
the number of members within the B<marry> array. 

The B<field_match()> function attempts to extract the 
B<re> defined capture fields. If the number of actual extracted 
fields does not match the length of the B<marry> array, 
the test fails. If the extracted number of fields matches 
the number of strings within B<marry>, then each matched 
field is compared against each string within the B<marry> 
array. If all compare, the B<field_match()> will set 
the pass fail flag. Otherwise, any mis-compares will fail 
this test.

A nice feature of the B<field_match()> function is that 
strings within the B<marry> array may be either 
text strings or quoted regular expressions. This 
allows the B<re> to contain general text captures 
that will be specifically analyzed by a regular 
expression within the B<marry> array.

=head4 field_match() Example

 # These HRs are individual tests. Each has all info needed.
 # The prog key points to the execuable.
 # A shell test -x will be run before its launch.
 {
   prog    => '/bin/sleep',
   args    => ['10'],
   # The res key is a pointer to an array of residual tests.
   # Each res entry will be a test on the text matched before the prompt.
   res     => [
              # The HRs within res have keys for test name, regular
              # expression, and a subroutine reference to handle 
              # the test.
              {
              name    => 'sleep fields',
              re      => qr/sl(ee)p\s+(\d+)/m,
              func    => \&field_match,
              marry   => ['ee', qr/^10$/],
              },
             ],
   # The xcode key is a flag to check the exit code of this
   # program.
   xcode   => 0,
   name    => 'sleep 10',
   to      => 12,
 },

In the example above, the program sleep(1) is being run 
and the text echoed back to the Expect(3pm) module 
is analyzed. First the text must match the B<re> key. 

In this case, the regular expression defined by B<re> 
specifies two capture fields. The first is 'ee' appearing 
after 'sl' and before 'p\s+' with no other characters. 

The second capture field simply picks up any decimal 
characters after 'p\s+'. Also note the regular expression 
option of 'm'. This option allows a multiple line search 
and in this example is not strictly needed since 
the B<re> contains no line anchors. Thus the B<re> will 
search the entire text looking for a match.

In the B<marry> array, the first capture field must match 
the text 'ee' exactly. The second capture in B<marry> is 
a quoted regular 
expression. It specifies that the tested string must 
start with a '1' which is then followed by a '0' and then 
an end of line (or string) is expected.

The ability to use the B<re> key to capture general text 
fields and then use the regular expressions contained 
within B<marry> allows very flexible pattern matching.

=head4 std_match()

 $ret_hr = std_match($txt, $hr);

The B<std_match()> function matches the regular expression 
defined by the key B<re> in the B<$hr> hash reference against 
the provided text in the B<$txt> argument.

The return of B<std_match()> is a hash reference with a pass 
fail flag S<(B<pf>)>. There is also a matching text key S<(B<mtxt>)>. If 
there is a failure to match, diagnostic information 
is provided in the B<mtxt> key.

=head4 std_match() Example:

 {
   prog    => '/bin/bash',
   args    => ['-c', q!'echo hello; exit 99'!],
   res     => [
              {
                name    => 'bash prints hello',
                re      => qr/hello/m,
                func    => \&std_match,
               },
              ],
   xcode   => 99,
   name    => 'Run bash, exit 99',
   to      => 3,
 },

In the B<std_match()> example above, the major test runs 
a sub-shell forcing it to print 'hello' and exit 
with an exit code of 99. 

The residual test wants to search the output text for the 
characters 'hello'. In this example, the 'm' option is used 
with the B<re>. This option is not strictly needed since 
there are no line anchors in the regular expression. With or 
without the 'm' option, the text is searched for 'hello' 
starting from the beginning 
until a match or end of text is encountered.

The B<std_match()> function will be run on the text output 
from the major test and if the B<re> matches, the B<pf> flag 
will be set to true.

=head4 pwr_this()

 pwr_this($hr);

The B<pwr_this()> function allows the use of hash references 
that are similar to hash references used to test the UUT. 
Powering the UUT may be the very first step in the fbt(1) 
program. If power is needed before B<mods::walker->new()> is 
called, the B<pwr_this()> function is used to perform 
similar steps to those used with the UUT. 

The B<pwr_this()> function is used in the B<mods::misc> module. 
Within the B<mods::misc> module, a local shell handle is opened 
for use by B<pwr_this()>. The normal keys used for tests are expected 
by B<pwr_this()> with these additions: 

The key B<plfn> is the absolute file name path to the log file to 
be used to log any power related conversations.

The key B<peh> is the Expect(3pm) handle used to communicate to 
the local Linux shell for use with all power related activities.

The key B<ppoke> is the prompt to be used in conjunction with 
the B<peh> handle. 

=head4 led_inquire()

 $ret_hr = led_inquire($txt, $hr);

The B<led_inquire()> function inquires from the operator the status 
of the LED CCA attached to the UUT. On a terminal, the operator will 
be provided with an outline of the LED positions with appropriate 
colors. When used through a GUI, a similar output is also displayed. 

After the user answers a yes or no question about whether the LED 
card displays the appropriate LED color sequence, that answer is 
returned in hash reference with keys B<pf> and B<mtxt> set. 

=head4 range_match()

 $ret_hr = range_match($txt, $hr);

The function B<range_match()> uses tolerances specified in the 
module B<mods::tolerance.pm>. The key B<name> for this residual 
test must match a key within the B<%Tol> hash contained within 
the B<mods::tolerance.pm> file.

The objective of the B<range_match()> function is to extract 
a field out of the enclosing test output text and check its 
value against the minimum and maximum values allowed 
as specified with the B<%Tol> hash.

=head4 range_match() Example:

 {
  prog    => '/usr/local/fbin/p1stat',
  res     => [
             {
              name    => 'start.std.p1v',
              re      => qr/,\s([\.\d]+)\s+volts/m,
              func    => \&mods::walker::range_match,
             },
             {
              name    => 'start.std.p1i',
              re      => qr/p1:\s([\.\d]+)\s+amperes/m,
              func    => \&mods::walker::range_match,
             },
             ],
             xcode   => 0,
             name    => 'ps1stat',
 },

The example use of the B<range_match()> function above uses 
the keys B<start.std.p1v> and B<start.std.p1i> contained within 
the B<mods::tolerance.pm> file. These two keys have 
tolerance ranges for the voltage and current expected at this 
step in the test.

Note that there is a capture field within each of the two regular 
expressions in the example above. The values parsed out of the returned 
text from the B<p1stat> program is compared with the tolerance 
specified by the key B<name> 
and a determination is made to see if it is within tolerance. 

See the B<mods::tolerance.pm> file for specific test tolerences.

=head4 std_1_match()

 $ret_hr = std_1_match($txt, $hr);

The B<std_1_match()> function is equal to the B<std_match()> fuction 
in all respects except that B<std_1_match()> will extract a field 
from the matched text and report this field in the test results. 

A key by the name of B<mval> will contain the matched text 
embedded capture field.

=head4 std_1_match() Example:

 {
        name    => 'Find size',
        re      => qr/Capacity:\s+(.+?)$/m,
        func    => \&std_1_match,
 },

Assume that the text B<std_1_match()> is presented with for the 
residual test above is the following:

 User Capacity:    60,022,480,890 bytes [60.0] GB

Then value of the B<mval> key will be:

 60,022,480,890 bytes [60.0] GB

The way the regular expression for the residual test above reads 
is as follows: In a multi-line text string, look for 'Capacity'. It 
is followed by a colon and then by one or more spaces. After the 
spaces, take one or more of any character into the capture field 
up to but not including the end of line. 

=head1 FILES

/usr/local/fbin/mods/walker.pm -- Module file name.

/usr/local/fbin/config/build_ver.conf -- Build version file.

/usr/local/fbin/config/fbt.config -- Last test configuration information.

=cut

