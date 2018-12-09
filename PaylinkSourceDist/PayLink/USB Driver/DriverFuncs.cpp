/*--------------------------------------------------------------------------*\
 * System Includes.
\*--------------------------------------------------------------------------*/
#ifndef __linux__
    #include <vcl.h>
    #include <setupapi.h> // Used for SetupDiXxx functions
    #include "WinUSB.h"
#else
    #include <sys/types.h>
    #include <sys/mman.h>
    #include <sys/wait.h>
    #include <libusb-1.0/libusb.h>
#endif
#include <time.h>
#include <sys/timeb.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>


/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
#include "DriverFuncs.h"

//-------------------------------------------------
//
// Global Data
//
//-------------------------------------------------
unsigned char*   Base ;
volatile int*    SharedMemoryBase ;      // This is the shared memory segment for the application
         int     Mirror[2048] ;          // This is what we know that the H8 knows about
         int     RecoveryMirror[2048] ;  // This is what the H8 tells us during a restart
PCInternalBlock* PCInternal ;
unsigned int     DiagReadIndex ;


static USBAccess*  ThePort ;
static USBMilan*   Milan ;


static bool PollReceived ;
static bool MemoryResetReceived ;           // The USB card is going to re-synchronize
       bool MemoryResetting ;               // The USB card is re-synchronizing
static bool MemoryConsistent ;              // The USB card has made a fundamental change
static bool SendConfig ;                    // The USB card has request the configuration



static bool NewUSBProtocol ;

// Data shared with diagnostic window
extern bool ShowTraffic ;
       int  ReportedCardSelfTest ;
       int  CurrentDriverState ;
       int  MemoryInterlock ;

//-------------------------------------------------
//
// Diagnostics Data
//
//-------------------------------------------------
static bool NeedTime = true ;


//-----------------------------------------------------
// Access routines for meaningful view
//-----------------------------------------------------
BasicControlBlock* BasicControl = 0 ;
OutputAreaBlock*   OutputArea ;
InputAreaBlock*    InputArea ;
DiagOutputBlock*   DiagOutputArea = 0 ;

bool OurInterfaceErrror ;

#include "PCLocalAccess.cpp"



//-----------------------------------------------------
// Shared Memory Segment routines
//-----------------------------------------------------
void SetupSharedSegment(void* SegmentAddress, char* DriverType)
    {
    SharedMemoryBase = (int*)SegmentAddress ;
    PCInternal = PC_INTERNAL_BLOCK(SegmentAddress) ;

    if (PCInternal->FlagWord    == AARDVARK_FLAG_WORD &&
          (PCInternal->USBUsage == STANDARD_DRIVER ||
           PCInternal->USBUsage == DRIVER_RESTART))
        {
        DiagReadIndex = PCInternal->DiagWriteIndex ;   // Where were they up to!
        /******************************************
         * Restart driver after possible crash
         ******************************************/
        DiagPrintf("PC: %s Driver restart (after crash)", DriverType) ;
        PCInternal->USBUsage = DRIVER_RESTART ;

        /*-- Let an old one exit! -------------------------------------*/
        Sleep (2000);
        }
    else
        {
        //    As it's our segment, we'll use our setup - if it's some else's we'll leave it
        memset((void*)PCInternal, 0, sizeof *PCInternal) ;
        DiagReadIndex = 0 ;
        PCInternal->DiagIndexMask = BUFFER_SIZE - 1 ;
        DiagPrintf("PC: %s Driver startup", DriverType) ;
        }

    // Now initialise the PCInternal struct
    PCInternal->FlagWord      = AARDVARK_FLAG_WORD ;
    PCInternal->USBUsage      = STANDARD_DRIVER ;

    PCInternal->FlagWord2                = AARDVARK_FLAG_WORD ;
    PCInternal->InternalVersion          = CURRENT_INTERNAL_VERSION ;
    PCInternal->DeviceInfo.CurrentStatus = DEVICE_NEVER_SEEN ;

    memset((void*)SharedMemoryBase, 0, BASIC_SHARED_SIZE) ;
    }





void ReportInterface(InterfaceDevice* DeviceInfo)
    {
    PCInternal->DeviceInfo = *DeviceInfo ;
    }



void CurrentInterface(InterfaceDevice* DeviceInfo)
    {
    *DeviceInfo = PCInternal->DeviceInfo  ;
    }


enum {
    UNPLUGGED,
    RECOVERING,
    SEND_RESET,
    USB_NORMAL
    } ;
static int UsbStatus = UNPLUGGED ;

static unsigned long uDriverCalled = 0 ;
static unsigned long RecoveringAt ;

void USBProblem(void)  // This should be called repeatedly while a USB device isn't functioning correctly
    {
    if (UsbStatus == USB_NORMAL)
        {
        UsbStatus = SEND_RESET ;
        }
    }

//
// Local function to get a millisecond time
//
static unsigned long Milli(void)
    {
    struct timeb buf ;
    ftime(&buf) ;
    return buf.time * 1000 + buf.millitm ;
    }

void uDriverOK(void)  // This should be called repeatedly by the PC exec, to detect hangs due to USB problems
    {
    uDriverCalled = Milli() ;
    }


