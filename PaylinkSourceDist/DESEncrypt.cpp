/* --------------------------------------------------------------------------

        File Name:              Encrypt.c

        Project:                Aardvark Game Interface

        Copyright:              © Aardvark Embedded Solutions Ltd. 2002

        Description:            Encryption Algorithm for Serial Compact
                                Hopper used on the Aardvark game interface.

        Author:                 Andy Graham,
                                Aardvark Embedded Solutions Ltd.
                                Tel:    +44 70107 12378
                                Email:  info@aardvark.eu.com

        Date            Eng     Details

        11/10-2002      AJG     Initial version for game interface.

   -------------------------------------------------------------------------- */
#include "Main.h"

/*************************************************************************************************

Portable Data Encryption Standard routines mainly based on Phil Karn, KA9Q library but trimmed down
with tables as constants in code and SpBox precalculated for speed and ram space.

Created by M Watson 2008 MoneyControls Ltd

Turned into class by Dave Bush - AES

**************************************************************************************************/

/* Key schedule-related tables from FIPS-46 */
/* permuted choice table (key) */
CODE_SPACE unsigned char pc1[] = {
    57, 49, 41, 33, 25, 17,  9,
     1, 58, 50, 42, 34, 26, 18,
    10,  2, 59, 51, 43, 35, 27,
    19, 11,  3, 60, 52, 44, 36,

    63, 55, 47, 39, 31, 23, 15,
     7, 62, 54, 46, 38, 30, 22,
    14,  6, 61, 53, 45, 37, 29,
    21, 13,  5, 28, 20, 12,  4
};
/* number left rotations of pc1 */
CODE_SPACE unsigned char totrot[] = {
    1,2,4,6,8,10,12,14,15,17,19,21,23,25,27,28
};
/* permuted choice key (table) */
CODE_SPACE unsigned char pc2[] = {
    14, 17, 11, 24,  1,  5,
     3, 28, 15,  6, 21, 10,
    23, 19, 12,  4, 26,  8,
    16,  7, 27, 20, 13,  2,
    41, 52, 31, 37, 47, 55,
    30, 40, 51, 45, 33, 48,
    44, 49, 39, 56, 34, 53,
    46, 42, 50, 36, 29, 32
};
/* End of DES-defined tables */


/* bit 0 is left-most in byte */
CODE_SPACE int bytebit[] = {
    0200,0100,040,020,010,04,02,01
};

// This combined sp constant table was constructed using version two of this library, functions have now been deleted as unecesary,
// but they can be found in my origional VC6.0 port of Phil Karn's unix based library.

