PROG= apple1
SRCS= src/main.c src/pia.c v6502/v6502/log.c v6502/v6502/debugger.c v6502/v6502/breakpoint.c
OBJS= $(SRCS:.c=.o)

V6502_PREFIX= v6502
include v6502/libvars.mk

AS=     $(V6502_PREFIX)/as6502/as6502
ROM=    apple1.rom
ROMSRC= src/wozmon.s

CFLAGS+= -I$(V6502_PREFIX) -std=c99 -D_XOPEN_SOURCE -D_XOPEN_SOURCE_EXTENDED -D_GNU_SOURCE
LDFLAGS+= -ledit -lcurses -ldis6502 -las6502 -lv6502

all: $(PROG) $(ROM)

$(ROM): $(ROMSRC) $(AS)
	$(AS) -o $(ROM) $(ROMSRC)

$(PROG): $(LIBV6502) $(LIBAS6502) $(LIBDIS6502) $(OBJS)
	$(CC) $(OBJS) -o $(PROG) $(LDFLAGS)

$(AS):
	$(MAKE) -C $(V6502_PREFIX)/as6502

clean:
	rm -f $(PROG) $(ROM) $(OBJS)
	make -C $(V6502_PREFIX) clean

include v6502/libtargets.mk
