PROG=		a1
SRCS=		apple1/main.c apple1/pia.c v6502/v6502/log.c

V6502_PREFIX= v6502
LIBV6502_DIR=	$(V6502_PREFIX)/v6502
LIBV6502=	$(LIBV6502_DIR)/libv6502.a
LIBAS6502_DIR= $(V6502_PREFIX)/as6502
LIBAS6502=   $(LIBAS6502_DIR)/libas6502.a
LIBDIS6502_DIR= $(V6502_PREFIX)/dis6502
LIBDIS6502=   $(LIBDIS6502_DIR)/libdis6502.a

CFLAGS+=	-I$(V6502_PREFIX) -std=c99
LDFLAGS+=	-lcurses -ldis6502 -las6502 -lv6502 -L$(LIBV6502_DIR) -L$(LIBAS6502_DIR) -L $(LIBDIS6502_DIR)
OBJS=		$(SRCS:.c=.o)

all: $(PROG)

$(PROG): $(LIBV6502) $(LIBAS6502) $(LIBDIS6502) $(OBJS)
	$(CC) $(OBJS) -o $(PROG) $(LDFLAGS)

$(LIBV6502):
	$(MAKE) -C $(LIBV6502_DIR) lib

$(LIBAS6502):
	$(MAKE) -C $(LIBAS6502_DIR) lib

$(LIBDIS6502):
	$(MAKE) -C $(LIBDIS6502_DIR) lib

cleanlib:
	rm -f $(LIBV6502) $(LIBV6502_OBJS)

clean: cleanlib
	rm -f $(PROG) $(OBJS)

