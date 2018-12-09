#ifndef __IMHEIINTERNAL_H
#define __IMHEIINTERNAL_H

enum { BUILD_NUMBER = 526 } ;                            // This *should* change whenever there is a noticeable change in Paylink

/***************************************************************

Definitions for the interface provided by the:
Aardvark Embedded Solutions Intelligent Money Handling Equipment Interface


****************************************************************/

/****************************************************************
The USB interface is based around a 32bit item.

There is no platform independant fundamental that is *always* 32 bits.

Therefore we use the type AESLong - which has to have been typedef ed as
a 32 bit integer - these lines check that has been done.
****************************************************************/
// Check that AESLong is exactly 4 bytes - if it is not one of these will generate a compile fault
extern AESLong CheckAESLOngSize[sizeof (AESLong) - 3] ;
extern AESLong CheckAESLongSize2[5 - sizeof (AESLong)] ;





/****************************************************************
General Stuff which distinguishes this from other USB Projects
****************************************************************/
#define APPLICATION_MEANING_VALUE 1               // Remeber, this value MUST be unique for each USB project.

#define USB_VID                   0x0403          // Standard FTDI  VID
#define USB_PID                   0xde50          // Standard Genoa PID
#define USB_HID                   0xde5f          // Genoa PID for HID
#define PRODUCT_NAME              "Genoa"
#define FULL_PRODUCT_NAME         "Genoa USB Hub"
#define BAUD_RATE                 9600            // Not relevant for 245 chip

#define DEFAULT_USB_SERIAL_NO     "AE000001"

#define LIGHT_CHECKSUM            -2
#define DONGLE_VID                   0x0403          // Standard FTDI  VID
#define DONGLE_PID                   0xde52          // Dongle PID





/****************************************************************
Stuff for for the USB shared memory on the PC
****************************************************************/
#ifdef __linux__
    #define SHARED_NAME "/AES"      // Load name for file-mapping object.
#else
    #define GLOBAL_SHARED_NAME "Global\\AESIMHEI mapped file"      // Load name for file-mapping object, in global namespace.
    #define LOCAL_SHARED_NAME  "AESIMHEI mapped file"
#endif

      ///////////////////////////////////////////////////////////////////////////
      // Driver code on the PC wants to report interface statuses
      // This function is provided in case that code is repurposed to run
      // in a different environment
      ///////////////////////////////////////////////////////////////////////////
void ReportInterface (InterfaceDevice* DeviceInfo) ;
void CurrentInterface(InterfaceDevice* DeviceInfo) ;

enum {
    BASIC_SHARED_SIZE    = 8 * 1024L,
    BUFFER_SIZE          = 32 * 1024L,
    EXTENDED_SHARED_SIZE = BASIC_SHARED_SIZE + BUFFER_SIZE + BASIC_SHARED_SIZE + 32 * 1024L
    } ;

#if PROCESSOR==ATMEGA
    #define POINTER_INT short             // The integer size of a pointer
    #define BUFFER_SIZE 1024 // The Atmega cannot handle the very idea of a 32K array - this overrides the enum!
#else
    #define POINTER_INT long               // The integer size of a pointer
#endif


typedef struct
    {
             AESLong         FlagWord ;          // == AARDVARK_FLAG_WORD
             AESLong         USBUsage ;
    volatile AESLong         DiagWriteIndex ;    // The index of the next character to be written to the buffer
             AESLong         DiagIndexMask ;     // The mask applied to the above to access that character in Buffer[]
             char            DiagBuffer[BUFFER_SIZE] ;
             AESLong         InternalVersion ;   // To allow for below and more expansion
             AESLong         FlagWord2 ;         // == AARDVARK_FLAG_WORD (to show above is valid)
             InterfaceDevice DeviceInfo ;        // An actual embedded structure
    } PCInternalBlock ;

#define CURRENT_INTERNAL_VERSION 0


#define PC_INTERNAL_BLOCK(Base)   ((PCInternalBlock*)(((char*)(Base)) + BASIC_SHARED_SIZE))

