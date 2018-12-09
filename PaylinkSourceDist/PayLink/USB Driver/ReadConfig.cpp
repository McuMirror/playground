/***********************************************************************

Milan Configuration System

************************************************************************/


/***********************************************************************

 Note, this produces two separate data structures:

   The first is an array of ConfigurationRecord called TheConfig.
   This is a series of records which match the layout in Configuration.h
   and are suitable for downloading to an extarnal Paylink.

   The second is an array of USBSpec called TheUSBSpec.
   These records are used by the Paylink code running on a PC.

   Each record in this is a description of a USB device, and can be
   used to open a comms channel to that device by passing the index in the
   array as a parameter to a port constructor.

   The USBSpec array is divided into two parts:
   The low end, from 0 to USBConfigCount-1 contains USB devices which
   require cctalk hanlders set up for them.

   The high end contains a Paylink Lite definition at [PAYLINK_LITE]
   and Auxiliary Paylink Lites in down to PAYLINK_LITE - LiteConfigCount-1.


   With a real Paylink, the ConfigurationRecord array is loaded to the Paylink,
   where MainCode\Conmfig.cpp processes it.

   With a PC Paylink, the ConfigurationRecord array is processed by LiteConfig.cpp,
   the central part of which is (should be) identical to MainCode\Conmfig.cpp.


   Regardles of which of these is used, anything in the USBSpec array is
   processed by OpenUSBDevices in uDriver.cpp


************************************************************************/

#ifndef __linux__
    #include <vcl.h>
    #include <time.h>
    #pragma warn -aus
#else
    #include <sys/types.h>
    #include <sys/mman.h>
    #include <sys/wait.h>
    #include <stdlib.h>
    #include <string.h>
    #include <cctype>
    #include <errno.h>
    #define strnicmp strncasecmp
    #define stricmp strcasecmp
#endif
#include "DriverFuncs.h"

// -------------------------------------
// Process input config files
// -------------------------------------




// Recognised words
enum ConfigWords {
    FILE_END_FLAG,
    NO_MATCH,

    ACCEPTOR,
    ARDAC,
    AT,
    AUX,
    AZKOYEN,
    BARCODE,
    BOARD,
    BCR,
    BCS,
    BETA,
    BNV,
    BULK,
    BYTES,
    CASHLESS,
    CCTALK,
    CCNET,
    CHANGER,
    CODE,
    COIN,
    COLOURS,
    COM,
    CONNECTOR,
    CR10x,
    CRC,
    CLS,
    CX25,
    DELAY,
    DELIVERY,
    DIAG,
    DISPENSER,
    DLLWORD,
    DONGLE,
    DRIVER,
    EBDS,
    EJECT,
    ELITE,
    ESCROW,
    FAIL,
    FRONT,
    FTDI,
    F56,
    GEN2,
    GLOBAL,
    GPT,
    HIDDEN,
    HOLD,
    HOPPER,
    HOPPERS,
    ID003,
    IOBOARD,
    INPUT_STR,
    LENGTH,
    LITE,
    PI,
    HAT,
    LOGFILE,
    MAX,
    MDB,
    MECH,
    MEIBNR,
    MERKUR,
    METER,
    MFS,
    MIN,
    MONITORED,
    NONE_STR,
    NO,
    NOTE,
    NR2,
    NUMBER_STR,
    NV200,
    OPTION_STR,
    OPTIONAL_STR,
    OUTPUT,
    POOL,
    POSITION,
    POWER,
    PROBLEM,
    PROTOCOL,
    READOUT,
    REAR,
    RECYCLER,
    RECYCLE,
    REMOVAL,
    RESET,
    RETURN,
    RJ45,
    RS232,
    RS232_2,
    RUN,
    SECURITY,
    SEGMENT,
    SERIAL,
    SCALE,
    SIMULTANEOUS,
    SIZE_STR,
    SMARTHOPPER,
    SMARTTICKET,
    SPECIAL,
    SSP,
    SYSTEM,
    TFLEX,
    TIMEOUT,
    UK,
    THICKNESS,
    USB,
    USE,
    VALUE,
    VISIBLE,
    VEGA,
    VERSION,
    WACS,
    WATCHDOG,
    YES,
    ZERO
    } ;




// Decode table
static struct
    {
    char*   Text ;
    int     Value ;
    } WordTable[] = {
        { "acceptor"    , ACCEPTOR },
        { "at"          , AT },            // Noise
        { "and"         , AT },
        { "auto"        , AT },
        { "by"          , AT },
        { "from"        , AT },
        { "is"          , AT },
        { "on"          , AT },
        { "with"        , AT },
        { "aux"         , AUX },
        { "auxiliary"   , AUX },
        { "using"       , DONGLE },
        { "azkoyen"     , AZKOYEN },
        { "barcode"     , BARCODE },
        { "bcr"         , BCR },
        { "bcs"         , BCS },
        { "beta"        , BETA },
        { "bill"        , NOTE },          // Synonym
        { "bills"       , NOTE },          // Synonym
        { "board"       , BOARD },
        { "bnv"         , BNV },
        { "british"     , UK },
        { "bulk"        , BULK },
        { "byte"        , BYTES },
        { "bytes"       , BYTES },
        { "cashless"    , CASHLESS },
        { "cassette"    , HOPPER },        // Synonym
        { "cassettes"   , HOPPER },        // Synonym
        { "cctalk"      , CCTALK },
        { "ccnet"       , CCNET },
        { "changer"     , CHANGER },
        { "code"        , CODE },
        { "coin"        , COIN },
        { "coins"       , COIN },
        { "colors"      , COLOURS },
        { "colours"     , COLOURS },
        { "com"         , COM },
        { "connector"   , CONNECTOR },
        { "currency"    , NOTE },
        { "cr10x"       , CR10x },
        { "cx25"        , CX25 },
        { "system"      , SYSTEM },
        { "cls"         , CLS },
        { "crc"         , CRC },
        { "delay"       , DELAY },
        { "delivery"    , DELIVERY },
        { "diag"        , DIAG },
        { "dispenser"   , DISPENSER },
        { "dll"         , DLLWORD },
        { "dongle"      , DONGLE },
        { "driver"      , DRIVER },
        { "ebds"        , EBDS },
        { "eject"       , EJECT },
        { "elite"       , ELITE },
        { "escrow"      , ESCROW },
        { "fail"        , FAIL },
        { "f53"         , F56 },
        { "f56"         , F56 },
        { "f400"        , F56 },
        { "float"       , MAX },
        { "front"       , FRONT },
        { "ftdi"        , FTDI },
        { "gen2"        , GEN2 },
        { "global"      , GLOBAL },
        { "hat"         , HAT },
        { "hidden"      , HIDDEN  },
        { "hold"        , HOLD },
        { "hopper"      , HOPPER },
        { "hoppers"     , HOPPERS },
        { "id003"       , ID003 },
        { "ioboard"     , IOBOARD },
        { "io"          , IOBOARD },
        { "input"       , INPUT_STR },
        { "length"      , LENGTH },
        { "lite"        , LITE },
        { "logfile"     , LOGFILE },
        { "max"         , MAX },
        { "maximum"     , MAX },
        { "mdb"         , MDB },
        { "mechanical"  , MECH },
        { "meter"       , METER },
        { "meibnr"      , MEIBNR },
        { "merkur"      , MERKUR },
        { "mfs"         , MFS },
        { "min"         , MIN },
        { "monitored"   , MONITORED },
        { "none"        , NONE_STR },
        { "no"          , NO },
        { "note"        , NOTE },
        { "notes"       , NOTE },
//      { "NR2"         , NR2 },
        { "number"      , NUMBER_STR },
        { "nv11"        , VEGA },
        { "nv200"       , NV200 },
        { "option"      , OPTION_STR },
        { "optional"    , OPTIONAL_STR },
        { "output"      , OUTPUT },
        { "paylink"     , CONNECTOR },      // Synonym
        { "pi"          , CONNECTOR },
        { "pool"        , POOL },
        { "position"    , POSITION },
        { "port"        , CONNECTOR },      // Synonym
        { "power"       , POWER },
        { "printer"     , GEN2 },           // Synonym
        { "problem"     , PROBLEM },
        { "protocol"    , PROTOCOL },
        { "recycle"     , RECYCLE },
        { "recycler"    , RECYCLER },
        { "readout"     , READOUT },
        { "rear"        , REAR },
        { "removal"     , REMOVAL },
        { "remove"      , REMOVAL },
        { "reset"       , RESET },
        { "return"      , RETURN },
        { "rj45"        , RJ45 },
        { "roll"        , HOPPER },
        { "rs232"       , RS232 },
        { "rs232-1"     , RS232 },
        { "rs232-2"     , RS232_2 },
        { "rs232-3"     , RJ45 },
        { "run"         , RUN },
        { "scale"       , SCALE },
        { "security"    , SECURITY },
        { "serial"      , SERIAL },
        { "segment"     , SEGMENT },
        { "simultaneous", SIMULTANEOUS },
        { "size"        , SIZE_STR },
        { "smarthopper" , SMARTHOPPER },
        { "smartpayout" , NV200 },
        { "smartticket" , SMARTTICKET },
        { "special"     , SPECIAL },
        { "SSP"         , SSP },
        { "sterling"    , UK },
        { "thickness"   , THICKNESS },
        { "tflex"       , TFLEX },
        { "t-flex"      , TFLEX },
        { "timeout"     , TIMEOUT },
        { "uk"          , UK },
        { "usb"         , USB },
        { "use"         , USE },
        { "value"       , VALUE },
        { "version"     , VERSION },
        { "visible"     , VISIBLE },
        { "vega"        , VEGA },
        { "watchdog"    , WATCHDOG },
        { "with"        , AT },
#ifdef DOING_OLD_NOTES
        { "gpt"         , GPT },
        { "wacs"        , WACS },
        { "ardac"       , ARDAC },
#endif
        { "yes"         , YES },
        { "zero"        , ZERO }
    } ;

#define WORD_COUNT (sizeof WordTable / sizeof WordTable[0])
#define MAX_WORD 16


char* const Protname[] = {
   "System",
   "ccTalk",
   "ID003",
   "MDB",
   "GPT",
   "ARDAC/WACS",
   "F56/F53",
   "GEN2",
   "Barcode",
   "Diag",
   "CCNET",
   "Checksum"
   } ;




//--------------------------------------------
// Shared variables used to describe the input.
//--------------------------------------------
static FILE* InputFile ;
static int LineNo = 1 ;
static char CurrentWord[16] = "" ;
static bool InputOK = true ;

unsigned int ConfiguredFirmware ;
int ConfiguredLocal ;

ConfigurationRecord TheConfig[255] ;
unsigned char ProtocolPort[255] ;
short ConCount = 0 ;


