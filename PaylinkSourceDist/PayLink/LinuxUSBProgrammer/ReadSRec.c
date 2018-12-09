/* -----------------02/08/2007 08:19-----------------
 *
 * 2/8/2007   AJT   - ReadSRec is now used to read multiple records into the same
 *                    memory space so ReadSRec() can no longer pre clear the
 *                    memory space. A new routine, ClearLoadSpace, has been created
 *                    to clear the memory space. It is important that the first call
 *                    to ReadSRec is preceeded by a call to ClearLoadSpace.
 *                  - Corrected Program space allocation from 0x100000 to 0x40000.
 *                  - The CheckChecksum routine has been updated to check the old
 *                    format checksum (0x0 to end of app) as well as the new format
 *                    checksum (kernel and app separately). Noteable differences
 *                    in the checksums is that unused code space is no longer checked
 *                    and the compile date and time do not affect the checksum.
 *
 * --------------------------------------------------*/



#include <stdio.h>
#include <stdlib.h>
#ifndef __linux__
#include <windows.h>
#include <io.h>
#else
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#endif
#include "../Headers.h"
#include "ReadSRec.h"
#include "../../Kernel.h"

unsigned char ProgramSpace[0x40000] ;            // 256K of program space
unsigned int  MaxAddress = 0 ;


static int Hex(char Ch)
{
  return ('0' <= Ch && Ch <= '9') ? (Ch & 0xf) : (Ch & 0xf) + 9 ;
}



void ClearLoadSpace(void)
{
  unsigned short i;

  // initialise unused data to 0xFF
  memset(ProgramSpace, 0xff, sizeof ProgramSpace) ;     // Set to unprogrammed

  // The area below 0x8000 can have a large gap in it. It is important to some Checksum routines
  // that these areas are forced to a repeatable value. The programming routine does not download 128 byte
  // chunks of memory that are all 0xFF. So if we were not careful some 4K erase blocks of memory would not be
  // erased because no new code would be sent to them. So the next loop sets one byte in every erase block to
  // ensure that every block is erased and therefore put to a consistent value.
  for (i = 0 ; i < 0x8000 ; i+= 0x1000)
  {
    ProgramSpace[i] = 0 ;                             // Make sure flash kernel space programmed
  }
}


char* ReadSRec(char* FileName)
{
  FILE*        InputFile ;
  int          i ;
  char         InputLine[256] ;
  unsigned int Address ;
  int          Length ;
  int          Checksum ;
  static char  ErrorMsg[1024] ;


  // Now we read in the S Record file
  MaxAddress = 0;

  if (FileName == 0)
  {
    InputFile = stdin ;
    FileName = (char *)"stdin" ;
  }
  else
  {
    InputFile = fopen(FileName, "r") ;
  }


  if (!InputFile)
  {
    sprintf(ErrorMsg, "Could not open <%s> - %s\n", FileName, strerror(errno)) ;
    return ErrorMsg ;
  }

  while (fgets(InputLine, sizeof InputLine, InputFile))
  {
    if (InputLine[0] != 'S')
    {
      sprintf(ErrorMsg, "Invalid line in <%s>:\n%s\n", FileName, InputLine) ;
      fclose(InputFile) ;
      return ErrorMsg ;
    }

    Length = (Hex(InputLine[2]) << 4) + Hex(InputLine[3]) ;

    // Checksum check
    Checksum = 0 ;
    for (i = 2 ; i < Length * 2 + 4 ; )          // Include Checksum in calculation
    {
      Checksum += (Hex(InputLine[i++]) << 4) ;
      Checksum +=  Hex(InputLine[i++]) ;
    }

    if ((Checksum & 0xff) != 0xff)
    {
      sprintf(ErrorMsg, "Checksum fail in <%s> (%x):\n%s\n", FileName, Checksum, InputLine) ;
      fclose(InputFile) ;
      return ErrorMsg ;
    }

    i       = 4 ;           // At start of address
    Address = 0 ;
    switch(InputLine[1])
    {
      case '0':                // Start of file - ignore
      {
        break ;
      }
      case '2':                // 24 bit data records - 3 byte addresses
      {
        Address  = (Hex(InputLine[i++]) << 20) ;
        Address += (Hex(InputLine[i++]) << 16) ;
        // break ;                              // drop through
      }

      case '1':                // 16 bit data records - 2 byte addresses
      {
        Address += (Hex(InputLine[i++]) << 12) ;
        Address += (Hex(InputLine[i++]) << 8 ) ;
        Address += (Hex(InputLine[i++]) << 4 ) ;
        Address += (Hex(InputLine[i++])      ) ;

        while (i < 2 + Length * 2)
        {
          if (Address >= sizeof ProgramSpace)
          {
            sprintf(ErrorMsg, "Illegal address in <%s> (%x):\n%s\n", FileName, Address, InputLine) ;
            fclose(InputFile) ;
            return ErrorMsg ;
          }
          ProgramSpace[Address]    = Hex(InputLine[i++]) << 4 ;
          ProgramSpace[Address++] += Hex(InputLine[i++]) ;
          if (Address > MaxAddress)
          {
            MaxAddress = Address ;
          }
        }
        break ;
      }

      case '8':                // Start addresses - ignore
      case '9':
      {
        break ;
      }

      default:
      {
        sprintf(ErrorMsg,  "Unrecognised line in <%s>:\n%s\n", FileName, InputLine) ;
        fclose(InputFile) ;
        return ErrorMsg ;
        //break;  // unreachable code
      }
    }
  }

  fclose(InputFile) ;
  return NULL ;
}





