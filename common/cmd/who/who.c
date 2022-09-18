/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)who:who.c	1.23.2.14"
#ident  "$Header: who.c 1.4 91/08/07 $"
/***************************************************************************
 * Command: who
 *
 * Inheritable Privileges: None
 *       Fixed Privileges: None
 *
 *
 * Notes:	This program analyzes information found in /var/adm/utmp.
 *
 *		Additionally information is gathered from /etc/inittab
 *		if requested.
 *	
 *	
 *		Syntax:
 *	
 *			who am i	Displays info on yourself
 *	
 *			who -a		Displays information about All 
 *					entries in /var/adm/utmp
 *	
 *			who -A		Displays ACCOUNTING information
 *					(non-functional)
 *	
 *			who -b		Displays info on last boot
 *	
 *			who -d		Displays info on DEAD PROCESSES
 *	
 *			who -H		Displays HEADERS for output
 *	
 *			who -l 		Displays info on LOGIN entries
 *	
 *			who -p 		Displays info on PROCESSES spawned
 *					by init
 *	
 *			who -q		Displays short information on
 *					current users who LOGGED ON
 *	
 *			who -r		Displays info of current run-level
 *	
 *			who -s		Displays requested info in SHORT form
 *	
 *			who -t		Displays info on TIME changes
 *	
 *			who -T		Displays writeability of each user
 *					(+ writeable, - non-writeable, ? hung)
 *	
 *			who -u		Displays LONG info on users
 *					who have LOGGED ON
 *
 ***************************************************************************/

#define		DATE_FMT	"%b %e %H:%M"
#define		DATE_FMTID	":734"
/*
 *  %b	Abbreviated month name
 *  %e	Day of month
 *  %H	hour (24-hour clock)
 *  %M  minute
 */
#include	<errno.h>
#include	<fcntl.h>
#include	<stdio.h>
#include	<string.h>
#include	<sys/types.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<sys/stat.h>
#include	<time.h>
#include	<utmp.h>
#include	<locale.h>
#include	<pfmt.h>

static	char	comment[80];	/* holds inittab comment	*/
static	char	*cptr;		/* general purpose char ptr	*/
static	int	filesw = 0;	/* 1 = Alternate file used	*/
static	int	Hopt = 0;	/* 1 = who -H			*/
static	char	*inittab;	/* ptr to inittab contents	*/
static	char	*iinit;		/* index into inittab		*/
static	int	justme = 0;	/* 1 = who am i			*/
static	long	*lptr;		/* general purpose long ptr	*/
static	char	*myname;	/* pointer to invoker's name 	*/
static	char	*mytty;		/* holds device user is on	*/
static	char	nameval[9];	/* holds invoker's name		*/
static	int	number = 8;	/* number of users per -q line	*/
extern	char 	*optarg;	/* for getopt()			*/
static	int	optcnt=0;	/* keeps count of options	*/
extern	int 	optind;		/* for getopt()			*/
static	char	outbuf[BUFSIZ];	/* buffer for output		*/	
static	char	*program;	/* holds name of this program	*/
static	int	qopt = 0;	/* 1 = who -q			*/
static	int	sopt = 0;	/* 1 = who -s 	       		*/
static	struct	stat stbuf;	/* area for stat buffer		*/
static	struct	stat *stbufp;	/* ptr to structure		*/
extern	char 	*sys_errlist[]; /* error msgs for errno    */
static	int	terse = 1;	/* 1 = print terse msgs		*/
static	int	Topt = 0;	/* 1 = who -T			*/
static	time_t	timnow;		/* holds current time		*/
char	timeval[40];		/* holds time of login?		*/
static	int	totlusrs = 0;	/* cntr for users on system	*/
static	int	uopt = 0;	/* 1 = who -u			*/
char	user[80];		/* holds user name		*/
static	struct	utmp *utmpp;	/* pointer for getutent()	*/
static	int	validtype[UTMAXTYPE+1];	/* holds valid types	*/
static	int	wrap;		/* flag to indicate wrap	*/
static	char	time_buf[40];	/* holds date and time string	*/
int initerr = 0;		/* holds which error occurred   */
int err;			/* holds errno in getinittab    */ 

