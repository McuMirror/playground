/******************************************************

This handles all the detail of accessing the HID link.

The application merely deals in packets

*******************************************************/
#ifndef HID_ACCESS
#define HID_ACCESS

#include "UsbAccess.h"

#include <stdio.h>

struct hid_device_;
typedef struct hid_device_ hid_device; /**< opaque hidapi structure on Linux */


class HIDAccess : public USBAccess
    {
    enum
       {
       NONE,
       OPEN,
       WRITE
       } Recovery ;
    int          m_Errors ;
    unsigned int m_OutputReportLength ;
    unsigned int m_InputReportLength ;
    char         m_ReportNumber ;           // The report number to send Data
   public:
    bool           m_ReadsReportNumber ;    // Flag to show report number will be on incoming data.
 #ifdef __linux__
    hid_device*    m_Handle ;
    unsigned char* BufferAddress ;          // This is a copy of Address for the write, for use during recovery
    char*          m_InputBuffer ;          // We may need mutliple reads to get a complete report
    unsigned int   m_InputSegment ;
  #else
    char*        BufferAddress ;            // This is a copy of Address for the write, for use during recovery
    bool         IORunning ;
    long         m_LastError ;
    HANDLE       m_Handle ;
    HANDLE       m_ThreadEvent ;
    OVERLAPPED   m_ReadOver ;
    OVERLAPPED   m_WriteOver ;
  #endif

    virtual bool CommonUSBOpen() ;

    bool         USBOpenNew(char* FullProductName, char* DefaultUSBSerialNumber) ;
    virtual void USBClose(void) ;
    virtual bool CheckUSBNowOK(char *) ;

    virtual bool WriteBuffer(char* Address, int Length) ;
    virtual int  ReadBuffer (char* Address, int Length) ;

    HIDAccess(char* ProductName, int Vid, int Pid, int Report) ;
    ~HIDAccess() ;

    void CheckError(char* Type) ;
    } ;

#endif
