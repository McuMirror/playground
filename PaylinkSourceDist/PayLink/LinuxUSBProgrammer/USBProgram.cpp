
/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
#include "../Headers.h"

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
#include "../FtdiAccess.h" /* AES Ftdi Header */
#include "ReadSRec.h"
#include "ProgramCard.h"

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
#include "../../win_types.h"
#include "../../Kernel.h"
/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
HeaderBlock* TheHeader ;
HeaderBlock* TheKernelHeader ;

int FileFlag ;


char* CompileDate ;
char* CompileTime ;
char  PaylinkSerialNumber[12];
char  ShareName[32];

bool Force ;
bool CheckMode ;
bool ShowTraffic ;

PCInternalBlock* PCInternal ;

USBAccess*  ThePort ;
USBMilan*   Milan ;


char* ErrorData = NULL;
/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
void  DiagText(char* Line)
    {
    ErrorData = Line ;

 //       puts(Line) ;

    }

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
static void ExitPrompt(int Val)
    {
    if (PCInternal)
        {
        ThePort->USBClose() ;
        PCInternal->USBUsage = USB_IDLE ;
        }

    exit(Val) ;
    }

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
void ErrorExit(char *Message)
    {
    printf("Error: %s", Message);
    if (strlen(PaylinkSerialNumber))
        {
        printf(" (Serial No. %s)", PaylinkSerialNumber) ;
        }
    printf("\n\n") ;
    ExitPrompt(1);
    }

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
char* SetVersion(int version_status)
    {
    switch(version_status)
        {
    case 0:  return (char*)"Special" ;
    case 1:  return (char*)"Development" ;
    case 2:  return (char*)"Alpha Test" ;
    case 3:  return (char*)"Beta Test" ;
    case 4:  return (char*)"Full Release" ;
    default: return (char*)"Unknown" ;
        }
    }

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
int main(int argc, char *argv[])
    {
    int     pagesize        =  getpagesize();
    int     num             =  0;
    int     option          =  0;
    int     shm_file        = -1;
    int     r               = -1;
    int    ms              =  0;
    int    Send            =  0;
    double  t1              =  0;
    double  t2              =  0;
    char    buff[128] ;
    char*   Base            = NULL;
    timeval old_time;
    timeval new_time;
    struct  timezone tz;

    ThePort = new FTDIAccess((char*)PRODUCT_NAME, USB_VID, USB_PID, BAUD_RATE) ;
    Milan   = new USBMilan(ThePort) ;


    extern void OurProcessIncomingPacket(unsigned short Address, int Value) ;
    Milan->ProcessIncomingPacket = OurProcessIncomingPacket ;


    /*-- Decode Command Line Arguments Supplied -----------------------*/
    while ((option=getopt(argc,argv,"?fcks:")) != -1)
        {       switch(option)
                    {
                case 'f':     Force     = true;                          break;
                case 's':     strcpy(PaylinkSerialNumber, optarg);       break;
                case 'k':     ForceKernelUpdate = true ;                 break;
                case 'c':     CheckMode = true;                          break;
                case '?':
                default :
                        printf("%s:\r\n", argv[0]);
                        printf(" -f         Force the loading of firmware.\r\n");
                        printf(" -c         Check if firmware upgrade is required.\r\n");
                        printf(" -s         Program specific serial number.\r\n");
                        printf(" -?         Display this help message.\r\n\r\n");
                        exit(0);
                    }
        }

    if (PaylinkSerialNumber[0] != '\0')
        {
        printf("AES Firmware Reprogramming System, paylink serial number: %s\n", PaylinkSerialNumber);
        }
    else
        {
        printf("AES Firmware Reprogramming System\n");
        }

    strcpy(ShareName, SHARED_NAME) ;
    if (PaylinkSerialNumber[0])
    {
        strcat(ShareName, PaylinkSerialNumber) ;
    }

    /*-- Get the shared Memory, to turn off the driver ----------------*/
    shm_file = shm_open(ShareName, O_CREAT | O_RDWR,  0777 );
    if (shm_file == -1)
        {   fprintf (stderr, "ERROR: Creating shared memory segment (shm_open).\r\n");
            fflush  (stderr);
            exit(0) ;
        }

    /*-- Calculate number of pages required ---------------------------*/
    num = abs(((EXTENDED_SHARED_SIZE) / pagesize));
    if ((num * pagesize) < (EXTENDED_SHARED_SIZE))
        num += 1;

    if ((ftruncate (shm_file, (num * pagesize))) == -1)
        {
        fprintf (stderr, "ERROR: Creating shared memory segment (ftruncate).\r\n");
        fflush  (stderr);
        exit(0) ;
        }

    /*-- Set up the mirrors -------------------------------------------*/
    Base = (char *)mmap( 0, num*pagesize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_file, 0);
    if (Base == (char *) -1)
        {
        fprintf (stderr, "ERROR: Creating shared memory segment (mmap).\r\n");
        fflush  (stderr);
        exit(0) ;
        }

    PCInternal           = PC_INTERNAL_BLOCK(Base) ;
    PCInternal->FlagWord = AARDVARK_FLAG_WORD ;
    PCInternal->USBUsage = FLASH_LOADER ;

        /*-- This is a total hack, but the structures not going to change -*/
    CompileDate = Base + 48 ;
    CompileTime = Base + 64 ;

        /*-- Now, spend 5 seconds trying to get the FTDI chip -------------*/
    CodeVersion = -1 ;
    KernelCodeVersion = -1 ;

    gettimeofday(&old_time, &tz);
    gettimeofday(&new_time, &tz);
    t1 = (double)(old_time.tv_sec*1000) + (double)old_time.tv_usec/(1000);
    t2 = (double)(new_time.tv_sec*1000) + (double)new_time.tv_usec/(1000);
    ms = (int)(t2-t1);
    bool    USBNowOpen = false ;

    Send = ms ;
    ShowTraffic = true ;
    ErrorData = (char*)"No Paylink found" ;
    while (ms < 5000)
        {
        if (!USBNowOpen)
            {
            if (strlen(PaylinkSerialNumber))
                {
                // The we are being asked to
                // reprogram a SPECIFIC Paylink

                USBNowOpen = ThePort->USBOpenSpecific(PaylinkSerialNumber);
                }
            else
                {
                // We are being asked to reprogram
                // ANY Paylink that we can find

                USBNowOpen = ThePort->USBOpen();
                }
            }


        if (USBNowOpen)
            {
            for (int i = 0 ; i < 100 ; ++i)
                {
                Milan->ProcessIncomingStream() ;    // This "only" processes the next 4096 bytes!
                }
            }


        if (CodeVersion != -1)
            {
            break ;                  // We've now got everything
            }


        if (USBNowOpen && ms >= Send)
            {
            Send = ms + 250 ;
            Milan->QueuePacket(USB_RESET,  USB_RESET);
            Milan->QueuePacket(USB_ACTION, USB_QUERY_APPLICATION);    // Application
            Milan->QueuePacket(USB_ACTION, USB_QUERY_CHECKSUM);       // Application
            Milan->QueuePacket(USB_ACTION, USB_QUERY_SELFTEST);       // Checksum
            Milan->QueuePacket(USB_ACTION, USB_QUERY_KERNEL_VERSION) ; // Kernel Version
            Milan->QueuePacket(USB_ACTION, USB_QUERY_VERSION);        // Version
            Milan->SendBuffer();
        }

        Sleep(1);
        gettimeofday(&new_time,&tz);
        t1 = (double)(old_time.tv_sec*1000) + (double)old_time.tv_usec/(1000);
        t2 = (double)(new_time.tv_sec*1000) + (double)new_time.tv_usec/(1000);
        ms = (int)(t2-t1);
        }
    ShowTraffic = false;


    if (ms >= 5000)
        {
        /*-- No USB chip / version - get last error -------------------*/
        ErrorExit(ErrorData) ;
        }


    /*-- OK - we've got a working USB card to talk to! ----------------*/
    if (CodeChecksum == LIGHT_CHECKSUM)
        {           // This will be a Paylink Lite unit
        ErrorExit((char*)"Paylink Lite cannot be re-programmed") ;
        }


    char*        ErrMsg ;
    int          MyFile ;
    int          DataSize = 0 ;
    int          HeaderStart = 0 ;

        /*-- Set to unprogrammed ------------------------------------------*/
    memset(ProgramSpace, 0xff, sizeof ProgramSpace) ;

    if ((num = readlink("/proc/self/exe", buff, 128)) != -1)
        buff[num] = 0;

    if((MyFile = open(buff, O_RDONLY)) < 0)
        {
        ThePort->USBClose() ;
        ErrorExit((char*)"Can't re-open own file") ;
        }

    /*-- If we've had an image attached, then we'll find that last   --*/
        /*-- three int words of our file are: an Ardvark flag, the size --*/
        /*-- of the image and an offset to the header block.             --*/

    r = lseek(MyFile, -12, SEEK_END) ;
    r = read(MyFile, &FileFlag, sizeof FileFlag) ;
    if ((unsigned)FileFlag == AARDVARK_FLAG)
    {
        r = read(MyFile, &DataSize, sizeof DataSize) ;
        r = read(MyFile, &HeaderStart, sizeof HeaderStart) ;
        r = lseek(MyFile, -(DataSize + 12), SEEK_END) ;     // End of the executable
        r = read(MyFile, ProgramSpace, DataSize) ;          // Get the data
        MaxAddress = DataSize ;                         // Fill in load info
        TheHeader = (HeaderBlock*)(ProgramSpace + HeaderStart) ;
        TheKernelHeader = (HeaderBlock*)(ProgramSpace + KERNEL_CODE_INFO_ADDR) ;
    }
    else
    {   /* ---------- No Data in file - maybe it's text in sysin */
        ErrMsg = ReadSRec(0) ;
        if (ErrMsg)
        {
            ThePort->USBClose() ;
            ErrorExit(ErrMsg) ;
        }
        TheHeader       = (HeaderBlock*)(ProgramSpace + APPLICATION_CODE_INFO_ADDR) ;
        TheKernelHeader = (HeaderBlock*)(ProgramSpace + KERNEL_CODE_INFO_ADDR) ;
    }
    close (MyFile) ;


    if ((ErrMsg = CheckCheckSum()) != 0)
    {
        ThePort->USBClose() ;
        ErrorExit(ErrMsg) ;
    }





        /*-----------------------------------------------------------------*\
         OK - All finished - we've got a kosher image loaded & ready to program
                       - do we want to do "anything"
        \*-----------------------------------------------------------------*/
    if (TheHeader)
    {
        if (TheHeader->VersionStatus    == ((CodeVersion >> 24) & 0xff) &&
            TheHeader->MajorVersion     == ((CodeVersion >> 16) & 0xff) &&
            TheHeader->MinorVersion     == ((CodeVersion >> 8 ) & 0xff) &&
            TheHeader->MinisculeVersion == ((CodeVersion      ) & 0xff) &&
            ((TheHeader->RomCheckSum >> 24) & 0xff) == ((CodeChecksum      ) & 0xff) &&
            ((TheHeader->RomCheckSum >> 16) & 0xff) == ((CodeChecksum >> 8 ) & 0xff) &&
            ((TheHeader->RomCheckSum >> 8 ) & 0xff) == ((CodeChecksum >> 16) & 0xff) &&
            ((TheHeader->RomCheckSum      ) & 0xff) == ((CodeChecksum >> 24) & 0xff) &&
            CardSelfTest                == SELFTEST_OK)
        {
            /*-- The images match -------------------------------------*/
            if (CheckMode)
                {
                ThePort->USBClose() ;
                printf("Images match\n") ;
                ExitPrompt(0) ;
                }
            else
                {
                AutoExit = (TheHeader->VersionStatus >= 3) ;
                }
            }
        else
            {
            AutoProgram = true ;
            AutoExit = true ;
            }
        }
    else
        {
        AutoProgram = false ;
        AutoExit    = false ;
        ErrorExit((char*)"No Image in file!") ;
        }



    printf("Loaded Firmware: %s\r\n", SetVersion((CodeVersion >> 24) & 0xff)) ;
    printf("  %d.%d.%d\n", (CodeVersion >> 16) & 0xff,
                              (CodeVersion >>  8) & 0xff,
                              (CodeVersion      ) & 0xff) ;


    if (CompileDate[0] != '\0' && CompileTime[0] != '\0')
        {
        printf("        Compiled on %s at %s\n", CompileDate,
                                                 CompileTime) ;
        }

    if (Force || AutoProgram)
        {
        printf("New Firmware:    %s\r\n", SetVersion(TheHeader->VersionStatus)) ;

        printf("  %d.%d.%d\n", TheHeader->MajorVersion,
                               TheHeader->MinorVersion,
                               TheHeader->MinisculeVersion) ;

        printf("        Compiled on %s at %s\n", TheHeader->CompileDate,
                                                 TheHeader->CompileTime) ;

        if (!AutoProgram)
            {
            printf("Re-programming only because of [-F] Force flag\n") ;
            }

        // These are out of date now, so make sure we don't re-use them!
        CompileDate[0] = '\0' ;
        CompileTime[0] = '\0' ;

        ProgramCard();
        printf("Programming...") ;
        Execute();

        if (CardSelfTest != SELFTEST_OK)
            {
            if (CardSelfTest & SELFTEST_CHECKSUM_FAIL)
                {
                printf("\r\nWarning: The reprogramming has failed!!\r\nThe unit is returning a checksum failure status\r\n" );
                }
            else
                {
                printf("\r\nWarning: The unit is returning\r\n  a self test error code of %x\r\n", CardSelfTest) ;
                }
            }

        printf("\nProgramming complete\n") ;

        }
    else
        printf("Card firmware is up to date\n") ;

    ExitPrompt(0) ;
    return 0 ;
}
