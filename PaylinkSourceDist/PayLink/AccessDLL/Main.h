#ifdef __linux__
    #include <string.h>
    #define WINAPI
    #define ERROR_INVALID_DATA      1
    #define ERROR_GEN_FAILURE       2
    #define ERROR_NOT_READY         3
    #define ERROR_BAD_UNIT          4
    #define ERROR_BUSY          170
    #define ERROR_DEVICE_NOT_CONNECTED  1167
#else
    #pragma warning( disable : 4244)               // Lose the char = long warning
    #pragma warning( disable : 4510)               // Lose the constructor warning
    #pragma warning( disable : 4610)               // Lose the constructor warning
#endif

// Definitions for modules shared with H8

#define CODE_SPACE                  const
#define CODE_SPACE_READ_LONG(x)     (x)
#define CODE_SPACE_READ_SHORT(x)    (x)
#define CODE_SPACE_READ_CHAR(x)     (x)


#include <time.h>
#include <stdlib.h>
#include "MapRam.h"
#include "../../Aesimhei.h"
#include "../../AESMessages.h"                // Messages shared with the H8
#include "../../DESEncrypt.h"

