/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)pm_cmds:pmlib.c	1.18.2.2"
#ident  "$Header: pmlib.c 1.2 91/06/27 $"

#include "pdf.h"

char	*strchr();

extern	int	close(),
		lvlin(),
		access(),
		fchown(),
		unlink(),
		lvlfile();

extern	long	strtol();

extern	unsigned long	strtoul();

static	FILE	*pff = NULL;
static	char	line[FP_BSIZ];
static	struct	pdf	pdf;

int	badent;

/*
 * Procedure:	setpfent
 *
 * Notes:	This routine either opens the privilege data file, or
 *		if already open, rewinds to the beginning.
*/
void
setpfent()
{
	if (pff == NULL)
		pff = fopen(PDF, "r");
	else
		rewind(pff);
}

/*
 * Procedure:	endpfent
 *
 * Notes:	This routine close the privilege data file.
*/
void
endpfent()
{
	if (pff != NULL) {
		(void) fclose(pff);
		pff = NULL;
	}
}


/*
 * Procedure:	pfskip
 *
 * Notes:	This routine adjusts the pointer passed to "skip" over
 *		the fields found in the privilege data file.  These
 *		fields are delimited by a ":".
*/
static char *
pfskip(p)
	register char *p;
{
	while(*p && *p != ':' && *p != '\n')
		++p;
	if (*p == '\n')
		*p = '\0';
	else if (*p)
		*p++ = '\0';
	return p;
}


/*
 * Procedure:	getpfent
 *
 * Restictions:
 *		open()	none
 *
 * Notes:	This routine opens the privilege data file (if its closed)
 *		and calls the "fgetpfent()" routine to read a line from
 *		the privilege data file.
*/
struct pdf *
getpfent()
{
	if (pff == NULL) {
		if ((pff = fopen(PDF, "r")) == NULL)
			return NULL;
	}
	return fgetpfent(pff);
}


/*
 * Procedure:	fgetpfent
 *
 * Notes:	This routine is the one that actually does the reading of
 *		the privilege data file.  It calls "pfskip()" to adjust the
 *		pointer.  It also does validity checking and sets the
 *		variable "badent" and returns NULL.
*/
struct pdf *
fgetpfent(f)
	FILE *f;
{
	register char	*p;
		 char	*end;
		 long	x;
	unsigned long	u_x;

	p = fgets(line, sizeof(line), f);
	if (p == NULL)
		return NULL;
	u_x = strtoul(p, &end, 10);	
	if (end != strchr(p, ':')) {
		/* check for numeric value */
		badent = 1;
		return NULL;
	}
	pdf.pf_size = u_x;
	p = pfskip(p);
	x = strtol(p, &end, 10);	
	if (end != strchr(p, ':')) {
		/* check for numeric value */
		badent = 1;
		return NULL;
	}
	if (*p == ':' || p == NULL)
		x = -1;
	pdf.pf_cksum = x;
	p = pfskip(p);
	u_x = strtoul(p, &end, 10);	
	if (end != strchr(p, ':')) {
		/* check for numeric value */
		badent = 1;
		return NULL;
	}
	pdf.pf_validity = u_x;
	p = pfskip(p);
	pdf.pf_privs = p;
	p = pfskip(p);
	pdf.pf_filep = p;
	if (*p == ':') {
		/* check for non-null path name */
		badent = 1;
		return NULL;
	}
	(void) pfskip(p);
	return &pdf;
}


/*
 * Procedure:	putpfent
 *
 * Notes:	format a privilege data file entry
*/
int
putpfent(p, f)
	register const struct pdf *p;
	register FILE *f;
{
	(void) fprintf(f, "%ld:", p->pf_size);
	if (p->pf_cksum >= 0) {
		(void) fprintf(f, "%ld", p->pf_cksum);
	}
	(void) fprintf(f, ":%ld:%s:%s",
		p->pf_validity,
		p->pf_privs,
		p->pf_filep);
	(void) putc('\n', f);
	(void) fflush(f);
	return ferror(f);
}

#define LOCKFILE	"/etc/security/tcb/.prv.lock"
#define	PRVDIR		"/etc/security/tcb"
#define S_WAITTIME	15

static struct flock flock =	{
			0,	/* l_type */
			0,	/* l_whence */
			0,	/* l_start */
			0,	/* l_len */
			0,	/* l_sysid */
			0	/* l_pid */
			} ;

/*
 *	lckprvf() returns a 0 for a successful lock within W_WAITTIME
 *	and -1 otherwise
*/

static int fildes = -1 ;
extern void (*sigset ())() ;
extern unsigned alarm() ;

/*
 * Procedure:	almhdlr
 *
 * Notes:	NULL routine called by lckprvf()
*/
static void
almhdlr()
{
}