void AddDiagTime(void) ;
/*--------------------------------------------------------------------------*\
 * DiagPutChar:
\*--------------------------------------------------------------------------*/
void DiagPutChar(char TheChar)
    {
    if (NeedTime && (TheChar != '\f'))
        {
        NeedTime = false ;
        AddDiagTime() ;
        }

    PCInternal->DiagBuffer[PCInternal->DiagWriteIndex++ & PCInternal->DiagIndexMask] = TheChar ;

    if (TheChar == '\n' || TheChar == '\f')
        {
        NeedTime = true ;
        }
    }


static void inline DiagStr(char* Buffer)
    {
    while (char TheChar = *Buffer++)
        {
        DiagPutChar(TheChar) ;
        }
    }


void AddDiagTime(void)
    {
    static short LastHour     =  -1 ;
    static int  LastVersion   =  0 ;
    char WorkStr[128] ;

    struct tm       *theTime;
    short  Milliseconds ;

    /*-- Get the current system time ----------------------------------*/
    #ifdef __linux__
        {
        struct timespec  clockNow;
        clock_gettime (CLOCK_REALTIME, &clockNow);
        theTime = localtime(&clockNow.tv_sec);
        Milliseconds = clockNow.tv_nsec / 1000000 ;
        }
    #endif
    {
    struct timeb ClockNow ;
    ftime(&ClockNow) ;
    theTime = localtime(&ClockNow.time);
    Milliseconds = ClockNow.millitm ;
    }

    int CodeVersion = 0 ;
    if (BasicControl
     && BasicControl->FlagWord == AARDVARK_FLAG_WORD)
        {
        CodeVersion = BasicControl->CodeVersion ;
        }


    if (LastHour    != theTime->tm_hour ||
        LastVersion != CodeVersion)
        {
        LastHour    = theTime->tm_hour ;
        LastVersion = CodeVersion ;

        /*-- Convert into Day/Month/Year string -----------------------*/
        strcpy   (WorkStr, "");
        strftime (WorkStr, 32, "%d %B %Y", theTime) ;

        DiagStr(WorkStr);

        if (BasicControl
         && BasicControl->FlagWord == AARDVARK_FLAG_WORD)
            {
            sprintf(WorkStr, "%d.%d.%d.%d-%d from %s at %s",
                                (CodeVersion >> 24) & 0xff,
                                (CodeVersion >> 16) & 0xff,
                                (CodeVersion >>  8) & 0xff,
                                (CodeVersion      ) & 0xff,
                                (int)BasicControl->BuildNumber,
                                (char*)&BasicControl->CompileDate[0],
                                (char*)&BasicControl->CompileTime[0]) ;

            DiagStr((char*)"     Firmware: ") ;
            DiagStr(WorkStr) ;
            }

        #ifdef __linux__
          DiagStr((char*)" Linux Driver:") ;
        #else
          DiagStr((char*)"       Driver:") ;
        #endif
        DiagStr(DriverVersion) ;

        DiagStr((char*)"\n") ;

        NeedTime = false;    /*-- Cancel the effect of the new line ---*/
        }

    /*-- Convert into Day/Month Hour:Minute:Second string -------------*/
    strftime (WorkStr, 32, "%H:%M:%S", theTime);
    DiagStr(WorkStr);

    /*-- Add milli-seconds to the end of the time string --------------*/
    sprintf (WorkStr, ".%03d ", (int)Milliseconds);
    DiagStr(WorkStr);
    }




/*--------------------------------------------------------------------------*\
 * DiagText:
\*--------------------------------------------------------------------------*/
void DiagText(char* Buffer)
    {
    while (char TheChar = *Buffer++)
        {
        DiagPutChar(TheChar) ;
        }
    DiagPutChar('\n') ;
    }

/***********************************************************************

Data for the frame checking protocol

***********************************************************************/
#define OUT_ITEMS     4096                // Must be a power of 2 so that.......
#define OUT_ITEM_MASK (OUT_ITEMS - 1)     // this can be a mask for modulo arithmetic
#define MAX_UNACK_FRAMES 127              // Do not send any more till these are acknowledged
#define IN_ITEMS 17


typedef struct {
    short Address ;
    int Value ;
    } Frame ;


bool GoodData = false ;
bool TxHold   = false ;


Frame         OutBuffer[OUT_ITEMS] ;
int           NextOutBuffer ;                       // Next slot to use
int           SavedNextOutBuffer ;                  // Next slot to use
int           NextOutTx ;                           // Next slot to send
int           LastAckedOut ;                        // Last Frame
unsigned char LastAckedSeq ;              // Last Seq that was Acked.
unsigned char OutSeq ;                    // No of Frames sent

Frame InBuffer[IN_ITEMS] ;
int           NextIn ;
unsigned char InSeq ;
unsigned char InAck ;






/* Checksum ***********************************************************

 Calculate the checksum of a series of queued frames

***********************************************************************/
static int Checksum(Frame* FrameList, int AfterLast, int Count)
    {
    int Sum = 0 ;
    int Ind = AfterLast ;

    while (Count)
        {
        Ind = (Ind - 1) & OUT_ITEM_MASK ;               // Note that the receive buffer dosn't wrap!!!
        Sum +=  FrameList[Ind].Address ;
        Sum +=  FrameList[Ind].Value        & 0xffff ;
        Sum += (FrameList[Ind].Value >> 16) & 0xffff ;
        --Count ;
        }
    return Sum ;
    }




