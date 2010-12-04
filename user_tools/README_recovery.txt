
The recovery tool assists the user in creating a bootable USB drive that can
recover a non-functional Chromium OS device. It generally operates in three
steps.

1. Download a config file from a known URL. This file describes the
   available images and where they can be found.

2. Ask the user to select the appropriate image, download it, and verify
   that it matches what the config file describes.

3. Ask the user to select a USB drive, and write the recovery image to it.


Here's the format of the config file:

The config file is a text file containing at least two paragraphs or
stanzas, which are separated by at least one blank line. Lines beginning
with '#' are completely ignored and do not count as blank lines. Non-blank
lines must consist of a non-blank key and non-blank value separated by a '='
with no spaces on either side. The key may not contain whitespace. The value
may contain spaces, but all trailing whitespace is discarded.

The first stanza must contain a key named "recovery_tool_version'. Its value
must match the version of the recovery tool. If the value does not match,
then the key 'recovery_tool_update', if it exists in the first stanza,
should contain a string to display to the user. Regardless, if the version
doesn't match, the recovery tool should exit.

The second and remaining stanzas describe recovery images to put on the USB
drive.

For recovery_tool_version=1.0, each image stanza must contain:

* One and only one of these keys:

   display_name    - string to show to the user
   file            - the name of the file to extract from the tarball
   size            - size in bytes of the tarball

* One or more of these keys:

   url             - where to find the tarball to download

* One or both of these keys:

   md5             - md5sum of the tarball
   sha1            - sha1sum of the tarball

* Any other keys are informational only and are not used by the recovery tool.



NOTE: This is still in flux. Possible additional keys are

  hwid
  name
  channel
