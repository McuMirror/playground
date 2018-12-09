/*******************************************************************************
 *
 *
 *  Queue     - A generalised template queuing system for real time
 *              If necessary - Interrupts are expected to be disabled externally
 *              but this should not normally be necessary
 *
 *              The buffer size must be a power of 2, so the initialiser takes
 *              a power of 2, not a size
 *
 *
 *******************************************************************************/
#ifndef QUEUE_H
#define QUEUE_H

enum {
    Q_NONE = -1,
    Q_OK   = 0,
    Q_FIRST  = 1
    } ;

// The buffer is indexed by (PutPtr & Mask) and by (GetPTr & Mask)
// The higher order bits of these are not cleared, but increment each time
// The buffer wraps. This is deliberate as these high order bits are used
// to distinguish Empty and Full !

// At all times (PutPtr - GetPtr) bytes of the buffer are in use

// Readers only update GetPtr, Writers only update PutPtr. So long as these
// don't interrupt themselves then no interlock is needed against simultaneous
// update.

template <class ItemType> class RTQueue
    {
  private:
    ItemType*  Buffer ;             // The buffer itself
    int        Mask ;               // Size -1 (all ones)
    int        PutPtr ;             // Index used by Put function
    int        GetPtr ;             // Index used by Get function

  public:
    inline RTQueue(char Power)
        {
        int Size = 1 << Power ;

        Buffer = new ItemType[Size] ;
        Mask   = Size - 1 ;
        PutPtr = 0 ;
        GetPtr = 0 ;
        }

    inline int QInUse(void)
        {
        return (PutPtr - GetPtr) ;
        }


    inline int QSpace(void)
        {
        return Mask + 1 - QInUse() ;
        }


    inline ItemType QCheck(void)
        {
        if (PutPtr == GetPtr)
            {
            return (ItemType)Q_NONE ;
            }
        else
            {
            return Buffer[GetPtr & Mask] ;
            }
        }


    inline int QStatus(void)
        {
        if (PutPtr == GetPtr)
            {
            return Q_NONE ;
            }
        else
            {
            return Q_OK ;
            }
        }


    inline ItemType QGet(void)
        {
        if (PutPtr == GetPtr)
            {
            printf("Q Error\n") ;
            return Buffer[Mask & GetPtr] ;
            }
        return Buffer[Mask & GetPtr++] ;
        }


    inline int QPut(ItemType Item)
        {
        if (QInUse() > Mask)
            {
            // Buffer overflow
            return Q_NONE ;
            }
        Buffer[Mask & PutPtr] = Item ;
        return (PutPtr++ == GetPtr) ? Q_FIRST: Q_OK ;
        }
    } ;


#endif
