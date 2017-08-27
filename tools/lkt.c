/*
 *	KAOS term
 *
 *	Terminal communication program similar to tip, cu.
 *
 *	Synopsis:
 *		kt <device>
 *
 *	Tries DEV, /dev/DEV, /dev/ttyDEV in that order.
 */


#undef KAOS
#define STICK
// #define STUBIES


#include		<stdio.h>
#include		<sys/termios.h>
#include		<signal.h>
#include		<errno.h>
#include		<fcntl.h>
#include		<string.h>
#include		<pwd.h>
#include		<ctype.h>
#include		<setjmp.h>
#include		<sys/select.h>
#include		<stdbool.h>
#include		<stdarg.h>
#include		<stdlib.h>
#include		<unistd.h>
#include		<sys/types.h>
#include		<sys/stat.h>


typedef unsigned char	u8;
typedef int		s32;
typedef unsigned int	u32;


#ifdef B230400
#define LINUX_SPEEDS 1
#endif
#ifdef __APPLE__
#define B460800     460800
#define B921600     921600
#endif // __APPLE__
#define EVENFASTER  1


// #define USE_LOCKING
#ifdef USE_LOCKING
#define UUCPLOCK	"/var/lock/LCK.."
#endif

int			Terminal;
char			TerminalName[256];
char			LockName[256];
int			Pid;
struct termios		SavedTs;
char *			MyName;
#ifdef STICK
int			Speed = B230400;
// int			Speed = B115200;
#else // STICK
int			Speed = B9600;
#endif // STICK
int			Size = CS8;
int			Parity = 0;
int			Stops = 0;
int			Flow = IXON | IXOFF;
int			Fast = 1;
int			Kaos = 0;		/* Set if altbaud available */
int			Decode = 0;		/* Decode non-printable chars */
char			Log[256];		/* Log file name */
int			LogFd = -1;
#ifdef USE_LOCKING
int			uucpUid;
int			uucpGid;
#endif

int	DoMonitor;

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

#ifdef USE_LOCKING
void
UUCPunlock()
{
	if (LockName[0])
		(void)unlink(LockName);
}
#endif // USE_LOCKING


void
Error(int flag, char * msg, ...)
{
	va_list ap;

	fprintf(stderr, "%s: ", MyName);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	if (flag)
		perror(MyName);
#ifdef USE_LOCKING
	UUCPunlock();
#endif // USE_LOCKING
	exit(1);
}


void
Warn(int flag, char * msg, ...)
{
	va_list ap;

	fprintf(stderr, "%s: ", MyName);
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	fprintf(stderr, "\n");
	if (flag)
		perror(MyName);
}


#ifdef USE_LOCKING
int
UUCPlock()
{
	extern int		errno;
	int			fd;
	FILE *			FD;
	int			pid;
	register char *		cp;


	{
		/*
		 *	Don't bother trying if we can't write the lock dir.
		 */
		int u = getuid();
		int e = geteuid();
		if (e != 0 && e != uucpUid && e == u)
			return 1;
	}


	if (cp = strrchr(TerminalName, '/'))
		cp++;
	else
		cp = TerminalName;

	(void)strcpy(LockName, UUCPLOCK);
	(void)strcat(LockName, cp);

	cp = LockName + strlen(LockName) - 1;
	if (isupper(*cp))
		*cp = tolower(*cp);

	if ((fd = open(LockName, O_RDWR|O_CREAT|O_EXCL, 0644)) >= 0)
	{
		/*
		 * We need to change ownership of the lock file
		 * because our effective uid is root.
		 */
		if (chown(LockName, uucpUid, uucpGid) == -1)
			Error(1, "Can't chown UUCP lock file");
		FD = fdopen(fd, "w");
	}
	else
	{
		if (errno != EEXIST)
			Error(1, "Can't create UUCP lock file");

		if ((FD = fopen(LockName, "r+")) == NULL)
		{
			LockName[0] = '\0';
			Error(1, "Can't open UUCP lock file");
		}

		pid = -1;
		if (fscanf(FD, "%d", &pid) < 1 || pid <= 0)
		{
			LockName[0] = '\0';
			Error(0, "Can't read UUCP lock file");
		}

		if (kill(pid, 0) == 0 || errno != ESRCH)
		{
			LockName[0] = '\0';
			(void)fclose(FD);	/* process is still running */
			return 0;
		}

		if (fseek(FD, 0L, 0) != 0)
		{
			LockName[0] = '\0';
			Error(1, "Can't seek UUCP lock file");
		}
	}

	pid = getpid();
	(void)fprintf(FD, "%10d\n", pid);
	(void)fclose(FD);

	return 1;
}
#endif // USE_LOCKING


char
ReadChar()
{
	char			c;


	if (read(0, &c, 1) < 0)
		Error(1, "Local read failed");

	return c;
}


void
xprintf(char * fmt, ...)
{
	va_list ap;
	char xxx[1024];

	va_start(ap, fmt);
	(void)vsnprintf(xxx, sizeof xxx, fmt, ap);
	va_end(ap);
	(void)write(1, xxx, strlen(xxx));
}


/*
 *	Run a command.  If 'flag' is set, setup stdin and stdout
 *	of the new process to be the terminal.  If 'cmd' is nil
 *	run an interactive shell.
 */
void
System(flag, cmd)
int			flag;
char *			cmd;
{
	int			pid;
	int			x;
	char *			shell;
	char *			shellname;
	extern char *		getenv();


	switch (pid = fork())
	{
	case -1:
		Error(1, "Can't fork");
		/* NOTREACHED */

	default:
		while ((x = wait((int *)0)) != pid && x != -1)
			xprintf("pid=%d,x=%d\n", pid, x);
			;
		return;

	case 0:
		if (flag)
		{
			(void)close(0);
			(void)close(1);
			(void)dup(Terminal);
			(void)dup(Terminal);
		}

		(void)setuid(getuid());
		(void)signal(SIGINT, SIG_DFL);
		(void)signal(SIGQUIT, SIG_DFL);

		if (!(shell = getenv("SHELL")))
			shell = "/bin/sh";

		if ((shellname = strrchr(shell, '/')))
			shellname++;
		else
			shellname = shell;

		if (cmd)
			(void)execl(shell, shellname, "-c", cmd, (char *)0);
		else
			(void)execl(shell, shellname, "-i", (char *)0);

		Error(1, "Exec failed");
		/* NOTREACHED */
	}
}


