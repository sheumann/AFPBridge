CFLAGS = -i -w

OBJS = dsitest.o aspinterface.o dsi.o readtcp.o endian.o tcpconnection.o atipmapping.o
PROG = dsitest

$(PROG): $(OBJS)
	occ $(CFLAGS) -o $@ $(OBJS)

.PHONY: clean
clean:
	$(RM) $(OBJS) $(PROG)
