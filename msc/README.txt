This directory contains the Microsoft Visual C makefile for building vboot
reference code (and testing, eventually) in a 32 bit DOS window.

Microsoft Visual C 2008 (or later) is the prerequisite for this to work.

To build vboot_reference tree in the DOS window do the following:

- untar or git clone the vboot_reference source tree

- open a DOS window

- run the MSVC provided script vcvars32.bat to create the command line build
  environment. Script location is MSVC installation specific. For instance:

  c:\> \bios\devtls\MSVC9\Vc\bin\vcvars32.bat

- define a directory where the nmake output should go into

  c:\> set MOD=z:\shared\tmp

- start the make job as follows:

 c:\> nmake /f %path_to_vboot_reference_tree%\msc\nmakefile

- observe the output generated in %MOD%