// Local PC configuration
USBSpec TheUSBSpec[PAYLINK_LITE + 1] ;          // PAYLINK_LITE is the index to the last entry
                                    // This allows PAYLINK_LITE to work as a port number to the USB commhandler
short   USBConfigCount  = 0 ;
short   LiteConfigCount = 0 ;
bool    UsingDongle     = false ;
bool    UsingLite       = false  ;
bool    UsingPiHat      = false ;
bool    UsingIOBoard    = false  ;
bool    MergedInterface = false  ;

F56Config TheF56Config[16] ;
short   F56ConfigCount  = 0 ;

static bool UsingStd    = false ;
//--------------------------------------------
// Utility / Lexical Functions.
//--------------------------------------------

void ReportError(char* ErrorString)
    {
    if (InputOK)
        {
        char Message[256] ;
        sprintf(Message, ErrorString, CurrentWord) ;
        sprintf(Message + strlen(Message), " at line %d", LineNo );

        ConfigErrorReport(Message) ;
        InputOK = false ;
        }
    }




static int GetKeyWord(void)
    {
    static bool eof = false ;           // Note: this means we can only process the file once!
    if (eof)
        {
        strcpy(CurrentWord, "<End of File>") ;
        return FILE_END_FLAG ;
        }

    static bool NewLine = false ;
    if (NewLine)
        {
        NewLine = false ;
        ++LineNo ;
        }



    // find next word - skip whitespace
    int Ch = fgetc(InputFile) ;

    while (true)
        {
        if (Ch == -1)
            {
            eof = true ;
            strcpy(CurrentWord, "<End of File>") ;
            return FILE_END_FLAG ;
            }

        if (Ch == '/')
            {                               // A comment - ignore
            while (true)
                {
                Ch = fgetc(InputFile) ;
                if (Ch == '\n')
                    {
                    break ;
                    }
                else if (Ch == -1)
                    {
                    eof = true ;
                    strcpy(CurrentWord, "<End of File>") ;
                    return FILE_END_FLAG ;
                    }
                }
            }

        if (Ch > ' ')
            {
            break ;
            }

        if (Ch == '\n')
            {
            ++LineNo ;
            }

        Ch = fgetc(InputFile) ;
        }



    // OK, now get the "word"
    unsigned int  InWord = 0 ;

    while (true)
        {
        if (Ch == -1)
            {
            eof = true ;
            }

        if (Ch <= ' ')
            {
            if (Ch == '\n')
                {
                NewLine = true ;
                }
            break ;
            }

        if ((isalnum(Ch) || Ch == '-')
          && InWord < sizeof CurrentWord - 1)
            {
            CurrentWord[InWord] = Ch ;
            InWord++ ;
            }
        else
            {                           // This is rubbish input
            ReportError("Invalid characters in input");
            return NO_MATCH ;
            }
        Ch = fgetc(InputFile) ;
        }
    CurrentWord[InWord] = 0 ;


// OK now look up the "word"
    for (unsigned int i = 0 ; i < WORD_COUNT ; ++i)
        {
        if (stricmp(CurrentWord, WordTable[i].Text) == 0)
            {
            return WordTable[i].Value ;
            }
        }
    return NO_MATCH ;
    }





int AsNumber(void)
    {
    char* EndChar ;
    int Value ;
    if (tolower(CurrentWord[strlen(CurrentWord) - 1]) == 'h')
        {
        Value = strtol(CurrentWord, &EndChar, 16);
        EndChar++ ;         // Move on over the 'h'
        }
    else
        {
        Value = strtol(CurrentWord, &EndChar, 0);
        }

    // check we consumed the string
    if (*EndChar == 0)
        {
        return Value ;
        }
    else
        {
        return -1 ;
        }
    }




//----------------------------------------------------------

// The main semantic routines

//----------------------------------------------------------


//----------------------------------------------------------

// Driver

//      RUN HIDDEN | VISIBLE
//      LOGFILE <Name> [SIZE <K bytes>]
//      SERIAL [NUMBER} <Paylink Serial>

//-----------------------------------------------------------
static int DoDriver(void)
    {
    int KeyWord = GetKeyWord() ;
    int NextKeyWord ;

    while (KeyWord != FILE_END_FLAG && InputOK)
        {
        switch (KeyWord)
            {
        case RUN:
            {
            int Keyword = GetKeyWord() ;
            if (Keyword == VISIBLE)
                {
                extern bool RunVisible ;
                RunVisible = true ;
                }
            else if (Keyword == HIDDEN)
                {
                extern bool RunHidden ;
                RunHidden = true ;
                }
            else
                {
                ReportError("Expected \"HIDDEN\" or \"VISIBLE\", got \"%s\"");
                return FILE_END_FLAG ;
                }
            NextKeyWord = GetKeyWord() ;
            break ;
            }


#ifndef __linux__
        case USE:
            {
            int Keyword = GetKeyWord() ;
            if (Keyword != GLOBAL)
                {
                ReportError("Expected \"GLOBAL\" , got \"%s\"");
                return FILE_END_FLAG ;
                }
            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord == SEGMENT)
                {
                NextKeyWord = GetKeyWord() ;
                }
            extern char SharedSegmentName[] ;
            strcpy(SharedSegmentName, GLOBAL_SHARED_NAME) ;
            break ;
            }
#endif

        case LOGFILE:
            {
            // find next word - skip whitespace
            bool FindingFile = false ;
            #ifndef VCL_H
                int Index = 0 ;
            #endif


            while (true)
                {
                int Ch = fgetc(InputFile) ;
                if (Ch == EOF)
                    {
                    ReportError("Problem reading file name.");
                    return FILE_END_FLAG ;
                    }
                if (Ch == '"')
                    {
                    if (FindingFile)
                        {
                        break ;
                        }
                    FindingFile = true ;
                    continue ;
                    }

                if (FindingFile)
                    {
                    #ifdef VCL_H
                        LoggingFileName += (AnsiString)(char)Ch ;
                    #else
                        LoggingFileName[Index] = Ch ;
                        LoggingFileName[++Index] = 0 ;      // Keep terminated
                    #endif
                    }
                }


            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord == SIZE_STR)
                {
                GetKeyWord() ;
                LogFileSize = AsNumber() * 1024 ;
                if (LogFileSize < 64)
                    {
                    ReportError("Log File size \"%s\" invalid");
                    return FILE_END_FLAG ;
                    }
                NextKeyWord = GetKeyWord() ;
                }
            break ;
            }

        case SERIAL:
            {
            GetKeyWord() ;              // Actually - get a word!
            #ifdef VCL_H
              Serial = CurrentWord ;
            #else
              Serial = (char*) malloc(strlen(CurrentWord) + 1) ;
              strcpy(Serial, CurrentWord) ;
            #endif

            NextKeyWord = GetKeyWord() ;
            break ;
            }


        default:
            return KeyWord ;
            }

        KeyWord = NextKeyWord ;
        }

    return FILE_END_FLAG ;
    }




//----------------------------------------------------------

// System

//    Simultaneous Hoppers 1
//    Watchdog   [on output] 1
//    Power on   [on output] 1 Delay 100
//    Power fail [on input] 1
//    Mechanical Meter 7 [on output] 2
//    Colours

