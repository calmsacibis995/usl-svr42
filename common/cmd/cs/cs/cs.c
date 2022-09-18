/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/cs.c	1.32.2.11"
#ident  "$Header: cs.c 1.2 91/06/26 $"

/*
 * Connection Server Daemon
 *
 * This daemon runs on all client machines and is used to establish
 * connections for all network services that communicate over TLI
 * connection-oriented and dialup connections.  This daemon receives
 * requests for network services from client applications that use the
 * cs_connect() library interface, then establishes connetions to the 
 * server-machine ports associated with the requested services, it may
 * invoke an authentication scheme, and lastly passes an fd and error 
 * information back to the calling application.
 *
 * Inheritable Privileges: compat,dacread,dacwrite,dev,driver,filesys,
 *			   fsysrange,macread,macupgrade,macwrite,
 *			   mount,owner,setflevel,setuid,sysops
 *       Fixed Privileges: None
 */

#include <fcntl.h>
#include <stdlib.h>
#include <sys/siginfo.h>
#include <sys/resource.h>
#include <sys/secsys.h>
#include <signal.h>
#include <sys/procset.h>
#include <sys/stropts.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <tiuser.h>
#include <netconfig.h>
#include <netdir.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#include <dial.h>
#include <priv.h>
#include <mac.h>
#include <pfmt.h>
#include <locale.h>
#include "global.h"
#include "extern.h"

static	struct 	t_call *sndcall; 	/* used to t_connect */
static	struct 	s_strrecvfd *recvfd;	/* for I_S_RECVFD ioctl */
static	struct 	netconfig *netconfigp;	/* holds transport info */
static	struct 	netconfig call_nc;	/* caller's netconfig */
static	struct 	netconfig *call_ncp;	/* caller's netconfig */
static	struct 	netbuf call_netbuf;	/* calling routine's netbuf */
static	struct 	netbuf *call_netbufp;	/* calling routine's netbuf */
static	int	conn_error;		/* connection error */
static	int 	option;			/* netdir_options arg */
extern	int	t_errno;		/* tli error */

/* If MAC is installed the running system may either have the MAC
 * module included (Mac_running == TRUE) or not.  If Mac_running == 
 * FALSE then EAN_running may be either TRUE or FALSE depending on
 * whether or not the running system is 4.0EAN. 
 */
static	int	Mac_running=FALSE;	/* true if MAC running */
static	int	Ean_running=FALSE;	/* true if on 4.0EAN */
static	level_t Client_level=0;		/* client's sec. level */
static	char	Level_alias[STRSIZE];	/* client's level alias */
static	int	Read_cache_file=FALSE;	/* true if demon should read
					   the CACHEFILE file */
static	int	Read_auth_file=FALSE;	/* true if demon should read
					   the AUTHFILE file */

/* Function prototypes
 */
static	int	update_auth_info(void);
static	int	mk_cs_pipe(void);
static	void	initialize(void);
static	void	sigalarm(int);
static	void	sigcache(int);
static	int	check_ean_privs(char *);
static	int	netdir_privs(void);
static	int	get_next_net(void);
static	int	tli_check_scheme(int *, int *, struct  schemeinfo **);
static	int	tli_addr_connect(struct  nd_addrlist *, int *);
static	int	do_listen_service(int, char *);
static	int	tli_auth(int *, int *, struct  schemeinfo *, int);
static	void	debug_dial(void);
static	void	dial_connect(void);
static	void	free_schemeinfo(struct schemeinfo *);
static	int	read_request(void);
static	void	write_status(int, int);
static	int	set_levels(int);
static	void	hangup(int);

void	log_cs();
void	log_status(int);

