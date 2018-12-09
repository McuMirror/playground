
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
#include "../HidAccess.h"
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
#include "RXChecksumConstants.h"

// Define pointers to the data in memory
unsigned int* ApFlagPtr              = (unsigned int*) &ProgramSpace[APP_AARDVARK_FLAG       - START_OF_FLASH_AREA] ;
unsigned int* ApAreaStartAddrPtr     = (unsigned int*) &ProgramSpace[APP_AREA_START_ADDR     - START_OF_FLASH_AREA] ;
unsigned int* ApAreaEndAddrPtr       = (unsigned int*) &ProgramSpace[APP_AREA_END_ADDR       - START_OF_FLASH_AREA] ;
unsigned int* ApChecksumPtr          = (unsigned int*) &ProgramSpace[APP_CHECKSUM            - START_OF_FLASH_AREA] ;
unsigned int* ApFirstSpareAddrPtr    = (unsigned int*) &ProgramSpace[APP_FIRST_SPARE_ADDR    - START_OF_FLASH_AREA] ;
unsigned int* ApLastSpareAddrPtr     = (unsigned int*) &ProgramSpace[APP_LAST_SPARE_ADDR     - START_OF_FLASH_AREA] ;
unsigned char* ApVersionPtr          = (unsigned char*)&ProgramSpace[APP_VERSION             - START_OF_FLASH_AREA] ;
unsigned char* ApDatePtr             = (unsigned char*)&ProgramSpace[APP_COMPILE_DATE        - START_OF_FLASH_AREA] ;
unsigned char* ApTimePtr             = (unsigned char*)&ProgramSpace[APP_COMPILE_TIME        - START_OF_FLASH_AREA] ;

unsigned int* POCFlagPtr             = (unsigned int*) &ProgramSpace[POC_AARDVARK_FLAG      - START_OF_FLASH_AREA] ;
unsigned int* POCAreaStartAddrPtr    = (unsigned int*) &ProgramSpace[POC_AREA_START_ADDR    - START_OF_FLASH_AREA] ;
unsigned int* POCAreaEndAddrPtr      = (unsigned int*) &ProgramSpace[POC_AREA_END_ADDR      - START_OF_FLASH_AREA] ;
unsigned int* POCChecksumPtr         = (unsigned int*) &ProgramSpace[POC_CHECKSUM           - START_OF_FLASH_AREA] ;
unsigned char* POCVersionPtr         = (unsigned char*)&ProgramSpace[POC_VERSION            - START_OF_FLASH_AREA] ;

unsigned int* FCPFlagPtr             = (unsigned int*) &ProgramSpace[FCP_AARDVARK_FLAG      - START_OF_FLASH_AREA] ;
unsigned int* FCPAreaStartAddrPtr    = (unsigned int*) &ProgramSpace[FCP_AREA_START_ADDR    - START_OF_FLASH_AREA] ;
unsigned int* FCPAreaEndAddrPtr      = (unsigned int*) &ProgramSpace[FCP_AREA_END_ADDR      - START_OF_FLASH_AREA] ;
unsigned int* FCPChecksumPtr         = (unsigned int*) &ProgramSpace[FCP_CHECKSUM           - START_OF_FLASH_AREA] ;
unsigned char* FCPVersionPtr         = (unsigned char*)&ProgramSpace[FCP_VERSION            - START_OF_FLASH_AREA] ;

unsigned int  MyChecksum;
unsigned int  StartOffset;
unsigned int  EndOffset;
unsigned int  POCLength;
unsigned int  FCPLength;
unsigned int  APPLength;
unsigned char* BytePtr ;
unsigned int  StartOfSpaceFlashAddr;
unsigned int  EndOfSpaceFlashAddr;
unsigned int  SpareAppSpace;
unsigned int  WriteLength;

unsigned int   POCSpareSpace;
unsigned char *POCSpacePtr;
unsigned int   FCPSpareSpace;
unsigned char *FCPSpacePtr;
unsigned int   APPSpareSpace;
unsigned char *APPSpacePtr;

extern bool AutoExit ;

char* CompileDate ;
char* CompileTime ;
char  PaylinkSerialNumber[12];
char  ShareName[32];

