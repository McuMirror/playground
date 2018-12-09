/******************************************************

This handles all the detail of accessing the FTDI link.

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
#include "FtdiAccess.h" /* AES Ftdi Header */







/***********************************************************************

Constructor

***********************************************************************/
FTDIAccess::FTDIAccess(char* ProductName, int Vid, int Pid, int Baud) :
            USBAccess(ProductName, Vid, Pid)
    {
    Type                    = FTDI ;
    ftDeviceSerialNumber    = 0 ;
    FtdiLatency             = 16 ;
    ftDevice                = -1 ;
    Opened                  = false ;
    OpenCalls               = 0 ;

    this->BaudRateSetting   = Baud ;
    #ifndef __linux__
        this->BitsSetting   = FT_BITS_8 ;
        this->StopSetting   = FT_STOP_BITS_1 ;
        this->ParitySetting = FT_PARITY_NONE ;
    #else
        this->BitsSetting   = BITS_8 ;
        this->StopSetting   = STOP_BIT_1 ;
        this->ParitySetting = NONE ;
        LastReadTime        = 0 ;
    #endif
    }


/***********************************************************************

SetPort Function

***********************************************************************/

#ifndef __linux__
bool FTDIAccess::SetPortF(int BaudRate, int Bits, int Stop, int Parity)
#else
bool FTDIAccess::SetPortF(int BaudRate, ftdi_bits_type Bits, ftdi_stopbits_type Stop, ftdi_parity_type Parity)
#endif
  {
  BaudRateSetting = BaudRate ;
  BitsSetting     = Bits ;
  StopSetting     = Stop ;
  ParitySetting   = Parity ;

#ifndef __linux__
    ftStatus = FT_ResetDevice(ftHandle);
    if (!FT_SUCCESS(ftStatus)) {
      DiagPrintf("SetPort Reset failed %d", ftStatus) ;
      return false;
      }

    ftStatus = FT_SetBaudRate(ftHandle, BaudRate);
    if (!FT_SUCCESS(ftStatus)) {
        DiagPrintf("SetPort Baud failed %d", ftStatus) ;
        return false;
    }

    ftStatus = FT_SetDataCharacteristics(
                    ftHandle,
                    Bits,
                    Stop,
                    Parity);
    if (!FT_SUCCESS(ftStatus)) {
        DiagPrintf("SetPort Bits failed %d", ftStatus) ;
        return false;
    }

    ftStatus = FT_SetFlowControl(
                    ftHandle,
                    FT_FLOW_NONE,
                    0,
                    0);
    if (!FT_SUCCESS(ftStatus)) {
        DiagPrintf("SetPort Flow failed %d", ftStatus) ;
        return false;
    }

    ftStatus = FT_SetChars(ftHandle, 0, 0, 0, 0) ;

    FT_Purge(ftHandle,FT_PURGE_TX | FT_PURGE_RX);

#else
    if ((ftStatus = ftdi_usb_reset(&Ftdi)) != 0)
    {
        DiagPrintf("ftdi_usb_reset failed: %d (%s)", ftStatus, ftdi_get_error_string(&Ftdi));
        return false;
    }

    if ((ftStatus = ftdi_set_baudrate(&Ftdi, BaudRate)) != 0)
    {
        DiagPrintf("ftdi_set_baudrate failed: %d (%s)", ftStatus, ftdi_get_error_string(&Ftdi));
        return false;
    }

    if ((ftStatus = ftdi_set_line_property(&Ftdi, Bits, Stop, Parity)) != 0)
    {
        DiagPrintf("ftdi_set_line_property failed: %d (%s)", ftStatus, ftdi_get_error_string(&Ftdi));
        return false;
    }

    if ((ftStatus = ftdi_setflowctrl(&Ftdi, SIO_DISABLE_FLOW_CTRL)) != 0) {
        DiagPrintf("ftdi_setflowctrl failed: %d (%s)", ftStatus, ftdi_get_error_string(&Ftdi));
        return false;
    }

    if ((ftStatus = ftdi_set_event_char (&Ftdi, 0, 0)) != 0)
    {
        DiagPrintf("ftdi_set_event_char failed: %d (%s)", ftStatus, ftdi_get_error_string(&Ftdi));
        return false;
    }

    if ((ftStatus = ftdi_usb_purge_buffers (&Ftdi)) != 0)
    {
        DiagPrintf("ftdi_usb_purge_buffers failed: %d (%s)", ftStatus, ftdi_get_error_string(&Ftdi));
        return false;
    }
#endif
    return true;
  }