static  const	char
	badread[] = ":341:Read error in %s: %s\n",
	badstat[] = ":5:Cannot access %s: %s\n",
	badopen[] = ":4:Cannot open %s: %s\n";

static	void	dump();
static	void	vt_dump();
static	void	process();
static	void	ck_file();
char	*getinittab();
int	errmsg();

main(argc, argv)
int	argc;
char	**argv;
{
	int	c;
	int	goerr = 0;	/* non-zero indicates cmd error	*/
	int	i;
	int	optsw;		/* switch for while of getopt()	*/

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:who");

	validtype[USER_PROCESS] = 1;
	validtype[EMPTY] = 0;
	stbufp = &stbuf;

	/*
		Strip off path name of this command
	*/
	for (i = strlen(argv[0]); i >= 0 && (c = argv[0][i]) != '/'; --i);
	if (i >= 0) argv[0] += i+1;
	program = argv[0];

	/*
		Buffer stdout for speed
	*/
	setbuf(stdout, outbuf);

	cptr = timeval;	
	
	/*
		Retrieve options specified on command line
	*/
	while ((optsw = getopt(argc, argv, "abdHln:pqrstTu")) != EOF) {
		optcnt++;
		switch(optsw) {
	
			case 'a':
				optcnt += 7;
				validtype[ACCOUNTING] = 1;
				validtype[BOOT_TIME] = 1;
				validtype[DEAD_PROCESS] = 1;
				validtype[LOGIN_PROCESS] = 1;
				validtype[INIT_PROCESS] = 1;
				validtype[RUN_LVL] = 1;
				validtype[OLD_TIME] = 1;
				validtype[NEW_TIME] = 1;
				validtype[USER_PROCESS] = 1;
				uopt = 1;
				Topt = 1;
				Hopt = 1;
				if (!sopt) terse = 0;
				break;
	
			case 'b':
				validtype[BOOT_TIME] = 1;
				if (!uopt) validtype[USER_PROCESS] = 0;
				break;
	
			case 'd':
				validtype[DEAD_PROCESS] = 1;
				if (!sopt) terse = 0;
				if (!uopt) validtype[USER_PROCESS] = 0;
				break;
	
			case 'H':
				optcnt--; /* Don't count Header */
				Hopt = 1;
				break;
	
			case 'l':
				validtype[LOGIN_PROCESS] = 1;
				if (!uopt) validtype[USER_PROCESS] = 0;
				terse = 0;
				break;

			case 'n':
				number = atoi(optarg);
				if (number < 1) {
					pfmt(stderr, MM_ERROR,
						":735:Number of users per line must be at least 1\n");
					exit(1);
				}
				break;

			case 'p':
				validtype[INIT_PROCESS] = 1;
				if (!sopt) terse = 0;
				if (!uopt) validtype[USER_PROCESS] = 0;
				break;
	
			case 'q':
				qopt = 1;
				break;
	
			case 'r':
				validtype[RUN_LVL] = 1;
				terse = 0;
				if (!uopt) validtype[USER_PROCESS] = 0;
				break;
	
			case 's':
				sopt = 1;
				terse = 1;
				break;
	
			case 't':
				validtype[OLD_TIME] = 1;
				validtype[NEW_TIME] = 1;
				if (!uopt) validtype[USER_PROCESS] = 0;
				break;
	
			case 'T':
				Topt = 1;
				/* FALLTHRU */
	
			case 'u':
				uopt = 1;
				validtype[USER_PROCESS] = 1;
				if (!sopt) terse = 0;
				break;
	
			case '?':
				goerr++;
				break;
		}
	}
	

	if (qopt) {
		Hopt=sopt=Topt=uopt=0;
		validtype[EMPTY] = 0;
		validtype[ACCOUNTING] = 0;
		validtype[BOOT_TIME] = 0;
		validtype[DEAD_PROCESS] = 0;
		validtype[LOGIN_PROCESS] = 0;
		validtype[INIT_PROCESS] = 0;
		validtype[RUN_LVL] = 0;
		validtype[OLD_TIME] = 0;
		validtype[NEW_TIME] = 0;
		validtype[USER_PROCESS] = 1;
	}

	if (argc == optind + 1) {
		optcnt++;
		ck_file(argv[optind]);
		utmpname(argv[optind]);
		filesw = 1;
	}

	/*
		Test for 'who am i' or 'who am I'
	*/

	if ((strcmp(argv[1], "am") == 0) && (argv[2][0] == 'i' || argv[2][0] == 'I') && (argv[2][1] == '\0')) {
		if (argc > 3) goerr++;
		justme = 1;
		myname = nameval;
		cuserid(myname);
		if ((mytty = ttyname(fileno(stdin))) == NULL &&
		    (mytty = ttyname(fileno(stdout))) == NULL &&
		    (mytty = ttyname(fileno(stderr))) == NULL) {
			pfmt(stderr, MM_ERROR,
				":750:Must be attached to a terminal for the 'am I' option\n");
			fflush(stderr);
			exit(1);
		} else mytty += 5; /* bump past "/dev/" */
	}

	if (goerr > 0) {
		pfmt(stderr, MM_ACTION, ":889:Usage:\t%s [-abdHlnpqrstTu] [am i] [utmp_like_file]\n", program);
		pfmt(stderr, MM_NOSTD, ":737:a\tall (Abdlprtu options)\n");
		pfmt(stderr, MM_NOSTD, ":738:b\tboot time\n");
		pfmt(stderr, MM_NOSTD, ":739:d\tdead processes\n");
		pfmt(stderr, MM_NOSTD, ":740:H\tprint header\n");
		pfmt(stderr, MM_NOSTD, ":741:l\tlogin processes\n");
		pfmt(stderr, MM_NOSTD, ":742:n #\tspecify number of users per line for -q\n");
		pfmt(stderr, MM_NOSTD, ":743:p\tprocesses other than getty or users\n");
		pfmt(stderr, MM_NOSTD, ":744:q\tquick %s\n", program);
		pfmt(stderr, MM_NOSTD, ":745:r\trun level\n");
		pfmt(stderr, MM_NOSTD, ":746:s\tshort form of %s (no time since last output or pid)\n", program);
		pfmt(stderr, MM_NOSTD, ":747:t\ttime changes\n");
		pfmt(stderr, MM_NOSTD, ":748:T\tstatus of tty (+ writable, - not writable, ? hung)\n");
		pfmt(stderr, MM_NOSTD, ":749:u\tuseful information\n");
		exit(1);
	}

	if (!terse) {
		if (Hopt) pfmt(stdout, MM_NOSTD,
			":751:NAME       LINE         TIME          IDLE    PID  COMMENTS\n");

		timnow = time(0);

		inittab = getinittab();
		iinit = inittab;
	}
	else if (Hopt) pfmt(stdout, MM_NOSTD,
		":753:NAME       LINE         TIME\n");

	process();

	/*
		'who -q' requires EOL upon exit,
		followed by total line
	*/
	if (qopt) pfmt(stdout, MM_NOSTD, ":754:\n# users=%d\n", totlusrs);
	return(0);
}

