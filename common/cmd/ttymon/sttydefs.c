/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/sttydefs.c	1.11.12.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/ttymon/sttydefs.c,v 1.1 91/02/28 20:16:05 ccs Exp $"

/***************************************************************************
 * Command: sttydefs
 *
 * Inheritable Privileges: P_MACWRITE,P_OWNER,P_SETFLEVEL
 *       Fixed Privileges: None
 *
 * Notes: sttydefs(1m) - add, remove or check entries in /etc/ttydefs
 *	  sttydefs(1m) mainly needs P_MACWRITE:
 *	  1. to write to /etc/ttydefs when adding an entry bec. the
 *	     file is at SYS_PUBLIC.
 * 	  2. to create temp file, to unlink the original /etc/ttydefs,
 *	     and to rename temp file bec. the /etc directory is at
 *	     SYS_PUBLIC.
 *	  There is no privilege required to just display /etc/ttydefs.
 *	  The administrator do need to pass DAC restriction by being in
 *	  the group "sys" for options "-r" or "-a".
 *
 *	  There is no need for restriction in the code because each
 *	  library routine/system call which CAN use P_MACWRITE needs it
 *	  to operate correctly.
 *
 *    The owner privilege is necessary for the chown system call in an
 *    RSTCHOWN kernel.
 * Usage:   sttydefs -a ttylabel [-n nextlabel] [-i initail-flags] 
 *	    	     [-f final-flags] [-b]
 *	    sttydefs -r ttylabel
 *	    sttydefs -l [ttylabel]
 *
 ***************************************************************************/

# include <stdio.h>
# include <unistd.h>
# include <stdlib.h>
# include <errno.h>
# include <sys/types.h>
# include <ctype.h>
# include <string.h>
# include <termio.h>
# include <sys/stat.h>
# include <signal.h>
# include <sys/termios.h>
# include <sys/stream.h>
# include <sys/tp.h>
# include <sys/mac.h>
# include <locale.h>
# include <pfmt.h>
# include "tmstruct.h"
# include "ttymon.h"

static	int  nflg = 0;		/* -n seen */
static	int  iflg = 0;		/* -i seen */
static	int  fflg = 0;		/* -f seen */
static	int  lflg = 0;		/* -l seen */

struct  Gdef Gdef[MAXDEFS];	/* array to hold entries in /etc/ttydefs */
int	Ndefs = 0;		/* highest index to Gdef that was used   */
char	Scratch[BUFSIZ];
static	struct Gdef default_setting = {		/* default terminal settings	*/
	"default",
	"9600",
	"9600 sane",
	0,
	/* 
	 * next label is set to 4800 so we can start searching ttydefs.
	 * if 4800 is not in ttydefs, we will loop back to use default_setting 
	 */
	"4800"
};


extern	void	log();

static	void	usage();
static	void	check_ref();
static	void	add_entry();
static	void	remove_entry();
static	int	copy_file();
static	int	verify();
static	int	replace();
static	FILE	*open_temp();
extern  void	read_ttydefs();
extern  int	check_version();
extern  int	find_label();

const char
	badopen[] = 	":4:Cannot open %s: %s\n",
	badversion[] = 	":591:%s version number is incorrect or missing.\n",
	errortmp[] = 	":592:Error accessing temp file.\n",
	badnext[] = 	":593:nextlabel <%s> of <%s> does not reference any existing ttylabel.\n";


/*
 * Procedure:     Msttydefs
 *
 * Restrictions:
                 setlocale: None
                 getopt: None
                 check_version: None
                 printf: None
                 _flsbuf: None
 * Notes - process options & call appropriate sub_routines.
*/

