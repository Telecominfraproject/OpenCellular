**coreboot binary policy v1.0**


While coreboot attempts to be binary free, some coreboot mainboards require
vendor binaries to support silicon and features. It is an unfortunate fact,
as silicon has become more complicated, vendors are using more binaries to
support their silicon. The coreboot community can not control the vendors,
nor completely eliminate binaries, but it can set standards and expectations
for vendor participation. coreboot needs policies and guidelines to meet GPL
licence requirements and to organize and maintain standards within coreboot.


To accept binaries in coreboot 3rdparty/blobs repository, **the binary
must meet the following**:


1. A publicly available (published) ABI
   a. In case of non-ISA binary, documented usage conventions are required
   b. examples:
      * The Intel® Firmware Support Package: External Architecture
      Specification v.1.0
      * The PCI firmware specification is the ABI for a standard PCI video BIOS.
      * Vendor microcode loading and placement instructions


2. Appropriate license (redistributable)
   The binary must be accompanied by a distribution license. The license
   must allow unlimited redistribution to allow coreboot contributors to
   create coreboot images for third parties which contain this and other blobs.


3. Linking
   Source code linked into coreboot may not be committed to the binary
   repository. Such source code and header files must be committed to the
   coreboot repository instead.


4. Binary version
   The binary must contain the version and how to extract the version must
   be published in the ABI


5. Release notes - updated with each version
   Each binary release must be accompanied by a release note that contains
   all of the following (if a field is unknown or unavailable, mark it as
   unknown or N/A):
      * version
      * release date
      * supported silicon
      * instructions, requirements, and dependencies
      * changes since the last version
      * errata, known issues
      * toolchain version(s), if applicable
      * ABI version and link to the published ABI (in the binary repository)


6. Good commit message
   The commit message should summarize the release note and contain
   any additional information that might be specific to coreboot. It is
   helpful to indicate how the binary was tested within coreboot and list
   any known exceptions or errata.
