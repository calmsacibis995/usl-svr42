/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mailx:main.c	1.16.2.6"
#ident "@(#)main.c	1.20 'attmail mail(1) command'"
/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "rcv.h"
#ifndef preSVr4
#include <locale.h>
# ifdef SVR4ES
#  include <mac.h>
# endif
#endif

/*
 * mailx -- a modified version of a University of California at Berkeley
 *	mail program
 *
 * Startup -- interface with user.
 */

static void		hdrstop ARGS((int));

static jmp_buf	hdrjmp;

/*
 * Find out who the user is, copy his mail file (if exists) into
 * /tmp/Rxxxxx and set up the message pointers.  Then, print out the
 * message headers and read user commands.
 *
 * Command line syntax:
 *	mailx [ -i ] [ -r address ] [ -h number ] [ -f [ name ] ]
 * or:
 *	mailx [ -i ] [ -r address ] [ -h number ] people ...
 * or:
 *	mailx -t
 *
 * and a bunch of other options.
 */

main(argc, argv)
	char **argv;
{
	register char *ef = NOSTR;
	register int argp = -1;
	int mustsend = 0, goerr = 0;
	void (*prevint)();
#ifdef USE_TERMIOS
	struct termios tbuf;
#else
	struct termio tbuf;
#endif
	int c;
	char *cwd = NOSTR, *mf;

	/*
	 * Set up a reasonable environment.
	 * Figure out whether we are being run interactively, set up
	 * all the temporary files, buffer standard output, and so forth.
	 */

#ifndef preSVr4
	(void)setlocale(LC_ALL, "");
# ifdef SVR4ES
	(void)setcat("uxemail");
	(void)setlabel("UX:mailx");
	(void)mldmode(MLD_VIRT);
	getwidth(&wp);
	wp._eucw2++;
	wp._eucw3++;
	maxeucw = wp._eucw1 > wp._eucw2 ?
			wp._eucw1 > wp._eucw3 ?
				wp._eucw1 : wp._eucw3 :
			wp._eucw2 > wp._eucw3 ?
				wp._eucw2 : wp._eucw3 ;
# else
#  ifdef preSVr4
#  else
	{	/* Older versions of gettxt() didn't have setcat(), */
		/* so this has to be done by hand. Unfortunately, to */
		/* do this means we can't use dynamic libraries. */
		extern char *_Msgdb;
		_Msgdb = "uxemail";
	}
#  endif
# endif
#endif
#ifdef SIGCONT
# ifdef preSVr4
	sigset(SIGCONT, SIG_DFL);
# else
	{
	struct sigaction nsig;
	nsig.sa_handler = SIG_DFL;
	sigemptyset(&nsig.sa_mask);
	nsig.sa_flags = SA_RESTART;
	(void) sigaction(SIGCONT, &nsig, (struct sigaction*)0);
	}
# endif
#endif
	progname = argv[0];
	myegid = getegid();
	myrgid = getgid();
	myeuid = geteuid();
	myruid = getuid();
	mypid = getpid();
	setgid(myrgid);
	setuid(myruid);
	inithost();
	intty = isatty(0);
#ifdef USE_TERMIOS
	if (tcgetattr(1, &tbuf) == 0) {
		outtty = 1;
		baud = cfgetospeed(&tbuf);
	}
#else
	if (ioctl(1, TCGETA, &tbuf)==0) {
		outtty = 1;
		baud = tbuf.c_cflag & CBAUD;
	}
#endif
	else
		baud = B9600;
	image = -1;

	/*
	 * Now, determine how we are being used.
	 * We successively pick off instances of -r, -h, -f, and -i.
	 * If there is anything left, it is the base of the list
	 * of users to mail to.  Argp will be set to point to the
	 * first of these users.
	 */

	while ((c = getopt(argc, argv, "efFh:HinNr:s:u:UdIT:V~tB")) != -1)
		switch (c) {
		case 'B':
			/*
			 * Unbuffered input and output.
			 */
			setbuf(stdin, (char*)0);
			setbuf(stdout, (char*)0);
			break;

		case 'd':
			/*
			 * Turn on debugging output.
			 */
			assign("debug", "");
			break;

		case 'e':
			/*
			 * exit status only
			 */
			exitflg++;
			break;

		case 'f':
			/*
			 * User is specifying file to "edit" with mailx,
			 * as opposed to reading system mailbox.
			 * If no argument is given after -f, we read his/her
			 * $MBOX file or mbox in his/her home directory.
			 */
			ef = (argc == optind || *argv[optind] == '-')
				? "" : argv[optind++];
			if (*ef && *ef != '/' && *ef != '+')
				cwd = getcwd(NOSTR, PATHSIZE);
			fflag++;
			break;

		case 'F':
			Fflag++;
			mustsend++;
			break;

		case 'h':
			/*
			 * Specified sequence number for network.
			 * This is the number of "hops" made so
			 * far (count of times message has been
			 * forwarded) to help avoid infinite mail loops.
			 */
			mustsend++;
			hflag = atoi(optarg);
			if (hflag == 0) {
				pfmt(stderr, MM_ERROR, 
					":313:-h needs non-zero number\n");
				goerr++;
			}
			break;

		case 'H':
			/*
			 * Print headers and exit
			 */
			Hflag++;
			break;

		case 'i':
			/*
			 * User wants to ignore interrupts.
			 * Set the variable "ignore"
			 */
			assign("ignore", "");
			break;

		case 'n':
			/*
			 * User doesn't want to source
			 *	/etc/mail/mailx.rc
			 */
			nosrc++;
			break;

		case 'N':
			/*
			 * Avoid initial header printing.
			 */
			noheader++;
			break;

		case 'r':
			/*
			 * Next argument is address to be sent along
			 * to the mailer.
			 */
			mustsend++;
			rflag = optarg;
			break;

		case 's':
			/*
			 * Give a subject field for sending from
			 * non terminal
			 */
			mustsend++;
			sflag = optarg;
			break;

		case 't':
			/*
			 * Use To: fields to determine
			 * recipients.
			 */
			tflag++;
			break;

		case 'T':
			/*
			 * Next argument is temp file to write which
			 * articles have been read/deleted for netnews.
			 */
			{
			int f;
			Tflag = optarg;
			if ((f = creat(Tflag, TEMPPERM)) < 0) {
				pfmt(stderr, MM_ERROR,
					":123:Cannot create %s: %s\n",
					Tflag, strerror(errno));
				exit(1);
			}
			close(f);
			}
			/* fall through for -I too */
			/* FALLTHROUGH */

		case 'I':
			/*
			 * print newsgroup in header summary
			 */
			newsflg++;
			break;

		case 'u':
			/*
			 * Next argument is person's mailbox to use.
			 * Treated the same as "-f /var/mail/user".
			 */
			{
			static char u[PATHSIZE];
			strcpy(u, maildir);
			strncat(u, optarg, PATHSIZE);
			ef = u;
			break;
			}

		case 'U':
			UnUUCP++;
			break;

		case 'V':
			puts(version);
			return 0;

		case '~':
			/*
			 * Permit tildas no matter where
			 * the input is coming from.
			 */
			escapeokay++;
			break;

		case '?':
		default:
			goerr++;
			break;
		}

	if ( optind != argc )
		argp = optind;

	/*
	 * Check for inconsistent arguments.
	 */

	if (newsflg && ef==NOSTR) {
		pfmt(stderr, MM_ERROR, ":314:Need -f with -I flag\n");
		goerr++;
	}
	if (ef != NOSTR && argp != -1) {
		pfmt(stderr, MM_ERROR, 
			":315:Cannot give -f and people to send to.\n");
		goerr++;
	}
	if (tflag && argp != -1) {
		pfmt(stderr, MM_ERROR, ":472:Cannot give -t and people to send to.\n");
		goerr++;
	}
	if (exitflg && (mustsend || argp != -1))
		exit(1);	/* nonsense flags involving -e simply exit */
	if (mustsend && argp == -1) {
		pfmt(stderr, MM_ERROR, 
			":316:The flags you gave are used only when sending mail.\n");
		goerr++;
	}
	if (goerr) {
		pfmt(stderr, MM_ACTION, 
			":471:Usage: %s -BeiIUdFnNHV~ -t -T FILE -u USER -h hops -r address -s SUBJECT -f FILE users\n",
			progname);
		exit(1);
	}
	tinit();
	input = stdin;
	rcvmode = !tflag && (argp == -1);
	if (!nosrc)
		load(MASTER);
	load(Getf("MAILRC"));

	if (tflag) {
		mailt();
		exit(senderr);
	}
	else if (argp != -1) {
		mail(&argv[argp]);
		exit(senderr);
	}

	/*
		If $MAIL is set, and -f has not, use $MAIL for mailbox.
	*/
	mf = getenv("MAIL");
	if ((mf != NOSTR) && (*mf != 0) && (ef == NOSTR))
		ef = mf;

	/*
	 * Ok, we are reading mail.
	 * Decide whether we are editing a mailbox or reading
	 * the system mailbox, and open up the right stuff.
	 */

	strcpy(origname, mailname);

	if (ef != NOSTR) {
		ef = *ef ? safeexpand(ef) : Getf("MBOX");
		strcpy(origname, ef);
		if (ef[0] != '/') {
			if (cwd == NOSTR)
				cwd = getcwd(NOSTR, PATHSIZE);
			strcat(cwd, "/");
			strcat(cwd, ef);
			ef = cwd;
		}
		strcpy(mailname, ef);
		/* If the file is not under /var/mail, we're editing it. */
		if (ismbox(mailname)) {
			lockname = strrchr(ef, '/') + 1;
		} else {
			editfile = ef;
			edit++;
		}
	}

	if (setfile(mailname, edit) < 0)
		exit(1);

	if (msgCount > 0 && !noheader && value("header") != NOSTR) {
		if (setjmp(hdrjmp) == 0) {
			if ((prevint = sigset(SIGINT, SIG_IGN)) != SIG_IGN)
				sigset(SIGINT, hdrstop);
			announce();
			fflush(stdout);
			sigset(SIGINT, prevint);
		}
	}
	if (Hflag || (!edit && msgCount == 0)) {
		if (!Hflag) {
			pfmt(stderr, MM_NOSTD, hasnomailfor, myname);
			Verhogen();
		}
		fflush(stdout);
		exit(0);
	}
	commands();
	if (!edit) {
		sigset(SIGHUP, SIG_IGN);
		sigset(SIGINT, SIG_IGN);
		sigset(SIGQUIT, SIG_IGN);
		quit();
		Verhogen();
	}
	exit(0);
	/* NOTREACHED */
}

/*
 * Interrupt printing of the headers.
 */
/* ARGSUSED */
static void
hdrstop(unused)
int unused;
{
	putchar('\n');
	pfmt(stdout, MM_WARNING, hasinterrupted);
	fflush(stdout);
	sigrelse(SIGINT);
	longjmp(hdrjmp, 1);
}

/* check to see if the given filename is a mailbox name */
int ismbox(mbox)
char *mbox;
{
    char svdir[PATHSIZE + 7];
    char *p;

    strncpy(svdir, mbox, PATHSIZE-1);
    p = strrchr(svdir, '/');
    if (!p) {
	strcpy(svdir, "./");
	p = svdir +2;
    }

    /* replace the last part of the filename with :saved */
    strcpy(p+1, ":saved");
    /* is .../:saved a directory? */
    /* if so, consider it a mailbox name */
    return isdir(svdir);
}
