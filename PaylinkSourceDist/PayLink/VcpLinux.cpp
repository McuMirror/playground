/******************************************************

    This is the PC based Linux VCP (/dev/USB0) Driver

*******************************************************/
#ifndef __linux__
 ---- This file is only for Linux
#endif
#include <stdio.h>   /* Standard input/output definitions */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <stdarg.h>
#include <time.h>
#include <dirent.h>
#include "win_types.h"
#include "VcpAccess.h"


/***********************************************************************

Constructor

***********************************************************************/
VCPAccess::VCPAccess(char* Name, int VID, int PID, int CommsNumber, char* SerialDevice, int BaudRate) :
            USBAccess(Name, VID, PID)
    {
    Type               = VCP ;
    this->BaudRate     = BaudRate ;
    this->CommsNumber  = CommsNumber ;
    this->SerialDevice = SerialDevice ;
    IORunning          = 0 ;
    m_Errors           = 0 ;
    m_LastError        = 0 ;
    BufferAddress      = 0 ;
    Recovery           = NONE ;

    m_Read             = 0 ;
    m_Written          = 0 ;
    m_SetCount         = 0 ;

    BaudRateSetting = BaudRate ;
    BitsSetting     = 8 ;
    StopSetting     = 1 ;
    ParitySetting   = 0 ;
    }



static void PrintError(char* Format, long LastError)
{
    DiagPrintf(Format, strerror(LastError)) ;
}



void VCPAccess::CheckError(char* Type)
    {
    if (m_LastError != errno)
        {
        m_LastError = errno ;
        DiagPrintf("VCP: %s Error %d", Type, m_LastError) ;
        PrintError(" - <%s>", m_LastError) ;
        }

    if (Opened && ++m_Errors > 5)
        {
        DiagPrintf("VCP: Total Communications Failure") ;
        USBClose() ;
        }
    }





/***********************************************************************

Close the VCP Device driver

***********************************************************************/
void VCPAccess::USBClose(void)
  {
  if (Opened)
     {
     try
         {
         close(m_Handle) ;
         DiagPrintf("VCP: Closing %s", m_Name) ;
         }
     catch (...)
         {
         DiagPrintf("VCP: Closed %s", m_Name) ;
         }
     }
  Opened = false ;
  }




VCPAccess::~VCPAccess()
{
  USBClose() ;
}


bool VCPAccess::CommonUSBOpen()
    {
    m_Handle   = -1 ;
    m_Read     = 0 ;
    m_Written  = 0 ;
    m_SetCount = 0 ;

    char* IDMatch = 0 ;
    // We *should* be able to find it from PID / VID - but this works!
    if ((Vid == 0x106f)
     && (Pid == 0x0003))
        {
        IDMatch = "Money_Controls_Ltd_Bulk_Coin_Recycler" ;
        }
    else if ((Vid == 0x0BED)
     &&      (Pid == 0x1100))
        {
        IDMatch = "Cashflow-SCR_Bill_Recycler" ;
        }
    else if(Vid == 0x0 && Pid == 0x0)
        {
        IDMatch = "PiHat";
        }
    else
        {
        DiagPrintf("VCP: Can't process %04x:%04x", Vid, Pid) ;
        return false ;
        }

    // OK - lets look for something in /dev/Serial
    DIR *dp;
    struct dirent *ep;
    char Path[2048] ;
    Path[0] = 0 ;

    if(strcmp(IDMatch, "PiHat") == 0)
      {
      DiagPrintf("VCP::PiHat::Opening /dev/ttyAMA0\n");

      m_Handle = open("/dev/ttyAMA0", O_RDWR | O_NOCTTY | O_NDELAY);
      if(m_Handle < 0)
        {
        m_LastError = errno;
        DiagPrintf("VcpLinux.cpp::CommonUSBOpen::Error %d Opening /dev/ttyAMA0\n", m_LastError);
        PrintError("VcpLinux.cpp::CommonUSBOpen::Error <%s> \n" , m_LastError);
        return false;
        }
      }
    else
      {
      dp = opendir("/dev/serial/by-id");
      if (dp != NULL)
        {
        while ((ep = readdir(dp)))
            {
            if (strstr(ep->d_name, IDMatch))
                {
                sprintf(Path, "/dev/serial/by-id/%s", ep->d_name) ;
                }
            }
        closedir(dp);
        if (Path[0])
            {
            DiagPrintf("VCP: Opening <%s>", Path) ;

            m_Handle = open(Path, O_RDWR | O_NOCTTY | O_NDELAY) ;
            if (m_Handle < 0)
                {
                m_LastError = errno ;
                DiagPrintf("Error %d Opening %s", m_LastError, Path) ;
                PrintError("VCP: Error <%s> ", m_LastError) ;
                return false ;
                }
            }
        else
            {
            DiagPrintf("VCP: Cannot find device in /dev/serial/by-id matching \"%s\"", IDMatch);
            return false ;
            }
        }
      else
        {
        DiagPrintf("VCP: No /dev/serial/by-id directory, attempting /dev/ttyUSB<n>");
        for (int i = 0 ; i < 99 ; ++i)
            {
            sprintf(Path, "/dev/ttyUSB%d", i) ;
            m_Handle = open(Path, O_RDWR | O_NOCTTY | O_NDELAY);
            if (m_Handle >= 0)
                {                   // So it's open - is it a real port
                struct termios options;
                if (tcgetattr(m_Handle, &options) >= 0)
                    {                // Yes - got it
                    DiagPrintf("VCP: Opened %s as it is a free serial port", Path);
                    break ;
                    }
                close(m_Handle) ;
                }
            }
        if (m_Handle < 0)
            {
            DiagPrintf("VCP: no free / operational /dev/ttyUSB<n>");
            return false ;
            }
        }
      }

    fcntl(m_Handle, F_SETFL, FNDELAY);
    if(!SetPortW(BaudRateSetting, BitsSetting, StopSetting, ParitySetting))
      {
      close(m_Handle) ;
      return false ;
      }
    if (tcflush(m_Handle, TCIOFLUSH) < 0)
      {
      close(m_Handle) ;
      DiagPrintf("VCP: %s is not serial\n", m_Name) ;
      m_LastError = errno ;
      return false ;
      }

    Opened = true ;
    return true ;
    }




