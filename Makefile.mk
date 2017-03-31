CFLAGS = -i -w

DSITEST_OBJS = dsitest.o aspinterface.o dsi.o readtcp.o endian.o tcpconnection.o atipmapping.o asmglue.o
DSITEST_PROG = dsitest

AFPMOUNTER_OBJS = afpmounter.o callat.o endian.o
AFPMOUNTER_PROG = afpmounter

DUMPCMDTBL_OBJS = dumpcmdtbl.o asmglue.o
DUMPCMDTBL_PROG = dumpcmdtbl

AFPBRIDGE_OBJS = afpbridge.o aspinterface.o dsi.o readtcp.o endian.o tcpconnection.o atipmapping.o asmglue.o installcmds.o cmdproc.o callat.o
AFPBRIDGE_PROG = afpbridge

PROGS = $(DSITEST_PROG) $(AFPMOUNTER_PROG) $(DUMPCMDTBL_PROG) $(AFPBRIDGE_PROG)

.PHONY: $(PROGS)
default: $(PROGS)

$(DSITEST_PROG): $(DSITEST_OBJS)
	occ $(CFLAGS) -o $@ $(DSITEST_OBJS)

$(AFPMOUNTER_PROG): $(AFPMOUNTER_OBJS)
	occ $(CFLAGS) -o $@ $(AFPMOUNTER_OBJS)

$(DUMPCMDTBL_PROG): $(DUMPCMDTBL_OBJS)
	occ $(CFLAGS) -o $@ $(DUMPCMDTBL_OBJS)

$(AFPBRIDGE_PROG): $(AFPBRIDGE_OBJS)
	occ $(CFLAGS) -o $@ $(AFPBRIDGE_OBJS)

%.macros: %.asm
	macgen $< $@ /lang/orca/Libraries/ORCAInclude/m16.*

.PHONY: clean
clean:
	$(RM) $(PROGS) *.o *.root > .null
