/***********************************************************************

Milan Configuration System

************************************************************************/

#ifndef PC_CONFIG_H
#define PC_CONFIG_H

// -------------------------------------
// Process input config files
// -------------------------------------

bool ProcessConfig(char* ConfigFile) ;

void ConfigErrorReport(char* Message) ;

//
//  The data structure used to hold Paylink configuration data
//
extern ConfigurationRecord TheConfig[255] ;
extern short ConCount ;


//
//  The data structure used to store USB devices locally
//

class Port ;

enum {
  FTDI_DRIVER = 1,     // Random number
  VCP_DRIVER,
  HID_DRIVER,
  DLL_DRIVER,
  COM_DRIVER
  } ;

typedef struct
    {
    int       Protocol ;
    bool      CoinRecyc ;
    bool      LiteUnit ;
    bool      Monitored ;
    int       ExtendedEscrow ;
    int       NoReturn ;
    int       VID ;
    int       PID ;
    int       BaudRate ;
    int       Report ;          // For HID devices
    char*     Name ;
    char*     FullName ;
    int       DriverType ;

    char*     SerialDevice ;
    int       SerialCommNumber ;
    int       MaxLevel ;
    int       Scale ;
    int       Count ;

    Port*     ThePort ;
    } USBSpec ;

extern USBSpec TheUSBSpec[] ;
extern short USBConfigCount ;
extern short LiteConfigCount ;

typedef struct
    {
    int Value ;
    int Address ;
    int LengthMax ;
    int LengthMin ;
    int Thickness ;
    } F56Config ;
extern F56Config TheF56Config[] ;
extern short F56ConfigCount ;


extern unsigned int ConfiguredFirmware ;

extern bool UsingDongle ;
extern bool MergedInterface ;
extern bool UsingLite ;
extern bool UsingPiHat;
extern bool UsingIOBoard ;
extern bool USBUsed ;


#define MCL_VID 0x106f       // MCL

#endif

