#ident	"@(#)debugger:libexecon/common/PtyList.C	1.2"

// Pseudo-Terminal records
// Contains the functions that are responsible for redirecting
// the I/O from subject processes created by the debugger.
// Programs created by the debugger may do I/O 
// through a pseudo-terminal.

#include "PtyList.h"
#include "Program.h"
#include "Interface.h"
#include "global.h"
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stropts.h>
#include <termio.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

extern char	*ptsname(int);
extern int	unlockpt(int);
extern int	grantpt(int);

#ifdef __cplusplus
}
#endif

PtyInfo		*first_pty = 0;

PtyInfo::~PtyInfo()
{
	delete _name;
	if (first_pty == this)
		first_pty = (PtyInfo *) next();
	close(pty);
	unlink();
}

// set up the debugger side of an I/O channel.
// Grabs the master side of a pty and opens it, setting up appropriate
// magic.

PtyInfo::PtyInfo()
{
	int	ptyflags;

	pty = open("/dev/ptmx", O_RDWR);

	if (pty < 0)
	{
		pty = -1;
		printe(ERR_sys_pty_setup, E_ERROR, strerror(errno));
		return;
	}
	sigset(SIGCLD, SIG_DFL);
	if (grantpt(pty) == -1)
	{
		close(pty);
		pty = -1;
		printe(ERR_sys_pty_setup, E_ERROR, strerror(errno));
		return;
	}
	sigignore(SIGCLD);
	if (unlockpt(pty) == -1)
	{
		close(pty);
		pty = -1;
		printe(ERR_sys_pty_setup, E_ERROR, strerror(errno));
		return;
	}

	// set it up to hit us with SIGPOLL
	if ((ioctl(pty, I_SETSIG, (void *) (S_INPUT|S_HIPRI)) == -1)
		|| ((ptyflags = fcntl(pty, F_GETFL, 0)) == -1) )
	{
		printe(ERR_sys_pty_setup, E_ERROR, strerror(errno));
		close(pty);
		pty = -1;
		return;
	}

	// set up master so read returns -1 with EAGAIN if no data
	// available
	ptyflags |= O_NDELAY;
	if (fcntl(pty, F_SETFL, ptyflags) == -1) 
	{
		printe(ERR_sys_pty_setup, E_ERROR, strerror(errno));
		close(pty);
		pty = -1;
		return;
	}
	if (first_pty)
		prepend(first_pty);
	first_pty = this;
	count = 1;

	char *tmp = ptsname(pty);
	char *slashpos;

	if (slashpos = strrchr(tmp, '/'))
		tmp = slashpos + 1;
	
	_name = new char[strlen(tmp) + sizeof("pts")];
	strcpy(_name, "pts");
	strcat(_name, tmp);
}

void
redirect_childio(int fd)
{
	// To redirect the subject process IO and cause it to
	// properly get characters and interrupts we need to arrange
	// for the PTY to be its controlling terminal.
	// Executed by the child process

	struct	termio	modes;

	close(0);
	if (open(ptsname(fd), O_RDWR) < 0 ||
		ioctl(0, I_PUSH, "ptem") < 0 ||
		ioctl(0, I_PUSH, "ldterm") < 0)
		exit(1);

	// set up pseudo-term so it doesn't map 
	// NL to CR-NL on output and doesn't
	// map NL to CR on input

	if (ioctl(0, TCGETA, &modes) < 0)
		exit(1);
	
	modes.c_oflag &= ~ONLCR;
	modes.c_iflag &= ~INLCR;
	modes.c_lflag &= ~ECHO;

	if (ioctl(0, TCSETAW, &modes) < 0)
		exit(1);
	
	close(1);
	close(2);

	dup(0);		// stdout
	dup(0);		// stderr
}

// Handler for SIGPOLL
void
FieldProcessIO(int sig)
{
	char buf[BUFSIZ+1];

	sigprocmask(SIG_BLOCK, &sset_UPI, 0);

	for (PtyInfo *pty = first_pty; pty; pty = (PtyInfo *) pty->next()) 
	{
		int count = read(pty->pt_fd(), buf, BUFSIZ);

		if (count > 0)
		{
			char	*p = buf, *q;
			buf[count] = 0;
			while(count)
			{
				q = (char *)memchr(p, '\n', count);
				if (q)
				{
					char	save_ch;

					// print up through new-line
					save_ch = *++q;
					*q = '\0';
					printm(MSG_proc_output, pty->name(), p);
					*q = save_ch;

					count -= q - p;
					p = q;
				}
				else
				{
					printm(MSG_proc_output, pty->name(), p);
					break;
				}
			}
		}
		else if (count == -1 && errno != EAGAIN) 
		{
			printe(ERR_sys_pty_read, E_ERROR, 
				pty->name(), strerror(errno));
		}
	}
	interrupt |= sigbit(sig);
	sigprocmask(SIG_UNBLOCK, &sset_UPI, 0);
}
