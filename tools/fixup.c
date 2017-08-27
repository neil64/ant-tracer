/*
 *	Fix up the final details of the main image.
 *
 *	It pads the image to a multiple of 64 bytes, stores the final size
 *	in the image header, then calculates the image checksum.
 *
 *	Usage:	fixup <image-file>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <err.h>

typedef int		s32;
typedef unsigned	u32;

/********************/

s32
checksum(char * b0, int sz)
{
	s32 * s = (s32 *)b0;
	s32 * e = (s32 *)(b0 + sz);
	s32 c = 0;

	while (s < e)
		c += *s++;

	return c;
}

/********************/

int
main(int argc, char ** argv)
{
	int fd;
	struct stat statb;
	s32 size;
	s32 x;
	char * buf;

	if (argc != 2)
	{
		fprintf(stderr, "usage: fixup <image-file>\n");
		exit(1);
	}

	fd = open(argv[1], O_RDWR);
	if (fd == -1)
		err(1, "can't open %s", argv[1]);

	if (fstat(fd, &statb) < 0)
		err(1, "can't fstat %s", argv[1]);

	size = statb.st_size + (64 - 1);
	size &= ~(64 - 1);
	buf = malloc(size);

	x = read(fd, buf, statb.st_size);
	if (x < 0)
		err(1, "read error on %s", argv[1]);
	if (x < statb.st_size)
		errx(1, "short read -- read %d of %d bytes", x, statb.st_size);

	while (x < size)
		buf[x++] = 0xff;

	/*
	 *	Figure on old or new head type.  Look for a branch insn at
	 *	offset 0, and if found, use the later header.  (Not at the
	 *	moment.)
	 */
	int off = 1;
	u32 magic = ((u32 *)buf)[8];
	if (magic != 0x326b7550) // Puk2
		errx(1, "invalid magic number (%08x)", magic);

	/*
	 *  Set the checksum field to 0, calculate the checksum and store the
	 *  negative of the value here.  The checksum for the entire image should
	 *  be 0.
	 */
	((s32 *)buf)[9] = size;
	((s32 *)buf)[10] = 0;

	((s32 *)buf)[10] = -checksum(buf, size);


#if 0
	printf("checksum is 0x%08x,  double check of checksum gives 0x%08x\n",
			((s32 *)buf)[9], checksum(buf, size));
#endif // 0

	if (lseek(fd, 0, SEEK_SET) < 0)
		err(1, "lseek failed on %s", argv[1]);

	x = write(fd, buf, size);
	if (x < 0)
		err(1, "write error on %s", argv[1]);

	if (x < size)
		errx(1, "short write -- wrote %d of %d bytes", x, size);

	if (close(fd) < 0)
		err(1, "close error");

	return 0;
}
