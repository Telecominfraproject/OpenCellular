libgfxinit
==========

*libgfxinit* is a graphics initialization (aka modesetting) library
for embedded environments. It currently supports only Intel hardware,
more specifically the Intel Core processor line.

It can query and set up most kinds of displays based on their EDID
information. You can, however, also specify particular mode lines.

*libgfxinit* is written in SPARK, an Ada subset with formal verifica-
tion aspects. Absence of runtime errors can be proved automatically
with SPARK GPL 2016.


Building on Linux
=================

Prerequisites
-------------

For compilation, the GNAT Ada compiler is required. Usual package
names in Linux distributions are `gcc-ada` and `gnat`.

Grab the Sources
----------------

You'll need *libhwbase* and *libgfxinit*. Best is to clone the reposi-
tories into a common parent directory (this way *libgfxinit* will know
where to find *libhwbase*).

    $ mkdir gfxfun && cd gfxfun
    $ git clone https://review.coreboot.org/p/libhwbase.git
    $ git clone https://review.coreboot.org/p/libgfxinit.git

Configure and Install libhwbase
-------------------------------

Both libraries are currently configured by hand-written config files.
You can either write your own `.config`, link one of the shipped files
in `configs/`, e.g.:

    $ ln -s configs/linux libhwbase/.config

or overwrite the config filename by specifying `cnf=<configfile>` on
the make command line.

Let's install *libhwbase*. We'll need `configs/linux` to build regular
Linux executables:

    $ cd libhwbase
    $ make cnf=configs/linux install

By default this installs into a new subdirectory `dest`. You can however
overwrite this decision by specifying `DESTDIR=`.

Build libgfxinit/`gfx_test`
---------------------------

*libgfxinit* is configured and installed in the same manner as de-
scribed above. You will have to select a configuration matching your
hardware.

The makefile knows an additional target `gfx_test` to build a small
Linux test application:

    $ cd ../libgfxinit
    $ make cnf=configs/sandybridge gfx_test

The resulting binary is `build/gfx_test`.


Testing libgfxinit on Linux
===========================

In its current state `gfx_test` doesn't know how to set up a frame-
buffer. It just assumes that enough memory is mapped. This is known
to work well, after running the VBIOS but before the Linux driver
*i915* took over (e.g. when booting with `nomodeset` in the kernel
command line or with *i915* blacklisted). After running *i915* it
only works by chance.

When running `gfx_test` (as root), it will access the graphics hard-
ware through the sysfs PCI interface. The path is

    /sys/devices/pci0000:00/0000:00:02.0/

for all supported platforms.

If you chose the right config above, you should be presented with a
nice test image. However, `gfx_test` is one-way only: The graphics
hardware will stay in this state, until another driver takes over.
