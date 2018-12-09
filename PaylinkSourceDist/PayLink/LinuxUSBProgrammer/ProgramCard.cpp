/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
#include "ReadSRec.h"
#include "../Headers.h"

#include "ProgramCard.h"
#include "../../Kernel.h"

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
int    CodeApplication;
int    CodeChecksum;
int    CodeVersion;
int    CardSelfTest;
int    KernelCodeVersion ;
bool    ForceKernelUpdate;
bool    AutoProgram;
static bool BlockAcknowledged ;
bool    AutoExit;
bool    Programming             = true;
int     BlocksToProgram         = 0;
int     BlocksProgrammed        = 0;

void ErrorExit(char *Message) ;

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
void ProgramCard(void)
{
    int           i;
    unsigned int  Address;
    bool          Found;
    unsigned int  StartAddress;

    Programming      = true;
    BlocksToProgram  = 0;
    BlocksProgrammed = 0;

    if( (KernelCodeVersion <= 0)
      || ForceKernelUpdate
      || ((GETLONG(&ProgramSpace[KERNEL_CODE_INFO_ADDR] ) != AARDVARK_FLAG))
      )
    {
    // Resident Kernel version -1 = No kernel; Kernel Version 0 = Dummy Kernel. In both cases must overwrite resident kernel
    // OR: User has requested Kernel is overwritten
    // OR: New file does not have a Kernel
    // No resident Kernel or have been forced to overwrite kernel
    StartAddress = 0;
    printf("Downloading Kernel\n") ;
    }
  else
    {
    // Do not overwrite the resident kernel unless forced to.
    StartAddress = APPLICATION_CODE_START_ADDR;
    }

  for (Address = StartAddress ; Address < sizeof ProgramSpace ; Address += 128)
  {
    // look for a non-erased block
        Found = 0 ;
    for (i = 0 ; i < 128 ; ++i)
    {
      Found |= (ProgramSpace[Address + i] != 0xff) ;
    }
    if (Found)
    {
      ++BlocksToProgram ;
    }
  }
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
           if (KernelCodeVersion > 0)                     // For version older than "kernel" ones, this will crash the reprogramming code!!!!
               {                                          // later version need it echoed to prevent USB resets.
               Milan->QueuePacket(USB_ACTION, USB_H8_RESPONSE) ;
               Milan->SendBuffer() ;
               }
           break ;



        default:
            DiagPrintf("Unexpected extended code %x received\r\n", Value) ;
            break ;

            }
         break ;


        default:
            DiagPrintf("Unexpected code %x received\r\n", Code) ;
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
    int          StartAddress;


        /*-- Send the data to the H8 --------------------------------------*/
    Milan->QueuePacket(USB_RESET,  USB_RESET) ;
    Milan->QueuePacket(USB_ACTION, USB_FLASH_START) ;
    Milan->SendBuffer() ;

    printf("%06.02f%%", (double)((double)BlocksProgrammed/BlocksToProgram)*100.0);

    // if the target system has a Kernel then do not download a new kernel.
    if( (KernelCodeVersion <= 0)
      || ForceKernelUpdate
      || ((GETLONG(&ProgramSpace[KERNEL_CODE_INFO_ADDR] ) != AARDVARK_FLAG))
      )
    {
      // Resident Kernel version -1 = No kernel; Kernel Version 0 = Dummy Kernel. In both cases must overwrite resident kernel
      // OR: User has requested Kernel is overwritten
      // OR: New file does not have a Kernel
      // No resident Kernel or have been forced to overwrite kernel
      StartAddress = 0;
    }
    else
    {
      // Do not overwrite the resident kernel unless forced to.
      StartAddress = APPLICATION_CODE_START_ADDR;
    }

    for (unsigned int Address = StartAddress ; Address < sizeof ProgramSpace ; Address += 128)
        {   /*-- Look for a non-erased block ------------------------------*/
        Found = 0 ;
        for (int i = 0 ; i < 128 ; ++i)
            {
            Found |= (ProgramSpace[Address + i] != 0xff) ;
            }

        if (Found)
            {                                               // Got one - so send it!
            BlockAcknowledged = false ;
            Milan->QueuePacket(USB_FLASH_ADDRESS, Address) ;

            for (int i = 0 ; i < 32 ; ++i)
                {
                Milan->QueuePacket(USB_FLASH_DATA,
                                ProgramSpace[Address + 0 + i * 4]       |   // Keep in same byte order!
                                ProgramSpace[Address + 1 + i * 4] <<  8 |
                                ProgramSpace[Address + 2 + i * 4] << 16 |
                                ProgramSpace[Address + 3 + i * 4] << 24 ) ;
                }

            if (Milan->SendBuffer())
                {
                printf("\n\n") ;
                ErrorExit((char*)"Paylink Unit Failed During Programming\n") ;
                return ;
                }

            while (!BlockAcknowledged )
                {
                Milan->QueuePacket(USB_ACTION, USB_QUERY_SELFTEST) ;    // Self Test results
                Sleep(100) ;
                if (!Milan->ProcessIncomingStream())
                    {
                    printf("\n\n") ;
                    ErrorExit((char*)"Paylink Unit Failed During Programming\n") ;
                    return ;
                    }
                }
            BlocksProgrammed++ ;
            printf("\b\b\b\b\b\b\b       \b\b\b\b\b\b\b%06.02f%%", (double)((double)BlocksProgrammed/BlocksToProgram)*100.0);
            fflush(stdout);
            }

        if (Address == APPLICATION_CODE_START_ADDR - 128)
            {
            printf("\nKernel Downloaded\nProgramming... %06.02f%%", (double)((double)BlocksProgrammed/BlocksToProgram)*100.0);
            }
        }

//  That's it - tell the H8 we're done

    Milan->QueuePacket(USB_ACTION, USB_FLASH_END) ;                  // Program Start
    Milan->SendBuffer() ;

    printf("\nWaiting for Paylink to restart\n") ;

    CardSelfTest = 0 ;                                        // Wait for the restart
    while (!CardSelfTest)
        {
        Milan->QueuePacket(USB_RESET,  USB_RESET) ;
        Milan->QueuePacket(USB_ACTION, USB_QUERY_SELFTEST) ;    // Self Test results
        if (Milan->SendBuffer() || !Milan->ProcessIncomingStream())
            {
            printf("Paylink Unit Reset\n") ;
            CardSelfTest = SELFTEST_OK ;                // Don't have a real result
            return ;
            }
        Sleep(100) ;
        }

    Programming = false ;
    }
//---------------------------------------------------------------------------

