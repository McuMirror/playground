#ifndef _CONFIGURATION_H
#define _CONFIGURATION_H

/***************************************************************

Definitions for the Configuration data use by the:
Aardvark Embedded Solutions Intelligent Money Handling Equipment Interface

There are three sections of data in a Configuration record:
Control - 16 bits
Address - 8 bits
Value   - 24 bits

Every Configuration "record" has Control and Address - a flag in the Control specifies
if there is a subsequent Value.


Control is:
    Protocol            5
    Unit                5
    Optional            1
    CRC                 1
    Key / Value Present 1



USB_CONFIG Packets are used to send Configuration records. Framing is done with:
    USB_CONFIG_START    Start Configuration data             (PC=>USB)
    USB_CONFIG_END      End Configuration data               (PC=>USB)
    USB_CONFIG_UPDATE   Configuration good - H8 restarting   (USB=>PC)
    USB_CONFIG_NAK      Configuration bad - rejected         (USB=>PC)
    USB_CONFIG_MATCH    Configuration matches stored version (USB=>PC)


****************************************************************/


// -----------------------------------
// The main Configuration record
// -----------------------------------
typedef struct
   {
   unsigned char  Protocol ;                // Note that due to USB packing this only holds 5 bits
   unsigned char  KeyPresent : 1 ;
   unsigned char  Crc        : 1 ;
   unsigned char  Optional   : 1 ;
   unsigned char  Unit       : 5 ;
   unsigned char  Address ;
   unsigned char  Key[3] ;
   } ConfigurationRecord ;



enum ConfigProtocols
   {
// The Protocols understood by the system
   CON_PROTOCOL_WIDTH = 5 ,     // Actual bits used in the protocol field

   CON_SYSTEM = 0,  // System Configuration (See units)
   CON_CCTALK,
   CON_ID003,
   CON_MDB,
   CON_GPT,
   CON_ARDAC_WACS,
   CON_F56_F53,
   CON_GEN2,
   CON_BARCODE,
   CON_DIAG,
   CON_CCNET,
   CON_MFS,
   CON_DLL,
   CON_TFLEX,
   CON_EBDS,
   CON_SSP,
   CON_IO_MESSAGE,
   CON_CLS,
   CON_CHECKSUM = 31,    // Key is checksum of LS 24 bits of Addresses and Values

// The Units understood by the system
   CON_UNIT_WIDTH = 5 ,         // Actual bits used in the Unit field

   CON_PROTOCOL_PORT = 0,       // Address contains a port value (see below)
   CON_COIN,
   CON_NOTE,
   CON_HOPPER,                  // Key contains Value
   CON_HOPPER_TIMEOUT,          // This is a 2nd record - Key contains Period
   CON_AZKOYEN,                 // Key contains Value
   CON_RECYCLER,
   CON_VEGA,                    // This also icludes NV11 (differentiated at run time)
   CON_BULK,                    // Bulk coin (needs motor control)
   CON_NV200A,                  // Duplicated in some drivers because of cock up !!!
   CON_NV200,
   CON_SMARTHOPPER,
   CON_SMARTSYSTEM,
   CON_PRINTER,
   CON_CASHLESS,
   CON_SECURITY = (1 << CON_UNIT_WIDTH) - 2, // Sorts to the end of the definitions
   CON_OPTION   = (1 << CON_UNIT_WIDTH) - 1, // Sorts to the end of the definitions


   // Specials / overlapping the above
   CON_CHANGER  = CON_COIN,
   CON_CASSETTE,
   CON_CASSETTE_LEN,
   CON_F56_DETAILS,

   CON_MERKUR = CON_RECYCLER,

// System "Units"
   CON_WATCHDOG = 0,            // Address is Output No.
   CON_SIMUL_HOPPER,            // Address is Count
   CON_POWER_ON,                // Address is Output No., Value is delay period
   CON_POWER_FAIL,              // Address is Input No.
   CON_MECH_METER,              // Address is Output No., value is meter number
   CON_COLOUR_DIS,
   CON_COLOUR_ENA,
   CON_POWER_RESET,             // Address is Output No., Value is delay period

// Channel Values
   CCTALK_CHANNEL      = 0,
   MILAN_HI2_CHANNEL   = 1,
   GENOA_RS232_CHANNEL = 1,
   MAIN_RS232_CHANNEL  = 2,
   MDB_CHANNEL         = 3,
   P2015_RS232_CHANNEL = 4,

   PAYLINK_LITE        = 127,   // Pseudo channel for Paylink Lite driver

   HOPPER_NO_VALUE =      -1,
   HOPPER_READOUT_VALUE = -2,

// F56 Address Values
   CON_NONE     =   0,
   CON_FRONT    =   1,
   CON_REAR     =   2,
   CON_DELIVERY = 0x3,                      // Delivery Mask
   CON_SPECIAL  = 0x4,                      // Special currency bit mask
   CON_HOLD     = 0x8,                      // Hold on problem bit mask
   CON_UK       = 0x10,                     // UK currency bit
   CON_POSITION = 0x20,                     // Cassette by position bit
   CON_POOL_SHIFT = 8                       // Shift for byte value
   } ;