void
main(argc, argv)
int argc;
char *argv[];
{
	int	CSfd;		/* mounted streams pipe fd */
	int	requestype;	/* request type */
	extern 	int optind;
        int     subsize;        /* size of sub structure */
	int	recv_type;	/* type of ioctl we'll do */
	int 	c;
	int	size;

	/* internationalization information */
	(void)setlocale(LC_ALL, "");
	setcat("uxnsu");
	setlabel("UX:cs");

	while ((c = getopt(argc, argv, "dx")) != -1) {
		switch (c) {
		case 'd':
			Debugging = 1;
			Debug =9;
			Verbose = 1;
			break;
		case 'x':
			exit(update_auth_info());
			break;
		default:
			pfmt(stderr, MM_ACTION, ":128:Usage: cs [-d]\n");
			exit(-1);
		}
	}

	if (optind != argc) {
		pfmt(stderr, MM_ACTION, ":128:Usage: cs [-d]\n");
		exit(-1);
	}

	initialize();
	CSfd = mk_cs_pipe();

        /* Get size of the subjects attributes structure */
        if ((subsize = secadvise(0, SA_SUBSIZE, 0)) < 0) {
		if (errno == ENOPKG) {
			Ean_running = TRUE;
			recv_type = I_RECVFD;
			if ((recvfd = (struct s_strrecvfd *)
			    malloc(sizeof(struct s_strrecvfd))) == 
			    (struct s_strrecvfd *)NULL) {
			        CS_LOG((msg,"malloc error in cs daemon"));
			}
		} else {
			CS_LOG((msg, "secadvise error in cs daemon"));
			exit(-1);
		}
	} else {
		recv_type = I_S_RECVFD;
        	if ((recvfd = (struct s_strrecvfd *) malloc(
		     sizeof(struct s_strrecvfd) + 
		     subsize - sizeof (struct sub_attr))) ==
		     (struct s_strrecvfd *)NULL) {;
			  CS_LOG((msg, "malloc error in cs daemon"));
		}
	}

        recvfd->fd = 0;

	/* Wait for connld on named stream to SENDFD; hence
	*  creating a unique connection between CS daemon and 
	*  CS library interface
	*/


	while (1) {
	    if (ioctl(CSfd, recv_type, recvfd)== 0) {
	
		DUMP((msg,"cs: conn done; client talks to server"));
		netfd = recvfd->fd;
		DUMP((msg,"cs: fd to read request from: %d",netfd));

		if (Read_cache_file == TRUE) {
			update_cache_list(CACHEFILE);
			Read_cache_file = FALSE;
		}

		if (Read_auth_file == TRUE) {
			update_cache_list(AUTHFILE);
			Read_auth_file = FALSE;
		}

		/* children will not be zombies */
		signal(SIGCLD,SIG_IGN);
		
		/* fork a child to handle request */
		
		switch(fork()) {
		case 0:		/* process child request */
			DUMP((msg,"cs: connection server child forked"));
			break;
		case -1:
			CS_LOG((msg,"cs: fork failed; errno=%d",errno));
			write_status(CS_FORK, CS_EXIT);
			break;
		default:	
			/* parent waits for requests; close old conn */
			close(netfd); 
			continue;
		} /*switch */		

		break;	/* All children break out of while loop here
			 * Parent continues waiting on ioctl(RECVFD).
			 */
	
	    } else {
		if (errno == EINTR) {
			if (Read_cache_file == TRUE) {
				update_cache_list(CACHEFILE);
				Read_cache_file = FALSE;
			} else if (Read_auth_file == TRUE) {
				update_cache_list(AUTHFILE);
				Read_auth_file = FALSE;
			} else {
				CS_LOG((msg,"unexpected EINTR in ioctl"));
			}
		} else {
			CS_LOG((msg,"ioctl receive fd error; errno=%d", errno));
		}
	    }

	} /* while */

	/*
	* Children continue processing here 	
	* Ask the kernel to send the SIGPOLL signal,
	* when the connection client exits.
	* Read connection request type until receive one of the 
	* following:
	*    TLI_REQUEST DIAL_REQUEST CS_READ_AUTH
	* Skip any garbage that any application may have written over pipe 
	*/

	do { 
		size = sizeof(int);
		if (read(netfd, &requestype, size) != size) {
			write_status(CS_SYS_ERROR, CS_EXIT);
		}
	} while ((requestype != TLI_REQUEST) && 
		 (requestype != DIAL_REQUEST) &&
		 (requestype != CS_READ_AUTH));

	if (requestype == CS_READ_AUTH) {
		/* take care of reading the auth file and exit */
		DUMP((msg,"cs: request-type: CS_READ_AUTH %s",AUTHFILE));
		sigsend(P_PID, getppid(), SIGUSR2);
		exit(0);
	}

	setpgid(0,0);
        signal(SIGPOLL, hangup);

	if (ioctl (netfd, I_SETSIG, S_HANGUP) != 0)
	{
		CS_LOG((msg,"cs: ioctl set signal error; errno=%d", errno));
		exit(-1);
	}

	/* get client's level */
	if (Mac_running) {
		if(flvlfile(netfd, MAC_GET, &Client_level) != 0) {
			CS_LOG((msg,"flvlfile() failed, errno=%d",errno));
			exit(-1);
		}
	} 

	DUMP((msg,"cs: request-type: %s", requestype==TLI_REQUEST?
	      "TLI_REQUEST": "DIAL_REQUEST"));

	switch(requestype){
	case DIAL_REQUEST:		/* process child request */
		(void)dial_connect();
		break;
	case TLI_REQUEST:

		/* tli_connect() routine was modified here to accomodate 
		   bnu calling tli_connect() for a dial-out TLI connection.
		   If dial is ever removed from the Connection Server
		   1) put the read_request and write_status calls (see below)
		   back into tli_connect() and 2) make netfd and returnfd 
		   static ints again.  Netfd and returnfd were made global ints
		   to accomodate dial and bnu.
		*/
		 	

		if((conn_error = read_request()) != CS_NO_ERROR) {
			write_status(conn_error, CS_EXIT);
		};

		returnfd = tli_connect();
		write_status(conn_error, CS_EXIT); 
		break;
	default:
		/* this cannot happen */
		write_status(CS_TIMEDOUT,  CS_EXIT); 
		break;
	} /*switch */
	
	exit(0);			/* child process exits */
}

/*
 * write to the CSPIPE to tell the daemon to read the AUTHFILE
 */
static
int
update_auth_info()
{
	int	msg = CS_READ_AUTH;
	int	msg_size;
	int	fd;

	if ((fd = open(CSPIPE, O_RDWR)) == -1) {
		return -1;
	}

	/* check that the stream to the daemon has been set up */
	if (isastream(fd) == 0) {
		close(fd);
		return -1;
	}

	msg_size = sizeof(msg);
	if (write(fd, &msg, msg_size) != msg_size) {
		close(fd);
		return -1;
	}

	if (close(fd) != 0) {
		return -1;
	}

	return 0;
}


/*
 * mk_cs_pipe - create the command pipe used by the connection server
 */

static 
int
mk_cs_pipe()
{
	int scratchfd;			/* scratch file descriptor */
	int fds[2];			/* pipe endpoints */
	/* fds[0] - CS end of request pipe */
	/* fds[1] - client end of request pipe */

	/* this call to unlink() prevents multiple cs daemons from 
	 * trying to link to this pipe 
	 */
	(void) unlink(CSPIPE);
	scratchfd = open(CSPIPE, O_RDWR | O_CREAT, 0666);
	if (scratchfd < 0) {
		CS_LOG((msg, "cs: Unable to create %s pipe, errno=%d",
			CSPIPE, errno));
		exit(-1);
	}
	chmod (CSPIPE, 0666);
	close(scratchfd);
		
	DUMP((msg,"cs: created CS pipe: %s", CSPIPE));

	if (pipe(fds) < 0) {
		CS_LOG((msg,"cs: Bad fd from CS pipe open, errno=%d",
			errno));
		exit(-1);
	}
	if (ioctl(fds[1], I_PUSH, "connld")) {
		CS_LOG((msg, "cs: Unable to PUSH connld, errno=%d",
			errno));
		exit(-1);
	}
	
	if (fattach(fds[1], CSPIPE) < 0){
		CS_LOG((msg, "fattach on CS pipe failed, errno=%d",
			errno));
		exit(-1);
	}

	close(fds[1]);
	return(fds[0]);
}