void
OpenDev(name)
char *			name;
{
	char			xx[256];
	extern int		errno;


	if ((Terminal = open(name, O_RDWR|O_NDELAY)) >= 0)
	{
		(void)strcpy(TerminalName, name);
		return;
	}

	(void)sprintf(xx, "/dev/%s", name);

	if ((Terminal = open(xx, O_RDWR|O_NDELAY)) >= 0)
	{
		(void)strcpy(TerminalName, xx);
		return;
	}

	(void)sprintf(xx, "/dev/tty%s", name);

	if ((Terminal = open(xx, O_RDWR|O_NDELAY)) >= 0)
	{
		(void)strcpy(TerminalName, xx);
		return;
	}

	if (errno != ENOENT)
		Error(1, "Can't open %s", xx);
	else
		Error(0, "No such device: %s", name);
}


void
IoctlDev()
{
	struct termios		ts;

	if (tcgetattr(Terminal, &ts) < 0)
		Error(1, "tcgetattr failed on device");

#ifdef __APPLE__
	ts.c_iflag = IGNBRK | IGNPAR | (Flow & (IXON|IXOFF));
	ts.c_oflag = 0;
	ts.c_cflag = CREAD | CLOCAL | Size | Parity | Stops;
	ts.c_lflag = 0;
	ts.c_cc[VMIN] = 0;
	ts.c_cc[VTIME] = 1;
	cfsetspeed(&ts, Speed);
#else // __APPLE__

	ts.c_iflag = IGNBRK | IGNPAR | (Flow & (IXON|IXOFF));
	ts.c_oflag = 0;
	ts.c_cflag = CREAD | CLOCAL | Speed | Size | Parity | Stops
#if defined(__linux__)
				| (Flow & CRTSCTS);
#elif defined(SCOunix) || defined(SCOxenix)
				| (Flow & (RTSFLOW|CTSFLOW));
#else
				;
#endif
	ts.c_lflag = 0;
	ts.c_cc[VMIN] = 0;
	ts.c_cc[VTIME] = 1;
#endif // __APPLE__

// printf("tcsetattr: 0x%08x 0x%08x 0x%08x\n",
//			ts.c_iflag, ts.c_oflag, ts.c_cflag);

	if (tcsetattr(Terminal, TCSANOW, &ts) < 0)
		Error(1, "tcsetattr failed on device");

	(void)fcntl(Terminal, F_SETFL, O_RDWR);
}


void
IoctlKaos()
{
#if KAOS
	Mode_t			mode;


	mode.m_id = 255;	/* id should be ignored */
	mode.m_neg = 0;
	mode.m_pos = KM_ALTBAUD;

	if (ioctl(Terminal, KAOSMODE, &mode) == -1)
		Kaos = 0;
	else
		Kaos = 1;
#else
	Kaos = 0;
#endif
}


void
IoctlLocal()
{
	struct termios		ts;
	
	if (!isatty(0) || !isatty(1))
		Error(0, "Standard input and output must be a tty");

	if (tcgetattr(0, &ts) < 0)
		Error(1, "tcgetattr failed on stdin");

	SavedTs = ts;

	ts.c_iflag &= ~(INLCR | ICRNL);
	ts.c_oflag &= ~OPOST;
	ts.c_lflag &= ~(ISIG | ICANON | ECHO);
	ts.c_cc[VMIN] = 1;
	ts.c_cc[VTIME] = 0;

	if (tcsetattr(0, TCSANOW, &ts) < 0)
		Error(1, "tcsetattr failed on stdin");
}


void
FixLocal()
{
	(void)tcsetattr(0, TCSANOW, &SavedTs);
}


void
Receive()
{
	char			buf[128];
	int			cnt;


	for (;;)
	{
		if ((cnt = read(Terminal, buf, (Fast ? sizeof buf : 1))) < 0)
			Error(1, "read on device failed");
		if (LogFd >= 0)
			write(LogFd, buf, cnt);
		if (!Decode)
			(void)write(1, buf, cnt);
		else
		{
			int		i;

			for (i = 0; i < cnt; i++)
			{
				char		c = buf[i];

				if (c == '\r')
					xprintf("\r\n");
				else
				if ((c >= ' ' && c <= '~') || c == '\n')
					write(1, &c, 1);
				else
					xprintf("\\x%02x", c & 0xff);
			}
		}
	}
}


void
FlipDecode(int sig)
{
	Decode = !Decode;
	(void)signal(SIGUSR2, FlipDecode);
}


void
StartChild()
{
	switch (Pid = fork())
	{
	case -1:
		Error(1, "Fork failed");
		/* NOTREACHED */

	case 0:
		(void)signal(SIGUSR1, SIG_DFL);	/* Just in-case */
		(void)signal(SIGUSR2, FlipDecode);
		Receive();
		/* NOTREACHED */
	}
}


void
KillChild()
{
	(void)kill(Pid, SIGUSR1);

	(void)wait((int *)0);
}


void
DoExit()
{
	KillChild();
	xprintf("\r\n");

	FixLocal();

#ifdef USE_LOCKING
	UUCPunlock();
#endif // USE_LOCKING

	exit(0);
}


/* VARARGS1 */
void
PutMenu(fmt, a1, a2, a3, a4, a5, a6)
char *			fmt;
{
	int			j;


	xprintf("\r");
	for (j = 0; j < 79; j++)
		xprintf(" ");
	xprintf("\r");
	xprintf(fmt, a1, a2, a3, a4, a5, a6);
}


void
ReceiveFile()
{
	FixLocal();
	KillChild();

	System(1, "rz");

	(void)sleep(2);

	IoctlDev();
	IoctlLocal();
	StartChild();
}


void
SendFile()
{
	char			xx[256];
	char			name[256];


	KillChild();
	FixLocal();

	name[0] = '\0';
	PutMenu("Filename?  ");
	(void)fgets(name, sizeof name, stdin);
	if (name[0] != '\0')
	{
		(void)sprintf(xx, "sz %s", name);
		System(1, xx);
	}

	IoctlDev();
	IoctlLocal();
	StartChild();
}