//-----------------------------------------------------------
static int DoSystem(void)
    {
    int KeyWord = GetKeyWord() ;
    int NextKeyWord ;

    while (KeyWord != FILE_END_FLAG && InputOK)
        {
        ProtocolPort[ConCount] = 0 ;        // System entries have a port of zero
        switch (KeyWord)
            {
        case AT:                            // Probably Using!
            NextKeyWord = GetKeyWord() ;
            break ;


        case CODE:
        case VERSION:
            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord == VERSION)
               {
               NextKeyWord = GetKeyWord() ;
               }

            if (NextKeyWord == AT)              // Is
               {
               NextKeyWord = GetKeyWord() ;
               }

            ConfiguredFirmware = AsNumber() ;

            if (NextKeyWord == BETA)
               {
               ConfiguredFirmware = 3 ;           // sort of says beta
               }
            else if (ConfiguredFirmware == 0)
               {
                ReportError("Expected Code version is <version>, got \"%s\"");
                return FILE_END_FLAG ;
               }
            NextKeyWord = GetKeyWord() ;
            break ;



        case SIMULTANEOUS:
            {
            if (GetKeyWord() != HOPPERS)
                {
                ReportError("Expected Hoppers, got \"%s\"");
                return FILE_END_FLAG ;
                }
            GetKeyWord() ;          // Actually get any word!
            int HopperCount = AsNumber() ;
            if (1 <= HopperCount && HopperCount <= 32)
                {
                TheConfig[ConCount].Protocol   = CON_SYSTEM ;
                TheConfig[ConCount].Unit       = CON_SIMUL_HOPPER ;
                TheConfig[ConCount].Crc        = false ;
                TheConfig[ConCount].KeyPresent = false ;
                TheConfig[ConCount].Address    = HopperCount ;
                ++ConCount ;
                }
            else
                {
                ReportError("Hopper count \"%s\" invalid");
                return FILE_END_FLAG ;
                }
            NextKeyWord = GetKeyWord() ;
            break ;
            }



        case WATCHDOG:
            {
            NextKeyWord = GetKeyWord() ;

            if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            if (NextKeyWord == OUTPUT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            int WatchdogOutput = AsNumber() ;
            if (0 <= WatchdogOutput && WatchdogOutput <= 15)
                {
                TheConfig[ConCount].Protocol   = CON_SYSTEM ;
                TheConfig[ConCount].Unit       = CON_WATCHDOG ;
                TheConfig[ConCount].Crc        = false ;
                TheConfig[ConCount].KeyPresent = false ;
                TheConfig[ConCount].Address    = WatchdogOutput ;
                ++ConCount ;
                }
            else
                {
                ReportError("Watchdog output \"%s\" invalid");
                return FILE_END_FLAG ;
                }
            NextKeyWord = GetKeyWord() ;
            break ;
            }




        case MECH:
            {
            if (GetKeyWord() != METER)
                {
                ReportError("Expected Meter, got \"%s\"");
                return FILE_END_FLAG ;
                }
            NextKeyWord = GetKeyWord() ;

            unsigned int MeterNumber = AsNumber() - 1 ;     // Zero goes to huge
            if (MeterNumber > 7)
                {
                ReportError("Meter Number \"%s\" invalid");
                return FILE_END_FLAG ;
                }
            NextKeyWord = GetKeyWord() ;

            if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            if (NextKeyWord == OUTPUT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            int MeterPin = AsNumber() ;
            if (0 <= MeterPin && MeterPin <= 15)
                {
                }
            else
                {
                ReportError("Meter output \"%s\" invalid");
                return FILE_END_FLAG ;
                }
            NextKeyWord = GetKeyWord() ;

            TheConfig[ConCount].Protocol   = CON_SYSTEM ;
            TheConfig[ConCount].Unit       = CON_MECH_METER ;
            TheConfig[ConCount].Crc        = false ;
            TheConfig[ConCount].KeyPresent = false ;
            TheConfig[ConCount].Address    = MeterNumber | (MeterPin << 3) ;
            ++ConCount ;
            break ;
            }



#ifndef __linux__
        case IOBOARD:
            {
            extern bool PCLinkUp ;
            UsingIOBoard = true ;
            PCLinkUp     = false ;          // We bring up host interface when IO Board is OK
            UsingDongle  = true ;           // We are a flavour of Dongle
            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord == BOARD)
                {
                NextKeyWord = GetKeyWord() ;
                }

           if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            if (NextKeyWord != COM)
                {
                ReportError("Expected COM, got \"%s\"");
                return FILE_END_FLAG ;
                }
            NextKeyWord = GetKeyWord() ;
            int CommsChannel = AsNumber() ;
            if (0 <= CommsChannel && CommsChannel <= 100)
                {
                }
            else
                {
                ReportError("Com Channel \"%s\" invalid");
                return FILE_END_FLAG ;
                }
            // OK, We're running an I/O Board
            TheUSBSpec[USBConfigCount].DriverType       = VCP_DRIVER ;
            TheUSBSpec[USBConfigCount].Protocol         = CON_IO_MESSAGE ;
            TheUSBSpec[USBConfigCount].VID              = 0 ;
            TheUSBSpec[USBConfigCount].PID              = 0 ;
            TheUSBSpec[USBConfigCount].BaudRate         = 38400 ;
            TheUSBSpec[USBConfigCount].CoinRecyc        = false ;
            TheUSBSpec[USBConfigCount].SerialCommNumber = CommsChannel ;
            TheUSBSpec[USBConfigCount].Name             = "IO" ;
            TheUSBSpec[USBConfigCount].FullName         = "AES IO Board" ;
            USBConfigCount++ ;
            NextKeyWord = GetKeyWord() ;

            break ;
            }
#endif




        case POWER:
            {
            int  PowerDelay = 0 ;
            int  PinType = CON_POWER_ON ;
            bool PinIsReset = false ;
            NextKeyWord = GetKeyWord() ;

            if (NextKeyWord == FAIL)
                {
                PinType = CON_POWER_FAIL ;
                NextKeyWord = GetKeyWord() ;
                }
            else if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            if ((NextKeyWord == RESET)
              &&(PinType  == CON_POWER_ON))
                {
                PinIsReset  = true ;
                NextKeyWord = GetKeyWord() ;
                }

            if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            if ((   NextKeyWord == OUTPUT
                 && PinType     == CON_POWER_ON)
             || (   NextKeyWord == INPUT_STR
                 && PinType     == CON_POWER_FAIL))
                {
                NextKeyWord = GetKeyWord() ;
                }

            int PowerPin = AsNumber() ;
            if (0 <= PowerPin && PowerPin <= 15)
                {
                }
            else
                {
                ReportError("Power pin \"%s\" invalid");
                return FILE_END_FLAG ;
                }
            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord == DELAY)
                {
                GetKeyWord() ;
                PowerDelay = AsNumber() ;
                if (1 <= PowerDelay && PowerDelay <= 60000)
                    {
                    }
                else
                    {
                    ReportError("Power delay \"%s\" invalid");
                    return FILE_END_FLAG ;
                    }
                NextKeyWord = GetKeyWord() ;
                }
            else if (PinType == CON_POWER_ON)
                {
                ReportError("\"Power on\" should be followed by \"Delay\", not \"%s\"");
                return FILE_END_FLAG ;
                }

            if (PinIsReset
              &&(PinType  == CON_POWER_ON))
                {
                PinType  = CON_POWER_RESET ;
                }

            TheConfig[ConCount].Protocol   = CON_SYSTEM ;
            TheConfig[ConCount].Unit       = PinType ;
            TheConfig[ConCount].Crc        = false ;
            TheConfig[ConCount].KeyPresent = true ;
            TheConfig[ConCount].Address    = PowerPin ;
            STORE_KEY(TheConfig[ConCount].Key, PowerDelay) ;
            ++ConCount ;
            break ;
            }



        case COLOURS:
            {
            int Colour[6] ;
            for (int i = 0 ; i < 6 ; ++i)
                {
                GetKeyWord() ;
                Colour[i] = AsNumber() ;
                if (Colour[i] < 0
                 || Colour[i] > 255)
                    {
                    ReportError("Colours must be followed by 6 numbers, not \"%s\"");
                    return -1 ;
                    }
                }
            TheConfig[ConCount].Protocol   = CON_SYSTEM ;
            TheConfig[ConCount].Unit       = CON_COLOUR_DIS ;
            TheConfig[ConCount].Crc        = false ;
            TheConfig[ConCount].KeyPresent = true ;
            TheConfig[ConCount].Address    = 0 ;
            STORE_KEY(TheConfig[ConCount].Key, (Colour[0] << 16) | (Colour[1] << 8) | (Colour[2] << 0)) ;
            ++ConCount ;

            TheConfig[ConCount].Protocol   = CON_SYSTEM ;
            TheConfig[ConCount].Unit       = CON_COLOUR_ENA ;
            TheConfig[ConCount].Crc        = false ;
            TheConfig[ConCount].KeyPresent = true ;
            TheConfig[ConCount].Address    = 0 ;
            STORE_KEY(TheConfig[ConCount].Key, (Colour[3] << 16) | (Colour[4] << 8) | (Colour[5] << 0)) ;
            ++ConCount ;

            NextKeyWord = GetKeyWord() ;
            break ;
            }



        case DONGLE:
            {                                       // This turns off Paylink driver, and turns on Dongle checking
            UsingDongle = true ;
            NextKeyWord = GetKeyWord() ;
            break ;
            }



        default:
            return KeyWord ;
            }

        KeyWord = NextKeyWord ;
        }

    return FILE_END_FLAG ;
    }





//----------------------------------------------------------

// cctalk peripherals

//    Hopper    [at]  3  [ Value 100 ] [read value] [ Timeout 10 ] [ CRC ]

//----------------------------------------------------------
static int ProcessHopper(void)
    {
    int NextKeyWord = GetKeyWord() ;
    bool Optional = false ;
    bool Crc = false ;
    bool Azkoyen = false ;
    int Value = HOPPER_NO_VALUE ;
    int Timeout = -1 ;
    int Address ;

    if (NextKeyWord == AT)
        {
        NextKeyWord = GetKeyWord() ;
        }

    Address = AsNumber() ;
    if (Address < 0 || 248 < Address)
        {
        ReportError("\"%s\" is not a valid ccTalk Address");
        return -1 ;
        }

    // Now we do the hopper options.
    NextKeyWord = GetKeyWord() ;
    while (true)
        {
        switch(NextKeyWord)
            {

         case CRC:
            Crc = true ;
            NextKeyWord = GetKeyWord() ;
            break ;


         case OPTIONAL_STR:
            Optional = true ;
            NextKeyWord = GetKeyWord() ;
            break ;


         case AZKOYEN:
            Azkoyen = true ;
            NextKeyWord = GetKeyWord() ;
            break ;


         case VALUE:
            NextKeyWord = GetKeyWord() ;
            Value = AsNumber() ;
            if (Value <= 0 || 100000 < Value)
                {
                ReportError("Hopper Value must be between 1 and 100000, not \"%s\"");
                return -1 ;
                }

            NextKeyWord = GetKeyWord() ;
            break ;


         case READOUT:
            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord != VALUE)
                {
                ReportError("Hopper Readout must be followed by \"Value\", not \"%s\"");
                return -1 ;
                }
            Value = HOPPER_READOUT_VALUE ;
            NextKeyWord = GetKeyWord() ;
            break ;


         case TIMEOUT:
            NextKeyWord = GetKeyWord() ;
            Timeout = AsNumber() ;
            if (Timeout < 1 || Timeout > 255)
                {
                ReportError("Hopper Timeout must be between 1 and 255, not \"%s\"");
                return -1 ;
                }

            NextKeyWord = GetKeyWord() ;
            break ;


       default :
            TheConfig[ConCount].Protocol   = CON_CCTALK ;
            TheConfig[ConCount].Unit       = (Azkoyen) ? CON_AZKOYEN : CON_HOPPER  ;
            TheConfig[ConCount].Crc        = Crc ;
            TheConfig[ConCount].Optional   = Optional ;
            TheConfig[ConCount].KeyPresent = true ;
            TheConfig[ConCount].Address    = Address ;
            STORE_KEY(TheConfig[ConCount].Key, Value) ;
            ++ConCount ;
            if (Timeout > 0)
                {
                TheConfig[ConCount] = TheConfig[ConCount - 1] ;
                TheConfig[ConCount].Unit = CON_HOPPER_TIMEOUT ;
                STORE_KEY(TheConfig[ConCount].Key, Timeout - 1) ;
                ++ConCount ;
                }
            return NextKeyWord ;
            }
        }
    }



//----------------------------------------------------------

// cctalk peripherals

//    Coin      [at]  2  [ BNV ] [ CRC ]
//    Bulk      [at]  2  [ BNV ] [ CRC ]
//    Hopper    ....
//    Note      [at] 40  [ BNV ] [ CRC ]
//    Bill      [at] 40  [ BNV ] [ CRC ]
//    Dispenser [at] 28H [ BNV ] [ CRC ]