static void
dump()
{
	char	device[13];
	time_t hr;
	time_t	idle;
	time_t min;
	char	path[20];
	char	pexit;
	char	pterm;
	int	rc;
	char	w;	/* writeability indicator */
	int	id_len;  

	/*
		Get and check user name
	*/
	strncpy(user, utmpp->ut_user, sizeof(utmpp->ut_user));
	user[sizeof(utmpp->ut_user)] = '\0';
	if ((rc = strlen(user)) > 8) user[8]='\0';
	if ((rc = strlen(user)) == 0) strcpy(user, "   .");
	totlusrs++;

	/*
		Do print in 'who -q' format
	*/
	if (qopt) {
		if ((totlusrs - 1)%number == 0 && totlusrs > 1) printf("\n");
		printf("%-8s ", user);
		return;
	}

	pexit = ' ';
	pterm = ' ';

	/*
		Get exit info if applicable
	*/
	if (utmpp->ut_type == RUN_LVL || utmpp->ut_type == DEAD_PROCESS) {
		pterm = utmpp->ut_exit.e_termination;
		pexit = utmpp->ut_exit.e_exit;
	}

	/*
		Massage ut_time field
	*/
	lptr = &utmpp->ut_time;
	cftime(time_buf, gettxt(DATE_FMTID, DATE_FMT), lptr);

	/*
		Get and massage device
	*/
	if ((rc = strlen(utmpp->ut_line)) == 0) strcpy(device, "     .");
	else {
		strncpy(device, utmpp->ut_line, sizeof(utmpp->ut_line));
		device[sizeof(utmpp->ut_line)] = '\0';
	}

	/*
		Get writeability if requested
	*/
	if (Topt && (utmpp->ut_type == USER_PROCESS )) {
		w = '-';
		strcpy(path, "/dev/");
		strncat(path, utmpp->ut_line, sizeof(utmpp->ut_line));
		path[sizeof(utmpp->ut_line)] = '\0';
		if ((rc = stat(path, stbufp)) == -1) w = '?';
		else if (stbufp->st_mode & S_IWGRP) w = '+';
	}
	else w = ' ';

	/*
		Print the TERSE portion of the output
	*/
	printf("%-8s %c %-12s %s", user, w, device, time_buf);

	if (!terse) {
		strcpy(path, "/dev/");
		strncat(path, utmpp->ut_line, sizeof(utmpp->ut_line));
		path[sizeof(utmpp->ut_line)] = '\0';

		/*
			Stat device for idle time
			(Don't complain if you can't)
		*/
		if ((rc = stat(path, stbufp)) != -1) {
			idle = timnow - stbufp->st_mtime;
			hr = idle/3600;
			min = (unsigned)(idle/60)%60;
			if (hr == 0 && min == 0) printf("   .  ");
			else {
				if (hr < 24) pfmt(stdout, MM_NOSTD,
					":755: %2d:%2.2d", hr, min);
				else pfmt(stdout, MM_NOSTD, ":756:  old ");
			}
		}

		/*
			Add PID for verbose output
		*/
		if (utmpp->ut_type != BOOT_TIME && utmpp->ut_type != RUN_LVL && utmpp->ut_type != ACCOUNTING) printf("  %5d", utmpp->ut_pid);

		/*
			Handle /etc/inittab comment
		*/
		if (utmpp->ut_type == DEAD_PROCESS) 
			pfmt(stdout, MM_NOSTD,
				":757:  id=%4.4s term=%-3d exit=%d  ",
				utmpp->ut_id, pterm, pexit);
		else if (utmpp->ut_type != INIT_PROCESS) {
			/*
				Search for each entry in inittab
				string. Keep our place from
				search to search to try and
				minimize the work. Wrap once if needed
				for each entry.
			*/
			wrap = 0;
			/*
				Look for a line beginning with 
				utmpp->ut_id
			*/
			id_len = get_id_len(utmpp->ut_id);
			while (id_len == 0 || (rc = strncmp(utmpp->ut_id, iinit, id_len)) != 0) {
				for (; *iinit != '\n'; iinit++);
				iinit++;

				/*
					Wrap once if necessary to 
					find entry in inittab
				*/
				if (*iinit == '\0') {
					if (!wrap) {
						iinit = inittab;
						wrap = 1;
					}else break;
				}
			}
	
			if (*iinit != '\0') {
				/*
					We found our entry
				*/
				for (iinit++; *iinit != '#' && *iinit != '\n'; iinit++);

				if (*iinit == '#') {
					for (iinit++; *iinit == ' ' || *iinit == '\t'; iinit++);
					for(rc = 0; *iinit != '\n'; iinit++) comment[rc++] = *iinit;
					comment[rc] = '\0';
				}
				else strcpy(comment, " ");

				printf("  %s", comment);
			}
			else iinit = inittab;	/* Reset pointer */
		}
		if (utmpp->ut_type == INIT_PROCESS)
			pfmt(stdout, MM_NOSTD, ":758:  id=%4.4s", utmpp->ut_id);
		
	}

	/*
		Handle RUN_LVL process (If no alt. file - Only one!)
	*/
	if (utmpp->ut_type == RUN_LVL) {
		printf("    %c%5d    %c", pterm, utmpp->ut_pid, pexit);
		if (optcnt == 1 && !validtype[USER_PROCESS]) {
			printf("\n");
			exit(0);
		}
	}

	/*
		Handle BOOT_TIME process (If no alt. file - Only one!)
	*/
	if (utmpp->ut_type == BOOT_TIME) {
		if (optcnt == 1 && !validtype[USER_PROCESS]) {
			printf("\n");
			exit(0);
		}
	}

	/*
		Now, put on the trailing EOL
	*/
	printf("\n");
	return;
}

