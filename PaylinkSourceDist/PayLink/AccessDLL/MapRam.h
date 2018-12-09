/*******************************************************************************
 * Copyright (c) 2003 Aardvark Embedded Solutions.
 *
 *
 * File Name:
 *
 *     MapRam.h
 *
 * Description:
 *
 *     This file contains the required function to Map the Dual Port Ram
 *
 *****************************************************************************/
#define EXPORT               /* Make sure PlxApi does not define PLX functions as DLL */

#include <stdio.h>

#ifndef __linux__
    #include "PlxApi.h"
    #include "Reg9030.h"
#else
    #define HANDLE int
    #include "PlxError.h"
#endif
#include "../Headers.h"        /* definitions shared with our H8 */

#ifdef __cplusplus
    extern "C" {
#endif

extern int      OurInterfaceErrror ;
extern HANDLE   MMFile ;


/****************************************************************************
These values are constants, so we don't to bother keep reading them.
*****************************************************************************/
extern volatile void*             SharedMemoryBase ;

/* Because the access is different, we have to have different names */
#ifdef __cplusplus
    #define BasicControl  ((BasicControlBlock*)0)
    extern InputAreaBlock*    InputArea ;
    extern OutputAreaBlock*   OutputArea ;
#else
    extern BasicControlBlock* BasicControl ;
    extern InputAreaBlock*    InputDpArea;
    extern OutputAreaBlock*   OutputDpArea ;
#endif



int MapDPRam(void) ;
int MapNamedDPRam(char * SerialNumber);

void UnMapRam(void) ;

#ifndef __linux__
    /********************************************
    The PLX DLLMain - not in its header file
    ********************************************/
    BOOLEAN WINAPI
    PlxDLLMain( HANDLE hInst,
                U32    ReasonForCall,
                LPVOID lpReserved
                ) ;
#endif

#ifdef __cplusplus
}
#endif
