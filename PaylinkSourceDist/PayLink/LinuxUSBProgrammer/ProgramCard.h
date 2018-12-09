/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
#ifndef ProgramCardH
#define ProgramCardH

extern USBAccess*  ThePort ;
extern USBMilan*   Milan ;

extern int  CodeApplication;
extern int  CodeChecksum;
extern int  CodeVersion;
extern int  KernelCodeVersion ;
extern int  CardSelfTest;
extern bool  ForceKernelUpdate;
extern bool  AutoProgram;
extern bool  AutoExit;
extern bool ShowTraffic ;

/*--------------------------------------------------------------------------*\
 * This is the data structure in the image. We know where to find this and
 * it tells us what we've got.
\*--------------------------------------------------------------------------*/
typedef struct
{
    AESULong        AardFlag;
    AESLong         RomCheckSum;
    AESULong        EndOfRom;
    unsigned char   VersionStatus;
    unsigned char   MajorVersion;
    unsigned char   MinorVersion;
    unsigned char   MinisculeVersion;

    unsigned char   CompileDate[16];
    unsigned char   CompileTime[16];
} HeaderBlock;

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
extern HeaderBlock *TheHeader;

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
void ProgramCard(void);
void Execute(void);

#endif