void
ParamsMenu()
{
	char *			speed;
	char *			size;
	char *			parity;
	char *			stops;
	char *			flow;


	for (;;)
	{
#ifdef KAOS
		if (Kaos)
		{
			switch (Speed)
			{
			case B300:		speed = "300";		break;
			case B1200:		speed = "1200";		break;
			case B2400:		speed = "2400";		break;
			case B4800:		speed = "4800";		break;
			case B9600:		speed = "9600";		break;
			case B12k:		speed = "12000";	break;
			case B14k4:		speed = "14.4k";	break;
			case B19200:		speed = "19.2k";	break;
			case B38400:		speed = "38.4k";	break;
			case B57k6:		speed = "57.6k";	break;
			case B64k:		speed = "64k";		break;
			case B115k2:		speed = "115.2k";	break;
			}
		}
		else
#endif
		{
			switch (Speed)
			{
			case B300:		speed = "300";		break;
			case B1200:		speed = "1200";		break;
			case B2400:		speed = "2400";		break;
			case B4800:		speed = "4800";		break;
			case B9600:		speed = "9600";		break;
			case B19200:		speed = "19200";	break;
			case B38400:		speed = "38400";	break;
#ifdef LINUX_SPEEDS
			case B57600:		speed = "57600";	break;
			case B115200:		speed = "115200";	break;
			case B230400:		speed = "230400";	break;
			case B460800:		speed = "460800";	break;
			case B921600:		speed = "921600";	break;
#endif
			}
		}
		size = ((Size == CS7) ? "7-bit" : "8-bit");
		switch (Parity)
		{
		case 0:			parity = "none";	break;
		case PARENB:		parity = "even";	break;
		case PARENB|PARODD:	parity = "odd";		break;
		}
		stops = ((Stops == CSTOPB) ? "2 bits" : "1 bit");
		switch (Flow)
		{
		case 0:			flow = "none";		break;
		case IXON|IXOFF:	flow = "XON/XOFF";	break;
#if defined(__linux__)
		case CRTSCTS:		flow = "RTS/CTS";	break;
#elif defined(SCOunix) || defined(SCOxenix)
		case RTSFLOW|CTSFLOW:	flow = "RTS/CTS";	break;
#endif
		}

		PutMenu("Speed(%s), Charsize(%s), Parity(%s), stopBits(%s), Flow(%s)?",
					speed, size, parity, stops, flow);

		switch (ReadChar())
		{
		case 's':
		case 'S':
#ifdef KAOS
			if (Kaos)
			{
				switch (Speed)
				{
				case B300:	Speed = B1200;	break;
				case B1200:	Speed = B2400;	break;
				case B2400:	Speed = B4800;	break;
				case B4800:	Speed = B9600;	break;
				case B9600:	Speed = B12k;	break;
				case B12k:	Speed = B14k4;	break;
				case B14k4:	Speed = B19200;	break;
				case B19200:	Speed = B38400;	break;
				case B38400:	Speed = B57k6;	break;
				case B57k6:	Speed = B64k;	break;
				case B64k:	Speed = B115k2;	break;
				case B115k2:	Speed = B300;	break;
				}
			}
			else
#endif
			{
				switch (Speed)
				{
				case B300:	Speed = B1200;	break;
				case B1200:	Speed = B2400;	break;
				case B2400:	Speed = B4800;	break;
				case B4800:	Speed = B9600;	break;
				case B9600:	Speed = B19200;	break;
				case B19200:	Speed = B38400;	break;
#ifndef LINUX_SPEEDS
				case B38400:	Speed = B300;	break;
#else
				case B38400:	Speed = B57600;	break;
				case B57600:	Speed = B115200; break;
				case B115200:	Speed = B230400; break;

#if !EVENFASTER
				case B230400:	Speed = B300; break;
#else // EVENFASTER
				case B230400:	Speed = B460800; break;
				case B460800:	Speed = B921600; break;
				case B921600:	Speed = B300;	break;
#endif // EVENFASTER
#endif // LINUX_SPEEDS
				}
			}
			break;

		case 'c':
		case 'C':
			switch (Size)
			{
			case CS7:	Size = CS8;	break;
			case CS8:	Size = CS7;	break;
			}
			break;

		case 'p':
		case 'P':
			switch (Parity)
			{
			case 0:		Parity = PARENB;		break;
			case PARENB:	Parity = PARENB | PARODD;	break;
			case PARENB|PARODD:	Parity = 0;		break;
			}
			break;

		case 'b':
		case 'B':
			switch (Stops)
			{
			case 0:		Stops = CSTOPB;		break;
			case CSTOPB:	Stops = 0;		break;
			}
			break;

		case 'f':
		case 'F':
			switch (Flow)
			{
			case 0:			Flow = IXON | IXOFF;	break;
#if defined(__linux__)
			case IXON|IXOFF:	Flow = CRTSCTS;		break;
			case CRTSCTS:		Flow = 0;		break;
#elif defined(SCOunix) || defined(SCOxenix)
			case IXON|IXOFF:	Flow = RTSFLOW | CTSFLOW; break;
			case RTSFLOW|CTSFLOW:	Flow = 0;		break;
#else
			case IXON|IXOFF:	Flow = 0;		break;
#endif
			}
			break;

		case '\n':
		case '\r':
			PutMenu("");
			xprintf("\r");
			return;
		}

		IoctlDev();
	}
}


#ifdef KAOS

void
KaosMenu()
{
	char *			fast;
	int			murder;


	murder = 0;
	for (;;)
	{
		fast = (Fast ? "fast" : "slow");

		PutMenu("Speed(%s)?", fast);

		switch (ReadChar())
		{
		case 's':
		case 'S':
			Fast = !Fast;
			murder = 1;
			break;

		case '\n':
		case '\r':
			PutMenu("");
			xprintf("\r");
			goto done;
		}
	}

done:
	if (murder)
	{
		KillChild();
		StartChild();
	}
}

#endif // KAOS

/**********************************************************************/
/*
 *	Data monitor.
 */

