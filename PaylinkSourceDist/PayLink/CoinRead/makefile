#-----------------------------------------------------------------------------
# File Name: makefile
# Description
# -----------
# This is the makefile for the CoinRead project.
#
#-----------------------------------------------------------------------------
SHELL 		= /bin/sh
EXECUTABLES	= CoinRead

CC=gcc-7
CXX=g++-7

#-- Release Compiler Flags ---------------------------------------------------

CFLAGS		+= -D__linux__ -O2 -g0 -Wall -pedantic -Wstrict-prototypes -std=c99
CFLAGS		+= -D__linux__ -pipe -fomit-frame-pointer -D_GNU_SOURCE

#-- Required libraries -------------------------------------------------------

LIBS		 = -lm -laes_access -lstdc++ 

#-- Dependencies -------------------------------------------------------------

OBJS = CoinRead.o 

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

CoinRead: $(OBJS)
		$(CC) $(OBJS) $(CFLAGS) $(LIBS) -o $@

#------------------------------ END OF FILE ----------------------------------