bool Force ;
bool CheckMode ;
bool Verbose ;
bool ShowTraffic ;

PCInternalBlock* PCInternal ;

USBAccess*  ThePort ;
USBMilan*   Milan ;


char* ErrorData = NULL;

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
// ********************************************************************************
// ********************************************************************************
// Calculate the POC checksum
// ********************************************************************************
// ********************************************************************************
bool CheckPOC(void)
{
  if (*POCFlagPtr == AARDVARK_FLAG)
  {
    // Calculate the checksum of the application file
    // Always start with the checksum set to the default value
    unsigned int OriginalXsum = *POCChecksumPtr ;
    *POCChecksumPtr = DEFAULT_CHECKSUM;
    MyChecksum = 0;
    StartOffset = *POCAreaStartAddrPtr - START_OF_FLASH_AREA;
    EndOffset = *POCAreaEndAddrPtr - START_OF_FLASH_AREA;  // points to the last byte tobe included in the checksum
    POCLength = EndOffset - StartOffset + 1;
    BytePtr = &ProgramSpace[StartOffset];

    for (unsigned int Count = StartOffset ; Count <= EndOffset ; Count++)
    {
      MyChecksum += *BytePtr++;
    }
    // find how much code space is spare
    POCSpareSpace = 0;
    POCSpacePtr = (unsigned char *)POCFlagPtr;
    POCSpacePtr--;
    while ( *POCSpacePtr-- == 0xFF )
    {
      // code space not used
      POCSpareSpace++;
    }

    if (Verbose)
        {
        printf("POC Version:       v%u.%u.%u.%u\n", POCVersionPtr[0], POCVersionPtr[1], POCVersionPtr[2], POCVersionPtr[3]) ;
        }

    // Print some file details
    if (OriginalXsum == MyChecksum)
    {
      return true ;
    }
    else
    {
    fprintf(stderr, "POC Code Xsum fail %08x<>%08x\n", OriginalXsum, MyChecksum) ;
    return false ;
    }
  }
  else
  {
    fprintf(stderr, "No POC Code in image\n") ;
    return false ;
  }
}


// ********************************************************************************
// ********************************************************************************
// Calculate the FCP checksum
// ********************************************************************************
// ********************************************************************************

bool CheckFCP(void)
{
  if (*FCPFlagPtr == AARDVARK_FLAG)
  {
    // Calculate the checksum of the application file
    // Always start with the checksum set to the default value
    unsigned int OriginalXsum = *FCPChecksumPtr ;
    *FCPChecksumPtr = DEFAULT_CHECKSUM;
    MyChecksum = 0;
    StartOffset = *FCPAreaStartAddrPtr - START_OF_FLASH_AREA;
    EndOffset = *FCPAreaEndAddrPtr - START_OF_FLASH_AREA;  // points to the last byte tobe included in the checksum
    FCPLength = EndOffset - StartOffset + 1;
    BytePtr = &ProgramSpace[StartOffset];

    for (unsigned int Count = StartOffset ; Count <= EndOffset ; Count++)
    {
      MyChecksum += *BytePtr++;
    }

    // find how much code space is spare
    FCPSpareSpace = 0;
    FCPSpacePtr = (unsigned char *)FCPFlagPtr;
    FCPSpacePtr--;
    while ( *FCPSpacePtr-- == 0xFF )
    {
      // code space not used
      FCPSpareSpace++;
    }

    // Print some file details
    if (Verbose)
        {
        printf("FCP Version:       v%u.%u.%u.%u\n", FCPVersionPtr[0], FCPVersionPtr[1], FCPVersionPtr[2], FCPVersionPtr[3]) ;
        }

    // Print some file details
    if (OriginalXsum == MyChecksum)
    {
      return true ;
    }
    else
    {
      fprintf(stderr, "FCP Code Xsum fail %08x<>%08x\n", OriginalXsum, MyChecksum) ;
      return false ;
    }
  }
  else
  {
  fprintf(stderr, "FCP Code in image\n") ;
  return false ;
  }
}