#ifdef __cplusplus
    extern "C" {
#endif

extern PCInternalBlock* PCInternal ;

#ifdef __cplusplus
    }

/****************************************************************************
In C++ we define ALL the data as belonging to access classes.
This allows us to write code that looks as though it is accessing normal variables
while the overloaded operators worry about mapping the data to the shared memory.

Baiscally all references are "captured" by the overloaded operators, which are given the
"address" of the item - which they then process in order to get the actual data!

The "address" that the compiler / programmer manipulates are offsets from the start
of the shared memory block - they don't correspond to normally accessible memory

Due to the facilities available in C++ the containing memory block is a defined as
a completely normal struct, with the individual items being the access classes. It would
be nicer to have overloaded the -> operator somehow, but we can't do that.

Note: that although the unary & operator is overloaded, and works, it appears to be impossible
to tell the compiler to use it for the address of an arrary (Struct->Arrary). To get the
overloaded operator we have to explicitly use the & (&Struct->Array[0]).
***************************************************************************/

class InterfaceLong                // The data item in the shared interface area.
    {
private:
                          InterfaceLong() ;   // Private constructor - no-one can call it
    volatile AESLong      SpaceForTheValue ;  // This is "virtual" and never referenced!!
                                          // It exists to make the class occupy the correct space
protected:
    AESLong               ReadInterfaceLong (void) const ;
    void                  WriteInterfaceLong(AESLong Value) ;
    bool                  CompareInterfaceLong(AESLong Value) const ;
public:
    const InterfaceLong&  operator= (const InterfaceLong& OtherOne) ;
//    InterfaceLong*        operator& () const ;
    AESLong*              operator& () const ;
    const InterfaceLong&  operator+=(const AESLong Increment) ;
    const InterfaceLong&  operator++() ;
    const InterfaceLong&  operator++(int) ;
    const InterfaceLong&  operator--() ;
    const InterfaceLong&  operator--(int) ;

                       /* this is simply the basic WriteInterfaceLong */
    inline const InterfaceLong&  operator= (const AESLong Value) {WriteInterfaceLong(Value);
                                                                           return *this;}
                       /* this is simply the basic ReadInterfaceLong */
         inline           operator AESLong()             const {return ReadInterfaceLong();}
                       /* these are all flavours of the basic CompareInterfaceLong */
    bool inline  operator==(const unsigned long Value) const {return  CompareInterfaceLong(Value);}
    bool inline  operator==(const long          Value) const {return  CompareInterfaceLong(Value);}
    bool inline  operator==(const unsigned int  Value) const {return  CompareInterfaceLong(Value);}
    bool inline  operator==(const int           Value) const {return  CompareInterfaceLong(Value);}
    bool inline  operator!=(const unsigned long Value) const {return !CompareInterfaceLong(Value);}
    bool inline  operator!=(const long          Value) const {return !CompareInterfaceLong(Value);}
    bool inline  operator!=(const unsigned int  Value) const {return !CompareInterfaceLong(Value);}
    bool inline  operator!=(const int           Value) const {return !CompareInterfaceLong(Value);}

    void                  CreateCopy(void) ;       /* This is used where field that is not in the output area */
                                                   /* is to be entered into the USB cache so it can be preset. */
    AESLong inline        Offset(void)                 const {return  (AESLong)(POINTER_INT)this;}
    } ;


/****************************************************************************
Still in C++ we define a descendant of the above class for each of our
structures, which return appropriately typed pointers to our structs
***************************************************************************/
class InterfaceString : public InterfaceLong    // A String within interface memory
    {
public:
    void StringCopy(unsigned char* Data, int Length) ;
    } ;

class pInterfaceString : public InterfaceLong       // A Pointer offset within shared memory
    {
public:                                      // Allow this "Pointer" to point to this struct
    operator char*()     const ;
    inline pInterfaceString& operator= (InterfaceString* Value)
        {
        WriteInterfaceLong((AESLong)(POINTER_INT)Value) ;
        return *this ;
        }
    } ;


