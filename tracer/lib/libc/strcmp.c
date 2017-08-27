/*
 *	A simple strcmp().
 */

#include	"types.h"


int
strcmp(const char * l, const char * r)
{
	char x;

	for (;;)
		if ((x = *l - *r++) != 0 ||
		    !*l++)
			break;
	return x;
}
