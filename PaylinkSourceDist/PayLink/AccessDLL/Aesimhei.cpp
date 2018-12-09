/*******************************************************************************
 * Copyright (c) 2003 Aardvark Embedded Solutions.
 *
 *
 * File Name:
 *
 *     Aesimhei.c
 *
 * Description:
 *
 *     This file contains all the Aardvark API functions.
 *
 *****************************************************************************/
#ifdef __linux__
    #define DLL
#else
    #define DLL __declspec(dllexport) __stdcall    // definition for our header
#endif

#include "Main.h"
#include "PCAccess.cpp"                 // The (inline) access functions


/****************************************************************
Internal values
****************************************************************/
static int  OpenResult = 0 ;
static int  CallerVersion = 0 ;

static void DESUpdatePaylink(void) ;



static void GetShared(void)
    {
    OpenResult = MapDPRam() ;

    if (OpenResult == ApiSuccess)
        {
        OutputArea->EnableInterfaceCount++ ;         // Ensure EnableInterfaceCount value is different from before

        InputArea  = BasicControl->InputPointer ;
        OutputArea = BasicControl->OutputPointer ;

        if (OutputArea->PayValueRandomCheck == 0)
        {
            OutputArea->PayValueRandomCheck = BasicControl->RandomNumber ; // Trigger pay checking system
        }

        OutputArea->RunningCount++ ;
        if (OutputArea->RunningCount < 1)
            {                           // This must be a (restart) error
            OutputArea->RunningCount = 1 ;
            }
        }
    }

static void GetSharedSpecific(char * SerialNumber)
    {
    OpenResult = MapNamedDPRam(SerialNumber) ;

    if (OpenResult == ApiSuccess)
        {
        static int  EnableCount = 0 ;
        // This code is intended to increment all the EnableInterfaceCounts
        // at least once per open to force the readout of events
        if (OutputArea->EnableInterfaceCount == 0
         && EnableCount == 0)
            {
            EnableCount = 1 ;
            }
        else if (EnableCount < OutputArea->EnableInterfaceCount)
            {
            EnableCount = OutputArea->EnableInterfaceCount + 1 ;
            }
        OutputArea->EnableInterfaceCount = EnableCount ;

        InputArea  = BasicControl->InputPointer ;
        OutputArea = BasicControl->OutputPointer ;

        if (OutputArea->PayValueRandomCheck == 0)
        {
            OutputArea->PayValueRandomCheck = BasicControl->RandomNumber ; // Trigger pay checking system
        }

        OutputArea->RunningCount++ ;
        if (OutputArea->RunningCount < 1)
            {                           // This must be a (restart) error
            OutputArea->RunningCount = 1 ;
            }
        }
    }

#ifndef __linux__
    /*****************************************************************************
     *
     * Function   :  DllMain
     *
     * Description:  DllMain is called by Windows each time a process or
     *               thread attaches or detaches to/from this DLL.
     *
     *****************************************************************************/
    BOOLEAN WINAPI
    DllMain(HANDLE hInst,
            U32    ReasonForCall,
            LPVOID lpReserved
            )
        {
        // Added to prevent compiler warning
        if (hInst == INVALID_HANDLE_VALUE)
            {
            }

        if (lpReserved == NULL)
            {
            }

        switch (ReasonForCall)
            {
            case DLL_PROCESS_ATTACH:
                GetShared() ;
                break;


            case DLL_PROCESS_DETACH:
                // Do our un-initialisation
                if (OpenResult == ApiSuccess && --(OutputArea->RunningCount) == 0)
                    {
                    DisableInterface() ;
                    OutputArea->PCPresent = 0 ;              // Admit we've all gone!
                    }

                // Then PLX's
                UnMapRam() ;
                break;


            case DLL_THREAD_ATTACH:
            case DLL_THREAD_DETACH:
            default:
                PlxDLLMain(hInst, ReasonForCall, lpReserved) ;
                break;
            }
        return TRUE;
        }
#else

    void __attribute__ ((constructor)) aes_access_init(void);
    void __attribute__ ((destructor))  aes_access_fini(void);

    void __attribute__ ((constructor)) aes_access_init(void)
        {
            GetShared() ;
        }

    void __attribute__ ((destructor))  aes_access_fini(void)
        {
            if ((OpenResult == ApiSuccess) && --(OutputArea->RunningCount) == 0)
                {
                DisableInterface() ;
                OutputArea->PCPresent = 0 ;
                }
        }
#endif





/****************************************************************
The OpenMHE call is made by the PC application software to open the
"Money Handling Equipment" Interface.

Parameters
None

Return Value
If the Open call succeeds then the value zero is returned.

In the event of a failure an error code will be returned,
either as a direct echo of a Windows API call failure,
or to indicate internally detected failures that closely
correspond to the quoted meanings.
****************************************************************/
#define ORIGINAL_VERSION    0x10001
#define DISPENSER_UPDATE    0x10002
#define STRINGS_RETURNED    0x10005


int DLL OpenMHEVersion(int  InterfaceVersion)
    {
    switch (InterfaceVersion)
        {
    case ORIGINAL_VERSION:
    case DISPENSER_UPDATE:              // Valid Versions
    case STRINGS_RETURNED:
    case BARCODE_ACCEPTOR:
        break ;

    default:                            // Non-valid version
        return ERROR_INVALID_DATA ;
        }

    CallerVersion = InterfaceVersion ;

    if (OpenResult != ApiSuccess)
        {                               // We've currently failed to get anything, but
        GetShared() ;                   // things might change
        }


    if (OpenResult == ApiSuccess)
        {                               // It worked !!
        if (BasicControl->FlagWord    != AARDVARK_FLAG_WORD ||
            InputArea->InterfaceError != 0)
            return ERROR_GEN_FAILURE ;

        if (BasicControl->MeaningVersion != APPLICATION_MEANING_VALUE)
            return ERROR_INVALID_DATA ;

        OutputArea->PCPresent = 1 ;              // Admit we're here!
        return 0 ;          // All right - we must be working!!
        }


    if (MMFile)
        {                       // Decode USB Errors
        if (PCInternal)
            {                   // Well, the driver's running
            switch (PCInternal->USBUsage)
                {
                case USB_ERROR:          return ERROR_DEVICE_NOT_CONNECTED ;

                case STANDARD_DRIVER:    return ERROR_NOT_READY ;

                case FLASH_LOADER:
                case MANUFACTURING_TEST:
                case DRIVER_RESTART:     return ERROR_BUSY ;

                default:                 return ERROR_GEN_FAILURE ;
                }
            }
        }
    else
        {                       // Decode PCI errors
        switch (OpenResult)
            {
            case ApiInvalidAddress:     return ERROR_NOT_READY ;

            case ApiInvalidDeviceInfo:  return ERROR_GEN_FAILURE ;

            case ApiNoActiveDriver:     return ERROR_BAD_UNIT ;

            default:                    return 0x10000000 + OpenResult ;
            }
        }

    return ERROR_GEN_FAILURE ;
    }

/****************************************************************
The OpenSpecificMHE call is made by the PC application software
to open a "Money Handling Equipment" Interface with a specific
serial number.

Parameters
Alphanumeric

Return Value
If the Open call succeeds then the value zero is returned.

In the event of a failure an error code will be returned,
either as a direct echo of a Windows API call failure,
or to indicate internally detected failures that closely
correspond to the quoted meanings.
****************************************************************/
int DLL OpenSpecificMHEVersion(char * SerialNumber, int  InterfaceVersion)
    {
    switch (InterfaceVersion)
        {
    case ORIGINAL_VERSION:
    case DISPENSER_UPDATE:              // Valid Versions
    case STRINGS_RETURNED:
    case BARCODE_ACCEPTOR:
        break ;

    default:                            // Non-valid version
        return ERROR_INVALID_DATA ;
        }

    CallerVersion = InterfaceVersion ;

        // We may already have successfully opened a shared memory area,
        // but we need to open another regardless. (This could be an
        // application trying to communicate with a DIFFERENT paylink!)

        GetSharedSpecific(SerialNumber) ;

    if (OpenResult == ApiSuccess)
        {                               // It worked !!
        if (BasicControl->FlagWord    != AARDVARK_FLAG_WORD ||
            InputArea->InterfaceError != 0)
            return ERROR_GEN_FAILURE ;

        if (BasicControl->MeaningVersion != APPLICATION_MEANING_VALUE)
            return ERROR_INVALID_DATA ;

        OutputArea->PCPresent = 1 ;              // Admit we're here!
        return 0 ;          // All right - we must be working!!
        }


    if (MMFile)
        {                       // Decode USB Errors
        if (PCInternal)
            {                   // Well, the driver's running
            switch (PCInternal->USBUsage)
                {
                case USB_ERROR:          return ERROR_DEVICE_NOT_CONNECTED ;

                case STANDARD_DRIVER:    return ERROR_NOT_READY ;

                case FLASH_LOADER:
                case MANUFACTURING_TEST:
                case DRIVER_RESTART:     return ERROR_BUSY ;

                default:                 return ERROR_GEN_FAILURE ;
                }
            }
        }
    else
        {                       // Decode PCI errors
        switch (OpenResult)
            {
            case ApiInvalidAddress:     return ERROR_NOT_READY ;

            case ApiInvalidDeviceInfo:  return ERROR_GEN_FAILURE ;

            case ApiNoActiveDriver:     return ERROR_BAD_UNIT ;

            default:                    return 0x10000000 + OpenResult ;
            }
        }

    return ERROR_GEN_FAILURE ;
    }

/****************************************************************
The EnableInterface call is used to allow users to enter coins
or notes into the system. This would be called when a game is
initialised and ready to accept credit.

Parameters
None

Return Value
None

Remarks
This must be called following the call to OpenMHE before
any coins / notes will be registered.
****************************************************************/
void DLL EnableInterface(void)
    {
    int i ;
    if (!OutputArea) return ;

    OutputArea->StartGuard++ ;

    if (OutputArea->EnableInterfaceCount < 1)
        {                           // Make sure that EnableInterfaceCount hasn't magically gone to zero !
        OutputArea->EnableInterfaceCount = 1 ;
        }


    if (!OutputArea->PeripheralsEnabled)
        {    // We *Can't be doing a Payout, so The PayValue variable should match PaidValue
        OutputArea->PayValue = InputArea->PaidValue ;
        }


    OutputArea->PeripheralsEnabled = OutputArea->EnableInterfaceCount ;

    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        {
        // Now turn on the drive for the input switches
        for (i = 0 ; i < 8 ; ++i)
            {
            OutputArea->Output[i] = TRUE ;
            }
        }


    OutputArea->EndGuard = OutputArea->StartGuard ;
    }




/****************************************************************
The DisableInterface call is used to prevent users from
entering any more coins or notes.

Parameters
None

Return Value
None

Remarks
1. There is no guarantee that a coin or note can not be
successfully read after this call has been made, a
successful read may be in progress.
****************************************************************/
void DLL DisableInterface (void)
    {
    if (!OutputArea) return ;

    OutputArea->StartGuard++ ;

    OutputArea->PeripheralsEnabled = 0 ;

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }




/****************************************************************
The CurrentValue call is used to determine the total value
of all coins and notes read by the money handling equipment
connected to the interface.

Parameters
None

Return Value
The current value, in the lowest denomination of the
currency (i.e. cents / pence etc.) of all coins and notes read.

Remarks
1. The value returned by this call is never reset, but
increments for the life of the interface card. Since
this is a int  (32 bit) integer, the card can accept
£21,474,836.47 of credit before it runs into any rollover
problems. This value is expected to exceed the life of the game.
2. It is the responsibility of the application to keep track
of value that has been used up and to monitor for new
coin / note insertions by increases in the returned value.
3. Note that this value should be read following the call
to OpenMHE and before the call to EnableInterface to establish
a starting point before any coins or notes are read.
****************************************************************/
int DLL CurrentValue(void)
    {
    int  EndGuard ;
    int  Value ;
    if (!InputArea) return 0 ;

    do
        {
        EndGuard = InputArea->EndGuard ;
        Value    = InputArea->ReadValue ;
        } while (EndGuard != InputArea->StartGuard) ;

    return Value ;
    }





/****************************************************************
The PayOut call is used by the PC application to instruct
the interface to pay out coins (or notes).


Parameters
This is the value, in the lowest denomination of the currency
(i.e. cents / pence etc.) of the coins and notes to be paid out.

Return Value
None

Remarks
1. This function operates in value, not coins. It is the
responsibility of the interface to decode this and to choose how
many coins (or notes) to pay out, and from which device to pay
them.
****************************************************************/

static int  CurrentPayStatus = PAY_FINISHED ;       // This is the status of the last activity
                                                    // PayoutStatus itself goes to FINISHED
                                                    // when an errored payout is reported.


static bool UnclearedPrecise = false ;       // This flag indicates that when a precise pay
                                             // Terminates, we should clear all the fields.




static void PayoutFunction(int  NewValue, int TypeFlag) ;

void DLL PayOut(int  NewValue)
    {
    PayoutFunction(NewValue, 0) ;
    }

    // PayoutFunction is the original Payout function, but now includes a flag to
    // set / clear the precision payout flag.
    // This flag is inplicitly set by the application depnding on which
    // function is called. (see PaySpecific())

static void PayoutFunction(int  NewValue, int TypeFlag)
    {
    int  EndGuard ;
    int  Value ;
    int  Status ;
    if (!OutputArea) return ;

    do
        {
        EndGuard    = InputArea->EndGuard ;
        Status      = InputArea->PayoutStatus ;
        Value       = InputArea->PaidValue ;
        } while (EndGuard != InputArea->StartGuard) ;


    if (Status != PAY_ONGOING &&
         Value != OutputArea->PayValue)     // Whoops - we're out of synch
        {
        OutputArea->StartGuard++ ;
        OutputArea->PayValue = Value ;      // Tell card we've noticed
        OutputArea->EndGuard = OutputArea->StartGuard ;
        do                                  // This is so unlikely we can just hang until
            {                               // the card notices
            EndGuard = InputArea->EndGuard ;
            Status   = InputArea->PayoutStatus ;
            } while (EndGuard != InputArea->StartGuard &&
                     Status != PAY_FINISHED) ;
        }

    OutputArea->StartGuard++ ;

    OutputArea->PayValue = OutputArea->PayValue + NewValue ;
    if (BasicControl->FieldVersion >= ESCROW_FIELDS)
        {
        OutputArea->PayValueRandomCheck = OutputArea->PayValue + BasicControl->RandomNumber ;
        }
    OutputArea->PrecisePayFlag = TypeFlag ;

    DESUpdatePaylink() ;                    // Update the DES system (no effect with old DLLs!)

    OutputArea->EndGuard = OutputArea->StartGuard ;

    CurrentPayStatus = PAY_FINISHED ;       // Predict the end!
    }



/****************************************************************
The PayStatus call provides the current status of the payout process.

Parameters

None


Remarks

Following a call to PayOut, the programmer should poll this to
check the progress of the operation.
****************************************************************/




/****************************************************************
 The exact mechanics of the status a bit complicated.


There are a number of returns from the Paylink which need translating.

Basically:
PaidValue != PayValue
    PayoutStatus == PAY_FINISHED
        A new request is being processed
    PAY_ONGING
        The new request has been spotted and is being processed
    Anythine else
        An error has not yet been completed

PaidValue == PayValue
    PayoutStatus == PAY_FINISHED
        The system is idle
        If the status call is made here we have to return the previous result!
    PAY_ONGING
        The payout is still being processed
    Anything else
        Can't happen


****************************************************************/