struct AcceptorStruct ;
class pAcceptorInterface : public InterfaceLong       // A Pointer offset within shared memory
    {
public:                                      // Allow this "Pointer" to point to this struct
    inline operator AcceptorStruct*()     const
        {
        return (AcceptorStruct*)(POINTER_INT)ReadInterfaceLong() ;
        }
    inline pAcceptorInterface& operator= (AcceptorStruct* Value)
        {
        WriteInterfaceLong((AESLong)(POINTER_INT)Value) ;
        return *this ;
        }
    } ;


struct AcceptorCoinStruct ;
class pAcceptorCoinInterface : public InterfaceLong       // Pointer offset within shared memory
    {
public:                                       // Allow this "Pointer" to point to this struct
    inline operator AcceptorCoinStruct*() const
        {
        return (AcceptorCoinStruct*)(POINTER_INT)ReadInterfaceLong()  ;
        }
    inline pAcceptorCoinInterface& operator= (AcceptorCoinStruct* Value)
        {
        WriteInterfaceLong((AESLong)(POINTER_INT)Value) ;
        return *this ;
        }
    } ;


struct DispenserStruct ;
class pDispenserInterface : public InterfaceLong       // Pointer offset within shared memory
    {
public:                                       // Allow this "Pointer" to point to this struct
    inline operator DispenserStruct*()    const
        {
        return (DispenserStruct*)(POINTER_INT)ReadInterfaceLong()  ;
        }
    inline pDispenserInterface& operator= (DispenserStruct* Value)
        {
        WriteInterfaceLong((AESLong)(POINTER_INT)Value) ;
        return *this ;
        }
    } ;


struct EscrowNoteStruct ;
class pEscrowNoteInterface : public InterfaceLong       // Pointer offset within shared memory
    {
public:                                       // Allow this "Pointer" to point to this struct
    inline operator EscrowNoteStruct*()    const
        {
        return (EscrowNoteStruct*)(POINTER_INT)ReadInterfaceLong()  ;
        }
    inline pEscrowNoteInterface& operator= (EscrowNoteStruct* Value)
        {
        WriteInterfaceLong((AESLong)(POINTER_INT)Value) ;
        return *this ;
        }
    } ;


struct EscrowStruct ;
class pEscrowInterface : public InterfaceLong       // Pointer offset within shared memory
    {
public:                                       // Allow this "Pointer" to point to this struct
    inline operator EscrowStruct*()    const
        {
        return (EscrowStruct*)(POINTER_INT)ReadInterfaceLong()  ;
        }
    inline pEscrowInterface& operator= (EscrowStruct* Value)
        {
        WriteInterfaceLong((AESLong)(POINTER_INT)Value) ;
        return *this ;
        }
    } ;


struct OutputAreaStruct ;
class pOutputAreaBlock : public InterfaceLong       // Pointer offset within shared memory
    {
public:                                       // Allow this "Pointer" to point to this struct
    inline operator OutputAreaStruct*()   const
        {
        return (OutputAreaStruct*)(POINTER_INT)ReadInterfaceLong() ;
        }
    inline pOutputAreaBlock& operator= (OutputAreaStruct* Value)
        {
        WriteInterfaceLong((AESLong)(POINTER_INT)Value) ;
        return *this ;
        }
    } ;

struct InputAreaStruct ;
class pInputAreaBlock : public InterfaceLong       // Pointer offset within shared memory
    {
public:                                       // Allow this "Pointer" to point to this struct
    inline operator InputAreaStruct*()    const
        {
        return (InputAreaStruct*)(POINTER_INT)ReadInterfaceLong() ;
        }
    inline pInputAreaBlock& operator= (InputAreaStruct* Value)
        {
        WriteInterfaceLong((AESLong)(POINTER_INT)Value) ;
        return *this ;
        }
    } ;


