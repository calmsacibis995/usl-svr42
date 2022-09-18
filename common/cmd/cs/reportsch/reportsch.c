/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:reportsch/reportsch.c	1.14.2.2"
#ident  "$Header: reportsch.c 1.2 91/06/26 $"

/* Reportscheme Network Service
 *
 * Reportscheme is a non-standing service whose address is managed
 * by a port monitor.  The connection server daemon forks a child
 * to process a client application request and this child calls
 * reportscheme on a given port monitor and sends it the name of the
 * network service the client wants to execute.  The reportscheme
 * service receives this name, finds the service entry in the _pmtab
 * and returns the authentication scheme listed there as well as a
 * flag for whether the network service will invoke the auth. scheme
 * as the imposer or the responder.  If no authentication scheme is
 * listed for the service in the _pmtab, a NULL auth. scheme is
 * assumed.  If the service name is not listed in the _pmtab 
 * reportscheme will write back a -1, on success reportsheme writes 
 * back 1:schem_name:imposer/responder_flag.
 *
 * Inheritable Privileges: dacread,dacwrite,dev,macread,macwrite,
 *                         setplevel,driver,fsysrange
 *       Fixed Privileges: None
 */

#define LOGFILE	"/var/adm/log/cs.log"
#define DBGFILE	"/var/adm/log/cs.debug"	
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <stropts.h>
#include <sys/siginfo.h>
#include <time.h>
#include <sys/types.h>
#include <mac.h>
#include <errno.h>
#include <pfmt.h>
#include <locale.h>

static	int	get_auth(char *, char **, char **);
static	void	opendebug(void);
static	void	debug(char *);

/* in order for DUMP to work properly "x" must be of the form 
 *      (msg, "string_with_printf_options", [args])
 * where args is optional
 */
#define DUMP(x) if (Debugging) {sprintf x; debug(msg);}
#define MSGSIZE	512     		/* size of scratch buffer */
#define PMADM	"/usr/sbin/pmadm"

static	char	msg[MSGSIZE];		/* error message buffer */
static	FILE	*Dfp;			/* debug file */
static	int	Debugging=0;

void
main(argc, argv)
int   argc;
char *argv[];
{
	char 	reqname[BUFSIZ]; 
	char 	*reqtransport;
	char 	*reqportmon;
	char	temp_buffer[BUFSIZ];
	char	cmdline[BUFSIZ];
	char	*authscheme;
	char	*role;
	FILE	*fs_popen;
	int	logfd, i,c;
	char	buf[BUFSIZ];
	extern 	int optind;
	level_t level;

        /* internationalization information */
        (void)setlocale(LC_ALL, "");
        setcat("uxnsu");
        setlabel("UX:rs");

	while ((c = getopt(argc, argv, "d")) != -1)
		switch (c) {
		case 'd':
			Debugging = 1;
			break;
		default:
			pfmt(stderr, MM_ACTION, ":133:Usage: reportscheme [-d]\n");
			exit(1);
		}

	if (optind != argc) {
		pfmt(stderr, MM_ACTION, ":133:Usage: reportscheme [-d]\n");
		exit(1);
	}

    /*
     *	Exit unsuccessfully if you can't get the service name
     *	or the provider...
     */

	if (Debugging)
		opendebug();

	/* if sec. run at the same level as pmadm (SYS_PRIVATE) */
	devstat((char *)NULL, 0, (struct devstat *)NULL);
	if (!((errno==ENOSYS) || (errno==ENOPKG))) {
		if (lvlfile(PMADM, MAC_GET, &level) != 0) {
			DUMP((msg,"rs: can't do lvlfile, errno=%d", errno));
			exit(1);
		}
		DUMP((msg,"rs: pmadm level = %d", level));
		if (lvlproc(MAC_SET, &level) != 0) {
			DUMP((msg,"rs: can't do lvlproc, errno=%d", errno));
			exit(1);
		}
		DUMP((msg,"rs: reportscheme level changed to = %d", level));
	}

	if (ioctl(0,I_PUSH, "tirdwr") < 0) {
		DUMP((msg, "rs: can't push tirdwr"));
		exit(1);
	}

	if (read(0, reqname, BUFSIZ) < 0) {
		DUMP((msg, "rs: can't read"));
		exit(1);
	} 

	if ((reqportmon=getenv("NLSPROVIDER")) == NULL) { 
		DUMP((msg, "rs: NLSPROVIDER can't get env"));
		exit(1); 
	}	
	
	if ((reqtransport=getenv("PMTAG")) == NULL) { 
		DUMP((msg, "rs: PMTAG can't get env"));
		exit(1); 
	} 

    	/* Determine authentication scheme and role for
     	 * requested service.
     	 */

	sprintf(cmdline, "%s -L -p %s -s %s",
		PMADM, reqtransport, reqname); 
	DUMP((msg, "rs: calling <%s>", cmdline));

	/* listener dups 0, 1, 2 so we must close 2 in order to
	   avoid error messages being written across pipe */
	(void) close(2);
	if (( fs_popen = popen( cmdline, "r")) != NULL) {
	    if( fgets( temp_buffer, BUFSIZ, fs_popen) == NULL) {

		/* service name not listed in given pmtab */
			
		DUMP((msg, "rs: fgets returns null"));
		write(0, "-1::", 5); 
		sleep(10);
		exit(0);
	    } else {
    	        if (get_auth(temp_buffer, &authscheme, &role) != -1) {
		    if (authscheme != NULL) {
		        if (strcmp(authscheme,"") == 0)
			    authscheme = "0";
		        sprintf(buf, "1:%s:%s", authscheme,role);
			DUMP((msg, "rs: wrote buffer <%s>",buf));
		        write(0, buf, strlen(buf) + 1);
		        sleep(10);
		        exit(0);
		    }
		}
	    }
    } else { 		/* server messed up; return error */

	DUMP((msg,"rs: popen returns null"));
	write(0, "0::", 4);
	sleep(10);
	exit(1);
    }	
}