/***********************************************************************

Close the FTDI Device driver

***********************************************************************/
void FTDIAccess::USBClose(void)
{
#ifndef __linux__
  ftStatus = FT_Close(ftHandle);
  ftHandle = 0 ;
#else
  ftdi_usb_close(&Ftdi);
  ftdi_deinit(&Ftdi);
#endif
  Opened   = false;
  DiagPrintf("%s Closed", ProductName);
}




FTDIAccess::~FTDIAccess()
{
  USBClose() ;
}


bool FTDIAccess::CommonUSBOpen()
{
#ifndef __linux__
  ++OpenCalls ;                                 // Keep long term track of things
  if (Opened)
  {
    USBClose() ;
  }

  bool ReturnCode ;

  if (Vid != 0)                    // The marker for "open anything"
  {
    ReturnCode = USBFuncs->LocateChip();      // This will find any Paylink FTDI chip - ignoring serial number
    if (!ReturnCode)
    {
      DiagPrintf("No matching USB Device") ;
      return false ;
    }

    if (USBFuncs->ChipDisabled())
    {
      USBFuncs->EnableChip() ;              // Try to turn it on
      Sleep(500) ;
      if (USBFuncs->ChipDisabled())         // Did it work?
      {
        DiagPrintf("USB Device Disabled!") ;
        return false ;
      }
    }
  }


  // Now we get a list of FTDI devices on the system
  DWORD FTDITotal ;
  ftStatus = FT_CreateDeviceInfoList(&FTDITotal) ;
  if (!FT_SUCCESS(ftStatus))
    {
    DiagPrintf("USB: Error %d getting device count", ftStatus) ;
    return false ;
    }
  FT_DEVICE_LIST_INFO_NODE* FTList = new FT_DEVICE_LIST_INFO_NODE[FTDITotal] ;
  ftStatus = FT_GetDeviceInfoList(FTList, &FTDITotal) ;
  if (!FT_SUCCESS(ftStatus))
  {
    DiagPrintf("USB: Error %d getting device list", ftStatus) ;
    delete[] FTList ;
    return false ;
  }


  for (unsigned int DeviceNo = 0 ; DeviceNo <  FTDITotal ; ++DeviceNo)
  {
    if (FTList[DeviceNo].Flags & FT_FLAGS_OPENED)
    {
      DiagPrintf("USB: Device %d not available", DeviceNo) ;
    }
    else
    {
      if ((Vid == 0)                    // The marker for "open anything"
       || ((FTList[DeviceNo].ID & 0xffff) == Pid && (FTList[DeviceNo].ID >> 16) == Vid))
      {
        // Then it IS ours - is it the correct one?

        // Note that we only check the serial number
        // if one has been supplied!

        if ((!strlen(DeviceSerialNumber))
        ||  (!strcmp(DeviceSerialNumber, FTList[DeviceNo].SerialNumber)))
        {
          // It's the one we want!
          DiagPrintf("USB: Unit %d, ID: 0x%04x 0x%04x", DeviceNo, FTList[DeviceNo].ID >> 16, FTList[DeviceNo].ID & 0xffff) ;
          DiagPrintf("    Description: %s",            FTList[DeviceNo].Description) ;
          DiagPrintf("      Serial No: %s",            FTList[DeviceNo].SerialNumber) ;

          ftStatus = FT_Open(DeviceNo, &ftHandle);

          // We should not reset the FTDI chip if we are trying
          // to talk to a SPECIFIC device, as we can end up
          // resetting the OTHER device!
          if (ftStatus == FT_INVALID_HANDLE && ((OpenCalls & 7) == 7))
          {
            if (!strlen(DeviceSerialNumber))
            {
              USBFuncs->ResetChip() ;                     // Every 8 tries, reset it off, to try to recover it
            }
          }
          if (!FT_SUCCESS(ftStatus))
          {
            DiagPrintf("       Open failed %d", ftStatus) ;
            delete[] FTList ;
            return false ;
          }

          DWORD    deviceID = 0 ;
          char*   Type = "Not recognised" ;
          ftStatus = FT_GetDeviceInfo(ftHandle, &ftDevice, &deviceID, SerialNumberBuf, DescriptionBuf, NULL);
          InterfaceType = ftDevice ;
          if (ftDevice == 0)
          {
            Type = "Revison 3 (Original)" ;
          }
          else if (ftDevice == 5)
          {
            Type = "Revision 4 (USB Powered)" ;
          }
          DiagPrintf("           Type: %s",            Type) ;

          FT_SetLatencyTimer(ftHandle, FtdiLatency) ;
          DiagPrintf("        Latency: %d",            FtdiLatency) ;

          ftDeviceSerialNumber = DeviceNo ;

          if (!SetPortF(BaudRateSetting, BitsSetting, StopSetting, ParitySetting))
          {
            DiagPrintf("SetPort failed") ;
            FT_Close(ftHandle) ;
            delete[] FTList ;
            return false ;
          }
          delete[] FTList ;
          Opened = true ;
          return true ;
        }
        else
        {
          DiagPrintf("USB: Serial number <%s> not <%s>", FTList[DeviceNo].SerialNumber, DeviceSerialNumber) ;
          FT_Close(ftHandle) ;
          continue;
        }
      }
      else
      {
        DiagPrintf("USB: Unit %d Not %s, ID: 0x%04x 0x%04x", DeviceNo, ProductName, FTList[DeviceNo].ID >> 16, FTList[DeviceNo].ID & 0xffff) ;
        FT_Close(ftHandle) ;
        DiagPrintf("") ;
      }
    }
  }
  DiagPrintf("%s not found", ProductName) ;
  return false ;          // We've run out of devices!


#else
  struct ftdi_device_list *devlist;
  int devnum;

  /*-- Ensure device is closed if already opened ----------------------*/
  ++OpenCalls;
  if (Opened == true)
  {
    USBClose() ;
  }

  /*-- Initialse the FTDI Library -------------------------------------*/
  if (ftdi_init(&Ftdi) != 0)
  {
    DiagPrintf("ftdi_init failed");
    return false;
  }

  /*-- If there is no device serial just plain open the device --------*/
  if (*DeviceSerialNumber == '\0')
  {
    if ((ftStatus = ftdi_usb_open(&Ftdi, Vid, Pid)) < 0)
    {
      DiagPrintf("ftdi_usb_open %s failed: %d (%s)", ProductName, ftStatus, ftdi_get_error_string(&Ftdi));
      ftdi_deinit(&Ftdi);
      return false;
    }

  }else
  {
    /*-- Determine number of devices connected ------------------------*/
    if ((devnum = ftdi_usb_find_all(&Ftdi, &devlist, Vid, Pid)) <= 0)
    {
      DiagPrintf("ftdi_usb_find_all failed: %d (%s)", devnum, ftdi_get_error_string(&Ftdi));
      ftdi_deinit(&Ftdi);
      return false;
    }
    ftdi_list_free(&devlist);

    while (1)
    {
      if (devnum == 0)
      {
        DiagPrintf("Paylink Serial [%s] not found", DeviceSerialNumber);
        ftdi_deinit(&Ftdi);
        return false;
      }
      --devnum ;

      if ((ftStatus = ftdi_usb_open_desc_index(&Ftdi, Vid, Pid, NULL, NULL, devnum)) != 0)
      {
        DiagPrintf("ftdi_usb_open_desc_index failed: %d (%s)", ftStatus, ftdi_get_error_string(&Ftdi));
        ftdi_deinit(&Ftdi);
        return false;
      }

      /*-- Read EEPROM Contents ---------------------------------------*/
      if ((ftStatus = ftdi_read_eeprom(&Ftdi)) != 0)
      {
        DiagPrintf("ftdi_read_eeprom failed: %d (%s)", ftStatus, ftdi_get_error_string(&Ftdi));
        ftdi_usb_close(&Ftdi);
        ftdi_deinit(&Ftdi);
        return false;
      }

      /*-- Decode EEPROM ----------------------------------------------*/
      if ((ftStatus = ftdi_eeprom_decode(&Ftdi, 0)) != 0)
      {
        DiagPrintf("ftdi_eeprom_decode failed: %d (%s)", ftStatus, ftdi_get_error_string(&Ftdi));
        ftdi_usb_close(&Ftdi);
        ftdi_deinit(&Ftdi);
        return false;
      }

      DiagPrintf("EEPROM Serial: [%s]", Ftdi.eeprom->serial);

      /*-- Determine if we have located the required device -----------*/
      if (!strcmp(Ftdi.eeprom->serial, DeviceSerialNumber))
      {
        break;
      }
      ftdi_usb_close(&Ftdi);
    }
  }

  /*-- Configure the port as required ---------------------------------*/
  if (!SetPortF(BaudRateSetting, BitsSetting, StopSetting, ParitySetting))
  {
    DiagPrintf("SetPort failed") ;
    ftdi_usb_close(&Ftdi);
    ftdi_deinit(&Ftdi);
    return false ;
  }

  ftdi_set_latency_timer( &Ftdi, FtdiLatency );
  DiagPrintf("FTDI Open        : %s", ProductName );
  DiagPrintf("FTDI VID PID     : %04x %04x", Vid, Pid );
  DiagPrintf("FTDI chip type   : %d", (int)Ftdi.type );
  DiagPrintf("Timeout: read %d, write %d", Ftdi.usb_read_timeout, Ftdi.usb_write_timeout );

  Opened = true;
  return true;
#endif
}




