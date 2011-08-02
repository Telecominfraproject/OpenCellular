#!/usr/bin/perl

use strict;
our $opt_u = 'http://www.chromium.org';
our $opt_m = 'Unsupported Prototype 0000';
our $opt_d = '.';

use File::Basename;
my $progdir = dirname($0);
my $prog = basename($0);

use Getopt::Std;
my $usage = "
Usage:  $prog

";
getopts('u:m:d:') or die $usage;

my @old = glob("$opt_d/linetxt_*");
unlink(@old) if @old;

$/ = undef;
$_ = <>;
s/\s+$//gs;

my $count = 1;
foreach (split(/\n/, $_))
{
    s/^\s+//;
    s/\s+$//;
    s/\$URL/$opt_u/g;
    s/\$MODEL/$opt_m/g;
    $_ = ' ' unless $_;
    my $big = s/^\$BIG:\s*//;
    my $filename = sprintf('%s/linetxt_%02d.%s', $opt_d, $count++,
                           $big ? 'TXT' : 'txt');
#    print "$filename: ($_)\n"; next;
    open(OUT, ">$filename") || die "$0 can't write $filename: $!\n";
    print OUT "$_";
    close(OUT);
}