static void
vt_dump()
{
	char	device[13];
	time_t hr;
	time_t	idle;
	time_t min;
	char	path[20];
	char	pexit;
	char	pterm;
	int	rc;
	char	w;	/* writeability indicator */
	int	id_len;

	/*
		Get and check user name
	*/
	strncpy(user, utmpp->ut_user, sizeof(utmpp->ut_user));
	user[sizeof(utmpp->ut_user)] = '\0';
	if ((rc = strlen(user)) > 8) user[8]='\0';
	if ((rc = strlen(user)) == 0) strcpy(user, "   .");
	totlusrs++;

	/*
		Do print in 'who -q' format
	*/
	if (qopt) {
		if ((totlusrs - 1)%number == 0 && totlusrs > 1) printf("\n");
		printf("%-8s ", user);
		return;
	}

	pexit = ' ';
	pterm = ' ';

	/*
		Get exit info if applicable
	*/
	if (utmpp->ut_type == RUN_LVL || utmpp->ut_type == DEAD_PROCESS) {
		pterm = utmpp->ut_exit.e_termination;
		pexit = utmpp->ut_exit.e_exit;
	}

	/*
		Massage ut_time field
	*/
	lptr = &utmpp->ut_time;
	cftime(time_buf, gettxt(DATE_FMTID, DATE_FMT), lptr);

	/*
		Get and massage device
	*/
	if ((rc = strlen(mytty)) == 0) strcpy(device, "     .");
	else strcpy(device, mytty);

	/*
		Get writeability if requested
	*/
	if (Topt && (utmpp->ut_type == USER_PROCESS)) { 
		w = '-';
		strcpy(path, "/dev/");
		strcat(path, mytty);

		if ((rc = stat(path, stbufp)) == -1) w = '?';
		else if (stbufp->st_mode & S_IWGRP) w = '+';
	}
	else w = ' ';

	/*
		Print the TERSE portion of the output
	*/
	printf("%-8s %c %-12s %s", user, w, device, time_buf);

	if (!terse) {
		strcpy(path, "/dev/");
		strcat(path, mytty);

		/*
			Stat device for idle time
			(Don't complain if you can't)
		*/
		if ((rc = stat(path, stbufp)) != -1) {
			idle = timnow - stbufp->st_mtime;
			hr = idle/3600;
			min = (unsigned)(idle/60)%60;
			if (hr == 0 && min == 0) printf("   .  ");
			else {
				if (hr < 24) pfmt(stdout, MM_NOSTD,
					":755: %2d:%2.2d", hr, min);
				else pfmt(stdout, MM_NOSTD, ":756:  old ");
			}
		}

		/*
			Add PID for verbose output
		*/
		if (utmpp->ut_type != BOOT_TIME && utmpp->ut_type != RUN_LVL && utmpp->ut_type != ACCOUNTING) printf("  %5d", utmpp->ut_pid);

		/*
			Handle /etc/inittab comment
		*/
		if (utmpp->ut_type == DEAD_PROCESS) 
			pfmt(stdout, MM_NOSTD,
				":757:  id=%4.4s term=%-3d exit=%d  ",
				utmpp->ut_id, pterm, pexit);
		else if (utmpp->ut_type != INIT_PROCESS) {
			/*
				Search for each entry in inittab
				string. Keep our place from
				search to search to try and
				minimize the work. Wrap once if needed
				for each entry.
			*/
			wrap = 0;
			/*
				Look for a line beginning with 
				utmpp->ut_id
			*/
			id_len = get_id_len(utmpp->ut_id);
			while (id_len == 0 || (rc = strncmp(utmpp->ut_id, iinit, id_len)) != 0) {
				for (; *iinit != '\n'; iinit++);
				iinit++;

				/*
					Wrap once if necessary to 
					find entry in inittab
				*/
				if (*iinit == '\0') {
					if (!wrap) {
						iinit = inittab;
						wrap = 1;
					}else break;
				}
			}
	
			if (*iinit != '\0') {
				/*
					We found our entry
				*/
				for (iinit++; *iinit != '#' && *iinit != '\n'; iinit++);

				if (*iinit == '#') {
					for (iinit++; *iinit == ' ' || *iinit == '\t'; iinit++);
					for(rc = 0; *iinit != '\n'; iinit++) comment[rc++] = *iinit;
					comment[rc] = '\0';
				}
				else strcpy(comment, " ");

				printf("  %s", comment);
			}
			else iinit = inittab;	/* Reset pointer */
		}
		if (utmpp->ut_type == INIT_PROCESS)
			pfmt(stdout, MM_NOSTD, ":758:  id=%4.4s", utmpp->ut_id);
		
	}

	/*
		Handle RUN_LVL process (If no alt. file - Only one!)
	*/
	if (utmpp->ut_type == RUN_LVL) {
		printf("    %c%5d    %c", pterm, utmpp->ut_pid, pexit);
		if (optcnt == 1 && !validtype[USER_PROCESS]) {
			printf("\n");
			exit(0);
		}
	}

	/*
		Handle BOOT_TIME process (If no alt. file - Only one!)
	*/
	if (utmpp->ut_type == BOOT_TIME) {
		if (optcnt == 1 && !validtype[USER_PROCESS]) {
			printf("\n");
			exit(0);
		}
	}

	/*
		Now, put on the trailing EOL
	*/
	printf("\n");
	return;
}

