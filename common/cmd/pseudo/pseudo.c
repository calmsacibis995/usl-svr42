/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)pseudo:pseudo.c	1.1.2.3"
#ident  "$Header: pseudo.c 1.2 91/06/27 $"

/*
 * Pseudo-tty controller.  This process forks.  The parent
 * retains control of the original stdio files (0, 1, 2).
 * The child gets the pseudo device on 0, 1, 2.  The child
 * execs a shell.  The parent divides into a reader and a
 * writer.  The parent-reader reads the real device and
 * writes the pty.  The parent-writer reads the pty and
 * writes the real device.  The parent-reader is the original
 * process; everyone else is a child.  The parent-reader
 * catches the child-death signal and exits.
 *
 * NOTE: as of 5/31/90, pseudo tries to push LDTERM
 * on top of PTEM in this order; you have
 * to change this source code to override this strategy.
 */

#include <sys/fcntl.h>
#include <sys/termio.h>
#include <sys/stropts.h>
#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>

int pid;	/* pid of child process */
int fdm, fds;	/* master & slave fdescs */
void handler();	/* child-death handler for original process */
int loginshell = 0;

main(argc, argv, envp)
	int    argc;
	char **argv;
	char **envp;
{
	char *namel;
	char *ptsname();

	extern int errno;
	extern int optind;
	extern char *optarg;
	extern int opterr;
	int optchr;
	char ldname[16];

	/* Initialize locale, message label, message catalog. */
	setlocale(LC_ALL, "");
	setlabel("UX:pseudo");
	setcat("uxnsu");

	opterr=0; *ldname = '\0';
	while ((optchr = getopt(argc, argv, "lm:")) != EOF)
		switch (optchr) {
		case '?':
		default:
			pfmt(stderr, MM_ACTION, ":1:Usage: pseudo [-m line] [-l]\n");
			exit(1);
		case 'l':
			loginshell = 1;
			break;
		case 'm':
			(void)strncpy(ldname, optarg, 16);
			break;
		}
	if (optind < argc) {
		pfmt(stderr, MM_ACTION, ":1:Usage: pseudo [-m line] [-l]\n");
		exit(1);
	}
	/*
	 * Open the master device. This is a clone
	 * device
	 */
	if ((fdm = open("/dev/ptmx", O_RDWR)) < 0) {
		pfmt(stderr, MM_ERROR, ":2:%s failed for master: %s\n", "open", strerror(errno));
		exit(1);
	}
	/*
	 * Change ownership of the slave device
	 */
	if (grantpt(fdm) < 0) {
		pfmt(stderr, MM_ERROR, ":3:%s failed: %s\n", "grantpt", strerror(errno));
		exit(1);
	}
	/*
	 * Take off an internal lock in the newly created
	 * invokation of /dev/ptmx
	 */
	if (unlockpt(fdm) < 0) {
		pfmt(stderr, MM_ERROR, ":3:%s failed: %s\n", "unlockpt", strerror(errno));
		exit(1);
	}
	/*
	 * Get the slave name
	 */
	if (!(namel = ptsname(fdm))) {
		pfmt(stderr, MM_ERROR, ":3:%s failed: %s\n", "ptsname", strerror(errno));
		exit(1);
	}
	fprintf(stderr, "pseudo tty: %s\n", namel);
	/* to avoid noise on the screen */
	sleep(1);
	/*
	 * May want to call setpgrp() to become
	 * a controlling terminal on the slave side
	 */
	setpgrp();
	/*
	 * Open the slave device
	 */
	if ((fds = open(namel, O_RDWR)) < 0) {
		pfmt(stderr, MM_ERROR, ":4:%s failed for slave: %s\n", "open", strerror(errno));
		exit(1);
	}
	/*
	 * Push the pseudo terminal module on the slave
	 */
	if (ioctl(fds, I_PUSH, "ptem") < 0) {
		pfmt(stderr, MM_ERROR, ":3:%s failed: %s\n", "ioctl(I_PUSH, ptem)", strerror(errno));
		exit(1);
	}
	/*
	 * Push the line discipline module on the slave
	 */
	if (*ldname != '\0') {
		if (ioctl(fds, I_PUSH, ldname) < 0) {
			pfmt(stderr, MM_ERROR, ":3:%s failed: %s\n", "ioctl(I_PUSH)", strerror(errno));
			exit(1);
			/* TRY ONCE ONLY */
		}
	} else for (;;) {
		if (ioctl(fds, I_PUSH, "ldterm") == 0)
			break;
		pfmt(stderr, MM_ERROR, ":3:%s failed: %s\n", "ioctl(I_PUSH)", strerror(errno));
		exit(1);
	}
	do_ttystuff();
	/*
	 * Master & slave now opened.  We fork.  The master(parent) gets
	 * control of the original device.  The slave(child) gets
	 * the pseudo device.
	 */
	switch (pid = fork()) {
	case -1:
		pfmt(stderr, MM_ERROR, ":3:%s failed: %s\n", "fork #1", strerror(errno));
		exit(1);
	case 0:	/* child/slave process */
		child(envp);
		break;
	default:/* parent/master process */
		parent();
	}
	exit(0);
}

