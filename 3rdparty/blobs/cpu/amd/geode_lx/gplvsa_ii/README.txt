This is the VSA (Virtual Systems Architecture) code used on the AMD
Geode series of processors. The Geode, rather than carrying lots of
legacy hardware interfaces that are presumed to exist on x86 systems
that might be painful to implement on a highly integrated, low power
processor, the Geode often emulates such interfaces by use of software that is
invoked by special traps that take place when the processor accesses
these devices.

Note that the code here is not currently buildable on open source
systems, being only buildable using very obsolete and no longer
commercially availble Windows based commercial toolchains.  On the
OLPC system, these "blobs" of binary code are concatenated together
with LinuxBIOS and the bootloader, and set up to be executed by
LinuxBIOS early in the Geode's initialization sequence (no linking is
involved).

If you are interested for some reason in making this code buildable on
free systems, please let us know of your progress.  It is under the 
GNU LGPL. 

Also note that VESA emulation is *not* included in this (nor does what
we use on the OLPC machine use VESA at this date; we use frame
buffer code for our console); that code was not owned by AMD and
therefore not theirs to make available.  Our thanks to AMD to making
the VSA code available.

		     Jim Gettys, OLPC, September 27, 2006
