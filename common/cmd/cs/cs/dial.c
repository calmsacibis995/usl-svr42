/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/dial.c	1.12.2.6"
#ident  "$Header: dial.c 1.2 91/06/26 $"

/*
 *      dial() returns an fd for an open tty-line connected to the
 *      specified remote.  The caller should trap all ways to
 *      terminate, and call undial(). This will release the `lock'
 *      file and return the outgoing line to the system.  This routine
 *      would prefer that the calling routine not use the `alarm()'
 *      system call, nor issue a `signal(SIGALRM, xxx)' call.
 *      If you must, then please save and restore the alarm times.
 *      The sleep() library routine is ok, though.
 *
 *	#include <sys/types.h>
 *	#include <sys/stat.h>
 *      #include "dial.h"
 *
 *      int dial(call);
 *      CALL call;
 *
 *      void undial(rlfd);
 *      int rlfd;
 *
 *      rlfd is the "remote-lne file descriptor" returned from dial.
 *
 *      The CALL structure as (defined in dial.h):
 *
 *      typedef struct {
 *              struct termio *attr;    ptr to term attribute structure
 *              int     baud;           no longer used --
 *					left in for backwards compatibility
 *              int     speed;          212A modem: low=300, high=1200
 *					negative for "Any" speed
 *              char    *line;          device name for out-going line
 *              char    *telno;         ptr to tel-no digit string
 *		int	modem		no longer used --
 *					left in for backwards compatibility
 *		char 	*device		no longer used --
 *					left in for backwards compatibility
 *		int	dev_len		no longer used --
 *					left in for backwards compatibility
 *      } CALL;
 *
 *      The error returns from dial are negative, in the range -1
 *      to -14, and their meanings are:
 *
 *              INTRPT   -1: interrupt occured
 *              D_HUNG   -2: dialer hung (no return from write)
 *              NO_ANS   -3: no answer (caller script failed)
 *              ILL_BD   -4: illegal baud-rate
 *              A_PROB   -5: acu problem (open() failure)
 *              L_PROB   -6: line problem (open() failure)
 *              NO_Ldv   -7: can't open Devices file
 *              DV_NT_A  -8: specified device not available
 *              DV_NT_K  -9: specified device not known
 *              NO_BD_A -10: no device available at requested baud-rate
 *              NO_BD_K -11: no device known at requested baud-rate
 *		DV_NT_E -12: requested speed does not match
 *		BAD_SYS -13: system not in Systems file
 *		CS_PROB -14: could not connect to connection server
 *
 *      Setting attributes in the termio structure indicated in
 *      the `attr' field of the CALL structure before passing the
 *      structure to dial(), will cause those attributes to be set
 *      before the connection is made.  This can be important for
 *      some attributes such as parity and baud.
 *
 *      With an error return (negative value), there will not be
 *      any `lock-file' entry, so no need to call undial().
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <fcntl.h>
#include <dial.h>
#include <netconfig.h>
#include <netdir.h>
#include "uucp.h"
#include "extern.h"
#include "global.h"

extern	char   *Mytype;
extern	char   *Myline;
extern	int	Evenflag;
extern	int	Oddflag;
extern	int	Terminal;
extern	int	line_8bit;

static	int	rlfd;		/* fd for remote comm line */
jmp_buf Sjbuf;			/*needed by connection routines*/

/*VARARGS*/
/*ARGSUSED*/
void
assert(s1,s2,i1,s3,i2)
char *s1, *s2, *s3;
int i1, i2;
{}	/* for ASSERT in conn() */

/*ARGSUSED*/
void
logent(s1,s2)
char *s1, *s2;
{}	/* so we can load unlockf() */

void
cleanup(Cn) 	/*this is executed only in the parent process*/
int Cn;		/*fd for remote comm line */
{
	(void)restline();
	(void)setuid(Euid);
	if(Cn > 0) {
		(void) close(Cn);
	}

	rmlock((char*) NULL);	/*uucp routine in ulockf.c*/	
	return;		/* code=negative for signal causing disconnect*/
}

/* set service so we know which Sysfiles entries to use, then
 * be sure can access Devices file(s).  use "cu" entries ...
 * dial is more like cu than like uucico.
 */