struct CashlessStruct ;
class pCashlessAreaBlock : public InterfaceLong       // Pointer offset within shared memory
    {
public:                                       // Allow this "Pointer" to point to this struct
    inline operator CashlessStruct*()    const
        {
        return (CashlessStruct*)(POINTER_INT)ReadInterfaceLong() ;
        }
    inline pCashlessAreaBlock& operator= (CashlessStruct* Value)
        {
        WriteInterfaceLong((AESLong)(POINTER_INT)Value) ;
        return *this ;
        }
    } ;


struct LoggedEventStruct ;
class pEventAreaBlock : public InterfaceLong       // Pointer offset within shared memory
    {
public:                                       // Allow this "Pointer" to point to this struct
    inline operator LoggedEventStruct*()    const
        {
        return (LoggedEventStruct*)(POINTER_INT)ReadInterfaceLong() ;
        }
    inline pEventAreaBlock& operator= (LoggedEventStruct* Value)
        {
        WriteInterfaceLong((AESLong)(POINTER_INT)Value) ;
        return *this ;
        }
    } ;


struct DiagOutputStruct ;
class pDiagOutputBlock : public InterfaceLong       // Pointer offset within shared memory
    {
public:                                       // Allow this "Pointer" to point to this struct
    inline operator DiagOutputStruct*()   const
        {
        return (DiagOutputStruct*)(POINTER_INT)ReadInterfaceLong() ;
        }
    inline pDiagOutputBlock& operator= (DiagOutputStruct* Value)
        {
        WriteInterfaceLong((AESLong)(POINTER_INT)Value) ;
        return *this ;
        }
    } ;

#else
/************************************************************************
In normal C on the PC we "know" the pointers are really funny integers, and
we can use the macro SHARED_ADDRESS to convert them to pointers into dual
port memory.

Note that on the H8 this 'C' representation is useless as the data is not
accessible excpet via access routines.
***********************************************************************/
    typedef AESLong InterfaceLong ;    // Pointer offset within shared memory
    typedef AESLong pAcceptorInterface ;
    typedef AESLong pDispenserInterface ;
    typedef AESLong pEscrowNoteInterface ;
    typedef AESLong pEscrowInterface ;
    typedef AESLong pAcceptorCoinInterface ;
    typedef AESLong pOutputAreaBlock ;
    typedef AESLong pInputAreaBlock ;
    typedef AESLong pCashlessAreaBlock ;
    typedef AESLong pEventAreaBlock ;
    typedef AESLong pDiagOutputBlock ;
    typedef AESLong InterfaceString ;
    typedef AESLong pInterfaceString ;
    #define SHARED_ADDRESS(Ptr)  ((void *)((char *)SharedMemoryBase + Ptr))
#endif



/************************************************************************
Now on to the definitions of the data areas themselves!
***********************************************************************/


enum InterfaceDefinitions
    {
#define AARDVARK_FLAG_WORD 0xaa8d4a8c
    TRANSFER_LONGS     = 80,
    TRANSFER_BYTES     = (TRANSFER_LONGS * 4),
    NO_OF_SWITCHES     = 48,
    NO_OF_LEDS         = 48,
    NO_OF_COUNTERS     = 32,
    BARCODE_DIGITS     = 20,
    BARCODE_LONGS      = (BARCODE_DIGITS / 4),

    BARCODE_NEW_DIGITS = 40,
    BARCODE_EXT_LONGS  = ((BARCODE_NEW_DIGITS - BARCODE_DIGITS) / 4)
    } ;



enum {
    SELFTEST_NOT_RUN            = 0 ,
    SELFTEST_OK                 = 1 ,
    SELFTEST_MEMORY_FAIL        = 2 ,
    SELFTEST_LEDSWITCH_FAIL     = 4 ,
    SELFTEST_MDB_FAIL           = 8 ,
    SELFTEST_CCTALK_FAIL        = 16 ,
    SELFTEST_HII_FAIL           = 32 ,
    SELFTEST_RS232_FAIL         = 32 ,
    SELFTEST_SSP_FAIL           = 64 ,
    SELFTEST_E2PROM_FAIL        = 128 ,
    SELFTEST_POWER_FAIL         = 256 ,
    SELFTEST_RS232_1_FAIL       = 256 ,
    SELFTEST_CHECKSUM_FAIL      = 512
    } ;


