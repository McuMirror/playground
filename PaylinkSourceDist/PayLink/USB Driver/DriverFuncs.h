//-------------------------------------------------
//
// Global Data
//
//-------------------------------------------------

#ifndef DRIVER_FUNCS
#define DRIVER_FUNCS

#include "PCStuff.h"            // The *contents* of this may vary in other projects!

enum
    {
    NotConnected,
    Pause,
    CheckingPC,
    CheckingUSB,
    Sending,
    Reading,
    DriverAbort
    } ;

extern          int     CurrentDriverState ;
extern          int     MemoryInterlock ;

extern unsigned int     DiagReadIndex ;

extern unsigned char*   Base ;
extern volatile int*    SharedMemoryBase ;      // This is the shared memory segment for the application
extern          int     Mirror[2048] ;          // This is what we know that the H8 knows about
extern          int     RecoveryMirror[2048] ;  // This is what the H8 tells us during a restart
extern PCInternalBlock* PCInternal ;

extern          int     LatencyParameter ;

extern          int     ReportedCardSelfTest ;

extern          char    DriverVersion[] ;
extern          char    ConfigFileStr[] ;
extern          char    SharedSegmentName[] ;

void StartDriverThread(void) ;
void StartDriverThread(char * SerialNumber);
void SetupSharedSegment(void* SegmentAddress, char* DriverType) ;
void uDriverOK(void) ; // This should be called repeatedly by the PC exec, to detect hangs due to USB problems




void DiagPutChar(char TheChar) ;
struct timeb GetTheTime(void) ;

extern volatile AESLong MonitorWriteIndex ;    // The index of the next character to be written to the buffer
extern          AESLong MonitorIndexMask ;   // The mask applied to the above to access that character in Buffer[]
extern          char    MonitorBuffer[BUFFER_SIZE] ;
extern          bool    MonitoringComms ;

void MonitorPrintTime(struct timeb ClockNow) ;
void MonitorPutChar(char TheChar) ;
void MonitorPrintf(char* Format, ...) ;


bool CommunicateWithCard(void) ;

#ifdef __linux__
    extern pthread_mutex_t  Mutex ;
#else
    extern HANDLE Timer ;
    extern HANDLE Mutex ;
#endif

// This function is part of the MCL USB support code. On Linux and PC Standalone it just need a null stub (it's not called)
bool SetupMergeProcess(void) ;
bool MergeProcessInput(int Index, int Value) ;
int  MergeProcessOutput(int Index, int Value) ;

// Check that int is exactly 4 bytes - if it is not one of these will generate a compile fault
extern char CheckIntSize[sizeof (int) - 3] ;
extern char CheckIntSize2[5 - sizeof (int)] ;

#endif
