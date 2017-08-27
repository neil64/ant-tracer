/*
 *  Intel hex generator.
 *
 *  Given an address and a value (32 bit), generate the Intel hex needed to
 *  append this to a nRF52 bound image.
 *
 *  Reference: https://en.wikipedia.org/wiki/Intel_HEX
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <err.h>

/**********************************************************************/

typedef unsigned char       uchar;
typedef unsigned short      ushort;

typedef uchar           u8;
typedef ushort          u16;
typedef unsigned int    u32;

/**********************************************************************/

typedef enum
{
    RT_Data = 0,                    //  Length encoded
    RT_EOF,                         //  01
    RT_ExtendedSegmentAddress,      //  02
    RT_StartSegmentAddress,         //  03
    RT_ExtendedLinearAddress,       //  04
    RT_StartLinearAddress,          //  05
}
    record_e;

/**********************************************************************/

/*
 *  Emit a single line of hex to the `ofd'.
 */
static void
emitHexLine(FILE * ofd, record_e type, u16 address, u8 * data, u8 length)
{
    /*
     *  Calculate the checksum for the entry's header.
     */
    u8 sum = 0;
    sum += length;
    sum += address >> 8;
    sum += address & 0xff;
    sum += type;

    /*
     *  Write the data up-to the record type.
     */
    fprintf(ofd, ":%02X%04X%02X", length, address, type);

    int i;
    for (i = 0; i < length; i++)
    {
        fprintf(ofd, "%02X", data[i]);
        sum += data[i];
    }
    sum = ~sum + 1;

    /*
     *  Write the checksum.
     */
    fprintf(ofd, "%02X\n", sum);
}


static u8 *
fill16(u8 * out, u16 x)
{
    out[0] = x >> 8;
    out[1] = x & 0xff;
    return out;
}


static u8 *
fill32(u8 * out, unsigned x)
{
    out[0] = x >> 24;
    out[1] = (x >> 16) & 0xff;
    out[2] = (x >> 8) & 0xff;
    out[3] = x & 0xff;
    return out;
}

/**********************************************************************/

/*
 *  Usage:  hexen <address> <value> [-o <file>]
 */
int
main(int argc, char ** argv)
{
    char * name = 0;
    u8 out[4] = {0};

    if (argc < 3)
    {
        fprintf(stderr, "usage: %s <address> <value> [-o <outfile>]\n",
                                argv[0]);
        exit(1);
    }

    int i;
    for (i = 3; i < argc && argv[i][0] == '-'; i++)
    {
        char opt = argv[i][1];
        switch (opt)
        {
        case 'o':
        case 'O':
            name = (argc > i + 1) ? &argv[i + 1][0] : &argv[i][2];
            break;

        default:
            err(1, "invalid option -%c\n", opt);
        }
    }


    FILE * ofd = stdout;
    if (name)
    {
        ofd = fopen(name, "w");
        if (!ofd)
            err(1, "can't create output: %s", name);
    }

    u32 addr = strtoul(argv[1], 0, 16);
    u32 value = strtoul(argv[2], 0, 16);
    u16 addrMsb = addr >> 16;
    u16 addrLsb = addr & 0xffff;

    //  Extended linear address with the MSB of the address.
    emitHexLine(ofd, RT_ExtendedLinearAddress, 0, fill16(out, addrMsb), 2);

    //  Data record.
    emitHexLine(ofd, RT_Data, addrLsb, (u8 *)&value, 4);

    //  A start linear address record (the linker seems to generate this ...).
    emitHexLine(ofd, RT_StartLinearAddress, 0, fill32(out, addr), 4);

    //  End of file record.
    emitHexLine(ofd, RT_EOF, 0, 0, 0);

    fclose(ofd);
    exit(0);
}
