/*
 *  A simple strlen().
 */

#include "types.h"


size_t
strlen(const char * str)
{
    const char *s;
    for (s = str; *s; ++s) {}
    return(s - str);
}