void
Monitor(char * dir, char * data, int len)
{
	if (!DoMonitor)
		return;

	int i;

	printf("Data %s (%d bytes):", dir, len);
	for (i = 0; i < len; i++)
	{
		if (i % 16 == 0)
			printf("\r\n  ");
		if (i % 8 == 0)
			printf(" ");
		if (i % 4 == 0)
			printf(" ");
		printf(" %02x", data[i] & 0xff);
	}
	printf("\n");
}

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
/*
 *	Stick stuff.
 */

#ifdef STICK

/******************************/

void
StickWrite(char * data, int len)
{
	if (len == -1)
		len = strlen(data);
	Monitor("output", data, len);
	(void)write(Terminal, data, len);
}


typedef enum
{
	R_NONE = 0,		// (Don't use)
	R_FLUSH,		// Flush all and return the last
	R_PUSHBACK,		// Push a character back into the queue
	R_CHAR,			// Read one character only
	R_PROMPT,		// Look for a "> "
	R_PROMPT_SPACE,		// Look for ' ' after '>'
	R_GREATER,		// Look for a '>'
	R_NEWLINE,		// Look for a'\n'
	R_ACK,			// Look for "#n"
	R_ACK_DIGIT,		// Look for '[0-9]' after '#'
	R_HEX,			// Look for "0xnn..."
	R_HEX_X,		// Look for 'x' after '0'
	R_HEX_DIGIT,		// Look for hex digit
}
	readOp_e;


int
StickRead(int timeout, readOp_e op)
{
	static char	buf[1024];
	static int	cnt = 0;
	u32		n;
	fd_set		rdSet;
	struct timeval	timeval;
	extern int	errno;

	if (DoMonitor)
		xprintf("StickRead(%d, %d)\n", timeout, op);

	if (op == R_NEWLINE)
	{
		if (DoMonitor)
			xprintf("-- look for newline (recursive)\n");
		for (;;)
		{
			n = StickRead(timeout, R_CHAR);
			if (n < 0)
				return n;
			if (n != '\r' && n != '\n')
				break;
		}

		timeout = n;
		op = R_PUSHBACK;
	}

	if (op == R_PUSHBACK)
	{
		if (DoMonitor)
			xprintf("-- push back %02x\n", timeout);
		if (cnt >= sizeof buf)
			cnt = sizeof buf - 1;

		memmove(&buf[1], &buf[0], cnt);
		buf[0] = timeout;
		cnt++;

		return 0;
	}

	if (op == R_FLUSH)
	{
		timeout = 0;
		cnt = 0;
	}

	for (;;)
	{
  again:
		if (cnt > 0)
		{
			readOp_e next = R_NONE;
			int chr;
			int idx = 0;

			switch (op)
			{
			default:
			case R_FLUSH:
				cnt = 0;
				return 0;	// Should be ignored

			case R_CHAR:
				goto wrap;

			case R_PROMPT:
				next = R_PROMPT_SPACE;
				// Fall through

			case R_GREATER:
				chr = '>';
				break;

			case R_PROMPT_SPACE:
				if (buf[0] == ' ')
					goto wrap;
				else
				{
					op = R_PROMPT;
					goto again;
				}

			case R_NEWLINE:
				chr = '\n';	// Should never get here
				break;

			case R_ACK:
				chr = '#';
				next = R_ACK_DIGIT;
				break;

			case R_ACK_DIGIT:
				if (buf[0] >= '0' && buf[0] <= '9')
					goto wrap;
				else
				{
					op = R_ACK;
					goto again;
				}

			case R_HEX:
				chr = '0';
				n = 0;
				next = R_HEX_X;
				break;

			case R_HEX_X:
				if (buf[0] == 'x')
				{
					next = R_HEX_DIGIT;
					goto wrap;
				}
				else
				{
					chr = '0';
					next = R_HEX_X;
					break;
				}

			case R_HEX_DIGIT:
				chr = buf[0];
				if (chr >= '0' && chr <= '9')
					chr -= '0';
				else if (chr >= 'A' && chr <= 'F')
					chr -= 'A' - 10;
				else if (chr >= 'a' && chr <= 'f')
					chr -= 'a' - 10;
				else
				{
					if (DoMonitor)
						xprintf("-- hex %08x\n", n);
					return n;
				}

				n *= 16;
				n += chr;
				next = op;
				goto wrap;
			}

			for (idx = 0; idx < cnt; idx++)
				if (buf[idx] == chr)
				{
  wrap:
					chr = buf[idx++];
					cnt -= idx;
					memmove(&buf[0], &buf[idx], cnt);

					op = next;
					if (op == R_NONE)
					{
						if (DoMonitor)
						    xprintf("-- wrap %02x\n",
									chr);

						return chr;
					}
					else
						goto again;
				}

			cnt = 0;
		}

		FD_ZERO(&rdSet);
		FD_SET(Terminal, &rdSet);
		timeval.tv_sec = timeout;
		timeval.tv_usec = 0;

		int x = select(Terminal+1, &rdSet, 0, 0, &timeval);

		if (x == 0)			// Timeout
			return -1;

		if (x > 0)			// Something to read
		{
			x = read(Terminal, &buf[0], sizeof buf);

			if (x > 0)
				Monitor("input", &buf[0], x);
		}

		if (x < 0)
		{
			if (errno == EINTR)
				return -2;
			else
			{
				xprintf("read error (%d)\r\n", errno);
				return -3;
			}
		}

		cnt = x;
	}
}


#if 0

int
StickReadNumber(int timeout)
{
	int n = 0;
	int c;
	bool neg = false;

	c = StickRead(timeout, R_CHAR);
	if (c == '-')
	{
		neg = true;
		c = StickRead(timeout, R_CHAR);
	}

	while (c >= '0' && c <= '9')
		n = n * 10 + (c - '0');
#warning "need to read another character here?"

	(void)StickRead(c, R_PUSHBACK);

	return neg ? -n : n;
}

#endif // 0


#if 0

enum
{
	R_ONE = 0x01,		// Wait for a single character
	R_GREATER = 0x02,	// Wait for '>'
	R_NEWLINE = 0x04,	// Flush until '\n'
	R_PROMPT = 0x08,	// Wait for "> "
	R_ACK = 0x10,		// Wait for "#x"
};