enum CriticalVersions {
    INITIAL_FIELDS   = 2,
    METER_FIELDS,
    EEPROM_FIELDS,
    ESCROW_FIELDS,
    DISPENSER_FIELDS,
    STRING_FIELDS,
    ESCROW_DATA,
    PRECISE_DISPENSE
    }  ;



typedef struct BasicControlStruct
    {
    InterfaceLong      FlagWord ;        // == AARDVARK_FLAG_WORD
    InterfaceLong      MeaningVersion ;
    InterfaceLong      FieldVersion ;
    InterfaceLong      CodeVersion ;
    pInputAreaBlock    InputPointer ;
    pOutputAreaBlock   OutputPointer ;
    InterfaceLong      CardSelfTest ;
    pDiagOutputBlock   DiagOutPointer ;
    InterfaceLong      AppCount ;        // No Longer used
    InterfaceLong      ElectronicSerialNumber ;
    InterfaceLong      MustBeZero ;
    InterfaceLong      RandomNumber ;
    InterfaceLong      CompileDate[4] ;
    InterfaceLong      CompileTime[4] ;
    InterfaceLong      DESSecure ;      // Maybe could be in meaning version - but this was spare.
    pCashlessAreaBlock CashlessPointer ;
    pEventAreaBlock    LoggedEventPointer ;
    InterfaceLong      BuildNumber ;
    InterfaceLong      Reserved[40] ;
    } BasicControlBlock ;




typedef struct InputAreaStruct
    {
    InterfaceLong      StartGuard ;                    // 100
    InterfaceLong      EndGuard ;                      // 104
    InterfaceLong      InterfaceError ;                // 108  Set if basic problem with interface
    InterfaceLong      TransferSequence ;              // 10c
    InterfaceLong      TransferBlock[TRANSFER_LONGS] ; // 110
    InterfaceLong      SwitchClosed[NO_OF_SWITCHES] ;  // 250
    InterfaceLong      DESLocked ;                     // 310  Current Status from Paylink - botom bit is boolean status
    InterfaceLong      DESReplyToPC[2] ;               // 314  PCChallenge *decrypted* by Paylink, which can be checked by the PC
    InterfaceLong      DESPaylinkChallenge[2] ;        // 31C  Set by Paylink, *encrypted* by PC into the reply, which is be checked by Paylink
                                                          // Note, we decrypt one way and encrypt the other so that
                                                          //       the baddy can't just copy the Paylink challenge
                                                          //        in into the PC challenge out.
    InterfaceLong      DESPaylinkChallengeCount ;      // 324  Incremented to tell PC to process challenge
    InterfaceLong      DESPaylinkResponseCount ;       // 328  Set == to PCChallengeCount when challenge processed
    InterfaceLong      IOCount ;                       // 32C
    InterfaceLong      PlatformType ;                  // 330
    InterfaceLong      Spare1[7] ;                     // 334
    InterfaceLong      SwitchOpened[NO_OF_SWITCHES] ;  // 350
    InterfaceLong      Spare2[16] ;
    InterfaceLong      ReadValue ;                     // 450
    InterfaceLong      PaidValue ;                     // 454
    InterfaceLong      PayoutStatus ;                  // 458  see Aesimhei.h
    InterfaceLong      AvailableValue ;                // 45c
    InterfaceLong      OverallStatus ;                 // 460  Or of all unit statuses
    pAcceptorInterface   Acceptors ;                   // 464
    pDispenserInterface  Dispensers ;                  // 468
    InterfaceLong        MeterStatus ;                 // 46C
    InterfaceLong        MeterSerialNo ;               // 470
    InterfaceLong        EscrowValueRead ;             // 474
    InterfaceLong        EventsWritten ;               // 478
    InterfaceLong        TheEvent ;                    // 47C
    InterfaceLong        BarcodeInEscrow[BARCODE_LONGS] ;
    InterfaceLong        BarcodesReadToEscrow ;
    InterfaceLong        BarcodeTicketsStacked ;
    InterfaceLong        BarcodeStacked[BARCODE_LONGS] ;
    InterfaceLong        BarcodePrinterStatus ;
    InterfaceLong        ReturnedSequence ;
    InterfaceLong        BarcodeInEscrowExt[BARCODE_EXT_LONGS] ;
    InterfaceLong        BarcodeStackedExt[BARCODE_EXT_LONGS] ;
    pEscrowInterface     EscrowBlock ;
    }  InputAreaBlock ;



