/******************************************************

This handles all the detail of accessing a generic USB link.

The application merely deals in packets

*******************************************************/
#ifndef __linux__
 #include <vcl.h>
 #pragma hdrstop
 #include <setupapi.h> // Used for SetupDiXxx functions
 #include "WinUSB.h"
#else
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 #include "win_types.h"
#endif
#include <stdarg.h>
#include "UsbAccess.h" /* AES Usb Header */





#ifndef __linux__
char* GetErrorText(unsigned long LastError)
{
    static char ErrorMessage[1024] ;
    if (LastError == 0xE0000235)
        {
        return "64 bit control not allowed from 32 bit" ;
        }

    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  LastError,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                  ErrorMessage,
                  sizeof ErrorMessage,
                  NULL);
    ErrorMessage[strlen(ErrorMessage) - 2] = 0 ;                // lose \r\n
    return ErrorMessage ;
}
#endif

/***********************************************************************

Constructor

***********************************************************************/
USBAccess::USBAccess(char* ProductName, int Vid, int Pid)
    {
    this->ProductName       = ProductName ;
    this->Vid               = Vid ;
    this->Pid               = Pid ;

    Opened                  = false ;
    Recovered               = false ;
    OpenCalls               = 0 ;
    memset(DeviceSerialNumber, '\0', sizeof DeviceSerialNumber) ;
    #ifndef __linux__
        USBFuncs      = new WinUSB(Vid, Pid) ;
    #endif
    }


bool  USBAccess::USBOpen()
    {
    memset(DeviceSerialNumber, '\0', sizeof DeviceSerialNumber) ;
    return CommonUSBOpen();
    }

bool  USBAccess::USBOpenSpecific(char *Serial)
    {
    strcpy(DeviceSerialNumber, Serial) ;
    return CommonUSBOpen();
    }





USBAccess::~USBAccess()
{
}






bool USBAccess::RecoverUSB(void)
{
  int       OpenAttempts ;
  bool      AccessResult ;

  Recovered = true ;                            // Flag we've had a problem


  if (!Opened)
  {
    DiagPrintf("USB was closed") ;
  }

#ifndef __linux__
//
//
//      Windows Recovery
//
//
  RecoverDevice() ;

  if (CheckUSBNowOK("Reset Device"))
  {
    return true ;
  }

  USBClose() ;

  AccessResult = CommonUSBOpen() ;                    // open the USB device we're using

  if (AccessResult)
  {
    if (CheckUSBNowOK("close & open"))
    {
      return true ;
    }
    USBClose() ;
  }

  // OK - Lets see if it comes back!

  for (OpenAttempts = 0 ; OpenAttempts < 15 ; ++OpenAttempts)
  {
    Sleep(500) ;
    AccessResult = USBFuncs->LocateChip() ;

    if (!AccessResult)
    {
      DiagPrintf("USB: No Device!") ;
      continue ;
    }

    if (USBFuncs->ChipDisabled())
    {
      DiagPrintf("USB: Re-enabling Device") ;
      USBFuncs->EnableChip() ;
      continue ;
    }

    AccessResult = CommonUSBOpen() ;

    if (AccessResult)
    {
      if (CheckUSBNowOK("Opened"))
      {
        return true ;
      }
      USBClose() ;
      continue ;
    }

    if ((OpenAttempts & 7) == 7)
    {
      DiagPrintf("USB: Resetting Device") ;
      USBFuncs->ResetChip() ;                     // Turn it back off
    }
  }
#else
//
//
//      Linux Recovery
//
//
  if (!Opened)
  {
    DiagPrintf("USB: %s Closed", ProductName) ;
    return false ;
  }
  DiagPrintf("USB: %s Recovery - close", ProductName) ;
  USBClose() ;

  AccessResult = CommonUSBOpen() ;
  if (AccessResult)
  {
    if (CheckUSBNowOK((char *)"Close + Open"))
    {
      return true ;
    }
  }

  for (OpenAttempts = 0 ; OpenAttempts < 50 ; ++OpenAttempts)
  {
    Sleep(1000) ;
    AccessResult = CommonUSBOpen() ;
    DiagPrintf("USB: %s Recovery - Open = %d", ProductName, AccessResult) ;

    if (AccessResult)
    {
      if (CheckUSBNowOK((char *)"Close + Open 2"))
      {
        return true ;
      }
      USBClose() ;
      continue ;
    }
  }
#endif
  DiagPrintf("USB: Recovery %s *** Failed! ***", ProductName) ;
  // All right - I give up
  return false ;
}





