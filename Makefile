CC=msp430-gcc

# regular
CFLAGS=-Wall -mmcu=msp430g2553 -std=gnu99 -O3 -g0

# for debugging
# CFLAGS=-Wall -mmcu=msp430g2553 -std=gnu99 -g3 -ggdb -gdwarf-2

LDFLAGS=-mmcu=msp430g2553

mbus-frontend.elf:	main.o
	$(CC) -o $@ $(LDFLAGS) $^

.c.o:	
	$(CC) $(CFLAGS) -c $<


.PHONY: all
all:	mbus-frontend.elf

.PHONY: clean
clean:
	-rm -f *.o *.elf

.PHONY: upload
upload: mbus-frontend.elf
	mspdebug rf2500 "prog mbus-frontend.elf"

.PHONY: debug
debug: upload
	mspdebug rf2500 gdb &
	ddd --debugger "msp430-gdb -x blinky1.gdb"