int DLL LastPayStatus(void)
    {
    int  EndGuard ;
    int  Value ;
    int  Status ;
    if (!InputArea) return PAY_US ;

    do
        {
        EndGuard = InputArea->EndGuard ;
        Status   = InputArea->PayoutStatus ;
        Value    = InputArea->PaidValue ;
        } while (EndGuard != InputArea->StartGuard) ;


    if (Status == PAY_ONGOING)
        {
        return PAY_ONGOING ;
        }


    if (Status == PAY_FINISHED)
        {
        if (Value != OutputArea->PayValue)  // has it really finished, or not yet started?
            {
            return PAY_ONGOING ;
            }
        }
    else
        {                             // An erroneous pay completion
        CurrentPayStatus = Status ;         // Preserve result
        OutputArea->StartGuard++ ;
        OutputArea->PayValue = Value ;      // Tell card we've noticed
        }


    if (UnclearedPrecise)
        {
        UnclearedPrecise = false ;      // "zero" all the precise pay values
        OutputArea->StartGuard++ ;      // There is no problem in doing this twice!
        for (DispenserInterface* Dispenser = InputArea->Dispensers ;
                        Dispenser != 0 ;
                                Dispenser = Dispenser->Next)
            {
            Dispenser->DispenseQuantity = 0 ;
            }
        }

    OutputArea->EndGuard = OutputArea->StartGuard ;    // Sometine this hasn't changed
    return static_cast<PayStatuses>(CurrentPayStatus) ;
    }






/****************************************************************
SetDispenseQuantity

Synopsis
The SetDispenseQuantity call will set the given quantity as the number of
coins (notes) to be dispensed from a specific dispenser on the next call
to PaySpecific ().


Parameters
1.  Index
This parameter specifies the dispenser that is being set up .
2.  Quantity
This sets the quantity of coins (notes) to be dispensed from the indicated
dispenser.
3.  Value
This is provided as a cross check, and must be the value of the
coin / notes dispensed by this dispenser.

Return Value
If the dispenser referenced is valid, and contains coins (notes) of the
specified value, the return value is value of the resultant payout
(i.e. Quantity * Value). If there is a problem in the specification,
then zero is returned.

Remarks
1.  Once a quantity has been set by use of this call, it remains set
through all other Paylink interface calls until cleared as a side
effect of a PaySpecific () call.
2.  Although both are not necessary, both the Index and the Value
parameters are required as a security check.
3.  A non-zero return indicates only that the payout will be attempted -
no reference is made to the operability of the dispenser.
****************************************************************/
int DLL SetDispenseQuantity(int Index,
                            int Quantity,
                            int Value)
    {
    if (!OutputArea
     || !InputArea
     || (BasicControl->FieldVersion < PRECISE_DISPENSE)
     || (InputArea->Dispensers == 0))
        {
        return 0 ;
        }

    if (UnclearedPrecise)
        {
        UnclearedPrecise = false ;      // "zero" all the precise pay values
        for (DispenserInterface* Dispenser = InputArea->Dispensers ;
                        Dispenser != 0 ;
                                Dispenser = Dispenser->Next)
            {
            Dispenser->DispenseQuantity = 0 ;
            }
        }


    DispenserInterface* Dispenser = InputArea->Dispensers ;
    while ((Dispenser != 0)
        && (--Index >= 0))
        {
        Dispenser = Dispenser->Next ;
        }

    if (Dispenser == 0)
        {
        return 0 ;
        }

    if (Dispenser->Value != Value)
        {
        return 0 ;
        }
    Dispenser->DispenseQuantity = Dispenser->Count + Quantity ;
    return Quantity * Value ;
    }



/****************************************************************
PaySpecific

Synopsis
The PaySpecific call takes no parameters. It causes Paylink to
attempt to pay out all the coins (notes) specified by earlier
calls to SetDispenseQuantity().


Parameters
None

Return Value
The total value of the payout being attempted.

Remarks
1.  The only differences between the progress of a payout started
by PaySpecific() and one started by the traditional PayOut() call
is the quantity of the different coins (notes) chosen, and the fact
that there is no "fall over" to a lower value dispenser if a higher
value dispenser is, or becomes, empty.
2.  As with PayOut(), progress is monitored by repeated calls to
LastPayStatus() waiting for PAY_ONGOING to change. Again, as with a
pay out started by PayOut(), the total value paid can be monitored
by calls to CurrentPaid() and the coins (notes) paid for each dispenser
found / monitored using the Count field of the Dispenser blocks
3.  Having transferred the counts set by PaySpecific() to the
Paylink unit for this pay out, the counts are then cleared.
****************************************************************/
int DLL PaySpecific ()
    {
    if (!OutputArea
     || !InputArea
     || (BasicControl->FieldVersion < PRECISE_DISPENSE))
        {
        return 0 ;
        }

    UnclearedPrecise = true ;               // We've used some precise pay values

    DispenserInterface* Dispenser = InputArea->Dispensers ;
    int TotalPayout = 0 ;
    while (Dispenser != 0)
        {
        if (Dispenser->DispenseQuantity != 0)
            {
            TotalPayout += (Dispenser->DispenseQuantity - Dispenser->Count)
                                              * Dispenser->Value ;
            }
        Dispenser = Dispenser->Next ;
        }

    if (TotalPayout > 0)
        {
        PayoutFunction(TotalPayout, 1) ;
        }
    return TotalPayout ;
    }






/****************************************************************
The IndicatorOn / IndicatorOff calls are used by the PC application
to control LED's and indicator lamps connected to the interface.

Parameters
This is the number of the Lamp that is being controlled.

Return Value
None

Remarks
1. Although the interface is described in terms of lamps, any
equipment at all may in fact be controlled by these calls,
depending only on what is physically connected to the interface card.
****************************************************************/
void DLL IndicatorOn (int  IndicatorNumber)
    {
    if (!OutputArea ||
         IndicatorNumber < 0 ||
         IndicatorNumber > NO_OF_LEDS) return ;

    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        {
        IndicatorNumber += 8 ;
        }

    long ValueToSet = OutputArea->PWMMaximum + 1 ;

    if (OutputArea->Output[IndicatorNumber] != ValueToSet)
        {                   // it has changed
        OutputArea->StartGuard++ ;
        OutputArea->Output[IndicatorNumber] = ValueToSet ;
        OutputArea->EndGuard = OutputArea->StartGuard ;
        }
    }



void DLL IndicatorOff(int  IndicatorNumber)
    {
    if (!OutputArea ||
         IndicatorNumber < 0 ||
         IndicatorNumber > NO_OF_LEDS) return ;

    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        {
        IndicatorNumber += 8 ;
        }

    if (OutputArea->Output[IndicatorNumber])
        {                   // it has changed
        OutputArea->StartGuard++ ;
        OutputArea->Output[IndicatorNumber] = 0 ;
        OutputArea->EndGuard = OutputArea->StartGuard ;
        }
    }





/****************************************************************
SetPWM

Synopsis

With the advent of Paylink+ all outputs are now PWM capable. This is
  "switched on" by a new Paylink API call SetPWM and used with a
  new call IndicatorValue.


Parameters
1.  PWMMaximum.
The maximum value for a PWM output - any output set to this value,
  or higher, will be permanently on.

Usage
Until this call is made with a parameter greater than 1, the Paylink
  outputs are either continuously on or continuously off. After this
  call, then calls to IndicatorValue will allow outputs to generate
  a high frequency PWM output, with the specified mark / space ratio.

Note that a call of SetPWM(1) will restore the system to its default
  mode, with all outputs permanently on or off.

****************************************************************/
void DLL SetPWM (int PWMMaximumParam)
    {
    if (!OutputArea
      || PWMMaximumParam < 1)
        {
        return ;
        }

    if (OutputArea->PWMMaximum != PWMMaximumParam)
        {
        OutputArea->StartGuard++ ;
        OutputArea->PeripheralUpdates++ ;    // Flag "something happened"
        OutputArea->PWMMaximum = PWMMaximumParam - 1 ;
        OutputArea->EndGuard = OutputArea->StartGuard ;
        }
    }




/****************************************************************
IndicatorValue

Synopsis

With the advent of Paylink+ all outputs are now PWM capable. This is
  new call IndicatorValue set the value of a PWM output.

Parameters
1.  IndicatorNumber.
This is the number of the Lamp that is being controlled.

2.  PWMValue
The mark space ratio required for this output.

Remarks
Note that an IndicatorValue of zero turns the output off, a value
   of PWMMaximum turns the output permanently on.
****************************************************************/
void DLL IndicatorValue (int IndicatorNumber,
                         int PWMValue)
    {
    if (!OutputArea
      || IndicatorNumber < 0
      || IndicatorNumber > NO_OF_LEDS)
        {
        return ;
        }

    if (OutputArea->Output[IndicatorNumber] != PWMValue)
        {
        OutputArea->StartGuard++ ;
        OutputArea->PeripheralUpdates++ ;    // Flag "something happened"
        OutputArea->Output[IndicatorNumber] = PWMValue ;
        OutputArea->EndGuard = OutputArea->StartGuard ;
        }
    }






/****************************************************************
The calls to SwitchOpens and SwitchCloses are made by the PC
application to read the state of switches connected to the
interface card.


Parameters
This is the number of the switch that is being controlled.

In principle the interface card can support 64 switches,
though note that not all of these may be physically present
within a game cabinet.

Return Value
The number of times that the specified switch has been
observed to open or to close, respectively.

Remarks
1. The value returned by this call is only (and always)
reset by the OpenMHE call.

2. The convention is that at initialisation time all
switches are open.

3. A switch that starts off closed will therefore return a
value of 1 to a SwitchCloses call immediately following the
OpenMHE call.

4. The expression (SwitchCloses(n) == SwitchOpens(n)) will
always return 0 if the switch is currently closed and 1 if
the switch is currently open.

5. The pressing / tapping of a switch by a user will be
detected by an increment in the value returned by
SwitchCloses or SwtichOpens.

6. The user only needs to monitor changes in one of the
two functions (in the same way as most windowing interfaces
only need to provide functions for button up or button
down events).
****************************************************************/
int DLL SwitchOpens (int  SwitchNumber)
    {
    int  EndGuard ;
    int  Value ;
    if (!InputArea) return 0 ;

    do
        {
        EndGuard = InputArea->EndGuard ;
        Value    = InputArea->SwitchOpened[SwitchNumber] ;
        } while (EndGuard != InputArea->StartGuard) ;

    return Value ;
    }



int DLL SwitchCloses(int  SwitchNumber)
    {
    int  EndGuard ;
    int  Value ;
    if (!InputArea) return 0 ;

    do
        {
        EndGuard = InputArea->EndGuard ;
        Value    = InputArea->SwitchClosed[SwitchNumber] ;
        } while (EndGuard != InputArea->StartGuard) ;

    return Value ;
    }




/****************************************************************
ReadLoggedEvent

The ReadEvent call retrieves an event as described above from the system.


Parameters
Number
This is the number of the desired event.
TheEvent
A pointer to a LoggedEventBock object, which is filled in by the call.

Return Value
Zero
The event was not known to the system.
Non Zero
The event has been retrieved.

Remarks
Occasionally system recovery processing may result in a dis-contiguous
sequence of events. These will be indicated by EVT_NULL the event should
be ignored, but the event should sill be marked as processed as it still
may exists in the system.
****************************************************************/
int DLL ReadLoggedEvent(int Number,
                        LoggedEventBlock* TheEvent)
    {
    int  EndGuard ;

    if (!InputArea) return 0 ;
    if (!BasicControl->LoggedEventPointer) return 0 ;

    LoggedEventInterface* EventData = BasicControl->LoggedEventPointer ;
    TheEvent->EventNumber           = Number ;

    do
        {
        EndGuard = InputArea->EndGuard ;

        if ((Number < EventData->EarliestEvent)
         || (Number > EventData->LatestEvent))
            {
            TheEvent->EventType = EVT_NULL ;
            return 0 ;
            }

        TheEvent->EventTime   = EventData->EventTime[Number & EVENT_MASK] ;
        TheEvent->EventType   = EventData->EventTypeInfo[Number & EVENT_MASK] >> 24 ;
        TheEvent->Information = EventData->EventTypeInfo[Number & EVENT_MASK] & 0xffffff ;
        } while (EndGuard != InputArea->StartGuard) ;
    return Number ;
    }



/****************************************************************
SetEventProcessed

Synopsis
When the application has completely finished with all processing
regarding an event, it should mark it as processed in order for the system
to reclaim the buffer space it occupies.

Parameters
Number
This is the number of the processed event. It must be the value last
returned by OldestUnprocessedEvent().

Return Value
None

Remarks
If events are not marked as processed, then the buffer space will
become full and new events not stored.
****************************************************************/
void DLL SetLoggedEventProcessed (int Number)
    {
    if (!InputArea) return ;
    if (!BasicControl->LoggedEventPointer) return ;

    LoggedEventInterface* EventData = BasicControl->LoggedEventPointer ;

    OutputArea->StartGuard++ ;
    OutputArea->PeripheralUpdates++ ;    // Flag "something happened"

    if (Number == EventData->EarliestEvent)
        {
        EventData->EventTime    [Number & EVENT_MASK] = 0 ;
        EventData->EventTypeInfo[Number & EVENT_MASK] = 0 ;
        EventData->ProcessedEvent = Number ;
        EventData->EarliestEvent  = Number + 1 ;
        }
    OutputArea->EndGuard = OutputArea->StartGuard ;
    }



/****************************************************************
OldestUnprocessedEvent

Synopsis
This call retrieves the number of the oldest event on the system
that has not been marked as processed.

Parameters
None

Return Value
Zero
Non unprocessed events are currently available.
Non Zero
The oldest event available that has not yet been retrieved.

Remarks
Note that at system startup this will return zero until the event
history has been recovered, and will continue to return zero at
times as the event log is retrieved from the IOBoard.
****************************************************************/
int DLL OldestUnprocessedLoggedEvent ()
    {
    int  EndGuard ;
    int  Value ;

    if (!InputArea) return 0 ;
    if (!BasicControl->LoggedEventPointer) return 0 ;
    LoggedEventInterface* EventData = BasicControl->LoggedEventPointer ;
    do
        {
        EndGuard = InputArea->EndGuard ;

        Value = EventData->EarliestEvent ;
        if (Value <= EventData->ProcessedEvent)
            {
            Value = 0 ;           // Earlier events have yet been processed
            }

        // OK, is this event actually valid
        if (Value > EventData->LatestEvent)
            {                                   // No, they're all done
            Value = 0 ;
            }
        } while (EndGuard != InputArea->StartGuard) ;
    return Value ;
    }



/****************************************************************
The CurrentPaid call is available to keep track of
the total money paid out because of calls to the
PayOut function.

Parameters
None

Return Value
The current value, in the lowest denomination of the
currency (i.e. cents / pence etc.) of all coins and notes
ever paid out.

Remarks

1. This value that is returned by this function is updated
in real time, as the money handling equipment succeeds in
dispensing coins.

2. The value that is returned by this call is never reset,
but increments for the life of the interface card. It is
the responsibility of the application to keep track of
starting values and to monitor for new coin / note successful
payments by increases in the returned value.

3. Note that this value can be read following the call to
OpenMHE and before the call to EnableInterface to establish
a starting point before any coins or notes are paid out.
****************************************************************/
int DLL CurrentPaid(void)
    {
    int  EndGuard ;
    int  Value ;
    if (!InputArea) return 0 ;

    do
        {
        EndGuard = InputArea->EndGuard ;
        Value    = InputArea->PaidValue ;
        } while (EndGuard != InputArea->StartGuard) ;

    return Value ;
    }





/****************************************************************
Synopsis
This call allows an application to check that the Paylink
and its connection to the PC are operational. It also allow
the application to automatically close down currency acceptance
in the event of any PC malfunction.



Parameters

1. Sequence
A unique number for this call, freely chosen by the application.
2. Timeout
A time in milliseconds before which another CheckOperation() call
must be made in order to continue the normal operation of Paylink.
If zero, then this functionality is inactive.

Return Value
The last Sequence value of which the Paylink unit has been notified,
or -1 if the Paylink does not support this facility.

Remarks
1. In normal operation, Paylink can be expected to have updated the
value to be returned by this within 100 milliseconds of the previous
call. It is suggested that this call is made every 500 milliseconds
or longer to allow for transient delays.
2. If the Timeout expires, Paylink will “silently” disable all the
acceptors that are connected to it. The next call to CheckOperation()
will “silently” re-enable them.
****************************************************************/
int DLL CheckOperation(int  Sequence, int  Timeout)
    {
    int  Result = -1 ;
    if (BasicControl->FieldVersion >= STRING_FIELDS)
        {                  // all three are independant, so there's no "Guard" problem
        Result = InputArea->ReturnedSequence ;
        OutputArea->InputSequence = Sequence ;
        OutputArea->TimeoutPeriod = Timeout ;
        }
    return Result ;
    }





