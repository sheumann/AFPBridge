CFLAGS = -i -w

DSITEST_OBJS = dsitest.o aspinterface.o dsi.o readtcp.o endian.o tcpconnection.o atipmapping.o
DSITEST_PROG = dsitest

AFPMOUNTER_OBJS = afpmounter.o callat.o endian.o
AFPMOUNTER_PROG = afpmounter

.PHONY: default
default: $(DSITEST_PROG) $(AFPMOUNTER_PROG)

$(DSITEST_PROG): $(DSITEST_OBJS)
	occ $(CFLAGS) -o $@ $(DSITEST_OBJS)

$(AFPMOUNTER_PROG): $(AFPMOUNTER_OBJS)
	occ $(CFLAGS) -o $@ $(AFPMOUNTER_OBJS)

endian.o: endian.asm
	occ $(CFLAGS) -c $<

callat.o: callat.asm
	occ $(CFLAGS) -c $<

.PHONY: clean
clean:
	$(RM) $(DSITEST_OBJS) $(DSITEST_PROG) $(AFPMOUNTER_PROG) $(AFPMOUNTER_OBJS) > .null