typedef struct OutputAreaStruct
    {
    InterfaceLong          StartGuard ;
    InterfaceLong          EndGuard ;
    InterfaceLong          PCPresent ;           // Set to (exactly) 1 to show PC interface loaded
    InterfaceLong          TransferSequence ;
    InterfaceLong          TransferBlock[TRANSFER_LONGS] ;
    InterfaceLong          PeripheralsEnabled ;
    InterfaceLong          Output[NO_OF_LEDS] ;
    InterfaceLong          DESLocked ;                          // Requested by PC when greater than Input area - botom bit is boolean status
    InterfaceLong          NewDESKey[2] ;                       // Contains the new key being set by PC (must be all even bytes)
    InterfaceLong          DESPCChallenge[2] ;                  // Set by PC, *decrypted* by Paylink into the reply, which can be checked by the PC
    InterfaceLong          DESReplyToPaylink[2] ;               // PaylinkChallenge *encrypted* by PC which is be checked by Paylink
                                                                // Note, we decrypt one way and encrypt the other so that
                                                                      //       the baddy can't just copy the Paylink challenge
                                                                      //        in into the PC challenge out.
    InterfaceLong          DESPCChallengeCount ;                // Incremented to tell Paylink to process challenge
    InterfaceLong          DESPCResponseCount ;                 // Set == to PaylinkChallengeCount when challenge processed
    InterfaceLong          EnableInterfaceCount ;
    InterfaceLong          PWMMaximum ;          // This contain one less than PWMMaximum - so that zero is the default 1
    InterfaceLong          Spare[4] ;
    InterfaceLong          PrecisePayFlag ;
    InterfaceLong          PayValue ;
    InterfaceLong          ValueNeeded ;
    InterfaceLong          PeripheralUpdates ;
    InterfaceLong          CounterValue[NO_OF_COUNTERS] ;
    InterfaceLong          CounterCaption[NO_OF_COUNTERS][3] ;  // 3 words / 12 chars per counter
    InterfaceLong          MeterDisplayCode ;
    InterfaceLong          EpromControl ;        // 8 x 4 bit fields, one per Data Block
    InterfaceLong          EpromData[8][8] ;
    InterfaceLong          EscrowEnabled ;
    InterfaceLong          EscrowValueAccepted ;
    InterfaceLong          EscrowValueReturned ;
    InterfaceLong          BarcodeEnabled ;
    InterfaceLong          BarcodesAccepted ;
    InterfaceLong          BarcodesReturned ;
    InterfaceLong          EventsRead ;
    InterfaceLong          PayValueRandomCheck ;
    InterfaceLong          RunningCount ;        // Used by the DLL internally
    InterfaceLong          InputSequence ;
    InterfaceLong          TimeoutPeriod ;
    }  OutputAreaBlock ;



// As at interface 1.1 these are almost identical to those presented to the application
// This may however change.