CODE_SPACE AESULong Spbox[8][64]={
{0x01010400 ,0x00000000 ,0x00010000 ,0x01010404 ,0x01010004 ,0x00010404 ,0x00000004 ,0x00010000
,0x00000400 ,0x01010400 ,0x01010404 ,0x00000400 ,0x01000404 ,0x01010004 ,0x01000000 ,0x00000004
,0x00000404 ,0x01000400 ,0x01000400 ,0x00010400 ,0x00010400 ,0x01010000 ,0x01010000 ,0x01000404
,0x00010004 ,0x01000004 ,0x01000004 ,0x00010004 ,0x00000000 ,0x00000404 ,0x00010404 ,0x01000000
,0x00010000 ,0x01010404 ,0x00000004 ,0x01010000 ,0x01010400 ,0x01000000 ,0x01000000 ,0x00000400
,0x01010004 ,0x00010000 ,0x00010400 ,0x01000004 ,0x00000400 ,0x00000004 ,0x01000404 ,0x00010404
,0x01010404 ,0x00010004 ,0x01010000 ,0x01000404 ,0x01000004 ,0x00000404 ,0x00010404 ,0x01010400
,0x00000404 ,0x01000400 ,0x01000400 ,0x00000000 ,0x00010004 ,0x00010400 ,0x00000000 ,0x01010004 },
{0x80108020 ,0x80008000 ,0x00008000 ,0x00108020 ,0x00100000 ,0x00000020 ,0x80100020 ,0x80008020
,0x80000020 ,0x80108020 ,0x80108000 ,0x80000000 ,0x80008000 ,0x00100000 ,0x00000020 ,0x80100020
,0x00108000 ,0x00100020 ,0x80008020 ,0x00000000 ,0x80000000 ,0x00008000 ,0x00108020 ,0x80100000
,0x00100020 ,0x80000020 ,0x00000000 ,0x00108000 ,0x00008020 ,0x80108000 ,0x80100000 ,0x00008020
,0x00000000 ,0x00108020 ,0x80100020 ,0x00100000 ,0x80008020 ,0x80100000 ,0x80108000 ,0x00008000
,0x80100000 ,0x80008000 ,0x00000020 ,0x80108020 ,0x00108020 ,0x00000020 ,0x00008000 ,0x80000000
,0x00008020 ,0x80108000 ,0x00100000 ,0x80000020 ,0x00100020 ,0x80008020 ,0x80000020 ,0x00100020
,0x00108000 ,0x00000000 ,0x80008000 ,0x00008020 ,0x80000000 ,0x80100020 ,0x80108020 ,0x00108000 },
{0x00000208 ,0x08020200 ,0x00000000 ,0x08020008 ,0x08000200 ,0x00000000 ,0x00020208 ,0x08000200
,0x00020008 ,0x08000008 ,0x08000008 ,0x00020000 ,0x08020208 ,0x00020008 ,0x08020000 ,0x00000208
,0x08000000 ,0x00000008 ,0x08020200 ,0x00000200 ,0x00020200 ,0x08020000 ,0x08020008 ,0x00020208
,0x08000208 ,0x00020200 ,0x00020000 ,0x08000208 ,0x00000008 ,0x08020208 ,0x00000200 ,0x08000000
,0x08020200 ,0x08000000 ,0x00020008 ,0x00000208 ,0x00020000 ,0x08020200 ,0x08000200 ,0x00000000
,0x00000200 ,0x00020008 ,0x08020208 ,0x08000200 ,0x08000008 ,0x00000200 ,0x00000000 ,0x08020008
,0x08000208 ,0x00020000 ,0x08000000 ,0x08020208 ,0x00000008 ,0x00020208 ,0x00020200 ,0x08000008
,0x08020000 ,0x08000208 ,0x00000208 ,0x08020000 ,0x00020208 ,0x00000008 ,0x08020008 ,0x00020200 },
{0x00802001 ,0x00002081 ,0x00002081 ,0x00000080 ,0x00802080 ,0x00800081 ,0x00800001 ,0x00002001
,0x00000000 ,0x00802000 ,0x00802000 ,0x00802081 ,0x00000081 ,0x00000000 ,0x00800080 ,0x00800001
,0x00000001 ,0x00002000 ,0x00800000 ,0x00802001 ,0x00000080 ,0x00800000 ,0x00002001 ,0x00002080
,0x00800081 ,0x00000001 ,0x00002080 ,0x00800080 ,0x00002000 ,0x00802080 ,0x00802081 ,0x00000081
,0x00800080 ,0x00800001 ,0x00802000 ,0x00802081 ,0x00000081 ,0x00000000 ,0x00000000 ,0x00802000
,0x00002080 ,0x00800080 ,0x00800081 ,0x00000001 ,0x00802001 ,0x00002081 ,0x00002081 ,0x00000080
,0x00802081 ,0x00000081 ,0x00000001 ,0x00002000 ,0x00800001 ,0x00002001 ,0x00802080 ,0x00800081
,0x00002001 ,0x00002080 ,0x00800000 ,0x00802001 ,0x00000080 ,0x00800000 ,0x00002000 ,0x00802080 },
{0x00000100 ,0x02080100 ,0x02080000 ,0x42000100 ,0x00080000 ,0x00000100 ,0x40000000 ,0x02080000
,0x40080100 ,0x00080000 ,0x02000100 ,0x40080100 ,0x42000100 ,0x42080000 ,0x00080100 ,0x40000000
,0x02000000 ,0x40080000 ,0x40080000 ,0x00000000 ,0x40000100 ,0x42080100 ,0x42080100 ,0x02000100
,0x42080000 ,0x40000100 ,0x00000000 ,0x42000000 ,0x02080100 ,0x02000000 ,0x42000000 ,0x00080100
,0x00080000 ,0x42000100 ,0x00000100 ,0x02000000 ,0x40000000 ,0x02080000 ,0x42000100 ,0x40080100
,0x02000100 ,0x40000000 ,0x42080000 ,0x02080100 ,0x40080100 ,0x00000100 ,0x02000000 ,0x42080000
,0x42080100 ,0x00080100 ,0x42000000 ,0x42080100 ,0x02080000 ,0x00000000 ,0x40080000 ,0x42000000
,0x00080100 ,0x02000100 ,0x40000100 ,0x00080000 ,0x00000000 ,0x40080000 ,0x02080100 ,0x40000100 },
{0x20000010 ,0x20400000 ,0x00004000 ,0x20404010 ,0x20400000 ,0x00000010 ,0x20404010 ,0x00400000
,0x20004000 ,0x00404010 ,0x00400000 ,0x20000010 ,0x00400010 ,0x20004000 ,0x20000000 ,0x00004010
,0x00000000 ,0x00400010 ,0x20004010 ,0x00004000 ,0x00404000 ,0x20004010 ,0x00000010 ,0x20400010
,0x20400010 ,0x00000000 ,0x00404010 ,0x20404000 ,0x00004010 ,0x00404000 ,0x20404000 ,0x20000000
,0x20004000 ,0x00000010 ,0x20400010 ,0x00404000 ,0x20404010 ,0x00400000 ,0x00004010 ,0x20000010
,0x00400000 ,0x20004000 ,0x20000000 ,0x00004010 ,0x20000010 ,0x20404010 ,0x00404000 ,0x20400000
,0x00404010 ,0x20404000 ,0x00000000 ,0x20400010 ,0x00000010 ,0x00004000 ,0x20400000 ,0x00404010
,0x00004000 ,0x00400010 ,0x20004010 ,0x00000000 ,0x20404000 ,0x20000000 ,0x00400010 ,0x20004010 },
{0x00200000 ,0x04200002 ,0x04000802 ,0x00000000 ,0x00000800 ,0x04000802 ,0x00200802 ,0x04200800
,0x04200802 ,0x00200000 ,0x00000000 ,0x04000002 ,0x00000002 ,0x04000000 ,0x04200002 ,0x00000802
,0x04000800 ,0x00200802 ,0x00200002 ,0x04000800 ,0x04000002 ,0x04200000 ,0x04200800 ,0x00200002
,0x04200000 ,0x00000800 ,0x00000802 ,0x04200802 ,0x00200800 ,0x00000002 ,0x04000000 ,0x00200800
,0x04000000 ,0x00200800 ,0x00200000 ,0x04000802 ,0x04000802 ,0x04200002 ,0x04200002 ,0x00000002
,0x00200002 ,0x04000000 ,0x04000800 ,0x00200000 ,0x04200800 ,0x00000802 ,0x00200802 ,0x04200800
,0x00000802 ,0x04000002 ,0x04200802 ,0x04200000 ,0x00200800 ,0x00000000 ,0x00000002 ,0x04200802
,0x00000000 ,0x00200802 ,0x04200000 ,0x00000800 ,0x04000002 ,0x04000800 ,0x00000800 ,0x00200002 },
{0x10001040 ,0x00001000 ,0x00040000 ,0x10041040 ,0x10000000 ,0x10001040 ,0x00000040 ,0x10000000
,0x00040040 ,0x10040000 ,0x10041040 ,0x00041000 ,0x10041000 ,0x00041040 ,0x00001000 ,0x00000040
,0x10040000 ,0x10000040 ,0x10001000 ,0x00001040 ,0x00041000 ,0x00040040 ,0x10040040 ,0x10041000
,0x00001040 ,0x00000000 ,0x00000000 ,0x10040040 ,0x10000040 ,0x10001000 ,0x00041040 ,0x00040000
,0x00041040 ,0x00040000 ,0x10041000 ,0x00001000 ,0x00000040 ,0x10040040 ,0x00001000 ,0x00041040
,0x10001000 ,0x00000040 ,0x10000040 ,0x10040000 ,0x10040040 ,0x10000000 ,0x00040000 ,0x10001040
,0x00000000 ,0x10041040 ,0x00040040 ,0x10000040 ,0x10040000 ,0x10001000 ,0x10001040 ,0x00000000
,0x10041040 ,0x00041000 ,0x00041000 ,0x00001040 ,0x00001040 ,0x00040040 ,0x10000000 ,0x10041000 },
};

