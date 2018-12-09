##-----------------------------------------------------------------------------
# File Name: makefile
# Description
# -----------
# This is the makefile for the USB Programmer project.
#
#-----------------------------------------------------------------------------
SHELL 		= /bin/sh
EXECUTABLES	= USBProgram

#-- Release Compiler Flags ---------------------------------------------------

CXXFLAGS	+= -D__linux__ -O2 -g0 -Wall -fno-strict-aliasing -pipe -fomit-frame-pointer -D_GNU_SOURCE
CFLAGS		+= -D__linux__ -O2 -g0 -Wall -pipe -fomit-frame-pointer -D_GNU_SOURCE

CC=gcc-8
CXX=g++-8

#-- Required libraries -------------------------------------------------------

LIBS		 	 =  -lusb-1.0 -lstdc++ -lpthread

#-- Dependencies -------------------------------------------------------------

USB_PROGRAM_OBJS 	= USBProgram.o ProgramCard.o ReadSRec.o ../UsbAccess.o ../FtdiAccess.o ../UsbMilan.o ../libftdi.o
USB_PROGRAM_DEPS 	= ProgramCard.h ReadSRec.h

#-- Compile All --------------------------------------------------------------

.PHONEY: all
all: 	$(EXECUTABLES)

#-- Clean Up -----------------------------------------------------------------

.PHONEY: clean
clean:
	rm -f ../FtdiAccess.o ../FtdiMilan.o USBProgram.o ProgramCard.o ReadSRec.o 
	rm -f *.[bo]
	rm -f *.err
	rm -f $(EXECUTABLES)

#-- Install ------------------------------------------------------------------

.PHONEY: install
install:

	cp -fv $(EXECUTABLES) /usr/local/bin/

#-- Build targets ------------------------------------------------------------

USBProgram:	$(USB_PROGRAM_OBJS)
		$(CC) $(USB_PROGRAM_OBJS) $(LIBS) -o $@

#-- Check on headers ---------------------------------------------------------

USBProgram.o:	$(USB_PROGRAM_DEPS)

#------------------------------ END OF FILE ----------------------------------