/* QueueCheckedFrame ************************************************

 Enter a frame into the transmit queue

***********************************************************************/
void QueueCheckedFrame(short Address, int Value)
    {
    if (NewUSBProtocol)
        {
        if (TxHold)
            {
            DiagPrintf("PC: USB Queue while held") ;
            }
        int Outstanding = (NextOutBuffer - LastAckedOut) & OUT_ITEM_MASK ;
        if (Outstanding >= OUT_ITEMS)
            {                                       // We can't do anything more until we get an Ack for some of what we've got buffered.
            DiagPrintf("PC: USB Blown Queue") ;
            return ;
            }
        OutBuffer[NextOutBuffer].Address = Address ;
        OutBuffer[NextOutBuffer].Value   = Value ;
        NextOutBuffer = (NextOutBuffer + 1) & OUT_ITEM_MASK ;
        }
    else
        {
        Milan->QueuePacket(Address, Value) ;
        }
    }





/* SendQueuedFrames  **************************************************

 send queued frames, but keep them in case we have to resend

***********************************************************************/
void SendQueuedFrames(void)
    {
    if (NewUSBProtocol)
        {
        int Outstanding = (NextOutBuffer - NextOutTx) & OUT_ITEM_MASK ;
        int SendCount = 0 ;

        if (Outstanding > MAX_UNACK_FRAMES)
            {
            TxHold = true ;
            Outstanding = MAX_UNACK_FRAMES ;
            }
        else
            {
            TxHold = false ;
            }

        for (int i = 0 ; i < Outstanding ; ++i)
            {
            Milan->QueuePacket(OutBuffer[NextOutTx].Address, OutBuffer[NextOutTx].Value) ;
            ++OutSeq ;
            NextOutTx = (NextOutTx + 1) & OUT_ITEM_MASK ;

            if (++SendCount == MILAN_IN_ITEMS)
                {                                       // We've now filled the receive buffer, so send a check frame
                Milan->QueuePacket(USB_CHECKPOINT, Checksum(OutBuffer, NextOutTx, MILAN_IN_ITEMS) | ((int)OutSeq << 24)) ;
                SendCount = 0 ;
                }
            }

        if (SendCount)
            {                                           // send trailing check frame
            Milan->QueuePacket(USB_CHECKPOINT, Checksum(OutBuffer, NextOutTx, SendCount) | ((int)OutSeq << 24)) ;
            }
        SavedNextOutBuffer = NextOutBuffer ;
        }
    }




/*  DiscardQueuedFrames ***********************************************

Changed our mind - don't send them

***********************************************************************/
void DiscardQueuedFrames(void)
    {
    NextOutBuffer = SavedNextOutBuffer ;                        // Reset Tx slot pointer
    if (!NewUSBProtocol)
        {
        Milan->ResetTxBuffer() ;                                       // In old system clear the buffer
        }
    }




