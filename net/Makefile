# fiwix/net/Makefile
#
# Copyright 2023, Jordi Sanfeliu. All rights reserved.
# Distributed under the terms of the Fiwix License.
#

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

OBJS = domains.o socket.o packet.o core.o unix.o ipv4.o
DIRS = lwip

all:	$(OBJS)
	tar zxf $(LWIP_STABLE_RELEASE)
	@for n in $(DIRS) ; do (cd $$n ; $(MAKE)) ; done

clean:
	@for n in $(DIRS) ; do (cd $$n ; $(MAKE) clean) ; done
	rm -f *.o
	rm -rf lwip-STABLE*