/***********************************************************************

Send and Receive Data

***********************************************************************/
#ifndef __linux__

// Windows access routines


bool FTDIAccess::WriteBuffer(char* Address, int Length)
{
  unsigned int   ToWrite = Length ;
  char*          TxPoint = Address ;

  unsigned long  BytesWritten ;
  unsigned long  RxQueue ;
  unsigned long  InTxQueue ;
  unsigned long  EventStatus ;

  // First, make sure he chip is OK (writes can hang!)
  ftStatus = FT_GetStatus(ftHandle, &RxQueue, &InTxQueue, &EventStatus) ;
  if (!FT_SUCCESS(ftStatus))
  {
    DiagPrintf("USB WR: Get Queue Status on %s failed %d!", ProductName, ftStatus) ;
    if (!RecoverUSB())               // Sort out the FTDI chip
    {
      return false ;
    }
  }

  while (ToWrite != 0)
  {
    ftStatus = FT_Write(ftHandle, TxPoint, ToWrite, &BytesWritten) ;
    while (!FT_SUCCESS(ftStatus))
    {
      DiagPrintf("Write failed %d!", ftStatus) ;
      if (RecoverUSB())               // Sort out the FTDI chip
      {
        ftStatus = FT_Write(ftHandle, TxPoint, ToWrite, &BytesWritten) ;
      }
      else
      {
        return false ;
      }
    }

    ToWrite -= BytesWritten ;
    TxPoint += BytesWritten ;
  }
  return true ;
}


