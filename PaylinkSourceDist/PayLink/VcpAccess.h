/******************************************************

This handles all the detail of accessing the VCP link.

The application merely deals in packets

*******************************************************/
#ifndef VCP_ACCESS
#define VCP_ACCESS

#include "UsbAccess.h"

#include <stdio.h>

class VCPAccess : public USBAccess
    {
    enum
       {
       NONE,
       OPEN,
       WRITE
       } Recovery ;
    char         m_Name[2048] ;
    int          CommsNumber ;
    char*        SerialDevice ;

    int          BaudRate ;
    int          m_Errors ;
    unsigned int m_OutputReportLength ;
    unsigned int m_InputReportLength ;
    bool         IORunning ;
    char*        BufferAddress ;            // This is a copy of Address for the write, for use during recovery
    int         m_LastError ;

    int         BaudRateSetting ;
    int         BitsSetting ;
    int         StopSetting ;
    int         ParitySetting ;

  #ifdef __linux__
    int          m_Handle ;
    int          m_Written ;            // Total Written
    int          m_Read ;               // Total Read
    int          m_SetCount ;           // Count of Set attempts to get line working
  #else
    HANDLE       m_Handle ;
    HANDLE       m_ThreadEvent ;
    OVERLAPPED   m_ReadOver ;
    OVERLAPPED   m_WriteOver ;
  #endif

  public:
    virtual bool CommonUSBOpen() ;

    bool         USBOpenNew(char* FullProductName, char* DefaultUSBSerialNumber) ;
    virtual void USBClose(void) ;
    virtual bool SetPortW(int Baud, int Bits, int Stop, int Parity) ;
    virtual bool CheckUSBNowOK(char *) ;
    virtual bool WriteBuffer(char* Address, int Length) ;
    virtual int  ReadBuffer (char* Address, int Length) ;
    VCPAccess(char* Name, int VID, int PID, int CommsNumber, char* SerialDevice, int BaudRate) ;
    ~VCPAccess() ;

    void CheckError(char* Type) ;
    } ;

#endif
