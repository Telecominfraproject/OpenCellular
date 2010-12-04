#!/bin/sh
#
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This attempts to guide linux users through the process of putting a recovery
# image onto a removeable USB drive.
#
# We may not need root privileges if we have the right permissions.
#
set -eu

##############################################################################
# Configuration goes here

# Where should we do our work? Use 'WORKDIR=' to make a temporary directory,
# but using a persistent location may let us resume interrupted downloads or
# run again without needing to download a second time.
WORKDIR=/tmp/tmp.crosrec

# Where do we look for the config file?
CONFIGURL='http://www.chromium.org/some/random/place.cfg'

# What version is this script? It must match the 'recovery_tool_version=' value
# in the config file that we'll download.
MYVERSION='1.0'


##############################################################################
# Some temporary filenames
debug='debug.log'
tmpfile='tmp.txt'
config='config.txt'
version='verson.txt'

##############################################################################
# Various warning messages

DEBUG() {
  echo "DEBUG: $@" >>"$debug"
}

warn() {
  echo "$@" 1>&2
}

quit() {
  warn "quitting..."
  exit 1
}

fatal() {
  warn "ERROR: $@"
  exit 1
}

ufatal() {
  warn "
ERROR: $@

You may need to run this program as a different user. If that doesn't help, try
using a different computer, or ask a knowledgeable friend for help.

"
  exit 1
}

gfatal() {
  warn "
ERROR: $@

You may need to run this program as a different user. If that doesn't help, it
may be a networking problem or a problem with the images provided by Google.
You might want to check to see if there is a newer version of this tool
available, or if someone else has already reported a problem.

If all else fails, you could try using a different computer, or ask a
knowledgeable friend for help.

"
  exit 1
}

##############################################################################
# Identify the external utilities that we MUST have available.
#
# I'd like to keep the set of external *NIX commands to an absolute minimum,
# but I have to balance that against producing mysterious errors because the
# shell can't always do everything. Let's make sure that these utilities are
# all in our $PATH, or die with an error.
#
# This also sets the following global variables to select alternative utilities
# when there is more than one equivalent tool available:
#
#   FETCH          = name of utility used to download files from the web
#   FETCHNEW       = command to invoke to download fresh each time
#   FETCHCONT      = command to invoke to download with resume if possible
#   CHECK          = command to invoke to generate checksums on a file
#
require_utils() {
  local external
  local errors
  local tool
  local tmp

  external='cat cut dd grep ls mkdir mount readlink sed sync umount unzip wc'
  if [ -z "$WORKDIR" ]; then
    external="$external mktemp"
  fi
  errors=

  for tool in $external ; do
    if ! type "$tool" >/dev/null 2>&1 ; then
      warn "ERROR: can't find \"$tool\""
      errors=yes
    fi
  done

  # We also need to a way to fetch files from teh internets. Note that the args
  # are different depending on which utility we find. We'll use two variants,
  # one to fetch fresh every time and one to try again from where we left off.
  FETCH=
  if [ -z "$FETCH" ] && tmp=$(type curl 2>/dev/null) ; then
    FETCH=curl
    FETCHNEW="curl -f -s -S -o"
    FETCHCONT="curl -f -C - -o"
  fi
  if [ -z "$FETCH" ] && tmp=$(type wget 2>/dev/null) ; then
    FETCH=wget
    FETCHNEW="wget -nv -O"
    FETCHCONT="wget -c -O"
  fi
  if [ -z "$FETCH" ]; then
    warn "ERROR: can't find \"curl\" or \"wget\""
    errors=yes
  fi

  # Once we've fetched a file we need to compute its checksum. There are a
  # couple of possiblities here too.
  CHECK=
  if [ -z "$CHECK" ] && tmp=$(type md5sum 2>/dev/null) ; then
    CHECK="md5sum"
  fi
  if [ -z "$CHECK" ] && tmp=$(type sha1sum 2>/dev/null) ; then
    CHECK="sha1sum"
  fi
  if [ -z "$CHECK" ]; then
    warn "ERROR: can't find \"md5sum\" or \"sha1sum\""
    errors=yes
  fi

  if [ -n "$errors" ]; then
    ufatal "Some required linux utilities are missing."
  fi
}

##############################################################################
# Helper functions to handle the config file and image tarball.

