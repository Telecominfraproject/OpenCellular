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

By default most debug messages won't be compiled into the binary. To
include them into the build, set `DEBUG=1` on the command line or in
your `.config`.

Let's install *libhwbase*. We'll need `configs/linux` to build regular
Linux executables:

    $ cd libhwbase
    $ make DEBUG=1 cnf=configs/linux install

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
    $ make DEBUG=1 cnf=configs/sandybridge gfx_test

The resulting binary is `build/gfx_test`.


Testing libgfxinit on Linux
===========================

`gfx_test` sets up its own framebuffer in the *stolen memory*. It
backs any current framebuffer mapping and contents up first and re-
stores it before exiting. This works somehow even while the *i915*
driver is running. A wrapper script `gfxtest/gfx_test.sh` is pro-
vided to help with the setup. It switches to a text console first
and tries to unload the *i915* driver. But ignores failures to do
so (it won't work if you still have any application running that
uses the gfx driver, e.g. an X server).

    # gfxtest/gfx_test.sh

If you chose the right config above, you should be presented with a
nice test image. But please be prepared that your console might be
stuck in that state afterwards. You can try to run it with *i915*
deactivated then (e.g. when booting with `nomodeset` in the kernel
command line or with *i915* blacklisted) and loading it afterwards.
