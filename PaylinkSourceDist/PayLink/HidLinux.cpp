/******************************************************

    This is the Linux HID Driver

*******************************************************/
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "win_types.h"

#include "hidapi.h"
#include "HidAccess.h" /* AES Ftdi Header */


/***********************************************************************

Constructor

***********************************************************************/
HIDAccess::HIDAccess(char* ProductName, int Vid, int Pid, int Report) :
            USBAccess(ProductName, Vid, Pid)
    {
    Type                 = HID ;
    m_OutputReportLength = 0 ;
    m_InputReportLength  = 0 ;
    m_ReportNumber       = Report ;
    m_Errors             = 0 ;
    BufferAddress        = 0 ;
    Recovery             = NONE ;
    m_ReadsReportNumber  = false ;
    m_InputBuffer  = 0 ;
    m_InputSegment = 0 ;
    }


/***********************************************************************

Close the HID Device driver

***********************************************************************/
void HIDAccess::USBClose(void)
  {
  if (Opened && m_Handle > 0)
     {
     try
         {
         DiagPrintf("HID: Closing %04x:%04x", Vid, Pid) ;
         hid_close(m_Handle) ;
         }
     catch (...)
         {
         DiagPrintf("HID: Close Error %04x:%04x", Vid, Pid) ;
         }
     }
  m_Handle = 0 ;
  Opened   = false ;
  }




HIDAccess::~HIDAccess()
{
  USBClose() ;
}


bool HIDAccess::CommonUSBOpen()
    {
    ++OpenCalls ;                                 // Keep long term track of things
    if (Opened)
    {
      USBClose() ;
    }
    m_InputSegment = 0 ;                // Re-align

    m_Handle = 0 ;

    struct hid_device_info* Devs ;
    Devs = hid_enumerate(Vid, Pid);
    if (!Devs)
      {                         // Now, that's weird as we think we've found it
      hid_free_enumeration(Devs);
      return false ;
      }
    m_Handle = hid_open_path(Devs->path) ;
    hid_free_enumeration(Devs) ;
    if (m_Handle < 0)
        {
        DiagPrintf("HID: Open failed") ;
        return false ;
        }
    Opened = true ;


    unsigned char Data[1024] ;
    int Count = AES_hid_get_report(m_Handle, Data, sizeof Data) ;
    if (Count <= 0)
        {
        DiagPrintf("HID: Report read failed %d", Count) ;
        return false ;
        }

    // Now "parse" the descriptor
    int FieldSize = 0 ;
    int FieldCount = 0 ;
    m_InputReportLength = 0 ;
    m_OutputReportLength = 0 ;

    for (int i = 0 ; i < Count && Data[i] != 0xC0 ; i += 2)
        {
        unsigned int NewLength ;
        switch (Data[i])
            {
        case 0x75:
            FieldSize = Data[i + 1] ;
            break ;

        case 0x95:
            FieldCount = Data[i + 1] ;
            break ;

        case 0x81:
            NewLength = (FieldSize * FieldCount) / 8 + 1 ;
            if (NewLength > m_InputReportLength)
              {
              m_InputReportLength = NewLength ;
              }
            FieldSize = 0 ;
            FieldCount = 0 ;
            break ;

        case 0x91:
            NewLength = (FieldSize * FieldCount) / 8 + 1 ;
            if (NewLength > m_OutputReportLength)
              {
              m_OutputReportLength = NewLength ;
              }
            FieldSize = 0 ;
            FieldCount = 0 ;
            break ;
            }

        if ((Data[i] & 1) == 0)
            {
            ++i ;                       // double byte field.
            }
        }
    hid_set_nonblocking(m_Handle, true);                        // Set reads to not hang the process

    DiagPrintf("HID: Opened %04x:%04x,", Vid, Pid) ;
    DiagPrintf("     Output %d, Input %d", m_OutputReportLength, m_InputReportLength) ;
    if (m_InputReportLength > 64)
        {
        m_InputBuffer = new char[m_InputReportLength] ;
        }
    Sleep(500) ;
    return true ;
    }




/***********************************************************************

Send and Receive Data

***********************************************************************/
bool HIDAccess::WriteBuffer(char* Address, int Length)
{
  int  BytesWritten     = 0 ;
  bool Working          = true ;
       Recovery         = WRITE ;

  while (Working && (Length > 0))
  {
    if ((signed)m_OutputReportLength - 1 > Length)
       {
       memset(Address + Length, 0xff, m_OutputReportLength - 1 - Length) ;
       }

    BufferAddress    = (unsigned char*) Address - 1;                             // This is specifically allowed.
    BufferAddress[0] = m_ReportNumber ;

    BytesWritten = hid_write(m_Handle, BufferAddress, m_OutputReportLength) ;
    if (BytesWritten < 0)
    {
        DiagPrintf("HID: Error %d on write\n", BytesWritten) ;
        Working = RecoverUSB() ;                 // Try and recover the chip
    }
    Length  -= (BytesWritten - 1) ;
    Address += (BytesWritten - 1) ;
  }
  return Working ;
}




int HIDAccess::ReadBuffer (char* Address, int Length)
{
    if (!Opened)
    {
      return  -1 ;
    }
    int DataRead = 0 ;  // no bytes read !!
    char* ReadTo  = (m_ReadsReportNumber) ? Address - 1 : Address ;

    memset(Address, 0xff, m_InputReportLength) ;


    if (m_InputReportLength > 64)
        {                                               // We need to do multiple reads!
        int JustRead = 0 ;
        do
            {
            JustRead = hid_read(m_Handle, (unsigned char*) m_InputBuffer       + m_InputSegment,
                                                           m_InputReportLength - m_InputSegment) ;
            m_InputSegment += JustRead ;

            if ((JustRead != 0)
             && (m_ReadsReportNumber)
             && (m_InputBuffer[0] == 0))
                {
                fprintf(stderr, "Realign Report\n") ;
                m_InputSegment = 0 ;
                }

            if (m_InputSegment + 8 > m_InputReportLength)
                {                   // We've done a complete read
                DataRead = m_InputSegment ;
                memcpy(ReadTo, m_InputBuffer, DataRead) ;
                m_InputSegment = 0 ;
                }
            } while (JustRead > 0) ;
        }
    else
        {
        DataRead = hid_read(m_Handle, (unsigned char*) ReadTo, m_InputReportLength) ;
        }

    if (DataRead && m_ReadsReportNumber)
        {
        DataRead-- ;
        }

    if (DataRead < 0)
    {
        DiagPrintf("HID: Error on read\n") ;
        Recovery = OPEN ;                          // We want to close and re-open it
        USBClose() ;
        return RecoverUSB() ? 0 : -1 ;
    }

    return DataRead ;
}






/*  **********************************************************

Is everything OK now! - called by RecoverUSB

***********************************************************************/

bool HIDAccess::CheckUSBNowOK(char* Attempt)
{
DiagPrintf("HID:Check Now OK %d\n", Recovery) ;
  if (Recovery == WRITE)
  {
    if (hid_write(m_Handle, BufferAddress, m_OutputReportLength) > 0)
        {
        DiagPrintf("HID: %s Fixed", Attempt) ;
        return true ;             // It's now working (worked!)
        }
    else
        {
        return false ;
        }
  }
  return m_Handle > 0 ;     // No recovery to do
}
