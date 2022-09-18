/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)r4xinit:xinit.c	1.23"
#endif

/*
 * xinit.c (C source file)
 * Copyright    Massachusetts Institute of Technology    1986
 */

#include <malloc.h>
#include <X11/Xlib.h>
#include <sys/types.h>
#include <sys/procset.h>
#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
/* Sun River work */
#include <sys/at_ansi.h>
#include <sys/kd.h>

/*
 * New fixed class added : Jan 1992
 * 0 - 49  : user class
 * 50 - 59 : fixed class
 * 60 - 100 : kernel mode
 * > 101    : real time mode
 *
 * The server by default runs in fixed class ie: 54 (mid range for fixed class)
 * but the user has different options : timeshare class, fixed class
 * or real time
 *
 * The fixedupri of 25 adds this number to default thus giving
 * user a higher priority than all other time share users.
 */

/*
 * TEMP: figure out how to differentiate between V4 and ES
 * default : SVR_ES
 * until then, if you are building on non ES machines comment the next line
 */

#define SVR4_ES  1

#define FIXEDUPRI	25

#ifdef SVR4_ES
#include <sys/fc.h>
#include <sys/fcpriocntl.h>
#endif

#define    TRUE                          1
#define    FALSE	                 0
#define    OK_EXIT                       0
#define    ERR_EXIT                      1
#define    DEFAULT_SERVER                "X"
#define    DEFAULT_SHUTDOWN \
		"cd /; /sbin/tfadmin /sbin/shutdown -i0 -g0 -y"

/* This define is used by olinit to determine if
 * the system should be shutdown.  This value must
 * be coordinated with similar defines in dtm.c and
 * wsm.c
 */

#define    SHUTDOWN_FLAG		 42
#define    DEFAULT_DISPLAY               ":0"
#define    XINITRC                       ".xinitrc"
#define    LOWERSERVERPRIORITYBY         1
#define    RAISECLIENTPRIORITYBY         1
#define    OTHEREXECUTE                  0000001
#define    GROUPEXECUTE                  0000010
#define    OWNEREXECUTE                  0000100
#define    OTHERREAD                     0000004
#define    GROUPREAD                     0000040
#define    OWNERREAD                     0000400
#define    OTHERWRITE                    0000002
#define    GROUPWRITE                    0000020
#define    OWNERWRITE                    0000200

extern int errno;
int fcntlflag;

char * getbasename();
void stdout_reopen ();

char * olinit_client[] = { "dtm", NULL};
char * xinit_client[] =  { "xterm", "-geometry", "+10+20", "-n", "login", NULL};

/* Sun River work */
extern void	get_display();
static char	DISPLAY[ 100 ] = "DISPLAY=";

typedef struct
{
	char * basename;
	char ** default_client;
	int invoke_xinitrc;
	char * out_file;
	char * err_file;
} PrimaryClientInfo;

#define NUM_PRIMARY_CLIENTS 4

PrimaryClientInfo client_info[NUM_PRIMARY_CLIENTS] =
{
	{ "olinit",  olinit_client, 0, ".olinitout", ".oliniterr"	},
	{ "desktop", olinit_client, 0, ".olinitout", ".oliniterr"	},
	{ "xinit",   xinit_client,  1, ".xinitout",  ".xiniterr"	},
	{ "default", xinit_client,  1, ".xinitout",  ".xiniterr"	}
};

int client_number;
char * basename;
char ** default_client;

char * server[100];
char * client[100];
char   *DisplayNumber;
char * program;

Display * xd = NULL;		/* server connection */

int serverpid = -1;
int clientpid = -1;

#define REALTIME	0
#define FIXEDCLASS	1
#define TIMESHARE	2

static int serverclass = FIXEDCLASS;

unsigned short getuid();
unsigned short getgid();
int ProcessTimeout(); /* pid, secs, pause, string, blip, waitingfor (0 = till dead) */
void getargs();
int WaitForServer();
void Error();
void Fatal();
int StartServer();
int StartOtherClients();
void Shutdown();
void sigCatch();
char * HomeFilename();

