/*******************************************************************************
 * Copyright (c) 2003 Aardvark Embedded Solutions.
 *
 *
 * File Name:
 *
 *     PCAccess.cpp
 *
 * Description:
 *
 * The Access functions for the shared memory classes
 *
**********************************************************************************/

extern char check_sizeof_char_is_one[2 - sizeof (char)] ;                   // Error if char is not a single byte

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
    volatile AESLong* InMemory = (AESLong*)((char *)SharedMemoryBase + (long)this) ;
    return *InMemory ;
    }

inline bool                   InterfaceLong::CompareInterfaceLong(AESLong Value) const
    {
    volatile AESLong* InMemory = (AESLong*)((char *)SharedMemoryBase + (long)this) ;
    return (*InMemory == Value) ;
    }

inline AESLong*         InterfaceLong::operator&() const
    {
    return (AESLong*)((char *)SharedMemoryBase + (long)this) ;
    }

inline     pInterfaceString::operator char*()     const
    {
    return (char*)((char *)SharedMemoryBase + ReadInterfaceLong()) ;
    }
/****************************************************************************
The RAM will fail to update if the H8 is reading the same location
 this function retries if such a clash happens
*****************************************************************************/
void InterfaceLong::WriteInterfaceLong(AESLong Value)
    {
    AESLong Retries = 0 ;
    volatile AESLong *Address = (AESLong*)((char *)SharedMemoryBase + (long)this) ;

    do
        {
        *Address = Value ;
        } while (*Address != Value &&
                 ++Retries < 1000000) ;

    if (*Address != Value)
        {
        OurInterfaceErrror = INTERFACE_FAILED ;
        Retries = 0 ;
        do
            {
            OutputArea->PeripheralsEnabled = 0 ;
            } while (OutputArea->PeripheralsEnabled != 0 &&
                     ++Retries < 1000000) ;

        }
    }


