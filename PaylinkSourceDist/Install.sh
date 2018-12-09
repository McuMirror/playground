#!/bin/bash
#-- Argument section to check if multiarch libraries are to be built ---------#
multiarch=0
if [ ! $# -eq 0 ]; then
    if [ "$1" = "multi" ]; then
        multiarch=1
    fi
fi
#-- lecho function shows [PASS]/[FAIL] at end of line ------------------------#
message=""
lecho()
{ chars=$(echo $message | wc -c)
  let "remChars=75 - $chars"
  spaces=""
  while [ "$remChars" -ne "2" ]
  do
    spaces="$spaces "
    let remChars=remChars-1
  done
  echo -n "$spaces"
  if [ $1 -ne 0 ]; then
    echo -en "[\033[1;31mFAIL\033[0m]\n"
  else
    echo -en "[\033[1;32mPASS\033[0m]\n"
  fi
}


#-----------------------------------------------------------------------------#
pd="$(pwd)"

#-----------------------------------------------------------------------------#
if [ $(id -u) != 0 ]; then
  echo -e "ERROR: Must be superuser (root) to run the Install script.\n"
  exit 1
fi
    

#-- Set up headers ---------------------------------------------#
message="  Copying win_types.h.....=>./usr/local/include/ ..."
echo -n $message
rm -f /usr/include/win_types.h
cp ./win_types.h      /usr/local/include/ 1>/dev/null 
lecho $?
message="  Copying ImheiEvent.h...=>./usr/local/include/ ..."
echo -n $message
rm -f /usr/include/ImheiEvent.h
cp ./ImheiEvent.h    /usr/local/include/ 1>/dev/null 
lecho $?
message="  Copying Aesimhei.h.....=>./usr/local/include/ ..."
echo -n $message
rm -f /usr/include/Aesimhei.h
cp ./Aesimhei.h      /usr/local/include/ 1>/dev/null 
lecho $?

# -- Now we're going to sort out libusb ------------------------------- #
# If we have a local libusb, then that's either ours, or
# they might want to update it
#

if [ -f /usr/local/lib/libusb-1.0.so ] && [  -f /usr/local/include/libusb-1.0/libusb.h ] ; then
  echo -en "\n/usr/local libusb exists. Re-build with Paylink version [y/n] : "
  read install
  if [ "$install" = "y" ] ; then
    install=Y
  fi
else

# If there's no local, and a system libusb then that's probably the one we want, 
# but it might be too old, so we need to allow for installing ours.
#
# So, if the system libusb is there, we ask if they want ours:

  if [ -f /lib/*/libusb-1.0.so ] || [ -f /lib/libusb-1.0.so ] ; then
    echo -en "\nSystem libusb-1.0 exists. Use Paylink Specific Version instead? [y/n] : "
    read install
    if [ "$install" = "y" ] ; then
      install=Y
    fi
  else
    install=Y
  fi
fi

if [ "$install" = "Y" ] ; then
  message="(Re)building libusb ..."
  echo -n $message
  cd PayLink
  rm -rf libusb-1.0.20
  tar -jxf libusb-1.0.20.tar.bz2
  cd libusb-1.0.20
  sh ./configure  --disable-udev CFLAGS="-g -O2" 1>/dev/null
  if [ "$?" = "0" ] ; then
    make 1>/dev/null
    if [ "$?" = "0" ] ; then
      make install 1>/dev/null 2>&1
      lecho $?
    else
      echo "libusb - Install Error"
      exit 1
    fi
  else
    echo "libusb - Build Error"
    exit 1
  fi
  ldconfig /usr/local/lib
  cd $pd
  rm -r PayLink/libusb-1.0.20
else
  echo Note: The link error 'libusb_handle_events_completed'
  echo .     can be caused by an old libusb release.
fi

#-- Build AES Access Shared Library ------------------------------------------#
message="Building AES Access Shared Library (libaes_access.so) ..."
if [ $multiarch -eq 1 ]; then
    message="Building 32 and 64-bit AES Access Shared Library (libaes_access.so) ..."
fi

echo -n $message
cd PayLink/AccessDLL
make clean 1>/dev/null
if [ "$?" = "0" ] ; then
    if [ $multiarch -eq 1 ]; then
        make check_lib_32 1>/dev/null 2>&1

        if [ ! $? -eq 0 ]; then
            echo -e "\n[\033[1;31mPlease install 32-bit libraries of the ones mentioned below ('cannot find -lX' -> sudo apt-get install libX:i386)\033[0m]"       
        fi
        
        make all_32_64 1>/dev/null 2>Errors
        if [ "$?" = "0" ] ; then
             make install_32_64 1>/dev/null  
             lecho $?
        else
             lecho $?
             cat Errors
        fi
    else
        make 1>/dev/null 2>Errors
        if [ "$?" = "0" ] ; then
             make install 1>/dev/null
             lecho $?
        else
             lecho $?
             cat Errors
        fi
    fi
fi
rm -f Errors

cd $pd


#-- Build AES Driver Executable ---------------------------------------------#
message="Building AES Driver Executable (AESCDriver) ..."
echo -n $message
cd PayLink/USB\ Driver/
make clean 1>/dev/null
make 1>/dev/null 2>Errors
if [ "$?" = "0" ] ; then
    make install 1>/dev/null
    lecho $?
else
    lecho $?
    cat Errors
fi
rm -f Errors


message="Copying (showtraf command) ..."
echo -n $message
cp ./showtraf.sh /usr/local/bin/showtraf.sh 1>/dev/null 
lecho $?

chmod 6775 /usr/local/bin/showtraf.sh 1>/dev/null 

cd $pd


#-- Build AES Reprogrammer Executables ---------------------------------------------#
#message="Building AES Reprogrammer Executables (USBProgram / RxProgram) ..."
#echo -n $message
#cd PayLink/LinuxUSBProgrammer/
#make clean 1>/dev/null
#make clean 1>/dev/null
#make 1>/dev/null 2>Errors
#if [ "$?" = "0" ] ; then
#    make install 1>/dev/null
#    lecho $?
#else
#    lecho $?
#    cat Errors
#fi
#rm -f Errors

#cd $pd
#cd PayLink/LinuxRxProgrammer/
#make clean 1>/dev/null
#make clean 1>/dev/null
#make 1>/dev/null 2>Errors
#if [ "$?" = "0" ] ; then
#    make install 1>/dev/null
#    lecho $?
#else
#    lecho $?
#    cat Errors
#fi
#rm -f Errors

cd $pd

echo ""