int FTDIAccess::ReadBuffer (char* Address, int Length)
{
  unsigned long CharsInQueue ;
  unsigned long RxBufferRead ;

  // check to see if there are any characters queued
  int ftStatus = FT_GetQueueStatus(ftHandle, &CharsInQueue) ;
  if (!FT_SUCCESS(ftStatus))
  {
    printf("USB RD: Get Queue Status on %s failed %d!", ProductName, ftStatus) ;
    if (RecoverUSB())               // Sort out the FTDI chip
    {
      ftStatus = FT_GetQueueStatus(ftHandle, &CharsInQueue) ;
    }
    else
    {
      printf("USB: %s Recovery Failed\n", ProductName) ;
      return -1 ;
    }
  }

  if (CharsInQueue)
  {
    if (CharsInQueue > (unsigned)Length)
    {
      CharsInQueue = Length ;
    }

    ftStatus = FT_Read(ftHandle, Address, CharsInQueue, &RxBufferRead);

    while (!FT_SUCCESS(ftStatus))
    {
      printf("USB: Read returned error status %d!\n", ftStatus) ;
      if (RecoverUSB())               // Sort out the FTDI chip
      {
        ftStatus = FT_Read(ftHandle, Address, CharsInQueue, &RxBufferRead);
      }
      else
      {
        printf("USB: Read Recovery Failed\n") ;
        return -1 ;
      }
    }
  }
  else
  {
    RxBufferRead = 0 ;
  }
  return RxBufferRead ;
}




