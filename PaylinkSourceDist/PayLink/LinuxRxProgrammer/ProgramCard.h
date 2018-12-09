/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
#ifndef ProgramCardH
#define ProgramCardH

extern USBAccess*  ThePort ;
extern USBMilan*   Milan ;

extern unsigned int CodeApplication;
extern unsigned int CodeChecksum;
extern          int CodeVersion;
extern          int ThePlatform ;
extern unsigned int CardSelfTest;
extern          int KernelCodeVersion ;
extern         bool ForceKernelUpdate;
extern bool    AutoProgram;

extern bool  ForceKernelUpdate;
extern bool  AutoProgram;
extern bool ShowTraffic ;

/*--------------------------------------------------------------------------*\
 *
\*--------------------------------------------------------------------------*/
void ProgramCard(void);
void Execute(void);

#endif
