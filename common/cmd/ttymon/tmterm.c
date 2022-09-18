/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/tmterm.c	1.11.10.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/ttymon/tmterm.c,v 1.1 91/02/28 20:16:34 ccs Exp $"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <termio.h>
#include <sys/stermio.h>
#include <sys/termiox.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <priv.h>
#include <pfmt.h>
#include "sys/stropts.h"
#include "sys/signal.h"
#include "ttymon.h" 
#include <sys/termios.h>
#include <sys/stream.h>
#include <sys/tp.h>
#include "tmstruct.h" 

extern	char	Scratch[];
extern	void	log();
extern	void	mkargv();

extern	const	char badioctl[], badioctlfd[];

/*
 * Procedure:	  set_termio
 *
 * Restrictions:
                 strerror: none
*/

/*
 *	set_termio	- set termio on device 
 *		fd	- fd for the device
 *		options - stty termio options 
 *		aspeed  - autobaud speed 
 *		clear	- if TRUE, current flags will be set to some defaults
 *			  before applying the options 
 *		    	- if FALSE, current flags will not be cleared
 *		mode	- terminal mode, CANON, RAW
 */
int
set_termio(fd,options,aspeed,clear,mode)
int	fd;
char	*options;
char	*aspeed;
int	clear;
long	mode;
{
	struct 	 termio termio;
	struct 	 termios termios;
	struct 	 stio stermio;
	struct 	 termiox termiox;
	struct 	 winsize winsize;
	struct 	 winsize owinsize;
	int	 term;
	int	 cnt = 1;
	char	 *uarg;	
	char	 *argvp[MAXARGS];	/* stty args */
	static   char	 *binstty = "/usr/bin/stty";
	static	 char	buf[BUFSIZ];
	extern 	 int get_ttymode(), set_ttymode();
	extern	 char	*sttyparse();

#ifdef	DEBUG
	debug("in set_termio");
#endif

	if ((term = get_ttymode(fd, &termio, &termios, &stermio, 
				&termiox, &winsize)) < 0) {
		log(MM_ERROR, ":666:set_termio: get_ttymode() failed: %s",
			strerror(errno));
		return(-1);
	}
	owinsize = winsize;
	if (clear) {
		termios.c_iflag = 0;
		termios.c_cflag = 0;
		termios.c_lflag = 0;
		termios.c_oflag = 0;

		termios.c_iflag |= (IGNPAR|ISTRIP|ICRNL|IXON); 
		termios.c_cflag |= CS7|CREAD|PARENB|(B9600&CBAUD);
		if (mode & CANON) {
			termios.c_lflag |= (ISIG|ICANON|ECHO|ECHOE|ECHOK); 
			termios.c_cc[VEOF] = CEOF;
			termios.c_cc[VEOL] = CNUL;
		}
		else  {
			termios.c_lflag &= ECHO;
			termios.c_cc[VMIN] = 1;
			termios.c_cc[VTIME] = 0;
		}
		termios.c_oflag |= OPOST|ONLCR;

	}

	if ((options != NULL) && (*options != '\0')) {
		/* just a place holder to make it look like invoking stty */
		argvp[0] = binstty;
		(void)strcpy(buf,options);
		mkargv(buf,&argvp[1],&cnt,MAXARGS-1);
		if ((aspeed != NULL) && (*aspeed != '\0')) {
			argvp[cnt++] = aspeed;
		}
		argvp[cnt] = (char *)0;
		if ((uarg = sttyparse(cnt, argvp, term, &termio, &termios, 
				&termiox, &winsize)) != NULL) {
			log(MM_ERROR, ":667:sttyparse: Unknown mode: %s", uarg);
			return(-1);
		}
	}
	if (set_ttymode(fd, term, &termio, &termios, &stermio, 
			&termiox, &winsize, &owinsize) != 0) {
		log(MM_ERROR, ":668:set_termio: set_ttymode() failed: %s",
			strerror(errno));
		return(-1);
	}
	return(0);
}

/*
 * Procedure:	  turnon_canon
 *
 * Restrictions:
		 ioctl(2): none
		 strerror: none
*/

/*
 *	turnon_canon	- turn on canonical processing
 *			- return 0 if succeeds, -1 if fails
 */
