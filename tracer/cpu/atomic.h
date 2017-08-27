/*
 *  Atomic operations.
 */

#ifndef __ATOMIC_H__
#define __ATOMIC_H__

/**********************************************************************/

typedef struct
{
    int val;
}
    Atomic_t;

/******************************/

static inline int
AtomicGet(Atomic_t * ap)                                    //  return ap
{
    return ap->val;
}


static inline int
AtomicTestAndSet(Atomic_t * ap, int v)                      //  t = ap
{                                                           //  ap = v
    int result, tmp;                                        //  return t
    asm volatile ("@ atomic set\n"
                    "1:      ldrex   %0, [%3]\n"
                    "        strex   %1, %4, [%3]\n"
                    "        teq     %1, #0\n"
                    "        bne     1b\n"
                   : "=&r" (result), "=&r" (tmp), "+oQ" (ap->val)
                   : "r" (&ap->val), "r" (v)
                   : "cc");
    return result;
}


static inline unsigned
AtomicSetFlags(Atomic_t * ap, unsigned mask)                //  t = ap
{                                                           //  ap |= mask
    int result, tmp;                                        //  return t
    asm volatile ("@ atomic set\n"
                    "1:      ldrex   %0, [%3]\n"
                    "        orr     %1, %0, %4\n"
                    "        strex   %1, %1, [%3]\n"
                    "        teq     %1, #0\n"
                    "        bne     1b\n"
                   : "=&r" (result), "=&r" (tmp), "+oQ" (ap->val)
                   : "r" (&ap->val), "Ir" (mask)
                   : "cc");
    return result;
}


static inline unsigned
AtomicTestAndClearFlags(Atomic_t * ap, unsigned mask)       //  t = ap
{                                                           //  ap &= ~mask
    int result, tmp;                                        //  t &= mask
    asm volatile ("@ atomic set\n"                          //  return t
                    "1:      ldrex   %0, [%3]\n"
                    "        bic     %1, %0, %4\n"
                    "        strex   %1, %1, [%3]\n"
                    "        teq     %1, #0\n"
                    "        bne     1b\n"
                    "        and     %0, %0, %4\n"
                   : "=&r" (result), "=&r" (tmp), "+oQ" (ap->val)
                   : "r" (&ap->val), "Ir" (mask)
                   : "cc");
    return result;
}


static inline int
AtomicAddAndReturn(Atomic_t * ap, int v)                    //  ap += v
{                                                           //  return ap
    int result, tmp;
    asm volatile ("@ atomic add return\n"
                    "1:      ldrex   %0, [%3]\n"
                    "        add     %0, %0, %4\n"
                    "        strex   %1, %0, [%3]\n"
                    "        teq     %1, #0\n"
                    "        bne     1b\n"
                   : "=&r" (result), "=&r" (tmp), "+oQ" (ap->val)
                   : "r" (&ap->val), "Ir" (v)
                   : "cc");
    return result;
}


static inline int
AtomicSubAndReturn(Atomic_t * ap, int v)                    //  ap -= v
{                                                           //  return ap
    int result, tmp;
    asm volatile ("@ atomic sub return\n"
                    "1:      ldrex   %0, [%3]\n"
                    "        sub     %0, %0, %4\n"
                    "        strex   %1, %0, [%3]\n"
                    "        teq     %1, #0\n"
                    "        bne     1b\n"
                   : "=&r" (result), "=&r" (tmp), "+oQ" (ap->val)
                   : "r" (&ap->val), "Ir" (v)
                   : "cc");
    return result;
}

/**********************************************************************/

#endif // __ATOMIC_H__

/**********************************************************************/