/*
 * Get the name of a authentication scheme and service for a
 * given pmtab entry.
 * Report reverse of role in _pmtab back to client Connection Server. 
 * Role reportscheme returns is the role that the client stores in 
 * internal cache.
 * Set role to default (r) if no role was specified.
 */

static
int
get_auth(buffer, authscheme, role )
char    *buffer;        /* Pointer to a typical pmtab entry */
char    **authscheme;
char 	**role;
{
	char 	*authstr;
	char 	*pp = buffer;
	int     count = 0;

	*role = (char *)strdup("r");	/* default value of role */

        while (1) {
                if (*pp++ == ':')
                        count++;
                if (count == 7)
                        break;
        }
        authstr=(char *)strdup(pp);
	pp = authstr;

        /* parse scheme & role */
	while( !isspace(*pp)  &&  *pp != ':')  {
		pp++;
	}

	if (isspace(*pp)) {
		*pp = '\0';
		pp++;
		while (*pp != ':') {
			/* this is faster than many calls to strncmp */
			if (*pp == '-'  &&  *(++pp) == 'r'  &&
			    (isspace(*(pp+1)) || *(pp+1) == ':')) {
				/* return reverse of role */
				*role = (char *)strdup("i");
				break;
			}
			if (*pp == '\\') 
				pp++;
			if (*pp == '"') 
				for (pp++; *pp != '"'; pp++);
			pp++;
		}

	}
	*pp = '\0';
	*authscheme = (char *)strdup(authstr);
	DUMP((msg,"rs: authscheme<%s>", *authscheme));
	DUMP((msg,"rs: role:%s",*role));
	return(0);
}



/*
 * opendebug - open debugging file, sets global file pointer Dfp
 */

static
void
opendebug()
{
	FILE *fp;	/* scratch file pointer for problems */

	Dfp = fopen(DBGFILE, "a+");
	if (Dfp == NULL) {
		fp = fopen("/dev/console", "w");
		if (fp) {
			pfmt(fp, MM_ERROR, ":132:could not open debug file %s\n",
                             DBGFILE);
		}
		exit(1);
	}
	setbuf(Dfp,NULL);
}


/*
 * debug - put a message into debug file
 *
 *	args:	msg - message to be output
 */

static
void
debug(msg)
char *msg;
{
	char 	*timestamp;	/* current time in readable form */
	long 	clock;		/* current time in seconds */
	char 	buf[MSGSIZE];	/* scratch buffer */
	struct 	tm *tms;

	(void) time(&clock);
	tms = localtime(&clock);
        (void) sprintf(buf,"%02d:%02d:%02d; %5ld; %s\n",
            tms->tm_hour, tms->tm_min, tms->tm_sec, getpid(),msg);
        (void) fprintf(Dfp, buf);
}
