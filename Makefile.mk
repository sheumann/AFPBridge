# Use stock ORCA libraries & headers, not GNO ones
USEORCALIBS = prefix 13 /lang/orca/Libraries
COMMAND = $(!eq,$(CMNDNAME),$(CC) $(CMNDNAME) $(USEORCALIBS)&&$(CC)) $(CMNDARGS)

CFLAGS = -i -w -O95

DSITEST_OBJS = dsitest.o aspinterface.o dsi.o readtcp.o endian.o tcpconnection.o atipmapping.o asmglue.o cmdproc.o installcmds.o afpoptions.o strncasecmp.o savenames.o
DSITEST_PROG = dsitest

LISTSESSIONS_OBJS = listsess.o callat.o
LISTSESSIONS_PROG = listsessions

MOUNTAFP_OBJS = afpmounter.o callat.o endian.o
MOUNTAFP_PROG = mountafp

DUMPCMDTBL_OBJS = dumpcmdtbl.o asmglue.o
DUMPCMDTBL_PROG = dumpcmdtbl

AFPBRIDGE_OBJS = afpinit.o afpbridge.o aspinterface.o dsi.o readtcp.o endian.o tcpconnection.o atipmapping.o asmglue.o installcmds.o cmdproc.o callat.o afpoptions.o strncasecmp.o savenames.o
AFPBRIDGE_RSRC = afpbridge.rez
AFPBRIDGE_PROG = AFPBridge

AFPMOUNTER_OBJS = cdevstart.o afpcdev.o afpurlparser.o afpoptions.o strncasecmp.o
AFPMOUNTER_RSRC = afpcdev.rez
AFPMOUNTER_CDEV = AFPMounter

PROGS = $(DSITEST_PROG) $(MOUNTAFP_PROG) $(DUMPCMDTBL_PROG) $(AFPBRIDGE_PROG) $(AFPMOUNTER_CDEV)

.PHONY: default
default: $(PROGS)

$(DSITEST_PROG): $(DSITEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $<

$(MOUNTAFP_PROG): $(MOUNTAFP_OBJS)
	$(CC) $(CFLAGS) -o $@ $<

$(DUMPCMDTBL_PROG): $(DUMPCMDTBL_OBJS)
	$(CC) $(CFLAGS) -o $@ $<

$(LISTSESSIONS_PROG): $(LISTSESSIONS_OBJS)
	$(CC) $(CFLAGS) -o $@ $<

$(AFPBRIDGE_PROG): $(AFPBRIDGE_OBJS) $(AFPBRIDGE_RSRC)
	$(CC) $(CFLAGS) -M -o $@ $(AFPBRIDGE_OBJS) > $@.map
	$(REZ) $(AFPBRIDGE_RSRC) -o $@
	chtyp -tpif $@

$(AFPMOUNTER_CDEV).obj: $(AFPMOUNTER_OBJS)
	$(CC) $(CFLAGS) -o $@ $<

$(AFPMOUNTER_CDEV): $(AFPMOUNTER_CDEV).obj $(AFPMOUNTER_RSRC)
	$(REZ) $(AFPMOUNTER_RSRC) -o $@
	chtyp -tcdv $@

%.macros: %.asm
	macgen $< $@ /lang/orca/Libraries/ORCAInclude/m16.*

.PHONY: install
install: $(AFPBRIDGE_PROG) $(AFPMOUNTER_CDEV)
	cp $(AFPBRIDGE_PROG) "*/System/System.Setup"
	cp $(AFPMOUNTER_CDEV) "*/System/CDevs"
	$(RM) "*/System/CDevs/CDev.Data" > .null

.PHONY: import
import:
	chtyp -ttxt *.mk
	chtyp -lcc *.c *.h
	chtyp -lasm *.asm *.macros
	chtyp -lrez *.rez
	udl -g *

.PHONY: clean
clean:
	$(RM) $(PROGS) *.o *.root *.obj *.map > .null