/* main
*
* The xinit client is responsible for starting the X server and one seed
* client.  Defaults are set in the defined and global names above.  The
* user can override the defaults through the command line.  A complete
* description of the command line arguments can be found in the man page
* for this client and are also briefly described below (in the getargs
* procedure).
*
* The process in the main is to parse the command arguments, start the X
* server and client, wait for either the client of server (or for a
* signal)
*/
char	rmfiles[48];

main(argc, argv, envp)
int argc;
char **argv;
char **envp;
{
	int pid, exit_val = 0, sfd;
	struct	stat	buf;
	char	disp[16];

	/* Sun River work: do this before we fork and exit */
	getargs( argc, argv);

	strcpy (disp, "/tmp/.dt.");
	strcat(disp, &DisplayNumber[1]);

	if ( !stat(disp, &buf) )
	{
	   printf ("Server is already running.\n");
	   printf ("If you are sure that server is not running, remove %s and try again.\n", disp);
	   exit (1);
	}
	strcpy (rmfiles, "rm -f /dev/X/server* ");
	strcat (rmfiles, disp);

	if (fork() != 0) exit(0);
	setpgrp();

	fcntl(2,F_GETFL,&fcntlflag);
	sfd = dup(1);
	stdout_reopen();
	signal( SIGQUIT, sigCatch);
	signal( SIGINT, sigCatch);
	signal(SIGHUP, sigCatch);	/*x11r4 change */
	signal (SIGPIPE, sigCatch);	/* x11r4 change*/

	/* if ((serverpid = StartServer(server,envp)) > 0 ) */

	if ((serverpid = StartServer(server,envp)) > 0 && (clientpid = StartClient(client)) > 0)
	{
		int status;

		fprintf(stderr,
		    "\n%s: %s\n\n     Server Process Id = %8d\n     Client Process Id = %8d\n\n",
		    basename, "Window system initialized:",serverpid,clientpid);

		fflush (stderr);

		if (client_info[client_number].invoke_xinitrc)
			(void)StartOtherClients();

		pid = -1;
		while (pid != clientpid && pid != serverpid)
			pid = wait(&status);

		if (pid == clientpid)
			if (WIFEXITED(status))
				exit_val = WEXITSTATUS(status);

	}
	system (rmfiles);

	signal( SIGQUIT, SIG_IGN);
	signal( SIGINT, SIG_IGN);
	signal( SIGCLD, SIG_IGN);
	signal (SIGHUP, SIG_IGN);	/* x11r4 change */
	signal (SIGPIPE, SIG_IGN);	/*x11r4 change*/

	Shutdown( serverpid, clientpid);
	fflush(stderr);

	if (serverpid < 0 || clientpid < 0) {
		Fatal(ERR_EXIT,"Shutdown of pid %d and/or %d failed!\n",serverpid,clientpid);
		/* NOT REACHED */
	}

	if (exit_val == SHUTDOWN_FLAG) {
			(void)close(1);
			(void)dup(sfd);
			(void)close(sfd);
			SysShutdown();
	}

	Fatal(OK_EXIT,"Window system terminated normally.\n");

} /* end of main */

/* SysShutdown
 *
 * This Procedure does a system(shutdown) if the exit value from
 * the desktop manger requested a shutdown.
 */

SysShutdown()
{
	char shutdown[128];
	int	status;

	strcpy(shutdown, DEFAULT_SHUTDOWN);
	status = system(shutdown);
}

/* getargs
 *
 * This procedure parses the command line arguments generating the server
 * and client argv lists in the process.  The DisplayNumber is also init-
 * ialized during the parse.
 */

