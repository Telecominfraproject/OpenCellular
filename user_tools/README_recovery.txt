
The recovery tool assists the user in creating a bootable USB drive that can
recover a non-functional Chromium OS device. It generally operates in three
steps.

1. Download a config file from a known URL. This file describes the
   available images and where they can be found.

2. Ask the user to select the appropriate image, download it, and verify
   that it matches what the config file describes.

3. Ask the user to select a USB drive, and write the recovery image to it.




Here's the format of the config file:

The config file is a machine-generated, human-readable text file containing
at least two paragraphs or stanzas, which are separated by at least one
blank line.

* Each line in the file ends with "\n" (LF), not "\r\n" (CRLF).

* The file shall not contain any tab characters.

* Lines beginning with '#' are completely ignored and do not count as blank
  lines.

* Non-blank lines must consist of a non-blank key and non-blank value
  separated by a '=' with no spaces on either side. The key may not contain
  whitespace. The value may contain spaces, but all trailing whitespace is
  discarded.

* The first stanza must contain a key named "recovery_tool_version'. Its
  value must match the version of the recovery tool. If the value does not
  match, then the key 'recovery_tool_update', if it exists in the first
  stanza, should contain a string to display to the user. Regardless, if the
  version doesn't match, the recovery tool should exit.

* The first stanza MAY have platform-specific keys such as
  'recovery_tool_linux_version'. If present, this key is used instead of the
  'recovery_tool_version' key to determine the required recovery tool
  version.

* The current recovery_tool_version value is 0.9.1

* The archive shall be in ZIP format.


The second and remaining stanzas describe recovery images to put on the USB
drive. Each image stanza must contain, in this order:

* One and only one of each of these keys:

   name           - string to show to the user
   zipfilesize    - size in bytes of the zipfile
   file           - the name of the file to extract from the zipfile
   filesize       - size in bytes of the extracted, uncompressed file
   channel        - the channel for this image ("beta-channel",
                    "dev-channel", "pilot-channel", etc.)

* Zero or more of these keys:

   hwid           - the HWID of the device which can use this image (no
                    entry means "any device")

* One or both of these keys:

   md5            - md5sum of the zipfile
   sha1           - sha1sum of the zipfile

* One or more of these keys:

   url            - where to find the zipfile to download


Any other keys are informational only and are not used by the recovery tool.

If more than one url is provided, then they should be tried, in turn, until
one succeeds.

At a minimum, the user shall be shown the name, channel, and all hwid values
for each image.
