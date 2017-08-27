/*
 *	A simple strcpy().
 */

#include "stdlib.h"


char *
strcpy(char * d, const char * s)
{
    char * d1 = d;
    while ((*d++ = *s++))
        ;
    return d1;
}


char *
stpcpy(char * d, const char * s)
{
    while ((*d++ = *s++))
        ;
    return d - 1;
    // while ((*d = *s))
        // d++, s++;
    // return d;
}
