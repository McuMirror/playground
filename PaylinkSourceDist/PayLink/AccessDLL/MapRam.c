/*******************************************************************************
 * Copyright (c) 2003 Aardvark Embedded Solutions.
 *
 *
 * File Name:
 *
 *     MapRam.c
 *
 * Description:
 *
 *     This file contains the required function to Map some sort of memory
 *     so that we can talk to the H8.
 *
 *     The initial part is either:
 *      - Windows shared memory segement from USB driver
 *      - Windows PCI bus dual port RAM mapping
 *      - Linux shared memory segement from USB driver
 *
 *     In theory we ought to support Linux PCI bus mapping but, at present, we don't bother.
 *
 *****************************************************************************/
#ifdef __linux__
    #include <sys/types.h>
    #include <sys/mman.h>
    #include <fcntl.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <string.h>
    #define NO_SEGMENT -1
#else
    #define NO_SEGMENT 0
#endif

#include "MapRam.h"

volatile void *               SharedMemoryBase = NULL ;

char ShareName[50];

/* The C Access Pointers  */
BasicControlBlock *           BasicControl = NULL ;
InputAreaBlock *              InputDpArea = NULL ;
OutputAreaBlock *             OutputDpArea = NULL ;

/* The C++ Access Pointers */
InputAreaBlock*               InputArea = 0 ;
OutputAreaBlock*              OutputArea = 0 ;

int                           OurInterfaceErrror = 0 ;

HANDLE                        MMFile = NO_SEGMENT ;

static int                    SegmentSize ;

PCInternalBlock*              PCInternal = 0 ;

static int CommonMapDPRam(void);

#ifdef __linux__
  #define GLOBAL_SHARED_NAME SHARED_NAME
#endif

int MapNamedDPRam(char * SerialNumber)
    {
        // This function is used in multiple Paylink systems
        // to open a shared memory area with the serial number
        // as a part of the name.

        strcpy(ShareName, GLOBAL_SHARED_NAME);
        strcat(ShareName, SerialNumber) ;
        return CommonMapDPRam() ;
    }

int MapDPRam(void)
    {
        // This function provides backward compatibility,
        // with traditional, single Paylink systems.
        strcpy(ShareName, GLOBAL_SHARED_NAME) ;
        return CommonMapDPRam() ;
    }

