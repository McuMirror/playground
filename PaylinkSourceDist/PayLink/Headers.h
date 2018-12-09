/******************************************************

This provides a project independant way of getting the
special to project headers for PC programs


*******************************************************/
#define PROCESSOR PC     // to fail the test PROCESSOR==ATMEGA
#define ATMEGA 22

typedef          int AESLong ;                              // Our standard 32 bit integer
typedef unsigned int AESULong ;                             // Our standard 32 bit unsigned integer

#include "../Aesimhei.h"
#include "../ImheiInternal.h"
#include "../ImheiUSB.h"

#ifdef __cplusplus
#include "FtdiAccess.h" /* AES Ftdi Header */
#include "UsbAccess.h"  /* AES Ftdi Header */
#endif

#include "Trace.h"

// #include "AESFtdi.h"