/***********************************************************************

Send and Receive Data

***********************************************************************/
bool VCPAccess::WriteBuffer(char* Address, int Length)
{
  char*           TxPoint = Address ;
  unsigned long   ToWrite = Length ;
  unsigned long BytesWritten = 0 ;


  struct termios options;
  // The write will not comment if the device is disconnected, but this *will* fail
  if (tcgetattr(m_Handle, &options) < 0)
  {
    CheckError("Send") ;
    return false ;
  }

  m_Written ++ ;                           // Account for data sent
  if (m_Written > 8)
  {
    if (m_Read < 3)
    {
      DiagPrintf("VCP: Checking line setting as low reply rate") ;
      if ((++m_SetCount > 3)
       || !SetPortW(BaudRateSetting, BitsSetting, StopSetting, ParitySetting))
        {
          close(m_Handle) ;
          return false ;
        }
    }
  m_Written = 0 ;
  m_Read = 0 ;
  }


//  Recovery      = WRITE ;
  while (ToWrite > 0)
  {
    BytesWritten = write(m_Handle, TxPoint, ToWrite) ;
    if (BytesWritten < 1)
    {
        CheckError("Send") ;
        return false ;
    }
  ToWrite -= BytesWritten ;
  TxPoint += BytesWritten ;
  }
return true ;
}


int VCPAccess::ReadBuffer (char* Address, int Length)
{
    if (!Opened)
    {
      return  -1 ;
    }
    long DataRead = read(m_Handle, Address, Length) ;
    if (DataRead < 0)
    {
        DataRead = 0 ;  // no bytes read !!
        if (errno != EAGAIN)
        {
            CheckError("Read") ;
        }
    }
    else
    {
        m_Errors = 0 ;
    }

  if (DataRead > 0)
    {
    m_Read++ ;
    }
  return DataRead ;
}


/*  **********************************************************

Comms port reconfiguration

***********************************************************************/

bool VCPAccess::SetPortW(int Baud, int Bits, int Stop, int Parity)
    {
    BaudRateSetting = Baud ;
    BitsSetting     = Bits ;
    StopSetting     = Stop ;
    ParitySetting   = Parity ;

    struct termios options;

    // Get the current options for the port...
    tcgetattr(m_Handle, &options);

    cfmakeraw(&options) ;
    options.c_cc[VTIME] = 0 ;
    options.c_cc[VMIN] = 0 ;

    int BaudCode = 0 ;
    switch (Baud)
        {
    case 4800:    BaudCode = B4800 ;   break ;
    case 9600:    BaudCode = B9600 ;   break ;
    case 38400:   BaudCode = B38400 ;  break ;
    case 115200:  BaudCode = B115200 ; break ;
    //case 921600:  BaudCode = B921600 ; break ;
        }

    // Set the baud rate
    cfsetispeed(&options, BaudCode);
    if (cfsetospeed(&options, BaudCode) != 0)
        {
        PrintError("Error %s setting terminal speed", errno) ;
        return false ;
        }

    // Enable the receiver and set local mode...
    options.c_cflag |= CLOCAL | CREAD ;

    // Turn off all processing
    options.c_lflag &= ~(ECHOKE
                       | ECHOE
                       | ECHOK
                       | ECHO
                       | ECHONL
                       | ECHOPRT
                       | ECHOCTL
                       | ISIG
                       | ICANON
                       | IEXTEN ) ;


    options.c_oflag &= ~OPOST ;   // When the OPOST option is disabled, all other option bits in c_oflag are ignored.

    // Get raw input
    options.c_iflag = IGNPAR ;

    if (Bits == 8)
        {
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;    /* Select 8 data bits */
        }
    else if (Bits == 7)
        {
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS7 ;    /* Select 7 data bits */
        options.c_iflag |= ISTRIP ; /* strip 8th bit off chars */
        }
    else
        {
        DiagPrintf("\n\nVCP: ******** Only does 7 or 8 bits *****\n") ;
        }

    if (Stop == 1)
        {
        options.c_cflag &= ~CSTOPB ;
        }
    else
        {
        DiagPrintf("\n\nVCP: ******** Only does 1 stop bit *****\n") ;
        }

    if (Parity == 0)
        {
        options.c_cflag &= ~PARENB ;
        }
    else if (Parity == 1)
        {
        options.c_cflag |= PARENB ;
        options.c_cflag &= ~PARODD ;
        }
    else
        {
        DiagPrintf("\n\nVCP: ******** Only does No or Even Parity *****\n") ;
        }

    // Set the new options for the port...
    if (tcsetattr(m_Handle, TCSANOW, &options) != 0)
        {
        PrintError("Error %s setting terminal", errno) ;
        return false ;
        }

    return true ;
    }

/*  **********************************************************

Is everything OK now! - called by RecoverUSB

***********************************************************************/

bool VCPAccess::CheckUSBNowOK(char* Attempt)
{
  if (Recovery == WRITE)
  {
    return true ;             // It's now working (worked!)
  }
  else if (Recovery == OPEN)
  {
    return true ;             // It's now working (worked!)
  }
  return true ;
}


