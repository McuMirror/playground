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
 * AJT  12/7/2012   Modified to work with GNURX files.
 *
 * --------------------------------------------------*/



#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include "ReadSRec.h"
#include "RXChecksumConstants.h"

unsigned char ProgramSpace[SIZE_OF_FLASH_AREA] ;   // 512K of program space
unsigned int LowestFlashAddress = 0xFFFFFFFF;                  // the lowest address written to in flash
unsigned int LowestProgramSpaceOffset;


static int Hex(char Ch)
{
  return ('0' <= Ch && Ch <= '9') ? (Ch & 0xf) : (Ch & 0xf) + 9 ;
}


void  ClearLoadSpace(void)
{
  memset(ProgramSpace, 0xff, sizeof ProgramSpace) ;     // Set to unprogrammed
}



char* ReadSRec(char* FileName)
{
  FILE*         InputFile ;
  int           i ;
  char          InputLine[256] ;
  unsigned int  Address ;
  int           Length ;
  int           Checksum ;
  static char   ErrorMsg[1024] ;

  if (FileName == 0)
  {
    InputFile = stdin ;
    FileName = (char *)"stdin" ;
  }
  else
  {
    InputFile = fopen(FileName, "r") ;
  }


  // Now we read in the S Record file
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
      case '3':                // 32 bit data records - 4 byte addresses
      {
        Address   = (Hex(InputLine[i++]) << 28) ;
        Address  += (Hex(InputLine[i++]) << 24) ;
        // break ;                              // drop through
      }
      case '2':                // 24 bit data records - 3 byte addresses
      {
        Address  += (Hex(InputLine[i++]) << 20) ;
        Address  += (Hex(InputLine[i++]) << 16) ;
        // break ;                              // drop through
      }

      case '1':                // 16 bit data records - 2 byte addresses
      {
        Address += (Hex(InputLine[i++]) << 12) ;
        Address += (Hex(InputLine[i++]) << 8 ) ;
        Address += (Hex(InputLine[i++]) << 4 ) ;
        Address += (Hex(InputLine[i++])      ) ;

        if ( Address <= LowestFlashAddress )
        {
          // lower the starrt address
          LowestFlashAddress = Address;
          LowestProgramSpaceOffset = Address - START_OF_FLASH_AREA;
          //printf("New lowest add:0x%X offset: 0x%X\n", LowestFlashAddress, LowestProgramSpaceOffset);
        }

        // Flash memory starts at 0xFFF8 0000. We reference everything from there.
        Address -= START_OF_FLASH_AREA;

        while (i < 2 + Length * 2)
        {
          unsigned char Byte;

          if (Address >= sizeof ProgramSpace)
          {
            sprintf(ErrorMsg, "Illegal address in <%s> (%x):\n%s\n", FileName, Address, InputLine) ;
            fclose(InputFile) ;
            return ErrorMsg ;
          }
          Byte  = (Hex(InputLine[i++]) << 4 ) ;
          Byte += Hex(InputLine[i++]);
          if ( (ProgramSpace[Address] != 0xFF)   // address has been written to already
             && (Byte != ProgramSpace[Address])  // data has changed
             )
          {
            // Warn that the POC or FCP is different between the two builds
            printf("WARNING: Address 0x%X, App = 0x%02X, overwritten by FCP = 0x%02X\n", Address+START_OF_FLASH_AREA, ProgramSpace[Address], Byte);
          }
          ProgramSpace[Address++] = Byte;
        }
        break ;
      }

      case '7':                // 32 bit Start addresses - ignore
      case '8':                // 24 bit Start addresses - ignore
      case '9':                // 16 bit Start addresses - ignore
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