// Constants for packing records into a USB frame
enum
   {
   CON_PROTOCOL_SHIFT = 0,
   CON_UNIT_SHIFT     = CON_PROTOCOL_WIDTH ,
   CON_CRC_SHIFT      = CON_PROTOCOL_WIDTH + CON_UNIT_WIDTH,
   CON_KEY_PRES_SHIFT = CON_PROTOCOL_WIDTH + CON_UNIT_WIDTH + 1,
   CON_OPT_SHIFT      = CON_PROTOCOL_WIDTH + CON_UNIT_WIDTH + 2,
   } ;



#ifdef __cplusplus

inline short RecordToAddress(ConfigurationRecord Record)
  {
  return ((short)Record.Protocol   << CON_PROTOCOL_SHIFT) |
         ((short)Record.Unit       << CON_UNIT_SHIFT)     |
         ((short)Record.Optional   << CON_OPT_SHIFT)      |
         ((short)Record.Crc        << CON_CRC_SHIFT)      |
         ((short)Record.KeyPresent << CON_KEY_PRES_SHIFT) ;
  }

inline AESLong RecordToValue(ConfigurationRecord Record)
  {
  if (Record.KeyPresent)
    {
    return ((AESLong)Record.Address << 0)  |
           ((AESLong)Record.Key[0]  << 8)  |
           ((AESLong)Record.Key[1]  << 16) |
           ((AESLong)Record.Key[2]  << 24) ;
    }
  else
    {
    return (AESLong)Record.Address ;
    }
  }

inline ConfigurationRecord PacketToRecord(short Address, AESLong Value)
  {
  ConfigurationRecord Record ;
  Record.Protocol   = (Address >> CON_PROTOCOL_SHIFT) & ((1 << CON_PROTOCOL_WIDTH) - 1) ;
  Record.Unit       = (Address >> CON_UNIT_SHIFT)     & ((1 << CON_UNIT_WIDTH)     - 1) ;
  Record.Optional   = Address >> CON_OPT_SHIFT ;
  Record.Crc        = Address >> CON_CRC_SHIFT ;
  Record.KeyPresent = Address >> CON_KEY_PRES_SHIFT ;
  Record.Address    = Value >> 0 ;
  Record.Key[0]     = Value >> 8 ;
  Record.Key[1]     = Value >> 16 ;
  Record.Key[2]     = Value >> 24 ;
  return Record ;
  }

#endif

#define STORE_KEY(Key, Value) \
    Key[0] = (Value) ; \
    Key[1] = (Value) >> 8 ; \
    Key[2] = (Value) >> 16 ;

#define GET_KEY(Key) \
           (((AESLong)(Key)[0]  << 0) | \
            ((AESLong)(Key)[1]  << 8) | \
            ((AESLong)(Key)[2]  << 16))

#endif