/****************************************************************
Detect updates to the data presented to the API by the firmware.

The fact that the value returned by CurrentUpdates has changed,
prompts the application to re-examine all the variable data
in which it is interested.

Parameters
None

Return Value
Technically CurrentUpdates returns the number of times that the
API data has been updated since the PC system initialised.
In practice, only changes in this value are significant.

Remarks
1. It is possible that the value could change without
any visible data changing.
****************************************************************/
int DLL CurrentUpdates (void)
    {
    return InputArea->EndGuard ;
    }





/****************************************************************
The SystemStatus call provides a single summary of the status
all the money handling equipment connected to the interface.
It is a logical OR of the status of all of the individual
device statuses.

Parameters
None

Return Value
Zero if all devices are completely normal.
Negative if there is a major problem with any device.

Remarks

This returns a logical OR of the status of all of the individual
device statuses.
         **************************************************
          As this is no longer used it's not in AesImhei.h
          We generate code for backwards compatiblity

****************************************************************/

extern "C" int DLL SystemStatus (void)
    {
    int                 EndGuard ;
    int                 Status ;
    AcceptorInterface*  Acceptor ;
    DispenserInterface* Dispenser ;

    if (!InputArea) return INTERFACE_FAILED ;

    do
        {
        EndGuard = InputArea->EndGuard ;

        Status   = OurInterfaceErrror ;
        Status  |= InputArea->InterfaceError ;

        Acceptor = InputArea->Acceptors ;
        while (Acceptor)
            {
            Status  |= Acceptor->Status ;
            Acceptor = Acceptor->Next ;
            }

        Dispenser = InputArea->Dispensers ;
        while (Dispenser)
            {
            Status   |= Dispenser->Status ;
            Dispenser = Dispenser->Next ;
            }
        } while (EndGuard != InputArea->StartGuard) ;

    return Status ;
    }









/****************************************************************
Des Lock
A DES system can be basically secured in one of two ways:
1. The PC and Paylink are both in the same secure enclosure.
    Here there is no need to provide any security control over
    the USB connection - access to the USB cable is equivalent to
    access to the hard disc of the PC, and this level of access c
    annot be contracted by electronic means.
2. The Paylink, or more particularly the USB connection, is
    accessible from the general cabinet area.
    Here a fraud attempt is possible by removing existing USB cable
    and connecting the Paylink USB socket to the fraudster’s laptop.

To prevent this security problem, the PC application can use DES lock,
the functions associated with DES lock are described in this section.
(Only) While Paylink is DES locked:
• The PC and Paylink cross check that each other are using the same key.
• The Payout call only works if the key has been matched
• New DES peripherals can’t be added without Paylink being unlocked
• Pressing the DES button deletes all DES keys (peripheral and Paylink) and unlocks Paylink

Some points about this system are:
• Updates to existing Paylink applications are optional
    - although there can be a security risk.
• The DES lock key is provided by the PC, and so can be held on a read only disk system.
• A DES lock aware application will spot if a different
    Paylink is substituted, or if the Paylink is unlocked in order to change the peripherals
****************************************************************/

static unsigned char OurDESKey[8] ;
static int  ReplyChangeSnapshot[2] ;
static int  OurExpectedResponse[2] = {-1, -1} ;


/****************************************************
This function takes the Challenge from Paylink and updates
it to give the reply - if the reply is not correct
then the Payout function won't work!!!
*****************************************************/
static void DESUpdatePaylink(void)
    {
    DESEncrypt   Des ;

    if (BasicControl->DESSecure != 0)                          // Check that we're a DES system!
        {
        int  PaylinkChallengeCount = InputArea->DESPaylinkChallengeCount ;
        if (OutputArea->DESPCResponseCount != PaylinkChallengeCount)
            {
            // Encrypt the value supplied by Paylink using our current key
            unsigned char  PaylinkChallenge[8] ;
            LongToKey(PaylinkChallenge, 0, InputArea->DESPaylinkChallenge[0])
            LongToKey(PaylinkChallenge, 4, InputArea->DESPaylinkChallenge[1])
            Des.Encrypt(OurDESKey, PaylinkChallenge) ;

            // And "send" it to Paylink.
            OutputArea->DESReplyToPaylink[0] = KeyToLong(PaylinkChallenge, 0) ;
            OutputArea->DESReplyToPaylink[1] = KeyToLong(PaylinkChallenge, 4) ;
            OutputArea->DESPCResponseCount   = PaylinkChallengeCount ;
            }
        }
    }









/****************************************************************
Inform the DLL that of the current key that is to be shared between the PC and Paylink


Parameters
1. Key
The 8 byte DES key previously applied using the DESLockSet function.


Return Value
None.

Remarks
1. The Key should be as unpredictable as possible. Ideally, it will
    be a random number generated by the application and saved for future use.
    For system with read only file systems, it could be derived from Processor ID or similar.

2. The DESStatus function (see below) will enable the application to
    determine the success of this function.
****************************************************************/
void DLL DESSetKey (char Key[8])
    {
    DESEncrypt   Des ;


    if (BasicControl->DESSecure != 0)                          // Check that we're a DES system!
        {
        OutputArea->StartGuard++ ;

        // Save the supplied key
        memcpy(OurDESKey, Key, sizeof OurDESKey) ; // Save the supplied key
        for (unsigned int i = 0 ; i < sizeof OurDESKey ; ++i)
            {                                      // Lose the parity bits
            OurDESKey[i] &= DES_PARITY ;
            }

        // Save current reply values from Paylink (so we can spot them change)
        ReplyChangeSnapshot[0] = InputArea->DESReplyToPC[0] ;
        ReplyChangeSnapshot[1] = InputArea->DESReplyToPC[1] ;


        // Generate an output random number for Paylink to encrypt
        srand((unsigned) time(NULL));
        int  NewRand = rand() ;
        while (NewRand == OutputArea->DESPCChallenge[0])     // It has to be different !!!!!!
            {
            NewRand = rand() ;
            }
        OutputArea->DESPCChallenge[0] = NewRand ;
        OutputArea->DESPCChallenge[1] = rand() ;

        // Encrypt *this* challenge value using our new key
        unsigned char  PCChallenge[8] ;
        LongToKey(PCChallenge, 0, OutputArea->DESPCChallenge[0])
        LongToKey(PCChallenge, 4, OutputArea->DESPCChallenge[1])
        Des.Decrypt(OurDESKey, PCChallenge) ;


        OurExpectedResponse[0] = KeyToLong(PCChallenge, 0) ;           // and save our expected response
        OurExpectedResponse[1] = KeyToLong(PCChallenge, 4) ;
        OutputArea->DESPCChallengeCount++ ;                            // Tell Paylink to process it

        OutputArea->DESPCResponseCount = -1 ;                          // and tell *us* to process it!
        DESUpdatePaylink() ;                                           // Update Paylink using the new key
        OutputArea->EndGuard = OutputArea->StartGuard ;
        }
    }





/****************************************************************
Apply the lock using the key quoted in this function call.

Parameters
1. Key
    The 8 byte DES key chosen by the PC.

Return Value
None

Remarks
If the Paylink is already DES Locked, then this function will not
change the key unless DESSetKey has already matched the key stored by Paylink.

****************************************************************/
void DLL DESLockSet (char Key[8])
    {
    OutputArea->StartGuard++ ;
    if (BasicControl->DESSecure != 0)
        {
        OutputArea->DESLocked = (InputArea->DESLocked + 1) | 1 ;            // Larger & bottom bit set
        OutputArea->NewDESKey[0] = KeyToLong(Key, 0) & DES_PARITY_LONG ;
        OutputArea->NewDESKey[1] = KeyToLong(Key, 4) & DES_PARITY_LONG ;

        OutputArea->DESPCChallengeCount++ ;                     // We've effectively sent a new challenge
        }
    OutputArea->EndGuard = OutputArea->StartGuard ;
    }





/****************************************************************
Clear any previous applied DES lock.

Parameters
None

Return Value
None

Remarks
1. If the Paylink is already DES Locked, then this function will
    not unlock Paylink unless DESSetKey has already matched the key stored by Paylink.

2. This function differs from pressing the DES button in that
    keys for the existing DES peripherals are not lost. This can therefore
    be used by application when an engineer wishes to only update a single peripheral.

****************************************************************/
void DLL DESLockClear (void)
    {
    OutputArea->StartGuard++ ;
    if (BasicControl->DESSecure != 0)
        {
        OutputArea->DESLocked = (InputArea->DESLocked + 2) & ~1 ;            // Larger & bottom bit clear
        }
    OutputArea->EndGuard = OutputArea->StartGuard ;
    }





/****************************************************************
Synopsis
The DESStatus call provides the current status of the DES lock system.

Parameters
None

Return Values.
     Mnemonic        Value   Meaning
     DES_UNLOCKED   = 0 ,    The Paylink is unlocked
     DES_MATCH      = 1 ,    DES Key matched by Paylink and PC
     DES_NOT        = -1,    Not a DES Paylink
     DES_WRONG      = -2,    Paylink wrong key
     DES_CHECKING   = -3,    DES Key checking is still being performed.
     DES_APPLYING   = -4     DES Lock is being applied

Remarks
1. Following a call to DESLockSet, or DESSetKey, the programmer
    should poll this to check the operation.

2. The Paylink system is only operation when either DES_UNLOCKED
    or DES_MATCH has been returned by this function.
****************************************************************/
int DLL DESStatus (void)
    {
    if (BasicControl->DESSecure == 0)                          // Check that we're a DES system!
        {
        return DES_NOT ;
        }

    // Keep the Paylink system up to date
    OutputArea->StartGuard++ ;
    DESUpdatePaylink() ;
    OutputArea->EndGuard = OutputArea->StartGuard ;


    if ((InputArea->DESLocked & 1) == 0)
        {
        return DES_UNLOCKED ;
        }

    // Are we waiting for Paylink to send us a reply to our last change?
    if (OutputArea->DESPCChallengeCount != InputArea->DESPaylinkResponseCount)
        {
        if (OutputArea->NewDESKey[0] != 0)
            {
            return DES_APPLYING ;
            }
        else
            {
            return DES_CHECKING ;
            }
        }

    // IF the current reply matches, then everything is OK
    if (OurExpectedResponse[0] == InputArea->DESReplyToPC[0]
     && OurExpectedResponse[1] == InputArea->DESReplyToPC[1])
        {
        return DES_MATCH ;
        }
    else
        {
        return DES_WRONG ;
        }
    }









/****************************************************************
NextEvent

Synopsis
All Acceptor / hopper events received will be queued (in a short queue).
These can be retrieved with NextEvent.

Parameters
1. EventDetail
   NULL or the address of the strucutre at which to store the details of
   the event.

Return Value
The return code is 0 (IMHEI_NULL) if no message is available, otherwise
it is the next event.

Remarks
1. In the event that one or more event is missed, the code
   IMHEI_OVERFLOW will replace the missed events.
2. As note, coin & hoper event codes do not overlap, the Device
   parameter can often be set to NULL as the device is implicit in the event.

****************************************************************/

int DLL NextEvent(EventDetailBlock* EventDetail)
    {
    if (!OutputArea || !InputArea) return 0 ;

    unsigned int  Event ;
    int EndGuard ;

    if (OutputArea->EventsRead == InputArea->EventsWritten)
        return 0 ;

    do
        {
        EndGuard = InputArea->EndGuard ;
        Event    = InputArea->TheEvent ;
        } while (EndGuard != InputArea->StartGuard) ;

    OutputArea->StartGuard++ ;

    OutputArea->EventsRead = InputArea->EventsWritten ;           // Acknowledge we've got it
    OutputArea->PeripheralUpdates++ ;    // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;


    if (EventDetail)
        {
        EventDetail->EventCode      = (unsigned short) Event & 0xffff ;
        EventDetail->RawEvent       = (unsigned short)(Event >> 16) & 0xff ;
        EventDetail->DispenserEvent = (unsigned char) (Event >> 31) ;
        EventDetail->Index          = (unsigned char) (Event >> 24) & 0x7f ;
        }

    return Event & 0xffff ;
    }




/****************************************************************
NextAcceptorEvent
NextDispenserEvent
NextSystemEvent

Synopsis
These calls provide controlled access to exactly the same set of events
as the NextEvent call described above.

The difference is that, rather than providing access to one single queue
with all events, these provide access to a number of queues. One independent
queue is provided for each acceptor is the system, one for each dispenser
in the system, and one  final queue for all system oriented events.


Parameters

1. Number
The same value as that used in a call to ReadxxxDetails. All events
returned will have an Index value equal to this.

2. EventDetail
NULL, or the address of the single structure at which to store more
details of the event given by the return value.

Return Value

Remarks
1. If these calls are used in a system that also calls NextEvent,
    the result is undefined.
2. Systems with more than 32 acceptors or dispensers should not
    use these calls
3. Un-accessed queues will silently discard events.
****************************************************************/

//
//
//
// This section handles transfering the information from one incoming queue
// into the various sepcfic queues
//
//
//

#include "Queue.h"
#define UNIT_TYPE_MASK ~0x03f

// We want to have an array of unsigned long RTQueues, all 64 elements long.
// To achieve this, we create a dedicated class, with a default constructor.
class Queue32 : public RTQueue<unsigned int>
    {
  public:
    inline Queue32() : RTQueue<unsigned int>(6) {}
    } ;


static Queue32 SystemEvents ;                // 32 element event system queue
static Queue32 AcceptorEvents[32] ;          // 32 element event queues for 32 Acceptors
static Queue32 DispenserEvents[32] ;         // 32 element event queues for 32 Dispensers


static void ReadPaylinkQueue()
    {
    // We can only get one event at a time!
    unsigned int  Event ;
    int           EndGuard ;
    if (!OutputArea || !InputArea) return ;

    if (OutputArea->EventsRead == InputArea->EventsWritten)
        return ;

    do
        {
        EndGuard = InputArea->EndGuard ;
        Event    = InputArea->TheEvent ;
        } while (EndGuard != InputArea->StartGuard) ;

    OutputArea->StartGuard++ ;

    OutputArea->EventsRead = InputArea->EventsWritten ;           // Acknowledge we've got it
    OutputArea->PeripheralUpdates++ ;    // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;

    // Now store this into a queue
    unsigned char  DispenserEvent = (unsigned char) (Event >> 31) ;
    unsigned char  Index          = (unsigned char) (Event >> 24) & 0x1f ;      // We only allow 32 units !!
    unsigned short EventCode      = (unsigned short) Event & 0xffff ;

    if ((EventCode & UNIT_TYPE_MASK) == 0)
        {
        SystemEvents.QPut(Event) ;
        }
    else if (DispenserEvent)
        {
        DispenserEvents[Index].QPut(Event) ;
        }
    else
        {
        AcceptorEvents[Index].QPut(Event) ;
        }
    }

//
//
//
// Now the processing for a queue
//
//
//
static int NextEventFromQueue(Queue32* TheQueue, EventDetailBlock* EventDetail)
    {
    if (TheQueue->QStatus() == Q_NONE)
        {
        return 0 ;
        }
    unsigned int Event = TheQueue->QGet() ;

    if (EventDetail)
        {
        EventDetail->EventCode      = (unsigned short) Event & 0xffff ;
        EventDetail->RawEvent       = (unsigned short)(Event >> 16) & 0xff ;
        EventDetail->DispenserEvent = (unsigned char) (Event >> 31) ;
        EventDetail->Index          = (unsigned char) (Event >> 24) & 0x7f ;
        }

    return Event & 0xffff ;
    }

//
//
//
// Now the interface functions
//
//
//

int DLL NextAcceptorEvent(int Number, EventDetailBlock* EventDetail)
    {
    ReadPaylinkQueue() ;

    if (Number > 31)
        {
        return 0 ;
        }
    return NextEventFromQueue(&AcceptorEvents[Number], EventDetail) ;
    }



int DLL NextDispenserEvent(int Number, EventDetailBlock* EventDetail)
    {
    ReadPaylinkQueue() ;

    if (Number > 31)
        {
        return 0 ;
        }
    return NextEventFromQueue(&DispenserEvents[Number], EventDetail) ;
    }




