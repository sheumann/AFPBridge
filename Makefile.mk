CFLAGS = -i -w

DSITEST_OBJS = dsitest.o aspinterface.o dsi.o readtcp.o endian.o tcpconnection.o atipmapping.o
DSITEST_PROG = dsitest

AFPMOUNTER_OBJS = afpmounter.o callat.o endian.o
AFPMOUNTER_PROG = afpmounter

DUMPCMDTBL_OBJS = dumpcmdtbl.o asmglue.o
DUMPCMDTBL_PROG = dumpcmdtbl

PROGS = $(DSITEST_PROG) $(AFPMOUNTER_PROG) $(DUMPCMDTBL_PROG)

.PHONY: $(PROGS)
default: $(PROGS)

$(DSITEST_PROG): $(DSITEST_OBJS)
	occ $(CFLAGS) -o $@ $(DSITEST_OBJS)

$(AFPMOUNTER_PROG): $(AFPMOUNTER_OBJS)
	occ $(CFLAGS) -o $@ $(AFPMOUNTER_OBJS)

$(DUMPCMDTBL_PROG): $(DUMPCMDTBL_OBJS)
	occ $(CFLAGS) -o $@ $(DUMPCMDTBL_OBJS)

%.macros: %.asm
	macgen $< $@ /lang/orca/Libraries/ORCAInclude/m16.*

.PHONY: clean
clean:
	$(RM) $(PROGS) *.o *.root > .null