//----------------------------------------------------------
static int ProcessCCTalk()
    {
    int NextKeyWord = GetKeyWord() ;
    int Device ;
    int Address ;

    while (true)
        {
        int Bnv   = 0 ;         // Note: Recyclers do not use BNV keyword, so this is also the float level!
        bool Crc = false ;
        bool Optional = false ;


        switch(NextKeyWord)
            {
         case COIN:        Device = CON_COIN       ; break ;
         case BULK:        Device = CON_BULK       ; break ;
         case NOTE:        Device = CON_NOTE       ; break ;
         case MERKUR:      Device = CON_MERKUR     ; break ;
         case VEGA:        Device = CON_VEGA       ; break ;
         case NV200:       Device = CON_NV200      ; break ;
         case CASHLESS:    Device = CON_CASHLESS   ; break ;
         case SMARTHOPPER: Device = CON_SMARTHOPPER; break ;
         default:          Device = 0              ; break ;
            }


        switch(NextKeyWord)
            {
         case COIN:
         case BULK:
         case NOTE:
         case MERKUR:
         case VEGA:
         case NV200:
         case CASHLESS:
         case SMARTHOPPER:
            NextKeyWord = GetKeyWord() ;

            if (NextKeyWord == AT)
              {
              NextKeyWord = GetKeyWord() ;
              }

            if (Device == CON_MERKUR
             || Device == CON_VEGA
             || Device == CON_NV200)
                {
                if (NextKeyWord != RECYCLER)
                    {
                    ReportError("Expected \"Recycler\", not \"%s\"");
                    return -1 ;
                    }
                NextKeyWord = GetKeyWord() ;
                }
            else if (NextKeyWord == ACCEPTOR)
                {
                if (Device == CON_SMARTHOPPER)
                    {
                    Device = CON_SMARTSYSTEM ;
                    }
                NextKeyWord = GetKeyWord() ;
                }



            if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            Address = AsNumber() ;
            if (Address < 0 || 248 < Address)
                {
                ReportError("\"%s\" is not a valid ccTalk Address");
                return -1 ;
                }


            NextKeyWord = GetKeyWord() ;
            while (NextKeyWord == BNV
                || NextKeyWord == CRC
                || NextKeyWord == OPTIONAL_STR
                || NextKeyWord == MAX)
                {
                if (NextKeyWord == BNV)
                    {
                    char* EndChar ;
                    NextKeyWord = GetKeyWord() ;
                    Bnv = strtol(CurrentWord, &EndChar, 10) ;      // Must be decimal
                    if ((Bnv <= 000000) || (999999 < Bnv))
                        {
                        ReportError("Bnv Key must be decimal digits, not \"%s\"");
                        return -1 ;
                        }
                    NextKeyWord = GetKeyWord() ;
                    }

                if (NextKeyWord == MAX)
                    {
                    if (Device != CON_MERKUR)
                        {
                        ReportError("Float level only available for Merkur");
                        return -1 ;
                        }
                    NextKeyWord = GetKeyWord() ;               // Get next keyword
                    if (NextKeyWord == NOTE)
                        {                               // (ignoring notes)
                        NextKeyWord = GetKeyWord() ;
                        }
                    Bnv = AsNumber() ;
                    if (Bnv <= 1 || 34 < Bnv)
                        {
                        ReportError("Float must be 1 - 34 not \"%s\"");
                        return -1 ;
                        }
                    NextKeyWord = GetKeyWord() ;
                    }

                if (NextKeyWord == CRC)
                    {
                    Crc = true ;
                    NextKeyWord = GetKeyWord() ;
                    }

                 if (NextKeyWord == OPTIONAL_STR)
                    {
                    Optional    = true ;
                    NextKeyWord = GetKeyWord() ;
                    }

                }

            TheConfig[ConCount].Protocol   = CON_CCTALK ;
            TheConfig[ConCount].Unit       = Device ;
            TheConfig[ConCount].Crc        = Crc ;
            TheConfig[ConCount].Optional   = Optional ;
            TheConfig[ConCount].KeyPresent = (Bnv != 0) ;
            TheConfig[ConCount].Address    = Address ;
            STORE_KEY(TheConfig[ConCount].Key, Bnv) ;
            ++ConCount ;
            break ;


         case HOPPER:
            NextKeyWord = ProcessHopper() ;
            break ;


       default :
            return NextKeyWord ;
            }
        }
    }




//----------------------------------------------------------

// local USB cctalk peripherals
//   Note Acceptor is Elite
//   Coin Recycler is CR10x
//   Coin Recycler is BCR
//   Coin Acceptor is BCS
//   Coin Acceptor is NR2


//----------------------------------------------------------
static int ProcessLocal()
    {
    int CoinRecyc = 0 ;
    bool BCRDevice = false ;

    while (true)
        {
        int Type = GetKeyWord() ;

        if (Type == MAX)
            {                                   // This is the aditional Max %d coins
            if (!BCRDevice)
                {
                ReportError("Maximum option only valid for a BCR");
                return -1 ;
                }
            GetKeyWord() ;
            int Level = AsNumber() ;
            if (Level < 1)
                {
                ReportError("Maximum must be a number, not \"%s\"");
                return -1 ;
                }
            TheUSBSpec[CoinRecyc].MaxLevel = Level ;

            Type = GetKeyWord() ;               // Get next keyword
            if (Type == COIN)
                {                               // (ignoring coins)
                Type = GetKeyWord() ;
                }
            }


        switch(Type)
            {
         case COIN:    break ;
         case NOTE:    break ;
         default:      return Type ;
            }

        int SubType = GetKeyWord() ;
        switch(SubType)
            {
         case ACCEPTOR:    break ;
         case RECYCLER:    break ;
         default:      ReportError("Unrecognised USB function \"%s\"");
                       return -1 ;
            }

       int NextKeyWord = GetKeyWord() ;

       if (NextKeyWord == AT)
            {
            NextKeyWord = GetKeyWord() ;
            }

        switch (NextKeyWord)
            {
        case ELITE:
            if (Type == NOTE && SubType == ACCEPTOR)
                {
                TheUSBSpec[USBConfigCount].DriverType       = FTDI_DRIVER ;
                TheUSBSpec[USBConfigCount].Protocol         = CON_CCTALK ;
                TheUSBSpec[USBConfigCount].VID              = MCL_VID ;
                TheUSBSpec[USBConfigCount].PID              = 0x0001 ;
                TheUSBSpec[USBConfigCount].BaudRate         = 921600 ;
                TheUSBSpec[USBConfigCount].Name             = "Elite" ;
                TheUSBSpec[USBConfigCount].FullName         = "Elite Note Acceptor" ;
                TheUSBSpec[USBConfigCount].CoinRecyc        = false ;
                USBConfigCount++ ;
                }
            else
                {
                ReportError("Wrong type for Elite");
                return -1 ;
                }
            break ;



        case BCR:
            if (Type == COIN && SubType == RECYCLER)
                {
                BCRDevice = true ;
                TheUSBSpec[USBConfigCount].DriverType       = VCP_DRIVER ;
                TheUSBSpec[USBConfigCount].Protocol         = CON_CCTALK ;
                TheUSBSpec[USBConfigCount].VID              = MCL_VID ;
                TheUSBSpec[USBConfigCount].PID              = 0x0003 ;
                TheUSBSpec[USBConfigCount].BaudRate         = 115200 ;
                TheUSBSpec[USBConfigCount].Name             = "BCR" ;
                TheUSBSpec[USBConfigCount].FullName         = "Bulk Coin Recycler" ;
                TheUSBSpec[USBConfigCount].SerialDevice     = "Silabser" ;
                TheUSBSpec[USBConfigCount].SerialCommNumber = 0 ;
                TheUSBSpec[USBConfigCount].CoinRecyc        = true ;
                CoinRecyc = USBConfigCount ;
                USBConfigCount++ ;
                }
            else
                {
                ReportError("Wrong type for BCR");
                return -1 ;
                }
            break ;


        case CR10x:
            if (Type == COIN && SubType == RECYCLER)
                {
                TheUSBSpec[USBConfigCount].DriverType       = FTDI_DRIVER ;
                TheUSBSpec[USBConfigCount].Protocol         = CON_CCTALK ;
                TheUSBSpec[USBConfigCount].VID              = MCL_VID ;
                TheUSBSpec[USBConfigCount].PID              = 0x000a ;
                TheUSBSpec[USBConfigCount].BaudRate         = 115200 ;
                TheUSBSpec[USBConfigCount].Name             = "CR10x" ;
                TheUSBSpec[USBConfigCount].FullName         = "CR10x Recycler"  ;
                TheUSBSpec[USBConfigCount].CoinRecyc        = true ;
                CoinRecyc = USBConfigCount ;
                USBConfigCount++ ;
                }
            else
                {
                ReportError("Wrong type for CR10x");
                return -1 ;
                }
            break ;



        case BCS:
            if (Type == COIN && SubType == ACCEPTOR)
                {
                TheUSBSpec[USBConfigCount].DriverType       = FTDI_DRIVER ;
                TheUSBSpec[USBConfigCount].Protocol         = CON_CCTALK ;
                TheUSBSpec[USBConfigCount].VID              = MCL_VID ;
                TheUSBSpec[USBConfigCount].PID              = 0x0006 ;
                TheUSBSpec[USBConfigCount].BaudRate         = 115200 ;
                TheUSBSpec[USBConfigCount].Name             = "BCS" ;
                TheUSBSpec[USBConfigCount].FullName         = "Bulk Coin Sorter"  ;
                TheUSBSpec[USBConfigCount].CoinRecyc        = true ;
                CoinRecyc = USBConfigCount ;
                USBConfigCount++ ;
                }
            else
                {
                ReportError("Wrong type for BCS");
                return -1 ;
                }
            break ;


        case NR2:
            if (Type == COIN && SubType == ACCEPTOR)
                {
                TheUSBSpec[USBConfigCount].DriverType       = FTDI_DRIVER ;
                TheUSBSpec[USBConfigCount].Protocol         = CON_CCTALK ;
                TheUSBSpec[USBConfigCount].VID              = MCL_VID ;
                TheUSBSpec[USBConfigCount].PID              = 0x0009 ;
                TheUSBSpec[USBConfigCount].BaudRate         = 115200 ;
                TheUSBSpec[USBConfigCount].Name             = "NR2" ;
                TheUSBSpec[USBConfigCount].FullName         = "NR2 Coin Sorter"  ;
                TheUSBSpec[USBConfigCount].CoinRecyc        = true ;
                CoinRecyc = USBConfigCount ;
                USBConfigCount++ ;
                }
            else
                {
                ReportError("Wrong type for NR2");
                return -1 ;
                }
            break ;



        default:
            ReportError("Unrecognised USB peripheral \"%s\"");
            return -1 ;
            }
        }
    }




//----------------------------------------------------------

// CCNet peripherals

//    Recycler  [at]  30
//      Escrow 5 [and Recycle 60] Bills on cassette 1
//      scale by 100
//      Eject Bill


