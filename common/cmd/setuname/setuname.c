/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)setuname:setuname.c	1.3.8.6"
#ident "$Header: setuname.c 1.2 91/05/08 $"

/*
 *  setuname [-t] [-s name] [-n node] 
 */

/*
 *  Header files referenced:
 *	<stdio.h>	Standard I/O 
 *	<unistd.h>	Standard UNIX definitions
 *	<string.h>	String handling 
 *	<fmtmsg.h>	Standard message generation 
 *	<ctype.h>	Character types
 *	<errno.h>	Error handling
 *	<signal.h>	Signal handling 
 *	<sys/types.h>	Data types
 *	<sys/fcntl.h>	File control
 *	<sys/utsname.h>	System Name
 */

#include	<stdio.h>
#include	<sys/mod.h>
#include	<unistd.h>
#include	<string.h>
#include	<fmtmsg.h>
#include	<ctype.h>
#include	<errno.h>
#include	<signal.h>
#include	<sys/types.h>
#include	<sys/fcntl.h>
#include	<sys/ksym.h>

#include	<sys/utsname.h>

/*
 * Externals referenced (and not defined in a header)
 *	optind		index to the next arg for getopt()
 *	opterr		FLAG, TRUE tells getopt() to write messages
 *	optarg		Ptr to an option's argument
 *	getopt()	Gets an option from the command line
 *	putenv()	Writes values into the environment
 *	exit()		Exit the process
 *	access()	Check accessibility of a file
 *	malloc()	Allocate a block of main memory
 *	free()		Free allocated space
 *	open()		Open a file
 *	close()		Close an open file
 */
 
extern	int		optind;		/* argv[] index of next arg */
extern	int		opterr;		/* TRUE if getopt() is to print msgs */
extern	char	       *optarg;		/* Argument to parsed option */
extern	int		getopt();	/* Get an option from the command line */
extern	int	       	putenv();	/* Put a value into the environment */
extern	void		exit();		/* Exit the process */
extern	int		access();	/* Check the accessibility of a file */
extern	int		open();		/* Open a file */
extern	int		close();	/* Close an open a file */

/*
 *  L O C A L   D E F I N I T I O N S
 */

/*
 * Constants 
 */

#ifndef	TRUE
#define	TRUE		(1)
#endif

#ifndef	FALSE
#define	FALSE		(0)
#endif

#ifndef	NULL
#define	NULL		(0)
#endif

#define	OPTSTRING	"tn:s:"

#define	EX_O_K		0
#define	EX_ERROR	1

#define	RC_FILENAME	"/etc/rc2.d/S18setuname"
#define RC_DIRNAME	"/etc/rc2.d"


/*
 *  Messages
 */

#define	E_USAGE		"usage: setuname [-t] [-s name] [-n node]"
#define	E_MISSING	"Either -s name or -n node must be specified"
#define	E_UNAME		"Unable to get existing uname values"
#define E_INVNAME	"System-name invalid: %s"
#define E_LONGNAME	"System-name too long: %s"
#define E_INVNODE	"Network node-name invalid: %s"
#define E_LONGNODE	"Network node-name too long: %s"
#define E_NOPERMS	"No permissions, request denied"
#define E_NOSUCHDIR	"Directory doesn't exist: %s"
#define	E_INTERNAL	"Internal error: %d"

/*
 * Macros:
 *	stdmsg(r,l,s,t)	    Write a standard message.  
 *				'r' is the recoverability flag
 *				'l' is the label
 *				's' is the severity 
 *				't' is the text.
 */
 
#define	stdmsg(r,l,s,t)	(void) fmtmsg(MM_PRINT|MM_UTIL|r,l,s,t,MM_NULLACT,MM_NULLTAG)

/*
 * Local functions:
 *	setuname	Changes the system name and the network node name 
 */

static int	setuname();		/* This does the "real" work */


/*
 * Local data
 *	lbl		Buffer for the standard message label
 *	txt		Buffer for the standard message text
 */
 
static	char		lbl[MM_MXLABELLN+1];	/* Space for std msg label */
static	char		msg[MM_MXTXTLN+1];	/* Space for std msg text  */

/*
 *  int main(argc, argv)
 *	int	argc
 *	char   *argv[];
 */