char *
StickRead(int timeout, int flags)
{
	int o = 0;
	static char	buf[64];
	extern int	errno;
	fd_set		rdSet;
	struct timeval	timeval;

	for (;;)
	{
  again:
		FD_ZERO(&rdSet);
		FD_SET(Terminal, &rdSet);
		timeval.tv_sec = timeout;
		timeval.tv_usec = 0;

		int x = select(Terminal+1, &rdSet, 0, 0, &timeval);
		if (x == 0)
			return (char *)0;
		if (x > 0)
		{
			if (flags & R_ONE)
				x = read(Terminal, &buf[o], 1);
			else
				x = read(Terminal, &buf[o], sizeof buf - o);

			if (x > 0)
				Monitor("input", &buf[o], x);
		}
		if (x < 0)
		{
			if (errno == EINTR)
				o = 0;
			else
			{
				xprintf("read error (%d)\r\n", errno);
				x = 0;
			}
			break;
		}

		if (flags & R_NEWLINE)
		{
			while (x > 0)
			{
				if (buf[o] == '\n')
				{
					x--;
					memmove(&buf[0], &buf[o+1], x);
					o = x;
					flags &= ~R_NEWLINE;
				}
				o++;
				x--;
			}

			o = 0;
			goto again;
		}

		o += x;

		if (!flags || (flags & R_ONE))
			break;
		if ((flags & R_GREATER) && buf[o-1] == '>')
			break;
		if ((flags & R_PROMPT) &&
		    (o >= 2 && buf[o-2] == '>' && buf[o-1] == ' '))
			break;
		if ((flags & R_ACK) &&
		    (o >= 2 && buf[o-2] == '#' &&
				buf[o-1] >= '0' && buf[o-1] <= '9'))
		{
			buf[o] = '\0';
			return &buf[o-1];
		}

		if (o == sizeof buf)
			Error(0, "too many characters from SAM-BA");
	}

	buf[o] = '\0';
	return &buf[0];
}

#endif // 0

/******************************/
/*
 *	X-modem transmitter.
 */

#define SOH	'\001'
#define ACK	'\006'
#define NAK	'\025'


unsigned short crcData[256] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};

#define crc16(ch, crc)	(crcData[((crc >> 8) & 255)] ^ (crc << 8) ^ ch)


/*
 *	Send one X-modem sector, complete with timeout and retry.
 *	Returns 1 if trnasmitted successfully, and 0 if failed (after
 *	retries).  A call with `data' == NIL sends the end-of-transmission
 *	indicator.
 */
int
StickXmodemSector(int sector, const u8 * data)
{
	int attempts;

	if (!data)
	{
		StickWrite("\4", 1);		// EOT
		(void)StickRead(1, R_CHAR);
		return 1;
	}

	for (attempts = 0; attempts <= 10; attempts++)
	{
		unsigned crc = 0;
		int chr;
		int i;
		char buf[140];

		buf[0] = SOH;
		buf[1] = sector;
		buf[2] = ~sector;

		const u8 * dp = data;
		for (i = 0; i < 128; i++)
		{
			unsigned x = *dp++ & 0xff;
			buf[i+3] = x;
			crc = crc16(x, crc);
		}

		crc = crc16(0, crc16(0, crc));
		buf[3+128] = crc >> 8;
		buf[4+128] = crc;

		StickWrite(&buf[0], 5+128);

		chr = StickRead(2, R_CHAR);
		if (chr == -1)
			continue;		// timeout
		if (chr < 0)
			return 0;		// Interrupt or error

		switch (chr)
		{
		case 'C':
		case ACK:
			return 1;

		case NAK:
			continue;

		default:
			return 0;
		}
	}

	return 0;		// Too many retries.
}


/*
 *	Send a complete file using the X-modem protocol.  `addr' is the
 *	upload address.  The size must be a multiple of 128 bytes;  the last
 *	sector will be rounded up to 128 bytes, sending whatever happens to
 *	be there in memory.  Returns 1 if the upload succeeded, 0 otherwise.
 */
int
StickXmodem(const u8 * data, u32 addr, int size)
{
	char cmd[32];

	sprintf(cmd, "S%x,#", addr);
	StickWrite(cmd, -1);
	(void)StickRead(1, R_NEWLINE);
	(void)StickRead(1, R_CHAR);

	int sectors = (size + 127) / 128;
	int sector;

	if (!DoMonitor)
	{
		printf("                        \r");
		fflush(stdout);
	}

	for (sector = 1; sector <= sectors; sector++)
	{
		if (!DoMonitor)
		{
			xprintf("%d/%d\r", sector, sectors);
			// fflush(stdout);
		}
		else
		{
			xprintf("%d/%d ", sector, sectors);
			// fflush(stdout);
		}

		if (!StickXmodemSector(sector, data))
		{
			Warn(0, "error uploading data");
			return 0;
		}

		data += 128;
	}

	StickXmodemSector(0, 0);

	(void)StickRead(1, R_GREATER);

	return 1;
}


/*
 *	Read an entire file into memory.  Returns 1 if the read was
 *	successfull or 0 on failure;  it reports failure first.  If success,
 *	the address and size are returned.  The memory pointed to by `data'
 *	should be freed when the data is no longer needed.  If `name' is
 *	not given, it is read from the user.  The size of the data read is
 *	rounded up to the next 256 bytes, and filled with 0xff.
 */
int
StickReadFile(char * name, u8 ** data, int * size)
{
	if (!name)
	{
		static char n[256];

		n[0] = '\0';
		PutMenu("Filename?  ");
		(void)fgets(n, sizeof n, stdin);

		int l = strlen(n);
		while (l > 0 &&
		      (n[l-1] == '\r' ||
		       n[l-1] == '\n' ||
		       n[l-1] == ' '))
			l--;
		n[l] = '\0';
		if (n[0] == '\0')
			return 0;

		name = n;
	}

	int fd = open(name, O_RDONLY);
	if (fd < 0)
	{
		Warn(1, "cannot open upload file (%s)", name);
		return 0;
	}

	struct stat statb;
	if (fstat(fd, &statb) < 0)
	{
		Warn(1, "cannot stat upload file (%s)", name);
		close(fd);
		return 0;
	}

	int z0 = statb.st_size;
	int z1 = z0 + 255;
	z1 &= ~255;
	u8 * d = malloc(z1);
	if (!d)
	{
		Warn(0, "can't allocate memory to read file");
		close(fd);
		return 0;
	}

	int x = read(fd, d, z0);
	close(fd);
	if (x < z0)
	{
		if (x < 0)
			Warn(1, "read error");
		else
			Warn(0, "short read");
		return 0;
	}

	*data = d;
	*size = z0;

	while (z0 < z1)
		d[z0++] = 0xff;

	return 1;
}