void 
getargs (argc,argv)
int argc;
char **argv;
{
	register char **sptr = server;
	register char **cptr = client;
	register char **ptr;

	program = *argv++;
	argc--;

	{
		int i;

		basename = getbasename(program);

		for (i = 0; i < NUM_PRIMARY_CLIENTS; ++i)
			if (!strcmp(basename, client_info[i].basename) ||
			    !strcmp("default", client_info[i].basename))
				break;

		default_client = client_info[i].default_client;
		client_number = i;
	}

	/* copy the client args.  */

	if (argc == 0 || (**argv != '/' && **argv != '.' ))
		for (ptr = default_client; *ptr; )
			*cptr++ = *ptr++;
	while (argc && strcmp(*argv, "--"))
	{
		/*
		 * if -serverclass <fixed|realtime|timeshare> specified, don't
		 * copy these args into client[], but set the class flag
		 */
	    if ( strcmp(*argv, "-serverclass") ) {
		*cptr++ = *argv++;
		argc--;
	    }
	    else {
		/* found "-serverclass" on command line, get the next argument */
		argv++;
		if ( !strcmp(*argv, "realtime") ) {
			serverclass = REALTIME; argv++;
		}
		else if ( !strcmp (*argv, "fixed") ) {
			serverclass = FIXEDCLASS; argv++;
		}
		else if ( !strcmp (*argv, "timeshare") ) {
			serverclass = TIMESHARE; argv++;
		}
	    }
	}

	*cptr = NULL;
	if (argc)
	{
		argv++;
		argc--;
	}

	/* Copy the server args.  */

	if (argc == 0 || (**argv != '/' && **argv != '.' ))
	{
		*sptr++ = DEFAULT_SERVER;
	}
	else 
	{
		*sptr++ = *argv++;
		argc--;
	}

	if (argc > 0 && (argv[0][0] == ':' && isdigit(argv[0][1])))
	{
		DisplayNumber = (char *)malloc( strlen( *argv ) +1 );
		strcpy( DisplayNumber, *argv );
	}
	else
	{
		DisplayNumber = (char *)malloc( 4 ); /* Current max Dispno == "259" */
		get_display( DisplayNumber );	/* Sun River work */
		*sptr++ = DisplayNumber;
	}

	while (--argc >= 0)
		*sptr++ = *argv++;
	*sptr = NULL;

} /* end of getargs */


/* getbasename
 *
 * This function returns a pointer to the 'basename' section of its
 * argument (ie. the first character after the last '/' or the first
 * character).
 */
char * getbasename(path)
char * path;
{
	char * ptr;

	ptr = strrchr(path, '/');

	if (ptr != NULL)
		return ptr + 1;

	return path;

} /* end of getbasename */

/*
 * This function reasigns stdout and stderr
 */
void 
stdout_reopen()
{
	char * file_name;

	file_name = HomeFilename(client_info[client_number].out_file);
	if (freopen(file_name, "w", stdout) == (FILE *)NULL)
		Fatal("%s: Can't open %s", basename, file_name);

	file_name = HomeFilename(client_info[client_number].err_file);
	if (freopen(file_name, "w", stderr) == (FILE *)NULL)
		Fatal("%s: Can't open %s", basename, file_name);

} /* end of stdout_reopen */


/*
 * WaitForServer
 *
 * This function is used to determine if the server has been sucessfully
 * started.  It uses the XOpenDisplay routine to determine if the server
 * is ready and able.   Since the server may not immediately respond the
 * open is performed within a loop executed "ncycles" number of times
 * If the display cannot be opened during this time, it is presumed that
 * the server is DEAD and the function returns FALSE, otherwise is returns
 * TRUE.  Note: The ProcessTimeout function is called to determine if the
 * process id passed is valid.  If not the routine will also return FALSE
 * indicating that the server is dead
 */
int 
WaitForServer(serverpid)
int serverpid;
{
	int ncycles = 50;	/* # of cycles to wait */
	int cycles;            /* Wait cycle count    */
	char display[100];     /* Display name        */
	int tmp = 0;

	strcpy(display, "unix");
	strcat(display, DisplayNumber);
	/* Sun River work */
	strcat(DISPLAY, display);
	putenv(DISPLAY);

	for (cycles = 0; cycles < ncycles; cycles++) {
	   if (xd = XOpenDisplay(display))/* Sun River work:added display arg */
	   {
	      XCloseDisplay(xd);
	      fprintf(stderr,"\n%s: the server is ready\n", basename);
	      return(TRUE);
	   }
	   else 
	   {
	      fprintf(stderr,".");
	      errno = 0;
	      if ( ProcessTimeout(serverpid, 2, 1, NULL, NULL, NULL, -1))
		 return(FALSE);
	   }
	}
	return(FALSE);
} /* end of WaitForServer */