# Each paragraph in the config file should describe a new image. Let's make
# sure it follows all the rules. This scans the config file and returns success
# if it looks valid. As a side-effect, it lists the line numbers of the start
# and end of each stanza in the global variables 'start_lines' and 'end_lines'
# and saves the total number of images in the global variable 'num_images'.
good_config() {
  local line
  local key
  local val
  local display_name
  local file
  local size
  local url
  local md5
  local sha1
  local skipping
  local errors
  local count
  local line_num

  display_name=
  file=
  size=
  url=
  md5=
  sha1=
  skipping=yes
  errors=
  count=0
  line_num=0

  # global
  start_lines=
  end_lines=

  while read line; do
    line_num=$(( line_num + 1 ))

    # We might have some empty lines before the first stanza. Skip them.
    if [ -n "$skipping" ] && [ -z "$line" ]; then
      continue
    fi

    # Got something...
    if [ -n "$line" ]; then
      key=${line%=*}
      val=${line#*=}
      if [ -z "$key" ] || [ -z "$val" ] || [ "$key=$val" != "$line" ]; then
        DEBUG "ignoring $line"
        continue
      fi

      # right, looks good
      if [ -n "$skipping" ]; then
        skipping=
        start_lines="$start_lines $line_num"
      fi

      case $key in
        display_name)
          if [ -n "$display_name" ]; then
            DEBUG "duplicate $key"
            errors=yes
          fi
          display_name="$val"
          ;;
        file)
          if [ -n "$file" ]; then
            DEBUG "duplicate $key"
            errors=yes
          fi
          file="$val"
          ;;
        size)
          if [ -n "$size" ]; then
            DEBUG "duplicate $key"
            errors=yes
          fi
          size="$val"
          ;;
        url)
          url="$val"
          ;;
        md5)
          md5="$val"
          ;;
        sha1)
          sha1="$val"
          ;;
      esac
    else
      # Between paragraphs. Time to check what we've found so far.
      end_lines="$end_lines $line_num"
      count=$(( count + 1))

      if [ -z "$display_name" ]; then
        DEBUG "image $count is missing display_name"
        errors=yes
      fi
      if [ -z "$file" ]; then
        DEBUG "image $count is missing file"
        errors=yes
      fi
      if [ -z "$size" ]; then
        DEBUG "image $count is missing size"
        errors=yes
      fi
      if [ -z "$url" ]; then
        DEBUG "image $count is missing url"
        errors=yes
      fi
      if [ "$CHECK" = "md5sum" ] && [ -z "$md5" ]; then
        DEBUG "image $count is missing required md5"
        errors=yes
      fi
      if [ "$CHECK" = "sha1sum" ] && [ -z "$sha1" ]; then
        DEBUG "image $count is missing required sha1"
        errors=yes
      fi

      # Prepare for next stanza
      display_name=
      file=
      size=
      url=
      md5=
      sha1=
      skipping=yes
    fi
  done < "$config"

  DEBUG "$count images found"
  num_images="$count"

  DEBUG "start_lines=($start_lines)"
  DEBUG "end_lines=($end_lines)"

  # return error status
  [ "$count" != "0" ] && [ -z "$errors" ]
}


# Make the user pick an image to download. On success, it sets the global
# variable 'user_choice' to the selected image number.
choose_image() {
  local show
  local count
  local line
  local num

  show=yes
  while true; do
    if [ -n "$show" ]; then
      echo
      echo "There are $num_images recovery images to choose from:"
      echo
      count=0
      echo "0 - <quit>"
      grep '^display_name=' "$config" | while read line; do
        count=$(( count + 1 ))
        echo "$line" | sed "s/display_name=/$count - /"
      done
      echo
      show=
    fi
    echo -n "Please select a recovery image to download: "
    read num
    if [ -z "$num" ] || [ "$num" = "?" ]; then
      show=yes
    elif echo "$num" | grep -q '[^0-9]'; then
      echo "Sorry, I didn't understand that."
    else
      if [ "$num" -lt "0" ] || [ "$num" -gt "$num_images" ]; then
        echo "That's not one of the choices."
      elif [ "$num" -eq 0 ]; then
        quit
      else
        break;
      fi
    fi
  done
  echo

  # global
  user_choice="$num"
}