int DLL NextSystemEvent(EventDetailBlock* EventDetail)
    {
    ReadPaylinkQueue() ;

    return NextEventFromQueue(&SystemEvents, EventDetail) ;
    }






/****************************************************************
The AvailableValue call is available to keep track of how much
money is available in the coin (or note) dispensers.

Parameters

None

Return Value

The approximate minimum value, in the lowest denomination
of the currency (i.e. cents / pence etc.) of all coins and
notes that could be paid out.

Remarks
The accuracy of the value returned by this call is entirely
dependent upon the accuracy of the information returned by
the money dispensers.

If no information is obtainable, this returns 10,000 if at
least one dispenser is working normally, and zero if all
dispensers are failing to pay out.
****************************************************************/
int DLL AvailableValue (void)
    {
    int  EndGuard ;
    int  Value ;
    if (!InputArea) return 0 ;

    do
        {
        EndGuard = InputArea->EndGuard ;
        Value    = InputArea->AvailableValue ;
        } while (EndGuard != InputArea->StartGuard) ;

    return Value ;
    }




/****************************************************************
The ValueNeeded call provides an interface to an optional
credit card acceptor unit.

It is not envisaged that this would be used within many systems,
but may be used, for example, in vending applications.


Parameters
The figure that CurrentValue is required to reach before
the next event can happen.

Return Value
None

Remarks
1. This function does not necessarily have any affect
on the system. If the MHE includes a credit card acceptor,
or similar, then the MHE interface unit will arrange for the
next use of that unit to bring CurrentValue up to latest
figure supplied by this routine.

2. If CurrentValue is greater or equal to the last supplied
figure then any such acceptors are disabled.
****************************************************************/
void DLL ValueNeeded(int  Amount)
    {
    if (!OutputArea) return ;

    OutputArea->StartGuard++ ;

    OutputArea->ValueNeeded = Amount ;

    OutputArea->PeripheralUpdates++ ;           // Flag "something happened"
    OutputArea->EndGuard = OutputArea->StartGuard ;
    }





/****************************************************************
SerialNumber

Synopsis
The SerialNumber call provides access to the electronic serial number
stored on the device.

Parameters
None

Return Value
32 bit serial number.

Remarks
1. A serial number of -1 indicates that a serial number has not been
set in the device.
2. A serial number of 0 indicates that the device firmware does not
support serial numbers
****************************************************************/
int DLL SerialNumber (void)
    {
    return BasicControl->ElectronicSerialNumber ; // This doesn't ever change!
    }