static 
void
initialize()
{
	struct  rlimit	rl;
	char	*netpath;
	int	i;

	/* Disassociate from controlling terminal and process group */
	if (fork() != 0) {
		exit(0);	
	}

	/* set up signals to catch */
	sigset(SIGUSR1, sigcache);
	sigset(SIGUSR2, sigcache);

	/* Ensure the process is a process group leader */
	setpgrp();

	/* any file descriptors to close ? */
	if ( getrlimit(RLIMIT_NOFILE, &rl) != 0 ) {
		pfmt(stderr, MM_ERROR, ":129:getrlimit failed\n");
		exit (-1);
	}
	
	for (i = 0; i < rl.rlim_cur; i++) 
		close(i);		

	/* determine if MAC is running */
	errno = 0;
	devstat((char *)NULL, 0, (struct devstat *)NULL);
	Mac_running = (!( (errno == ENOSYS) || (errno == ENOPKG)));

	openlog();
	log("*** CONNECTION SERVER starting ***");

	if (Debugging){
		opendebug();
		debug("cs: Debugging turned on");
	}

	/* Log message if the NETPATH environment variable is set
	 * for the daemon 
	 */
	if ((netpath = getenv(NETPATH)) != NULL) {
		CS_LOG((msg, "WARNING: NETPATH env. var. set to <%s>",
			netpath));
		CS_LOG((msg, "TO FIX: type \"unset NETPATH\" before running cs"));
	}

	/*move current directory off mounted filesystem*/
	if (chdir(ROOT) < 0) {
		CS_LOG((msg, "chdir(ROOT) failed"));
		exit(-1);
	}
	DUMP((msg,"cs: chdir to ROOT"));

	/* read in system administrator authentication file */
	update_cache_list(AUTHFILE);
}

/*
 * sigalarm:  Catch timeout signals.  If t_open times out sigalarm 
 * sends status message back over the  CS pipe to the application.  
 * The status message contains an error datum indicating connect 
 * failure was due to a timeout.
 */

static
void
sigalarm(dummy)
int	dummy;
{
	write_status(CS_TCONNECT, CS_EXIT);
}


/*
 * sigcache:  Catch signal SIGUSR1 which tells the daemon to
 * read it's temp. cache (CACHEFILE).
 */

static
void
sigcache(sig_type)
int	sig_type;
{
	DUMP((msg,"cs: daemon signaled to read %s", 
	      sig_type== SIGUSR1? CACHEFILE: AUTHFILE));
	if (sig_type== SIGUSR1) {
		Read_cache_file = TRUE;
	} else {
		Read_auth_file = TRUE;
	}
}

/*	For the EAN package secadvise will not work so the DAC
 *	check must be done with this routine.
 */

static
int
check_ean_privs(dev_path)
char *dev_path;
{
	struct	stat	sbuf;

	if (stat(dev_path, &sbuf) != 0) {
		CS_LOG((msg, "couldn't get stat info, errno=%d",
			errno));
		return (CS_SYS_ERROR);
	}

	if (recvfd->uid == sbuf.st_uid) {
		return (((sbuf.st_mode & 0600) == 0600)? 
			CS_NO_ERROR: CS_DAC);
	} else if (recvfd->gid == sbuf.st_gid) {
		return (((sbuf.st_mode & 060) == 060)? 
			CS_NO_ERROR: CS_DAC);
	} else {
		return (((sbuf.st_mode & 006) == 006)? 
			CS_NO_ERROR: CS_DAC);
	}
}

int
check_device_privs(dev_path)
char *dev_path;
{
	struct  dev_alloca  devdata;
	struct	stat       sbuf;
        struct  obj_attr   obj;

	/* sacadvise won't work in 4.0EAN so call this instead */
	if (Ean_running) {
		return(check_ean_privs(dev_path));
	}

	/* check that client's level is within dev's level range */
	if (Mac_running) {
		if (devalloc(dev_path, DEV_GET, &devdata) != 0){
			DUMP((msg, "couldn't get dev info in devdb"));
			DUMP((msg, "dev_path = <%s>",dev_path));
			return (CS_SYS_ERROR);
		}
		if (lvldom(&Client_level, &(devdata.lolevel)) <= 0) {
			DUMP((msg,"client lvl %d doesn't dominate dev lolvl %d",
			      Client_level, devdata.lolevel));
			return (CS_MAC);
		}
		if (lvldom(&devdata.hilevel, &Client_level) <= 0) {
			DUMP((msg,"dev hilvl %d doesn't dominate client lvl %d",
			      devdata.hilevel, Client_level));
			return (CS_MAC);
		}
	}

	if (stat(dev_path, &sbuf) != 0) {
		DUMP((msg, "couldn't do stat, errno=%d", errno));
		return (CS_SYS_ERROR);
	}
	obj.uid = sbuf.st_uid;
        obj.gid = sbuf.st_gid;
        obj.mode = sbuf.st_mode;
	/* already checked level so use client level */
        obj.lid = Client_level;        

	/* check that the subject (client) can access the obj (dev) */
        if(secadvise(&obj, SA_WRITE, &(recvfd->s_attrs)) < 0) {
		DUMP((msg, "cs: client DAC can't write to dev"));
		return (CS_DAC);
        }
        if(secadvise(&obj, SA_READ, &(recvfd->s_attrs)) < 0) {
		DUMP((msg, "cs: client DAC can't read from dev"));
		return (CS_DAC);
        }

	/* success */
	DUMP((msg,"cs: client passes MAC/DAC check for device"));
	return (CS_NO_ERROR);
}