/*  ProcessCheckedPacket ***********************************************

 these packets are saved unti a good checkpoint is received

***********************************************************************/
static void ProcessCheckedPacket(unsigned short Address, int Value)
    {
    unsigned short Code  =  Address & USB_CONTROL_MASK ;
    unsigned short Index = (Address & USB_ADDRESS_MASK) >> 2 ;

    switch (Code)
        {
    case USB_VALUE:               // Value is a replacement value
        if (Index == 0)
            {
            if (Value != AARDVARK_FLAG_WORD)
                {
                DiagPrintf("PC: *** Flag sent as %08x ***\n") ;
                }

            if (MemoryResetting)
                {
                MemoryResetting = false ;   // This is the end of memory setup


                if (MemoryConsistent)
                    {                       // OK - now we've got to reconcile the **3** versions of the interface memory
                    OutputArea = BasicControl->OutputPointer ;
                    InputArea  = BasicControl->InputPointer ;
                    for (int Index = 0 ; Index < 2048 ; ++Index)
                        {
                        int RelAddr = Index << 2 ;
                        if (SharedMemoryBase[Index] != Mirror[Index])
                            {               // This is an update by the application -
                            }               // leave it to do a normal update

                        else if (RelAddr >= (long)OutputArea && RelAddr < (long)(OutputArea + 1))
                            {                 // This is the output area - we accept any "update" EXCEPT PayValue and normal processing
                            Mirror[Index] = RecoveryMirror[Index] ;    //  will put it back how it was

                            if (Index == ((int*)&(OutputArea->PayValue) - SharedMemoryBase))
                                {
                                DiagPrintf("PC: Input Update PayValue %ld, Old %ld", RecoveryMirror[Index], SharedMemoryBase[Index]) ;
                                SharedMemoryBase[Index] = RecoveryMirror[Index] ;
                                }
                            }

                        else if (RelAddr >= (long)InputArea && RelAddr < (long)(InputArea + 1))
                            {                 // This is the Pc Input area - we just accept the "update"
                            if (Mirror[Index] != RecoveryMirror[Index])
                                {
                                DiagPrintf("PC: Input Update @%x, %x=>%x", RelAddr, Mirror[Index], RecoveryMirror[Index]) ;
                                }
                            Mirror[Index]           = RecoveryMirror[Index] ;
                            SharedMemoryBase[Index] = RecoveryMirror[Index] ;
                            }

                        else if (RecoveryMirror[Index] != Mirror[Index])
                            {               // This is an H8 update of shared heap (or basic control)
                            #ifdef PC_USES_HEAP
                                            // but that's owned by the PC so we accept the "update" and normal processing
                                Mirror[Index] = RecoveryMirror[Index] ;   //  will put it back how it was
                            #else
                                            // it's a real update so put it into interface and mirror
                                if ((Index != (int *)&BasicControl->RandomNumber - SharedMemoryBase)
                                 && (RelAddr < ((long) DiagOutputArea) || RelAddr > (long)(DiagOutputArea + 1)))
                                    {
                                    DiagPrintf("PC: Update @%x, %x=>%x", RelAddr, Mirror[Index], RecoveryMirror[Index]) ;
                                    }
                                Mirror[Index]           = RecoveryMirror[Index] ;
                                SharedMemoryBase[Index] = RecoveryMirror[Index] ;
                            #endif
                            }
                        }

                    // Now do a bit of tidying up!
                    if (OutputArea->TransferSequence != InputArea->TransferSequence)
                        {
                        OutputArea->TransferSequence = InputArea->TransferSequence ;
                        DiagPrintf("PC: Transfer Reset") ;
                        }

                    DiagPrintf("PC: USB unit re-synch complete") ;
                    }
                else
                    {
                    if (SharedMemoryBase[0] == AARDVARK_FLAG_WORD)
                        {
                        DiagPrintf("PC: *** USB unit major memory reset ***") ;
                        }
                    // Lose all the dispcrepancies in the shared memory
                    memcpy((void*)SharedMemoryBase, RecoveryMirror, sizeof RecoveryMirror) ;
                    OutputArea = BasicControl->OutputPointer ;
                    InputArea  = BasicControl->InputPointer ;
                    memcpy(Mirror, RecoveryMirror, sizeof RecoveryMirror) ;
                    }
                }

            if (!MergedInterface
             || SetupMergeProcess())
                {
                RecoveryMirror[0]   = AARDVARK_FLAG_WORD ;
                Mirror[0]           = AARDVARK_FLAG_WORD ;
                SharedMemoryBase[0] = AARDVARK_FLAG_WORD ;
                }
            break ;
            }


        if (MergedInterface
         && MergeProcessInput(Index, Value))
            {
            //  The update has been completely handled by the Merge Process
            }
        else
            {
            if (MemoryResetting)
                {
                if (SharedMemoryBase[Index] != Value)
                    {                   // This *may* indicate a mis-match - but some values don't match.
                    unsigned int RelAddr = Index << 2 ;
                    if (Index == (int *)&BasicControl->RandomNumber - SharedMemoryBase)
                        {               // The random number in basic control
                        }
                    else if (MemoryConsistent
                          && RelAddr < sizeof (BasicControlBlock))
                        {
                        DiagPrintf("*** PC: Basic Resync fail @%x, %x<>%x", RelAddr, SharedMemoryBase[Index], Value) ;
                        MemoryConsistent = false ;
                        }
                    }
                RecoveryMirror[Index] = Value ;             // Recovery - create our copy of the H8
                }
            else            // Normal running - update both version of the interface memory
                {
                SharedMemoryBase[Index] = Value ;
                Mirror          [Index] = Value ;
                }
            }
        break ;



    case USB_INCREMENT:            // Value is a (signed) increment
        Mirror          [Index] += Value ;
        RecoveryMirror  [Index] = Mirror[Index] ;
        SharedMemoryBase[Index] = Mirror[Index] ;
        break ;



    case USB_COPY:                  // Value is in fact an Index to be copied into this

        Mirror          [Index] = Mirror[(Value & USB_ADDRESS_MASK) >> 2] ;
        RecoveryMirror  [Index] = Mirror[Index] ;
        SharedMemoryBase[Index] = Mirror[Index] ;
        break ;
        }
    }









/***********************************************************************

ProcessIncomingPacket - called by chip handler to process a packet

***********************************************************************/