main(argc, argv)
int argc;
char *argv[];
{
	int c;			/* option letter */
	int errflg = 0;		/* error indicator */
	int  aflg = 0;		/* -a seen */
	int  bflg = 0;		/* -b seen */
	int	ret;
	const	char	*argtmp;
	char	*nextlabel;
	struct Gdef ttydef, *ptr;

	extern	char	*optarg;
	extern	int	optind;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:sttydefs");

	if (argc == 1)
		usage(1);
	ptr = &ttydef;
	ptr->g_autobaud = 0;
	while ((c = getopt(argc, argv, "a:n:i:f:br:l")) != -1) {
		switch (c) {
		case 'a':
			aflg = TRUE;
			ptr->g_id = optarg;
			break;
		case 'n':
			nflg = TRUE;
			ptr->g_nextid = optarg;
			break;
		case 'i':
			iflg = TRUE;
			ptr->g_iflags = optarg;
			break;
		case 'f':
			fflg = TRUE;
			ptr->g_fflags = optarg;
			break;
		case 'b':
			bflg = TRUE;
			ptr->g_autobaud |= A_FLAG;
			break;
		case 'r':
			if ((argc > 3) || (optind < argc))
				usage(1);
			remove_entry(optarg);
			break;
		case 'l':
			lflg = TRUE;
			if (argc > 3) 
				usage(1);
			if ((ret = check_version(TTYDEFS_VERS, TTYDEFS)) != 0) {
				if (ret != 2) {
					(void)pfmt(stderr, MM_ERROR, badversion, TTYDEFS);
					exit(1);
				}
				(void)pfmt(stderr, MM_ERROR, badopen, TTYDEFS,
					strerror(errno));
				exit(1);
			}
			if (argv[optind] == NULL) {
				read_ttydefs(NULL,TRUE);
				printf("\n");
				check_ref();
			}
			else {
				if (argc == 3) { /* -l ttylabel */
					if (verify(argv[optind],0) != 0) {
						errflg++;
						break;
					}
					argtmp = argv[optind];
				}
				else { /* -lttylabel */
					argtmp = argv[optind]+2;
				}
				read_ttydefs(argtmp,TRUE);
				if (Ndefs == 0) {
					(void)pfmt(stderr, MM_ERROR,
					":594:ttylabel <%s> not found.\n", argtmp);
					exit(1);
				}
				nextlabel = Gdef[--Ndefs].g_nextid;
				Ndefs = 0;
				read_ttydefs(nextlabel,FALSE);
				if (Ndefs == 0) {
					putc('\n', stderr);
					(void)pfmt(stderr,  MM_WARNING, badnext,
						nextlabel, argtmp);
				}
			}
			exit(0);
			break;		/*NOTREACHED*/
		case '?':
			errflg++;
			break;
		} /* end switch */
		if (errflg) 
			usage(0);
	} /* end while */
	if (optind < argc) 
		usage(1);

	if (aflg) {
		add_entry(ptr); 	/* never return */
	}
	if ((iflg) || (fflg) || (bflg) || (nflg)) 
		usage(1);
	exit(0); 
	/* NOTREACHED */
}

/*
 * Procedure: verify	- to check if arg is valid
 *			- i.e. arg cannot start with '-' and
 *			  arg must not longer than maxarglen
 *			- return 0 if ok. Otherwise return -1
 */
static	int
verify(arg,maxarglen)
char	*arg;
int	maxarglen;
{
	if (*arg == '-') {
		(void)pfmt(stderr, MM_ERROR, ":595:Invalid argument -- %s.\n", arg);
		return(-1);
	}
	if ((maxarglen) && ((int)strlen(arg) > maxarglen)) {
		arg[maxarglen] = '\0';
		(void)pfmt(stderr, MM_ERROR, ":596:String too long, truncated to %s.\n",arg);
		return(-1);
	}
	return(0);
}

/*
 * Procedure: usage - print out a usage message
 */

static	void
usage(complain)
int complain;
{
	if (complain)
		(void)pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
	(void)pfmt(stderr, MM_ACTION, ":597:Usage:\n\tsttydefs -a ttylabel [-n nextlabel] [-i initial-flags]\n\t\t [-f final-flags] [-b]\n");
	(void)pfmt(stderr, MM_NOSTD, ":598:\tsttydefs -r ttylabel\n");
	(void)pfmt(stderr, MM_NOSTD, ":599:\tsttydefs -l [ttylabel]\n");
	exit(2);
}

/*
 * Procedure:     add_entry
 *
 * Restrictions:
                 fopen: None
                 check_version: None
                 fclose: None
                 fprintf: None
 * Notes - add an entry to /etc/ttydefs
 */

static	void
add_entry(ttydef)
struct Gdef *ttydef;
{
	FILE *fp;
	int	errflg = 0;
	char tbuf[BUFSIZ], *tp;
	int  add_version = FALSE;
	extern	int	check_flags();

	tp = tbuf;
	*tp = '\0';
	if ((fp = fopen(TTYDEFS, "r")) != NULL) {
		if (check_version(TTYDEFS_VERS, TTYDEFS) != 0) {
			(void)pfmt(stderr, MM_ERROR, badversion, TTYDEFS);
			exit(1);
		}
		if (find_label(fp,ttydef->g_id)) {
			(void)fclose(fp);
			(void)pfmt(stderr, MM_ERROR,
			":600:Invalid request -- ttylabel <%s> already exists.\n",
			ttydef->g_id);
			exit(1);
		}	
		(void)fclose(fp);
	}
	else  {
		add_version = TRUE;
	}
	if ((fp = fopen(TTYDEFS, "a+")) == NULL) {
		(void)pfmt(stderr, MM_ERROR, badopen, TTYDEFS, strerror(errno));
		exit(1);
	}

