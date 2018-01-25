# Copyright (c) 2017-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.

package mods::Logio;

sub TIEHANDLE	{
	my $class	= shift;
	my $outfhs	= [ @_ ];
	return bless $outfhs, $class;
}

sub PRINT	{
	my $fhs	= shift;
	my $sh	= $fhs->[1];
	my $lh	= $fhs->[0];
	print $sh @_;
	print $lh '<c ',  @_;
}

sub READLINE	{
	my $fhs	= shift;
	my $rh	= $fhs->[1];
	my $lh	= $fhs->[0];

	my $line	= <$rh>;
	print $lh '>c ' . $line;
	return $line;
}

1;
