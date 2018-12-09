#-----------------------------------------------------------------------------
# File Name: makefile
# Description
# -----------
# This is the makefile for the AESCDriver project.
#
#-----------------------------------------------------------------------------
SHELL 		= /bin/sh
EXECUTABLES	= AESCDriver

CC = gcc-8
CXX = g++-8

#-- Release Compiler Flags ---------------------------------------------------

CXXFLAGS	+= -D__linux__ -O2 -g0 -Wall -I. -I.. -I../.. -I/usr/local/include/ -Wno-sign-compare -fno-strict-aliasing \
			-L/usr/local/Cellar/libusb/1.0.22/lib/ \
                   -Wno-write-strings -pipe -fomit-frame-pointer -D_GNU_SOURCE

CFLAGS		+= -D__linux__ -O2 -g0 -Wall -I. -I.. -I../.. -I/usr/local/include/ -Wno-sign-compare -fno-strict-aliasing \
			-L/usr/local/Cellar/libusb/1.0.22/lib/ \
                   -Wno-write-strings -pipe -fomit-frame-pointer -D_GNU_SOURCE

#-- Required libraries -------------------------------------------------------

LIBS		 	 =  -lusb-1.0 -lstdc++ -lpthread -liconv

#-- Dependencies -------------------------------------------------------------

AESC_DRIVER_OBJS  = LinuxMain.o DriverFuncs.o ReadConfig.o ../libftdi.o ../libhid.o ../UsbAccess.o ../VcpLinux.o ../HidLinux.o ../FtdiAccess.o ../UsbMilan.o
AESC_DRIVER_DEPS  = DriverFuncs.h ../Ftdi.h ../../Configuration.h

#-- Compile All --------------------------------------------------------------

.PHONEY: all
all: 	$(EXECUTABLES)

#-- Clean Up -----------------------------------------------------------------

.PHONEY: clean
clean:
	rm -f ../FtdiAccess.o* ../FtdiMilan.o* ../VcpLinux.o* 
	rm -f *.[bo]*
	rm -f *.err
	rm -f $(EXECUTABLES)

#-- Install ------------------------------------------------------------------

.PHONEY: install
install:

	cp -fv $(EXECUTABLES) /usr/local/bin/

#-- Build targets ------------------------------------------------------------


AESCDriver:	$(AESC_DRIVER_OBJS)
		rm -f AESCDriver
		$(CC) $(AESC_DRIVER_OBJS) $(CFLAGS) $(LIBS) -o AESCDriver
		chown root AESCDriver
		chmod 6711 AESCDriver



		
#-- Check on headers ---------------------------------------------------------

AESCDriver.o: $(AESC_DRIVER_DEPS)

#------------------------------ END OF FILE ----------------------------------