static void ProcessOurIncomingPacket(unsigned short Address, int Value)
    {
    unsigned short Code  =  Address & USB_CONTROL_MASK ;
    unsigned short Index = (Address & USB_ADDRESS_MASK) >> 2 ;

    if (ShowTraffic && (Address != USB_DIAGNOSTIC))
        {
        DiagPrintf(" <--%04x %08x", Address, Value) ;
        }



    switch (Code)
        {
    case USB_VALUE:                 // Value is a replacement value
    case USB_INCREMENT:             // Value is a (signed) increment
    case USB_COPY:                  // Value is in fact an Index to be copied into this
        if (GoodData)
            {
            if (!NewUSBProtocol)
                {                           // Old "protocol" - just process the packet
                ProcessCheckedPacket(Address, Value) ;
                break ;
                }

            InBuffer[NextIn].Address = Address ;
            InBuffer[NextIn].Value   = Value ;
            ++InSeq ;
            if (++NextIn > IN_ITEMS)
                {                           // Everything has gone pear shaped
                Milan->ResetTxBuffer() ;
                Milan->QueuePacket(USB_RESET , USB_RESET) ;
                Milan->QueuePacket(USB_RESEND, InAck) ;
                DiagPrintf("PC: USB queue overflow at %d, resend Req from %d", NextIn, InAck) ;
                NextIn = 0 ;
                GoodData = false ;
                }
            }
        break ;


#if 0
      case USB_FIRST_LOCKOUT:       // This is the first in a set of locked out updates
        break ;

      case USB_LAST_LOCKOUT:        // This is the last in a set of locked out updates
        break ;
#endif

    default:
        DiagPrintf("PC: Unknown code %x received", Address) ;
        if (GoodData)
            {
            DiagPrintf("PC: Resend Requested") ;
            Milan->ResetTxBuffer() ;
            Milan->QueuePacket(USB_RESET , USB_RESET) ;
            Milan->QueuePacket(USB_RESEND, InAck) ;
            NextIn = 0 ;
            GoodData = false ;
            }
        break ;


    // Now, the codes, that don't need an Address
    case USB_SET_COMMAND:
        switch(Address)
            {
        case USB_DIAGNOSTIC:
            {
            unsigned int Shifter = Value ;         // Get zeros shifted in at top
            while (Shifter)
                {
                DiagPutChar(Shifter & 0xff) ;
                Shifter >>= 8 ;
                }
            }
            break ;





        case USB_CHECKPOINT:
            if (MemoryResetting
             && (Value == -1 || (Value & 0xffffff) == 0))
                {                                   // It's an admin frame, but we're in the
                DiagPrintf("PC: Re-sending restart") ;         // middle of recovery!!!
                Milan->ResetTxBuffer() ;
                Milan->QueuePacket(USB_RESET , USB_RESET) ;
                Milan->QueuePacket(USB_ACTION, USB_RESTART) ;      // Init  / Start
                NextIn = 0 ;
                break ;
                }


            if (!GoodData)
                {                                   // We're waiting a valid data stream
                if (Value == -1)
                    {                               // But they-re hanging on a send
                    DiagPrintf("PC: Unit stuck at send") ;
                    Milan->ResetTxBuffer() ;
                    Milan->QueuePacket(USB_RESET , USB_RESET) ;
                    Milan->QueuePacket(USB_RESEND, InAck) ;
                    }
                NextIn = 0 ;
                break ;
                }


            if (USBConfigCount)
                {
                // We've got comms established with the Paylink, so other USB connections should be working
                if (UsbStatus == SEND_RESET)
                    {
                    Milan->QueuePacket(USB_ACTION, USB_RESET_HUB) ;
                    DiagPrintf("USB: Reset Hub sent to Paylink\n") ;
                    RecoveringAt = Milli() ;
                    UsbStatus = UNPLUGGED ;
                    }
                else if ((UsbStatus == UNPLUGGED)
                     && ((Milli() - RecoveringAt) > 2000))              // Give Paylink time to re-act
                    {
                    UsbStatus = RECOVERING ;
                    RecoveringAt = Milli() ;
                    DiagPrintf("USB: Recovering\n") ;
                    }
                else if ((UsbStatus == RECOVERING)
                     && ((Milli() - RecoveringAt) > 10000))
                    {
                    UsbStatus = USB_NORMAL ;
                    DiagPrintf("USB: Recovered\n") ;
                    }
                }

            if (Value == ((int)InAck << 24)
             && (NextIn == 0))
                {             // This is the "keep alive" poll
                Milan->QueuePacket(USB_ACK, InAck) ;
                Milan->QueuePacket(USB_CHECKPOINT, ((int)OutSeq << 24)) ;  // so we send one too
                }
            else if (Checksum(InBuffer, NextIn, NextIn) == (Value & 0xffffff)
                   &&((Value >> 24) & 0xff) == InSeq)
                {                // Process the good data
                InAck = InSeq ;
                Milan->QueuePacket(USB_ACK, InAck) ;

                for (int i = 0 ; i < NextIn ; ++i)
                    {
                    ProcessCheckedPacket(InBuffer[i].Address, InBuffer[i].Value) ;
                    }
                }
            else
                {                                       // Oops - need to do a recovery
                DiagPrintf("PC: USB Checkpoint fault %x<>%x %d<>%d (%d)",
                                        Checksum(InBuffer, NextIn, NextIn), Value & 0xffffff,
                                        InSeq, (Value >> 24) & 0xff, NextIn) ;
                DiagPrintf("PC: USB Resend from %d", InAck) ;
                Milan->ResetTxBuffer() ;
                Milan->QueuePacket(USB_RESET , USB_RESET) ;
                Milan->QueuePacket(USB_RESEND, InAck) ;
                GoodData = false ;
                }
            NextIn = 0 ;
            break ;





        case USB_RESEND_START:
            if (InAck == Value)
                {
                GoodData = true ;
                NextIn = 0 ;                        // OK - this is the start of a re-send
                InSeq = InAck ;
                }
            else
                {
                DiagPrintf("PC: Bad resend %d<>%d", InAck, Value) ;
                }
            break ;







        case USB_ACK:
                {
                unsigned char Acked = Value - LastAckedSeq ;    // Count of Acked frames
                LastAckedOut += Acked ;       // For use in output queue measurement
                LastAckedSeq = Value ;        // Remember where ACKs have reached
                }
            break ;


        case USB_RESEND:
            if (NewUSBProtocol)
                {
                unsigned char ReSend    = OutSeq - Value ;         // This many frames are not acknowledged.
                unsigned char NewSeq    = Value ;                  // This frame was acknowled
                int           NextFrame = (NextOutTx - ReSend) & OUT_ITEM_MASK ;

                DiagPrintf("PC: unit requests %d frames from %d", ReSend, NewSeq) ;
                LastAckedSeq = Value ;                            // This is also an ACK up to this point

                Milan->ResetTxBuffer() ;
                Milan->QueuePacket(USB_RESET, USB_RESET) ;
                Milan->QueuePacket(USB_RESEND_START, NewSeq) ;

                for (int i = 0 ; i < ReSend ; ++i)
                    {
                    Milan->QueuePacket(OutBuffer[NextFrame].Address, OutBuffer[NextFrame].Value) ;
                    ++NewSeq ;

                    NextFrame = (NextFrame + 1) & OUT_ITEM_MASK ;
                    Milan->QueuePacket(USB_CHECKPOINT, Checksum(OutBuffer, NextFrame, 1) | ((int)NewSeq << 24)) ;
                    }
                }
            break ;


        case USB_SEND_CONIFG:                       // This really belongs with  below
                                                    // but that breaks old driver programs!
            SendConfig          = true ;
                          // We have already just received a USB_NEW_MEMORY_CLEAR, so we have to undo some of it
            MemoryResetReceived = false ;
            GoodData            = false ;
                          // and duplicate the rest, just in case
            NextIn              = 0 ;
            InSeq               = 0 ;
            InAck               = 0 ;
            break ;


        default:
            DiagPrintf("PC: Unknown Set sub-code %x received", Address) ;
            break ;
            }
        break ;



    // And finally - the codes, that don't need a value
    case USB_ACTION:
        if (Index != 0)
            {
            DiagPrintf("PC: Extended Code flag with non zero Address %x", Address) ;
            }
        else switch (Value)
            {
             // Note that USB_MEMORY_CLEAR and USB_NEW_MEMORY_CLEAR **should** arrive in the same block and so
             // will overwrite each other. Even if they don't it just generates a redundant diagnostic on the H8
        case USB_MEMORY_CLEAR:
            MemoryResetReceived = true ;
            GoodData            = true ;
            NewUSBProtocol      = false ;
            break ;

        case USB_NEW_MEMORY_CLEAR:
            GoodData            = true ;
            MemoryResetReceived = true ;
            NewUSBProtocol      = true ;
            NextIn              = 0 ;
            InSeq               = 0 ;
            InAck               = 0 ;
            break ;


        case USB_CONFIG_NAK:
            DiagPrintf("Config: Paylink rejected Config download - restarting") ;
            /* No Break */



        case USB_CONFIG_MATCH:
            DiagPrintf("Config: Paylink Checked OK against file \"%s\"", ConfigFileStr) ;
                            // This equate to a memory clear
            GoodData            = true ;
            MemoryResetReceived = true ;
            NewUSBProtocol      = true ;
            NextIn              = 0 ;
            InSeq               = 0 ;
            InAck               = 0 ;
            break ;


        case USB_CONFIG_UPDATE:
            DiagPrintf("Config: Paylink rebooting with new Config") ;
            BasicControl->FlagWord = 0 ;                  // We can't do a resync !!!!
            break ;


#if 0
        case USB_RESTART:                                 //  Restart (not on PC)
           break ;
#endif


        case USB_H8_RESPONSE:
           while(true) {}                                 // crash


        case USB_POLL:                                    // No Op from USB - echo it
           PollReceived = true ;
           break ;


        default:
            DiagPrintf("PC: Unknown extended code %x received", Value) ;
            if (GoodData)
                {
                DiagPrintf("PC: Resend Requested") ;
                Milan->ResetTxBuffer() ;
                Milan->QueuePacket(USB_RESET , USB_RESET) ;
                Milan->QueuePacket(USB_RESEND, InAck) ;
                NextIn = 0 ;
                GoodData = false ;
                }
            break ;
            }
        }
    }




