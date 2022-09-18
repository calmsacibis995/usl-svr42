/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

#ident	"@(#)wall:common/cmd/wall/wall.c	1.13.2.10"
#ident "$Header: wall.c 1.2 91/03/13 $"

/***************************************************************************
 * Command: wall
 * Inheritable Privileges: P_MACWRITE,P_DACWRITE
 *       Fixed Privileges:
 *
 ***************************************************************************/

#include <signal.h>

char	mesg[3000];

#include <stdio.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <utmp.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <pwd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <locale.h>
#include <priv.h>
#include <sys/euc.h>
#include <getwidth.h>
#include <pfmt.h>
#include "wall.h"

int	entries;
char	*infile;
int	group;
struct	group *pgrp;
char	*grpname;
char	line[MAXNAMLEN+1] = "???";
char	*systm;
long	tloc;
unsigned int usize;
struct	utmp *utmp;
struct	utsname utsn;
char	who[9]	= "???";
static char time_buf[50];
eucwidth_t eucwidth;
#define DATE_FMT	"%a %b %e %H:%M:%S"
#define DATE_FMTID	":541"

#define equal(a,b)		(!strcmp( (a), (b) ))

static const char badopen[] = ":92:Cannot open %s: %s\n";

/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: none
                 pfmt:	none
                 strerror: none
                 open(2): none
                 ttyname: none
                 getpwuid: none
                 fopen: none
                 fgets: none
                 fclose: none
                 cftime: none
                 gettxt: none
 * Notes: ttyname is not restricted because it opens /etc/ttysrch for
 *				reading only. Also it writes only a specific message to /dev/console.
*/

main(argc, argv)
int	argc;
char	*argv[];
{
	int	i=0, fd;
	register	struct utmp *p;
	FILE	*f;
	struct	stat statbuf;
	register	char *ptr;
	struct	passwd *pwd;
	char	*term_name;
	void readargs(), sendmes();

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:wall");

	getwidth(&eucwidth);

	if(uname(&utsn) == -1) {
		pfmt(stderr, MM_ERROR, ":542:uname() failed: %s\n",
			strerror(errno));
		exit(2);
	}
	systm = utsn.nodename;

	if(stat(UTMP_FILE, &statbuf) < 0) {
		pfmt(stderr, MM_ERROR, ":21:Cannot access %s: %s\n", UTMP_FILE,
			strerror(errno));
		exit(1);
	}
	/*
		get usize (an unsigned int) for malloc call
 		and check that there is no truncation (for those 16 bit CPUs)
 	*/
	usize = statbuf.st_size;
	if(usize != statbuf.st_size) {
		pfmt(stderr, MM_ERROR, ":543:'%s' too big.\n", UTMP_FILE);
		exit(1);
	}
	entries = usize / sizeof(struct utmp);
	if((utmp=(struct utmp *)malloc(usize)) == NULL) {
		pfmt(stderr, MM_ERROR,
			":544:Cannot allocate memory for '%s': %s\n",
			UTMP_FILE, strerror(errno));
		exit(1);
	}

	if((fd=open(UTMP_FILE, O_RDONLY)) < 0) {
		pfmt(stderr, MM_ERROR, badopen, UTMP_FILE, strerror(errno));
		exit(1);
	}
	if(read(fd, (char *) utmp, usize) != usize) {
		pfmt(stderr, MM_ERROR, ":205:Cannot read %s: %s\n", UTMP_FILE,
			strerror(errno));
		exit(1);
	}
	close(fd);
	readargs(argc, argv);

	/*
		Get the name of the terminal wall is running from.
	*/

	if ((term_name = ttyname(fileno(stderr))) != NULL)
	/*
		skip the leading "/dev/" in term_name
	*/
		strncpy(line, &term_name[5], sizeof(line) - 1);
	if (who[0] == '?') {
		if (pwd = getpwuid(getuid()))
			strncpy(&who[0],pwd->pw_name,sizeof(who));
	}

	f = stdin;
	if(infile) {
		f = fopen(infile, "r");
		if(f == NULL) {
			pfmt(stderr, MM_ERROR, badopen, infile, strerror(errno));
			exit(1);
		}
	}

	for (ptr = &mesg[0]; (char)(*ptr = getc(f)) != (char) EOF; ptr++) {
		insert_cr(ptr);
		if (ptr > &mesg[2997])
			break;
	}
	*ptr = '\0';

	fclose(f);
	time(&tloc);
	cftime(time_buf, gettxt(DATE_FMTID, DATE_FMT), &tloc);
	for(i=0;i<entries;i++) {
		if((p=(&utmp[i]))->ut_type != USER_PROCESS) continue;
		sendmes(p);
	}
	alarm(60);
	do {
		i = (int)wait((int *)0);
	} while(i != -1 || errno != ECHILD);
	exit(0); /* NOTREACHED */
}