#ifdef DELETE
int 
WaitForServer(serverpid)
int serverpid;
{
	int ncycles =  45;                         /* # of cycles to wait */
	int cycles;                                /* Wait cycle count    */
	int tmp = 0;

	char display[100];                         /* Display name        */
	char 		name[100];
	char 		buf1[25];
	int		id;
	struct	stat	buf;
	FILE		*fp;

	strcpy(display, "unix");
	strcat(display, DisplayNumber);
	/* Sun River work */
	strcat(DISPLAY, display);
	putenv(DISPLAY);

	strcpy(buf1, "/dev/X/server.");
	strcat(buf1, &DisplayNumber[1]);
	/*
	 * DisplayNumber is a ptr to a string, ex: ":0" or ":1" and so on
	 * get rid of ':' and then get the number
	 */
	strcpy(name, "/dev/X/server.");
	strcat(name, &DisplayNumber[1]);
	strcat(name, ".pid");

	for (cycles = 0; cycles < ncycles; cycles++) {
		/*
		 * first check if file, "/dev/X/server.?.pid" file exists;
		 * if it does, that file has the process id of the server
		 * Just make sure the process id of the current server and the
		 * number in server.?.pid are the same; this is an extra
		 * check because sometimes 
		 * server.0.pid may be left over (when the server doesn't exit
		 * normally ) from previous session
		 *
		 * Now try to connect to the server
		 */
		if ( !stat(name, &buf) )
		{
		   if ( (fp = fopen(name,"r")) != NULL ) {
			fscanf (fp, "%d", &id);
			fclose (fp);
			if ( id == serverpid ) {
			   if (xd = XOpenDisplay(display))
			   {
				XCloseDisplay(xd);
				fprintf(stderr,"\n%s: the server is ready\n", basename);
				return(TRUE);
			   }
			   else 
			   {
				fprintf(stderr,".");
				errno = 0;
				if ( ProcessTimeout(serverpid, 2, 1, NULL, NULL, NULL, -1))
					return(FALSE);
			   }
			}
		   }
		}
	}
	return(FALSE);
} /* end of WaitForServer */
#endif

/* Error
 *
 * This procedure is used to print error messages. It accepts a format
 * string and up to 10 int arguments to be printed
 * The procedure also determines if an error message needs to be displayed
 * using the errno and sys_errlist facilities
 */
void 
Error(fmt, x0,x1,x2,x3,x4,x5,x6,x7,x8,x9)
char*fmt;
{
	extern char * sys_errlist[];

	fprintf(stderr, "%s: ", program);

	if (errno)
		fprintf(stderr, "%s: ", sys_errlist[ errno ]);

	fprintf(stderr, fmt, x0,x1,x2,x3,x4,x5,x6,x7,x8,x9);

} /* end of Error */

/* Fatal
 *
 * This procedure is called when a fatal error has occurred.  It calls
 * the Error routine to print the error message, restores stderr's status,
 * sends SIGTERM to all members of our process group, and exits with the
 * given rc exit code
 */
void 
Fatal(rc,fmt, x0,x1,x2,x3,x4,x5,x6,x7,x8,x9)
int rc;
char*fmt;
{
	if (rc == OK_EXIT) errno = 0;
	Error(fmt, x0,x1,x2,x3,x4,x5,x6,x7,x8,x9);
	fcntl(2,F_SETFL,fcntlflag);
	kill(0, SIGTERM);
	exit(rc);
} /* end of Fatal */

/*
 * switchClass :
 *	switch the runtime class for the server to be fixed, realtime or
 *	timeshare; default is fixed (SVR4_ES or above); for previous version
 *	of OS (pre SVR4_ES), the default class is timeshare.
 */