	if (add_version) {
	   (void)fprintf(fp,"# VERSION=%d\n", TTYDEFS_VERS);
	}


	/* if optional fields are not provided, set to default */
	if (!iflg)
		ttydef->g_iflags = default_setting.g_iflags;
	else
		if (check_flags(ttydef->g_iflags) != 0 )
			errflg++;
	if (!fflg)
		ttydef->g_fflags = default_setting.g_fflags;
	else
		if (check_flags(ttydef->g_fflags) != 0 )
			errflg++;
	if (errflg)
		exit(1);

	if (!nflg)
		ttydef->g_nextid = ttydef->g_id;

	if (ttydef->g_autobaud & A_FLAG)  {
	   (void)fprintf(fp,"%s:%s:%s:A:%s\n", ttydef->g_id, ttydef->g_iflags,
		ttydef->g_fflags, ttydef->g_nextid);
	}
	else {
	   (void)fprintf(fp,"%s:%s:%s::%s\n", ttydef->g_id, ttydef->g_iflags,
		ttydef->g_fflags, ttydef->g_nextid);
	}
	(void)fclose(fp);
	exit(0);
}

/*
 * Procedure:     remove_entry
 *
 * Restrictions:
                 fopen: None
                 check_version: None
                 fprintf: None
                 fclose: None
                 unlink(2): None
 *
 * Notes - remove entry from /etc/ttydefs
 *
 *     creates a temp file, copy over information from original
 *	   file minus the entry, and then rename temp file back to
 *	   /etc/ttydefs.
 */
static	void
remove_entry(ttylabel)
char	*ttylabel;
{
	FILE *tfp;		/* file pointer for temp file */
	int line;		/* line number entry is on */
	FILE *fp;		/* scratch file pointer */
	char *tname = "/etc/.ttydefs";	/* temp file for /etc/ttydefs */

	fp = fopen(TTYDEFS, "r");
	if (fp == NULL) {
		(void)pfmt(stderr, MM_ERROR, badopen, TTYDEFS, strerror(errno));
		exit(1);
	}
	if (check_version(TTYDEFS_VERS, TTYDEFS) != 0) {
		(void)pfmt(stderr, MM_ERROR, badversion, TTYDEFS);
		exit(1);
	}
	if ((line = find_label(fp, ttylabel)) == 0) {
		(void)pfmt(stderr, MM_ERROR,
		":601:Invalid request -- ttylabel <%s> does not exist.\n", ttylabel);
		exit(1);
	}
	tfp = open_temp(tname);
	if (line != 1) 
		if (copy_file(fp, tfp, 1, line - 1)) {
			(void)pfmt(stderr, MM_ERROR, errortmp);
			exit(1);
		}
	if (copy_file(fp, tfp, line + 1, -1)) {
		(void)pfmt(stderr, MM_ERROR, errortmp);
		exit(1);
	}
	(void)fclose(fp);
	if (fclose(tfp) == EOF) {
		(void)unlink(tname);
		(void)pfmt(stderr, MM_ERROR, errortmp);
		exit(1);
	}
	if (replace(TTYDEFS, tname) != 0 ) {
		exit(1);
	}
	exit(0);
}

/*
 * Procedure:     open_temp
 *
 * Restrictions:
                 access(2): None
                 fopen: None

 *
 * Notes:  open up a temp file
 *
 *	args:	tname - temp file name
 */

static	FILE *
open_temp(tname)
char *tname;
{
	FILE *fp;			/* fp associated with tname */
	struct sigaction sigact;	/* for signal handling */

	sigact.sa_flags = 0;
	sigact.sa_handler = SIG_IGN;
	(void) sigemptyset(&sigact.sa_mask);
	(void) sigaddset(&sigact.sa_mask, SIGHUP);
	(void) sigaddset(&sigact.sa_mask, SIGINT);
	(void) sigaddset(&sigact.sa_mask, SIGQUIT);
	(void) sigaction(SIGHUP, &sigact, NULL);
	(void) sigaction(SIGINT, &sigact, NULL);
	(void) sigaction(SIGQUIT, &sigact, NULL);
	(void)umask(0333);
	if (access(tname, 0) != -1) {
		(void)pfmt(stderr, MM_ERROR, ":603:Tempfile busy\n");
		(void)pfmt(stderr, MM_ACTION, ":364:Try again later.\n");
		exit(1);
	}
	fp = fopen(tname, "w");
	if (fp == NULL) {
		pfmt(stderr, MM_ERROR, ":424:Cannot create temp file: %s\n", strerror(errno));
		exit(1);
	}
	return(fp);
}