int 
main(argc, argv)
	int	argc;			/* Argument count  */
	char   *argv[];			/* Argument vector */
{
	/* Automatic data */
	char	       *n_arg;			/* Ptr to arg for -n */
	char	       *s_arg;			/* Ptr to arg for -s */
	int		t_seen;			/* FLAG, -t option seen */
	char	       *cmdname;		/* Ptr to the command's name */
	char	       *p;			/* Temp pointer */
	int		usageerr;		/* FLAG, TRUE if usage error */
	int		exitcode;		/* Value to exit with */
	int		c;			/* Temp character */
	int		ok;			/* Flag, everything okay? */

	/* Build the standard-message label */
	if (p = strrchr(argv[0], '/')) cmdname = p+1;
	else cmdname = argv[0];
	(void) strcat(strcpy(lbl, "UX:"), cmdname);

	/* Make only the text in standard messages appear (SVR4.0 only) */
	(void) putenv("MSGVERB=text");

	
	/* Initializations */
	n_arg = s_arg = (char *) NULL;
	t_seen = FALSE;


	/* 
	 * Parse command 
	 */

	usageerr = FALSE;
	opterr = FALSE;
	while (!usageerr && (c = getopt(argc, argv, OPTSTRING)) != EOF) switch(c) {

	case 'n':			/* -n node */
	    if (n_arg) usageerr = TRUE;
	    else n_arg = optarg;
	    break;

	case 's':			/* -s name */
	    if (s_arg) usageerr = TRUE;
	    else s_arg = optarg;
	    break;

	case 't':			/* -t */
	    if (t_seen) usageerr = TRUE;
	    else t_seen = TRUE;
	    break;
	    
	default:			/* Something that doesn't exist */
	    usageerr = TRUE;
	}   /* switch() */

	/* If there was a usage error, report the error and exit */
	if ((argc >= (optind+1)) || usageerr) {
	    stdmsg(MM_NRECOV, lbl, MM_ERROR, E_USAGE);
	    exit(EX_ERROR);
	}

	/* Either -n <node> or -s <name> has to be specified */
	if (!(n_arg || s_arg)) {
	    stdmsg(MM_NRECOV, lbl, MM_ERROR, E_MISSING);
	    exit(EX_ERROR);
	}


	/* 
	 * Validate arguments:
	 *  - The length of the system name must be less than SYS_NMLN-1
	 *    characters,
	 *  - The length of the network node-name must be less than 
	 *    SYS_NMLN-1 characters,
	 *  - The system name must equal [a-zA-Z0-9-_]+,
	 *  - The network node-name must equal [a-zA-Z0-9-_]+.
	 */

	/* Check the length and the character-set of the system name */
	if (s_arg) {

	    /* Check length of the system name */
#if (defined (_POSIX_SOURCE) || defined(_XOPEN_SOURCE))
	    if (strlen(s_arg) > (size_t)(_SYS_NMLN-1))
#else
	    if (strlen(s_arg) > (size_t)(SYS_NMLN-1))
#endif
	    {
		(void) sprintf(msg, E_LONGNAME, s_arg);
		stdmsg(MM_NRECOV, lbl, MM_ERROR, msg);
		exit(EX_ERROR);
	    }

	    /* Check the character-set */
	    ok = TRUE;
	    for (p = s_arg ; ok && *p ; p++) {
		if (!isalnum(*p) && (*p != '-') && (*p != '_')) ok = FALSE;
	    }
	    if (!ok || (p == s_arg)) {
		(void) sprintf(msg, E_INVNAME, s_arg);
		stdmsg(MM_NRECOV, lbl, MM_ERROR, msg);
		exit(EX_ERROR);
	    }
	}

	/* Check the length and the character-set of the network node-name */

	if (n_arg) {

	    /* Check length of the network node-name */
#if (defined (_POSIX_SOURCE) || defined(_XOPEN_SOURCE))
	    if (strlen(n_arg) > (size_t)(_SYS_NMLN-1))
#else
	    if (strlen(n_arg) > (size_t)(SYS_NMLN-1))
#endif
	    {
		(void) sprintf(msg, E_LONGNODE, n_arg);
		stdmsg(MM_NRECOV, lbl, MM_ERROR, msg);
		exit(EX_ERROR);
	    }

	    /* Check the character-set */
	    ok = TRUE;
	    for (p = n_arg ; ok && *p ; p++) {
		if (!isalnum(*p) && (*p != '-') && (*p != '_')) ok = FALSE;
	    }
	    if (!ok || (p == n_arg)) {
		(void) sprintf(msg, E_INVNODE, n_arg);
		stdmsg(MM_NRECOV, lbl, MM_ERROR, msg);
		exit(EX_ERROR);
	    }
	}


	/*
	 * Make sure we have access to needed resources:
	 *   -  Read/write access to kernel memory (/dev/kmem)
	 *   -  If -t is not specified, read/write access to /etc/rc2.d
	 *   -  If -t is not specified, read access to /etc/rc2.d/S18setuname
	 */
	
	if (access("/dev/kmem", R_OK|W_OK) == 0) {
	    if ( ! t_seen ) {
	       if (access(RC_DIRNAME, R_OK|W_OK) == 0) {
		   if ((access(RC_FILENAME, R_OK) != 0) && 
		       (access(RC_FILENAME, F_OK) == 0)) {
		       stdmsg(MM_NRECOV, lbl, MM_ERROR, E_NOPERMS);
		       exit(EX_ERROR);
		   }
	       } 
	       else {
		   if (access(RC_DIRNAME, F_OK) == 0) {
		       stdmsg(MM_NRECOV, lbl, MM_ERROR, E_NOPERMS);
		       exit(EX_ERROR);
		   } 
		   else {
		       (void) sprintf(msg, E_NOSUCHDIR, RC_DIRNAME);
		       stdmsg(MM_NRECOV, lbl, MM_ERROR, msg);
		       exit(EX_ERROR);
		   }
	       }
	    }
	} 
	else {
	    stdmsg(MM_NRECOV, lbl, MM_ERROR, E_NOPERMS);
	    exit(EX_ERROR);
	}


	/* Attempt the setuname */
	if (setuname(t_seen, s_arg, n_arg) == 0) exitcode = EX_O_K;
	else {
	    (void) sprintf(msg, E_INTERNAL, errno);
	    stdmsg(MM_NRECOV, lbl, MM_ERROR, msg);
	    exitcode = EX_ERROR;
	}

	/* Finished */
	exit(exitcode);

#ifdef	lint
	return(0);
#endif
}  /* main() */