/*
 *	Try to figure out which device we are talking to.  To do
 *	this, read the device ID register.
 */
void
StickWhichAT91(int * size, int * pg)
{
	// StickWrite("wfffff240,#", -1);
	StickWrite("w400e0740,#", -1);
	u32 id = StickRead(1, R_HEX);
	(void)StickRead(1, R_GREATER);

	static u8 nvSz[16] =
		{  0, 13, 14, 15,
		   0, 16,  0, 17,
		   0, 18, 19,  0,
		  20,  0, 21,  0 };
	u32 sz = (id >> 8) & 0xf;
	*size = 1 << nvSz[sz];
	*pg = (sz <= 5) ? 128 : 256;
}


/*
 *	Check the header on a given file.  Validate the size and return
 *	the load address.
 */
int
StickHeader(u8 * data, int size, int * addr)
{
	u32 * d = (u32 *)data;

	if (size < 256)
		return 0;
	if (d[8] != 0x35424f42)
		return 0;
	*addr = d[9];
	if (d[10] > size)
		return 0;

	return 1;
}


void
StickHello()
{
	StickWrite("\0", 1);
	usleep(10*1000);
	StickWrite("\x80", 1);
	usleep(10*1000);
	StickWrite("\x80", 1);
	usleep(10*1000);
	StickWrite("#", 1);
	if (StickRead(5, R_GREATER) >= 0)
		xprintf("Hello seen\r\n");
}


#if STUBIES

#include	"../stub0/stub0.i"
#include	"../stub1/stub1.i"


void
StickSend(void)
{
	u8 * fileData;
	int fileSize;
	int flashSize;
	int flashPage;
	int x;
	char cmd[32];

	/*
	 *	Figure out the type of device, then start an X-modem
	 *	sequence to upload the SAM-BA loader.
	 */
	StickWhichAT91(&flashSize, &flashPage);
	x = 0x202000;
	if (flashSize > 0 && flashSize <= 32*1024)
		x = 0x201400;

	if (!StickXmodem(Stub0, x, Stub0_size))
		return;
	xprintf("\n");

	sprintf(cmd, "G%x#", x);
	StickWrite(cmd, -1);

	if (StickRead(1, R_GREATER) < 0)
	{
		Warn(0, "Stub0 is not running");
		return;
	}

	x = 0x200800;
	if (!StickXmodem(Stub1, x, Stub1_size))
		return;
	xprintf("\n");

	sprintf(cmd, "G%x#", x);
	StickWrite(cmd, -1);

	if (StickRead(1, R_GREATER) < 0)
	{
		Warn(0, "Stub1 is not running");
		return;
	}

	/*
	 *	Read a file, then send it using X-modem.
	 */
	if (!StickReadFile(0, &fileData, &fileSize))
		return;

	if (!StickHeader(fileData, fileSize, &x))
		return;

	if (!StickXmodem(fileData, x, fileSize))
		return;

	printf("done.\r\n");
	{
		char cmd[32];

		sprintf(cmd, "G%x#", x);
		// StickWrite(cmd, -1);
		// (void)StickRead(1, R_GREATER);
	}
}

#endif // STUBIES


void
StickUpload(int main)
{
	char	name[256];
	int	fd;
	int	addr;
	struct stat statb;

	name[0] = '\0';
	PutMenu("Filename?  ");
	(void)fgets(name, sizeof name, stdin);

	addr = strlen(name);
	while (addr > 0 && (name[addr-1] == '\r' || name[addr-1] == '\n'))
		addr--;
	name[addr] = '\0';
	if (name[0] == '\0')
		return;

	fd = open(name, O_RDONLY);
	if (fd < 0)
	{
		Warn(1, "cannot open upload file (%s)", name);
		return;
	}

	if (fstat(fd, &statb) < 0)
	{
		Warn(1, "cannot stat upload file (%s)", name);
		close(fd);
		return;
	}

#if 1
	if (main)
		StickWrite("S404000,#", -1);
	else
		StickWrite("S20001000,#", -1);
	(void)StickRead(1, R_NEWLINE);
	(void)StickRead(1, R_CHAR);

	addr = 0;

	for (;;)
	{
		u8 buf[128];
		int x = read(fd, &buf[0], sizeof buf);
		// if (x < sizeof buf)
		if (x <= 0)
			break;

		if (!DoMonitor)
		{
			printf("%d/%d\r",
				addr,
				(int)(statb.st_size / sizeof buf));
			fflush(stdout);
		}

		if (!StickXmodemSector(++addr & 0xff, &buf[0]))
		{
			Warn(0, "error uploading data");
			return;
		}
	}

	StickXmodemSector(0, 0);

	if (!DoMonitor)
	{
		printf("                        \r");
		fflush(stdout);
	}

#else
	addr = 0x20001000;

	for (;;)
	{
		char a[32];
		long l;
		int x = read(fd, &l, sizeof l);
		if (x < sizeof l)
			break;

		snprintf(a, sizeof a, "W%x,%x#", addr, l);
		StickWrite(a, -1);
		if (StickRead(5, R_GREATER) < 0)
		{
			Warn(0, "timeout uploading data");
			return;
		}

		addr += sizeof l;
	}
#endif

	close(fd);

	printf("done.\r\n");
	if (!main)
		StickWrite("G20001000#", -1);
}