/* Combined SP lookup table, linked in
 * For best results, ensure that this is aligned on a 32-bit boundary;
  */

#define F(l,r,key){\
    work = ((r >> 4) | (r << 28)) ^ key[0] ;\
    l ^= CODE_SPACE_READ_LONG(Spbox[6][ work        & (AESULong)0x3f]) ;\
    l ^= CODE_SPACE_READ_LONG(Spbox[4][(work >> 8)  & (AESULong)0x3f]) ;\
    l ^= CODE_SPACE_READ_LONG(Spbox[2][(work >> 16) & (AESULong)0x3f]) ;\
    l ^= CODE_SPACE_READ_LONG(Spbox[0][(work >> 24) & (AESULong)0x3f]) ;\
    work = r ^ key[1] ;\
    l ^= CODE_SPACE_READ_LONG(Spbox[7][ work        & (AESULong)0x3f]) ;\
    l ^= CODE_SPACE_READ_LONG(Spbox[5][(work >> 8)  & (AESULong)0x3f]) ;\
    l ^= CODE_SPACE_READ_LONG(Spbox[3][(work >> 16) & (AESULong)0x3f]) ;\
    l ^= CODE_SPACE_READ_LONG(Spbox[1][(work >> 24) & (AESULong)0x3f]) ;\
}