int
dial_auth()
{
char *alt[7];
char speed[10];		/* character value of speed passed to dial */
int  alt_flag;		/* used for "cu -c lanname sysname" command */

	/* set " cu " as service default only if service is not set
	 * in CALL_EXT structure 
	 */
	if (((CALL_EXT	*)Call.device) != NULL) {
		if (((CALL_EXT	*)Call.device)->service != NULL) {
			(void)strcpy(Progname,((CALL_EXT *)Call.device)->service);
		} else {
			(void)strcpy(Progname,"cu");
		}

		if (((CALL_EXT *)Call.device)->class != NULL) {
			Mytype = ((CALL_EXT *)Call.device)->class;
		}
	} else {
		(void)strcpy(Progname,"cu");
	}

	/* this should be done in read_dialrequest() to parallel
	 * read_request() but the default value ("cu") isn't filled
	 * until we get here.
	 */

	Nd_hostserv.h_host = Call.telno;
	Nd_hostserv.h_serv = Progname;

	setservice(Progname);
	if ( sysaccess(EACCESS_DEVICES) != 0 ) {
		/* can't read Devices file(s)	*/
		return(NO_Ldv);
	}

	if (Call.attr != NULL) {
		if ( Call.attr->c_cflag & PARENB ) {
			Evenflag = ((Call.attr->c_cflag & PARODD) ? 0 : 1);
			Oddflag = ((Call.attr->c_cflag & PARODD) ? 1 : 0);
		}
		Terminal = (Call.attr->c_oflag & OPOST ? 1 : 0);
		line_8bit = ((Call.attr->c_cflag & CSIZE) == CS8 ? 1 : 0);
	}

	if (Call.speed <= 0)
		strcpy(speed,"Any");
	else
		sprintf(speed,"%d",Call.speed);

	/* Determine whether contents of "telno" is a system name. */
	 alt_flag = TRUE;/* assume we will use the alternate path */
	if ( (Call.telno != NULL) &&
	     (strlen(Call.telno) != strspn(Call.telno,"0123456789=-*#")) ) {
		/* use conn() for system names */
		if( ( (rlfd = conn(Call.telno)) != -1 ) || !Mytype )
			alt_flag = FALSE; /* Success. Forget alternate path */
	} 
	if(alt_flag) {
		alt[F_NAME] = "dummy";	/* to replace the Systems file fields */
		alt[F_TIME] = "Any";    /* needed for getto(); [F_TYPE] and */
		alt[F_TYPE] = "";	/* [F_PHONE] assignment below       */
		alt[F_CLASS] = speed;
		alt[F_PHONE] = "";
		alt[F_LOGIN] = "";
		alt[6] = "";

		if ( (Call.telno != NULL) && (*Call.telno != '\0') ) {
			/* given a phone number, use an ACU */
			alt[F_PHONE] = Call.telno;
			alt[F_TYPE] = "ACU";
		} else {
			/* otherwise, use a Direct connection */
			alt[F_TYPE] = "Direct";
			/* If device name starts with "/dev/", strip it off  */
			/* since Devices file entries will also be stripped. */
			if ( (Call.line != NULL) &&
				(strncmp(Call.line, "/dev/", 5) == 0) )
				Myline = (Call.line + 5);
			else
				Myline = Call.line;
		}
		if (( (CALL_EXT*)Call.device)->class != NULL)
                        alt[F_TYPE] = ((CALL_EXT*)Call.device)->class;
	
	
		rlfd = getto(alt);
	}

	if ( EQUALS(Nd_hostserv.h_serv,"uucico") ) {
		fixline(rlfd, 0, D_TRANSFER);
	}

	returnfd = rlfd;
	if (rlfd < 0){
		DEBUG(4, "dial_auth: writing fd and dial structure back to client\n", "");
		switch (Uerror) {
			case SS_NO_DEVICE:	return(DV_NT_A);
			case SS_DIAL_FAILED:	return(D_HUNG);
			case SS_LOCKED_DEVICE:	return(DV_NT_A);
			case SS_BADSYSTEM:	return(BAD_SYS);
			case SS_CANT_ACCESS_DEVICE:	return(L_PROB);
			case SS_CHAT_FAILED:	return(NO_ANS);
			case SS_TIME_WRONG:	return(DV_NT_A);
			case SS_DEVICE_FAILED:	return(L_PROB);
			default:	return(CS_PROB);
		}
	}
	return(returnfd);
}

/*
 * undial(fd) 
 */
void
undial(fd)
int fd;
{
	sethup(fd);
	sleep(2);
	cleanup(fd);
}
