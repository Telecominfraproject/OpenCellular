#!/bin/bash
#
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# Generate version information

if ghash=$(git rev-parse --short --verify HEAD 2>/dev/null); then
	if gdesc=$(git describe --dirty --match='v*' 2>/dev/null); then
		IFS="-" fields=($gdesc)
		tag="${fields[0]}"
		IFS="." vernum=($tag)
		numcommits=$((${vernum[2]}+${fields[1]:-0}))
		ver_major="${vernum[0]}"
		ver_branch="${vernum[1]}"
	else
		numcommits=$(git rev-list HEAD | wc -l)
		ver_major="v0"
		ver_branch="0"
	fi
	# avoid putting the -dirty attribute if only the timestamp
	# changed
	dirty=$(sh -c "[ '$(git diff-index --name-only HEAD)' ] \
                && echo '-dirty'")
	ver="${ver_major}.${ver_branch}.${numcommits}-${ghash}${dirty}"
else
	ver="unknown"
fi

date=$(date '+%F %T')

echo "const char futility_version[] = \"${ver} ${date} ${USER}\";";
