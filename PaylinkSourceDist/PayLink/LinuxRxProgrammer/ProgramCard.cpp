/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
#include "ReadSRec.h"
#include "../Headers.h"

#include "ProgramCard.h"
#include "RXChecksumConstants.h"

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
unsigned int CodeApplication;
unsigned int CodeChecksum;
         int CodeVersion;
         int ThePlatform ;
unsigned int CardSelfTest;
         int KernelCodeVersion ;
        bool ForceKernelUpdate;
bool    AutoProgram;
static bool BlockAcknowledged ;
bool    AutoExit;
int     BlocksToProgram         = 0;
int     BlocksProgrammed        = 0;
       bool ResendBlock ;

extern bool Verbose ;

void ErrorExit(char *Message)
;
#define _16K                  (0x4000)
#define _4K                   (0x1000)

#define BLOCK_SIZE _16K
/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/


static unsigned int StartAddress;
static unsigned int CodeLength;

void ProgramCard(void)
{
    BlocksToProgram  = 0;
    BlocksProgrammed = 0;

  // only program the application code

  StartAddress    = LowestFlashAddress ;
  CodeLength      = ((ForceKernelUpdate) ? 0 : FCPROM_START) - (unsigned int)StartAddress  ;
  BlocksToProgram = CodeLength / BLOCK_SIZE ;
}


/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
//---------------------------------------------------------------------------
void OurProcessIncomingPacket(unsigned short Address, int Value)
    {
    unsigned short Code  =  Address & USB_CONTROL_MASK ;
    unsigned short Index = (Address & USB_ADDRESS_MASK) >> 2 ;

    switch (Code)
        {
    case USB_QUERY_REPLY:                    // Reply to enquiry
        switch(Index)
            {
        case USB_QUERY_APPLICATION:
            CodeApplication = Value ;
            break ;


        case USB_QUERY_CHECKSUM:
            CodeChecksum = Value ;
            break ;


        case USB_QUERY_KERNEL_VERSION:
            KernelCodeVersion = Value ;
            break ;  //


        case USB_QUERY_VERSION:
            CodeVersion = Value ;
            break ;


        case USB_QUERY_PLATFORM:
            ThePlatform = Value ;
            break ;


        case USB_QUERY_SELFTEST:
            CardSelfTest = Value ;
            break ;



        default:
            DiagPrintf("Invalid Enquiry Reply %x\r\n", Index) ;
            break ;
            }
        break ;


    case USB_ACTION:
        if (Index != 0)
            {
            DiagPrintf("Extended Code flag with non zero Index %x\r\n", Index) ;
            }
        else switch (Value)
            {
        case USB_FLASH_ACK:
            BlockAcknowledged = true ;
            break ;

        case USB_MEMORY_CLEAR:
            break ;

        case USB_NEW_MEMORY_CLEAR:
            break ;


        case USB_POLL:                                    // No Op from USB
           Milan->QueuePacket(USB_ACTION, USB_H8_RESPONSE) ;
           Milan->SendBuffer() ;
           break ;



        default:
            DiagPrintf("Unexpected extended code %x received\r\n", Value) ;
            break ;

            }
         break ;

    case USB_SET_COMMAND:
         break ;

    default:
         if (Code != 0) DiagPrintf("Unexpected code %x received\r\n", Code) ;
         break ;
        }

    if (ShowTraffic)
        {
        DiagPrintf(" <--%04x %08x\r\n", Address, Value) ;
        }
    }

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
void Execute(void)
    {
    bool         Found ;
        /*-- Send the data to the H8 --------------------------------------*/
    Milan->QueuePacket(USB_RESET,  USB_RESET) ;
    Milan->QueuePacket(USB_ACTION, USB_FLASH_START) ;              // Program Start
    Milan->SendBuffer() ;
    Sleep(500) ;
    ThePort->USBClose() ;                                          // The unit is resetting ;
    Sleep(200) ;
    while (!ThePort->USBOpen())
        {
        Sleep(500) ;
        }
    DiagPrintf("Link Re-opened") ;

    Milan->PacketIndex = 0 ;
    Milan->QueuePacket(USB_RESET,  USB_RESET) ;
    Milan->QueuePacket(USB_RESET,  USB_RESET) ;

    printf("%06.02f%%", (double)((double)BlocksProgrammed/BlocksToProgram)*100.0);

  for (unsigned int Offset = 0 ; Offset < CodeLength ; Offset += BLOCK_SIZE)
        {                                                   // look for a non-erased block
        Found = 0 ;
        unsigned int Address = StartAddress + Offset ;

//        for (int i = 0 ; i < BLOCK_SIZE ; ++i)
//            {
//            Found |= (ProgramSpace[Address - START_OF_FLASH_AREA + i] != 0xff) ;
//            }
        Found = true ;                                      // Don't need the speed

        if (Found)
            {                                               // Got one - so send it!
            if (Verbose)
                {
                printf("Sending block @ %08x\n", Address) ;
                }
            BlockAcknowledged = false ;
            Milan->QueuePacket(USB_FLASH_ADDRESS, Address) ;
            int PacketCount = 0 ;
            for (int i = 0 ; i < (BLOCK_SIZE / 4) ; ++i)
                {
                Milan->QueuePacket(USB_FLASH_DATA,
                                ProgramSpace[Address - START_OF_FLASH_AREA + 0 + i * 4]       |   // Keep in same byte order!
                                ProgramSpace[Address - START_OF_FLASH_AREA + 1 + i * 4] <<  8 |
                                ProgramSpace[Address - START_OF_FLASH_AREA + 2 + i * 4] << 16 |
                                ProgramSpace[Address - START_OF_FLASH_AREA + 3 + i * 4] << 24 ) ;
                if (++PacketCount > 256)
                    {
                    PacketCount = 0 ;
                    Milan->SendBuffer() ;
                    }
                }
            Milan->SendBuffer() ;

            while (!BlockAcknowledged )
                {
                Sleep(10) ;
                Milan->ProcessIncomingStream() ;
                if (ResendBlock)
                    {
                    if (Verbose)
                        {
                        printf("*** Resending block") ;
                        }
                    ResendBlock = false ;
                    Milan->PacketIndex = 0 ;
                    Milan->QueuePacket(USB_RESET,  USB_RESET) ;
                    Milan->QueuePacket(USB_RESET,  USB_RESET) ;
                    Milan->QueuePacket(USB_ACTION, USB_FLASH_START) ;              // Program Start
                    Offset -= BLOCK_SIZE ;
                    BlocksProgrammed-- ;
                    break ;
                    }
                }
            BlocksProgrammed++ ;
            printf("\b\b\b\b\b\b\b       \b\b\b\b\b\b\b%06.02f%%", (double)((double)BlocksProgrammed/BlocksToProgram)*100.0);
            fflush(stdout);
            }
        }

    printf("\nWaiting for Paylink to restart\n") ;

//  That's it - tell the unit we're done
    Milan->QueuePacket(USB_ACTION, USB_FLASH_END) ;                  // Program Start
    Milan->SendBuffer() ;


    CardSelfTest = 0 ;                                        // Wait for the restart
    while (!CardSelfTest)
        {
        Milan->QueuePacket(USB_RESET,  USB_RESET) ;
        Milan->QueuePacket(USB_ACTION, USB_QUERY_SELFTEST) ;    // Self Test results
        if (Milan->SendBuffer() || !Milan->ProcessIncomingStream())
            {
            printf("Paylink Unit Reset\n") ;
            CardSelfTest = SELFTEST_OK ;                // Don't have a real result
            }
        Sleep(100) ;
        }
    }
//---------------------------------------------------------------------------