/*
 * int setuname(temp, name, node)
 *	int	temp
 *	char   *name
 *	char   *node
 *
 *	Set any or all of the following machine parameters, either
 *	temporarily or permanently, depending on <temp>.
 *	    - System name
 *	    - Network Node-name
 *
 */

static int 
setuname(temp, sysname, nodename)
	int	temp;		/* Set in kernel only flag */
	char   *sysname;	/* System name */
	char   *nodename;	/* Network node-name */
{
	/* Automatic Data */
	struct utsname	utsname;	/* Space for the kernel's utsname information */
	unsigned int	utsname_addr;	/* Addr of "utsname" in the kernel */
	char	       sysnm[30];		/* System name to set (from file or arg) */
	char	       nodenm[30];		/* Network node-name to set (from file or arg) */
	FILE	       *fd;		/* Std I/O File Descriptor for /etc/rc2.d/S18setuname */
	void	      (*oldsighup)();	/* Function to call for SIGHUP */
	void	      (*oldsigint)();	/* Function to call for SIGINT */
	int		rtncd;		/* Value to return to the caller */
	int		memfd;		/* File descriptor:  open kernel memory */
	int		i;		/* Temp counter */	
	unsigned long	tmp;


	/* Nothing's gone wrong yet (but we've only just begun!) */
	rtncd = 0;
	sysnm[0] = nodenm[0] = '\0';


	/* 
	 * Open the kernel's memory, get the existing "utsname" structure,
	 * change the system name and/or the network node-name in that structure,
	 * write it back out to kernel memory, then close kernel memory.
	 */

	if ((memfd = open("/dev/kmem", O_RDWR, 0)) > 0) {
	    struct mioc_rksym rks;
	    rks.mirk_symname = "utsname";
	    rks.mirk_buf = &utsname;
	    rks.mirk_buflen = sizeof(utsname);
	    if (ioctl(memfd, MIOC_READKSYM, &rks) == 0) {
		if (sysname) (void) strncpy(utsname.sysname, sysname, sizeof(utsname.sysname));
		if (nodename) (void) strncpy(utsname.nodename, nodename, sizeof(utsname.nodename));
		if(ioctl(memfd,MIOC_WRITEKSYM,&rks) != 0)
			rtncd = -1;
		(void) close(memfd);
	    } else rtncd = -1;
	} else rtncd = -1;
	if (rtncd != 0) return(rtncd);


	/*
	 * If the "temp" flag is FALSE, we need to permanently set the
	 * system name and node name in the file  /etc/rc2.d/S18setuname
	 * and update /etc/systemid as well as /etc/nodename
	 */

	if (!temp) {
		/* for the sake of security, array size = BUFSIZ */
		char cmd[BUFSIZ];
		char opt[BUFSIZ];
		char line[BUFSIZ];

	    /* 
	     * use the utsname structure to update the "S18setuname" file
	     * in rc2.d 
	     */
		if ((fd = fopen(RC_FILENAME, "r")) != (FILE *) NULL) {
			char *token_ptr;
		   while (fgets(line,60,fd) != NULL) {
				/* it is not sure /etc/rc2.d/S18setuname really contains
				an entry like "setuname -s par1 -n par2" (e.g the root can edit
				this file). Perform sscanf(sysnm), sscanf(nodenm) only if the above
				entry really exists, and only for the given option (s and/or n). */
				sscanf(line, "%s", cmd);
				if (strcmp(cmd, "setuname") == 0) {
					if (!sysname && (token_ptr = strstr(line, "-s")) != NULL) {
						token_ptr += strlen("-s");
						sscanf(token_ptr, "%s", sysnm);
					}
					if (!nodename && (token_ptr = strstr(line, "-n")) != NULL) {
						token_ptr += strlen("-n");
						sscanf(token_ptr, "%s", nodenm);
					}
					break; 
				}
			}
		}

		if ( !*sysnm) sprintf(sysnm,"%s", utsname.sysname);
		if ( !*nodenm) sprintf(nodenm,"%s", utsname.nodename);


	    /* 
	     * Write the file /etc/rc2.d/S18setuname so that the system's
	     * system-name and nodename  is set on boots and state changes.  
	     *
	     * DISABLED SIGNALS: SIGHUP, SIGINT
	     */

		/* Give us a reasonable chance to complete without interruptions */
		oldsighup = signal(SIGHUP, SIG_IGN);
		oldsigint = signal(SIGINT, SIG_IGN);

		/* Write the new setuname "rc" file */
		/*NOTE: whenever a cleanup is done to eliminate S11uname in */
		/*rc2.d, please include -t option for setuname cmd in this rc file */

		if (*sysnm || *nodenm) {
			if ((fd = fopen(RC_FILENAME, "w")) != (FILE *) NULL) {
				fprintf(fd, "# %s - %s\n", sysnm, nodenm);
				fprintf(fd, "#\n");
				fprintf(fd, "# This script, generated by the setuname command,\n");
				fprintf(fd, "# sets the system's system-name and nodename\n");
				fprintf(fd, "#\n");
				fprintf(fd, "setuname ");
				if (*sysnm)
					fprintf(fd, "-s %s ", sysnm);
				if (*nodenm)
					fprintf(fd, "-n %s ", nodenm);
				fprintf(fd, "\n");
				fclose(fd);
			} else return(rtncd = -1);
		}

		if(nodename != NULL) {
#if (defined (_POSIX_SOURCE) || defined(_XOPEN_SOURCE))
			char curname[_SYS_NMLN];
#else
			char curname[SYS_NMLN];
#endif
			int curlen;
			FILE *file;

			if ((file = fopen("/etc/nodename", "r")) != NULL) {
#if (defined (_POSIX_SOURCE) || defined(_XOPEN_SOURCE))
				curlen = fread(curname, sizeof(char), _SYS_NMLN, file);
#else
				curlen = fread(curname, sizeof(char), SYS_NMLN, file);
#endif
				for (i = 0; i < curlen; i++) {
					if (curname[i] == '\n') {
						curname[i] = '\0';
						break;
					}
				}
				if (i == curlen) {
					curname[curlen] = '\0';
				}
				(void)fclose(file);
			} else {
				curname[0] = '\0';
			}
			if (strcmp(curname, utsname.nodename) != 0) {
				if ((file = fopen("/etc/nodename", "w")) == NULL) {
					(void) fprintf(stderr, "setuname: error in opening /etc/nodename\n");
					exit(1);
				} 
				if (fprintf(file, "%s\n", utsname.nodename) < 0) {
					(void) fprintf(stderr, "setuname: error in writing node name\n");
					exit(1);
				}
				(void)fclose(file);
			}		
		}
		if(sysname != NULL) {
#if (defined (_POSIX_SOURCE) || defined(_XOPEN_SOURCE))
			char curname[_SYS_NMLN];
#else
			char curname[SYS_NMLN];
#endif
			int curlen;
			FILE *file;

			if ((file = fopen("/etc/systemid", "r")) != NULL) {
#if (defined (_POSIX_SOURCE) || defined(_XOPEN_SOURCE))
				curlen = fread(curname, sizeof(char), _SYS_NMLN, file);
#else
				curlen = fread(curname, sizeof(char), SYS_NMLN, file);
#endif
				for (i = 0; i < curlen; i++) {
					if (curname[i] == '\n') {
						curname[i] = '\0';
						break;
					}
				}
				if (i == curlen) {
					curname[curlen] = '\0';
				}
				(void)fclose(file);
			} else {
				curname[0] = '\0';
			}
			if (strcmp(curname, utsname.sysname) != 0) {
				if ((file = fopen("/etc/systemid", "w")) == NULL) {
					(void) fprintf(stderr, "setuname: error in opening /etc/systemid\n");
					exit(1);
				} 
				if (fprintf(file, "%s\n", utsname.sysname) < 0) {
					(void) fprintf(stderr, "setuname: error in writing system name\n");
					exit(1);
				}
				(void)fclose(file);
			}		
		}
	    /* Restore signal handling */
		(void) signal(SIGHUP, oldsighup);
		(void) signal(SIGINT, oldsigint);
	}	/* if (!temp) */

	/* Fini */
	return(rtncd);
}
