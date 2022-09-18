/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)shl:newmain.c	1.9.10.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/shl/newmain.c,v 1.1 91/02/28 20:09:33 ccs Exp $"
#include	"defs.h"
#include	<utmp.h>
#include	<unistd.h>
#include	<sys/stat.h>
#include	<sys/stropts.h>
#include	<locale.h>
#include	<string.h>

static char	rl_ttynm[100];
static char	base_ttynm[100];
int 	real_tty_fd;
static int	adjusted;
static int	ttytype;
static struct termio	ttystart;

char			*ttyname();

struct utmp		u_start;
struct utmp		*getutent();
struct utmp		*getutline();
void			setutline();

main(argc, argv, envp)
	int argc;
	char *argv[];
	char *envp[];
{
	int retval;
	int i;
	int temp_fd;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxue");
	(void)setlabel("UX:shl");
	
	adjusted = 0;

	uid = getuid();
	gid = getgid();

	cntlf = cntl_fl;
		
	if ((cntl_chan_fd = open_cntl_chan()) == SYSERROR)
		exit(1);

	if (isastream(cntl_chan_fd))
		ioctl(cntl_chan_fd,I_SETSIG,S_HANGUP);

	setsig();
	get_ttynm();

	if ((real_tty_fd = open(rl_ttynm, O_RDWR)) == SYSERROR)
	{
		pfmt(stderr, MM_ERROR, 
			":40:Real tty open failed: %s\n", strerror (errno));
		exit(2);
	}

	if (real_tty_fd < 3)			/* check if 0, 1, or 2 closed when invoked */
	{
		temp_fd = fcntl(real_tty_fd, F_DUPFD, 3);
		close(real_tty_fd);
		real_tty_fd = temp_fd;
	}
	
	ioctl(real_tty_fd, TCGETA, &ttystart);		/* save real tty settings */
	ttysave = ttystart;

	if (ttysave.c_cc[VSWTCH] == CNSWTCH)
	{
		ttysave.c_cc[VSWTCH] = CSWTCH;
		ioctl(real_tty_fd, TCSETAW, &ttysave);
	}


	if (multiplex())
	{
		for (i = 0; i < 3; ++i)
		{
			close(i);
			dup(cntl_chan_fd);
		}

		if (ioctl(cntl_chan_fd, TCSETAW, &ttysave) == SYSERROR)
		{
			pfmt(stderr, MM_ERROR, badioctl, strerror (errno));
			reset();
			exit(8);
		}
		
		adj_utmp(0);
		adjusted = 1;

		prompt();
		yyparse();				/* command loop	*/
		retval = 0;
	}
	else
		retval = 3;

	reset();
	
	exit(retval);
}


reset()
{
	int i;
	extern int cook;

	if(close_cntl_chan() == SYSERROR){
		pfmt(stderr, MM_WARNING, 
			":41:Could not reset permissions on sxt devices\n");
	}

	if (ioctl(cntl_chan_fd, I_UNLINK, cook) == SYSERROR)
	{
		pfmt(stderr, MM_WARNING, 
			":42:ioctl() I_UNLINK failed: %s\n", strerror (errno));
	}
	close(cntl_chan_fd);
	for (i = 0; i < 3; ++i)
		close(i);

	if (adjusted)
	{
		strncpy(u_entry->ut_line, base_ttynm, sizeof(u_entry->ut_line));
		pututline(u_entry);
		endutent();
	}

	ioctl(real_tty_fd, TCSETAW, &ttystart);		/* restore real tty */

}


get_ttynm()
{
	char *ttynm;
	struct stat	fd0, fd1, fd2, usb;
	
	char	*strcat();

	if ((ttynm = ttyname(0)) != NULL)
	{
		if (strncmp(ttynm+5,"xt",2) == 0) {
			pfmt(stderr, MM_ERROR, 
				":43:Cannot invoke shl from within layers\n");
			exit(1);
		}
		fstat(0, &fd0);
		if (fstat(1, &fd1) == 0					&&
		    fstat(2, &fd2) == 0)
		{
			if (fd0.st_rdev == fd1.st_rdev		&&
			    fd1.st_rdev == fd2.st_rdev)
			{
				strcpy(rl_ttynm, ttynm);
				/* skip over "/dev/" in ttynm[] */
				strcpy(base_ttynm, &ttynm[5]);

				strcpy(u_start.ut_line, base_ttynm);
				if ((u_entry = getutline(&u_start)) == NULL)
				{
					setutent();
					while ((u_entry = getutent()) != NULL)
					{
						if (u_entry->ut_type == USER_PROCESS)
						{
							strcpy(rl_ttynm, "/dev/");
							strcat(rl_ttynm, u_entry->ut_line);
							if (stat(rl_ttynm, &usb) == 0)
							{
								if (usb.st_rdev == fd0.st_rdev)
								{
									strcpy(base_ttynm, u_entry->ut_line);
									return;
								}
							}
						}
					}
				}
				else
					return;

				pfmt(stderr, MM_ERROR, 
					":44:Cannot find your utmp entry\n");
				exit(11);
			}
		}
	}

	pfmt(stderr, MM_ERROR, ":45:Cannot determine your login terminal\n");
	exit(9);
}