static void
process()
{
	int	rc;

	/*
		Loop over each entry in /var/adm/utmp
	*/
	while ((utmpp = getutent()) != NULL) {
#ifdef DEBUG
	printf("ut_user '%s'\nut_id '%s'\nut_line '%s'\nut_type '%d'\n\n", utmpp->ut_user, utmpp->ut_id, utmpp->ut_line,utmpp->ut_type);
#endif
		if (utmpp->ut_type <= UTMAXTYPE) {
			/*
				Handle "am i"
			*/
			if (justme) {
				if (!strncmp(myname, utmpp->ut_user, sizeof(utmpp->ut_user)) && 
				    !strncmp(mytty, utmpp->ut_line, sizeof(utmpp->ut_user))) {
					dump();
					exit(0);
				}
				continue;
			}
	
			/*
				Print the line if we want it
			*/
			if (validtype[utmpp->ut_type]) dump();
		}
		else pfmt(stderr, MM_ERROR,
			":759:Entry has ut_type of %d when maximum is %d\n",
			utmpp->ut_type, UTMAXTYPE);
	}


	if (justme) {
		/*
		 * If justme and got here, must be a vt.
		 * Just try for name match this time.
		 * Assuming utmp hasn't been updated with vt name.
		 */

		setutent();

		while ((utmpp = getutent()) != NULL) {
			if (utmpp->ut_type <= UTMAXTYPE) {
				if (!strncmp(myname, utmpp->ut_user, sizeof(utmpp->ut_user))){
					vt_dump();
					exit(0);
				}
				continue;
	
			}
		}
	}
}

