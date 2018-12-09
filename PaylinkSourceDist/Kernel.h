// The data is CODE_INFO area is not checksummed except for the
// version details which are added into the checksum to ensure the checksum changes
// if the version changes.

typedef struct
{
  // Do NOT change the size of this structure as it affects the loading of Kernels.
  AESULong       Flag;                          //  4 bytes
  AESLong        RomChecksum;                   //  4 bytes - checksum is a signed sum of words
  AESULong       EndOfRom;                      //  4 bytes - actually a pointer
  AESULong       Version;                       //  4 bytes
  unsigned char  Date[16];                      // 16 bytes
  unsigned char  Time[16];                      // 16 bytes
}TYPE_CODE_INFO;                                //=48 bytes = 0x30

typedef struct
{
  // Do NOT change the size of this structure as it affects the loading of Kernels.
  TYPE_CODE_INFO KernelInfo;
  AESULong JumpVectors[16];
  AESULong StartRamCode;
  AESULong EndRamCode;
  AESULong RamCodeAddress;
}TYPE_KERNEL_JUMP_INFO;

typedef void (*FUNC_PTR)(void) ;

// Kernel routines access table JumpVectors[].
enum
{
  JUMPVECTORS_REPROGRAM = 0,
  JUMPVECTORS_DOWNLOADCOPY,
  JUMPVECTORS_RAMCODE_UPDATEKERNEL,
};



#define KERNEL_CODE_START_ADDR      0x0000      // this address should be on a Flash Block erase boundary
#define KERNEL_CODE_INFO_ADDR       0x1000      // this address should be on a Flash Block erase boundary
#define KERNEL_JUMP_INFO_ADDR       (KERNEL_CODE_INFO_ADDR + sizeof(TYPE_CODE_INFO))  // must be immediately after the info block

#define APPLICATION_CODE_START_ADDR 0x8000      // this address should be on a Flash Block erase boundary
#define APPLICATION_CODE_INFO_ADDR  0x8000      // for backward compatibility this must stay at 0x8000
#define APPLICATION_JUMP_INFO_ADDR  (APPLICATION_CODE_INFO_ADDR + sizeof(TYPE_CODE_INFO))  // must be immediately after the info block


//       Add new entry points                                               Change vector index
//       vvvvvvvvvvvvvvvvvvvvv                                                       vvvv
#define  KERNEL_REPROGRAM()     {FUNC_PTR Vector = (FUNC_PTR) (KERNEL_JUMP_INFO_ADDR + 0x00);     Vector();}
#define  KERNEL_DOWNLOADCOPY()  {FUNC_PTR Vector = (FUNC_PTR) (KERNEL_JUMP_INFO_ADDR + 0x04);     Vector();}
#define  RAMCODE_UPDATEKERNEL() {FUNC_PTR Vector = (FUNC_PTR) (KERNEL_JUMP_INFO_ADDR + 0x08);     Vector();}

#define  START_APPLICATION()    {FUNC_PTR Vector = (FUNC_PTR) (APPLICATION_JUMP_INFO_ADDR + 0x00);Vector();}