# Fetch and verify the user's chosen image. On success, it sets the global
# variable 'image_file' to indicate the local name of the unpacked binary that
# should be written to the USB drive.
fetch_image() {
  local start
  local end
  local line
  local key
  local val
  local file
  local size
  local url
  local md5
  local sha1
  local line_num
  local tarball
  local err
  local sum

  file=
  size=
  url=
  md5=
  sha1=
  line_num="0"

  # Convert image number to line numbers within config file.
  start=$(echo $start_lines | cut -d' ' -f$1)
  end=$(echo $end_lines | cut -d' ' -f$1)

  while read line; do
    # Skip to the start of the desired stanza
    line_num=$(( line_num + 1 ))
    if [ "$line_num" -lt "$start" ] || [ "$line_num" -ge "$end" ]; then
      continue;
    fi

    # Process the stanza.
    if [ -n "$line" ]; then
      key=${line%=*}
      val=${line#*=}
      if [ -z "$key" ] || [ -z "$val" ] || [ "$key=$val" != "$line" ]; then
        DEBUG "ignoring $line"
        continue
      fi

      case $key in
        # The descriptive stuff we'll just save for later.
        file)
          file="$val"
          ;;
        size)
          size="$val"
          ;;
        md5)
          md5="$val"
          ;;
        sha1)
          sha1="$val"
          ;;
        url)
          # Try to download each url until one works.
          if [ -n "$url" ]; then
            # We've already got one (it's very nice).
            continue;
          fi
          warn "Downloading image tarball from $val"
          warn
          tarball=${val##*/}
          if $FETCHCONT "$tarball" "$val"; then
            # Got it.
            url="$val"
          else
            # If you give curl the '-C -' option but the file you want is
            # already complete and the server doesn't report the total size
            # correctly, it will report an error instead of just doing nothing.
            # We'll try to work around that.
            err=$?
            if [ "$FETCH" = "curl" ] && [ "$err" = "18" ]; then
              warn "Ignoring spurious complaint"
              url="$val"
            fi
          fi
          ;;
      esac
    fi
  done < "$config"

  if [ -z "$url" ]; then
    DEBUG "couldn't fetch tarball"
    return 1
  fi

  # Verify the tarball
  if ! ls -l "$tarball" | grep -q "$size"; then
    DEBUG "size is wrong"
    return 1
  fi
  sum=$($CHECK "$tarball" | cut -d' ' -f1)
  DEBUG "$CHECK is $sum"
  if [ "$CHECK" = "md5sum" ] && [ "$sum" != "$md5" ]; then
    DEBUG "wrong $CHECK"
    return 1
  elif [ "$CHECK" = "sha1sum" ] && [ "$sum" != "$sha1" ]; then
    DEBUG "wrong $CHECK"
    return 1
  fi

  # Unpack the file
  warn "Unpacking the tarball"
  rm -f "$file"
  if ! unzip "$tarball" "$file"; then
    DEBUG "Can't unpack the tarball"
    return 1
  fi

  # global
  image_file="$file"
}

##############################################################################
# Helper functions to manage USB drives.

# Return a list of base device names ("sda sdb ...") for all USB drives
get_devlist() {
  local dev
  local t
  local r

  for dev in $(cat /proc/partitions); do
    [ -r "/sys/block/$dev/device/type" ] &&
    t=$(cat "/sys/block/$dev/device/type") &&
    [ "$t" = "0" ] &&
    r=$(cat "/sys/block/$dev/removable") &&
    [ "$r" = "1" ] &&
    readlink -f "/sys/block/$dev" | grep -q -i usb &&
    echo "$dev" || true
  done
}

# Return descriptions for each provided base device name ("sda sdb ...")
get_devinfo() {
  local dev
  local v
  local m
  local s
  local ss

  for dev in $1; do
    v=$(cat "/sys/block/$dev/device/vendor") &&
    m=$(cat "/sys/block/$dev/device/model") &&
    s=$(cat "/sys/block/$dev/size") && ss=$(( $s * 512 / 1000000 )) &&
    echo "/dev/$dev ${ss}MB $v $m"
  done
}

# Enumerate and descript the specified base device names ("sda sdb ...")
get_choices() {
  local dev
  local desc
  local count

  count=1
  echo "0 - <quit>"
  for dev in $1; do
    desc=$(get_devinfo "$dev")
    echo "$count - Use $desc"
    count=$(( count + 1 ))
  done
}