#else


// Linux access routines

bool    FTDIAccess::WriteBuffer(char* Address, int Length)
{
  unsigned int   ToWrite = Length ;
  unsigned char* TxPoint = (unsigned char*)Address ;
  unsigned int   BytesWritten ;
  unsigned short status = 0;

  ftStatus = ftdi_poll_modem_status( &Ftdi, &status );
  if (ftStatus != 0)
  {
    DiagPrintf("USB WR: Status of %s ** FAILED ** %d!", ProductName, ftStatus) ;
    if (!RecoverUSB())
    {
        return false ;
    }
  }

  while (ToWrite != 0)
  {
    BytesWritten = ftdi_write_data(&Ftdi, TxPoint, ToWrite);
    while (BytesWritten <= 0)
    {
      DiagPrintf("Write failed %d!", BytesWritten) ;
      if (RecoverUSB())               // Sort out the FTDI chip
      {
        BytesWritten = ftdi_write_data(&Ftdi, TxPoint, ToWrite);
      }
      else
      {
        return false ;
      }
    }

    ToWrite -= BytesWritten ;
    TxPoint += BytesWritten ;
  }
  return true ;
}



int      FTDIAccess::ReadBuffer (char* Address, int Length)
{
  unsigned short status = 0;

  ftStatus = ftdi_poll_modem_status( &Ftdi, &status );
  if (ftStatus != 0)
  {
    DiagPrintf("USB WR: Status of %s ** FAILED ** %d!", ProductName, ftStatus) ;
    if (!RecoverUSB())
    {
        return -1 ;
    }
  }

  unsigned int   RxBufferRead ;
  RxBufferRead = ftdi_read_data( &Ftdi, (unsigned char*)Address,  Length);
  while (RxBufferRead < 0)
  {
    DiagPrintf("Read returned error status %d!", ftStatus) ;
    if (RecoverUSB())               // Sort out the FTDI chip
    {
      RxBufferRead = ftdi_read_data( &Ftdi, (unsigned char*)Address,  Length);
    }
    else
    {
      return -1 ;
    }
  }


  if (RxBufferRead == 0)
  {
    /*-- This is to prevent lock-ups on certain kernels (2.4 / 2.6) ---*/
    /*-- Unsure as what these kernels are doing but they seem to sit --*/
    /*-- calling this function endlessly  --------------*/

    if (LastReadTime && ((time(NULL) - LastReadTime) >= 10))
    {
      DiagPrintf("PC: %s Read timeout (%d)", ProductName, (time(NULL) - LastReadTime));
      LastReadTime = time(NULL);
      usleep(1); /*-- Let the kernel scheduler kick in  -------------*/
      RecoverUSB() ;
      usleep(1); /*-- Let the kernel scheduler kick in  -------------*/
    }
    return 0 ;
  }

  LastReadTime = time(NULL);
  return RxBufferRead ;
}
#endif