/*
*	Check if the caller has privilege to perform netdir_options.
*	For BSD compatibility, only root can bind to a reserved port.
*	If caller wants a reserved port, check if the caller is
*	root.  If caller specifies a port, check that either the
*	port is not reserved, or that the caller is root.
*/

static
int
netdir_privs()
{
	int is_root=(recvfd->uid == 0); 	/* is caller root? */

	/* If caller wants reserved port, check if caller is root */
	if (option == ND_SET_RESERVEDPORT) {
		if (is_root) {
			return TRUE;
		} else {
			DUMP((msg,"cs: non-root requesting reserved port"));
			return FALSE;
		}
	}

	/* If caller wants a specific port check that either the
	 * the caller is root, or that port is not reserved.
	 */
/* ONLY ROOT ALLOWED TO REQUEST RESERVED PORT -- don't exec this code */
/*
	if (call_netbuf.buf != NULL) {
		DUMP((msg,"cs: call_netbuf.buf != NULL"));
		if (is_root  ||  
		    netdir_options(NULL, ND_CHECK_RESERVEDPORT, 
		    0, (char *)&call_netbuf) != 0 ) {
			return TRUE;
		} else {
			DUMP((msg,"cs: non-root requesting reserved port"));
			return FALSE;
		}
	}
*/

	return TRUE;
}

/* Determines if another netconfig structure should be retrieved.
 * Returns true if there are any differences between the caller's
 * netconfig elements and the current netconfig.  Only non-NULL
 * values are checked since a NULL indicates it was not set by the
 * caller.
 */

static
int
get_next_net()
{
	struct	netconfig *ncp = netconfigp;
	int	problem_count = 0;

	if (call_ncp == NULL)
		return FALSE;
	if (call_ncp->nc_netid  &&  
	    strcmp(ncp->nc_netid, call_ncp->nc_netid) != 0)
		problem_count++;
	if (call_ncp->nc_semantics  &&  
	    ncp->nc_semantics != call_ncp->nc_semantics)
		problem_count++;
	if (call_ncp->nc_flag  &&  
	    ncp->nc_flag != call_ncp->nc_flag)
		problem_count++;
	if (call_ncp->nc_protofmly  &&  
	    strcmp(ncp->nc_protofmly, call_ncp->nc_protofmly) != 0)
		problem_count++;
	if (call_ncp->nc_proto  &&  
	    strcmp(ncp->nc_proto, call_ncp->nc_proto) != 0)
		problem_count++;

	return problem_count==0? FALSE: TRUE;
}

/*
 */

static
int
tli_check_scheme(local, next, schemepp)
int *local; 			/* schemeinfo in local cache? */
int *next;
struct 	schemeinfo **schemepp;
{
	if (*schemepp == NULL) {
		if (*local) {
			*local = FALSE;
			*next = TRUE;
			return (CS_NO_ERROR);
		} else {
			/* Since !local, assume no auth scheme needed */
			DUMP((msg,"cs: assuming NULL auth scheme"));

			if ((*schemepp = (struct schemeinfo *)
			      malloc(sizeof(struct schemeinfo))) == NULL) 
				return (CS_MALLOC);
			(*schemepp)->s_name = NULL;
			(*schemepp)->s_flag = 0;

		} 
	} 

	DUMP((msg,"cs: obtained authscheme <%s>...",
	      (*schemepp)->s_name? (*schemepp)->s_name: "NULL"));

	/* Check to make sure that the scheme is acceptable to client */

	if (!checkscheme((*schemepp)->s_name, Nd_hostserv.h_host, 
	     Nd_hostserv.h_serv)) {

		DUMP((msg,"cs: authscheme <%s> is NOT acceptable!\n",
		      (*schemepp)->s_name? (*schemepp)->s_name: "NULL"));
		free_schemeinfo(*schemepp);
		*schemepp = NULL;

		/* If we got the info from the local cache, try again 
		 * (but this time get the auth info from the server).
		 */
		if (*local) {
			*local = FALSE;
			*next = TRUE;
			return (CS_NO_ERROR);
		}

		return (CS_AUTH);
	}

	DUMP((msg,"cs: authscheme IS acceptable"));
	*next = FALSE;
	return (CS_NO_ERROR);
}

/*
 */

static
int
tli_addr_connect(list, rfd)
struct 	nd_addrlist *list; 		/* holds address info */
int	*rfd;
{
	struct  netbuf *netbufp;      	/* holds transport info */
	int return_error;
	int i;
	int fd;

	netbufp = list->n_addrs;
	return_error = CS_CONNECT;

	for (i = 0; i < list->n_cnt; i++, netbufp++) {
		/* Open the device and bind to a return address...  */
		(void) signal(SIGALRM, sigalarm);
		(void) alarm(ALARMTIME);
		if((fd = t_open(netconfigp->nc_device, O_RDWR, NULL)) == -1) { 
			DUMP((msg,"cs: t_open failed; t_errno=%d", t_errno));
			return_error = CS_TOPEN; 
			continue;
		}
		(void) alarm(0);
		
		if (option == ND_SET_RESERVEDPORT) {
			if (netdir_options(netconfigp, option,fd,
			    (char *)call_netbufp) != 0) {
				DUMP((msg,"cs: netdir_options failed"));
				t_close(fd);
				return_error = CS_TBIND; 
				continue;
			}
		} else {
			if(t_bind(fd, NULL, NULL) == -1) {
				DUMP((msg,"cs: t_bind failed; t_errno=%d",
				      t_errno));
				t_close(fd);
				return_error = CS_TBIND; 
				continue;
			}
		}

		/* Connect to the address given in netbufp. */
		if((sndcall = (struct t_call *)t_alloc(fd,T_CALL,T_ADDR)) == NULL) {
			DUMP((msg,"cs: t_alloc failed; t_errno=%d",
			      t_errno));
			t_close(fd);
			return_error = CS_TALLOC; 
			continue;
		}

		(void) memcpy(sndcall->addr.buf, netbufp->buf, netbufp->len);
		sndcall->addr.len = netbufp->len;
		sndcall->opt.len = 0;	/* default options are used */
		sndcall->udata.len = 0;

		if (t_connect(fd, sndcall, (struct t_call *)NULL) != 0) {
			DUMP((msg,"cs: t_connect failed"));
			t_free(sndcall, T_CALL);
			t_close(fd);
			return_error = CS_TCONNECT; 
			continue;
		}

		return_error = CS_NO_ERROR;
		break;
	} /* for */

	*rfd = (return_error == CS_NO_ERROR)? fd: -1;
	return (return_error);
}