//----------------------------Des port-------------------------------
/* Encrypt or decrypt a block of data in ECB mode */
void DESEncrypt::des(unsigned char Data[8])
{
    AESULong left  = 0 ;
    AESULong right = 0 ;
    AESULong work  = 0 ;

    /* Read input block and place in left/right in big-endian order */
    left =  (((AESULong)Data[0]) << 24)
         |  (((AESULong)Data[1]) << 16)
         |  (((AESULong)Data[2]) << 8)
         |  (((AESULong)Data[3]));
    right = (((AESULong)Data[4]) << 24)
         |  (((AESULong)Data[5]) << 16)
         |  (((AESULong)Data[6]) << 8)
         |  (((AESULong)Data[7]));

    /* Hoey's clever initial permutation algorithm, from Outerbridge
     * (see Schneier p 478)
     *
     * The convention here is the same as Outerbridge: rotate each
     * register left by 1 bit, i.e., so that "left" contains permuted
     * input bits 2, 3, 4, ... 1 and "right" contains 33, 34, 35, ... 32
     * (using origin-1 numbering as in the FIPS). This allows us to avoid
     * one of the two rotates that would otherwise be required in each of
     * the 16 rounds.
     */
    work = ((left >> 4) ^ right) & (AESULong)0x0f0f0f0f;
    right ^= work;
    left ^= work << 4;
    work = ((left >> 16) ^ right) & (AESULong)0xffff;
    right ^= work;
    left ^= work << 16;
    work = ((right >> 2) ^ left) & (AESULong)0x33333333;
    left ^= work;
    right ^= (work << 2);
    work = ((right >> 8) ^ left) & (AESULong)0xff00ff;
    left ^= work;
    right ^= (work << 8);
    right = (right << 1) | (right >> 31);
    work = (left ^ right) & (AESULong)0xaaaaaaaa;
    left ^= work;
    right ^= work;
    left = (left << 1) | (left >> 31);

    /* Now do the 16 rounds */
    F(left,right,ks[0]);
    F(right,left,ks[1]);
    F(left,right,ks[2]);
    F(right,left,ks[3]);
    F(left,right,ks[4]);
    F(right,left,ks[5]);
    F(left,right,ks[6]);
    F(right,left,ks[7]);
    F(left,right,ks[8]);
    F(right,left,ks[9]);
    F(left,right,ks[10]);
    F(right,left,ks[11]);
    F(left,right,ks[12]);
    F(right,left,ks[13]);
    F(left,right,ks[14]);
    F(right,left,ks[15]);

    /* Inverse permutation, also from Hoey via Outerbridge and Schneier */
    right = (right << 31) | (right >> 1);
    work = (left ^ right) & (AESULong)0xaaaaaaaa;
    left ^= work;
    right ^= work;
    left = (left >> 1) | (left  << 31);
    work = ((left >> 8) ^ right) & (AESULong)0xff00ff;
    right ^= work;
    left ^= work << 8;
    work = ((left >> 2) ^ right) & (AESULong)0x33333333;
    right ^= work;
    left ^= work << 2;
    work = ((right >> 16) ^ left) & (AESULong)0xffff;
    left ^= work;
    right ^= work << 16;
    work = ((right >> 4) ^ left) & (AESULong)0x0f0f0f0f;
    left ^= work;
    right ^= work << 4;

    /* Put the Data[ back into the user's buffer with final swap */
    Data[0] = (unsigned char) (right >> 24);
    Data[1] = (unsigned char) (right >> 16);
    Data[2] = (unsigned char) (right >> 8);
    Data[3] = (unsigned char) (right);
    Data[4] = (unsigned char) (left >> 24);
    Data[5] = (unsigned char) (left >> 16);
    Data[6] = (unsigned char) (left >> 8);
    Data[7] = (unsigned char) (left);
}







