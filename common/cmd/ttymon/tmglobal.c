/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ttymon:common/cmd/ttymon/tmglobal.c	1.9.11.3"
#ident  "$Header: tmglobal.c 1.2 91/06/24 $"

#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <sys/resource.h>
#include <sac.h>
#include <sys/termio.h>
#include <sys/stream.h>
#include <sys/tp.h>
#include "tmstruct.h"
#include "ttymon.h"

/*
 *	global fd and fp
 */
int	Pfd;			/* for pmpipe 				*/
int	PCpipe[2];		/* pipe between Parent & Children 	*/

char	State = PM_STARTING;	/* current state			*/
char	*Tag;			/* port monitor tag			*/
int	Maxfds;			/* Max no of devices ttymon can monitor */

int	Reread_flag = FALSE;	/* reread pmtab flag			*/

int	Retry;			/* retry open_device flag		*/

int	EnhancedSecurityInstalled = FALSE;
int	MACRunning = FALSE;
int	B2Running = FALSE;

int	express_mode = FALSE;

char	*Consoledsf = "/dev/systty";

struct  pmtab *PMtab = NULL;	/* head pointer to pmtab linked list 	*/
int	Nentries = 0;		/* # of entries in pmtab linked list	*/

struct  Gdef Gdef[MAXDEFS];	/* array to hold entries in /etc/ttydefs */
int	Ndefs = 0;		/* highest index to Gdef that was used   */

char	Scratch[BUFSIZ];	/* general scratch buffer 	*/

gid_t	Tty_gid = 7;		/* group id for all tty devices		*/

/*
 * Nlocked - 	number of ports that are either locked or have active
 *		sessions not under this ttymon.
 */
int	Nlocked = 0;

/* original rlimit value */
struct	rlimit	Rlimit;

/*
 * places to remember original signal dispositions and masks
 */

sigset_t	Origmask;		/* original signal mask */
struct	sigaction	Sigalrm;	/* SIGALRM */
struct	sigaction	Sigcld;		/* SIGCLD */
struct	sigaction	Sigint;		/* SIGINT */
struct	sigaction	Sigpoll;	/* SIGPOLL */
struct	sigaction	Sigterm;	/* SIGTERM */
#ifdef	DEBUG
struct	sigaction	Sigusr1;	/* SIGUSR1 */
struct	sigaction	Sigusr2;	/* SIGUSR2 */
#endif