//----------------------------------------------------------
static int ProcessCCNet(void)
    {
    int NextKeyWord = GetKeyWord() ;
    int Address ;
    int Device ;
    int OptionBytes = 0 ;
    int SecurityBytes = 0 ;
    long Setting = 2 ;                  // 0-7   (8) = Power of 10 for actual scale
                                        // 8-13  (6) = Escrow count (0 - 63)
                                        // 14-20 (7) = Recycle count (0 - 127)
                                        // 21-23 (3) = Recycl cassette

    if (NextKeyWord == RECYCLER)
        {
        Device = CON_RECYCLER ;
        }
    else if (NextKeyWord == ACCEPTOR)
        {
        Device = CON_NOTE ;
        }
    else
        {
        return NextKeyWord ;
        }

    NextKeyWord = GetKeyWord() ;

    if (NextKeyWord == AT)
        {
        NextKeyWord = GetKeyWord() ;
        }

    Address = AsNumber() ;
    if (Address < 1 || 3 < Address)
        {
        ReportError("\"%s\" is not a valid ccNet Address");
        return -1 ;
        }
    NextKeyWord = GetKeyWord() ;

    bool CCNetOption = true ;
    while (CCNetOption)
        {
        switch (NextKeyWord)
            {
        case SCALE:
            {
            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            int Scale  = AsNumber() ;
            int Decode = 1 ;

            Setting = 0 ;
            while (Decode < Scale)
                {
                ++Setting ;
                Decode *= 10 ;
                }

            if (Decode != Scale)
                {
                ReportError("Scale must be power of 10, not \"%s\"") ;
                return -1 ;
                }
            NextKeyWord = GetKeyWord() ;
            }
            break ;


        case ESCROW:
            {
            NextKeyWord = GetKeyWord() ;
            int BillCount = AsNumber() ;
            if (BillCount < 1 || BillCount > 63)
                {
                ReportError("Bill Count must be 1 - 63, not \"%s\"") ;
                return -1 ;
                }
            Setting |= BillCount << 8 ;
            NextKeyWord = GetKeyWord() ;

            if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }


            if (NextKeyWord == RECYCLE)
                {
                NextKeyWord = GetKeyWord() ;
                int BillCount = AsNumber() ;
                if (BillCount < 1 || BillCount > 127)
                    {
                    ReportError("Recycle Count must be 1 - 127, not \"%s\"") ;
                    return -1 ;
                    }
                Setting |= BillCount << 14 ;
                NextKeyWord = GetKeyWord() ;
                }


            if (NextKeyWord == NOTE)
                {
                NextKeyWord = GetKeyWord() ;
                }

            if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }
            if (NextKeyWord == HOPPER)
                {
                NextKeyWord = GetKeyWord() ;
                }
            int Cass = AsNumber() ;
            if (Cass < 1 || Cass > 3)
                {
                ReportError("Escrow cassette must be 1 - 3, not \"%s\"") ;
                return -1 ;
                }
            Setting |= Cass << 21 ;
            NextKeyWord = GetKeyWord() ;
            }
            break ;


        case EJECT:
            if (GetKeyWord() == NOTE)
                {
                TheConfig[ConCount].Crc    = true ;                 // We're re-using this
                NextKeyWord = GetKeyWord() ;
                }
            break ;


        case OPTION_STR:
            {
            int i = 0 ;
            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord != BYTES)
                {
                ReportError("Keyword Bytes expected, not \"%s\"") ;
                return -1 ;
                }
            NextKeyWord = GetKeyWord() ;

            while (AsNumber() != -1)
                {
                int Byte = AsNumber() ;
                if (Byte > 255)
                    {
                    ReportError("Byte value must be < 255, not \"%s\"") ;
                    return -1 ;
                    }
                OptionBytes |= Byte << (i * 8) ;
                ++i ;
                NextKeyWord = GetKeyWord() ;
                }

            if (i < 1 || i > 3)
                {
                ReportError("Between 1 and 3 option bytes required") ;
                return -1 ;
                }
            }
            break ;



        case SECURITY:
            {
            int i = 0 ;
            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord != BYTES)
                {
                ReportError("Keyword Bytes expected, not \"%s\"") ;
                return -1 ;
                }
            NextKeyWord = GetKeyWord() ;

            while (AsNumber() != -1)
                {
                int Byte = AsNumber() ;
                if (Byte > 255)
                    {
                    ReportError("Byte value must be < 255, not \"%s\"") ;
                    return -1 ;
                    }
                SecurityBytes |= Byte << (i * 8) ;
                ++i ;
                NextKeyWord = GetKeyWord() ;
                }

            if (i < 1 || i > 3)
                {
                ReportError("Between 1 and 3 security bytes required") ;
                return -1 ;
                }
            }
            break ;



        default:
            CCNetOption = false ;
            break ;
            }
        }


    if (NextKeyWord == OPTIONAL_STR)
        {
        TheConfig[ConCount].Optional    = true ;
        NextKeyWord = GetKeyWord() ;
        }


    TheConfig[ConCount].Protocol   = CON_CCNET ;
    TheConfig[ConCount].Unit       = Device ;
    TheConfig[ConCount].Address    = Address ;
    if (Setting == 2)
        {
        TheConfig[ConCount].KeyPresent = false ;
        }
    else
        {
        TheConfig[ConCount].KeyPresent = true ;
        STORE_KEY(TheConfig[ConCount].Key, Setting) ;
        }
    ++ConCount ;

    if (OptionBytes)
        {
        TheConfig[ConCount] = TheConfig[ConCount - 1] ;
        TheConfig[ConCount].Unit       = CON_OPTION ;
        TheConfig[ConCount].KeyPresent = true ;
        STORE_KEY(TheConfig[ConCount].Key,OptionBytes) ;
        ++ConCount ;
        }

    if (SecurityBytes)
        {
        TheConfig[ConCount] = TheConfig[ConCount - 1] ;
        TheConfig[ConCount].Unit       = CON_SECURITY ;
        TheConfig[ConCount].KeyPresent = true ;
        STORE_KEY(TheConfig[ConCount].Key,SecurityBytes) ;
        ++ConCount ;
        }

    return NextKeyWord ;
    }





static int ProcessBillCassette(int Protocol, int CommsChannel)
    {
    int NextKeyWord = GetKeyWord() ;
    int Value = -1 ;
    int Address ;
    int LengthMax = 0 ;
    int LengthMin = 0 ;
    int Thickness = 0 ;

    if (NextKeyWord == AT)
        {
        NextKeyWord = GetKeyWord() ;
        }

    Address = AsNumber() ;
    if (Address < 0 || 15 < Address)
        {
        ReportError("\"%s\" is not a valid Cassette Number");
        return -1 ;
        }

    // Now we do the hopper options.
    NextKeyWord = GetKeyWord() ;
    while (true)
        {
        switch(NextKeyWord)
            {
         case VALUE:
            NextKeyWord = GetKeyWord() ;
            Value = AsNumber() ;
            if (Value <= 0 || 1000000 < Value)
                {
                ReportError("Cassette Value must be between 1 and 1000000, not \"%s\"");
                return -1 ;
                }

            NextKeyWord = GetKeyWord() ;
            break ;


         case LENGTH:
            NextKeyWord = GetKeyWord() ;
            break ;


         case MIN:
            NextKeyWord = GetKeyWord() ;
            LengthMin = AsNumber() ;
            if (LengthMin < 50 || 187 < LengthMin)
                {
                ReportError("Min length must be between 50 and 187, not \"%s\"");
                return -1 ;
                }
            NextKeyWord = GetKeyWord() ;
            break ;


         case MAX:
            NextKeyWord = GetKeyWord() ;
            LengthMax = AsNumber() ;
            if (LengthMax < 50 || 187 < LengthMax)
                {
                ReportError("Max length must be between 50 and 187, not \"%s\"");
                return -1 ;
                }
            NextKeyWord = GetKeyWord() ;
            break ;


         case THICKNESS:
            NextKeyWord = GetKeyWord() ;
            Thickness = AsNumber() ;
            if (Thickness < 9 || 20 < Thickness)
                {
                ReportError("Thickness must be between 9 and 20, not \"%s\"");
                return -1 ;
                }
            NextKeyWord = GetKeyWord() ;
            break ;



       default :
            if (CommsChannel != 0)
                {
                TheF56Config[F56ConfigCount].Value     = Value ;
                TheF56Config[F56ConfigCount].Address   = Address ;
                TheF56Config[F56ConfigCount].LengthMax = LengthMax ;
                TheF56Config[F56ConfigCount].LengthMin = LengthMin ;
                TheF56Config[F56ConfigCount].Thickness = Thickness ;
                F56ConfigCount++ ;
                }
            else
                {
                TheConfig[ConCount].Protocol   = Protocol ;
                TheConfig[ConCount].Unit       = CON_CASSETTE ;
                TheConfig[ConCount].KeyPresent = true ;
                TheConfig[ConCount].Address    = Address ;
                STORE_KEY(TheConfig[ConCount].Key, Value) ;
                ++ConCount ;

                if (LengthMax + LengthMin + Thickness != 0)
                    {
                    TheConfig[ConCount].Protocol   = CON_F56_F53 ;
                    TheConfig[ConCount].Unit       = CON_CASSETTE_LEN ;
                    TheConfig[ConCount].KeyPresent = true ;
                    TheConfig[ConCount].Address    = Address ;
                    STORE_KEY(TheConfig[ConCount].Key, ((long)Thickness << 16) + (LengthMax << 8) + LengthMin) ;
                    ++ConCount ;
                    }
                }
            return NextKeyWord ;
            }
        }
    }



//----------------------------------------------------------

// F56 "peripherals"

//    Delivery [at] Rear | Front | None
//    Cassette 10 Value 1000 Max 160 Min 150 Thickness 13


