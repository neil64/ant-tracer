/*
 *	Generate version files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <err.h>


int
main(int argc, char ** argv)
{
	char * vs = *++argv;
	if (!vs)
	{
		errx(1, "expected version number like 040216");
		return 1;
	}

	int vn = atoi(vs);
	int vers = (vn / 10000) % 100;
	int rel = (vn / 100) % 100;
	int rev = (vn / 1) % 100;

	printf("#ifndef __VERSION_H__\n");
	printf("#define __VERSION_H__\n");
	printf("\n");
	printf("/**********************************************************************/\n");
	printf("\n");
	printf("#define VERSION_VERSION		%d\n", vers);
	printf("#define VERSION_RELEASE		%d\n", rel);
	printf("#define VERSION_REVISION	%d\n", rev);
	printf("#define VERSION_STRING		\"%d.%d.%d\"\n",
							vers, rel, rev);
	printf("\n");
	printf("/**********************************************************************/\n");
	printf("\n");
	printf("#endif // __VERSION_H__\n");

	return 0;
}