typedef struct AcceptorCoinStruct
    {
    pAcceptorCoinInterface Next ;              // Next block in chain
    InterfaceLong          Flags ;             // Flags from Application
    InterfaceLong          Value ;             // Value of this coin
    InterfaceLong          Inhibit ;           // Set by PC: "this coin inhibited"
    InterfaceLong          Count ;             // Total number read "ever"
    InterfaceLong          Path ;              // Set to specify the coin specific output path
    InterfaceLong          PathCount ;         // Number "ever" sent down the specific Path
    InterfaceLong          PathSwitchLevel ;   // PathCount level to switch coin to default path
    InterfaceLong          DefaultPath ;       // Default path for this specific coin
    InterfaceLong          HeldInEscrow ;      // count of this note / coin in escrow (usually max 1)
    InterfaceLong          CurrencySet ;       // Currency set to which this coin belongs
    pInterfaceString       CoinName ;          // Pointer to null terminated string in I/Face memory
    } AcceptorCoinInterface ;



typedef struct AcceptorStruct
    {
    pAcceptorInterface     Next ;              // Next block in chain
    InterfaceLong          Flags ;             // Flags from Application
    InterfaceLong          Unit ;              // Specification of this unit
    InterfaceLong          Status ;            // AcceptorStatuses - zero if device OK
                                                        // Must be same as Dispenser
    InterfaceLong          NoOfCoins ;         // The number of different coins handled
    InterfaceLong          Interface ;         // The bus / connection
    InterfaceLong          UnitAddress ;       // For addressable units
    InterfaceLong          DefaultPath ;
    InterfaceLong          BarcodesStacked ;   // Total barcodes stacked by this acceptor - top bit is escrow indicator
    InterfaceLong          Currency ;          // Currency code reported
                                                     // by an intelligent acceptor
    pAcceptorCoinInterface FirstCoin ;         // Start of chain
    InterfaceLong          SerialNumber ;      // Reported serial number
    pInterfaceString       Description ;       // Device specific string for type / revision / coin set
    } AcceptorInterface ;


typedef struct DispenserStruct
    {
    pDispenserInterface  Next ;               // Next block in chain
    InterfaceLong        Flags ;              // Flags from Application
    InterfaceLong        Unit ;               // Specification of this unit
    InterfaceLong        Status ;             // AcceptorStatuses - zero if device OK
                                                // Must be same as Dispenser
    InterfaceLong        Interface ;          // The bus / connection
    InterfaceLong        UnitAddress ;        // For addressable units
    InterfaceLong        Value ;              // The value of the coins in this dispensor
    InterfaceLong        Count ;              // Number dispensed since interface commissioned
    InterfaceLong        Inhibit ;
    InterfaceLong        NotesToDump ;        // Only read by Paylink in conjunction with DISPENSER_PARTIAL_DUMP
    InterfaceLong        CoinCount ;          // Coins currently in the Dispenser
    InterfaceLong        CoinCountStatus ;    // Coin load status of Dispenser
    InterfaceLong        SerialNumber ;       // Reported serial number (may be "")
    pInterfaceString     Description ;        // Device specific string for type / revision / coin set
    InterfaceLong        DispenseQuantity ;   // The number of coins to be dispensed from this dispenser
    } DispenserInterface ;






typedef struct EscrowNoteStruct
    {
    pEscrowNoteInterface  Next ;              // Next block in chain
    InterfaceLong         Value ;             // Value of this note
    InterfaceLong         NoteNumber ;        // The index of the AcceptorCoin in the AcceptorBlock
    InterfaceLong         LocationFlags ;     // The dispenser on which this note is stored and the flags
    } EscrowNoteInterface  ;



enum EscrowNoteFlags
    {
    ESCROW_NOTE_DISP_MASK    = 0xff,
    ESCROW_NOTE_STATUS_SHIFT = 8
    } ;


