#ifdef isalpha            // These may have come from Main.h
  #undef isalpha
  #undef isdigit
#endif

#ifndef __linux__
  #ifndef VCL_H
    #include <vcl.h>
  #endif
#else
  #include <pthread.h>
  #include <unistd.h>
  #include <stdio.h>
  #include <fcntl.h>
  #include <sys/stat.h>
#endif

#ifndef MAIN_HEADER
  #include "../Headers.h"
#endif
#include "../UsbAccess.h" /* AES Usb Header */
#include "../FtdiAccess.h" /* AES Ftdi Header */
#include "../HidAccess.h" /* AES HID Header */
#include "../VcpAccess.h" /* AES HID Header */
#include "../../Configuration.h"
#include "ReadConfig.h"



#ifndef __linux__
// Data for Driver from Config system
  extern        AnsiString      Serial ;
  extern        AnsiString      LoggingFileName ;
#else
  extern        char*     Serial ;
  extern        char      LoggingFileName[256] ;
#endif
extern        int             LogFileSize ;
extern        int             BCRComPort ;