/*
 * Procedure:	lckprvf
 *
 * Notes:	This routine sets a lock on the PDF LOCKFILE.
*/
int
lckprvf()
{
	int	retval,
		no_lock = 0;
	level_t	f_lid = 0;
	struct	stat	buf;
	const	char	*_PRVDIR = "/etc/security/tcb",
			*_LOCKFILE = "/etc/security/tcb/.prv.lock";

	/*
	 * set the umask to 464 so that file is writable
	 * only by group.
	*/
	(void) umask(~(S_IRUSR|S_IRGRP|S_IWGRP|S_IROTH));
	/*
	 * check if the MAC feature is installed.
	 * If it isn't, set ``f_lid'' to an invalid LID.
	*/
	if (lvlin("SYS_PRIVATE", &f_lid) < 0) {
		f_lid = 0;
	}
	/*
	 * determine if _LOCKFILE exists.  If it doesn't, set "no_lock"
	 * to 1 so that the stat, chown and lvlfile will be done.
	*/
	if (stat(_LOCKFILE, &buf) < 0) {
		no_lock = 1;
		/*
		 * Only stat the directory to get owner, group and
		 * level if the _LOCKFILE did not exist.
		*/
		if (stat(_PRVDIR, &buf) < 0) {
			return -1; 		/* stat() FAILED! */
		}
	}
	/*
	 * If either of the two stat() call above succeeded and
	 * either returned a valid level, compare that level to
	 * "SYS_PRIVATE".  If they're not equal, set the lock file
	 * level to the level of the successful stat().
	*/
	if (buf.st_level && (buf.st_level != f_lid)) {
		f_lid = buf.st_level;
	}
	/*
	 * the creat() succeeds if MAC is installed and the process
	 * is either at the same level as the _LOCKFILE, or the process
	 * has P_MACWRITE turned on in its working set.
	 *
	 * Regardless of the status of MAC, the process must still pass
	 * the discretionary checks for a successful creat() so the
	 * process may also require P_DACWRITE.
	*/
	if ((fildes = creat(_LOCKFILE, 0060)) == -1)
		return -1;
	else {
		if (no_lock) {
			/*
			 * if the _LOCKFILE didn't exist and MAC is installed,
			 * set the level of the _LOCKFILE to the value in
			 * ``f_lid''.
			*/
			if (f_lid) {
				(void) close(fildes);
				(void) lvlfile(_LOCKFILE, MAC_SET, &f_lid);
				if ((fildes = creat(_LOCKFILE, 0060)) == -1) {
					(void) unlink(_LOCKFILE);
					return -1;
				}
			}
			/*
			 * change the owner and group of the _LOCKFILE
			 * to the _PRVDIR owner.  On failure, remove
			 * the file.
			*/
			if (fchown(fildes, buf.st_uid, buf.st_gid) < 0) {
				(void) unlink(_LOCKFILE);
				return -1;
			}
		}
		flock.l_type = F_WRLCK;
		(void) sigset(SIGALRM, (void (*)())almhdlr);
		(void) alarm(S_WAITTIME);
		retval = fcntl(fildes, F_SETLKW, (int)&flock); 
		(void) alarm(0);
		(void) sigset(SIGALRM, SIG_DFL);
		return retval;
	}
}


/*
 * Procedure:	ulckprvf
 *
 * Notes: 	ulckprvf() returns 0 for a successful unlock and -1 otherwise
*/
int
ulckprvf()
{
	if (fildes == -1) 
		return FAILURE;
	else {
		flock.l_type = F_UNLCK ;
		(void) fcntl(fildes, F_SETLK, (int)&flock) ;
		(void) close(fildes) ;
		fildes = -1;
		return SUCCESS;
	}
}	


/*
 * Procedure:	getcksum
 *
 * Notes:	This routine opens the named file passed as an argument, reads
 *		every byte in the file, and calculates a checksum based on the
 *		sum of all the bytes in the file.
*/
long
getcksum(fp)
	const	char	*fp;
{
	FILE	*in;
	register long cksum = 0;
	int	c;

	if ((in = fopen(fp, "r")) == NULL)
		return FAILURE;

	while((c = getc(in)) != EOF) {
		if (cksum & 01)
			cksum = (cksum >> 1) + 0x8000;
		else
			cksum >>= 1;
		cksum += c;
		cksum &= 0xFFFF;
	}
	(void) fclose(in);		/* close the open file */
	return cksum;
}


/*
 * Procedure:	init_sdefs
 *
 * Notes:	This routine initializes a malloc'ed area that contains
 *		particular information about the privilege mechanism that
 *		is installed.
*/
setdef_t	*
init_sdefs(nsets)
	register int nsets;
{
	setdef_t	*sdefs;

	sdefs = (setdef_t *)malloc(nsets * sizeof(setdef_t));

	if (sdefs) {
		(void) secsys(ES_PRVSETS, (char *)sdefs);
	}

	return sdefs;
}



/*
 * Procedure:	set_supported
 *
 * Notes:	This routine returns a 1 if the privilege set name is
 *		supported by the installed privilege mechanism.  Other-
 *		wise, it returns 0.
*/
int
set_supported(sdefs, nsets, setname, otype)
	setdef_t	*sdefs;
	register int	nsets;
	register char  *setname;
	register ulong	otype;
{
	register int	i;

	for (i = 0; i < nsets; ++i, ++sdefs) {
		if (sdefs->sd_objtype == otype) {
			if (!strcmp(sdefs->sd_name, setname)) {
				return 1;
			}
		}
	}
	return 0;
}