static int CommonMapDPRam(void)
    {
    int             OpenResult = ApiInvalidDeviceInfo ;
    /************************************************************************

    First we try to map a memory segment shared with the USB driver

    ************************************************************************/

        // Even if we already have a file open, we need to map onto
        // the new view, as we may be looking at a different Paylink

    if (MMFile != NO_SEGMENT)
        {
                UnMapRam();
        }

    #ifndef __linux__
    /****************************
    Windows Mapping
    *****************************/
        MMFile = OpenFileMapping (FILE_MAP_WRITE,
                                  FALSE,
                                  ShareName) ;
        if (!MMFile)
            {                       // This is the *usual* situation - no global segment
            char* LocalName = ShareName ;
            while (*LocalName++ != '\\')
                { }
            MMFile = OpenFileMapping (FILE_MAP_WRITE,
                                      FALSE,
                                      LocalName) ;
            }

        SharedMemoryBase = (char *)MapViewOfFile (MMFile,
                                                  FILE_MAP_ALL_ACCESS,
                                                  0, 0, 0);
    #else
    /****************************
    Linux Mapping
    *****************************/
        MMFile = shm_open(ShareName, O_RDWR,  0777 );
        if (MMFile != NO_SEGMENT)
            {
            /*-- Calculate number of pages required -------------------*/
            int pagesize = getpagesize();
            int num      = abs(((EXTENDED_SHARED_SIZE) / pagesize));
            if ((num * pagesize) < (EXTENDED_SHARED_SIZE))
                num += 1;

            SegmentSize =  pagesize * num ;

            SharedMemoryBase = (u_char *)mmap( 0, SegmentSize, PROT_READ | PROT_WRITE, MAP_SHARED, MMFile, 0);
            if (SharedMemoryBase == MAP_FAILED)
                {
                SharedMemoryBase = NULL ;
                }
            }
    #endif

    if (MMFile != NO_SEGMENT && SharedMemoryBase != NULL)
        {                   // We've got USB driver shared memory available
        BasicControl = SHARED_ADDRESS(0) ;
        PCInternal   = PC_INTERNAL_BLOCK(BasicControl) ;
        if (BasicControl->FlagWord == AARDVARK_FLAG_WORD)
            {
            OpenResult = ApiSuccess ;
            }
        else
            {
            OpenResult = ApiInvalidDeviceInfo ;
            }
        }

    /************************************************************************

    If that didn't work and we're on Windows, try to set up PCI card DP memory access

    ************************************************************************/
    #ifndef __linux__
        if (!BasicControl)
            {
            DEVICE_LOCATION Device ;
            HANDLE          DeviceHandle = 0 ;
            HANDLE          NewHandle ;

            int             i ;
            int             Result ;

            PlxDLLMain(0, DLL_PROCESS_ATTACH, 0) ;

            i = 0 ;
            do
                {
                Device.BusNumber  = -1 ;
                Device.SlotNumber = -1 ;
                Device.VendorId   = -1 ;
                Device.DeviceId   = -1 ;


                sprintf(Device.SerialNumber, "AEIMHEI-%d", i) ;
                Result = PlxPciDeviceOpen(&Device, &NewHandle) ;
                if (Result == ApiSuccess)
                    {                    /* A device found */
                    if (DeviceHandle)               /* We "hold on" to any previous while we look for another */
                        {                           /* (If we're fault finding we wnat to open *any* 9030) */
                        PlxPciDeviceClose(DeviceHandle) ;
                        }
                    DeviceHandle = NewHandle ;

            #ifdef SET_UP_THE_9030_REGISTERS
                OpenResult = PlxRegisterWrite(DeviceHandle, PCI9030_RANGE_SPACE0, 0x0FFFE000) ;
                    if (OpenResult != ApiSuccess)
                        {
                        continue ;              /* Try next one!!! */
                        }
                    OpenResult = PlxRegisterWrite(DeviceHandle, PCI9030_REMAP_SPACE0, 0x00000001) ;
                    if (OpenResult != ApiSuccess)
                        {
                        continue ;              /* Try next one!!! */
                        }
                    OpenResult = PlxRegisterWrite(DeviceHandle, PCI9030_DESC_SPACE0,  0x00400000) ;
                    if (OpenResult != ApiSuccess)
                        {
                        continue ;              /* Try next one!!! */
                        }
                    OpenResult = PlxRegisterWrite(DeviceHandle, PCI9030_BASE_CS0,     0x00010001) ;
                    if (OpenResult != ApiSuccess)
                        {
                        continue ;              /* Try next one!!! */
                        }
            #endif


                    OpenResult = PlxPciBarMap(DeviceHandle, 2, (void *)&SharedMemoryBase) ;
                    if (OpenResult == ApiSuccess)
                        {
                        BasicControl = SHARED_ADDRESS(0) ;
                        if (BasicControl->FlagWord == AARDVARK_FLAG_WORD)
                            {                                   /* Aardvark flag found */
                            /* Set write wait states in case EEPROM hasn't */
                            PlxRegisterWrite(DeviceHandle, PCI9030_DESC_SPACE0, 0x50508000) ;
                            break ;
                            }
                        else
                            {
                            OpenResult = ApiInvalidDeviceInfo ;  /*  Carry On - record this failure */
                            }
                        }
                    }
                } while (++i < 128) ;
            }
    #endif

    if (!BasicControl)
        {
        UnMapRam() ;
        return OpenResult ;
        }

    if (BasicControl->FlagWord == AARDVARK_FLAG_WORD)
        {
        InputArea        = (InputAreaBlock *)BasicControl->InputPointer ;
        InputDpArea      = (InputAreaBlock *)SHARED_ADDRESS(BasicControl->InputPointer) ;
        OutputArea       = (OutputAreaBlock *)BasicControl->OutputPointer ;
        OutputDpArea     = (OutputAreaBlock *)SHARED_ADDRESS(BasicControl->OutputPointer) ;
        }

    return OpenResult ;
    }




void UnMapRam(void)
    {
    if (MMFile != NO_SEGMENT)
        {
        #ifdef __linux__
            if (SharedMemoryBase)
                {
                munmap((void*)SharedMemoryBase, SegmentSize);
                }
            close(MMFile);
        #else
            UnmapViewOfFile((void *)SharedMemoryBase);
            CloseHandle(MMFile);
        #endif
        }
    #ifndef __linux__
        else
            {
            PlxDLLMain(0, DLL_PROCESS_DETACH, 0) ;
            }
    #endif
    BasicControl     = NULL ;
    SharedMemoryBase = NULL ;
    MMFile           = NO_SEGMENT ;
    }