/*
 * do_auth()
 *	this function formats the command line options for an
 *	invoke() library function call. Any arguments provided
 *	with the scheme parameter are passed through transparently.
 * input
 *	int fd - network file descriptor
 *	char *scheme - scheme name (may include arguments)
 *	char *role - "-r" for responder role
 */
int
do_auth(int fd, char *scheme, char *role)
{
	int 	rval=0;		/* return value from invoke */
	char	buf[BUFSIZ*2];
	char	level_buf[BUFSIZ];
	char	lvl_name[BUFSIZ];
	level_t	level;

	level_buf[0] = '\0';
	if (lvlproc(MAC_GET, &level) == 0) {
		if ( (flvlfile(netfd, MAC_GET, &level) != 0)
		   ||(lvlout(&level,lvl_name,BUFSIZ,LVL_ALIAS) != 0) )
			return(CS_MAC);
		sprintf(level_buf, "-l %s", lvl_name);
	} else if ((errno != ENOPKG) && (errno != ENOSYS))
		return(CS_MAC);

	sprintf(buf, "%s %s %s -u %s -M %s -S %s",
        	scheme, role, level_buf,
		getpwuid(recvfd->uid)->pw_name,
		Nd_hostserv.h_host, Nd_hostserv.h_serv);

	DUMP((msg, "cs: invoke called with:%s", buf));

	/* We should set the device so the scheme can access it
	 * without privilege. This is where it could be done.
	 * Until then, the scheme should inherit MACREAD, MACWRITE,
	 * DEV privileges.
	 */

	if ((rval = invoke(fd, buf)) != 0) {
		CS_LOG((msg,"ERROR: invoke failed error==%d",rval));
		if (rval == 29) {
			CS_LOG((msg, "Ensure both cs and keymaster run under the same id"));
		}
		return(CS_INVOKE);
	} else {
		DUMP((msg, "cs: invoke() succeeded"));
		return(CS_NO_ERROR);
	}
}

/*
 */

static
int
tli_auth(local, next, schemep, rfd)
int	*local;
int 	*next;
struct  schemeinfo *schemep;
int	rfd;
{
	if (do_auth(rfd, schemep->s_name,
			 schemep->s_flag == AU_RESPONDER ? "-r" : "") 
			 != CS_NO_ERROR) {
		if (*local) {
			*local = FALSE;
			*next = TRUE;
			t_close(rfd);
			return(CS_NO_ERROR);
	 	} else {
			*next = FALSE;
			return(CS_INVOKE);
		}
	} else {
		*next = FALSE;
		return(CS_NO_ERROR);
	}
}

/*
 */
static
int
do_listen_service(rfd,listen_service)
int	rfd;
char	*listen_service;
{
	struct	schemeinfo *schemep;
	int	next_scheme=TRUE;
	int	local=TRUE;
	char	buf[BUFSIZ];
	int	bsize;

	sprintf(buf, "NLPS:000:001:%s", listen_service);

	/* write string and trailing blank to listener */
	bsize = strlen(buf) + 1;
	if (t_snd(rfd, buf, bsize, 0) != bsize) {
		CS_LOG((msg,"t_snd to listen failed, t_errno=%d", 
			t_errno));
			return(conn_error);
	}

	DUMP((msg,"cs: now changing service from <%s> to <%s>",
	      Nd_hostserv.h_serv, listen_service));
	Nd_hostserv.h_serv = listen_service;

	DUMP((msg,"cs: checking schemes for service <%s>", 
	      Nd_hostserv.h_serv));

	while (next_scheme) {
		next_scheme = FALSE;
		if ((conn_error = getscheme(Nd_hostserv.h_host,
       		     listen_service, netconfigp, local,
		     &schemep)) != CS_NO_ERROR)
			return(conn_error);
	
		if ((conn_error = tli_check_scheme(&local, 
		    &next_scheme, &schemep)) != CS_NO_ERROR) 
			return(conn_error);

		if (next_scheme == TRUE)
			continue;

		if (schemep->s_name != NULL  && (conn_error = 
		    tli_auth(&local,&next_scheme,schemep,rfd)) != CS_NO_ERROR) {
			CS_LOG((msg,"Listen service's scheme failed"));
			t_close(rfd);
		}
	}
	
	return(conn_error);

}

/* 
 * Ensure that LIDAUTH.map permits request 
 */
static
int
check_LID_auth(host)
char	*host;
{
	char	in_buf[STRSIZE];
	char	out_buf[STRSIZE];

	DUMP((msg,"cs: ensuring that LIDAUTH.map allows connection"));
	if (lvlout(&Client_level, Level_alias, STRSIZE, LVL_ALIAS) < 0) {
		CS_LOG((msg,"lvlout failed, errno = %d", errno));
		CS_LOG((msg,"Level_alias = <%s>", Level_alias));
		return(CS_SYS_ERROR);
	}

	sprintf(in_buf, "%s:%s", Nd_hostserv.h_host, Level_alias);
	return(attrmap("LIDAUTH", in_buf, out_buf) == 0?
	   CS_NO_ERROR: CS_LIDAUTH);
}

/*
*	Processes TLI connection request; invokes authentication
*	scheme.
*/