/*
	This routine checks the following:

	1.	File exists

	2.	We have read permissions

	3.	It is a multiple of utmp entries in size

	Failing any of these conditions causes who(1) to
	abort processing.

	4.	If file is empty we exit right away as there
		is no info to report on.
*/

static void
ck_file(name)
char	*name;
{
	FILE	*file;
	struct	stat sbuf;
	int	rc;

	/*
		Does file exist? Do stat to check, and save structure
		so that we can check on the file's size later on.
	*/
	if ((rc = stat(name,&sbuf)) == -1) {
		pfmt(stderr, MM_ERROR, badstat, name, strerror(errno));
		exit(1);
	}

	/*
		The only real way we can be sure we can access the
		file is to try. If we succeed then we close it.
	*/
	if ((file = fopen(name,"r")) == NULL) {
		pfmt(stderr, MM_ERROR, badopen, name, strerror(errno));
		exit(1);
	}
	fclose(file);

	/*
		If the file is empty, we are all done.
	*/
	if (!sbuf.st_size) exit(0);

	/*
		Make sure the file is a utmp file.
		We can only check for size being a multiple of
		utmp structures in length.
	*/
	rc = sbuf.st_size % sizeof(struct utmp);
	if (rc) {
		pfmt(stderr, MM_ERROR, ":760:File '%s' is not a utmp file\n",
			name);
		exit(1);
	}
}

