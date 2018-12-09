/*******************************************************************************
 * Copyright (c) 2003 Aardvark Embedded Solutions.
 *
 *
 * File Name:
 *
 *     PCLocalAccess.cpp
 *
 * Description:
 *
 * The Access functions for the shared memory classes for USB devices and
 * Associated Macros etc.
 *
**********************************************************************************/
#ifndef PC_ACCESS
#define PC_ACCESS

// A Macro that when given an item in shared memory returns the index to SharedMemoryBase[] or Mirror[].
#define FIELD_TO_INDEX(Field) ((int*)&(Field) - SharedMemoryBase)

// A Macro to difem the word in Share memory base that is to be updated
//       This is necessary because normal assignments as done below update Mirro[]r as well,
//         as they are not intended to be shared with the remote Paylink
#define UPDATE_SHARED(Field) SharedMemoryBase[FIELD_TO_INDEX(Field)]



inline const InterfaceLong&   InterfaceLong::operator=(const InterfaceLong& OtherOne)
    {
    WriteInterfaceLong(OtherOne.ReadInterfaceLong()) ;
    return *this ;
    }

inline const InterfaceLong&  InterfaceLong::operator++()
    {
    AESLong Value = ReadInterfaceLong() + 1 ;
    WriteInterfaceLong(Value) ;
    return *this ;
    }

inline const InterfaceLong&  InterfaceLong::operator++(int)
    {
    AESLong Value = ReadInterfaceLong() ;
    WriteInterfaceLong(Value + 1) ;
    return *this ;
    }

inline const InterfaceLong&  InterfaceLong::operator+=(const AESLong Increment)
    {
    AESLong Value = ReadInterfaceLong() ;
    WriteInterfaceLong(Value + Increment) ;
    return *this ;
    }

inline const InterfaceLong&  InterfaceLong::operator--()
    {
    AESLong Value = ReadInterfaceLong() - 1 ;
    WriteInterfaceLong(Value) ;
    return *this ;
    }

inline const InterfaceLong&  InterfaceLong::operator--(int)
    {
    AESLong Value = ReadInterfaceLong() ;
    WriteInterfaceLong(Value - 1) ;
    return *this ;
    }

inline AESLong               InterfaceLong::ReadInterfaceLong(void) const
    {
    return *((AESLong*)((char *)SharedMemoryBase + (long)this)) ;
    }

inline bool                   InterfaceLong::CompareInterfaceLong(AESLong Value) const
    {
    return (*((AESLong*)((char *)SharedMemoryBase + (long)this)) == Value) ;
    }

inline AESLong*         InterfaceLong::operator&() const
    {
    return (AESLong*)((char *)SharedMemoryBase + (long)this) ;
    }

inline pInterfaceString::operator char*()     const
    {
    return (char*)((char *)SharedMemoryBase + ReadInterfaceLong()) ;
    }

inline void InterfaceLong::WriteInterfaceLong(AESLong Value)
    {
    AESLong *Address = (AESLong*)((char *)SharedMemoryBase + (long)this) ;
    *Address = Value ;
    if ((unsigned long)this < sizeof Mirror)
        {
        AESLong *Address2 = (AESLong*)((char *)Mirror + (long)this) ;
        *Address2 = Value ;
        }
    }





inline void InterfaceLong::CreateCopy(void)   // This is used where field that is not in the output area
                                             // is to be entered into the USB cache so it can be preset (meaningless on PC).
    {
    }


inline void InterfaceString::StringCopy(unsigned char* Data, int Length)
                                              // Copy character string data to the Shared Memory.
    {
    AESLong *Address = (AESLong*)((char *)SharedMemoryBase + (long)this) ;
    AESLong *Address2 = (AESLong*)((char *)Mirror + (long)this) ;
    int Words = (Length + 3) >> 2 ;

    for (int i = 0 ; i < Words ; ++i)
        {
        AESLong Value = ((AESLong)Data[0]      ) |
                        ((AESLong)Data[1] << 8 ) |
                        ((AESLong)Data[2] << 16) |
                        ((AESLong)Data[3] << 24) ;

        if ((AESLong)(long)this < (AESLong)sizeof Mirror)
            {
            *Address2 = Value ;
            Address2++ ;
            }
        *Address = Value ;
        Data    += 4 ;
        Address++ ;
        }
    }


#endif