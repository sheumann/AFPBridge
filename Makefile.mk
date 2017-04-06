# Use stock ORCA libraries & headers, not GNO ones
USEORCALIBS = prefix 13 /lang/orca/Libraries
COMMAND = $(!eq,$(CMNDNAME),$(CC) $(CMNDNAME) $(USEORCALIBS)&&$(CC)) $(CMNDARGS)

CFLAGS = -i -w -O95

DSITEST_OBJS = dsitest.o aspinterface.o dsi.o readtcp.o endian.o tcpconnection.o atipmapping.o asmglue.o cmdproc.o
DSITEST_PROG = dsitest

AFPMOUNTER_OBJS = afpmounter.o callat.o endian.o
AFPMOUNTER_PROG = afpmounter

DUMPCMDTBL_OBJS = dumpcmdtbl.o asmglue.o
DUMPCMDTBL_PROG = dumpcmdtbl

AFPBRIDGE_OBJS = afpinit.o afpbridge.o aspinterface.o dsi.o readtcp.o endian.o tcpconnection.o atipmapping.o asmglue.o installcmds.o cmdproc.o callat.o
AFPBRIDGE_PROG = AFPBridge

PROGS = $(DSITEST_PROG) $(AFPMOUNTER_PROG) $(DUMPCMDTBL_PROG) $(AFPBRIDGE_PROG)

.PHONY: default
default: $(PROGS)

$(DSITEST_PROG): $(DSITEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $(DSITEST_OBJS)

$(AFPMOUNTER_PROG): $(AFPMOUNTER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(AFPMOUNTER_OBJS)

$(DUMPCMDTBL_PROG): $(DUMPCMDTBL_OBJS)
	$(CC) $(CFLAGS) -o $@ $(DUMPCMDTBL_OBJS)

$(AFPBRIDGE_PROG): $(AFPBRIDGE_OBJS)
	$(CC) $(CFLAGS) -M -o $@ $(AFPBRIDGE_OBJS) > $@.map
	chtyp -tpif $@

%.macros: %.asm
	macgen $< $@ /lang/orca/Libraries/ORCAInclude/m16.*

.PHONY: install
install: $(AFPBRIDGE_PROG)
	cp $(AFPBRIDGE_PROG) "*/System/System.Setup"

.PHONY: import
import:
	chtyp -ttxt *.mk
	chtyp -lcc *.c *.h
	chtyp -lasm *.asm *.macros
	udl -g *

.PHONY: clean
clean:
	$(RM) $(PROGS) *.o *.root *.map > .null