/****************************************************************
EscrowEnable - Change the mode of operation of all escrow capable
acceptors to hold inserted currency in escrow until a call of
EscrowAccept.

The EscrowEnable call is used to start using the escrow system

Parameters
None

Return Value
None
****************************************************************/
void DLL EscrowEnable (void)
    {
    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        return ;             // Card doesn't do escrow

    if (!OutputArea)
        return ;

    OutputArea->StartGuard++ ;

    OutputArea->EscrowEnabled = TRUE ;
    OutputArea->PeripheralUpdates++ ;          // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }



/****************************************************************
EscrowDisable - Change the mode of operation of all escrow capable
acceptors back to the default mode in which all currency is fully
accepted on insertion

Parameters
None

Return Value
None

Remarks
1. If any currency is currently held in escrow when this call
is made, it will be accepted without comment.
****************************************************************/
void DLL EscrowDisable (void)
    {
    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        return ;             // Card doesn't do escrow

    if (!OutputArea)
        return ;

    OutputArea->StartGuard++ ;

    OutputArea->EscrowEnabled = FALSE ;
    OutputArea->PeripheralUpdates++ ;              // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }


/****************************************************************
EscrowThroughput - Determine the cumulative monetary value that
has been held in escrow since the system was reset.

The EscrowThroughput call is used to determine the cumulative total
value of all coins and notes read by the money handling equipment
that have ever been held in escrow.

Parameters
None

Return Value
The current value, in the lowest denomination of the currency
(i.e. cents / pence etc.) of all coins and notes read.

Remarks
1. It is the responsibility of the application to keep track of
value that has been accepted and to monitor for new coin / note
insertions by increases in the returned value.

2. Note that this value should be read following the call to OpenMHE
and before the call to EnableInterface / EscrowEnable to establish a
starting point before any coins or notes are read.

3. If the acceptor auto-returns the coin / note then this value will
fall to its previous value. This can (potentially) occur after a
call to EscrowAccept() or EscrowReturn() if the acceptor has already
started its return sequence.


****************************************************************/
int DLL EscrowThroughput (void)
    {
    int  EndGuard ;
    int  Value ;

    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        return 0 ;             // Card doesn't do escrow

    if (!InputArea) return 0 ;

    do
        {
        EndGuard = InputArea->EndGuard ;
        Value    = InputArea->EscrowValueRead ;
        } while (EndGuard != InputArea->StartGuard) ;

    return Value ;
    }





/****************************************************************
EscrowAccept - If the acceptor that was last reported as holding
currency in escrow is still in that state, this call will cause
it to accept that currency.

Parameters
None

Return Value
None

Remarks
1. If a second acceptor has (unreported) currency in escrow
at the time this call is made, it will immediately cause the
EscrowThroughput to be updated.
2. If no currency is currently held in escrow when this call is
made, it will be silently ignored.
****************************************************************/
void DLL EscrowAccept (void)
    {
    int  AmountAccepted ;

    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        return ;             // Card doesn't do escrow

    if (!OutputArea)
        return ;

    OutputArea->StartGuard++ ;

    // Accept everything that's not Returned
    AmountAccepted = EscrowThroughput() - OutputArea->EscrowValueReturned ;

    OutputArea->EscrowValueAccepted = AmountAccepted ;
    OutputArea->PeripheralUpdates++ ;       // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }




/****************************************************************
EscrowReturn - If the acceptor that was last reported as holding
currency in escrow is still in that state, this call will cause
it to Return that currency.

Parameters
None

Return Value
None

Remarks
1. If a second acceptor has (unreported) currency in escrow at
the time this call is made, it will immediately cause the
EscrowThroughput to be updated.
2. If no currency is currently held in escrow when this call
is made, it will be silently ignored.
****************************************************************/
void DLL EscrowReturn (void)
    {
    int  AmountAccepted ;

    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        return ;             // Card doesn't do escrow

    if (!OutputArea)
        return ;

    OutputArea->StartGuard++ ;

    // Return everything that's not accepted
    AmountAccepted = EscrowThroughput() - OutputArea->EscrowValueAccepted ;
    OutputArea->EscrowValueReturned = AmountAccepted ;
    OutputArea->PeripheralUpdates++ ;      // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }





/****************************************************************
BarcodeEnable - Change the mode of operation of all Barcode capable
acceptors to accept tickets with barcodes on them.

The BarcodeEnable call is used to start using the Barcode system

Parameters
None

Return Value
None
****************************************************************/
void DLL BarcodeEnable (void)
    {
    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        return ;             // Card doesn't do escrow

    if (!OutputArea)
        return ;

    OutputArea->StartGuard++ ;

    OutputArea->BarcodeEnabled = TRUE ;
    OutputArea->PeripheralUpdates++ ;          // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }




/****************************************************************
BarcodeDisable - Change the mode of operation of all Barcode
capable acceptors back to the default mode in which only
currency is accepted.

Parameters
None

Return Value
None

Remarks
1. If a Barcoded ticket is currently held when this call is made,
it will be returned without comment.
****************************************************************/
void DLL BarcodeDisable (void)
    {
    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        return ;             // Card doesn't do escrow

    if (!OutputArea)
        return ;

    OutputArea->StartGuard++ ;

    OutputArea->BarcodeEnabled = FALSE ;
    OutputArea->PeripheralUpdates++ ;          // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }





/****************************************************************
BarcodeInEscrow - This is the regular “polling” call that the
application should make into the DLL to obtain the current
status of the barcode system. If a barcode is read by an acceptor
it will be held in escrow and this call will return true in
notification of the fact.

Parameters
1. BarcodeString
A pointer to a buffer of at least 19 characters into which the
last barcode read from any acceptor is placed. This will be all
NULL if no barcoded ticket has been read since system start-up.

2. BufferLength
The available length of the buffer. This can be larger than 19 to
accomodate increases in barcode length.

Return Value
The return value is true if there is a barcode ticket currently
held in an Acceptor, flase if there is not.

Remarks
1. There is no guarantee that at the time the call is made the
acceptor has not irrevocably decided to auto-eject the ticket.
****************************************************************/
int DLL BarcodeInEscrow (char BarcodeString[19])
    {
    return BarcodeInEscrowExt(BarcodeString, 19) ;
    }

int DLL BarcodeInEscrowExt (char* BarcodeString, int BufferLength)
    {
    int  EndGuard ;
    int  Status ;
    int BasicLength = BufferLength ;
    if (BasicLength > 20)
        {
        BasicLength = 20 ;
        }
    int ExtLength   = BufferLength - BARCODE_DIGITS ;

    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        return FALSE ;             // Card doesn't do escrow

    if (!OutputArea)
        return FALSE ;

    if (!InputArea) return 0 ;

    do
        {
        EndGuard = InputArea->EndGuard ;
        Status   = InputArea->BarcodesReadToEscrow >
                                 (OutputArea->BarcodesReturned +
                                  OutputArea->BarcodesAccepted) ;
        memcpy(BarcodeString, (char*)&InputArea->BarcodeInEscrow[0], BasicLength) ;
        if (ExtLength > 0)
            {
            memcpy(BarcodeString + BARCODE_DIGITS, (char*)&InputArea->BarcodeInEscrowExt[0], ExtLength) ;
            }
        } while (EndGuard != InputArea->StartGuard) ;


    if (BufferLength > BARCODE_NEW_DIGITS)
        {                           // Terminate max length string.
        BarcodeString[BufferLength] = 0 ;
        }

    return Status ;
    }






/****************************************************************
BarcodeStacked - Following a call to BarcodeAccept the system may
complete the reading of a barcoded ticket. If it does, then the
count returned by BarcodeStacked will increment. There is no
guarantee that this will take place, so the application should
continue to poll BarcodeInEscrow.

Parameters
1. BarcodeString
A pointer to a buffer of at least 19 characters into which the
last barcode read from any acceptor is placed. This will be all
NULL if no barcoded ticket has been read since system start-up.

2. BufferLength
The available length of the buffer. This can be larger than 19 to
accomodate increases in barcode length.

Return Value
The count of all the barcoded tickets that have been stacked since
system start-up. An increase in this value indicates that the
current ticket has been stacked - its contents will be in the
BarcodeString buffer.

Remarks
2. It is the responsibility of the application to keep track of
the number of tickets that have been accepted and to monitor for
new insertions by increases in the returned value.

3. Note that this value should be read following the call to
OpenMHE and before the call to EnableInterface / BarcodeEnable
to establish a starting point before any new tickets are read.
****************************************************************/
int DLL BarcodeStacked (char BarcodeString[19])
    {
    return BarcodeStackedExt(BarcodeString, 19) ;
    }

int DLL BarcodeStackedExt (char* BarcodeString, int BufferLength)
    {
    int  EndGuard ;
    int  Count ;
    int BasicLength = BufferLength ;
    if (BasicLength > 20)
        {
        BasicLength = 20 ;
        }
    int ExtLength   = BufferLength - BARCODE_DIGITS ;

    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        return FALSE ;             // Card doesn't do escrow

    if (!OutputArea)
        return FALSE ;

    if (!InputArea) return 0 ;

    do
        {
        EndGuard = InputArea->EndGuard ;
        Count    = InputArea->BarcodeTicketsStacked ;
        memcpy(BarcodeString, (char*)&InputArea->BarcodeStacked[0], BasicLength) ;
        if (ExtLength > 0)
            {
            memcpy(BarcodeString + BARCODE_DIGITS, (char*)&InputArea->BarcodeStackedExt[0], ExtLength) ;
            }
        } while (EndGuard != InputArea->StartGuard) ;

    if (BufferLength > BARCODE_NEW_DIGITS)
        {                           // Terminate max length string.
        BarcodeString[BufferLength] = 0 ;
        }

    return Count ;
    }







/****************************************************************
BarcodeAccept - If the acceptor that was last reported as holding
a Barcode ticket is still in that state, this call will cause it to
accept that currency.

Parameters
None

Return Value
None

Remarks
1. If a second acceptor has (unreported) currency in Barcode
at the time this call is made, it will immediately cause the
BarcodeTicket to be updated.
2. If no ticket is currently held when this call is made, it
will be silently ignored.
****************************************************************/
void DLL BarcodeAccept (void)
    {
    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        return ;             // Card doesn't do escrow

    if (!OutputArea)
        return ;

    OutputArea->StartGuard++ ;

    OutputArea->BarcodesAccepted = InputArea->BarcodesReadToEscrow -
                                   OutputArea->BarcodesReturned  ;
    OutputArea->PeripheralUpdates++ ;       // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }






/****************************************************************
BarcodeReturn - If the acceptor that was last reported as holding
a Barcode ticket is still in that state, this call will cause it to
return that currency.

Parameters
None

Return Value
None

Remarks
1. If a second acceptor has (unreported) currency in Barcode at
the time this call is made, it will immediately cause the
BarcodeTicket to be updated.
2. If no ticket is currently held when this call is made,
it will be silently ignored.
****************************************************************/
void DLL BarcodeReturn (void)
    {
    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        return ;             // Card doesn't do escrow

    if (!OutputArea)
        return ;

    OutputArea->StartGuard++ ;

    OutputArea->BarcodesReturned = InputArea->BarcodesReadToEscrow -
                                   OutputArea->BarcodesAccepted ;
    OutputArea->PeripheralUpdates++ ;       // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }







/****************************************************************
BarcodePrint - This call is used to print a barcoded ticket,
if the IMHEI system supports a printer.

Parameters
1. TicketContents.
Pointer to a TicketDescription structure that hold pointers to
the strings that the application is “filling in”. NULL pointers
will cause the relevant fields to default (usually to blanks).

typedef struct
    {
    int     TicketType ;          // The "template" for the ticket
    char*   BarcodeData ;
    char*   AmountInWords ;
    char*   AmountAsNumber ;      // But still a string
    char*   MachineIdentity ;
    char*   DatePrinted ;
    char*   TimePrinted ;
    } TicketDescription ;


Return Value
None

Remarks
1. There are a number of fields that can be printed a barcode ticket.
Rather than provide a function with a large number of possibly null
parameters, we use a structure, which may have fields added to end.
The user should ensure that all unused pointers are zero.
2. Before issuing this call the application should ensure that
BarcodePrintStatus has returned a status of PRINTER_IDLE
3. The mechanics of the priniting mechanism rely on BarcodePrintStatus
being called regularly after this call, in order to “stage” the data
to the interface.
****************************************************************/
static char* PrinterOutputString = 0 ;
static char* PrinteroutputPoint = 0 ;
static int   PrinterLength = 0 ;

// We create one int  string
void DLL BarcodePrint (TicketDescription* TicketContents)
    {
    unsigned int j, i;
    char Template[128] ;
    char PrintableBarcode[45 + 4] ;

    if (!(BarcodePrintStatus() & PRINTER_IDLE) || !TicketContents->BarcodeData)
        {
        return ;    //either no printer, or busy, or faulty etc.
        }

    // Get Template number as a string
    sprintf(Template, "%d", TicketContents->TicketType) ;

    // Format a "human readable" barcode string ;
    for (i = 0, j = 0 ; i < strlen(TicketContents->BarcodeData) ; ++i, ++j)
        {
        PrintableBarcode[j] = TicketContents->BarcodeData[i] ;
        if ((i & 3) == 1)
            {
            PrintableBarcode[++j] = '-' ;
            }
        }

    if (((i - 1) & 3) == 1)
        {
        PrintableBarcode[j - 1] = 0 ;
        }

    // At this point we have to find out the printer type from
    // the firmware, so that we can create the single string
    // with components in the correct order

    // Ah - Fancy that, it's a GEN 2!!!!!

    PrinterLength = strlen(Template) + 128 ;
    PrinterLength += strlen(PrintableBarcode) ;
    PrinterLength += strlen(PrintableBarcode) ;
    PrinterLength += strlen(TicketContents->BarcodeData) ;

    if (TicketContents->DatePrinted)
        PrinterLength += strlen(TicketContents->DatePrinted) ;
    if (TicketContents->TimePrinted)
        PrinterLength += strlen(TicketContents->TimePrinted) ;
    if (TicketContents->AmountInWords)
        PrinterLength += strlen(TicketContents->AmountInWords) ;
    if (TicketContents->AmountAsNumber)
        PrinterLength += strlen(TicketContents->AmountAsNumber) ;
    if (TicketContents->MachineIdentity)
        PrinterLength += strlen(TicketContents->MachineIdentity) ;

    PrinterOutputString  = new char[PrinterLength] ;


    strcpy(PrinterOutputString, "^C|^^P|") ;            // Reset errors & start print
    strcat(PrinterOutputString, Template) ;
    strcat(PrinterOutputString, "|1|||||") ;
    strcat(PrinterOutputString, PrintableBarcode) ;
    strcat(PrinterOutputString, "|") ;
    strcat(PrinterOutputString, PrintableBarcode) ;
    strcat(PrinterOutputString, "|") ;

    if (TicketContents->DatePrinted)
        strcat(PrinterOutputString, TicketContents->DatePrinted) ;
    strcat(PrinterOutputString, "|") ;

    if (TicketContents->TimePrinted)
        strcat(PrinterOutputString, TicketContents->TimePrinted) ;
    strcat(PrinterOutputString, "|") ;

    if (TicketContents->AmountInWords)
        strcat(PrinterOutputString, TicketContents->AmountInWords) ;
    strcat(PrinterOutputString, "|") ;

    if (TicketContents->AmountAsNumber)
        strcat(PrinterOutputString, TicketContents->AmountAsNumber) ;
    strcat(PrinterOutputString, "|") ;

    if (TicketContents->MachineIdentity)
        strcat(PrinterOutputString, TicketContents->MachineIdentity) ;
    strcat(PrinterOutputString, "|") ;

    strcat(PrinterOutputString, TicketContents->BarcodeData) ;
    strcat(PrinterOutputString, "|^") ;

    PrinterLength        = strlen(PrinterOutputString) ;
    PrinteroutputPoint   = PrinterOutputString ;

    //
    // O.K. - Start the transfer
    //

    CommandBlock Command ;         // We transfer data as a "command"
            Command.Code   = DATA_FOR_PRINTER ;
            Command.Length = PRINTER_DATA_BLOCK ;
    memmove(Command.Data,    PrinteroutputPoint, Command.Length) ;

    WriteInterfaceBlock(-1, (char *)&Command, 8 + Command.Length) ;
    }







/****************************************************************
BarcodeTicketPrint - This call is used to print a totally general ticket,
if the IMHEI system supports a printer.

Parameters
1. Template Number
If relevant, the template of the ticket to be printed.

2. TicketContents.
Pointer to a string that contains all the variable data that is required.


Return Value
None

Remarks
1. This function differs from the previous in that it provides for a totally
arbitrary set of data to be output to the ticket.
2. All the variable fields for the ticket should be concatenated with
a '|' character between them.
Omitted / Null fields should be indicated by two succesive || characters.
3. Before issuing this call the application should ensure that
BarcodePrintStatus has returned a status of PRINTER_IDLE
4. The mechanics of the priniting mechanism rely on BarcodePrintStatus
being called regularly after this call, in order to “stage” the data
to the interface.
****************************************************************/
void DLL BarcodeTicketPrint (char* TicketContents)
    {
    PrinterLength = strlen(TicketContents) + 1 ;
    PrinterOutputString  = new char[PrinterLength] ;
    strcpy(PrinterOutputString, TicketContents) ;
    PrinteroutputPoint   = PrinterOutputString ;

    //
    // O.K. - Start the transfer
    //

    CommandBlock Command ;         // We transfer data as a "command"
            Command.Code   = DATA_FOR_PRINTER ;
            Command.Length = PRINTER_DATA_BLOCK ;
    memmove(Command.Data,    PrinteroutputPoint, Command.Length) ;

    WriteInterfaceBlock(-1, (char *)&Command, 8 + Command.Length) ;
    }







/****************************************************************
BarcodePrintStatus

Synopsis
This call is used to determine the status of the barcoded ticket
printing system.

Parameters
None

Returns
    Mnemonic                       Value             Meaning
    PRINTER_NONE                 =  0,            // Printer completely non functional / not present
    PRINTER_FAULT                =  0x10000000,   // There is a fault somewhere
    PRINTER_IDLE                 =  0x00000001,   // The printer is OK / Idle /Finised
    PRINTER_BUSY                 =  0x00000002,   // Printing is currently taking place

    PRINTER_PLATEN_UP            =  0x00000004,
    PRINTER_PAPER_OUT            =  0x00000008,
    PRINTER_HEAD_FAULT           =  0x00000010,
    PRINTER_VOLT_FAULT           =  0x00000040,
    PRINTER_TEMP_FAULT           =  0x00000080,
    PRINTER_INTERNAL_ERROR       =  0x00000100,
    PRINTER_PAPER_IN_CHUTE       =  0x00000200,
    PRINTER_OFFLINE              =  0x00000400,
    PRINTER_MISSING_SUPPY_INDEX  =  0x00000800,
    PRINTER_CUTTER_FAULT         =  0x00001000,
    PRINTER_PAPER_JAM            =  0x00002000,
    PRINTER_PAPER_LOW            =  0x00004000,
    PRINTER_NOT_TOP_OF_FORM      =  0x00008000,
    PRINTER_OPEN                 =  0x00010000
****************************************************************/
int DLL BarcodePrintStatus (void)
    {
    int  EndGuard ;
    int  Status = 0 ;
    CommandBlock Command ;         // We transfer data as a "command"

    if (BasicControl->FieldVersion < ESCROW_FIELDS)
        return 0 ;                 // Card doesn't do escrow

    if (!OutputArea)
        return 0 ;

    if (!InputArea) return 0 ;

    // Now we have to stage the data through the interface.
    if (PrinteroutputPoint)
        {        // First we check on the prvious writes
        char Reply[128] ;
        int  InterfaceStatus = ReadInterfaceBlock(-1, Reply, 128) ;
        if (InterfaceStatus == 0)
            {                       // Last data block not yet gone through
            return PRINTER_BUSY ;
            }
        if (InterfaceStatus > 0)
            {                       // Last block was processed
            PrinteroutputPoint += PRINTER_DATA_BLOCK ;
            if (PrinteroutputPoint - PrinterOutputString > PrinterLength)
                {                   // We've finished!
                delete[] PrinterOutputString ;
                PrinterOutputString = 0 ;
                PrinteroutputPoint = 0 ;
                }
            }
        }

    if (PrinteroutputPoint)
        {    // And now (if we've still got data) we send it.
                Command.Code   = DATA_FOR_PRINTER ;
                Command.Length = PRINTER_DATA_BLOCK ;
        memmove(Command.Data,    PrinteroutputPoint, Command.Length) ;

        WriteInterfaceBlock(-1, (char *)&Command, 8 + Command.Length) ;
        return PRINTER_BUSY ;
        }


    // If we make it to here - then the data transfer system is idle
    // so we get the real printer's status and return that
    do
        {
        EndGuard = InputArea->EndGuard ;
        Status   = InputArea->BarcodePrinterStatus ;
        } while (EndGuard != InputArea->StartGuard) ;

    return Status ;
    }








/****************************************************************
The ReadAcceptorDetails call provides a snapshot of all
the information possessed by the interface on a single
unit of money handling equipment.


Parameters

1. Number
The serial number of the coin or note acceptor about
which information is required.

2. Snapshot
A pointer to a program buffer into which all the
information about the specified acceptor will be copied.

Return Value

True if the specified input device exists, False if the
end of the list is reached.

Remarks
The serial numbers of the acceptors are contiguous and
run from zero upwards.
****************************************************************/
int DLL ReadAcceptorDetails (int               Number,
                                 AcceptorBlock *   Snapshot)
    {
    int                       EndGuard ;
    int                       CoinNo ;
    AcceptorInterface*        Acceptor ;
    AcceptorCoinInterface*    Coin ;
    AcceptorCoin*             SnapshotCoin ;
    char*                     CoinPointer ;

    if (!InputArea) return FALSE ;

    if (InputArea->Acceptors == 0)
        return FALSE ;

    Acceptor = InputArea->Acceptors ;

    while (--Number >= 0)
        {
        if (Acceptor->Next == 0)
            return FALSE ;
        Acceptor = Acceptor->Next ;
        }

   // If we get here Acceptor points at the required block

    do
        {
        EndGuard = InputArea->EndGuard ;

        Snapshot->Unit              = Acceptor->Unit ;
        Snapshot->Status            = Acceptor->Status | (Acceptor->Flags & ACCEPTOR_INHIBIT);
        Snapshot->NoOfCoins         = Acceptor->NoOfCoins ;
        Snapshot->InterfaceNumber   = Acceptor->Interface ;
        Snapshot->UnitAddress       = Acceptor->UnitAddress ;
        Snapshot->DefaultPath       = Acceptor->DefaultPath ;
        *(int  *)Snapshot->Currency = Acceptor->Currency ;
        if (CallerVersion >= STRINGS_RETURNED)
            {
            if (BasicControl->FieldVersion >= STRING_FIELDS)
                {
                Snapshot->SerialNumber = Acceptor->SerialNumber ;
                Snapshot->Description  = Acceptor->Description ;
                }
            else
                {
                Snapshot->SerialNumber = 0 ;
                Snapshot->Description  = "" ;     // Null string
                }
            }

        if (CallerVersion >= BARCODE_ACCEPTOR)
            {                                   // Don't worry about Paylink, as this was unused
            Snapshot->BarcodesStacked   = (Acceptor->BarcodesStacked) & 0x7fffffff ;
            Snapshot->EscrowBarcodeHere = (Acceptor->BarcodesStacked) >> 31 ;
            }

        Coin = Acceptor->FirstCoin ;
        CoinPointer = reinterpret_cast<char*>(Snapshot->Coin) ;
        for (CoinNo = 0 ; CoinNo < Acceptor->NoOfCoins ; ++CoinNo)
            {
            SnapshotCoin                  = reinterpret_cast<AcceptorCoin*>(CoinPointer) ;
            SnapshotCoin->Value           = Coin->Value ;
            SnapshotCoin->Inhibit         = Coin->Inhibit ;
            SnapshotCoin->Count           = Coin->Count ;
            SnapshotCoin->Path            = Coin->Path ;
            SnapshotCoin->PathCount       = Coin->PathCount ;
            SnapshotCoin->DefaultPath     = (char)Coin->DefaultPath ;
            SnapshotCoin->PathSwitchLevel = Coin->PathSwitchLevel ;
            SnapshotCoin->FutureExpansion  = 0 ;
            SnapshotCoin->FutureExpansion2 = 0 ;

            if (BasicControl->FieldVersion >= ESCROW_FIELDS)
                {                      // Card does escrow
                SnapshotCoin->HeldInEscrow = (char)Coin->HeldInEscrow ;
                }
            else
                {                      // Card doesn't do escrow - clear old int  field
                SnapshotCoin->HeldInEscrow = 0 ;
                }


            if (CallerVersion >= STRINGS_RETURNED)
                {
                if (BasicControl->FieldVersion >= STRING_FIELDS)
                    {
                    SnapshotCoin->CoinName = Coin->CoinName ;
                    }
                else
                    {
                    SnapshotCoin->CoinName = "" ;     // Null string
                    }
                CoinPointer += sizeof *SnapshotCoin ;
                }
            else
                {
                CoinPointer += sizeof *SnapshotCoin - sizeof SnapshotCoin->CoinName ;
                }

            Coin = Coin->Next ;
            }
        } while (EndGuard != InputArea->StartGuard) ;

    return TRUE ;
    }





/****************************************************************
The WriteAcceptorDetails call updates all the changeable
information to the interface for a single unit of money
accepting equipment.

Parameters
1. Number
        The serial number of the coin or note acceptor
        being configured.

2. Snapshot
        A pointer to a program buffer containing the
        configuration data for the specified acceptor.
        See below for details.

Return Value
None.

Remarks
The serial numbers of the acceptors are contiguous
and run from zero upwards.
A call to ReadAcceptorDetails followed by call to
WriteAcceptorDetails for the same data will have no
effect on the system.
****************************************************************/
void DLL WriteAcceptorDetails(int              Number,
                              AcceptorBlock *  Snapshot)
    {
    int                     CoinNo ;
    AcceptorInterface *     Acceptor ;
    AcceptorCoinInterface * Coin ;
    AcceptorCoin*           SnapshotCoin ;
    char*                   CoinPointer ;


    if (!InputArea || !OutputArea) return ;

    if (InputArea->Acceptors == 0)
        return ;

    Acceptor = InputArea->Acceptors ;

    while (--Number >= 0)
        {
        if (Acceptor->Next == 0)
            return ;
        Acceptor = Acceptor->Next ;
        }

   // If we get here Acceptor points at the required block

    OutputArea->StartGuard++ ;

    Acceptor->Flags       = Snapshot->Status ;
    Acceptor->DefaultPath = Snapshot->DefaultPath ;

    Coin = Acceptor->FirstCoin ;
    CoinPointer = reinterpret_cast<char*>(Snapshot->Coin) ;
    for (CoinNo = 0 ; CoinNo < Acceptor->NoOfCoins ; ++CoinNo)
        {
        SnapshotCoin           = reinterpret_cast<AcceptorCoin*>(CoinPointer) ;
        Coin->Inhibit          = SnapshotCoin->Inhibit ;
        Coin->Path             = SnapshotCoin->Path ;
        Coin->PathSwitchLevel  = SnapshotCoin->PathSwitchLevel ;
        Coin->DefaultPath      = SnapshotCoin->DefaultPath ;

        if (CallerVersion >= STRINGS_RETURNED)
            {
            CoinPointer += sizeof *SnapshotCoin ;
            }
        else
            {
            CoinPointer += sizeof *SnapshotCoin - sizeof SnapshotCoin->CoinName ;
            }

        Coin = Coin->Next ;
        }
    OutputArea->PeripheralUpdates++ ;      // Flag "something happened"
    OutputArea->EndGuard = OutputArea->StartGuard ;
    }



/****************************************************************
The ReadDispenserDetails call provides a snapshot of all the
information possessed by the interface on a single unit of
money dispensing equipment.

Parameters
1. Number
The serial number of the coin or note dispenser about which
information is required.

2. Snapshot
A pointer to a program buffer into which all the information about
the specified dispenser will be copied.

Return Value
True if the specified input device exists, False if the end of the
list is reached.

Remarks
The serial numbers of the dispensers are contiguous and run from
zero upwards.
****************************************************************/
int DLL ReadDispenserDetails(int               Number,
                                 DispenserBlock *  Snapshot)
    {
    int                        EndGuard ;
    DispenserInterface*        Dispenser ;
    if (!InputArea) return FALSE ;

    if (InputArea->Dispensers == 0)
        return FALSE ;

    Dispenser = InputArea->Dispensers ;

    while (--Number >= 0)
        {
        if (Dispenser->Next == 0)
            return FALSE ;
        Dispenser = Dispenser->Next ;
        }

   // If we get here Dispenser points at the required block

    do
        {
        EndGuard = InputArea->EndGuard ;

        Snapshot->Unit            = Dispenser->Unit ;
        Snapshot->Status          = Dispenser->Status ;
        Snapshot->InterfaceNumber = Dispenser->Interface ;
        Snapshot->UnitAddress     = Dispenser->UnitAddress ;
        Snapshot->Value           = Dispenser->Value ;
        Snapshot->Count           = Dispenser->Count ;
        Snapshot->Inhibit         = Dispenser->Inhibit ;
        Snapshot->NotesToDump     = Dispenser->NotesToDump ;
        if (CallerVersion >= STRINGS_RETURNED)
            {
            if (BasicControl->FieldVersion >= STRING_FIELDS)
                {
                Snapshot->SerialNumber = Dispenser->SerialNumber ;
                Snapshot->Description  = Dispenser->Description ;
                }
            else
                {
                Snapshot->SerialNumber = 0 ;
                Snapshot->Description  = "" ;     // Null string
                }
            }

        if (CallerVersion >= DISPENSER_UPDATE)
            {
            if (BasicControl->FieldVersion >= DISPENSER_FIELDS)
                {
                Snapshot->CoinCount       = Dispenser->CoinCount ;
                Snapshot->CoinCountStatus = Dispenser->CoinCountStatus ;
                }
            else
                {
                Snapshot->CoinCount       = 0 ;
                Snapshot->CoinCountStatus = DISPENSER_COIN_NONE ;
                }
            }
        } while (EndGuard != InputArea->StartGuard) ;

    return TRUE ;
    }





/****************************************************************
The WriteDispenserDetails call updates all the changeable information
to the interface for a single unit of money handling equipment.


Parameters
1. Number
The serial number of the coin or note dispenser being configured.

2. Snapshot
A pointer to a program buffer containing the configuration data for
the specified dispenser. See below for details.

Return Value
None.

Remarks

The serial numbers of the dispensers are contiguous and run
from zero upwards. A call to ReadDispenserDetails followed by
call to WriteDispenserDetails for the same data will have no
effect on the system.
****************************************************************/
void DLL WriteDispenserDetails(int               Number,
                               DispenserBlock *  Snapshot)
    {
    DispenserInterface *       Dispenser ;
    if (!InputArea || !OutputArea) return ;

    if (InputArea->Dispensers == 0)
        return ;

    Dispenser = InputArea->Dispensers ;

    while (--Number >= 0)
        {
        if (Dispenser->Next == 0)
            return ;
        Dispenser = Dispenser->Next ;
        }

   // If we get here Dispenser points at the required block

    OutputArea->StartGuard++ ;

    Dispenser->Inhibit = Snapshot->Inhibit ;

    if (Snapshot->Status == DISPENSER_REASSIGN_VALUE
     && Snapshot->Value  != Dispenser->Value)
        {                                       // We're reassigning the value *now*.
        Dispenser->Flags  = DISPENSER_REASSIGN_VALUE ;
        Dispenser->Status = DISPENSER_REASSIGN_VALUE ;  // Status is "in progress"
        Dispenser->Value  = Snapshot->Value ;
        }

    if (Snapshot->Status == DISPENSER_UPDATE_COUNT)
        {
        Dispenser->Flags            = DISPENSER_UPDATE_COUNT ;
        Dispenser->Status           = DISPENSER_UPDATE_COUNT ;  // Status is "in progress"
        Dispenser->DispenseQuantity = Snapshot->CoinCount ;     // We use this as it runs in the correct Direction!
        UnclearedPrecise = true ;                               // We now need to clear that! (It's not a very good idea to have used it)
        }

    if (Snapshot->Status == DISPENSER_CASHBOX_DUMP)
        {
        Dispenser->Flags  = DISPENSER_CASHBOX_DUMP ;
        Dispenser->Status = DISPENSER_CASHBOX_DUMP ;
        }

    if (Snapshot->Status == DISPENSER_RESET_STATUS)
        {
        Dispenser->Flags  = DISPENSER_RESET_STATUS ;
        Dispenser->Status = DISPENSER_RESET_STATUS ;
        }

    if (Snapshot->Status      == DISPENSER_PARTIAL_DUMP)
        {
        Dispenser->Flags       = DISPENSER_PARTIAL_DUMP ;
        Dispenser->Status      = DISPENSER_PARTIAL_DUMP ;
        Dispenser->NotesToDump = Snapshot->NotesToDump ;
        }


    OutputArea->PeripheralUpdates++ ;          // Flag "something happened"
    OutputArea->EndGuard = OutputArea->StartGuard ;
    }











/****************************************************************
The WriteInterfaceBlock call sends a "raw" block to the specified
interface.

There is no guarantee as to when, in relation to this, regular
polling sequences will be sent, except that while the system is
disabled, the interface card will not put any traffic onto the
interface.

Parameters
1. Interface
The serial number of the interface that is being accessed.

2. Block
A pointer to program buffer with a raw message for the interface.
This must be a sequence of bytes, and must have any checksums
and addresses required by the peripheral device included.
3. Length

The number of bytes in the message.

Return Value
None

Remarks
Using this function with some interfaces does not make sense,
see status returns from ReadInterfaceBlock.
****************************************************************/
static int InterfaceError = INTERFACE_NON_EXIST ;
                        // This is used where the write function generates the error
void DLL WriteInterfaceBlock (int     Interface,
                              char *  Block,
                              int     Length)
    {
    int *    DataPtr ;
    int      EndGuard ;
    int      Sequence ;
    unsigned OutOffset ;
    unsigned i ;

    if (!OutputArea || !InputArea ||
        (Length > (int ) (sizeof (OutputArea->TransferBlock) - 8)))
        {
        InterfaceError = INTERFACE_TOO_LONG ;
        return ;
        }

// Now check that the previous Interaction has completed
    do
        {
        EndGuard = InputArea->EndGuard ;
        Sequence = InputArea->TransferSequence ;
        } while (EndGuard != InputArea->StartGuard) ;

   if (Sequence != OutputArea->TransferSequence)
       {
       InterfaceError = INTERFACE_OVERFLOW ;
       return ;
       }


    DataPtr = (int *)Block ;        // DPUpdate works in longs!

    if (Interface < 0)
        {                    // This is our "Raw" interface
        if (Length != (DataPtr[1] + 8))
            {
            InterfaceError = INTERFACE_NON_EXIST ;
            return ;
            }
        OutOffset = 0 ;
        }
    else
        {                    // Set the interface number & Length
        OutputArea->TransferBlock[0] = APPLICATION_INTERFACES + Interface ;
        OutputArea->TransferBlock[1] = Length ;
        OutOffset = 2 ;
        }

// OK we're now going to send the data - clear our overide error
    InterfaceError = 0 ;

// First Lock the interface
    OutputArea->StartGuard++ ;


    // Now copy the data
    for (i = 0 ; i < ((Length + 3) / sizeof (int )) ; ++i)
        {
        OutputArea->TransferBlock[i + OutOffset] = DataPtr[i] ;
        }
    OutputArea->TransferSequence++ ;

    OutputArea->EndGuard = OutputArea->StartGuard ; // Done
    }






/****************************************************************
The ReadInterfaceBlock call reads the "raw" response to a
single WriteInterfaceBlock.


Parameters
1. Interface
The serial number of the interface being accessed

2. Block
A pointer to the program buffer into which any response is read.

3. Length
The space available in the program buffer.

Return Values
     Mnemonic             Value     Meaning
     INTERFACE_TOO_LONG  =  -4, //  input command is too int
     INTERFACE_NON_EXIST =  -3, //  Non command oriented interface
     INTERFACE_OVERFLOW  =  -2, //  Command buffer overflow
     INTERFACE_TIMEOUT   =  -1, //  Timeout on the interface - no response occurred
     INTERFACE_BUSY      =  0,  //  The response from the WriteInterfaceBlock has not
                                //     yet been received
Remarks

1. Repeated calls to WriteInterfaceBlock without a successful
response are not guaranteed not to overflow internal buffers.

2. The program is expected to "poll" the interface for a response,
indicated by a non-zero return value.
****************************************************************/
int DLL ReadInterfaceBlock (int    Interface,
                             char*  Block,
                             int    Length)
    {
    int  Sequence ;
    int  CopyLength ;
    int  LengthCode ;
    int  EndGuard ;

    if (InterfaceError)
        {
        return static_cast<InterfaceStatus>(InterfaceError) ;
        }

    // Now retrieve the data
    do
        {
        EndGuard   = InputArea->EndGuard ;
        Sequence   = InputArea->TransferSequence ;
        LengthCode = InputArea->TransferBlock[0] ;
        if (LengthCode > 0)
            {
            CopyLength = (Length < LengthCode) ?
                          Length : LengthCode ;
            memcpy(Block, 4 + (char*)&InputArea->TransferBlock[0], CopyLength) ;
            }
        } while (EndGuard !=   InputArea->StartGuard) ;

    // *Now* check if that was worth it!!

    if (Sequence == OutputArea->TransferSequence)
        {
        return static_cast<InterfaceStatus>(LengthCode) ;
        }
    else
        {
        return INTERFACE_BUSY ;
        }
    }






/****************************************************************
The RawPort functions operates at a lower level than the above. These
calls are not expected to be used by user written application.
****************************************************************/


/****************************************************************
The DoRawPort sends a command block to the raw port handler.

The results of that command then have to be retrieved by polling
RawPortResult for a non-zero result.

All Raw port access *must* start with a RAW_CLAIM_PORT block, which
will disable the normal port handler and end with a RAW_RELEASE_PORT
which will re-instate the normal port handler.

A successfull RAW_WRITE_DATA will be indicated by a return value of
1 to a subsequent RawPortResult call, after all the data has been transmitted.

A RAW_READ_DATA call will ignore the Length and Message, a subsequent call
to RawPortResult will either return INTERFACE_NO_DATA or all the data that
fas been read so far. (This will then be lost from the IMHEI)

****************************************************************/
void DLL DoRawPort(RawPortStruct* Block)
    {
    CommandBlock Command ;
    Command.Code   =  Block->Function + RAW_PORT_OFFSET ;
    if (Block->Function == RAW_WRITE_DATA)
        {
        Command.Length = Block->Length ;
        memmove(Command.Data, Block->Message, Command.Length) ;
        }
    else
        {
        Command.Data[0] = (char)Block->PortNumber ;
        Command.Length  = 1 ;
        }
    WriteInterfaceBlock(-1, (char *)&Command, 8 + Command.Length) ;
    }


/****************************************************************
The RawPortResult retruens the response to a single DoRawPort call.

Parameters
1. Message
   If the DoRawPort function was RAW_READ_DATA, this is a pointer
   to a RAW_BLOCK_SIZE byte block.

Return Values are as for ReadInterfaceBlock above.
****************************************************************/

int DLL RawPortResult(char Message[RAW_BLOCK_SIZE])
    {
    return ReadInterfaceBlock(RAW_PORT_OFFSET, Message, RAW_BLOCK_SIZE) ;
    }
















/****************************************************
*****

***** Caution - the meter interface is not present on all PCI cards!!!

*****
*****************************************************/


/****************************************************************
The CounterIncrement call is made by the PC application software
to increment a specific counter value.

Parameters
1. CounterNo
This is the number of the counter to be incremented.


2. Increment
This is the value to be added to the specified counter.


Return Value
None

Remarks
If the counter specified is higher than the highest supported,
then call is silently ignored.

****************************************************************/
void DLL CounterIncrement(int  Counter,
                          int  Increment)
    {
    unsigned char CounterNo = Counter - 1 ;  // Starts at 1 (unsigned gives you 255 for 0!)

    if (!OutputArea ||
         CounterNo > NO_OF_COUNTERS) return ;

    if (BasicControl->FieldVersion < METER_FIELDS)
        return ;             // Card doesn't do meters

    OutputArea->StartGuard++ ;

    OutputArea->CounterValue[CounterNo] = OutputArea->CounterValue[CounterNo] + Increment ;

    OutputArea->PeripheralUpdates++ ;                // Flag "something happened"
    OutputArea->EndGuard = OutputArea->StartGuard ;
    }





/****************************************************************
The CounterCaption call is used to associate a caption with the
specified counter. This is related to the CounterDisplay call
described below.


Parameters
1. CounterNo
This is the number of the counter to be incremented.

Caption
2. This is an ASCII string that will be associated with the counter.

Return Value
None

Remarks

1. The meter hardware may have limited display capability.
It is the system designer’s responsibility to use captions
that are within the meter hardware’s capabilities.

2. If the counter specified is higher than the highest supported,
then call is silently ignored.
****************************************************************/
void DLL CounterCaption(int   Counter,
                        char* Caption)
    {
    int  *CaptionPtr ;
    int i ;
    unsigned char CounterNo = Counter - 1 ;  // Starts at 1 (unsigned gives you 255 for 0!)
    if (!OutputArea ||
         CounterNo > NO_OF_COUNTERS) return ;

    if (BasicControl->FieldVersion < METER_FIELDS)
        return ;             // Card doesn't do meters

    CaptionPtr = (int *)Caption ;        // The access works in longs!

    OutputArea->StartGuard++ ;

    for (i = 0 ; i < (int)(sizeof OutputArea->CounterCaption[0] / sizeof (int )) ; ++i)
        {
        OutputArea->CounterCaption[CounterNo][i] = CaptionPtr[i] ;
        }

    OutputArea->PeripheralUpdates++ ;                // Flag "something happened"
    OutputArea->EndGuard = OutputArea->StartGuard ;
    }




/****************************************************************
The CounterRead call is made by the PC application software to
obtain a specific counter value as stored by the meter interface.

Parameters
1. CounterNo
   This is the number of the counter to be incremented.

Return Value
The Value of the specified meter at system start-up.

Remarks
1. If the counter specified is higher than the highest supported,
   then the call returns -1

2. If the counter external hardware does not support counter
   read-out, then this will return the total of all increments
   since PC start-up.

3. If error conditions prevent the meter updating, this call will
   show the value it should be at, not its actual value.
   (The value is only read from the meter at system start-up.)
****************************************************************/
int DLL CounterRead(int  Counter)
    {
    unsigned char CounterNo = Counter - 1 ;  // Starts at 1 (unsigned gives you 255 for 0!)
    if (!OutputArea ||
         CounterNo > NO_OF_COUNTERS) return 0 ;

    if (BasicControl->FieldVersion < METER_FIELDS)
        return 0 ;             // Card doesn't do meters

    return OutputArea->CounterValue[CounterNo] ;
    }




/****************************************************************
The ReadCounterCaption call is used to determine the caption for
the specified counter

Parameters
1. CounterNo
   This is the number of the counter to be incremented.

Return Value
None

Remarks
1. If the counter specified is higher than the highest supported,
   then the call returns an empty string (“”).
2. All captions stored in the meter are read out at system start-up
   and used to initialise the captions used by the interface.
****************************************************************/
__pchar DLL ReadCounterCaption(int  Counter)
    {
    static   int  CopySpace[32][3] ;
    unsigned char CounterNo = Counter - 1 ;  // Starts at 1 (unsigned gives you 255 for 0!)
    if (!OutputArea ||
         CounterNo > NO_OF_COUNTERS) return (char*)"" ;

    if (BasicControl->FieldVersion < METER_FIELDS)
        return (char*)"" ;             // Card doesn't do meters

    // Note that we are reading out our *input* area - this means
    // that we don't worry about interlock!

    memmove(CopySpace, &OutputArea->CounterCaption[0][0], sizeof CopySpace) ;

    return (char*)CopySpace[CounterNo] ;
    }








/****************************************************************
The CounterDisplay call is used to control what is displayed on the meter.

Parameters
1. DisplayCode
If positive, this specifies the counter that will be continuously
             display by the meter hardware.

If negative, then the display will cycle between the caption (if set)
             for the specified counter for 1 second, followed by its
             value for 2 seconds.

Return Value
None

Remarks
1. This result of this call with a negative parameter is undefined
   if no counters have an associated caption.
2. Whenever the meter displayed is changed, the caption (if set)
   is always displayed for one second.
****************************************************************/

void DLL CounterDisplay (int  DisplayCode)
    {
    if (!OutputArea) return ;

    if (BasicControl->FieldVersion < METER_FIELDS)
        return ;             // Card doesn't do meters

    // Although meters start at 1, we pass through unchanged 'cos we want to
    // distinguish between +1 & -1 !
    OutputArea->StartGuard++ ;

    OutputArea->MeterDisplayCode = DisplayCode ;

    OutputArea->PeripheralUpdates++ ;          // Flag "something happened"
    OutputArea->EndGuard = OutputArea->StartGuard ;
    }




/****************************************************************
The MeterStatus call is used determine whether working meter
equipment is connected.

int  MeterStatus ( void );

Parameters
None

Return Value
One of the following:
Return Values.
     Mnemonic        Value     Meaning
     METER_OK      =  0,   //  A Meter is present and working correctly
     METER_MISSING =  1,   //  No Meter has ever been found
     METER_DIED    =  2,   //  The Meter is no longer functioning
     METER_FAILED  =  3,   //  The Meter is functioning, but is itself
                           //     reporting internal problems

Remarks
None
****************************************************************/
int DLL MeterStatus ( void )
    {
    int  EndGuard ;
    int  Value ;

    if (!InputArea) return static_cast<MeterStatuses>(0) ;

    if (BasicControl->FieldVersion < METER_FIELDS)
        return METER_MISSING ;      // Card doesn't do meters

    do
        {
        EndGuard = InputArea->EndGuard ;
        Value    = InputArea->MeterStatus ;
        } while (EndGuard != InputArea->StartGuard) ;

    return static_cast<MeterStatuses>(Value) ;
    }





/****************************************************************
The MeterSerialNo call is used determine which item meter
equipment is connected.


Parameters
None

Return Value
The 32-bit serial number retrieved from the meter equipment.

Remarks

Where the meter equipment is not present or does not have serial
number capabilities, zero is returned.
****************************************************************/
int DLL MeterSerialNo ( void )
    {
    int  EndGuard ;
    int  Value ;

    if (!InputArea) return 0 ;

    if (BasicControl->FieldVersion < METER_FIELDS)
        return 0 ;      // Card doesn't do meters

    do
        {
        EndGuard = InputArea->EndGuard ;
        Value    = InputArea->MeterSerialNo ;
        } while (EndGuard != InputArea->StartGuard) ;

    return Value ;
    }






/****************************************************************
The E2PromReset call is made by the PC application software to
clear all the E2PROM counter on the card.


Parameters
1. LockE2Prom
   This is a Boolean flag.
   If zero, then the counters may be reset again later.
   If non zero, then all future calls to this function will have
   no effect on the card.

Return Value
None

Remarks
None
****************************************************************/
void DLL E2PromReset(int  LockE2Prom)
    {
    char         Reply[128] ;
    CommandBlock Command ;          // We do this as a "command"

    Command.Code    = RESET_EEPROM ;
    Command.Length  = 2 ;
    Command.Data[0] = (LockE2Prom) ? 1 : 0 ;

    WriteInterfaceBlock(-1, (char *)&Command, 8 + Command.Length) ;

    while (InterfaceError == INTERFACE_OVERFLOW)    // Incredibly unlikley, but....
        {
        Sleep(1) ;
        WriteInterfaceBlock(-1, (char *)&Command, 8 + Command.Length) ;
        }

    while (ReadInterfaceBlock(-1, Reply, 128) == 0)
        {                           // Wait for the action to happen
        Sleep(1) ;
        }
    // We have no interest in a response!
    }




/****************************************************************
The E2PromWrite call is made by the PC application software
to write to all or part of the user E2PROM on the card.


Parameters
1. UserBuffer
   This is the address of the user’s buffer, from which BufferLength
   bytes of data are copied to the start of the user area.
2. BufferLength
   This is the count of the number bytes to be transferred.
   If this is greater than 256 the extra will be silently ignored.

Return Value
None

Remarks
1. This call schedules the write to the E2PROM memory and returns
   immediately. There is no way of knowing when the E2PROM has
   actually been updated but, barring hardware errors, it will be
   complete within one second of the call.
****************************************************************/
void DLL E2PromWrite (void* UserBuffer,
                      int   BufferLength)
    {
    int  Staging[8][8] ;            // Intermediate area accessible in int  words
    int Section ;
    int i ;
    int  NewControl ;
    int  Mask ;
    int  Increment ;
    int  Changed ;

    if (BasicControl->FieldVersion < EEPROM_FIELDS)
        return ;             // Card doesn't do eeprom

    memcpy(Staging, (void *)&OutputArea->EpromData[0][0], sizeof Staging) ; // Initialise it


    if (BufferLength > (int )(sizeof OutputArea->EpromData))
        {
        BufferLength = sizeof OutputArea->EpromData ;
        }
    memcpy(Staging, UserBuffer, BufferLength) ;  // Copy in the updates

    // Now we can update the DP ram
    OutputArea->StartGuard++ ;

    // as we go we have to spot the differences!
    NewControl = OutputArea->EpromControl ;
    Mask = 0xf ;
    Increment = 1 ;
    for (Section = 0 ; Section < 8 ; ++Section)
        {
        Changed = 0 ;
        for (i = 0 ; i < 8 ; ++i)
            {
            if (OutputArea->EpromData[Section][i] != Staging[Section][i])
                {
                Changed = 1 ;
                OutputArea->EpromData[Section][i] = Staging[Section][i] ;
                }
            }
        if (Changed)
            {               // Notify the H8 that this section has changed
            NewControl = ((NewControl + Increment) &  Mask) |       // New value for this section
                         ( NewControl              & ~Mask) ;       // Old value for all other sections
            }
        Mask     <<= 4 ;
        Increment <<= 4 ;
        }
    OutputArea->EpromControl = NewControl ;      // Finally tell H8 about changes
    OutputArea->PeripheralUpdates++ ;   // Flag "something happened"
    OutputArea->EndGuard = OutputArea->StartGuard ;
    }




/****************************************************************
The E2PromRead call is made by the PC application software to obtain
all or part of the user E2PROM from the card.

Parameters
1. UserBuffer
   This is the address of the user’s buffer, into which the current
   contents of the user E2PROM area are copied.
2. BufferLength
   This is the count of the number bytes to be transferred. If this
   is greater than 256 the extra will be silently ignored.

Return Value
None

Remarks
1. Unwritten E2Prom memory is initialised all one bits.
2. Writes performed by E2PromWrite will be reflected immediately
   in the data returned by this function, regardless of whether
   or not they have been committed to E2Prom memory.
****************************************************************/
void DLL E2PromRead (void* UserBuffer,
                     int   BufferLength)
    {
    if (BasicControl->FieldVersion < EEPROM_FIELDS)
        return ;             // Card doesn't do eeprom

    if (BufferLength > (int (sizeof OutputArea->EpromData)))
        {
        BufferLength = sizeof OutputArea->EpromData ;
        }
    memcpy(UserBuffer, (void *)&OutputArea->EpromData[0][0], BufferLength) ;
    }








/***************************************************************
ReadEscrowBlock

Synopsis
The ReadEscrowBlock call is used to obtain the latest information for an
EscrowControlBlock.



Parameters
1.  Number
The sequence number of the escrow control system about which information is required.
2.  Snapshot
A pointer to a program buffer into which all the information about the specified
acceptor will be copied.

Return Value
Non zero if the specified Escrow control block exists, Zero if the end of the list is
reached.

Remarks
1.  Zero can be returned when Number has the value of zero if no escrow control systems
are running.

****************************************************************/

int DLL ReadEscrowBlock (int                 Number,
                         EscrowControlBlock* Snapshot)
    {
    int                     EndGuard ;
    EscrowInterface*        Escrow ;
    if (!InputArea) return FALSE ;

    if (BasicControl->FieldVersion < ESCROW_DATA)
        {
        return FALSE ;
        }

    if (InputArea->EscrowBlock == 0)
        return FALSE ;

    Escrow = InputArea->EscrowBlock ;

    while (--Number >= 0)
        {
        if (Escrow->Next == 0)
            return FALSE ;
        Escrow = Escrow->Next ;
        }


    do
        {
        EndGuard = InputArea->EndGuard ;

        Snapshot->EscrowVersion  = Escrow->EscrowVersion ;
        Snapshot->State          = Escrow->State ;
        Snapshot->Result         = Escrow->Result ;
        Snapshot->TotalValue     = Escrow->TotalValue ;
        Snapshot->ValueReturned  = Escrow->ValueReturned ;
        Snapshot->AcceptorNo     = Escrow->AcceptorNo ;
        Snapshot->NoInEscrow     = Escrow->NoInEscrow  ;

        EscrowNoteInterface* EscrowNote = Escrow->FirstNote ;

        for (int Note = 0 ; Note < Snapshot->NoInEscrow ; ++Note)
            {
            Snapshot->EscrowNote[Note].Value       = EscrowNote->Value ;
            Snapshot->EscrowNote[Note].NoteNumber  = EscrowNote->NoteNumber ;
            long LocationFlags                     = EscrowNote->LocationFlags ;
            Snapshot->EscrowNote[Note].Location    = LocationFlags & ESCROW_NOTE_DISP_MASK ;
            Snapshot->EscrowNote[Note].Status      = LocationFlags >> ESCROW_NOTE_STATUS_SHIFT ;
            EscrowNote = EscrowNote->Next ;
            }

        // The result field has to reflect the communications with the Milan unit, and hence
        // is more complicated than simply a copy over.

        if (Escrow->CommandsIssued != Escrow->CommandsProcessed)
            {
            Snapshot->Result = EXT_ESCROW_BUSY ;
            }


        } while (EndGuard != InputArea->StartGuard) ;

    return TRUE ;
    }







/***************************************************************
EscrowCommand

Synopsis
The EscrowCommand call is used by the application to handle all interaction with the extended
escrow system.

The following commands are defined:

    EXT_ESCROW_START,                    // Turn on the Escrow system
    EXT_ESCROW_STOP,                     // Turn off the Escrow system
    EXT_ESCROW_ACCEPT,                   // Allows the acceptor to input notes to the Escrow system
    EXT_ESCROW_PAUSE,                    // Stop accepting notes and allow system to settle
    EXT_ESCROW_STACK,                    // Transfer notes to the cash box (or retain them on Rolls)
    EXT_ESCROW_RETURN                    // Return all escrowed notes to the user.

Parameters
1.  Number
The sequence number of the escrow control system for which the command is intended.
2.  Command
The command being issued..

Return Value
None

Remarks
1.  The success or failure and subsequent progress of a command are determined by value set into
the Result and State fields of the EscrowControlBlock.
2.  Immediately following this call, the Result field of the EscrowControlBlock will always be
EXT_ESCROW_BUSY.
3.  For any particular State of the escrow control system only a small subset of commands are valid.
Any other command will generate EXT_ESCROW_WRONGSTATE.

****************************************************************/
void DLL EscrowCommand(int    Number,
                       int    Command)
    {
    if (!InputArea) return ;

    if (BasicControl->FieldVersion < ESCROW_DATA)
        {
        return ;
        }

    if (InputArea->EscrowBlock == 0)
        return ;

    EscrowInterface* Escrow = InputArea->EscrowBlock ;

    while (--Number >= 0)
        {
        if (Escrow->Next == 0)
            return ;
        Escrow = Escrow->Next ;
        }

    // If we get here Escrow points at our block
    OutputArea->StartGuard++ ;

    Escrow->TheCommand     = Command ;
    Escrow->CommandsIssued = Escrow->CommandsProcessed + 1 ;            // make not equall

    OutputArea->PeripheralUpdates++ ;      // Flag "something happened"
    OutputArea->EndGuard = OutputArea->StartGuard ;
    }







/*******************************************************************************************
********************************************************************************************

Paylink provides for a Cashless system if the appropriate hardware is present
connected provide sufficient facilities for that.

********************************************************************************************
********************************************************************************************/

char BarcodeText[19] ;


/***************************************************************
CashlessReadData

Synopsis
Return the current information related to the delivery of credit from the cashless system.

Parameters
1.  Number
The sequence number of the Cashless peripheral
2.  DataBlock
A pointer to a Cashless structure that is to be updated.

Return Value
Zero if the sequence number refers to a non existent peripheral.
     The CurrentState item in the updated DataBlock will have the value CR_NO_UNIT.

Remarks
1.  This function should be called regularly to monitor the Cashless facility.

****************************************************************/
int DLL CashlessReadData(int Number,
                         CashlessControlBlock* DataBlock)
    {
    DataBlock->CurrentState = CR_NO_UNIT ;

    if (Number != 0)
      return 0 ;

    if (!InputArea)
      return 0 ;

    if (BasicControl->FieldVersion < PRECISE_DISPENSE)
        {
        return 0 ;
        }

    if (BasicControl->CashlessPointer == 0)
        return 0 ;

    int EndGuard ;
    do
        {
        EndGuard = InputArea->EndGuard ;
        CashlessInterface* Cashless = BasicControl->CashlessPointer ;

        DataBlock->CashlessType      = Cashless->CashlessType ;
        DataBlock->Description       = Cashless->Description ;
        DataBlock->SerialNumber      = Cashless->SerialNumber ;
        DataBlock->TotalAcquisitions = Cashless->TotalAcquisitions ;
        DataBlock->TotalCredit       = Cashless->TotalCredit ;
        DataBlock->CurrentState      = Cashless->CurrentState ;
        DataBlock->StateDetails      = Cashless->StateDetails ;
        DataBlock->CreditValue       = Cashless->CreditValue ;
        DataBlock->ReferenceData     = BarcodeText ;
        if (Cashless->CommandsIssued != Cashless->CommandsProcessed)
            {
            DataBlock->CurrentState  = CR_BUSY ;
            }
        } while (EndGuard != InputArea->StartGuard) ;


    return 1 ;
    }


/***************************************************************
CashlessEnable

Synopsis
Where relevant (e.g. card systems) this enables peripheral equipment to be start accepting requests. This is also used following any transaction sequence, to return the system to state of CR_IDLE.

Parameters
1.  Number
The sequence number of the Cashless peripheral

Return Value
  None

Remarks
1.  With ticket based systems this has no effect on the peripheral, as the application is responsible for initiating the process
2.  Following this call, CurrentState should become CR_IDLE
****************************************************************/
void DLL CashlessEnable (int Number)
    {
    if (!InputArea)
      return ;

    if (BasicControl->FieldVersion < PRECISE_DISPENSE)
        {
        return ;
        }

    if (BasicControl->CashlessPointer == 0)
        return ;

    CashlessInterface* Cashless = BasicControl->CashlessPointer ;

    OutputArea->StartGuard++ ;
    Cashless->TheCommand      = CASHLESS_ENABLE ;
    Cashless->CommandsIssued  = Cashless->CommandsIssued + 1 ;
    OutputArea->PeripheralUpdates++ ;      // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }






/***************************************************************
CashlessDisable

Synopsis
Where relevant (e.g. card systems) this causes peripheral equipment to cease accepting requests.

Parameters
1.  Number
The sequence number of the Cashless peripheral

Return Value
  None

Remarks
1.  With ticket based systems this has no effect on the peripheral, as the application is responsible for initiating the process
2.  Following this call, CurrentState should become CR_DISABLED
****************************************************************/
void DLL CashlessDisable (int Number)
    {
    if (!InputArea)
      return ;

    if (BasicControl->FieldVersion < PRECISE_DISPENSE)
        {
        return ;
        }

    if (BasicControl->CashlessPointer == 0)
        return ;

    CashlessInterface* Cashless = BasicControl->CashlessPointer ;

    OutputArea->StartGuard++ ;
    Cashless->TheCommand      = CASHLESS_DISABLE ;
    Cashless->CommandsIssued  = Cashless->CommandsIssued + 1 ;
    OutputArea->PeripheralUpdates++ ;      // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }





/***************************************************************
SubmitTicket
Synopsis
Only relevant for ticket based system, this provides the
reference number read from a barcoded ticket,
in the expectation of this resulting in a credit becoming available.

Parameters
1.  Number
The sequence number of the Cashless peripheral
2.  TicketReference
A pointer to a ASCII string containing the reference number.

Return Value
  N/A

Remarks
1.  This function should cause CurrentState to eventually become CR_AVAILABLE, with the value of the ticket shown in the CreditDelivered.

****************************************************************/
void DLL SubmitTicket(int Number,
                      char* TicketReference)
    {
    if (!InputArea) return ;

    if (BasicControl->FieldVersion < PRECISE_DISPENSE)
        {
        return ;
        }

    if (BasicControl->CashlessPointer == 0)
        return ;

    strncpy(BarcodeText, TicketReference, sizeof BarcodeText - 1) ;

  CashlessInterface* Cashless = BasicControl->CashlessPointer ;

    long long TicketNumber = 0 ;
    for (int i = 0 ; TicketReference[i] ; ++i)
        {
        TicketNumber *= 10 ;
        TicketNumber += TicketReference[i] - '0' ;
        }

    OutputArea->StartGuard++ ;

    Cashless->TicketNo[0]     = TicketNumber ;
    Cashless->TicketNo[1]     = TicketNumber >> 32 ;
    Cashless->CommandsIssued  = Cashless->CommandsIssued + 1 ;
    Cashless->TheCommand      = CASHLESS_SUBMIT_TICKET ;
    Cashless->CurrentState    = CR_BUSY ;

    OutputArea->PeripheralUpdates++ ;      // Flag "something happened"
    OutputArea->EndGuard = OutputArea->StartGuard ;
    }


/***************************************************************
CashlessRequestCredit
Synopsis
When Paylink is indicating that credit is available, this call allows the application
to notify Paylink how much of the available credit the application wishes to take.

Parameters
1.  Number
The sequence number of the Cashless peripheral
2.  AmountRequested
The amount of credit that the application is requesting from the source of credit.

Return Value
  N/A

Remarks
1.  This must only be called when CurrentState is CR_AVAILABLE.
2.  Some peripheral configurations (typically ticket based) will only allow
        the AmountRequested to be equal to CreditValue.
3.  The credit described may not actually be available. The application
        must wait for a state of CR_CONFIRMED before starting to use any credit.
4.  Under error conditions; following this call, the state of CR_TAKEN
        may be reached, even without the application calling CashlessTakeCredit().
5.  If the application doesn't want to use the available credit, it should
        call CashlessRefuseCredit instead.
****************************************************************/
void DLL CashlessRequestCredit(int Number,
                               int AmountRequested)
    {
    if (!InputArea)
      return ;

    if (BasicControl->FieldVersion < PRECISE_DISPENSE)
        {
        return ;
        }

    if (BasicControl->CashlessPointer == 0)
        return ;

    CashlessInterface* Cashless = BasicControl->CashlessPointer ;

    OutputArea->StartGuard++ ;

    Cashless->CommandsIssued  = Cashless->CommandsIssued + 1 ;
    Cashless->RequestedValue  = AmountRequested ;
    Cashless->TheCommand      = CASHLESS_REQUEST_CREDIT ;
    Cashless->CurrentState    = CR_BUSY ;
    OutputArea->PeripheralUpdates++ ;      // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }






/***************************************************************
CashlessRefuseCredit
Synopsis
When Paylink is indicating that credit is available, this call allows the application to notify Paylink that the application wishes to cancel the transaction.

Parameters
1.  Number
The sequence number of the Cashless peripheral.

Return Value
  N/A

Remarks
1.  This must only be called when CurrentState is CR_AVAILABLE.
2.  This call will be complete when  CurrentState is CR_REFUSED.
****************************************************************/
void DLL CashlessRefuseCredit(int Number)
    {
    if (!InputArea)
      return ;

    if (BasicControl->FieldVersion < PRECISE_DISPENSE)
        {
        return ;
        }

    if (BasicControl->CashlessPointer == 0)
        return ;

    CashlessInterface* Cashless = BasicControl->CashlessPointer ;

    OutputArea->StartGuard++ ;

    Cashless->CommandsIssued  = Cashless->CommandsIssued + 1 ;
    Cashless->TheCommand      = CASHLESS_REFUSE_CREDIT ;
    Cashless->CurrentState    = CR_BUSY ;

    OutputArea->PeripheralUpdates++ ;      // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }


/***************************************************************
CashlessTakeCredit
Synopsis
When Paylink is indicating that credit has been confirmed, this is
 the final call to commit the transaction.

Parameters
1.  Number
    The sequence number of the Cashless peripheral

Return Value
  N/A

Remarks
1.  This must only be called when CurrentState is CR_CONFIRMED
2.  The application must wait for the subsequent state of CR_TAKEN
        before regarding the transaction as having completed.
****************************************************************/
void DLL CashlessTakeCredit(int Number)
    {
    if (!InputArea)
      return ;

    if (BasicControl->FieldVersion < PRECISE_DISPENSE)
        {
        return ;
        }

    if (BasicControl->CashlessPointer == 0)
        return ;

    CashlessInterface* Cashless = BasicControl->CashlessPointer ;

    OutputArea->StartGuard++ ;

    Cashless->CommandsIssued  = Cashless->CommandsIssued + 1 ;
    Cashless->TheCommand      = CASHLESS_TAKE_CREDIT ;
    Cashless->CurrentState    = CR_BUSY ;

    OutputArea->PeripheralUpdates++ ;      // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }



/***************************************************************
CashlessCancelCredit
Synopsis
When Paylink is indicating that credit has been confirmed with CR_CONFIRMED,
     this is a "last chance" to cancel the transaction because there has
   been a problem using the credit. This call may or may not succeed.

Parameters
1.  Number
     The sequence number of the Cashless peripheral

Return Value
  N/A

Remarks
1.  This must only be called when CurrentState is CR_CONFIRMED
2.  If successful, CurrentState will become CR_CANCELLED
3.  The application may find a subsequent state of CR_TAKEN and will
     then have to take an appropriate alternative action for the
   credit specified in CreditValue.
****************************************************************/
void DLL CashlessCancelCredit(int Number)
    {
    if (!InputArea)
      return ;

    if (BasicControl->FieldVersion < PRECISE_DISPENSE)
        {
        return ;
        }

    if (BasicControl->CashlessPointer == 0)
        return ;

    CashlessInterface* Cashless = BasicControl->CashlessPointer ;

    Cashless->CommandsIssued  = Cashless->CommandsIssued + 1 ;
    Cashless->TheCommand      = CASHLESS_CANCEL_CREDIT ;
    Cashless->CurrentState    = CR_BUSY ;

    OutputArea->StartGuard++ ;
    OutputArea->PeripheralUpdates++ ;      // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }



/***************************************************************
CashlessReset
Synopsis
This call notifies the Paylink cashless system that the application is
  now ready to start another cashless transaction,


Parameters
1.  Number
The sequence number of the Cashless peripheral

Return Value
  N/A

Remarks
1.  Following this call the cashless system is guaranteed to return to CR_IDLE.
****************************************************************/
void DLL CashlessReset(int Number)
    {
    if (!InputArea)
      return ;

    if (BasicControl->FieldVersion < PRECISE_DISPENSE)
        {
        return ;
        }

    if (BasicControl->CashlessPointer == 0)
        return ;

    CashlessInterface* Cashless = BasicControl->CashlessPointer ;

    OutputArea->StartGuard++ ;

    Cashless->CommandsIssued  = Cashless->CommandsIssued + 1 ;
    Cashless->TheCommand      = CASHLESS_RESET ;
    Cashless->CurrentState    = CR_BUSY ;

    OutputArea->PeripheralUpdates++ ;      // Flag "something happened"

    OutputArea->EndGuard = OutputArea->StartGuard ;
    }









/****************************************************************
Synopsis
The SetDeviceKey call is made by the PC application software to
set such things as an encryption key.


Parameters
1. InterfaceNo
   The Interface on which the device is located
2. Address
   The address of the device whose key is being updated
3. Key
   The 32 bit key to be remembered for the device.

Return Value
None

Remarks
1. At present, this can only be used for a Lumina acceptor at address
   40 on interface 2, the cctalk interface. The key (as 6 hex digits)
   is used as the encryption key.

2. An example application for this is available within the SDK folder
   structure.

****************************************************************/
void DLL SetDeviceKey (int  InterfaceNo,
                       int  Address,
                       int  Key)
    {
    char         Reply[128] ;
    CommandBlock Command ;          // We do this as a "command"

    Command.Code            = SET_DEVICE_SERIAL_NO ;
    Command.Length          = 4 ;
    *(int  *)Command.Data   = Key ;

    WriteInterfaceBlock(-1, (char *)&Command, 8 + Command.Length) ;

    while (InterfaceError == INTERFACE_OVERFLOW)    // Incredibly unlikley, but....
        {
        Sleep(1) ;
        WriteInterfaceBlock(-1, (char *)&Command, 8 + Command.Length) ;
        }

    while (ReadInterfaceBlock(-1, Reply, 128) == 0)
        {                           // Wait for the action to happen
        Sleep(1) ;
        }
    // We have no interest in an actual response!
    }




/****************************************************************
ReadInterfaceDevice
Synopsis
The ReadLoggedEvent call retrieves an event as described above
from the system.

Parameters
    Information
      A pointer to a block of type InterfaceDevice.
****************************************************************/
void DLL ReadInterfaceDevice(InterfaceDevice* Information)
    {
    memset(Information, 0, sizeof *Information) ; // set unused / out of action

    if (!PCInternal) return ;
    if (PCInternal->FlagWord2 != AARDVARK_FLAG_WORD) return ;
    *Information = PCInternal->DeviceInfo ;
    }







/****************************************************************
The USBDriverStatus call allows an interested application to retrieve
the status of the USBDriver program for Paylink system.

Parameters
    None

Return Values
     Mnemonic              Value     Meaning
    NOT_USB                = -1,     // Interface is to a PCI card
    USB_IDLE               = 0,      // No driver or other program running
    STANDARD_DRIVER        = 1,      // The driver program is running normally
    FLASH_LOADER           = 2,      // The flash re-programming tool is using the link
    MANUFACTURING_TEST     = 3,      // The manufacturing test tool is using the link
    DRIVER_RESTART         = 4,      // The standard driver is in the process of exiting / restarting
    USB_ERROR              = 5       // The driver has received an error from the low level driver
    } USBStatus ;

  Remarks

For PCI systems this is obviously meaningless and the system returns NOT_USB
****************************************************************/
int DLL USBDriverStatus (void)
    {
    if (PCInternal)
        {
        return static_cast<USBStatus>(PCInternal->USBUsage) ;
        }
    else
        {
        return NOT_USB ;
        }
    }





/****************************************************************
The USBDriverExit call allows a control application to request that
the USB driver program exits in a controlled manner.

Parameters
    None

Return Values
    None

Remarks

Driver programs with a version

For PCI systems this is obviously meaningless and has no effect.
****************************************************************/
void DLL USBDriverExit (void)
    {
    if (PCInternal)
        {
        PCInternal->USBUsage = DRIVER_RESTART ;
        }
    }






/****************************************************************
The FirmwareVersion call allows a control application to discover the
exact description of the firmware running on the unit.

Parameters
    1.  CompileDate
        This is a pointer to a 16 byte area that receives a null terminated
        printable version of the date on which the firmware was installed.
    2.  CompileTime
        This is a pointer to a 16 byte area that receives a null terminated
        printable version of the time at which the firmware was installed.

Return Values
    The firware version as a 32 bit integer. This is normally shown as
    4 x 8 bit numbers separated by dots.

Remarks

Either or both of the charecter pointers may be null.
****************************************************************/
int DLL FirmwareVersion (char* CompileDate,
                          char* CompileTime)
    {
    if (CompileDate)
        {
        memcpy(CompileDate, (void *)&BasicControl->CompileDate[0], 16) ;
        }
    if (CompileTime)
        {
        memcpy(CompileTime, (void *)&BasicControl->CompileTime[0], 16) ;
        }
    return BasicControl->CodeVersion ;
    }





/****************************************************************
The HardwarePlatform call allows a control application to discover the
hardware platform.

Parameters
    None

****************************************************************/
int DLL PlatformType (void)
    {
    if (!InputArea) return 0 ;

    return InputArea->PlatformType ;
    }














__pchar DLL IMHEIConsistencyError(int CoinTime, int NoteTime)
    {
    if (!InputArea || !OutputArea) return (char*)"Interface not initialised" ;

    static char Result[1024] ;
    typedef struct
        {
        bool          Set ;
    #ifdef __linux__
        timeval       Time ;
    #else
        LARGE_INTEGER Time ;
    #endif

        int           Value ;
        int           AcceptorCount ;
        int           AcceptorCoins ;
        int           AcceptorNotes ;
        int           AcceptorValue ;
        } TheValues ;

    static TheValues Old ;
           TheValues New ;


    if (!InputArea || !OutputArea) return (char*)"IMHEI not functional" ;


    Result[0] = 0 ;

    /******************************************************************************
     * The First task is to retrieve a consistent set of new values
     ******************************************************************************/
    int   EndGuard ;
    do
        {
        EndGuard = InputArea->EndGuard ;

        TheValues Blank = {0} ;                 // Clear snapshot
        New = Blank ;


        New.Value  = InputArea->ReadValue ;

        for (AcceptorInterface* Acceptor = InputArea->Acceptors ; Acceptor ; Acceptor = Acceptor->Next)
            {
            New.AcceptorCount++ ;
            AcceptorCoinInterface* Coin = Acceptor->FirstCoin ;
            for (int CoinNo = 0 ; CoinNo < Acceptor->NoOfCoins ; ++CoinNo)
                {
                New.AcceptorValue +=  Coin->Count * Coin->Value ;

                if (IS_COIN_ACCEPTOR(Acceptor->Unit))
                    {
                    New.AcceptorCoins += Coin->Count ;
                    }
                else
                    {
                    New.AcceptorNotes += Coin->Count ;
                    }

                Coin = Coin->Next ;
                }
            }

        } while (EndGuard != InputArea->StartGuard) ;


    /******************************************************************************
     * Now find out the elapsed time since the last call
     ******************************************************************************/
    static  int           MilliSecondConversion ;
    #ifdef __linux__
        static struct timezone tz;
        if (!MilliSecondConversion)
            {
            gettimeofday(&Old.Time, &tz);
            MilliSecondConversion = (int )((double)(Old.Time.tv_sec*1000) + (double)Old.Time.tv_usec/(1000));
            }
        gettimeofday(&New.Time,&tz);
        double t1         = (double)(Old.Time.tv_sec*1000) + (double)Old.Time.tv_usec/(1000);
        double t2         = (double)(New.Time.tv_sec*1000) + (double)New.Time.tv_usec/(1000);
        int  MilliSeconds = (int )(t2-t1);
    #else
        if (!MilliSecondConversion)
            {
            LARGE_INTEGER Frequency ;
            QueryPerformanceFrequency(&Frequency) ;
            MilliSecondConversion = (int )Frequency.QuadPart / 1000 ;
            }


        QueryPerformanceCounter(&New.Time) ;
        int  MilliSeconds = (int )(New.Time.QuadPart - Old.Time.QuadPart) / MilliSecondConversion ;
    #endif


    if (MilliSeconds < 10)
        {                               // Nothing can have happened yet!
        return 0 ;
        }



    if (Old.Set && (Old.AcceptorCount == New.AcceptorCount))
        {
        /******************************************************************************
         * Now compare this against saved Values
         ******************************************************************************/
        int  ValueIncrease = New.Value - Old.Value ;
        int  AcceptorValueIncrease = New.AcceptorValue - Old.AcceptorValue ;

        if (ValueIncrease < 0)
            {
            sprintf(Result, "CurrentValue decremented by %d", -ValueIncrease) ;
            }
        else if (ValueIncrease != AcceptorValueIncrease)
            {
            sprintf(Result, "CurrentValue increment %d, but acceptors show %d", ValueIncrease, AcceptorValueIncrease) ;
            }
        /******************************************************************************
         * Finally sanity check the data against time
         ******************************************************************************/
        else if (MilliSeconds < CoinTime * 2
             ||  MilliSeconds < NoteTime * 2)
            {                                       // We don't do a time check at this precision
            return 0 ;                              // return "OK" **without** saving the new time and values
            }

        int  MaxCoins = MilliSeconds / CoinTime + 1 ;
        int  MaxNotes = MilliSeconds / NoteTime + 1 ;

        if (New.AcceptorCoins - Old.AcceptorCoins > MaxCoins)
            {
            sprintf(Result, "%d coins read, but only time for %d", New.AcceptorCoins - Old.AcceptorCoins, MaxCoins) ;
            }
        else if (New.AcceptorNotes - Old.AcceptorNotes > MaxNotes)
            {
            sprintf(Result, "%d notes read, but only time for %d", New.AcceptorNotes - Old.AcceptorNotes, MaxNotes) ;
            }
        }


    New.Set = true ;       // Save all values for next time.
    Old = New ;

    if (Result[0] == 0)
        {
        return 0 ;
        }
    else
        {
        return Result ;
        }
    }
