int
tli_connect()
{
	void	*handlep;	/* handle to net selection */
	int 	i;		/* general index counter */
	int 	rfd;		/* authenticated fd to return */
	struct 	nd_addrlist *adlist; /* holds address info */
	struct 	schemeinfo *schemep;
	int	next_scheme;
	int	local; 		/* schemeinfo in local cache? */

	/* Some services are preceeded by "listen:" which instructs
	 * the daemon to connect first to listen and then ask listen
	 * to invoke NLPS to find the service.  If a request of this
	 * type is received listen_service will contain the name of
	 * the service
	 */

	char	*listen_service = NULL;

	/*
	 *  Set sigalarm as t_open timeout signal handler only for 
	 *  tli_connections; dial() has it's own SIGALRM ( open 
	 *  timout ) mechanisms.
	 */
	
	if (signal(SIGALRM, sigalarm) == SIG_IGN)
		(void)signal(SIGALRM, SIG_IGN);

	/* Ensure that LIDAUTH.map permits request */
	if (Mac_running  &&
	    check_LID_auth(Nd_hostserv.h_host) != CS_NO_ERROR) {
		CS_LOG((msg,"connection not permitted by %s", LIDFILE));
		write_status(CS_LIDAUTH, CS_EXIT);
	} 

	/* Verify privleges for performing netdir operations */
	if (netdir_privs() == FALSE) {
		DUMP((msg,"cs: caller cannot do netdir_options"));
		write_status(CS_NETPRIV, CS_EXIT);
	}

	/*  	Get the authentication scheme needed for the 
	 *	service.  Attempt to get the info from the
	 *	local cache first, and, if that fails, get it
	 *	from the server (by setting "local" to 0).
	 *	If the second call returns NULL, then assume
	 *	no authentication scheme is needed.
	 */
	
	if ((i = get_alias()) == 0) {
		DUMP((msg,"cs: %s in wrong format", SERVEALIAS));
	}

	conn_error = CS_SETNETPATH;
	if ((handlep = setnetpath()) == NULL) {
		CS_LOG((msg,"FAIL: setnetpath() returns NULL"));
		return(-1);
	}

	/* check if the connnection to the service needs to go
	 * through listen, this is indicated by service argument
	 * of the type listen:service_name
	 */

	if (strncmp("listen:", Nd_hostserv.h_serv,7) == 0) {
		listen_service = &Nd_hostserv.h_serv[7];
		Nd_hostserv.h_serv[6] = '\0';
		DUMP((msg,"cs: service <%s> requested through <%s>",
			listen_service, Nd_hostserv.h_serv));
	}

	while ((netconfigp = getnetpath(handlep)) != NULL) {

		/* Skip over conectionless transports...  */
		if (netconfigp->nc_semantics == NC_TPI_CLTS) {
			continue;
		}

		if (call_ncp != NULL  &&  get_next_net()) {
			DUMP((msg,"cs: skipping transport"));
			continue;
		}

		if ((conn_error = 
		     check_device_privs(netconfigp->nc_device))
		     != CS_NO_ERROR) {
			CS_LOG((msg,"MAC/DAC failed, skip transport"));
			continue;
		}

		/* Skip this transport if we can't get the address
		 * of the service...
		 */

		DUMP((msg,"cs: transport <%s>", netconfigp->nc_netid));
		DUMP((msg,"cs: nc_protofmly<%s>", netconfigp->nc_protofmly));
		if (netdir_getbyname(netconfigp, &Nd_hostserv, 
		    &adlist) != 0) {
			CS_LOG((msg,"getbyname failed for <%s>", 
			      netconfigp->nc_netid));
			conn_error = CS_SETNETPATH;
			continue;
		}

		local = TRUE;
		next_scheme = TRUE;
		conn_error = CS_NO_ERROR;
		while (conn_error == CS_NO_ERROR  &&  next_scheme) {
			next_scheme = FALSE;
			if ((conn_error = getscheme(Nd_hostserv.h_host,
	       		     Nd_hostserv.h_serv, netconfigp, local,
			     &schemep)) != CS_NO_ERROR)
				break;

			if ((conn_error = tli_check_scheme(&local, 
			    &next_scheme, &schemep)) != CS_NO_ERROR) 
				break;

			if (next_scheme == TRUE)
				continue;

			if ((conn_error = tli_addr_connect(adlist, &rfd)) 
			     != CS_NO_ERROR) {
				DUMP((msg, "cs: connect failed"));
				break;
			}
		
			if (schemep->s_name == NULL) {
				DUMP((msg,"cs: NULL auth, don't call invoke()"));
				break;
			}

			if ((conn_error = tli_auth(&local,&next_scheme,
			     schemep,rfd)) != CS_NO_ERROR) {
				t_close(rfd);
			}

		}

		if (listen_service!=NULL  &&  conn_error==CS_NO_ERROR) {
			conn_error = do_listen_service(rfd,listen_service);
			if (conn_error != CS_NO_ERROR)
				t_close(rfd);
		}

		if (conn_error == CS_NO_ERROR) {
			endnetpath(handlep); 
			return(rfd);
		}
	}

	log_cs(conn_error);

	endnetpath(handlep); 
	return(-1);
}


/*
 *	Print out returned error value from cs_connect() or cs_dial()
 */

