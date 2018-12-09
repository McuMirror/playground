
extern unsigned char ProgramSpace[0x40000] ;            // 256K of program space
extern unsigned int  MaxAddress ;

#define AARDVARK_FLAG 0xaa8d4a8c
#define GETLONG(x) (((((((unsigned int)(x)[0] << 8) + (x)[1]) << 8) + (x)[2]) << 8) + (x)[3])
#define GETWORD(x) (((short)(x)[0] << 8) + (x)[1])

#ifdef __cplusplus
extern "C" {
#endif

char* ReadSRec(char* FileName) ;
void  ClearLoadSpace(void);
char* CheckCheckSum(void) ;

#ifdef __cplusplus
}
#endif
