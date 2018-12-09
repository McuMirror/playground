/******************************************************

This handles all the detail of accessing an arbitrary USB link.

The application merely deals in packets

*******************************************************/
#ifndef USB_ACCESS
#define USB_ACCESS

//
// This class allows for arbitrary access to a USB chip.
//

extern char* GetErrorText(unsigned long LastError) ;

class WinUSB ;

class USBAccess
    {
  public:
    enum
        {
        UNASSIGNED,
        FTDI,
        HID,
        VCP }   Type ;

    bool        Opened ;
    bool        Recovered ;
    int         OpenCalls ;
    char        DeviceSerialNumber[12];

    char*       ProductName ;
    unsigned int Vid ;
    unsigned int Pid ;

    #ifndef __linux__
      WinUSB*   USBFuncs ;
    #endif
    virtual bool CommonUSBOpen() = 0 ;

    bool         USBOpen() ;
    bool         USBOpenSpecific(char *Serial) ;
    virtual void USBClose(void) = 0 ;
    virtual bool CheckUSBNowOK(char *) = 0 ;
    bool         RecoverUSB(void) ;
    virtual void RecoverDevice(void) {}
    virtual bool SetPortF(int BaudRate, int Bits, int Stop, int Parity) { return true ; }
    virtual bool SetPortW(int BaudRate, int Bits, int Stop, int Parity) { return true ; }
    virtual bool        WriteBuffer(char* Address, int Length) = 0 ;
    virtual int         ReadBuffer (char* Address, int Length) = 0 ;

    USBAccess(char* ProductName, int Vid, int Pid) ;
    virtual ~USBAccess() = 0 ;
    } ;


//
// This class allows for access using the above to a Paylink device
//
class USBMilan
    {
    USBAccess* Port ;                         // This is the related port

    #ifndef __linux__
      char        TxSpace[2048 * 6 + 16] ;              // an entry for each word in the shared memory
      char*       TxBuffer ;
    #else
      unsigned char  TxSpace[2048 * 6 + 16] ;     // an entry for each word in the shared ememory
      unsigned char* TxBuffer ;
    #endif
#define Sizeof_TxBuffer (sizeof TxSpace - 16)
    short         TxBufferIndex ;

    unsigned char RxSpace[4096] ;

  public:
    void        QueuePacket(unsigned short Address, int Value) ;
    int         SendBuffer(void) ;
    void        ResetTxBuffer(void) ;
    bool        ProcessIncomingStream(void) ;                                  // This will call ProcessIncomingPacket as necessary
    void        (*ProcessIncomingPacket)(unsigned short Address, int Value) ;  // This is supplied by the application!
    char        Packet[6] ;
    unsigned short PacketIndex ;
    USBMilan(USBAccess* Port)
        {
        this->Port = Port ;
        TxBuffer  = TxSpace + 16 ;
        TxBufferIndex = 0 ;
        ProcessIncomingPacket = 0 ;
        TxSpace[0] = 0 ;
        RxSpace[0] = 0 ;
        Packet[0] = 0 ;
        PacketIndex = 0 ;
        }
    } ;


//-------------------------------------------------
// Diagnostics Support
//-------------------------------------------------

void  DiagPrintf(const char* Format, ...) ;
void  DiagText(char* Line) ;

// Check that int is exactly 4 bytes - if it is not one of these will generate a compile fault
extern char CheckIntSize[sizeof (int) - 3] ;
extern char CheckIntSize2[5 - sizeof (int)] ;


#endif