//----------------------------------------------------------
static int ProcessF56(int CommsChannel)
    {
    int NextKeyWord = GetKeyWord() ;
    int Delivery = 0 ;
    int Special = 0 ;
    int Hold = 0 ;
    int Pool = 0 ;
    int Position = 0 ;

    if (NextKeyWord == OPTIONAL_STR)
        {
        TheConfig[ConCount].Optional = true ;
        NextKeyWord = GetKeyWord() ;
        }

    if (NextKeyWord == SPECIAL)
        {
        Special = CON_SPECIAL ;
        NextKeyWord = GetKeyWord() ;
        if (NextKeyWord == NOTE)
            {
            NextKeyWord = GetKeyWord() ;
            }
        }

    if (NextKeyWord == UK)
        {
        Special = CON_UK ;
        NextKeyWord = GetKeyWord() ;
        if (NextKeyWord == NOTE)
            {
            NextKeyWord = GetKeyWord() ;
            }
        }

    if (NextKeyWord == AT)
        {
        NextKeyWord = GetKeyWord() ;
        }

    if (NextKeyWord == POSITION)
        {
        Position    = CON_POSITION ;
        NextKeyWord = GetKeyWord() ;
        }

    if (NextKeyWord != DELIVERY)
        {
        ReportError("F56 / F53 must include \"delivery\"");
        return -1 ;
        }

    NextKeyWord = GetKeyWord() ;

    if (NextKeyWord == AT)
       {
       NextKeyWord = GetKeyWord() ;
       }

    switch (NextKeyWord)
        {
    case FRONT:
        Delivery = CON_FRONT ;
        break ;

    case REAR:
        Delivery = CON_REAR ;
        break ;

    case NONE_STR:
        Delivery = CON_NONE ;
        break ;

    default:
         ReportError("Delivery must be \"front\", \"rear\" or \"none\", not \"%s\"");
         return -1 ;
        }

    NextKeyWord = GetKeyWord() ;
    if (NextKeyWord == HOLD)
        {
        Hold = CON_HOLD ;
        NextKeyWord = GetKeyWord() ;
        if (NextKeyWord == AT)
            {
            NextKeyWord = GetKeyWord() ;
            }
        if (NextKeyWord != PROBLEM)
            {
            ReportError("Hold must be followed by \"on problem\"");
            return -1 ;
            }
        NextKeyWord = GetKeyWord() ;
        }



    if (NextKeyWord == POOL)
        {
        NextKeyWord = GetKeyWord() ;
        Pool = AsNumber() ;
        if (Pool < 1)
            {
            ReportError("Pool must specify a number, not \"%s\"");
            return -1 ;
            }
        NextKeyWord = GetKeyWord() ;
        if (NextKeyWord == NOTE)
           {
           NextKeyWord = GetKeyWord() ;
           }
        }

    if (CommsChannel != 0)
        {
        TheUSBSpec[USBConfigCount].DriverType   = VCP_DRIVER ;
        TheUSBSpec[USBConfigCount].Protocol     = CON_F56_F53 ;
        TheUSBSpec[USBConfigCount].Scale        = Delivery | Special | Hold | Position ;
        TheUSBSpec[USBConfigCount].Count        = Pool ;
        TheUSBSpec[USBConfigCount].BaudRate     = 9600 ;
        TheUSBSpec[USBConfigCount].Name         = "F56" ;
        TheUSBSpec[USBConfigCount].FullName     = "F56 Note Dispenser" ;
        TheUSBSpec[USBConfigCount].SerialDevice = 0 ;
        TheUSBSpec[USBConfigCount].SerialCommNumber = CommsChannel ;
        USBConfigCount++ ;
        }
    else
        {
        TheConfig[ConCount].Protocol = CON_F56_F53 ;
        TheConfig[ConCount].Unit     = CON_F56_DETAILS ;
        TheConfig[ConCount].Address = Delivery | Special | Hold | Position ;
        if (Pool)
            {
            TheConfig[ConCount].KeyPresent = true ;
            STORE_KEY(TheConfig[ConCount].Key, Pool) ;
            }
        ++ConCount ;
        }

    while (true)
        {
        switch(NextKeyWord)
            {
         case HOPPER:
            NextKeyWord = ProcessBillCassette(CON_F56_F53, CommsChannel) ;
            break ;


       default :
            return NextKeyWord ;
            }
        }
    }





//----------------------------------------------------------

// MFS "peripherals"

//    Cassette 10 Value 1000 Max 160 Min 150 Thickness 13


//----------------------------------------------------------
static int ProcessMFS(void)
    {
    int NextKeyWord = GetKeyWord() ;

    if (NextKeyWord == OPTIONAL_STR)
        {
        TheConfig[ConCount].Optional = true ;
        NextKeyWord = GetKeyWord() ;
        }

    while (true)
        {
        switch(NextKeyWord)
            {
         case HOPPER:
            NextKeyWord = ProcessBillCassette(CON_MFS, 0) ;
            break ;


       default :
            return NextKeyWord ;
            }
        }
    }






//----------------------------------------------------------

// IOD003 "peripherals"

//    [with] Acceptor
//    [with] Recycler

//----------------------------------------------------------
static int ProcessIDoo3(void)
    {
    int NextKeyWord = GetKeyWord() ;
    int Unit = 0 ;

    while (Unit == 0)
        {
        switch(NextKeyWord)
            {
         case RECYCLER:
            Unit = CON_RECYCLER ;
            break ;


         case ACCEPTOR:
            Unit = CON_NOTE ;
            break ;


         case NOTE:
            Unit = CON_NOTE ;
            break ;


         case AT:
            NextKeyWord = GetKeyWord() ;
            break ;


       default :
         ReportError("You must specify \"Recycler\" or \"Acceptor\", not \"%s\"");
            return NextKeyWord ;
            }
        }


    NextKeyWord = GetKeyWord() ;
    if (NextKeyWord == ACCEPTOR)                // Note Acceptor
        {
        NextKeyWord = GetKeyWord() ;
        }


    if (NextKeyWord == OPTIONAL_STR)
        {
        TheConfig[ConCount].Optional    = true ;
        NextKeyWord = GetKeyWord() ;
        }
    TheConfig[ConCount].Protocol   = CON_ID003 ;
    TheConfig[ConCount].Unit       = Unit  ;
    ++ConCount ;
    return NextKeyWord ;
    }






//----------------------------------------------------------

// EBDS "peripherals"

//    [with] Acceptor
//    [with] Recycler

//----------------------------------------------------------
static int ProcessEBDS(bool USBChannel)
    {
    int Scale     = 2 ;
    int NoteCount = 16 ;
    bool Escrow = false ;
    bool NoReturn = false ;

    int NextKeyWord = GetKeyWord() ;
    int Unit = 0 ;

    while (Unit == 0)
        {
        switch(NextKeyWord)
            {
         case RECYCLER:
            Unit = CON_RECYCLER ;
            break ;


         case ACCEPTOR:
            Unit = CON_NOTE ;
            break ;


         case AT:
            NextKeyWord = GetKeyWord() ;
            break ;


       default :
         ReportError("You must specify \"Recycler\" or \"Acceptor\", not \"%s\"");
            return NextKeyWord ;
            }
        }
    NextKeyWord = GetKeyWord() ;


    // Now check for options
    bool Processing = true ;
    while (Processing)
        {
        switch(NextKeyWord)
            {
        case SCALE:
            {
            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            int Input  = AsNumber() ;
            int Decode = 1 ;

            Scale = 0 ;
            while (Decode < Input)
                {
                ++Scale ;
                Decode *= 10 ;
                }

            if (Decode != Input)
                {
                ReportError("Scale must be power of 10, not \"%s\"") ;
                return -1 ;
                }
            NextKeyWord = GetKeyWord() ;
            break ;
            }


        case AT:
            NextKeyWord = GetKeyWord() ;
            break ;


        case ESCROW:
            Escrow = true ;
            NextKeyWord = GetKeyWord() ;
            break ;


        case MAX:
            NextKeyWord = GetKeyWord() ;               // Get next keyword
            NoteCount = AsNumber() ;
            if (NoteCount < 16 || 128 < NoteCount)
                {
                ReportError("Notes must be 16 - 128 not \"%s\"");
                return -1 ;
                }
            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord == NOTE)
                {                               // (ignoring notes)
                NextKeyWord = GetKeyWord() ;
                }
            break ;


        case NO:
            NextKeyWord = GetKeyWord() ;               // Get next keyword
            if (NextKeyWord == RETURN)
                {                               // (ignoring notes)
                NextKeyWord = GetKeyWord() ;
                NoReturn    = true ;
                }
            else
                {
                ReportError("No must be followed by Return");
                return -1 ;
                }
            break ;


        default:
            Processing = false ;
            break ;
            }
        }

    if (USBChannel)
        {
        TheUSBSpec[USBConfigCount].DriverType   = VCP_DRIVER ;
        TheUSBSpec[USBConfigCount].Protocol     = CON_EBDS ;
        TheUSBSpec[USBConfigCount].VID          = 0x0BED ;
        TheUSBSpec[USBConfigCount].PID          = 0x1100  ;
        TheUSBSpec[USBConfigCount].Scale        = Scale ;
        TheUSBSpec[USBConfigCount].Count        = NoteCount ;
        TheUSBSpec[USBConfigCount].BaudRate     = 9600 ;
        TheUSBSpec[USBConfigCount].CoinRecyc    = (Unit == CON_RECYCLER) ;
        TheUSBSpec[USBConfigCount].Name         = "EBDS" ;
        TheUSBSpec[USBConfigCount].FullName     = "EBDS Bill Device" ;
        TheUSBSpec[USBConfigCount].SerialDevice = "Silabser" ;
        TheUSBSpec[USBConfigCount].SerialCommNumber = 0 ;
        TheUSBSpec[USBConfigCount].ExtendedEscrow = Escrow ;
        TheUSBSpec[USBConfigCount].NoReturn      = NoReturn ;
        USBConfigCount++ ;
        }
    else
        {
        if ((Scale == 2 )
         && (NoteCount == 16)
         && !Escrow
         && !NoReturn)
            {
            TheConfig[ConCount].KeyPresent = false ;
            }
        else
            {
                           // 0-3   (4) = Power of 10 for actual scale - def 2
                           // 4-5   (2) = unused
                           // 6     (1) = No return flag
                           // 7     (1) = Extended Escrow flag
                           // 8-16  (8) = Note Count - def 16
            TheConfig[ConCount].KeyPresent = true ;
            if (Escrow)
              {
              Scale |= 0x80 ;
              }
            if (NoReturn)
              {
              Scale |= 0x40 ;
              }
            STORE_KEY(TheConfig[ConCount].Key, (NoteCount << 8) | Scale) ;
            }

        TheConfig[ConCount].Protocol   = CON_EBDS ;
        TheConfig[ConCount].Unit       = Unit  ;
        ++ConCount ;
        }
    return NextKeyWord ;
    }


//----------------------------------------------------------

// MDB peripherals

//    Changer  [at] 8H
//    Bill     [at] 30H
//    Note     [at] 30H
//    Cashless [at] 10H