void log_cs(err)
int err;
{
	switch (err) {
	case CS_NO_ERROR:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_NO_ERROR",err));
		break;
	case CS_SYS_ERROR:
		CS_LOG((msg,"ERROR=%s : cs_error=%d: System Error: %s\n","CS_SYS_ERROR",err, strerror(errno)));
		break;
	case CS_DIAL_ERROR:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_DIAL_ERROR",err));
		break;
	case CS_MALLOC:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_MALLOC",err));
		break;
	case CS_AUTH:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_AUTH",err));
		break;
	case CS_CONNECT:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_CONNECT",err));
		break;
	case CS_INVOKE:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_INVOKE",err));
		break;
	case CS_SCHEME:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_SCHEME",err));
		break;
	case CS_TRANSPORT:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_TRANSPORT",err));
		break;
	case CS_PIPE:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_PIPE",err));
		break;
	case CS_FATTACH:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_FATTACH",err));
		break;
	case CS_CONNLD:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_CONNLD",err));
		break;
	case CS_FORK:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_FORK",err));
		break;
	case CS_CHDIR:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_CHDIR",err)); 
		break;
	case CS_SETNETPATH:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_SETNETPATH",err));
		break;
	case CS_TOPEN:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_TOPEN",err));
		break;
	case CS_TBIND:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_TBIND",err));
		break;
	case CS_TCONNECT:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_CONNECT",err));
		break;
	case CS_TALLOC:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_TALLOC",err));
		break;
	case CS_MAC:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_MAC",err));
		break;
	case CS_DAC:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_DAC",err));
		break;
	case CS_TIMEDOUT:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_TIMEDOU",err));
		break;
	case CS_NETPRIV:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_NETPRIV",err));
		break;
	case CS_NETOPTION:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_NETOPTION",err));
		break;
	case CS_NOTFOUND:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","CS_NOTFOUND",err));
		break;
	case CS_LIDAUTH:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:\n","LIDAUTH",err));
		break;
	default:
		CS_LOG((msg,"ERROR=%s : cs_error=%d:  cs_perror invalid error number\n",err));
		break;
	}
}


/*	Print out all information in the stuctures used by dial();
 */

static 
void
debug_dial()
{

/* a special define for debug_dial */
#define D_DIAL(x) {sprintf x; debug(msg);}

	D_DIAL((msg, "cs: dial Call structure set up as follows:"));
	D_DIAL((msg, "cs: baud<%d>",Call.baud));
	D_DIAL((msg, "cs: speed<%d>",Call.speed));
	D_DIAL((msg, "cs: modem<%d>",Call.modem));
	D_DIAL((msg, "cs: dev_len<%d>",Call.dev_len));

	if (Call.attr != NULLPTR) {
		D_DIAL((msg, "cs: c_iflag<%d>",Call.attr->c_iflag));
		D_DIAL((msg, "cs: c_oflag<%d>",Call.attr->c_oflag));
		D_DIAL((msg, "cs: c_cflag<%d>",Call.attr->c_cflag));
		D_DIAL((msg, "cs: c_lflag<%d>",Call.attr->c_lflag));
		D_DIAL((msg, "cs: c_line<%c>",Call.attr->c_line));
	} else {
		D_DIAL((msg, "cs: termio = NULL"));
	}

	D_DIAL((msg, "cs: line<%s>", Call.line != NULLPTR? 
	      Call.line: "NULL"));
	D_DIAL((msg, "cs: telno<%s>", Call.telno != NULLPTR? 
	      Call.telno: "NULL"));

	if (Call.device != NULLPTR) {	
		D_DIAL((msg,"cs: service<%s>",
		      ((CALL_EXT *)Call.device)->service != NULL?
	    	      ((CALL_EXT *)Call.device)->service: "NULL"));
		D_DIAL((msg,"cs: class<%s>",
	    	      ((CALL_EXT *)Call.device)->class != NULL?
	    	      ((CALL_EXT *)Call.device)->class: "NULL"));
		D_DIAL((msg,"cs: protocol<%s>",
	    	      ((CALL_EXT *)Call.device)->protocol != NULL?
	    	      ((CALL_EXT *)Call.device)->protocol: "NULL"));
	} else {
		D_DIAL((msg, "cs: deviceptr = NULL"));
	}
}

/*	Processes dial-out connection requests; invokes
 *	authentication scheme.
 *	NOTE: error code for dial conn is encoded in returnfd
 */

static 
void
dial_connect()
{
	DUMP((msg, "cs: fd to read from <%d>",netfd));

	if ((Callp=read_dialrequest(netfd)) == NULL)
		write_status(CS_SYS_ERROR, CS_EXIT);

	if (Debugging) {
		debug_dial();
	}

	/* Ensure that LIDAUTH.map permits request */
	if (Mac_running && check_LID_auth(Call.telno) != CS_NO_ERROR) {
		CS_LOG((msg,"connection not permited by %s", LIDFILE));
		conn_error = CS_DIAL_ERROR;
		returnfd = CS_PROB;
	} else {
		conn_error = CS_NO_ERROR;
		if (( returnfd = dial_auth()) < 0 ) {
			conn_error = CS_DIAL_ERROR;
			DUMP((msg,"cs: dial_auth returned error"));
		}
	}
	
	/* Write call struct and error structure back over pipe	*/
	write_status(conn_error, NO_EXIT);
	write_dialrequest(netfd, Call);

}


/*
 *	free_scheme() free up space associated with schemeinfop.
 */

static
void
free_schemeinfo(schemeinfop)
struct schemeinfo *schemeinfop;
{
	if (schemeinfop == NULL) {
		return;
	}

	if (schemeinfop->s_name) {
		free(schemeinfop->s_name);
	}
	free(schemeinfop);
	return;
}

