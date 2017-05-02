Building AFPBridge
==================

AFPBridge is designed to be built under GNO 2.0.6, with ORCA/C and ORCA/M
installed under `/lang/orca` as described in the GNO documentation.
I am using a custom version of ORCA/C with several patches applied,
but I believe a stock version of ORCA/C 2.1.x should also work.

You also need to get the `AppleTalk.h` header file, which is included
under `Libraries/APWCInclude` in an ORCA/C installation.  Copy it either
to `/lang/orca/Libraries/ORCACDefs` or to the directory with the AFPBridge
source files.  The original version of that file does not include
prototypes in its function declarations, which will cause an ORCA/C error
with the settings in the makefile.  To avoid this, either remove the 
`-w` flag from `CFLAGS` in `Makefile.mk`, or add the prototypes in
`AppleTalk.h`.  If adding the prototypes, they should be `RamForbid(void)`,
`RamPermit(void)`, and `_CALLAT(void*)`.

To build AFPBridge using source files copied directly from the Git repository,
first run:

    make import

This sets the file types appropriately, converts files to Apple II-style
line endings, and generates required assembly-language macro files.

Once that is done, you can build the code by running:

    make

This builds the `AFPBridge` init, the `AFPMounter` CDev, and several
command-line utilities that can be useful for testing and debugging.

You can also run `make install` to install the init and CDev in your
system folder, or `make clean` to remove the generated files.