//----------------------------------------------------------
static int ProcessMDB(void)
    {
    int NextKeyWord = GetKeyWord() ;
    int Address ;
    while (true)
        {
        switch(NextKeyWord)
            {
        case CHANGER:
            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            Address = AsNumber() ;
            if (Address < 0 || 248 < Address)
                {
                ReportError("\"%s\" is not a valid MDB Address");
                return -1 ;
                }
            NextKeyWord = GetKeyWord() ;
            TheConfig[ConCount].Protocol   = CON_MDB ;
            TheConfig[ConCount].Unit       = CON_CHANGER ;
            TheConfig[ConCount].Crc        = false ;
            TheConfig[ConCount].KeyPresent = false ;
            TheConfig[ConCount].Address    = Address ;

            if (NextKeyWord == OPTIONAL_STR)
                {
                TheConfig[ConCount].Optional    = true ;
                NextKeyWord = GetKeyWord() ;
                }

            if (NextKeyWord == ZERO)
                {                               // Special for customer - ZERO ON CASSETTE REMOVAL
                NextKeyWord = GetKeyWord() ;
                int Count = AsNumber() ;

                NextKeyWord = GetKeyWord() ;
                if (NextKeyWord == HOPPER)
                   {
                   NextKeyWord = GetKeyWord() ;
                   }

                if (NextKeyWord == AT)
                   {
                   NextKeyWord = GetKeyWord() ;
                   }

                if (NextKeyWord != REMOVAL)
                   {
                   ReportError("Expected \"REMOVAL\", not \"%s\"");
                   return -1 ;
                   }

                if (Count < 1)
                   {
                   ReportError("Expected count, not \"%s\"");
                   return -1 ;
                   }

                NextKeyWord = GetKeyWord() ;
                TheConfig[ConCount].KeyPresent = true ;
                STORE_KEY(TheConfig[ConCount].Key, Count) ;
                }


            ++ConCount ;
            break ;


         case NOTE:
            NextKeyWord = GetKeyWord() ;
            if (NextKeyWord == ACCEPTOR)
                {
                NextKeyWord = GetKeyWord() ;
                }
            if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            Address = AsNumber() ;
            if (Address < 0 || 248 < Address)
                {
                ReportError("\"%s\" is not a valid MDB Address");
                return -1 ;
                }
            NextKeyWord = GetKeyWord() ;

           if (NextKeyWord == OPTIONAL_STR)
                {
                TheConfig[ConCount].Optional    = true ;
                NextKeyWord = GetKeyWord() ;
                }
            TheConfig[ConCount].Protocol   = CON_MDB ;
            TheConfig[ConCount].Unit       = CON_NOTE ;
            TheConfig[ConCount].Crc        = false ;
            TheConfig[ConCount].KeyPresent = false ;
            TheConfig[ConCount].Address    = Address ;
            ++ConCount ;
            break ;


         case CASHLESS:
            NextKeyWord = GetKeyWord() ;

            if (NextKeyWord == AT)
                {
                NextKeyWord = GetKeyWord() ;
                }

            Address = AsNumber() ;
            if (Address < 0 || 248 < Address)
                {
                ReportError("\"%s\" is not a valid MDB Address");
                return -1 ;
                }
            NextKeyWord = GetKeyWord() ;

           if (NextKeyWord == OPTIONAL_STR)
                {
                TheConfig[ConCount].Optional    = true ;
                NextKeyWord = GetKeyWord() ;
                }
            TheConfig[ConCount].Protocol   = CON_MDB ;
            TheConfig[ConCount].Unit       = CON_CASHLESS ;
            TheConfig[ConCount].Crc        = false ;
            TheConfig[ConCount].KeyPresent = false ;
            TheConfig[ConCount].Address    = Address ;
            ++ConCount ;
            break ;


       default :
            return NextKeyWord ;
            }
        }
    }



static int ProcessSSP()
    {
    int NextKeyWord = GetKeyWord() ;
    int Device ;
    int Address ;

    while (true)
        {
        switch(NextKeyWord)
            {
         case NOTE:        Device = CON_NOTE      ; break ;
         case SMARTTICKET: Device = CON_PRINTER   ; break ;
         case NV200:       Device = CON_NV200     ; break ;
         case SMARTHOPPER: Device = CON_SMARTHOPPER ; break ;
         default:          return NextKeyWord ;
            }

        NextKeyWord = GetKeyWord() ;

        if (NextKeyWord == AT)
          {
          NextKeyWord = GetKeyWord() ;
          }

        if (Device == CON_NV200)
            {
            if (NextKeyWord != RECYCLER)
                {
                ReportError("Expected \"Recycler\", not \"%s\"");
                return -1 ;
                }
            NextKeyWord = GetKeyWord() ;
            }
        else if (NextKeyWord == ACCEPTOR)
            {
            if (Device == CON_SMARTHOPPER)
                {
                Device = CON_SMARTSYSTEM ;
                }
            NextKeyWord = GetKeyWord() ;
            }


        if (NextKeyWord == AT)
            {
            NextKeyWord = GetKeyWord() ;
            }

        Address = AsNumber() ;
        if (Address < 0 || 248 < Address)
            {
            ReportError("\"%s\" is not a valid SSP Address");
            return -1 ;
            }

        TheConfig[ConCount].Protocol   = CON_SSP ;
        TheConfig[ConCount].Unit       = Device ;
        TheConfig[ConCount].Crc        = false ;
        TheConfig[ConCount].Optional   = false ;
        TheConfig[ConCount].KeyPresent = false ;
        TheConfig[ConCount].Address    = Address ;
        ++ConCount ;


        NextKeyWord = GetKeyWord() ;
        }
    }


//----------------------------------------------------------

// Main function (top level!)

// Configuration

// Protocol [ ]
//    Port [ ]


//-----------------------------------------------------------

