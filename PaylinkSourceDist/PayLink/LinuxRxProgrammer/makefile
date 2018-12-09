##-----------------------------------------------------------------------------
# File Name: makefile
# Description
# -----------
# This is the makefile for the USB Programmer project.
#
#-----------------------------------------------------------------------------
SHELL 		= /bin/sh
EXECUTABLES	= RxProgram

#-- Release Compiler Flags ---------------------------------------------------

CXXFLAGS	+= -D__linux__ -O2 -g0 -Wall -fno-strict-aliasing -pipe -fomit-frame-pointer -D_GNU_SOURCE
CFLAGS		+= -D__linux__ -O2 -g0 -Wall -pipe -fomit-frame-pointer -D_GNU_SOURCE

CC=gcc-8
CXX=g++-8

#-- Required libraries -------------------------------------------------------

LIBS		 	 =  -lusb-1.0 -lstdc++ -lpthread -liconv

#-- Dependencies -------------------------------------------------------------

USB_PROGRAM_OBJS 	= RxProgram.o ProgramCard.o ReadSRec.o ../UsbAccess.o ../HidLinux.o ../UsbMilan.o ../libhid.o
USB_PROGRAM_DEPS 	= ProgramCard.h ReadSRec.h

#-- Compile All --------------------------------------------------------------

.PHONEY: all
all: 	$(EXECUTABLES)

#-- Clean Up -----------------------------------------------------------------

.PHONEY: clean
clean:
	rm -f ../FtdiAccess.o ../FtdiMilan.o RxProgram.o ProgramCard.o ReadSRec.o 
	rm -f *.[bo]
	rm -f *.err
	rm -f $(EXECUTABLES)

#-- Install ------------------------------------------------------------------

.PHONEY: install
install:

	cp -fv $(EXECUTABLES) /usr/local/bin/

#-- Build targets ------------------------------------------------------------

RxProgram:	$(USB_PROGRAM_OBJS)
		$(CC) $(USB_PROGRAM_OBJS) $(LIBS) -o $@

#-- Check on headers ---------------------------------------------------------

RxProgram.o:	$(USB_PROGRAM_DEPS)

#------------------------------ END OF FILE ----------------------------------

