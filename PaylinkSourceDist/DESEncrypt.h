/* --------------------------------------------------------------------------

        File Name:              Encrypt.h

        Project:                Aardvark Game Interface

        Copyright:              © Aardvark Embedded Solutions Ltd. 2002

        Description:            Defines the entry point of the encryption
                                algorithm for Serial Compact Hopper used on
                                the Aardvark game interface.

        Author:                 Andy Graham,
                                Aardvark Embedded Solutions Ltd.
                                Tel:    +44 70107 12378
                                Email:  info@aardvark.eu.com

        Date            Eng     Details

        11/10-2002      AJG     Initial version for game interface.

   -------------------------------------------------------------------------- */

#ifndef DESENCRYPTION
#define DESENCRYPTION

#define DES_PARITY 0xFE                       // DES keys don't use the bottom bit
#define DES_PARITY_LONG 0xFEFEFEFE

typedef AESULong DES_KS[16][2];    /* Single-key DES key schedule */

class DESEncrypt {
  public:
    DES_KS ks;                          // 128 bytes!
    unsigned char TheKey[8];

    void des(unsigned char Data[8]) ;
    void deskey(int decrypt) ;   /* 0 = encrypt, 1 = decrypt */


    void Encrypt(unsigned char Key[8], unsigned char Data[8]) ;
    void Decrypt(unsigned char Key[8], unsigned char Data[8]) ;
    } ;


#endif