/***********************************************************************

Miscellaneous FTDI Functions

***********************************************************************/


#ifndef __linux__

// The Windows versions

void FTDIAccess::WriteCBus(int Pattern)
{
    FT_SetBitMode(ftHandle, 0xF0 | Pattern, 0x20) ;
}



void FTDIAccess::SetDtr(bool On)
{
  if (On)
  {
    FT_SetDtr(ftHandle) ;
  }

  else
  {
    FT_ClrDtr(ftHandle) ;
  }
}



int FTDIAccess::ModemStatus(void)
{
  DWORD ReadPattern ;
  if (FT_SUCCESS(FT_GetModemStatus(ftHandle, &ReadPattern)))
    {
    CurrentModemStatus = ReadPattern ;
    }
  return CurrentModemStatus ;
}

#else



// The Linux Versions

void FTDIAccess::WriteCBus(int Pattern)
{
  ftdi_set_bitmode(&Ftdi, 0xF0 | Pattern, 0x20) ;
}




void FTDIAccess::SetDtr(bool On)
{
  ftdi_setdtr(&Ftdi, On) ;
}



int FTDIAccess::ModemStatus(void)
{
  short unsigned int ReadPattern ;
  if(ftdi_poll_modem_status(&Ftdi, &ReadPattern) == 0)
    {
    CurrentModemStatus = ReadPattern ;
    }
  return CurrentModemStatus ;

}

#endif