# Make the user pick a USB drive to write to. On success, it sets the global
# variable 'user_choice' to the selected device name ("sda", "sdb", etc.)
choose_drive() {
  local show
  local devlist
  local choices
  local num_drives
  local msg
  local num

  show=yes
  while true; do
    if [ -n "$show" ]; then
      devlist=$(get_devlist)
      choices=$(get_choices "$devlist")
      if [ -z "$devlist" ]; then
        num_drives="0"
        msg="I can't seem to find a valid USB drive."
      else
        num_drives=$(echo "$devlist" | wc -l)
        if [ "$num_drives" != "1" ]; then
          msg="I found $num_drives USB drives"
        else
          msg="I found $num_drives USB drive"
        fi
      fi
      echo -n "

$msg

$choices

"
      show=
    fi
    echo -n "Tell me what to do (or just press Enter to scan again): "
    read num
    if [ -z "$num" ] || [ "$num" = "?" ]; then
      show=yes
    elif echo "$num" | grep -q '[^0-9]'; then
      echo "Sorry, I didn't understand that."
    else
      if [ "$num" -lt "0" ] || [ "$num" -gt "$num_drives" ]; then
        echo "That's not one of the choices."
      elif [ "$num" -eq 0 ]; then
        quit
      else
        break;
      fi
    fi
  done

  # global
  user_choice=$(echo $devlist | cut -d' ' -f$num)
}


##############################################################################
# Okay, do something...

# Make sure we have the tools we need
require_utils

# Need a place to work. We prefer a fixed location so we can try to resume any
# interrupted downloads.
if [ -n "$WORKDIR" ]; then
  if [ ! -d "$WORKDIR" ] && ! mkdir "$WORKDIR" ; then
    warn "Using temporary directory"
    WORKDIR=
  fi
fi
if [ -z "$WORKDIR" ]; then
  WORKDIR=$(mktemp -d)
  # Clean up temporary directory afterwards
  trap "cd; rm -rf ${WORKDIR}" EXIT
fi

cd "$WORKDIR"
warn "Working in $WORKDIR/"
rm -f "$debug"

# Download the config file to see what choices we have.
warn "Downloading config file from $CONFIGURL"
$FETCHNEW "$tmpfile" "$CONFIGURL" || \
  gfatal "Unable to download the config file"

# Un-DOS-ify the config file and separate the version info from the images
sed 's/\r//g' "$tmpfile" | grep '^recovery_tool' > "$version"
sed 's/\r//g' "$tmpfile" | grep -v '^#' | grep -v '^recovery_tool' > "$config"
# Add one empty line to the config file to terminate the last stanza
echo >> "$config"

# Make sure that the config file version matches this script version
tmp=$(grep '^recovery_tool_version=' "$version") || \
  gfatal "The config file doesn't contain a version string."
filevers=${tmp#*=}
if [ "$filevers" != "$MYVERSION" ]; then
  tmp=$(grep '^recovery_tool_update=' "$version");
  msg=${tmp#*=}
  warn "This tool is version $MYVERSION." \
    "The config file is for version $filevers."
  fatal ${msg:-Please download a matching version of the tool and try again.}
fi

# Check the config file to be sure it's valid. As a side-effect, this sets the
# global variable 'num_images' with the number of image stanzas read, but
# that's independent of whether the config is valid.
good_config || gfatal "The config file isn't valid."

# Make the user pick an image to download, or exit.
choose_image

# Download the user's choice
fetch_image "$user_choice" || \
  gfatal "Unable to download a valid recovery image."

# Make the user pick a USB drive, or exit.
choose_drive

# Be sure
dev_desc=$(get_devinfo "$user_choice")
echo "
Is this the device you want to put the recovery image on?

  $dev_desc
"
echo -n "You must enter 'YES' to continue: "
read tmp
if [ "$tmp" != "YES" ]; then
  quit
fi

# Be very sure
echo "

I'm really going to erase this device. This will permanently ERASE
whatever you may have on that drive. You won't be able to undo it.

  $dev_desc
"

echo -n "If you're sure that's the device to use, enter 'DoIt' now:  "
read tmp
if [ "$tmp" != "DoIt" ]; then
  quit
fi
echo "

Installing the recovery image

"

# Unmount anything on that device.
echo "unmounting..."
for tmp in $(mount | grep ^"/dev/${user_choice}" | sed 's/[ \t].*//'); do
  umount $tmp || ufatal "Unable to unmount $tmp."
done

# Write it.
echo "copying... (this may take several minutes)"
dd of=/dev/${user_choice} if="$image_file" ||
  ufatal "Unable to write the image."
sync

echo "

Done. Remove the USB drive and insert it in your Chrome OS netbook.

"

exit 0
