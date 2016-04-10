PROG=		apple1
SRCS=		src/main.c src/pia.c v6502/v6502/log.c v6502/v6502/debugger.c v6502/v6502/breakpoint.c

V6502_PREFIX= v6502
LIBV6502_DIR=	$(V6502_PREFIX)/v6502
LIBV6502=	$(LIBV6502_DIR)/libv6502.a
LIBAS6502_DIR= $(V6502_PREFIX)/as6502
LIBAS6502=   $(LIBAS6502_DIR)/libas6502.a
LIBDIS6502_DIR= $(V6502_PREFIX)/dis6502
LIBDIS6502=   $(LIBDIS6502_DIR)/libdis6502.a

AS=	$(LIBAS6502_DIR)/as6502
ROM= apple1.rom
ROMSRC= src/wozmon.s

CFLAGS+=	-I$(V6502_PREFIX) -std=c99
LDFLAGS+=	-ledit -lcurses -ldis6502 -las6502 -lv6502 -L$(LIBV6502_DIR) -L$(LIBAS6502_DIR) -L $(LIBDIS6502_DIR)
OBJS=		$(SRCS:.c=.o)

all: $(PROG) $(ROM)

$(ROM): $(ROMSRC) $(AS)
	$(AS) -o $(ROM) $(ROMSRC)

$(PROG): $(LIBV6502) $(LIBAS6502) $(LIBDIS6502) $(OBJS)
	$(CC) $(OBJS) -o $(PROG) $(LDFLAGS)

$(AS):
	$(MAKE) -C $(LIBAS6502_DIR)

$(LIBV6502):
	$(MAKE) -C $(LIBV6502_DIR) lib

$(LIBAS6502):
	$(MAKE) -C $(LIBAS6502_DIR) lib

$(LIBDIS6502):
	$(MAKE) -C $(LIBDIS6502_DIR) lib

cleanlib:
	$(MAKE) -C $(V6502_PREFIX) clean

clean: cleanlib
	rm -f $(PROG) $(ROM) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -o $@ -c $<

