/*
 *	An abbreviated memcpy.
 */

#include "types.h"
#include "stdlib.h"


void *
memcpy(void * dest, const void * src, size_t cnt)
{
	void * ret = dest;

	if (((int)dest & 3) == 0 &&
	    ((int)src & 3) == 0)
	{
		u32 * d = dest;
		const u32 * s = src;
		while (cnt >= 4)
		{
			*d++ = *s++;
			cnt -= 4;
		}
		dest = d;
		src = s;
	}

	u8 * d = dest;
	const u8 * s = src;
	while (cnt-- > 0)
		*d++ = *s++;

	return ret;
}