char* CheckCheckSum(void)
{
  TYPE_CODE_INFO* ApHeader = (TYPE_CODE_INFO*)APPLICATION_CODE_INFO_ADDR;
  TYPE_CODE_INFO* KeHeader = (TYPE_CODE_INFO*)KERNEL_CODE_INFO_ADDR;
  unsigned char* Flag              = &ProgramSpace[(long)&ApHeader->Flag];        // &ProgramSpace[0x8000] ;
  unsigned char* RomChecksum       = &ProgramSpace[(long)&ApHeader->RomChecksum]; // &ProgramSpace[0x8004] ;
  unsigned char* EndOfRom          = &ProgramSpace[(long)&ApHeader->EndOfRom];    // &ProgramSpace[0x8008] ;
  unsigned char* ApVersion         = &ProgramSpace[(long)&ApHeader->Version];     // &ProgramSpace[0x800C] ;

  unsigned char* KernelFlag        = &ProgramSpace[(long)&KeHeader->Flag];        // &ProgramSpace[0x1000] ;
  unsigned char* KernelRomChecksum = &ProgramSpace[(long)&KeHeader->RomChecksum]; // &ProgramSpace[0x1004] ;
  unsigned char* KernelEndOfRom    = &ProgramSpace[(long)&KeHeader->EndOfRom];    // &ProgramSpace[0x1008] ;
  unsigned char* KernelVersion     = &ProgramSpace[(long)&KeHeader->Version];     // &ProgramSpace[0x100C] ;

  unsigned char* KernelFlagCopy    = &ProgramSpace[(long)&KeHeader->Flag + GETLONG(EndOfRom)];

  unsigned char* WordPtr ;
  short          Word ;
  unsigned int   MyChecksum;
  static char    Message[1024] ;
  short          KernelPresent;


  if (GETLONG(Flag) != AARDVARK_FLAG)
  {
    // Application code info not found
    return "No checksum block found\n" ;
  }

  // check if there is a kernel present
  KernelPresent = (GETLONG(KernelFlag) == AARDVARK_FLAG);

  if ( !KernelPresent )
  {
    // Check old format checksum
    printf("Kernel Not Present\n");

    MyChecksum = 0;
    for (WordPtr = ProgramSpace ; WordPtr < &ProgramSpace[GETLONG(EndOfRom)] ;
                                  WordPtr += 2)
    {
      Word = GETWORD(WordPtr) ;
      MyChecksum += Word ;
    }

    Word = GETWORD(RomChecksum) ;
    MyChecksum -= Word ;
    Word = GETWORD(RomChecksum + 2) ;
    MyChecksum -= Word ;

    if (MyChecksum != GETLONG(RomChecksum))
    {
      sprintf(Message, "Checksum error: Generated = %08ux, Stored = %08ux, Checksum address = 0x%ux\n",
                        MyChecksum, GETLONG(RomChecksum), (int)(long)&ApHeader->RomChecksum) ;
      return Message ;
    }

    if (MaxAddress != GETLONG(EndOfRom))
    {
      sprintf(Message, "Length mismatch: Read = %ud, Stored = %d\n",
                        MaxAddress, GETLONG(EndOfRom)) ;
      return Message ;
    }
  }
  else
  {
    // kernel is present so check all 3 code blocks
    printf("Kernel Present\n");

    // ********************************************************************************
    // Check Application checksum
    // New Checksum is split into 3 areas. 1. The code before the CODEINFO block, the version (int), and the code after the CODEINFO block
    MyChecksum = 0;
    // Get the code before the codeinfo block (NB this might be zero length)
    for (WordPtr = &ProgramSpace[APPLICATION_CODE_START_ADDR] ; WordPtr < &ProgramSpace[APPLICATION_CODE_INFO_ADDR] ; WordPtr += 2)
    {
      Word = GETWORD(WordPtr) ;
      MyChecksum += Word ;
    }

    // add the version
    Word = GETWORD(ApVersion) ;
    MyChecksum += Word ;
    Word = GETWORD(ApVersion + 2) ;
    MyChecksum += Word ;

    // get the code after the code info block
    for (WordPtr = &ProgramSpace[APPLICATION_JUMP_INFO_ADDR] ; WordPtr < &ProgramSpace[GETLONG(EndOfRom)] ; WordPtr += 2)
    {
      Word = GETWORD(WordPtr) ;
      MyChecksum += Word ;
    }

    if (MyChecksum != GETLONG(RomChecksum))
    {
      sprintf(Message, "Application Checksum error: Generated = %08ux, Stored = %08ux, Checksum address = 0x%x\n",
                        MyChecksum, GETLONG(RomChecksum), (int)(long)&ApHeader->RomChecksum) ;
      return Message ;
    }


    // ********************************************************************************
    // Check Kernel checksum
    MyChecksum = 0;
    for (WordPtr = &ProgramSpace[KERNEL_CODE_START_ADDR] ; WordPtr < &ProgramSpace[KERNEL_CODE_INFO_ADDR] ; WordPtr += 2)
    {
      Word = GETWORD(WordPtr) ;
      MyChecksum += Word ;
    }

    Word = GETWORD(KernelVersion) ;
    MyChecksum += Word ;
    Word = GETWORD(KernelVersion + 2) ;
    MyChecksum += Word ;

    for (WordPtr = &ProgramSpace[KERNEL_JUMP_INFO_ADDR] ; WordPtr < &ProgramSpace[GETLONG(KernelEndOfRom)] ; WordPtr += 2)
    {
      Word = GETWORD(WordPtr) ;
      MyChecksum += Word ;
    }

    if (MyChecksum != GETLONG(KernelRomChecksum))
    {
      sprintf(Message, "Checksum error: Generated = %08ux, Stored = %08ux, Checksum address = 0x%x\n",
                        MyChecksum, GETLONG(KernelRomChecksum), (int)(long)&KeHeader->RomChecksum) ;
      return Message ;
    }


    // ********************************************************************************
    // Now check the Kernel copy
    if (KernelFlagCopy >= ProgramSpace + sizeof ProgramSpace)
    {
      printf("No space for Kernel Copy\n") ;
    }
    else if ( GETLONG(KernelFlagCopy) == AARDVARK_FLAG)
    {
      // Kernel copy exists
      printf("Kernel Copy present\n");
      if ( memcmp(&ProgramSpace[0], &ProgramSpace[GETLONG(EndOfRom)], GETLONG(KernelEndOfRom)) )
      {
        sprintf(Message, "Kernel Copy does not match the Kernel\n") ;
        return Message ;
      }

      if (MaxAddress != (GETLONG(EndOfRom) + GETLONG(KernelEndOfRom)))
      {
        sprintf(Message, "Length mismatch (with Kernel Copy): Read = 0x%X, Stored = 0x%X\n", MaxAddress, (GETLONG(EndOfRom) + GETLONG(KernelEndOfRom))) ;
        return Message ;
      }
    }
    else
    {
      printf("Kernel Copy not found\n");
      if (MaxAddress != GETLONG(EndOfRom))
      {
        sprintf(Message, "Length mismatch: Read = 0x%X, Stored = 0x%X\n", MaxAddress, GETLONG(EndOfRom)) ;
        return Message ;
      }
    }
  }
  return 0 ;
}