/* Generate key schedule for encryption or decryption
 * depending on the value of "decrypt"
 */
void DESEncrypt::deskey(int decrypt)    /* 0 = encrypt, 1 = decrypt */
    {
    register int l=0;
    int m = 0;
    unsigned char ksT[8];

    unsigned char pc1m[56];     /* place to modify pc1 into */
    unsigned char pcr[56];      /* place to rotate pc1 into */

    for (int j = 0; j < 56; j++)
        {      /* convert pc1 to bits of key */
        l = CODE_SPACE_READ_CHAR(pc1[j]) - 1 ;     /* integer bit location  */
        m = l & 07;     /* find bit      */
        pc1m[j] = (TheKey[l>>3] &                               /* find which key byte l is in */
                   CODE_SPACE_READ_SHORT(bytebit[m])) /* and which bit of that byte */
                   ? 1 : 0;    /* and store 1-bit result */
        }

    for (int i = 0; i < 16; i++)
        {                              /* key chunk for each iteration */
        memset(ksT, 0, sizeof(ksT));  /* Clear key schedule */
        for (int j = 0; j < 56; j++)    /* rotate pc1 the right amount */
            {
            l = j + CODE_SPACE_READ_CHAR(totrot[decrypt ? 15 - i : i]) ;
            pcr[j] = pc1m[l < (j < 28 ? 28 : 56) ? l : l - 28];
            }

            /* rotate left and right halves independently */
        for (int k = 0; k < 48; k++)
            {                            /* select bits individually */
            /* check bit that goes to ks[k] */
            if (pcr[CODE_SPACE_READ_CHAR(pc2[k]) - 1]){
                /* mask it in if it's there */
                ksT[k / 6] |= CODE_SPACE_READ_SHORT(bytebit[k % 6]) >> 2;
            }
        }
        /* Now convert to packed odd/even interleaved form */
        ks[i][0] = ((AESLong)ksT[0] << 24)
                 | ((AESLong)ksT[2] << 16)
                 | ((AESLong)ksT[4] << 8)
                 | ((AESLong)ksT[6]);
        ks[i][1] = ((AESLong)ksT[1] << 24)
                 | ((AESLong)ksT[3] << 16)
                 | ((AESLong)ksT[5] << 8)
                 | ((AESLong)ksT[7]);
        }
    }




/* --------------------------------------------------------------------------

        Function:       DESEncrypt

        Description:    This function is the entry point of the encryption
                        code

        Parameters:     Key - a 64 bit DES Key

                        Data - a buffer to be encrypted

                        Length - the length of the data

        Returns:        None

   -------------------------------------------------------------------------- */
void DESEncrypt::Encrypt(unsigned char Key[8], unsigned char Data[8])
    {
    memcpy(TheKey, Key, sizeof TheKey) ;
    deskey(0);
    des(Data);
    }




/* --------------------------------------------------------------------------

        Function:       DESDecrypt

        Description:    This function is the entry point of the decryption
                        code

        Parameters:     Key - a 64 bit DES Key

                        Data - a buffer to be encrypted

                        Length - the length of the data

        Returns:        None

   -------------------------------------------------------------------------- */
void DESEncrypt::Decrypt(unsigned char Key[8], unsigned char Data[8])
    {
    memcpy(TheKey, Key, sizeof TheKey) ;
    deskey(1);
    des(Data);
    }