/*
 * attached to the user
 */
child(envp)
	char	**envp;
{
	close(0);
	close(1);
	close(2);
	close(fdm);
	dup(fds);
	dup(fds);
	dup(fds);
	close(fds);
	if (loginshell)
		execle("/bin/sh", "-sh", (char *)0, envp);
	else
		execle("/bin/sh", "sh -i", (char *)0, envp);
	pfmt(stderr, MM_ERROR, ":3:%s failed: %s\n", "execle(sh -i)", strerror(errno));
	exit(1);
}

/*
 * attached to the original device
 */
parent()
{
	char	buf[512];
	int	i, s;

	close(fds);
	close(2);
	/*
	 * Now, 0, 1 are original device, 2, fdm are pseudo device
	 */
	switch (fork()) {
	case 0:	/* original dev reader */
		close(1);
		for (s=0; s<NSIG; s++)
			signal(s, SIG_IGN);
		signal(SIGHUP, SIG_DFL);
		signal(SIGCLD, SIG_DFL);
		while (1) {
			i = read(0, buf, 512);
			if (i > 0)
				write(fdm, buf, i);
		}
		break;
	default: /* pty reader */
		close(0);
		for (s=0; s<NSIG; s++)
			signal(s, SIG_IGN);
		signal(SIGCLD, handler);
		while (1) {
			if ((i = read(fdm, buf, 512)) < 0) {
				pfmt(stderr, MM_ERROR, "READ ERROR\n");
				ioctl(1, TCSETA, &s);
				exit(1);
			}
			if (i >= 1)
				write(1, buf, i);
		}
		break;
	case -1:
		pfmt(stderr, MM_ERROR, ":3:%s failed: %s\n", "fork #2", strerror(errno));
		exit(1);
	}
}

/*
 * "s" is the Saved states on original tty.
 * "t" becomes "target" on original tty.
 */
 
struct termio t, s;

void
handler()

{
	FILE 	*fd;

	ioctl(1, TCSETA, &s);
	write(1, "\n", 1);
	exit(0);
}

/*
 * Set totally-raw mode on the original device and set whatever WE
 * got on the new pseudo device.
 */
do_ttystuff()
{
	if (ioctl(0, TCGETA, &t) < 0) {
		pfmt(stderr, MM_ERROR, ":3:%s failed: %s\n", "ioctl(TCGETA)", strerror(errno));
		exit(1);
	}
	/*
	 * Cook the pty, freeze the original.
	 */
	if (ioctl(fds, TCSETA, &t) < 0) {
		pfmt(stderr, MM_ERROR, ":3:%s failed: %s\n", "ioctl(TCSETA)", strerror(errno));
		exit(1);
	}
	s = t;
	t.c_lflag = t.c_iflag = t.c_oflag = 0;
	/*
	 * stty cs8 -parity
	 */
	t.c_cflag |= CS8;
	t.c_cflag &= ~PARENB;
	t.c_cc[VMIN] = 1;
	t.c_cc[VTIME] = 0;
	if (ioctl(0, TCSETA, &t) < 0) {
		pfmt(stderr, MM_ERROR, ":3:%s failed: %s\n", "ioctl(TCSETA)", strerror(errno));
		exit(1);
	}
}
