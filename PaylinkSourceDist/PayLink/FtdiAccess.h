/******************************************************

This handles all the detail of accessing the FTDI link.

The application merely deals in packets

*******************************************************/
#ifndef FTDI_ACCESS
#define FTDI_ACCESS

#include "UsbAccess.h"

#include <stdio.h>
#ifndef __linux__
  #include "FTD2xx.h"
#else
  #include "ftdi_i.h"
  #include "ftdi.h"
#endif

class FTDIAccess : public USBAccess
    {
    char        ManufacturerBuf[32]   ;
    char        ManufacturerIdBuf[16] ;
    char        DescriptionBuf[64]    ;
    char        SerialNumberBuf[16]   ;

    int         ftDeviceSerialNumber ;
    unsigned long ftDevice ;

    int         CurrentModemStatus ;

    #ifndef __linux__
      int       BaudRateSetting ;
      int       BitsSetting ;
      int       StopSetting ;
      int       ParitySetting ;
    #else
      int                 BaudRateSetting ;
      ftdi_bits_type      BitsSetting ;
      ftdi_stopbits_type  StopSetting ;
      ftdi_parity_type    ParitySetting ;
    #endif

    virtual bool CommonUSBOpen() ;

  public:
    #ifndef __linux__
      FT_PROGRAM_DATA     ftData ;
      FT_HANDLE           ftHandle ;
      FT_STATUS           ftStatus ;

    #else
      struct ftdi_context Ftdi ;
      int                 ftStatus ;
      time_t              LastReadTime ;
    #endif
    int          FtdiLatency ;
    int          InterfaceType ;

    bool         USBOpenNew(char* FullProductName, char* DefaultUSBSerialNumber) ;
    virtual void USBClose(void) ;
    virtual bool CheckUSBNowOK(char *) ;
    virtual void RecoverDevice(void) ;
#ifndef __linux__
    virtual bool SetPortF(int BaudRate, int Bits, int Stop, int Parity) ;
#else
    virtual bool SetPortF(int BaudRate, ftdi_bits_type Bits, ftdi_stopbits_type Stop, ftdi_parity_type Parity) ;
#endif
    virtual bool WriteBuffer(char* Address, int Length) ;
    virtual int  ReadBuffer (char* Address, int Length) ;

    void        WriteCBus(int Pattern) ;
    void        SetDtr(bool On) ;
    int         ModemStatus(void) ;


    FTDIAccess(char* ProductName, int Vid, int Pid, int Baud) ;
    ~FTDIAccess() ;
    } ;

#endif
