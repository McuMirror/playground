#-----------------------------------------------------------------------------
# File Name: makefile
# Description
# -----------
#
#-----------------------------------------------------------------------------
SHELL 		= /bin/sh
EXECUTABLES	= mem

#-- Release Compiler Flags ---------------------------------------------------

CFLAGS		+= -D__linux__ -O2 -g0 -Wall -pedantic -Wstrict-prototypes -std=c99
CFLAGS		+= -D__linux__ -pipe -fomit-frame-pointer -D_GNU_SOURCE

CC = gcc-7

#-- Required libraries -------------------------------------------------------

LIBS		 = -lm -laes_access -lstdc++ 

#-- Dependencies -------------------------------------------------------------

OBJS = mem.o 

#-- Compile All --------------------------------------------------------------

.PHONEY: all
all: 	$(EXECUTABLES)

#-- Clean Up -----------------------------------------------------------------

.PHONEY: clean
clean:

	rm -f *.[bo]
	rm -f *.err
	rm -f $(EXECUTABLES)

#-- Build targets ------------------------------------------------------------

mem: $(OBJS)
		$(CC) $(OBJS) $(CFLAGS) $(LIBS) -o $@

#------------------------------ END OF FILE ----------------------------------

