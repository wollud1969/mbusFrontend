Build:
* make all

Flash:
* make upload

Debugger:
* enable debugging in Makefile
* mspdebug rf2500 gdb
* ddd --debugger msp430-gdb
* for every code change upload using Makefile and restart both mspdebug and ddd (or find a way to reset the both)