int
StickSend85(int device, u32 loc, u8 * data, int len, int dport)
{
	char buf[256 * 5 / 4 + 16];	// Big enough to encode 256 bytes
	char * bp = &buf[0];
	u32 cksum = 0;
	u8 * dp = data;

	while (len > 0)
	{
		u32 n = 0;
		if (len >= 1)
			n = dp[0] << 24;
		if (len >= 2)
			n |= dp[1] << 16;
		if (len >= 3)
			n |= dp[2] << 8;
		if (len >= 4)
		{
			n |= dp[3] << 0;
			if (n == 0)
			{
				*bp++ = 'z';
				cksum += 0xbabeface;
				cksum <<= 1;
				goto skip;
			}
		}

		cksum += 0xbabeface;
		cksum <<= 1;
		cksum += n;

		bp[4] = (n % 85) + '!';
		n /= 85;
		bp[3] = (n % 85) + '!';
		n /= 85;
		bp[2] = (n % 85) + '!';
		n /= 85;
		bp[1] = (n % 85) + '!';
		n /= 85;
		bp[0] = (n % 85) + '!';

		if (len > 4)
			bp += 5;
		else
			bp += len + 1;
  skip:
		len -= 4;
		dp += 4;
	}

	*bp = '\0';
	cksum &= 0x7fffffff;

	len = bp - &buf[0];
	bp = &buf[0];

	if (!dport)
	{
		char cmd[32];

		StickWrite("\eX", 2);		// Start string
		StickWrite(bp, len);		// ASCII-85 data.
		StickWrite("\e\\", 2);		// End string

		sprintf(cmd, "\e[91;%d;%d;%d|", device, loc, cksum);
		StickWrite(cmd, -1);

		int x = StickRead(1, R_CHAR);
		if (x != '\6')		// Not ACK
			return 0;
	}
	else
	{
		while (len > 0)
		{
			int l = len;
			if (l > 56)
				l = 56;

			StickWrite("str ", -1);
			StickWrite(bp, l);
			StickWrite("\n", -1);

			bp += l;
			len -= l;

			if (StickRead(1, R_NEWLINE) < 0 ||
			    StickRead(1, R_PROMPT) < 0)
				return 0;
		}

		char cmd[32];
		sprintf(cmd, "memory write %d %d %d\n", device, loc, cksum);
		StickWrite(cmd, -1);

		int x = StickRead(1, R_ACK);
		if (x == '0')		// Not ACK
			return 0;

		if (StickRead(1, R_PROMPT) < 0)
			return 0;
	}

	return 1;
}


void
StickUserUpload(int dport)
{
	char	name[256];
	int	fd;
	int	addr;
	struct stat statb;

	name[0] = '\0';
	PutMenu("Filename?  ");
	(void)fgets(name, sizeof name, stdin);

	addr = strlen(name);
	while (addr > 0 && (name[addr-1] == '\r' || name[addr-1] == '\n'))
		addr--;
	name[addr] = '\0';
	if (name[0] == '\0')
		return;

	fd = open(name, O_RDONLY);
	if (fd < 0)
	{
		Warn(1, "cannot open upload file (%s)", name);
		return;
	}

	if (fstat(fd, &statb) < 0)
	{
		Warn(1, "cannot stat upload file (%s)", name);
		close(fd);
		return;
	}

	name[0] = '\0';
	PutMenu("Device?  ");
	(void)fgets(name, sizeof name, stdin);

	int dev = atoi(name);
	if (dev < 0 || dev > 15)
	{
		Warn(0, "bad device number (%s)", name);
		close(fd);
		return;
	}

	if (!dport)
	{
		char cmd[32];

		sprintf(cmd, "\e[93;%d;0|", dev);
		StickWrite(cmd, -1);

		int x = StickRead(60, R_CHAR);
		if (x != '\6')		// Not ACK
			Warn(0, "no ack from erase");
	}
	else
	{
		(void)StickRead(0, R_FLUSH);
		StickWrite("\n", 1);
		if (StickRead(1, R_PROMPT) < 0)
		{
			Warn(0, "no prompt");
			return;
		}

		char cmd[32];
		sprintf(cmd, "memory erase %d 0\n", dev);
		StickWrite(cmd, -1);

		int x = StickRead(60, R_ACK);
		if (x == '0')		// Not ACK
			Warn(0, "no ack from erase");

		if (StickRead(1, R_PROMPT) < 0)
			Warn(0, "no prompt after erase");
	}

	addr = 0;
	for (;;)
	{
		u8 buf[256];
		int x = read(fd, &buf[0], sizeof buf);
		if (x <= 0)
			break;

		if (!DoMonitor)
		{
			printf("%d/%d\r", (int)(addr / sizeof buf),
					  (int)(statb.st_size / sizeof buf));
			fflush(stdout);
		}

		if (!StickSend85(dev, addr, &buf[0], x, dport))
		{
			printf("failed\n");
			close(fd);
			return;
		}

		addr += 256;
	}

	if (!dport)
	{
		char cmd[32];

		sprintf(cmd, "\e[92;%d;1|", dev);
		StickWrite(cmd, -1);

		int x = StickRead(1, R_CHAR);
		if (x != '\6')		// Not ACK
			Warn(0, "no ack from flush");
	}
	else
	{
		char cmd[32];
		sprintf(cmd, "memory flush %d 1\n", dev);
		StickWrite(cmd, -1);

		int x = StickRead(1, R_ACK);
		if (x == '0')		// Not ACK
			Warn(0, "no ack from flush");

		if (StickRead(1, R_PROMPT) < 0)
			Warn(0, "no prompt after flush");
	}

	close(fd);

	printf("done.                     \r\n");
}


static jmp_buf stickJmp;

static void
stickCatch(int sig)
{
	signal(sig, stickCatch);
	longjmp(stickJmp, 1);
}


void
StickScript(int c)
{
	char buf[1024];
	snprintf(buf, sizeof buf, "script-%c", c);

	FILE * fd = fopen(&buf[0], "r");
	if (!fd)
	{
		xprintf("no script file: %s\r\n", &buf[0]);
		return;
	}

	int cnt = 0;
	for (;;)
	{
		int x = fread(&buf[0], 1, sizeof buf, fd);
		if (x <= 0)
			break;
		(void)write(Terminal, &buf[0], x);
		cnt += x;
	}

	fclose(fd);

	xprintf("sent %d bytes\r\n", cnt);
}


