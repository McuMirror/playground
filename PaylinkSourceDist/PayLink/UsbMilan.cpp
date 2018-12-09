/******************************************************

This handles all the detail of accessing the USB link.

The application merely deals in packets

*******************************************************/
#ifndef __linux__
 #include <vcl.h>
 #pragma hdrstop
#else
 #include <stdlib.h>
 #include <string.h>
#endif
#include <stdarg.h>
#include "FtdiAccess.h" /* AES Ftdi Header */

extern bool ShowTraffic ;


/******************************************************************

Generalised formatted Diagnostics output function. It requires the
main program to provide DiagText(char* Line), a function that will
acceptr a null terminate string (with an assumed, but not present,
line end character)

******************************************************************/
void DiagPrintf(const char* Format, ...)
{
  char Buffer[2048] ;
  va_list ap ;
  va_start(ap, Format) ;

  vsprintf(Buffer, Format, ap) ;
  DiagText(Buffer) ;
  va_end(ap) ;
}





/***********************************************************************

SendBuffer - returns 0 if good, -1 if a problem

***********************************************************************/
int  USBMilan::SendBuffer(void)
{
  if (TxBufferIndex == 0)
  {
    return 0 ;
  }

  if (Port->WriteBuffer((char*)TxBuffer, TxBufferIndex))
  {
    TxBufferIndex = 0 ;             // Reset buffer
    return 0 ;
  }
  else
  {
    DiagPrintf("PC: USB Write Failed") ;
    TxBufferIndex = 0 ;             // Reset buffer
    return -1 ;
  }
}






/***********************************************************************

QueuePacket

***********************************************************************/
void USBMilan::QueuePacket(unsigned short Address, int Value)
{
  if (ShowTraffic)
  {
    DiagPrintf(" -->%04x %08x", Address, Value) ;
  }

  short* pShort = (short*)&TxBuffer[TxBufferIndex] ;
  *pShort = Address ;
  TxBufferIndex += 2 ;

  int*  pint  =  (int*)&TxBuffer[TxBufferIndex] ;
  *pint  = Value ;
  TxBufferIndex += 4 ;

  if (TxBufferIndex > (short)(Sizeof_TxBuffer))
  {                   // Tihs can't happen !!!
    DiagPrintf(" ******   Buffer overflow ******") ;
    SendBuffer() ;
  }
}





/***********************************************************************

ResetTxBuffer

***********************************************************************/
void USBMilan::ResetTxBuffer(void)
{
  TxBufferIndex = 0 ;
}



/***********************************************************************

ProcessIncomingStream

***********************************************************************/

bool USBMilan::ProcessIncomingStream(void)
{
  int                   RxBufferRead ;
  unsigned char*        RxBuffer = RxSpace + 16 ;


  RxBufferRead = Port->ReadBuffer((char*)RxBuffer, sizeof RxSpace - 16) ;

  while (RxBufferRead > 0)
  {
    bool SyncSent = false ;

    for (int i = 0 ; i < RxBufferRead ; ++i)
    {
      if (PacketIndex == 0 && RxBuffer[i] == 0xff)
      {
        // Skip Sync characters
        if ((i < 6)              // (HID buffers are padded so only test 1st packet in buffer)
         && !SyncSent)
        {                        // The remote system has resynched
          QueuePacket(-1 , -1) ; // So we will too!
          SyncSent = true ;
        }
      }
      else
      {
        Packet[PacketIndex] = RxBuffer[i] ;
        if (++PacketIndex == 6)
        {           // Process the packet
          (*ProcessIncomingPacket)(*(short*)Packet, *(int*)&Packet[2]) ;
          PacketIndex = 0 ;
        }
      }
    }

  // This is required for HID devices to work efficiently
  RxBufferRead = Port->ReadBuffer((char*)RxBuffer, sizeof RxSpace - 16) ;
  }

  return !(RxBufferRead < 0) ;
}