turnon_canon(fd)
int	fd;
{
	struct termio termio;
	static const char name[] = "turnon_canon";
	
#ifdef	DEBUG
	debug("in turnon_canon");
#endif
	/* All of the ioctl(2) calls in this function require privilege.
	 * Warning! New system calls placed in this function must be
	 * evaluated for their use of privilege!
	 *
	 * P_DEV      -  access devices in private state
	 * P_MACWRITE -  ensure write access
	 */
	if (ioctl(fd, TCGETA, &termio) != 0) {
		log(MM_ERROR, badioctlfd, name, "TCGETA", fd, strerror(errno));
		return(-1);
	}
	termio.c_lflag |= (ISIG|ICANON|ECHO|ECHOE|ECHOK); 
	termio.c_cc[VEOF] = CEOF;
	termio.c_cc[VEOL] = CNUL;
	if (ioctl(fd, TCSETA, &termio) != 0) {
		log(MM_ERROR, badioctlfd, name, "TCSETA", fd, strerror(errno));
		return(-1);
	}
	return(0);
}

/*
 * Procedure:	  flush_input
 *
 * Restrictions:
                 ioctl(2): none
                 strerror: none
*/

/*
 *	flush_input	- flush the input queue
 */
void
flush_input(fd)
int	fd;
{
	if (ioctl(fd, I_FLUSH, FLUSHR) == -1) {
		log(MM_ERROR, badioctlfd, "flush_input", "I_FLUSH", fd,
			strerror(errno));
	}
	return;
}

/*
 * Procedure:	  push_linedisc
 *
 * Restrictions:
                 ioctl(2): none
                 strerror: none
*/

/*
 * push_linedisc	- if modules is not NULL, pop everything
 *			- then push modules specified by "modules"
 */

push_linedisc(fd,modules,device)
int	fd;		/* fd to push modules on			 */
char	*modules;	/* ptr to a list of comma separated module names */
char	*device;	/* device name for printing msg			 */
{
	char	*p, *tp;
	char	buf[BUFSIZ];

#ifdef	DEBUG
	debug("in push_linedisc");
#endif
	/*
	 * copy modules into buf so we won't mess up the original buffer
	 * because strtok will chop the string
	 */
	p = strcpy(buf,modules);

	while(ioctl(fd, I_POP) >= 0);  /* pop everything */ 
		;
	for (p=(char *)strtok(p,","); p!=(char *)NULL; 
		p=(char *)strtok(NULL,",")) {
		for (tp = p + strlen(p) - 1; tp >= p && isspace(*tp); --tp)
			*tp = '\0';
		if (ioctl(fd, I_PUSH, p) == -1) {
			log(MM_ERROR, ":669:push (%s) on %s failed: %s",
				p, device, strerror(errno));
			return(-1);
		}  
	}
	return(0);
}

/*
 * Procedure:	  hang_up_line
 *
 * Restrictions:
                 ioctl(2): none
		 strerror: none
*/

/*
 *	hang_up_line	- set speed to B0. This will drop DTR
 */
hang_up_line(fd)
int	fd;
{
	struct termio termio;
	static const char name[] = "hang_up_line";

#ifdef	DEBUG
	debug("in hang_up_line");
#endif
	if (ioctl(fd,TCGETA,&termio) < 0) {
		log(MM_ERROR, badioctl, name, "TCGETA", strerror(errno));
		return(-1);
	}
	termio.c_cflag &= ~CBAUD;
	termio.c_cflag |= B0;

	if (ioctl(fd,TCSETA,&termio) < 0) {
		log(MM_ERROR, badioctl, name, "TCSETA", strerror(errno));
		return(-1);
	}
	return(0);
}

/*
 * initial_termio	- set initial termios
 *			- return 0 if successful, -1 if failed.
 */
int
initial_termio(fd,pmptr)
int	fd;
struct	pmtab	*pmptr;
{
	int	ret;
	struct	Gdef *speedef;
	struct	Gdef *get_speed();
	extern	int  auto_termio();