static
int
read_request()
{
	static	struct 	con_request	req_buf;
	int i;
	static char netpathenv[STRSIZE] = "NETPATH=";
	char	*where=(char *)&req_buf;
	int 	reqsize = sizeof( struct con_request );

	while ((i=read(netfd, where, reqsize)) != reqsize) {
		if (i <= 0) 
			return(CS_SYS_ERROR);
		where = where + i;
		reqsize = reqsize -i;
	}

	Nd_hostserv.h_serv = strdup(req_buf.service);
	Nd_hostserv.h_host = strdup(req_buf.host);

	option = req_buf.option;
	if (option != 0  &&  option != ND_SET_RESERVEDPORT) {
		return(CS_NETOPTION);
	}

	if (req_buf.netpath[0] != '\0') {
		strcat(netpathenv, req_buf.netpath); 
		if (putenv(netpathenv) != 0) {
			CS_LOG((msg,"petenv() failed"));
		}
	} 
	
	/* fill in the netbuf from the caller */
	call_netbufp = (struct netbuf *)NULL;
	if (req_buf.nb_set) {
		call_netbufp = &call_netbuf;
		call_netbuf.maxlen = req_buf.maxlen;
		call_netbuf.len = req_buf.len;
		call_netbuf.buf = req_buf.buf;
		DUMP((msg,"cs: req_buf.len:<%d>", req_buf.len));
		DUMP((msg,"cs: req_buf.maxlen:<%d>", req_buf.maxlen));
	} 

	/* fill in the netconfig from the caller */
	call_ncp = (struct netconfig *)NULL;
	if (req_buf.nc_set) {
		call_ncp = &call_nc;
		memset(call_ncp, '\0', sizeof(struct netconfig));
		call_nc.nc_netid = req_buf.netid[0]? req_buf.netid: NULL;
		call_nc.nc_semantics = req_buf.semantics;
		call_nc.nc_flag = req_buf.flag;
		call_nc.nc_protofmly = 
			req_buf.protofmly[0]? req_buf.protofmly: NULL;
		call_nc.nc_proto = req_buf.proto[0]? req_buf.proto: NULL;
		DUMP((msg,"cs: netid=<%s>",call_nc.nc_netid[0]?call_nc.nc_netid:"NULL"));
		DUMP((msg,"cs: actual netid=<%s>",call_nc.nc_netid));
	}

	DUMP((msg,"cs: netopt:<%d>",option));
	DUMP((msg,"cs: netpath:<%s>",netpathenv));
	DUMP((msg,"cs: host:<%s>",Nd_hostserv.h_host));
	DUMP((msg,"cs: service:<%s>",Nd_hostserv.h_serv));
	
	return(CS_NO_ERROR);

}



/*
 *
 *	Write a status structure over the streams pipe
 * 	If no error has been generated; SENDFD back to 
 *	CS library via the streams pipe.
 *
 *	args:	msgid - id of message to be output
 *		action - action to be taken (EXIT or not)
 */

static
void
write_status(cserror, exitstatus)
int cserror;
int exitstatus;
{
	struct 	status	emsg;
	char	*where=(char *)&emsg;
	int 	reqsize = sizeof(struct status);
	int	i=0;
	struct	passwd	*pinfo;

	/* should also have _nderror (see netdir(3N)) ? */
	/* do tli_error */

	/*	initialize	*/
	emsg.dial_error = 0;
	emsg.tli_error = 0;
	emsg.sys_error = 0;

	log_status(cserror);

	if(cserror == CS_NO_ERROR  &&  Mac_running == TRUE)
		cserror = set_levels(returnfd);

	if(cserror != CS_NO_ERROR && ioctl (netfd, I_SETSIG, 0) != 0)
	{
		CS_LOG((msg,"cs: ioctl set signal error; errno=%d", errno));
		exit(-1);
	}

	emsg.cs_error = cserror;
	if (cserror == CS_SYS_ERROR) 
		emsg.sys_error = errno;
	
	if (cserror == CS_DIAL_ERROR) 
		emsg.dial_error = returnfd;

	while ((i=write(netfd, where, reqsize)) != reqsize) {
		if (i <= 0) {
			CS_LOG((msg,"error in writing back to client"));
			exit(-1);
		}
		where = where + i;
		reqsize = reqsize -i;
	}

	DUMP((msg,"cs: wrote cserror:<%d>", emsg.cs_error));
	DUMP((msg,"cs: wrote syserror:<%d>", emsg.sys_error));
	DUMP((msg,"cs: wrote dial_error:<%d>", emsg.dial_error));
	
	if(cserror == CS_NO_ERROR) {
	   	if (ioctl(netfd, I_SENDFD,returnfd) != 0) {
			DUMP((msg,"cs: write_status: ioctl error"));
			exit(0);
		}
	}

	if (exitstatus) {
		DUMP((msg,"cs: write_status: EXIT\n"));
		exit(0);
	}
	DUMP((msg,"cs: write_status: NO_EXIT"));
}




/*
*
* set_levels sets up the security devices attributes for the
* allocated clone device and the file descriptor that corresponds
* to that device ( connection ). set_levels also changes the level
* of the file descriptor (connection) that will be returned to
* the application to be at the same level as the requesting 
* application. 
*/

static
int
set_levels(connfd)
int	connfd;
{
	struct	dev_alloca	devdata;
	struct	devstat		devatr;	/* dev info in dev database */
	int	i;

	/* set level & flags of device */
	devatr.dev_relflag = DEV_LASTCLOSE;
	devatr.dev_state = DEV_PUBLIC;
	devatr.dev_mode = DEV_STATIC;
	devatr.dev_hilevel = Client_level;
	devatr.dev_lolevel = Client_level;

	if (flvlfile(connfd, MAC_SET, &Client_level) != 0) {
		CS_LOG((msg,"set level failed for connfd"));
		return(CS_SYS_ERROR);
	}

	if (fdevstat(connfd, DEV_SET, &devatr) != 0) {
		log("CS daemon could not set device info");
		CS_LOG((msg,"fdevstat SET error=<%d>", errno));
		return(CS_SYS_ERROR);
	}

	return(CS_NO_ERROR);
}

void
hangup(int sig)
{
	CS_LOG((msg,"client exited unexpectedly"));
	log_status(CS_INTERRUPT);
	sigsend(P_PGID,P_MYID,SIGINT);

}

void
log_status(cserror)
int cserror;
{

        CS_LOG((msg,"Request by process uid<%d> gid<%d> %s %s",
		recvfd->uid, recvfd->gid, Mac_running? "at level":"",
		Mac_running? Level_alias: ""));
	CS_LOG((msg,"  for service<%s> on host<%s> %s",
		Nd_hostserv.h_serv, Nd_hostserv.h_host,
		(cserror == CS_NO_ERROR)? "SUCCEEDED": "FAILED"));
}
