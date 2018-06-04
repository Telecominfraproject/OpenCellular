Introduction
============

This project provides a tool which compiles BCT (Boot Configuration Table)
images to place into the boot flash of a Tegra-based device.

The tool will either:

a) Compile a textual representation of a BCT into a binary image.

b) Generate an entire boot image from a previously compiled BCT and a
   bootloader binary.

Submitting Changes
==================

To submit patches to this project, please use the following commands:

* git format-patch --subject-prefix="cbootimage PATCH"

  Creates a patch file from your git commit.

* git send-email --to linux-tegra@vger.kernel.org *.patch

  Sends the patch by email to the Tegra mailing list.

Even though the primary upstream repository for this project is hosted on
github, contributions aren't accepted via github pull requests. Github pull
requests would bypass public code review on the project mailing list.

Patches should be signed off (include a signed-off-by line) to indicate your
acceptance of the code's license (see COPYING and the license header in each
file). See http://developercertificate.org/ for details of what signed-off-by
implies.
