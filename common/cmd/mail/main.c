/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/main.c	1.13.2.5"
#ident "@(#)main.c	2.26 'attmail mail(1) command'"
/*
    NAME
	mail,rmail - read or send mail

    SYNOPSIS
	mail [ -ehpPqrtw ] [-x debuglevel] [ -f file ] [ -F user(s) ]
	mail -T file persons
	mail [ -tw ] [ -m messagetype ] persons
	rmail [ -tw ] persons

    DESCRIPTION

    SECURITY LEVEL
	Mail may optionally take the P_OWNER fixed privilege.
	This would only be used on systems with the RSTCHOWN
	{POSIX_CHOWN_RESTRICTED} configuration option turned
	on within the kernel.
*/
#include "mail.h"
#ifdef SVR4_1
# include <priv.h>
# include <mac.h>
# include <sys/secsys.h>
#endif
static char CMDCLASS[] = "UX:";   /* Command classification */

main(argc, argv)
char	**argv;
{
	static const char pn[] = "main";
	register int i;		/* temp variable */
	int parse_err = 0;	/* counts parsing errors */
	char label[MAXLABEL+1];		/* Space for the catalog label */

	(void) setlocale(LC_ALL, "");
#ifdef SIGCONT
# ifdef SVR4
	{
	struct sigaction nsig;
	nsig.sa_handler = SIG_DFL;
	sigemptyset(&nsig.sa_mask);
	nsig.sa_flags = SA_RESTART;
	(void) sigaction(SIGCONT, &nsig, (struct sigaction*)0);
	}
# else
	sigset(SIGCONT, SIG_DFL);
# endif
#endif
	/* This is protection. Without this, we cannot wait on children. */
	signal(SIGCLD, SIG_DFL);

	/* Strip off path name of this command for use in messages */
	if ((progname = strrchr(argv[0],'/')) != NULL)
		progname++;
	else
		progname = argv[0];

	(void) strcpy(label, CMDCLASS);
	(void) strncat(label, progname, (MAXLABEL - sizeof(CMDCLASS) - 1));
	(void) setlabel(label);
	(void) setcat("uxemail");
#ifdef SVR4_1
	(void) mldmode(MLD_VIRT);
#endif /* SVR4_1 */

	/* Close all file descriptors except stdin, stdout & stderr */
	closeallfiles(3);

	/*
		Parse the command line and adjust argc and argv
		to compensate for any options
	*/
	i = parse(argc, argv, &parse_err);
	argv += (i - 1);
	argc -= (i - 1);
	init();

	/* Block a potential security hole. */
	/* If -T is specified, run the surrogate commands with setgid-mail only */
	/* if we are running as the system administrator. */
#ifdef SVR4_1
	if (flgT && (my_euid != (uid_t) secsys(ES_PRVID, 0)))
#else
	if (flgT && (my_euid != 0))
#endif
		{
		setgid(my_gid);
		Tout (pn, "Setgid unset\n");
		}

	if (!ismail && ((parse_err > 0) || (argc == 1)))
		{
		Dout(pn, 11, "!ismail, parse_err=%d, i=%d\n", parse_err, i);
		pfmt(stderr, MM_ERROR, ":120:Incorrect usage\n");
		if (parse_err > 0)
			pfmt(stderr, MM_ACTION,
				":121:Usage: rmail [-wt] person(s)\n");
		if (argc == 1)
			pfmt(stderr, MM_ACTION,
				":122:At least one user must be specified\n");
		error = E_SYNTAX;
		Dout(pn, 0,"error set to %d\n", error);
		Dout(pn, 11, "exiting!\n");
		done(0);
		}

	/* Catch signals for cleanup */
	if (setjmp(sjbuf))
		done(0);
	for (i=SIGINT; i<SIGCLD; i++)
		setsig(i, delete);
	setsig(SIGHUP, done);

	cksaved(my_name);

	/* Rmail is always invoked to send mail */
	Dout(pn, 11, "ismail=%d, argc=%d\n", ismail, argc);
	if (ismail && (argc==1))
		{
		if (flgF)
			doFopt();
		else if (flge)
			doeopt();
		else
			printmail();
		}
	else
		{
		init_altenviron();
		sendmail(argc, argv);
		}

	done(0); /*NOTREACHED*/
}
