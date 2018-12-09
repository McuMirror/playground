
extern unsigned char ProgramSpace[0x80000] ;            // 512K of program space
extern unsigned int LowestFlashAddress;                  // the lowest address written to in flash
extern unsigned int LowestProgramSpaceOffset;            // the lowest address written to in flash

#ifdef __cplusplus
extern "C" {
#endif

char* ReadSRec(char* FileName) ;

void  ClearLoadSpace(void);

#ifdef __cplusplus
}
#endif
