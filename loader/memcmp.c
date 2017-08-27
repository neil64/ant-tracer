/*
 *  A simple memcmp().
 */

#include "bl.h"


int
memcmp(const void * l, const void * r, size_t len)
{
    unsigned i = 0;
    for (i = 0; i < len; l++, r++)
    {
        if (*(char *)l < *(char *)r)
            return -1;
        else if (*(char *)l > *(char *)r)
            return 1;
    }
    return 0;
}