/***********************************************************************

The main code

***********************************************************************/

extern char SavedSerialNumber[];

bool CommunicateWithCard(void)
    {
    UsbStatus = UNPLUGGED ;
#ifndef __linux__
    // Check for (any) FTDI Connection
    WinUSB FTDICheck(USB_VID, USB_PID) ;
    if (FTDICheck.LocateChip()
        && (FTDICheck.Vid != 0))
        {
        ThePort = new FTDIAccess(PRODUCT_NAME, USB_VID, USB_PID, BAUD_RATE) ;
        }
    else
        {
        // Check for an HID Connection
        WinUSB HIDCheck(USB_VID, USB_HID) ;
        if (HIDCheck.LocateChip()
        && (HIDCheck.Vid != 0))
            {
            ThePort = new HIDAccess(PRODUCT_NAME, USB_VID, USB_HID, 0) ;
            }
        else
            {
            DiagPrintf("PC: No USB devices connected") ;
            return false ;
            }
        }
#else
    ThePort = 0 ;
    libusb_context *context = NULL;
    libusb_device **list = NULL;
    ssize_t count = 0;

    libusb_init(&context);
    count = libusb_get_device_list(context, &list);
    for (int i = 0; i < count; ++i)
        {
        libusb_device *device = list[i];
        libusb_device_descriptor desc = {0};
        libusb_get_device_descriptor(device, &desc);
        if (desc.idVendor == USB_VID && desc.idProduct == USB_PID)
            {
            ThePort = new FTDIAccess(PRODUCT_NAME, USB_VID, USB_PID, BAUD_RATE) ;
            }
        if (desc.idVendor == USB_VID && desc.idProduct == USB_HID)
            {
            ThePort = new HIDAccess(PRODUCT_NAME, USB_VID, USB_HID, 0) ;
            }
        }

    if (!ThePort)
        {
        DiagPrintf("PC: No USB devices connected") ;
        return false ;
        }

#endif

    Milan   = new USBMilan(ThePort) ;
    Milan->ProcessIncomingPacket = ProcessOurIncomingPacket ;

    /***********************************************************************

    Get the FTDI chip

    ***********************************************************************/

       DiagPrintf("PC: Driver start up") ;

    if (!strlen(SavedSerialNumber))
        {
        // Then it's a request to open any old Paylink

        if (!ThePort->USBOpen())
            {                     // The error is already reported via DiagPrintf
            PCInternal->USBUsage = USB_ERROR ;
            delete Milan ;
            delete ThePort ;
            return false ;
            }
        }
    else
        {
        if (!ThePort->USBOpenSpecific(SavedSerialNumber))
            {                     // The error is already reported via DiagPrintf
            PCInternal->USBUsage = USB_ERROR ;
            delete Milan ;
            delete ThePort ;
            return false ;
            }
        }

    // Now invalidate the H8 Memory
    BasicControl->FlagWord = 0 ;
    PCInternal->USBUsage   = STANDARD_DRIVER ; // And state we're running

    MemoryResetting = true ;
    MemoryConsistent = false ;                  // We might get rubbish before restart!

/***********************************************************************

And start everything up

***********************************************************************/
    Milan->ResetTxBuffer() ;
    Milan->QueuePacket(USB_RESET , USB_RESET) ;
    Milan->QueuePacket(USB_ACTION, USB_RESTART) ;      // Init  / Start

    // let's **pretend** we're running normally !!!!!
    memcpy(Mirror, (void*)SharedMemoryBase, sizeof Mirror) ;



/***********************************************************************

Main Loop

***********************************************************************/
    int            ThisGuard = -1 ;
    int            LastGuard = 0 ;
    short          EndIndex = 0 ;
    short          StartIndex = 0 ;
    unsigned char  CurrentDiagIndex = 0 ;


    while (true)
        {
        if (SendConfig)
            {
            SendConfig = false ;
            if (ConCount == 0)
                {
                DiagPrintf("Config: No Config File\n") ;
                MemoryResetReceived = true ;                  // So treat Config request as memory reset
                continue ;
                }

            MemoryResetting = true ;                          // This is part of "memory reseting"
            // Download the configuration
            Milan->QueuePacket(USB_ACTION, USB_CONFIG_START) ;
            for (int i = 0 ; i < ConCount ; ++i)
                {
                Milan->QueuePacket(USB_CONFIG | RecordToAddress(TheConfig[i]) ,
                                                RecordToValue  (TheConfig[i])) ;   // Config Packet
                }
            Milan->QueuePacket(USB_ACTION, USB_CONFIG_END) ;
            }

        CurrentDriverState = Pause ;
        #ifdef __linux__
            Sleep(1);
            pthread_mutex_lock(&Mutex) ;
        #else
            WaitForSingleObjectEx(Timer, INFINITE, true) ;
            WaitForSingleObject(Mutex, INFINITE) ; // Claim Shared segment
        #endif


        if (MemoryResetReceived)
            {
            if (BasicControl->FlagWord == AARDVARK_FLAG_WORD ||
                BasicControl->FlagWord == 0x12345678)
                {
                MemoryResetting = true ;
                MemoryConsistent = true ;
                DiagPrintf("PC: Memory Resynch - %s protocol", (NewUSBProtocol) ? "new" : "old") ;
                memset(RecoveryMirror, 0, 8192) ;
                }
            else
                {
                memset((void*)SharedMemoryBase, 0, 8192) ;
                memset(Mirror, 0, 8192) ;
                DiagPrintf("PC: Memory Reset - %s protocol", (NewUSBProtocol) ? "new" : "old") ;
                }

            Milan->ResetTxBuffer() ;
            Milan->QueuePacket(USB_RESET , USB_RESET) ;
            if (NewUSBProtocol)
                {
                Milan->QueuePacket(USB_ACTION, USB_NEW_MEMORY_CLEAR) ;   // echo back & agree new protocol
                NextOutBuffer       = 0 ;
                SavedNextOutBuffer  = 0 ;
                NextOutTx           = 0 ;
                LastAckedOut        = 0 ;
                LastAckedSeq        = 0 ;
                OutSeq              = 0 ;
                NextIn              = 0 ;           // This frame clears these at the far end.
                InSeq               = 0 ;
                InAck               = 0 ;
                }
            else
                {
                Milan->QueuePacket(USB_ACTION, USB_MEMORY_CLEAR) ;   // echo back for old card
                }
            MemoryResetReceived = false ;
            }


        CurrentDriverState = CheckingPC ;

        if (!MemoryResetting && !TxHold)
            {
            /*
             * Scan the shared memory for changes
             */
            int NewMirror[2048] ;

            OutputArea = 0 ;
            if (BasicControl->FlagWord == AARDVARK_FLAG_WORD)
                {
                ReportedCardSelfTest = BasicControl->CardSelfTest ;
                DiagOutputArea       = BasicControl->DiagOutPointer ;
                OutputArea           = BasicControl->OutputPointer ;

                StartIndex = (int*)&OutputArea->StartGuard - SharedMemoryBase ;
                EndIndex   = (int*)&OutputArea->EndGuard - SharedMemoryBase ;
                ThisGuard   = OutputArea->EndGuard ;
                if (LastGuard != ThisGuard)
                    {
                    QueueCheckedFrame(StartIndex << 2, ThisGuard) ;
                    }
                }

            for (int i = 0 ; i < 2048 ; ++i)
                {
                int Value = SharedMemoryBase[i] ;
                if (Value != Mirror[i])
                    {
                    if (i != StartIndex && i != EndIndex)
                        {                   // Don't send Guards here
                        int FrameValue = Value ;
                        if (MergedInterface)
                            {
                            FrameValue = MergeProcessOutput(i, FrameValue) ;
                            }
                       QueueCheckedFrame(i << 2, FrameValue) ;
                        }
                    }
                NewMirror[i] = Value ;
                }


            if (OutputArea &&
                        LastGuard != ThisGuard)
                {
                QueueCheckedFrame(EndIndex << 2, ThisGuard) ;
                }

            /*
             * Don't Send changes if inconsistent
             */
            if (OutputArea &&
                        OutputArea->StartGuard != ThisGuard)
                {                           // The data is inconsistent - don't send it!
                DiscardQueuedFrames() ;                // Reset Message buffer in case re-try
                if (++MemoryInterlock == 1)
                    {
                    int StartGuard = OutputArea->StartGuard ;
                    DiagPrintf("PC: Update Interlock Set. (%d != %d / %d)", StartGuard,
                                                                            ThisGuard,
                                                                            LastGuard) ;
                    }
                }
            else
                {
                if (MemoryInterlock)
                    {
                    DiagPrintf("PC: Interlock Now Clear. (After %d tries.)", MemoryInterlock) ;
                    MemoryInterlock = 0 ;
                    }

                LastGuard = ThisGuard ;
                CurrentDriverState = Sending ;
                memcpy(Mirror, NewMirror, sizeof Mirror) ;
                }
            }

        SendQueuedFrames() ;


        /*
         * Now Check for "incoming" traffic
         */
        CurrentDriverState = Reading ;   // This will call ProcessIncomingPacket as necessary
        bool ReadOk = Milan->ProcessIncomingStream() ;

        #ifndef __linux__
            ReleaseMutex(Mutex) ;           // We've finished accessing the shared segment
        #else
            pthread_mutex_unlock(&Mutex) ;  //      Do this here as we may exit!
        #endif

        if (!ReadOk)
            {                     // The error is already reported via DiagPrintf
            PCInternal->USBUsage = USB_ERROR ;
            DiagPrintf("PC: Driver Exit as Read Problem") ;
            ThePort->USBClose() ;
            delete Milan ;
            delete ThePort ;
            return false ;
            }




        if (PollReceived)
            {
            Milan->QueuePacket(USB_ACTION, USB_H8_RESPONSE) ;     // echo back
            PollReceived = false ;
            }

        /*
         * Tell the H8 what's going on
         */
        if (Milan->SendBuffer())
            {                     // The error is already reported via DiagPrintf
            PCInternal->USBUsage = USB_ERROR ;
            DiagPrintf("PC: Driver Exit as Send Problem") ;
            ThePort->USBClose() ;
            delete Milan ;
            delete ThePort ;
            return false;
            }


        /*
         * Check for old style USB card application diagnostics
         */

        if (DiagOutputArea)
            {
            unsigned int     NewDiagIndex   = DiagOutputArea->ByteIndex ;
            while ((unsigned)CurrentDiagIndex != (NewDiagIndex & 0xff))
                {
                DiagPutChar(((char*)&DiagOutputArea->Data[0])[CurrentDiagIndex++]) ;
                }
            }


        /*
         * Check for problems with PC based USB peripherals
         */
        if (USBConfigCount)
            {
            unsigned long Mill = Milli() ;
            if ((Mill > (uDriverCalled + 60000))
               && (UsbStatus == USB_NORMAL))
                {
                DiagPrintf("USB: uDriver dead  %ld<>%ld\n", Mill, uDriverCalled) ;
                USBProblem() ;
                }
            }


        /*
         * And finally Check for driver restarts
         */

        if (PCInternal->USBUsage != STANDARD_DRIVER)
            {
            if (PCInternal->USBUsage == DRIVER_RESTART)
                {
                PCInternal->USBUsage = USB_IDLE ;
                DiagPrintf("PC: Driver Exit Requested") ;
                ThePort->USBClose() ;
                CurrentDriverState = DriverAbort ;
                #ifdef __linux__
                    _exit(0);
                #else
                    ExitThread(0) ;
                #endif
                delete Milan ;
                delete ThePort ;
                return false ;
                }

            DiagPrintf("PC: USB link in use") ;
            ThePort->USBClose() ;
            Sleep(2000) ;
            delete Milan ;
            delete ThePort ;
            return true ;
            }
        }
    }
