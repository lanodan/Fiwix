LWIPDIR = .
include Filelists.mk

.c.o:
	$(CC) $(CFLAGS) -Iinclude -c -o $@ $<

OBJS = $(LWIPNOAPPSFILES:.c=.o)

liblwip.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

all: liblwip.a

clean:
	rm -f $(OBJS) liblwip.a