switchClass (class)
 int class;
{
	pcinfo_t  info;
	pcparms_t args;

	switch (class) {
	   case REALTIME:
		strcpy (info.pc_clname, "RT");
		if (priocntl (0, 0, PC_GETCID, &info) >= 0) {
			args.pc_cid = info.pc_cid;
			((rtparms_t *)args.pc_clparms)->rt_tqnsecs = RT_TQDEF;
			((rtparms_t *)args.pc_clparms)->rt_pri = RT_NOCHANGE;
			if (priocntl (P_PID, P_MYID, PC_SETPARMS, &args) >= 0) {
				fprintf (stderr, "Server switched to RealTime.\n");
				return;
			}
		}

	   case FIXEDCLASS:
	   default:
#ifdef SVR4_ES
		strcpy (info.pc_clname, "FC");
		if (priocntl (0, 0, PC_GETCID, &info) >= 0) {
			args.pc_cid = info.pc_cid;
			((fcparms_t *)args.pc_clparms)->fc_uprilim = FIXEDUPRI;
			((fcparms_t *)args.pc_clparms)->fc_upri = FIXEDUPRI;
			if (priocntl (P_PID, P_MYID, PC_SETPARMS, &args) >= 0) {
				fprintf (stderr, "Server switched to Fixed Class.\n");
				return;
			}
		}
#endif

	   case TIMESHARE:
		fprintf (stderr, "Running in TimeShare mode.\n");
		break;
	}
	return;
}

/* StartServer
 *
 * This function is called to start the X server.  It immediately forks
 *
 * The CHILD process lowers its priority,
 * and execs the server process.  If the exec fails a SIGINT signal
 * is sent to the PARENT (i.e., xinit) and the Fatal routine is called to
 * write an error message and exit
 *
 * The PARENT process checks to see if the server process is valid (using
 * the ProcessTimeout function); if not then -1 is returned (indicating
 * that the server could not be started
 */
int 
StartServer (server, envp)
char * server[];
char **envp;
{
	int serverpid;
	int myuid, myeuid;
	FILE	*fp;
	char	disp[16];

	myuid = getuid ();
	myeuid = geteuid ();

	serverpid = fork ();
	switch (serverpid)
	{
	case 0:  /* child process */
		errno = 0;
		switchClass (serverclass);
		fflush (stderr);
		setuid (myuid);
		strcpy (disp, "/tmp/.dt.");
		strcat(disp, &DisplayNumber[1]);
		/*
		 * create a flag file,  /tmp/.dt.<dispnum>
		 */
		if ( (fp = fopen(disp,"w+")) != NULL )
			fclose (fp);

		nice(-LOWERSERVERPRIORITYBY);
		execvp(server[0], server, envp);
		kill(getppid(),SIGINT);
		system (rmfiles);
		Fatal(ERR_EXIT,"Server \"%s\" died on startup\n", server[0]);
		break;

	case -1: /* fork failed */
		setuid (myuid);
		break;

	default: /* parent continues on... */
		setuid (myuid);
		errno = 0;

		nice(RAISECLIENTPRIORITYBY);
		if (! ProcessTimeout(serverpid, 1, 1, "server to start\n",NULL, NULL, 0))
			serverpid = -1;
		else
		{
			errno = 0;
			if ( !WaitForServer(serverpid) )
			{
				Error("Can't connect to server\n");
				Shutdown(serverpid, -1);
				serverpid = -1;
			}
		}
		break;
	}
	return(serverpid);
} /* end of StartServer */

/* 
 * Start Client
 * This function is called to start the client process.  It immediately
 * forks
 *
 * The CHILD process sets the process group (used later when killing the X
 * processes, resets the user id to the real user, and execs the client
 * process.  In the event that the exec fails the Fatal routine is called
 * to inform the user and exit
 *                                or
 * If the exec fails a SIGINT signal
 */