// ********************************************************************************
// ********************************************************************************
// Calculate the Application checksum
// ********************************************************************************
// ********************************************************************************
bool CheckApp(void)
{
  if (*ApFlagPtr == AARDVARK_FLAG)
  {
    // Calculate the checksum of the application file
    // Always start with the checksum set to the default value
    unsigned int OriginalXsum = *ApChecksumPtr ;
    *ApChecksumPtr = DEFAULT_CHECKSUM;
    MyChecksum = 0;
    StartOffset = *ApAreaStartAddrPtr - START_OF_FLASH_AREA;
    EndOffset   = *ApAreaEndAddrPtr   - START_OF_FLASH_AREA;  // points to the last byte to be included in the checksum
    APPLength = EndOffset - StartOffset + 1;
    BytePtr = &ProgramSpace[StartOffset];

    for (unsigned int Count = StartOffset ; Count <= EndOffset ; Count++)
    {
      MyChecksum += *BytePtr++;
    }

    // find how much code space is spare
    APPSpareSpace = 0;
    APPSpacePtr = (unsigned char *)ApFlagPtr;
    APPSpacePtr--;
    while ( *APPSpacePtr-- == 0xFF )
    {
      // code space not used
      APPSpareSpace++;
    }

    // ApEndOfChecksumAddrPtr points to the last byte of code
    StartOfSpaceFlashAddr = *ApFirstSpareAddrPtr;
    EndOfSpaceFlashAddr = *ApLastSpareAddrPtr;
    SpareAppSpace = EndOfSpaceFlashAddr - StartOfSpaceFlashAddr;


    // Print some file details
    if (Verbose)
        {
        printf("APP Version:       v%u.%u.%u.%u,  %s, %s\n", ApVersionPtr[0], ApVersionPtr[1], ApVersionPtr[2], ApVersionPtr[3], ApDatePtr, ApTimePtr) ;
        }

    if (OriginalXsum == MyChecksum)
    {
      return true ;
    }
    else
    {
      fprintf(stderr, "App Code Xsum fail %08x<>%08x\n", OriginalXsum, MyChecksum) ;
      return false ;
    }
  }
  else
  {
  fprintf(stderr, "FCP Code in image\n") ;
  return false ;
  }
}





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
    fprintf(stderr, "Error: %s", Message);
    if (strlen(PaylinkSerialNumber))
        {
        fprintf(stderr, " (Serial No. %s)", PaylinkSerialNumber) ;
        }
    fprintf(stderr, "\n\n") ;
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
    int    ms              =  0;
    int    Send            =  0;
    double  t1              =  0;
    double  t2              =  0;
    char*   Base            = NULL;
    timeval old_time;
    timeval new_time;
    struct  timezone tz;


    ThePort = new HIDAccess((char*)PRODUCT_NAME, USB_VID, USB_HID, 0) ;
    Milan   = new USBMilan(ThePort) ;


    extern void OurProcessIncomingPacket(unsigned short Address, int Value) ;
    Milan->ProcessIncomingPacket = OurProcessIncomingPacket ;


    /*-- Decode Command Line Arguments Supplied -----------------------*/
    while ((option=getopt(argc,argv,"?fvcks:")) != -1)
        {       switch(option)
                    {
                case 'f':     Force     = true;                          break;
                case 's':     strcpy(PaylinkSerialNumber, optarg);       break;
                case 'k':     ForceKernelUpdate = true ;                 break;
                case 'c':     CheckMode = true;                          break;
                case 'v':     Verbose   = true;                          break;
                case '?':
                default :
                        printf("%s:\r\n", argv[0]);
                        printf(" -f         Force the loading of firmware.\r\n");
                        printf(" -c         Check if firmware upgrade is required.\r\n");
                        printf(" -s         Program specific serial number.\r\n");
                        printf(" -v         Verbose Diagnostics.\r\n");
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
        exit(0) ;
        }

    PCInternal           = PC_INTERNAL_BLOCK(Base) ;
    PCInternal->FlagWord = AARDVARK_FLAG_WORD ;
    PCInternal->USBUsage = FLASH_LOADER ;

        /*-- This is a total hack, but the structures not going to change -*/
    CompileDate = Base + 48 ;
    CompileTime = Base + 64 ;



    /* ----------  Get the firmware image */
    ClearLoadSpace() ;
    char* ErrMsg = ReadSRec(0) ;
    if (ErrMsg)
    {
        ThePort->USBClose() ;
        ErrorExit(ErrMsg) ;
    }

    if (*ApFlagPtr != AARDVARK_FLAG)
    {
      fprintf (stderr, "Application Area: \tFlag = 0x%X\n", *ApFlagPtr) ;
      fprintf (stderr, "Given image not an Aardvark application\n") ;
      exit(0) ;
    }


    bool Good = CheckFCP() & CheckPOC() & CheckApp() ;
    if (!Good)
    {
        ThePort->USBClose() ;
        ErrorExit((char*)"Checksum Error") ;
    }


        /*-- Now, spend 5 seconds trying to get the FTDI chip -------------*/
    CodeVersion       = -1 ;
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


        /*-----------------------------------------------------------------*\
         OK - All finished - we've got a kosher image loaded & ready to program
                       - do we want to do "anything"
        \*-----------------------------------------------------------------*/

    if (ApVersionPtr[0]   == ((CodeVersion >> 24 ) & 0xff) &&
        ApVersionPtr[1]   == ((CodeVersion >> 16 ) & 0xff) &&
        ApVersionPtr[2]   == ((CodeVersion >> 8  ) & 0xff) &&
        ApVersionPtr[3]   == ((CodeVersion       ) & 0xff) &&
        ApChecksumPtr[0]  == ((CodeChecksum      ) & 0xff) &&
        ApChecksumPtr[1]  == ((CodeChecksum >> 8 ) & 0xff) &&
        ApChecksumPtr[2]  == ((CodeChecksum >> 16) & 0xff) &&
        ApChecksumPtr[3]  == ((CodeChecksum >> 24) & 0xff) &&
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
            AutoExit = (ApVersionPtr[0] >= 3) ;
            }
        }
    else
        {
        AutoProgram = true ;
        AutoExit = true ;
        }


    if (((KernelCodeVersion >> 8) & 0xff) < 3)
        {
        fprintf(stderr, "*** This Paylink cannot be be re-programmed.\n"
                        "    Its reprogramming code is version %d.%d which is not Linux compatible\n"
                        "    It will have to be reprogrammed on a Windows system.\n",
                    (KernelCodeVersion >> 8) & 0xff, KernelCodeVersion  & 0xff);
        exit(0) ;
        }



    printf("Current Firmware: %s\r\n", SetVersion((CodeVersion >> 24) & 0xff)) ;
    printf("                  %d.%d.%d\n",        (CodeVersion >> 16) & 0xff,
                                                  (CodeVersion >>  8) & 0xff,
                                                  (CodeVersion      ) & 0xff) ;


    if (CompileDate[0] != '\0' && CompileTime[0] != '\0')
        {
        printf("                  Compiled on %s at %s\n", CompileDate,
                                                 CompileTime) ;
        }

    if (Force || AutoProgram)
        {
        printf("New Firmware:     %s\r\n", SetVersion(ApVersionPtr[0])) ;

        printf("                  %d.%d.%d\n", ApVersionPtr[1],
                                           ApVersionPtr[2],
                                           ApVersionPtr[3]) ;

        printf("                  Compiled on %s at %s\n", ApDatePtr, ApTimePtr) ;

        if (!AutoProgram)
            {
            printf("Re-programming only because of [-F] Force flag\n") ;
            }


        if ((((KernelCodeVersion >> 24) & 0xff) != FCPVersionPtr[0])
          ||(((KernelCodeVersion >> 16) & 0xff) != FCPVersionPtr[1])
          ||(((KernelCodeVersion >>  8) & 0xff) != FCPVersionPtr[2])
          ||(((KernelCodeVersion      ) & 0xff) != FCPVersionPtr[3]))
            {
            ForceKernelUpdate = true ;
            printf("Kernel Version changed\n") ;
            }



        // These are out of date now, so make sure we don't re-use them!
        CompileDate[0] = '\0' ;
        CompileTime[0] = '\0' ;

        printf("Programming... ") ;
        fflush(stdout) ;
        ProgramCard();
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