/*
 * Procedure:     copy_file
 *
 * Restrictions:
                 rewind: None
                 fgets: None
                 fputs: None
 * Notes: copy information from one file to another
 *
 * Return codes: return 0 on success, -1 on failure
 *
 *	args:	fp - source file's file pointer
 *		tfp - destination file's file pointer
 *		start - starting line number
 *		finish - ending line number (-1 indicates entire file)
 */


static	int
copy_file(fp, tfp, start, finish)
FILE *fp;
FILE *tfp;
register int start;
register int finish;
{
	register int i;		/* loop variable */
	char dummy[BUFSIZ];	/* scratch buffer */

/*
 * always start from the beginning because line numbers are absolute
 */

	rewind(fp);

/*
 * get to the starting point of interest
 */

	if (start != 1) {
		for (i = 1; i < start; i++)
			if (!fgets(dummy, BUFSIZ, fp))
				return(-1);
	}

/*
 * copy as much as was requested
 */

	if (finish != -1) {
		for (i = start; i <= finish; i++) {
			if (!fgets(dummy, BUFSIZ, fp))
				return(-1);
			if (fputs(dummy, tfp) == EOF)
				return(-1);
		}
	}
	else {
		for (;;) {
			if (fgets(dummy, BUFSIZ, fp) == NULL) {
				if (feof(fp))
					break;
				else
					return(-1);
			}
			if (fputs(dummy, tfp) == EOF)
				return(-1);
		}
	}
	return(0);
}

/*
 * Procedure: check_ref	- to check if nextlabel are referencing
 *			  existing ttylabel
 */
static	void
check_ref()
{
	int	i;
	struct	Gdef	*np;
	extern	struct	Gdef	*find_def();
	np = &Gdef[0];
	for (i = 0; i < Ndefs; i++, np++) {
		if (find_def(np->g_nextid) == NULL) {
			(void)pfmt(stderr, MM_WARNING, badnext,	np->g_nextid, np->g_id);
		}
	}
	return;
}

/*
 * Procedure: log - print a message to stdout
 */

void
log(sev, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9)
int	sev;
char	*msg, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	FILE *ofile = lflg ? stdout : stderr;
	pfmt(ofile, sev, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	putc('\n', ofile);
}

#ifdef DEBUG
/*
 *	dlog	- print a debugging message to stdout
 */

void
dlog(msg, a1, a2, a3, a4, a5, a6, a7, a8, a9)
char	*msg, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
{
	FILE *ofile = lflg ? stdout : stderr;
	pfmt(ofile, MM_INFO|MM_NOGET, msg, a1, a2, a3, a4, a5, a6, a7, a8, a9);
	putc('\n', ofile);
}
#endif


/*
 * Procedure:     replace
 *
 * Restrictions:
                 lvlfile(2): None!
                 unlink(2): None! 
                 chmod(2): None!
                 chown(2): None!
                 rename(2): None!
 * Procedure: replace - replace one file with another
 *
 * Notes: replace() resets the original
 *	  file's attributes after the rename if the original file
 *	  exists.
 *
 * Args:  fname - full path name of target file
 *	  tname - full path name of source file
 */


static	int
replace(fname, tname)
char *fname;
char *tname;
{
	struct stat attr;	/* buffer for attributes */
	int attr_ret = 0;	/* found atttributes */
	level_t level;		/* MAC level identifier */
	
	/* get attributes if original file exists */ 
	if ( stat(fname, &attr) == 0 ) 
		attr_ret = 1;
	
	/*
	 * set to original mode & ownership
	 * set level only if attr.st_level has a valid value
	 */
	if ( attr_ret ) {
                if ( attr.st_level != 0 ) {
			level = attr.st_level;
			if ( lvlfile(tname, MAC_SET, &level) != 0) {
				(void)pfmt(stderr, MM_ERROR, 
				":844:could not set level on %s: %s\n", 
				fname, strerror(errno));
				(void) unlink(tname);
				return(-1);
			}
		}
		if (chmod(tname, attr.st_mode) != 0 ||
		    chown(tname, attr.st_uid, attr.st_gid) != 0) {
			(void)pfmt(stderr, MM_ERROR, 
			":845:could not set attributes on %s: %s\n", 
			fname, strerror(errno));
			(void) unlink(tname);
			return(-1);
		}
		
	}

	if ( rename(tname, fname) < 0) {
		(void) unlink(tname);
		(void)pfmt(stderr, MM_ERROR, 
		":602:%s failed: %s\n", "rename()", strerror(errno));
		return(-1);
	}
	return(0);
}