/***********************************************************************

Open and program a new FTDI Device

***********************************************************************/
bool FTDIAccess::USBOpenNew(char* FullProductName, char* DefaultUSBSerialNumber)
{
#ifndef __linux__
  DiagPrintf("Programming Generic FTDI USB unit... ") ;

  ftStatus = FT_Open(0, &ftHandle);

  if (!FT_SUCCESS(ftStatus))
  {
    DiagPrintf("Device Open failed %d", ftStatus) ;
    return false ;
  }

  // OK - we have succeeded in opening the device!

  DiagPrintf("success") ;

  memset(&ftData, 0, sizeof ftData) ;

  // So the port is open - we unconditionally program the device
  ftData.Signature1      = 0x00000000;
  ftData.Signature2      = 0xffffffff;
  ftData.Version         = 2;

  ftData.VendorId        = Vid ;              // Standard FTDI  VID
  ftData.ProductId       = Pid ;              // Standard Project PID

  ftData.Manufacturer    = "Aardvark" ;
  ftData.ManufacturerId  = "AE" ;


  ftData.Description    = FullProductName ;
  ftData.SerialNumber   = DefaultUSBSerialNumber ;

  ftData.MaxPower        = 80 ;               // 80mA power
  ftData.PnP             = 1 ;                // 0 = disabled,    1 = enabled
  ftData.SelfPowered     = 0 ;                // 0 = powered from cable
  ftData.RemoteWakeup    = 1 ;                // changed to 1 = capable
  ftData.IFAIsFifo       = 1 ;

  // These are essentially read from a "blank" 245R

  ftData.PnP             = 0 ;
  ftData.RemoteWakeup    = 1 ;
  ftData.Rev4            = 0 ;
  ftData.SerNumEnable    = 0 ;
  ftData.USBVersionEnable= 0 ;
  ftData.USBVersion      = 0x110 ;
  ftData.EndpointSize    = 0x40 ;
  ftData.SerNumEnableR   = 0 ;
  ftData.Cbus0           = 3 ;
  ftData.Cbus1           = 2 ;
  ftData.Cbus2           = 0 ;
  ftData.Cbus3           = 1 ;
  ftData.Cbus4           = 5 ;

  ftStatus = FT_EE_Program(ftHandle, &ftData) ;
  if (ftStatus != FT_OK)
  {
    FT_Close(ftHandle) ;
    DiagPrintf("Failed to Program EEPROM") ;
    return false ;
  }
  DWORD     deviceID = 0 ;
  ftStatus = FT_GetDeviceInfo(ftHandle, &ftDevice, &deviceID, SerialNumberBuf, DescriptionBuf, NULL);
  return true ;

#else
  printf("***Linux Eprom Write, not implemented***\n") ;
  return false ;
  #if 0
  unsigned char eeprom_buff[128];
  struct ftdi_eeprom eeprom;

  /*-- Initialse the FTDI Library -------------------------------------*/
  if ((ftStatus = ftdi_init(&Ftdi)) != 0)
  {
    DiagPrintf("ftdi_init failed: %d", ftStatus);
    return false;
  }

  if ((ftStatus = ftdi_usb_open(&Ftdi, Vid, Pid)) < 0)
  {
    DiagPrintf("ftdi_usb_open %s failed: %d (%s)", ProductName, ftStatus, ftdi_get_error_string(&Ftdi));
    ftdi_deinit(&Ftdi);
    return false;
  }

  ftdi_eeprom_initdefaults (&eeprom);
  eeprom.vendor_id    = Vid ;
  eeprom.product_id   = Pid ;
  eeprom.usb_version  = 0x110;
  eeprom.manufacturer = (char *)"Aardvark" ;
  eeprom.product      = FullProductName ;
  eeprom.serial       = DefaultUSBSerialNumber ;
  eeprom.size         = 128;

  if ((ftStatus = ftdi_eeprom_build(&eeprom, eeprom_buff)) != 0)
  {
    DiagPrintf("Build eeprom failed: %d (%s)", ftStatus, ftdi_get_error_string(&Ftdi)) ;
    ftdi_usb_close(&Ftdi);
    ftdi_deinit(&Ftdi);
    return false;
  }

  if ((ftStatus = ftdi_write_eeprom(&Ftdi, eeprom_buff)) != 0)
  {
    DiagPrintf("Write eeprom failed: %d (%s)", ftStatus, ftdi_get_error_string(&Ftdi)) ;
    ftdi_usb_close(&Ftdi);
    ftdi_deinit(&Ftdi);
    return false;
  }

  ftdi_usb_close(&Ftdi);
  ftdi_deinit(&Ftdi);
  return true;
  #endif
#endif
}












/***********************************************************

The FTDI chip has gone funny!
Try to sort it out. (Note that we can spend quite a while here, as no one's going nowhere!

***********************************************************************/

bool FTDIAccess::CheckUSBNowOK(char* Attempt)
{
#ifndef __linux__
  FT_STATUS      CurrentStatus ;
  unsigned long  RxQueue ;
  unsigned long  InTxQueue ;
  unsigned long  EventStatus ;

  CurrentStatus = FT_GetStatus(ftHandle, &RxQueue, &InTxQueue, &EventStatus) ;
  DiagPrintf("USB: %s Recovery: %s %d/%d", ProductName, Attempt, ftStatus, CurrentStatus) ;
  if ((ftStatus == FT_OK) && (CurrentStatus == FT_OK))
  {
    DiagPrintf("Recovery: %d Fixed!", ProductName) ;
    return true ;
  }
  return false ;
#else
  unsigned short status = 0;

  ftStatus = ftdi_poll_modem_status( &Ftdi, &status );
  if (ftStatus != 0)
  {
    DiagPrintf("USB %s fail: %d (%s)", ProductName, ftStatus, ftdi_get_error_string(&Ftdi));
    return false;
  }
  DiagPrintf("USB %s now OK", ProductName);
  return true;
#endif
}




void FTDIAccess::RecoverDevice()
  {
  DiagPrintf("FTDI Chip recovery") ;
  // OK - lets try a device reset
#ifndef __linux__
    ftStatus = FT_ResetDevice(ftHandle);
#else
    ftStatus = ftdi_usb_reset(&Ftdi);
#endif
  }