/* get the length of the ut_id */
get_id_len(ut_id)
register char *ut_id;
{
	register int len;
	register int len_sz;

	len_sz = sizeof(ut_id);
	for (len = 0; len <  len_sz; len++)
	   if (*ut_id)
		ut_id++;
		
	    else
		break;
	return(len);
}
 /* getinittab()
 *	Reads the contents of the /etc/inittab file and put them into
 *      the allocated inittab area for later use.  If access on /etc/inittab
 *	fails, try reading alternate file /etc/conf/init.d/kernel.
 */




char *
getinittab()
{
	int	fildes;			/* file descriptor for inittab       */
	int	rc;			/* function return codes  	     */
	int	initbase = 0;		/* 1 = alternate inittab 	     */
	char	*inittab;		/* ptr to inittab contents	     */
	struct	stat	stbuf;		/* area for stat buffer   	     */


	if ((rc = stat("/etc/inittab", &stbuf)) == -1) {
		err = errno;
		initerr = 1;
		initbase = 1;
	 } else if ((fildes = open("/etc/inittab", O_RDONLY)) == -1) {
		err = errno;
		initerr = 2;
		initbase = 1;
	 } else if (stbuf.st_size == 0 ){
		err = errno;
		initerr = 3;
		initbase = 1;
	 } else if ((inittab = malloc(stbuf.st_size + 1)) == NULL) {
		pfmt(stderr, MM_ERROR,
			":752:Cannot allocate %d bytes: %s\n",
			stbuf.st_size, strerror(errno));
		exit(errno);
	}else if ((rc=read(fildes, inittab, stbuf.st_size)) != stbuf.st_size) {
		err = errno;
		initerr = 4;
		free(inittab);
		initbase = 1;
	}

	if (initbase) {
		if ((rc = stat("/etc/conf/init.d/kernel", &stbuf)) == -1) {
			exit(errmsg());
		}
		if ((fildes = open("/etc/conf/init.d/kernel", O_RDONLY)) == -1) 
			exit(errmsg());
		if (stbuf.st_size == 0)
			exit(errmsg());
		if ((inittab = malloc(stbuf.st_size + 1)) == NULL) {
			pfmt(stderr, MM_ERROR,
				":752:Cannot allocate %d bytes: %s\n",
				stbuf.st_size, strerror(errno));
			exit(errno);
		}
		if ((rc=read(fildes, inittab, stbuf.st_size)) != stbuf.st_size){
			free(inittab);
			exit(errno);
		}
	}

	inittab[stbuf.st_size] = '\0';
	return (inittab);
}

/* errmsg () -							*/
/*	print out error message regarding access to /etc/inttab */
/* and return with the errno stored in err.			*/

int
errmsg() {
	switch (initerr) {
		case 1:	
			pfmt(stderr, MM_ERROR, badstat,
				"/etc/inittab", strerror(errno));
			return(err);

		case 2:
			pfmt(stderr, MM_ERROR, badopen,
				"/etc/inittab", strerror(errno));
			return(err);

		case 3:
			pfmt(stderr, MM_ERROR|MM_NOGET,
				"/etc/inittab is zero length\n");
			return(err);
		case 4:
			pfmt(stderr, MM_ERROR, badread,
				"/etc/inittab", strerror(errno));
 			return(1);
	}

}