bool ProcessConfig(char* ConfigFile)
    {
    ConCount = 0 ;

    if (ConfigFile == 0 || ConfigFile[0] == 0)
        {
        ConfigErrorReport("The configuration file name MUST be present as a parameter.") ;
        return false ;
        }


    InputFile = fopen(ConfigFile, "r") ;
    if (InputFile == 0)
        {
        char Message[256] ;
        sprintf(Message, "Failed to open <%s>: %s", ConfigFile, strerror(errno));
        ConfigErrorReport(Message) ;
        return false ;
        }

    InputOK  = true ;

    int NextKeyWord = GetKeyWord() ;

    if (NextKeyWord == NO_MATCH)
        {
        char Message[256] ;
        sprintf(Message, "File <%s> does not appear to contain configuration", ConfigFile);
        ConfigErrorReport(Message) ;
        return false ;
        }


    while (NextKeyWord != FILE_END_FLAG && InputOK)
        {
        bool AuxFound = false ;
        switch (NextKeyWord)
            {
        case SYSTEM:
            NextKeyWord = DoSystem() ;
            break ;



        case DRIVER:
            NextKeyWord = DoDriver() ;
            break ;



        case PROTOCOL:
            char ProtocolText[16] ;
            int ProtocolKey ;
            short ConfigProtocol ;
            int PortKey ;
            int PortNumber ;
            bool USBChannel ;
            bool Monitored ;
            Monitored = false ;

            ProtocolKey = GetKeyWord() ;
            memcpy(ProtocolText, CurrentWord, sizeof ProtocolText) ;

            PortKey = GetKeyWord() ;
            if (PortKey == AT)
                {
                PortKey = GetKeyWord() ;
                }

            if (PortKey == MONITORED)
                {
                PortKey = GetKeyWord() ;
                Monitored = true ;
                MonitoringComms = true ;
                }

            if (PortKey == AUX)
                {
                AuxFound = true ;
                PortKey = GetKeyWord() ;
                }

            if (PortKey == DLLWORD)
                {
                TheUSBSpec[USBConfigCount].DriverType  = DLL_DRIVER ;
                TheUSBSpec[USBConfigCount].CoinRecyc   = false ;
                TheUSBSpec[USBConfigCount].Name        = new char[strlen(ProtocolText) + 1] ;
                strcpy(TheUSBSpec[USBConfigCount].Name, ProtocolText) ;
                TheUSBSpec[USBConfigCount].FullName    = "Paylink DLL Interface" ;
                NextKeyWord = GetKeyWord() ;
                if (NextKeyWord == AT)
                  {
                  NextKeyWord = GetKeyWord() ;
                  }
                if (NextKeyWord == ESCROW)
                  {
                  NextKeyWord = GetKeyWord() ;
                  TheUSBSpec[USBConfigCount].ExtendedEscrow = true ;
                  }
                USBConfigCount++ ;
                continue ;
                }

            if (PortKey != CONNECTOR)
                {
                ReportError("Connector/Port/Paylink/Pi keyword required, not \"%s\"");
                break ;
                }

            // Validate the port here, as it's common code!
            PortNumber = -1 ;
            USBChannel = false ;
            int CommsChannel ;
            CommsChannel = 0 ;

            switch (GetKeyWord())
                {
            case USB:
                if (USBUsed
                 && (ProtocolKey == CCTALK
                  || ProtocolKey == TFLEX
                  || ProtocolKey == EBDS
                  || ProtocolKey == CLS
                  || ProtocolKey == CX25))
                    {
                    USBChannel = true ;
                    }
                break ;

            case COM:
                if (USBUsed
                 && (ProtocolKey == F56))
                    {
                    USBChannel = true ;
                    NextKeyWord = GetKeyWord() ;
                    CommsChannel = AsNumber() ;
                    if (0 <= CommsChannel && CommsChannel <= 100)
                        {
                        }
                    else
                        {
                        ReportError("Com Channel \"%s\" invalid");
                        return FILE_END_FLAG ;
                        }
                    }
                break ;

            case LITE:
                if (USBUsed)
                    {
                    UsingLite  = true ;
                    PortNumber = PAYLINK_LITE ;
                    if (AuxFound)
                        {
                        LiteConfigCount++ ;
                        PortNumber = PAYLINK_LITE - LiteConfigCount ;
                        TheUSBSpec[PortNumber].PID      = DONGLE_PID ;
                        TheUSBSpec[PortNumber].Name     = "Aux" ;
                        TheUSBSpec[PortNumber].FullName = "Auxiliary Lite" ;
                        }
                    else
                        {
                        // Set up the Paylink Lite device as the PAYLINK_LITE USB Device
                        TheUSBSpec[PortNumber].PID      = USB_PID ;
                        TheUSBSpec[PortNumber].Name     = "Lite" ;
                        TheUSBSpec[PortNumber].FullName = "Paylink Lite 2" ;
                        }
                    TheUSBSpec[PortNumber].DriverType = FTDI_DRIVER ;
                    TheUSBSpec[PortNumber].Protocol   = 0 ;
                    TheUSBSpec[PortNumber].VID        = USB_VID ;
                    TheUSBSpec[PortNumber].BaudRate   = 9600 ;
                    TheUSBSpec[PortNumber].LiteUnit   = true ;
                    TheUSBSpec[PortNumber].Monitored  = Monitored ;
                    }
                break ;
            case HAT :
              if(USBUsed)
              {
                printf("HAT USB Used\n");
                //Run the drivers locally.
                UsingLite = false;
                UsingPiHat = true;
                UsingStd = false;
                PortNumber = 0;
                TheUSBSpec[USBConfigCount].DriverType       = VCP_DRIVER ;
                TheUSBSpec[USBConfigCount].Protocol         = 0 ;
                TheUSBSpec[USBConfigCount].VID              = 0 ;
                TheUSBSpec[USBConfigCount].PID              = 0 ;
                TheUSBSpec[USBConfigCount].BaudRate         = 9600 ;
                TheUSBSpec[USBConfigCount].Name             = "PIHAT" ;
                TheUSBSpec[USBConfigCount].FullName         = "Pi Hat" ;
                TheUSBSpec[USBConfigCount].SerialDevice     = "Pi Hat" ;
                TheUSBSpec[USBConfigCount].SerialCommNumber = 0 ;
                USBConfigCount++ ;
              }

              break;
            case FTDI:
                if (USBUsed)
                    {
                    UsingLite  = true ;
                    PortNumber = PAYLINK_LITE ;
                    // Set up the FTDI device as the PAYLINK_LITE USB Device
                    TheUSBSpec[PortNumber].PID      = 0x6001 ;
                    TheUSBSpec[PortNumber].Name     = "FTDI" ;
                    TheUSBSpec[PortNumber].FullName = "FTDI Converter" ;

                    TheUSBSpec[PortNumber].DriverType = FTDI_DRIVER ;
                    TheUSBSpec[PortNumber].Protocol   = 0 ;
                    TheUSBSpec[PortNumber].VID        = USB_VID ;
                    TheUSBSpec[PortNumber].BaudRate   = 9600 ;
                    TheUSBSpec[PortNumber].LiteUnit   = true ;
                    TheUSBSpec[PortNumber].Monitored  = Monitored ;
                    }
                break ;

            case CCTALK:
                PortNumber = CCTALK_CHANNEL ;
                UsingStd = true ;
                break ;

            case GEN2:
            case RS232:
                PortNumber = GENOA_RS232_CHANNEL ;
                UsingStd = true ;
                break ;

            case RS232_2:
                PortNumber = P2015_RS232_CHANNEL ;
                UsingStd = true ;
                break ;

            case ARDAC:
            case RJ45:
                PortNumber = MAIN_RS232_CHANNEL ;
                UsingStd = true ;
                break ;

            case MDB:
                PortNumber = MDB_CHANNEL ;
                UsingStd = true ;
                break ;

            case NO_MATCH:
                // Maybe they used the number from the lid
                switch (AsNumber())
                    {
                case 1:
                    PortNumber = MDB_CHANNEL ;
                    UsingStd = true ;
                    break ;

                case 8:
                    PortNumber = CCTALK_CHANNEL ;
                    UsingStd = true ;
                    break ;

                case 9:
                    PortNumber = MAIN_RS232_CHANNEL ;
                    UsingStd = true ;
                    break ;

                case 11:
                    PortNumber = GENOA_RS232_CHANNEL ;
                    UsingStd = true ;
                    break ;
                    }
                break ;
                }


            if (PortNumber == -1 && !USBChannel)
                {
                ReportError("Unknown port \"%s\"");
                break ;
                }


            memset(ProtocolPort + ConCount, PortNumber, 255 - ConCount) ;                  // Set port for items from here on

            switch(ProtocolKey)
                {
#ifdef DOING_OLD_NOTES
            case ARDAC:
            case WACS:
                ConfigProtocol = CON_ARDAC_WACS ;
                NextKeyWord = GetKeyWord() ;
                break ;

            case GPT:
                ConfigProtocol = CON_GPT ;
                NextKeyWord = GetKeyWord() ;
                break ;
#endif
            case DIAG:
                ConfigProtocol = CON_DIAG ;
                NextKeyWord = GetKeyWord() ;
                break ;


            case BARCODE:
                ConfigProtocol = CON_BARCODE ;
                NextKeyWord = GetKeyWord() ;
                break ;

            case CCTALK:
                ConfigProtocol = CON_CCTALK ;
                if (USBChannel)
                    {
                    NextKeyWord = ProcessLocal() ;
                    }
                else
                    {
                    NextKeyWord = ProcessCCTalk() ;
                    }
                break ;

            case CCNET:
                ConfigProtocol = CON_CCNET ;
                NextKeyWord = ProcessCCNet() ;
                break ;

            case F56:
                ConfigProtocol = CON_F56_F53 ;
                NextKeyWord = ProcessF56(CommsChannel) ;
                break ;

            case MFS:
                ConfigProtocol = CON_MFS ;
                NextKeyWord = ProcessMFS() ;
                break ;

            case GEN2:
                ConfigProtocol = CON_GEN2 ;
                NextKeyWord = GetKeyWord() ;
                break ;

            case EBDS:
                ConfigProtocol = CON_EBDS ;
                NextKeyWord = ProcessEBDS(USBChannel) ;
                break ;

            case SSP:
                ConfigProtocol = CON_SSP ;
                NextKeyWord = ProcessSSP() ;
                break ;

            case TFLEX:
            case CX25:
                ConfigProtocol = CON_TFLEX ;
                if (USBChannel)
                    {
                    TheUSBSpec[USBConfigCount].DriverType       = HID_DRIVER ;
                    TheUSBSpec[USBConfigCount].Protocol         = CON_TFLEX ;
                    TheUSBSpec[USBConfigCount].VID              = 0x14d4 ;
                    TheUSBSpec[USBConfigCount].Report           = 1 ;
                    TheUSBSpec[USBConfigCount].BaudRate         = 0 ;
                    TheUSBSpec[USBConfigCount].CoinRecyc        = false ;
                    if (ProtocolKey == CX25)
                        {
                        TheUSBSpec[USBConfigCount].PID          =  0x0100  ;
                        TheUSBSpec[USBConfigCount].Name         = "CX25" ;
                        TheUSBSpec[USBConfigCount].FullName     = "CX25x Coin Dispenser" ;
                        }
                    else
                        {
                        TheUSBSpec[USBConfigCount].PID          =  0x0000 ;
                        TheUSBSpec[USBConfigCount].Name         = "TFlex" ;
                        TheUSBSpec[USBConfigCount].FullName     = "T-Flex Coin Dispenser" ;
                        }
                    USBConfigCount++ ;
                    }
                NextKeyWord = GetKeyWord() ;
                break ;


            case CLS:
                ConfigProtocol = CON_CLS ;
                if (USBChannel)
                    {
                    TheUSBSpec[USBConfigCount].DriverType   = HID_DRIVER ;
                    TheUSBSpec[USBConfigCount].Protocol     = CON_CLS ;
                    TheUSBSpec[USBConfigCount].VID          = 0x14d4 ;
                    TheUSBSpec[USBConfigCount].PID          = 0x0500 ;
                    TheUSBSpec[USBConfigCount].Report       = 1 ;
                    TheUSBSpec[USBConfigCount].BaudRate     = 0 ;
                    TheUSBSpec[USBConfigCount].CoinRecyc    = false ;
                    TheUSBSpec[USBConfigCount].Name         = "CLS" ;
                    TheUSBSpec[USBConfigCount].FullName     = "CLS Coin Handler" ;
                    USBConfigCount++ ;
                    }
                NextKeyWord = GetKeyWord() ;
                break ;


            case MDB:
                ConfigProtocol = CON_MDB ;
                NextKeyWord = ProcessMDB() ;
                break ;

            case ID003:
                ConfigProtocol = CON_ID003 ;
                NextKeyWord = ProcessIDoo3() ;
                break ;

            default:
                ConfigProtocol = -1 ;
                memcpy(CurrentWord, ProtocolText, sizeof CurrentWord) ;         // For Report Error!
                ReportError("Unknown protocol \"%s\"") ;
                InputOK = false ;
                break ;
                }

           if (NextKeyWord == OPTIONAL_STR)
                {
                TheConfig[ConCount].Optional    = true ;
                NextKeyWord = GetKeyWord() ;
                }


            if (!USBChannel)
                {
                TheConfig[ConCount].Protocol   = ConfigProtocol ;
                TheConfig[ConCount].Unit       = CON_PROTOCOL_PORT ;
                TheConfig[ConCount].Crc        = false ;
                TheConfig[ConCount].KeyPresent = false ;
                TheConfig[ConCount].Address    = PortNumber ;
                ++ConCount ;
                }

            break ;

        default:
            ReportError("Unrecognised / Invalid input \"%s\"");
            break ;
            }

        }
    fclose(InputFile) ;


    // Sort into standard (ascending) order (for config to match on H8)
    bool NotSorted ;
    do
        {
        NotSorted = false ;
        for (int i = 1 ; i < ConCount ; ++i)
            {
            bool Swap = false ;
            // Sort everything for a particular port together
            if (ProtocolPort[i] < ProtocolPort[i - 1])
                {
                Swap = true ;
                }
            else if (ProtocolPort[i] == ProtocolPort[i - 1])
                {
                // Then the protocol field  may contain things that need to go first
                if (TheConfig[i].Protocol < TheConfig[i - 1].Protocol)
                    {
                    Swap = true ;
                    }
                else if (TheConfig[i].Protocol == TheConfig[i - 1].Protocol)
                    {
                    // Then the port definition itself has a unit of zero and needs to go first
                    if (TheConfig[i].Unit < TheConfig[i - 1].Unit)
                        {
                        Swap = true ;
                        }
                    else if (TheConfig[i].Unit == TheConfig[i - 1].Unit)
                        {
                        // and I'm not sure if this helps !!!!!!
                        if (TheConfig[i].Address < TheConfig[i - 1].Address)
                            {
                            Swap = true ;
                            }
                        else if (TheConfig[i].Address == TheConfig[i - 1].Address)
                            {
                            char Message[256] ;
                            sprintf(Message, "Duplicate unit for %s at Address %d",
                                          Protname[TheConfig[i].Protocol],
                                          TheConfig[i].Address);
                            ConfigErrorReport(Message) ;

                            InputOK = false ;
                            }
                        }
                    }
                }

            if (Swap)
                {
                ConfigurationRecord Temp = TheConfig   [i - 1] ;
                unsigned char      PTemp = ProtocolPort[i - 1] ;
                TheConfig   [i - 1] = TheConfig   [i] ;
                ProtocolPort[i - 1] = ProtocolPort[i] ;
                TheConfig   [i]     = Temp ;
                ProtocolPort[i]     = PTemp ;
                NotSorted = true ;
                }
            }
        } while (NotSorted) ;


    // Now store the checksum record !
    int Check = 0 ;
    for (int i = 0 ; i < ConCount ; ++i)
        {
        Check += RecordToAddress(TheConfig[i]) ;
        Check += RecordToValue  (TheConfig[i]) ;
        }

    TheConfig[ConCount].Protocol   = CON_CHECKSUM ;
    TheConfig[ConCount].Unit       = 0 ;
    TheConfig[ConCount].Crc        = false ;
    TheConfig[ConCount].KeyPresent = true ;
    TheConfig[ConCount].Address    = 0 ;
    STORE_KEY(TheConfig[ConCount].Key, Check) ;
    ++ConCount ;


    if (UsingStd && (UsingLite || UsingPiHat))
        {                                           // Either use Standard *or* Lite
        ConfigErrorReport("Cannot use Paylink Lite with standard Paylink\n") ;
        return false ;
        }

    // Finally, set the global Merged interface flag if relevant.
    MergedInterface = (USBConfigCount > 0 && !UsingDongle) ;

    return InputOK ;
    }
