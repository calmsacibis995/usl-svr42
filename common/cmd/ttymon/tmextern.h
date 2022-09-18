/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/tmextern.h	1.9.7.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/ttymon/tmextern.h,v 1.1 91/02/28 20:16:17 ccs Exp $"
/* ttymon.c	*/
	extern	void	setup_PCpipe();

/* tmautobaud.c	*/
	extern	int	auto_termio();
	extern	char	*autobaud();

/* tmchild.c 	*/
	extern	void	write_prompt();
	extern 	void 	timedout();

/* tmexpress.c 	*/
	extern	void	ttymon_express();

/* tmhandler.c 	*/
	extern	void	do_poll();
	extern 	void 	sigterm();
	extern 	void 	sigchild();
	extern 	void	state_change();
	extern 	void	re_read();
	extern 	void	got_carrier();

/* tmlog.c 	*/
	extern 	void	openlog(); 
	extern 	void 	log();
	extern 	void 	logexit();

/* tmparse.c 	*/
	extern	char	*getword();
	extern	char	quoted();

/* tmpeek.c 	*/
	extern	int	poll_data();

/* tmpmtab.c 	*/
	extern	void	read_pmtab();
	extern	void	purge();

/* tmsac.c 	*/
	extern 	void	openpid();
	extern 	void	openpipes();
	extern 	void	get_environ();
	extern	void	sacpoll();

/* tmsig.c 	*/
	extern 	void catch_signals();
	extern 	void child_sigcatch();

/* tmterm.c 	*/
	extern  int	push_linedisc();
	extern	int	set_termio();
	extern	int	initial_termio();
	extern	int	turnon_canon();
	extern	int	hang_up_line();
	extern	void 	flush_input();

/* tmttydefs.c 	*/
	extern	void	read_ttydefs();
	extern 	struct 	Gdef *find_def();
	extern  char 	*getword();
	extern	void	mkargv();

/* tmutmp.c 	*/
	extern 	int 	account();
	extern 	void 	cleanut();

/* tmutil.c 	*/
	extern	int	check_device();
	extern	int	check_cmd();

/* misc sys call or lib function call */
	extern	int	check_version();
	extern	int	fchown();
	extern	int	fchmod();

#ifdef	SYS_NAME
	extern 	void sys_name();
#endif


/* tmglobal.c 	*/
	extern	struct	pmtab	*PMtab;
	extern	int	Nentries;

	extern	int	Npollfd;

	extern	struct 	Gdef Gdef[];	
	extern	int	Ndefs;	
	extern	long	Mtime;

	extern	char	Scratch[];
	extern	int	Pfd;
	extern	int	PCpipe[];

	extern	char	State;
	extern	char	*Tag;
	extern	int	Reread_flag;

	extern	int 	Maxfds;

	extern	char	**environ;
	extern	int	errno;
	extern	char	*optarg;
	extern	int	optind, opterr;

	extern	int	Nlocked;

	extern	sigset_t	Origmask;
	extern	struct	sigaction	Sigalrm;	/* SIGALRM */
	extern	struct	sigaction	Sigcld;		/* SIGCLD */
	extern	struct	sigaction	Sigint;		/* SIGINT */
	extern	struct	sigaction	Sigpoll;	/* SIGPOLL */
	extern	struct	sigaction	Sigterm;	/* SIGTERM */
#ifdef	DEBUG
	extern	struct	sigaction	Sigusr1;	/* SIGUSR1 */
	extern	struct	sigaction	Sigusr2;	/* SIGUSR2 */
#endif

#ifdef	DEBUG
	extern	void	debug();
#endif

	extern	gid_t	Tty_gid;
