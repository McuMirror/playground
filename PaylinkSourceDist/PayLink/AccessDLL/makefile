#-----------------------------------------------------------------------------
# File Name: makefile
# Description
# -----------
# This is the makefile for the AccessDLL project.
#
#-----------------------------------------------------------------------------
SHELL 		= /bin/sh

#-- Release Compiler Flags ---------------------------------------------------

CXXFLAGS	+= -D__linux__ -g0 -Wall -Wno-write-strings -Wno-sign-compare -shared -D_GNU_SOURCE -Wno-invalid-offsetof -I. -I../../
CFLAGS		+= -D__linux__ -g0 -Wall -Wno-int-to-pointer-cast -pedantic -Wstrict-prototypes -D_GNU_SOURCE -std=c99 -I. -I../../

CC = gcc-8
CXX = g++-8

#-- Required libraries -------------------------------------------------------

LIBS		 = -lstdc++ -lm -lc

#-- Compile All --------------------------------------------------------------

.PHONEY: all
all:		libaes_access.so

#-- Check if required libraries are available --------------------------------

.PHONEY: check_lib_32
check_lib_32: 
	! $(CC) -m32 $(LIBS) 2>&1 | egrep "cannot find -l" 1>&2

#-- Compile 32 and 64-bit library --------------------------------------------

.PHONEY: all_32_64
all_32_64:	libaes_access_64.so libaes_access_32.so

#-- Clean Up -----------------------------------------------------------------

.PHONEY: clean
clean:

	rm -f *.[bo]
	rm -f *.err
	rm -f /usr/local/lib/libaes_access*
	rm -f libaes_access.a libaes_access.so* libaes_access_64.so* libaes_access_32.so* 1>/dev/null 2>&1

#-- Install ------------------------------------------------------------------

.PHONEY: install
install:

	cp libaes_access.so.1.2.0  /usr/local/lib/
	ln -sf /usr/local/lib/libaes_access.so.1.2.0 /usr/local/lib/libaes_access.so
	ln -sf /usr/local/lib/libaes_access.so.1.2.0 /usr/local/lib/libaes_access.so.1

#-- Install 32- and 64-bit versions ------------------------------------------

.PHONEY: install_32_64
install_32_64:

	cp libaes_access_64.so.1.2.0  /usr/local/lib/libaes_access_64.so.1.2.0
	cp libaes_access_32.so.1.2.0  /usr/local/lib/
	ln -sf /usr/local/lib/libaes_access_64.so.1.2.0 /usr/local/lib/libaes_access_64.so
	ln -sf /usr/local/lib/libaes_access_64.so.1.2.0 /usr/local/lib/libaes_access_64.so.1
	ln -sf /usr/local/lib/libaes_access_32.so.1.2.0 /usr/local/lib/libaes_access_32.so
	ln -sf /usr/local/lib/libaes_access_32.so.1.2.0 /usr/local/lib/libaes_access_32.so.1

#-- Build targets ------------------------------------------------------------

libaes_access.so:
	$(CC)  -fPIC -g -c $(CFLAGS) MapRam.c
	$(CXX) -fPIC -g -c $(CXXFLAGS) Aesimhei.cpp
	$(CXX) -fPIC -g -c $(CXXFLAGS) ../../DESEncrypt.cpp
	$(CC)  -shared -Wl,-install_name,libaes_access.so.1 \
		-o libaes_access.so.1.2.0 MapRam.o Aesimhei.o DESEncrypt.o $(LIBS)

libaes_access_32.so:
	$(CC)  -fPIC -g -c $(CFLAGS) -m32 MapRam.c
	$(CXX) -fPIC -g -c $(CXXFLAGS) -m32 Aesimhei.cpp
	$(CXX) -fPIC -g -c $(CXXFLAGS) -m32 ../../DESEncrypt.cpp
	$(CC)  -m32 -shared -Wl,-install_name,libaes_access_32.so.1 \
		-o libaes_access_32.so.1.2.0 MapRam.o Aesimhei.o DESEncrypt.o $(LIBS)

libaes_access_64.so: libaes_access.so
	mv libaes_access.so.1.2.0 libaes_access_64.so.1.2.0
        
#------------------------------ END OF FILE ----------------------------------