/*
 * Procedure:     sendmes
 *
 * Restrictions:
                 sprintf: none
                 fopen: none
                 pfmt: none
                 strerror: none
                 fprintf: none
                 putc: none
                 fputs: none
                 fflush: none
                 fclose: none
*/
void
sendmes(p)
struct utmp *p;
{
	register i;
	register char *s;
	static char device[] = "/dev/123456789012";
	register char *bp;
	int ibp;
	FILE *f;

	if(group)
		if(!chkgrp(p->ut_user))
			return;
	while((i=(int)fork()) == -1) {
		alarm(60);
		wait((int *)0);
		alarm(0);
	}

	if(i)
		return;

	signal(SIGHUP, SIG_IGN);
	alarm(60);
	s = &device[0];
	sprintf(s,"/dev/%s",&p->ut_line[0]);
#ifdef DEBUG
	f = fopen("wall.debug", "a");
#else
	f = fopen(s, "w");
#endif
	if(f == NULL) {
		pfmt(stderr, MM_ERROR,
			":545:Cannot send to %.-8s on %s: %s\n",
			&p->ut_user[0], s, strerror(errno));
		exit(1);
	}

#ifdef SNI
	if (group)
		pfmt(f, MM_NOSTD, BRCMSGTG, who, line, systm, time_buf,grpname);
	else
		pfmt(f, MM_NOSTD, BRCMSG, who, line, systm, time_buf);
#else
#ifdef i386
	if (group)
		pfmt(f, MM_NOSTD,
			":582:\r\n\07\07\07Broadcast Message from %s (%s) on %s %19.19s to group %s...\r\n",
			who, line, systm, time_buf, grpname);
	else
		pfmt(f, MM_NOSTD,
			":583:\r\n\07\07\07Broadcast Message from %s (%s) on %s %19.19s...\r\n",
			who, line, systm, time_buf);
#else
	if (group)
		pfmt(f, MM_NOSTD,
			":546:\07\07\07Broadcast Message from %s (%s) on %s %19.19s to group %s...\n",
			who, line, systm, time_buf, grpname);
	else
		pfmt(f, MM_NOSTD,
			":547:\07\07\07Broadcast Message from %s (%s) on %s %19.19s...\n",
			who, line, systm, time_buf);
#endif
#endif /* SNI */

#ifdef DEBUG
#ifdef SNI
	fprintf(f, DEBUGMSG, p->ut_user, s);
#else
#ifdef i386
	fprintf(f,"DEBUG: To %.8s on %s\r\n", p->ut_user, s);
#else
	fprintf(f,"DEBUG: To %.8s on %s\n", p->ut_user, s);
#endif
#endif /* SNI */
#endif

	i = strlen(mesg);
	for (bp = mesg; --i >= 0; bp++) {
		ibp = (unsigned int) *bp;
		if (*bp == '\n')
			putc('\r', f);
		if (ISPRINT(ibp, eucwidth) || *bp == '\r' || *bp == '\013' ||
                    *bp == ' ' || *bp == '\t' || *bp == '\n' || *bp == '\007') {
			putc(*bp, f);
		} else {
			if (!isascii(*bp)) {
                                fputs("M-", f);
                                *bp = toascii(*bp);
			}
			if (iscntrl(*bp)) {
                                putc('^', f);
                                putc(*bp + 0100, f);
			}
			else
                                putc(*bp, f);
		}

		if (*bp == '\n')
			fflush(f);

		if (ferror(f) || feof(f)) {
			pfmt(stdout, MM_ERROR, ":548:\n\007Write failed: %s\n",
				strerror(errno));
			exit(1);
		}
	}
	fclose(f);
	exit(0);
}

/*
 * Procedure:     readargs
 *
 * Restrictions:
                 pfmt: none
                 getgrnam: none
*/

void
readargs(ac, av)
int ac;
char **av;
{
	register int i;

	for(i = 1; i < ac; i++) {
		if(equal(av[i], "-g")) {
			if(group) {
				pfmt(stderr, MM_ERROR,
					":549:Only one group allowed\n");
				exit(1);
			}
			i++;
			if((pgrp=getgrnam(grpname= av[i])) == NULL) {
				pfmt(stderr, MM_ERROR,
					":550:Unknown group %s\n", grpname);
				exit(1);
			}
			group++;
		}
		else
			infile = av[i];
	}
}

/*
 * Procedure:     chkgrp
 *
 * Restrictions:  none
*/

#define BLANK		' '
chkgrp(name)
register char *name;
{
	register int i;
	register char *p;

	for(i = 0; pgrp->gr_mem[i] && pgrp->gr_mem[i][0]; i++) {
		for(p=name; *p && *p != BLANK; p++);
		*p = 0;
		if(equal(name, pgrp->gr_mem[i]))
			return(1);
	}

	return(0);
}
