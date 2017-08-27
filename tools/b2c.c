/*
 *	Binary to C converter.
 *
 *	Take arbitrary binary and generate a C array from it.
 */


#include	<stdio.h>
#include	<stdlib.h>
#include	<err.h>

/*
 *  Enums to define byte or word mode.
 */
enum
{
	BYTE_MODE,
	WORD_MODE
}
	mode = BYTE_MODE;

int
main(int argc, char ** argv)
{
	char *	name;
	FILE *	ifd;
	FILE *	ofd;
	int	c;
	int	i;
	int	cnt;
	char *	buf;

    for (i = 1; i < argc && argv[i][0] == '-'; i++)
    {
    	char opt = argv[i][1];
        switch (opt)
        {
        case 'W':
        case 'w':
        	mode = WORD_MODE;
        	break;

        default:
            err(1, "invalid option -%c\n", opt);
        }
    }

    char ** rest = &argv[i - 1];
    argc -= (i - 1);

	if (argc < 2)
	{
		fprintf(stderr, "usage: %s [-w] <array_name> [input [output]]\n",
								argv[0]);
		exit(1);
	}
	name = rest[1];

	if (argc >= 3)
	{
		ifd = fopen(rest[2], "r");
		if (!ifd)
			err(1, "can't open input: %s", rest[2]);
	}
	else
		ifd = stdin;

	if (argc >= 4)
	{
		ofd = fopen(rest[3], "w");
		if (!ofd)
			err(1, "can't create output: %s", rest[3]);
	}
	else
		ofd = stdout;

	// -----------

#define BSZ	(1024)
	buf = malloc(BSZ);
	cnt = 0;

	for (;;)
	{
		int x = fread(&buf[cnt], 1, BSZ, ifd);
		cnt += x;
		if (x == BSZ)
			buf = realloc(buf, cnt + BSZ);
		else
			break;
	}

	// -----------

	unsigned div 	= 1;
	if (mode == WORD_MODE)
		div 	= 4;

	const char * type = (mode == WORD_MODE) ? "" : "char";
	fprintf(ofd, "const unsigned %s_size = %d;\n", name, (cnt+div-1)/div);
	fprintf(ofd, "const unsigned %s %s[] =\n{", type, name);

	for (i = 0; i < cnt / div; i++)
	{
		switch (mode)
		{
		case BYTE_MODE:
			{
				c = buf[i];

				if ((i % 8) == 0)
					fprintf(ofd, "\n\t");
				if ((i % 8) == 4)
					fprintf(ofd, " ");

				fprintf(ofd, " 0x%02x,", c & 0xff);
			}
			break;
		case WORD_MODE:
			{
				unsigned * bp = (unsigned *)buf;
				unsigned x = bp[i];

				if ((i % 4) == 0)
					fprintf(ofd, "\n\t");
				if ((i % 4) == 2)
					fprintf(ofd, " ");

				fprintf(ofd, " 0x%08x,", x);
			}
			break;
		}
	}

	fprintf(ofd, "\n};\n");

	fclose(ofd);
	fclose(ifd);

	exit(0);
}
