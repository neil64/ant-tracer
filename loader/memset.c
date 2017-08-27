/*
 *	An abbreviated memset.
 */

#include "bl.h"


void *
memset(void * dest, int c, size_t sz)
{
	void * ret = dest;
	c &= 0xff;
	u32 v = c | (c << 8);
	v |= (v << 16);

	if (((int)dest & 3) == 0)
	{
		u32 * d = dest;
		while (sz >= 4)
		{
			*d++ = v;
			sz -= 4;
		}
		dest = d;
	}

	u8 * d = dest;
	while (sz-- > 0)
		*d++ = v;

	return ret;
}