int 
StartClient(client)
char * client[];
{
	int clientpid = fork();

	if (clientpid  == 0)
	{
		setuid((int) getuid());
		execvp(client[0], client);
		Fatal(ERR_EXIT,"Client \"%s\" died on start up\n", client[0]);
	}

	return (clientpid);
} /* end of StartClient */

/* 
 * Shutdown
 * This procedure is used to shutdown (kill) the clients and server.  It
 * kills (or attempts to kill) the clients by sending the SIGINT signal. If
 * the kill fails (e.g., any client is ignoring SIGINT) Error is called to
 * report the failure to the user.  The server is killed by first sending
 * the SIGTERM signal and (if the server refuses to die) then SIGKILL.
 * the function ProcessTimeout is used to determine if the server has died
 * in the evenet that it refuses to die (even after SIGKILL is sent) the
 * Fatal routine is called to report the failure and exit.  If everything
 * goes well Shutdown merely returns
 */
void 
Shutdown(serverpid, clientpid)
int serverpid;
int clientpid;
{
#define TESTFORVALIDPID 0

	if (clientpid > 0)
	{
		errno = 0;

		fprintf(stderr,"%s: Killing client pid  = %d\n", basename, clientpid);

		if (!kill(clientpid, TESTFORVALIDPID))
			if (kill(clientpid, SIGINT) != 0)
				Error("can't kill(%d, SIGINT) for client\n", clientpid);
	}


	if (serverpid > 0)
	{
		errno = 0;

		fprintf(stderr,"%s: Killing server pid  = %d\n",basename, serverpid);
		if (kill(serverpid, SIGTERM) < 0)
		{
			if (errno == EPERM)
				Fatal(ERR_EXIT,"Can't kill server\n");
			else
				if (errno == ESRCH)
					return;
		}
		if (!ProcessTimeout(serverpid, 10, 1, "server to terminate", ".", "!\n", -1))
		{
			fprintf(stderr, "timeout...sending SIGKILL\n");
			errno = 0;
			if (kill(serverpid, SIGKILL) < 0)
			{
				if (errno == ESRCH)
					return;
			}
			if (!ProcessTimeout(serverpid, 3, 3,"server to die", ".", "!\n", -1))
				Fatal(ERR_EXIT,"Can't kill server\n");
		}
	}
} /* end of Shutdown */

/* 
 * sigCatch
 * This procedure is responsible for handling signals.  It simply reports
 * which signal was received, calls Shutdown (to kill the server and its
 * client(s), and exits
 */
void 
sigCatch(sig)
int sig;
{
	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN); /* x11r4 change */
	signal(SIGPIPE, SIG_IGN); /* x11r4 change */

	Error("received signal %d.\n", sig);

	Shutdown(serverpid, clientpid);

	if (serverpid < 0 || clientpid < 0)
		Fatal(ERR_EXIT,"Shutdown of pid %d and/or %d failed!\n",serverpid,clientpid);
	else
		Fatal(OK_EXIT,"Window system terminated normally.\n");

} /* end of sigCatch */

/*
 * StartOtherClients
 *
 * The StartOtherClients procedure is used to start the process(s) that the
 * user can specify in the XINITRC file in his home directory.
 * This procedure forks immediately (with the PARENT returning immediately)
 * The CHILD continues on to test the XINITRC file in the HOME directory
 * for existence, and if it exists it will invoke /bin/sh (via the exec
 * routine) with "~/XINITRC" as the filename if it is readable  by the user
 * but not executable.  If it is executable then the file "~/XINITRC" is
 * simply execed.
 * After it has completed execution the CHILD exits.
 */