	speedef = get_speed(pmptr->p_ttylabel);
	if (speedef->g_autobaud & A_FLAG) {
		pmptr->p_ttyflags |= A_FLAG;
		if (auto_termio(fd) == -1) {
			/* -close is commented out let calling function close
			**  fd	TP !!!
			**(void)close(fd);
			*/
			return(-1);
		}
	}
	else {
		if (pmptr->p_ttyflags & R_FLAG)
			ret = set_termio(fd,speedef->g_iflags,
				(char *)NULL, TRUE, (long)RAW);
		else 
			ret = set_termio(fd,speedef->g_iflags,
				(char *)NULL, TRUE, (long)CANON);
		if (ret == -1) {
			log(MM_ERROR, ":670:Initial termio on (%s) failed",
				pmptr->p_device);
			/* -close is commented out let calling function close
			**  fd	TP !!!
			**(void)close(fd);
			*/
			return(-1);
		}
	}
	return(0);
}



/*
 * Procedure:	  tpctrl_termio
 *
 * Restrictions:
		 strerror: none
*/

/* tpctrl_termio(ctrlfd, pmptr)
**
** -set termio, on real device via ctrl channel, for detecting SAKs
**
** -If type of SAK is not NONE modify termios characteristics that
**  might prevent SAK detection.
**
** -return 0 if successful
** -return -1 on failure
*/
int
tpctrl_termio(ctrlfd, pmptr)
	int		ctrlfd;
	struct	pmtab	*pmptr;
{
		struct	termio termio;
		struct	termios termios;
		struct	stio stermio;
		struct	termiox termiox;
		struct	winsize winsize;
		struct	winsize owinsize;
		int	i;
		int	term;
		int	cnt = 1;
		int	speed = B1200;
		char	*uarg;	
		char	*argvp[MAXARGS];	/* stty args */
		struct	Gdef *speeddef;
	static	char	*binstty = "/usr/bin/stty";
	static	char	buf[BUFSIZ];
	extern	struct	Gdef *get_speed();
	extern	int	get_ttymode(), set_ttymode();
	extern	char	*sttyparse();


	if ((term = get_ttymode(ctrlfd, &termio, &termios, &stermio, &termiox, &winsize)) < 0){
		log(MM_ERROR, ":868:%s: get_ttymode failed: %s",
			"tpctrl_termio",strerror(errno));
		return (-1);
	}
	owinsize = winsize;

	/*
	** Get General Terminal Interface characteristics from /etc/ttydefs
	** for given speed label and set them in the various termio structures
	*/
	speeddef = get_speed(pmptr->p_ttylabel);
	argvp[0] = binstty;
	(void)strcpy(buf, speeddef->g_iflags);
	mkargv(buf, &argvp[1], &cnt, MAXARGS - 1);
	if ((uarg = sttyparse(cnt, argvp, term, &termio, &termios, &termiox,
	 &winsize)) != NULL) {
		log(MM_ERROR, ":869:%s: sttyparse unknown mode: %s",
		 "tpctrl_termio",uarg);
		return (-1);
	}

	/*
	** The termio characteristics that are modified cover all possible
	** definitions for the SAK
	*/
	if (pmptr->p_sak.sak_type != saktypeNONE){
		/*
		** If SAK is Break, do not ignore Break condition and do
		** not convert Break condition into a SIGINT.
		*/
		termios.c_iflag &= ~(BRKINT|IGNBRK);

		/*
		** If SAK type is CHAR do not map or ignore carriage return
		** and new line.
		*/
		termios.c_iflag &= ~(INLCR|IGNCR|ICRNL);

		termios.c_cflag |= (termios.c_cflag & CSIZE ? 0 : CS8);

		/*
		** If SAK is Line Drop dont have CLOCAL set.
		*/
		termios.c_cflag &= ~(CLOCAL);

		for (i = 0; i < NCCS; i++)
			termios.c_cc[i] = CNUL;
	}

	/* set a default baud rate if one is not defined */
	termios.c_cflag |= (CREAD|HUPCL|(termios.c_cflag & CBAUD ? 0 : speed));

	/*
	** Will not be doing any output or line discipline processing on
	** the ctrl channel.
	*/
	termios.c_oflag = 0;
	termios.c_lflag = 0;

	if (set_ttymode(ctrlfd, term, &termio, &termios, &stermio, &termiox,
	 &winsize, &owinsize) != 0) {
		log(MM_ERROR,
		 ":870:%s: set_ttymode failed: %s", "tpctrl_termio",strerror(errno));
		return (-1);
	}

	return (0);
}