void
StickMenu()
{
	if (setjmp(stickJmp))
	{
		xprintf("intr\r\n");
	}
	else
	{
		int c;

  again:
		PutMenu("Hello, Boot-upload, Main-upload, "
			"User-upload, Yser-upload, Script(0-9)?");

		c = ReadChar();
		PutMenu("");
		xprintf("\r");

		KillChild();
		FixLocal();
		(void)signal(SIGINT, stickCatch);

		switch (c)
		{
		case 'h':
		case 'H':
			StickHello();
			break;

		case 'b':
		case 'B':
			StickUpload(0);
			break;

		case 'm':
		case 'M':
			StickUpload(1);
			break;

#if STUBIES
		case 's':
		case 'S':
			StickSend();
			break;
#endif // STUBIES

		case 'u':
		case 'U':
			StickUserUpload(0);
			break;

		case 'y':
		case 'Y':
			StickUserUpload(1);
			break;

		case '\n':
		case '\r':
			break;

		case '0' ... '9':
			StickScript(c);
			break;

		default:
			goto again;
		}
	}

	(void)signal(SIGINT, SIG_IGN);
	IoctlLocal();
	StartChild();
}

#endif // STICK

/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

void
MainMenu()
{
	for (;;)
	{
		PutMenu("eXit, Break, 8=0x80, Port params, sHell, Send, Rcv, "
#ifdef KAOS
		"Kaos, "
#endif // KAOS
#ifdef STICK
		"sTick, "
#endif // STICK
		"Log, Decode?");

		switch (ReadChar())
		{
		case 's':
		case 'S':
			SendFile();
			PutMenu("Done.");
			xprintf("\r\n");
			return;

		case 'r':
		case 'R':
			ReceiveFile();
			PutMenu("Done.");
			xprintf("\r\n");
			return;

		case 'X':
		case 'x':
			PutMenu("Exit.");
			DoExit();
			/* NOTREACHED */

		case '8':
			{
				char c = 0x80;
				(void)write(Terminal, &c, 1);
			}
			goto done;

		case 'b':
		case 'B':
			(void)tcsendbreak(Terminal, 0);
			/* Fall trough */

		case '\n':
		case '\r':
		done:
			PutMenu("");
			xprintf("\r");
			return;

		case 'p':
		case 'P':
			ParamsMenu();
			return;

#ifdef KAOS
		case 'k':
		case 'K':
			KaosMenu();
			return;
#endif // KAOS

#ifdef STICK
		case 't':
		case 'T':
			StickMenu();
			return;
#endif // STICK

		case 'l':
		case 'L':
			KillChild();
			FixLocal();

			if (Log[0])
			{
				Log[0] = 0;
				LogFd = -1;
			}
			else
			{
				int x;

				PutMenu("Filename?  ");
				(void)fgets(Log, sizeof Log, stdin);

				x = strlen(Log);
				while (x > 0 &&
						(Log[x-1] == '\r' ||
						 Log[x-1] == '\n'))
					x--;
				Log[x] = '\0';
				if (Log[0] != '\0')
				{
					LogFd = open(Log, O_WRONLY |
							  O_APPEND |
							  O_CREAT,
							0666);
					if (LogFd == -1)
					{
						Warn(1, "Can't append to "
							"log file: %s",
								Log);
						Log[0] = 0;
					}

					(void)fchown(LogFd, getuid(), getgid());
				}
			}

			IoctlLocal();
			StartChild();
			PutMenu("");
			xprintf("\r");
			return;

		case 'd':
		case 'D':
			Decode = !Decode;
			if (Decode)
				PutMenu("Decode on");
			else
				PutMenu("Decode off");
			xprintf("\r\n");
			(void)kill(Pid, SIGUSR2);	/* Kick receive part */
			return;

		case 'H':
		case 'h':
		case '!':
			PutMenu("Shell.");
			xprintf("\r\n");

			FixLocal();

			System(0, (char *)0);

			IoctlLocal();

			xprintf("\r\nOnline.\r\n");
			break;
		}
	}
}


void
Transmit()
{
	char			c;


	for (;;)
	{
		c = ReadChar();

		if (c == '\1')		/* Control-A */
			MainMenu();
		else
			(void)write(Terminal, &c, 1);
	}
}


void
SetSpeed(char * speed)
{
	int s = atoi(speed);
	int sx;

	switch (s)
	{
	case 300:	sx = B300;		break;
	case 1200:	sx = B1200;		break;
	case 2400:	sx = B2400;		break;
	case 4800:	sx = B4800;		break;
	case 9600:	sx = B9600;		break;
	case 19200:	sx = B19200;		break;
	case 38400:	sx = B38400;		break;
#ifdef LINUX_SPEEDS
	case 57600:	sx = B57600;		break;
	case 115200:	sx = B115200;		break;
	case 230040:	sx = B230400;		break;
	case 460800:	sx = B460800;		break;
	case 921600:	sx = B921600;		break;
#endif
	default:
		Error(0, "unknown speed: %d", s);
		// Not reached
	}

	Speed = sx;
}


int
main(argc, argv)
int			argc;
char **			argv;
{
	extern int			errno;
#ifdef USE_LOCKING
	struct passwd			*uucpPwEnt;
#endif // USE_LOCKING

	MyName = *argv++;
	argc--;

	while (*argv && **argv == '-')
	{
		if (strcmp(*argv, "-m") == 0)
			DoMonitor = 1;
		if (strcmp(*argv, "-s") == 0)
		{
			argc--;
			SetSpeed(*++argv);
		}
		else if (strncmp(*argv, "-s", 2) == 0)
		{
			SetSpeed(&(*argv)[2]);
		}

		argv++;
		argc--;
	}

	if (argc != 1)
	{
		(void)fprintf(stderr, "usage: %s <device>\n", MyName);
		exit(1);
	}

	/*
	 *	Open device.  Try DEV, /dev/DEV & /dev/ttyDEV in that order.
	 */
	OpenDev(*argv);

	(void)signal(SIGINT, SIG_IGN);
	(void)signal(SIGQUIT, SIG_IGN);

#ifdef USE_LOCKING
	/*
	 * Get uucp's uid and gid and store it for later use.
	 */
	if ((uucpPwEnt = getpwnam("uucp")) == NULL)
		Error(0, "Unable to read uucp entry in /etc/passwd");
	uucpUid = uucpPwEnt->pw_uid;
	uucpGid = uucpPwEnt->pw_gid;

	if (!UUCPlock())
		Error(0, "Device %s in use", TerminalName);
#endif // USE_LOCKING

	IoctlDev();
	IoctlKaos();
	IoctlLocal();

	(void)write(1, "Connected.\r\n", 12);

	StartChild();

	Transmit();

	/* NOTREACHED */
	return 0;
}