int 
StartOtherClients()
{
	int pid = fork();

	if (pid == 0)
	{
		struct stat statbuf;
		char * filename = HomeFilename(XINITRC);
		char * argv[3];
		unsigned short uid = getuid();
		unsigned short gid = getgid();

		setuid((int) uid);
		setgid((int) gid);

		if (!stat(filename, &statbuf))
		{ /* if executable then try fork-execing "~/.xinitrc" */
			if (modeis('x',statbuf.st_mode, statbuf.st_uid-uid, statbuf.st_gid-gid))
			{
				argv[0] = filename;
				argv[1] = NULL;
#ifdef DEBUG
				Error("Running XINITRC as `%s'.\n", filename);
#endif
				execvp(argv[0],argv);
				Error("exec of .xinitrc failed!!!\n");
			}
			else
				/* readable then try fork-execing "/bin/sh ~/.xinitrc" */
				if (modeis('r',statbuf.st_mode,statbuf.st_uid-uid,statbuf.st_gid-gid))
				{
					argv[0] = "/bin/sh";
					argv[1] = filename;
					argv[2] = NULL;
#ifdef DEBUG
					Error("Running XINITRC as `/bin/sh %s'.\n", filename);
#endif
					execvp(argv[0],argv);
					Error("exec of /bin/sh .xinitrc failed!!!\n");
				}
#ifdef DEBUG
			Error(".xinitrc file is not readable.\n");
#endif
		}
		else
		{
#ifdef DEBUG
			Error(".xinitrc file does not exist.\n");
#endif
		}

		exit(1);
	}

	return (pid);
} /* end of StartOtherClients */

/* 
 * HomeFilename
 * This function returns a char pointer to a path constructed from a given
 * name appended to the users HOME directory
 */
char * 
HomeFilename (name)
char * name;
{
	char * getenv();
	char * home = getenv("HOME");
	char * retval = name;

	if (home)
	{
		retval = malloc(strlen(home) + strlen(name) + 2);

		strcpy(retval,home);
		strcat(retval,"/");
		strcat(retval,name);
	}

	return retval;

} /* end of HomeFilename */

/*
 * modeis
 * This function returns a int flag indicating whether the given mode meets
 * the given testmode for a given uid and gid (flags)
 */
int 
modeis(testmode, mode, uidflag, gidflag)
char testmode;
int mode;
int uidflag;
int gidflag;
{
	register retval = 0;
	int ownerflag = 0;
	int groupflag = 0;
	int otherflag = 0;

	switch (testmode)
	{
	case 'x': 
		ownerflag = OWNERREAD | OWNEREXECUTE;
		groupflag = GROUPREAD | GROUPEXECUTE;
		otherflag = OTHERREAD | OTHEREXECUTE;
		break;
	case 'r': 
		ownerflag = OWNERREAD;
		groupflag = GROUPREAD;
		otherflag = OTHERREAD;
		break;
	case 'w': 
		ownerflag = OWNERWRITE;
		groupflag = GROUPWRITE;
		otherflag = OTHERWRITE;
		break;
	default : 
		break;
	}
	retval = 
	    (((mode & ownerflag) == ownerflag) && (uidflag == 0)) ||
	    (((mode & groupflag) == groupflag) && (uidflag != 0) && (gidflag == 0)) ||
	    (((mode & otherflag) == otherflag) && (uidflag != 0) && (gidflag != 0));

	return retval;

} /* end of modeis */

/* Sun River work:

    This ioctl info is from the kd driver folks:

       -1 => a tty like your 630
       0x6B64 => your console
       otherwise the lower 8 bits are Sun River term #,

       I map the Sun River channel number using (d * 10) + 100 to
       get a display number to use when creating /dev/X/server.n.
       This is an arbitrary decision for a display number scheme.
       We choose it over several alternatives.
*/
void
get_display (display)
char	display[];
{
	int	   fd;
	int	   ret;
	int    dispno;

	fd = open( "/dev/tty", O_RDWR );
	ret = ioctl( fd, KIOCINFO );
	close( fd );

	if (( ret & 0xFF00 ) == 0x7300 )	/* Sun River Station */
	{
		dispno = ((ret & 0xFF) * 10) + 100;
		sprintf( display, ":%d", dispno );
	}
	else /* if ( ret == -1 || ret == 0x6B64 ) */
	{
		display[ 0 ] = ':';
		display[ 1 ] = '0';
		display[ 2 ] = '\0';
	}
}
