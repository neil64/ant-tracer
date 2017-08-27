/*
 *  Simple telnet like tool to connect to a puck or relay v.2 via a segger.
 *  (It was easier to write this than get telnet to work the way we want.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/termios.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <err.h>


typedef unsigned int u32;

char * Host = "localhost";
u32 Addr;
int Port = 19021;
int Conn = -1;
int Restart = true;

struct sockaddr_in SockAddr;

int FlushTerm;

void Catch(int sig);
void IoctlLocal(void);

/**********************************************************************/

void
Connect(void)
{
    /*
     *  Look up the server IP address.
     */
    struct hostent * host;
    host = gethostbyname(Host);
    if (!host)
        err(1, "can resolve the server address");

    if (host->h_addrtype != AF_INET)
        errx(1, "host address does not resolve to an IP address");
    if (host->h_length != 4)
        errx(1, "only IPv4 addesses supported");
    Addr = *(u32 *)host->h_addr;

    // printf("address type = %d (%d)\n", host->h_addrtype, AF_INET);
    // printf("address length = %d\n", host->h_length);
    // printf("address = %08x\n", Addr);
    // printf("port = %08x\n", Port);

    /*
     *  Create a socket for the connection to the server.
     */
    Conn = socket(AF_INET, SOCK_STREAM, 0);
    if (Conn < 0)
        err(1, "can't create a socket");

#if 0
    /*
     *  Set the socket to non-blocking.
     */
    if (fcntl(Conn, F_SETFL, O_NONBLOCK) < 0)
        err(1, "can't set non-blocking on server connection socket");
#endif // 0

    /*
     *  Make the connection.
     */
    SockAddr.sin_family = AF_INET;
    SockAddr.sin_addr.s_addr = Addr;
    SockAddr.sin_port = htons(Port);

    if (connect(Conn, (struct sockaddr *)&SockAddr, sizeof SockAddr) < 0)
        err(1, "can't connect to server (%s)", Host);
}


void
Reconnect(void)
{
    int spin = 0;

    fprintf(stderr, "-----------------------------------------------------\n");

    for (;;)
    {
        close(Conn);

        fprintf(stderr, "   %c\r", "|/-\\"[spin++]);
        spin &= 3;

        sleep(2);

        Conn = socket(AF_INET, SOCK_STREAM, 0);
        if (Conn < 0)
            err(1, "can't create a socket");

        if (connect(Conn, (struct sockaddr *)&SockAddr, sizeof SockAddr) >= 0)
            break;

        if (errno == ECONNREFUSED)
            continue;
        else
            err(1, "can't connect to server (%s)", Host);
    }

    fprintf(stderr, "       \n");
    IoctlLocal();
    FlushTerm = true;
}

/**********************************************************************/

struct termios SavedTs;
int LocalTermSet = false;


void
IoctlLocal(void)
{
    struct termios          ts;

    if (!isatty(0) || !isatty(1))
        errx(1, "Standard input and output must be a tty");

    if (tcgetattr(0, &ts) < 0)
        err(1, "tcgetattr failed on stdin");

    SavedTs = ts;

    ts.c_iflag &= ~(INLCR | ICRNL);
    ts.c_oflag &= ~OPOST;
    // ts.c_lflag &= ~(ISIG | ICANON | ECHO);
    ts.c_lflag &= ~(ICANON | ECHO);
    ts.c_lflag &= ISIG;
    ts.c_cc[VINTR] = 0;
    ts.c_cc[VMIN] = 1;
    ts.c_cc[VTIME] = 0;

    if (tcsetattr(0, TCSANOW, &ts) < 0)
        err(1, "tcsetattr failed on stdin");
    LocalTermSet = true;

    signal(SIGINT, SIG_IGN);
}


void
FixLocal()
{
    if (LocalTermSet)
        (void)tcsetattr(0, TCSANOW, &SavedTs);
    signal(SIGINT, Catch);
}


void
Catch(int sig)
{
    FixLocal();
    printf("\n");
    exit(0);
}

/**********************************************************************/

void
CharacterLoop(void)
{
    for (;;)
    {
        fd_set rdSet;
        FD_ZERO(&rdSet);
        FD_SET(0, &rdSet);
        FD_SET(Conn, &rdSet);

        fd_set exSet;
        FD_ZERO(&exSet);
        FD_SET(0, &exSet);
        FD_SET(Conn, &exSet);

        int x = select(Conn+1, &rdSet, 0, &exSet, 0);
        if (x < 0)
            err(1, "select failed");

        if (x == 0)
            errx(1, "select returned 0");
            // continue;

        if (FD_ISSET(0, &rdSet))
        {
            /*
             *  Got something on standard input.
             */
            char buf[1024];
            x = read(0, &buf[0], sizeof buf);
            if (x < 0)
                err(1, "can't read standard input");
            int y = 0;
            if (!FlushTerm)
                y = write(Conn, &buf[0], x);
            else
            {
                FlushTerm = false;
                y = x;
            }

            if (y < 0)
                err(1, "can't write to the server");
            if (y < x)
                errx(1, "can't write all data to the server (%d of %d)", y, x);
        }

        if (FD_ISSET(Conn, &rdSet))
        {
            /*
             *  Got something from the server.
             */
            char buf[1024];
            x = read(Conn, &buf[0], sizeof buf);
            if (x < 0)
                err(1, "can't read from the socket");
            if (x == 0)
            {
                FixLocal();
                fprintf(stderr, "\nEOF from connection\n");
                if (Restart)
                {
                    Reconnect();
                    continue;
                }
                else
                    exit(0);
            }
            int y = write(1, &buf[0], x);
            if (y < 0)
                err(1, "can't write to standard output");
            if (y < x)
                errx(1, "can't write all data from the server (%d of %d)", y,x);
        }

        if (FD_ISSET(0, &exSet))
            errx(1, "exception on stdin");
        if (FD_ISSET(Conn, &exSet))
            errx(1, "exception on connection");
    }
}

/**********************************************************************/

int
main(int argc, char ** argv)
{
    /*
     *  Get options.
     */
    extern int optind;
    extern char * optarg;
    int c;

    while ((c = getopt(argc, argv, "")) != -1)
        switch (c)
        {
        }

    if (optind < argc)
        Host = argv[optind++];
    if (optind < argc)
        Port = strtol(argv[optind++], 0, 0);

    /*
     *  Connect to the server.
     */
    Connect();

    /*
     *  Set up the local terminal.
     */
    IoctlLocal();
    signal(SIGQUIT, Catch);
    signal(SIGTERM, Catch);

    /*
     *  Move characters.
     */
    CharacterLoop();

    /*
     *  Clean up.
     */
    FixLocal();

    /*
     *  Bye.
     */
    return 0;
}