typedef struct EscrowStruct
    {
    pEscrowInterface     Next ;               // Next block in chain
    InterfaceLong        EscrowVersion ;      // The version of the escrow system available.
    InterfaceLong        State ;              // The current state of the Escrow System
    InterfaceLong        Result ;             // The result of the previous Escrow Command
    InterfaceLong        TotalValue ;         // The total values of all the notes in Escrow
    InterfaceLong        ValueReturned ;      // The total values of all the notes just returned from Escrow
    InterfaceLong        AcceptorNo ;         // The index of the acceptor running escrow
    InterfaceLong        NoInEscrow  ;        // The number of notes currently in escrow
    pEscrowNoteInterface FirstNote ;          // the first in the list of notes

    // This section is responsible for sending commands into the Escrow system

    InterfaceLong        TheCommand ;         // This is the current command from the application to the Escrow system
    InterfaceLong        CommandsIssued ;     // This increments whenever a command is sent
    InterfaceLong        CommandsProcessed ;  // This copies CommandIssued when the Result field has been updated
    } EscrowInterface ;



enum CashlessCommand
    {
    CASHLESS_RESET,
    CASHLESS_ENABLE,
    CASHLESS_DISABLE,
    CASHLESS_SUBMIT_TICKET,
    CASHLESS_REQUEST_CREDIT,
    CASHLESS_REFUSE_CREDIT,
    CASHLESS_TAKE_CREDIT,
    CASHLESS_CANCEL_CREDIT
    } ;



typedef struct CashlessStruct
    {
    InterfaceLong        CashlessType ;       // See CashlessTypeConstants
    pInterfaceString     Description ;        // Device specific string for type / revision / coin set
    InterfaceLong        SerialNumber ;       // Reported serial number (may be 0)
    InterfaceLong        CashlessVersion ;    // The version of the Cashless system available.
    InterfaceLong        TotalAcquisitions ;  // The lifetime total count of cashless credit acquisition..
    InterfaceLong        TotalCredit ;        // The lifetime total cashless credit has been acquired.
    InterfaceLong        CurrentState ;       // The current state of the Escrow System
    InterfaceLong        StateDetails ;       // Extra Details for the current state
    InterfaceLong        CreditValue ;        // The amount of credit from Paylink for the current status

    // This section is responsible for sending commands into the Cashless system

    InterfaceLong        TheCommand ;         // This is the current command from the application to the Escrow system
    InterfaceLong        RequestedValue ;     // Only for CASHLESS_REQUEST_CREDIT
    InterfaceLong        CommandsIssued ;     // This increments whenever a command is sent
    InterfaceLong        CommandsProcessed ;  // This copies CommandIssued when the Result field has been updated
    InterfaceLong        TicketNo[2] ;        // 64 bit ticket number
    } CashlessInterface ;


enum EventData
    {
    STORED_EVENTS = 512,
    EVENT_MASK    = STORED_EVENTS - 1
    } ;

typedef struct LoggedEventStruct
    {
    InterfaceLong        ProcessedEvent ;
    InterfaceLong        EarliestEvent ;
    InterfaceLong        LatestEvent ;
    InterfaceLong        EventTime[STORED_EVENTS] ;
    InterfaceLong        EventTypeInfo[STORED_EVENTS] ;
    } LoggedEventInterface ;

#define KeyToLong(Key, Start)                \
          (((AESLong)(unsigned char)Key[0 + Start]  << 24) |   \
           ((AESLong)(unsigned char)Key[1 + Start]  << 16) |   \
           ((AESLong)(unsigned char)Key[2 + Start]  <<  8) |   \
           ((AESLong)(unsigned char)Key[3 + Start]  <<  0) )

#define LongToKey(Key, Start, LongParam)     \
        { AESLong TheLong = LongParam ;       /* LongParam is frequently an interface variable*/ \
          Key[0 + Start] = (TheLong >> 24) ; \
          Key[1 + Start] = (TheLong >> 16) ; \
          Key[2 + Start] = (TheLong >>  8) ; \
          Key[3 + Start] = (TheLong >>  0) ; }


typedef struct DiagOutputStruct
    {
    InterfaceLong ByteIndex ;
    InterfaceLong Data[64] ;
    } DiagOutputBlock ;


#endif
