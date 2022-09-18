/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/smtpaux.c	1.8.2.4"
#ident "@(#)smtpaux.c	1.16 'attmail mail(1) command'"
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>		/* needed for socket.h */
#include <unistd.h>
#include <setjmp.h>
#include "s5sysexits.h"
#ifdef	SVR4
# include <sys/uio.h>
#endif
#ifdef SOCKET
# include <sys/socket.h>
# ifdef SYS_INET_H
#  include <sys/in.h>	/* WIN/3B */
#  include <sys/inet.h>	/* WIN/3B */
# endif
# ifdef NETINET_IN_H
#  include <netinet/in.h>
# endif
# ifdef ARPA_INET_H
#  include <arpa/inet.h>
# endif
# include <netdb.h>
#endif
#ifdef	DATAKIT
# include <datakit/dk.h>
#endif
#if defined(SVR3) || defined(SVR4)
# include <memory.h>
# define bzero(p, n)	memset(p, '\0', n)
# define bcopy(f, t, n)	memcpy(t, f, n)
#endif
#include "miscerrs.h"
#include "s_string.h"
#include "smtp.h"
#include "smtp_decl.h"
#if defined(TLI) || defined(SVR4_1)
#include <netconfig.h>
#include <netdir.h>
#include <tiuser.h>
#include <fcntl.h>
#include <sys/stropts.h>
#include "t_decl.h"
#endif
#ifdef SVR4_1
#include <pfmt.h>
#include <stdlib.h>
#endif

#ifdef	SVR3
dont_ask()
{
	/* Force these to be loaded from /usr/lib/libnet.a for WIN/3B */
	read();
	write();
	dup();
	close();
	sync();
	fcntl();
}
#endif

#ifndef SERVNAME
#define SERVNAME	"smtp"		/* service we wanna talk to */
#endif

#ifdef SVR4_1
extern int connsrv_connect();	/* Connect using the connection server */
#endif

#ifdef TLI
extern int tli_connect();	/* Connect using TLI transports */
#endif

#ifdef SOCKET
#ifdef BIND
#define	TCPconnect mxconnect	/* Connect using TCP (with MX records) */
#else
#define	TCPconnect nomxconnect	/* Connect directly using TCP */
#endif
extern int TCPconnect();
#endif

#ifdef DATAKIT
extern int dkconnect();
#endif

struct	conntype {
	char	*prefix;
	int	(*connector)();
} conntype[] = {
#ifdef SVR4_1
	{"cs",	connsrv_connect },
#endif
#ifdef TLI
	{"tli", tli_connect},
#endif
#ifdef SOCKET
	{"tcp", TCPconnect},
#endif
#ifdef	DATAKIT
	{"dk",	dkconnect},
#endif
	{NULL,	NULL}
};

extern int debug;
extern char *progname;

/*
 * setup -- setup tcp/ip connection to/from server
 */
void setup(host, sfip, sfop)
	char *host;
	FILE **sfip, **sfop;
{	int s;
	struct	conntype *ct;
	char *p;

	p = strchr(host, '!');
	if (p) {
		*p = '\0';
		for (ct = conntype; ct->prefix; ct++)
			if (strcmp(ct->prefix, host) == 0)
				break;
		*p++ = '!';
		if (!ct->prefix) ct = conntype;
	}
	else {
		p = host;
		ct = conntype;
	}

	s = (ct->connector)(p);

	if (((*sfip = fdopen(s, "r")) == (FILE *) NULL) ||
	    ((*sfop = fdopen(s, "w")) == (FILE *) NULL)) {
#ifdef SVR4_1
		pfmt(stderr, MM_ERROR, ":57:setup - fdopen: %s\n", strerror(errno));
#else
		perror("setup - fdopen");
#endif
		bomb(E_CANTOPEN);
	}
#ifdef	u3b2
	setbuf(*sfip, (char *) 0);
#endif
}

#ifndef BIND
#ifdef SOCKET
/*
 * nomxconnect -- open socket to/from server
 */
nomxconnect(host)
	char *host;
{	struct hostent *hp;
	struct servent *sp;
	struct sockaddr_in sin;
	int s, i;
	extern int errno;

	bzero((char *)&sin, sizeof(sin));

	if ((hp = gethostbyname(host)) == (struct hostent *) NULL) {
#ifdef SVR4_1
		(void) pfmt(stderr, MM_ERROR, ":13:unknown host (%s).\n", host);
#else
		(void) fprintf(stderr, "%s: unknown host (%s).\n", progname, host);
#endif
		bomb(E_NOHOST);
	}

	for (i=0; hp->h_addr_list[i]; i++) {
		bzero((char *)&sin, sizeof(sin));
		bcopy(hp->h_addr_list[i], &sin.sin_addr, hp->h_length);
	
		if ((sp = getservbyname (SERVNAME, "tcp")) == NULL) {
#ifdef SVR4_1
			(void) pfmt(stderr, MM_ERROR, ":14:unknown service TCP/%s\n", SERVNAME);
#else
			(void) fprintf(stderr,"%s: unknown service TCP/%s\n", progname, SERVNAME);
#endif
			bomb(E_OSFILE);
		}
		sin.sin_port = sp->s_port;
		sin.sin_family = hp->h_addrtype;
	
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s < 0) {
#ifdef SVR4_1
			pfmt(stderr, MM_ERROR, ":58:setup - socket: %s\n", strerror(errno));
#else
			perror("setup - socket");
#endif
			bomb(E_CANTOPEN);
		}
	        if (debug) {
	                int debugval = 1;
	
	                if (setsockopt(s, SOL_SOCKET, SO_DEBUG, (char *)&debugval, sizeof(int)) < 0)
#ifdef SVR4_1
				pfmt(stderr, MM_INFO, ":15:Can't set SO_DEBUG\n");
#else
				fprintf(stderr, "Can't set SO_DEBUG\n");
#endif
		}
	
		if (connect(s, (struct sockaddr *) &sin, sizeof (sin)) >= 0)
			return s;
	}
#ifdef SVR4_1
	pfmt(stderr, MM_ERROR, ":59:setup - connect: %s\n", strerror(errno));
#else
	perror("setup - connect");
#endif
	/*
	 * check for conditions that (we think) are temporary;
	 * try them later; bomb utterly on all others.
	 */
	if (errno == ETIMEDOUT || errno == ECONNREFUSED || 
		errno == EHOSTDOWN || errno == EHOSTUNREACH)
		bomb(E_TEMPFAIL);
	else
		bomb(E_CANTOPEN);

}
#endif
#endif

/*
 * bomb(code) - exit program, map smtp error code into mailsystem code
 * Codes with E_ are defined in miscerrs.h.
 * Codes with EX_ are from <sysexits.h>
 * Lines with FOO are placeholders until we decrypt more appropriate codes.
 *
 * If catch_bombs is non-zero, do a longjmp instead to the place defined
 * by bomb_jmp.
 */
int catch_bombs = 0;
jmp_buf bomb_jmp;

void bomb(code)
int code;
{
	if (catch_bombs)
		longjmp(bomb_jmp, 1);

	switch(code) {
	case 451:			/* host not responding */
		exit(EX_UNAVAILABLE);	/* service unavailable */
		/*NOTREACHED*/
	case 550:			/* no such user */
	case 554:			/* syntax error in address */
	case 501:			/* data format error */
		exit(EX_NOUSER);	/*   == addressee unknown */
		/*NOTREACHED*/
	case E_IOERR:
		exit(EX_IOERR);		/* input/output error */
		/*NOTREACHED*/
	case E_NOHOST:
		exit(EX_NOHOST);	/* host name unknown */
		/*NOTREACHED*/
	case E_OSFILE:			/* no "smtp" -> /etc/services f'd */
		exit(EX_OSFILE);	/* critical OS file missing */
		/*NOTREACHED*/
	case E_USAGE:
		exit(EX_USAGE);		/* command line usage error */
		/*NOTREACHED*/
	case E_TEMPFAIL:
		exit(EX_TEMPFAIL);	/* temp failure; user can retry */
		/*NOTREACHED*/
	case E_DATAERR:
		exit(EX_DATAERR);	/* data format error */
		/*NOTREACHED*/
#ifdef	NOTDEF				/* remainder not used here */
	case FOO:
		exit(EX_OSERR);		/* system error (e.g., can't fork) */
		/*NOTREACHED*/
	case FOO:
		exit(EX_NOINPUT);	/* cannot open input */
		/*NOTREACHED*/
	case FOO:
		exit(EX_CANTCREAT);	/* can't create (user) output file */
		/*NOTREACHED*/
	case FOO:
		exit(EX_PROTOCOL);	/* remote error in protocol */
		/*NOTREACHED*/
	case FOO:
		exit(EX_NOPERM);	/* permission denied */
		/*NOTREACHED*/
#endif	/* NOTDEF */
	default:			/* can't happen? */
		exit(EX_SOFTWARE);	/* internal software error */
		/*NOTREACHED*/
	}
}

#ifdef DATAKIT
dkconnect(host)
	char *host;
{
	extern int dk_verbose, dk_errno;
	static short dkrmode = DKR_BLOCK | DKR_TIME;
	int s;

	dk_verbose = 0;
	s = dkdial(maphost(host, 's', "smtp", "", ""));
	if (s < 0)
		bomb(-dk_errno);

	ioctl(s, DIOCRMODE, &dkrmode);
	return s;
}
#endif


#ifdef TLI

int tli_connect(host)
char *host;
{
	register struct netconfig *	ncp;
	register int			i;
	struct nd_hostserv		ndh;
	struct nd_addrlist *		nap;
	struct t_call *			tcp;
	int				fd;
	void *				handle;
	extern int			debug;

	/* Lookup address of service "smtp" on host `host' (any transport) */
	ndh.h_host = host;
	ndh.h_serv = SERVNAME;

	if ((handle = setnetpath()) != NULL) {
		while ((ncp = getnetpath(handle)) != NULL) {
			if (mxnetdir(ncp, &ndh, &nap) != 0)
				continue;

			if (nap->n_cnt <= 0)
				continue;

			for (i = 0; i < nap->n_cnt; i++) {
				if ((fd = t_open(ncp->nc_device, O_RDWR, NULL)) < 0) {
					t_log("can't t_open transport");
					i = nap->n_cnt;
					break;
				}

				if (t_bind(fd, NULL, NULL) < 0) {
					t_log("can't bind address for client");
					t_close(fd);
					i = nap->n_cnt;
					break;
				}

				if ((tcp = (struct t_call *) t_alloc(fd, T_CALL, T_ALL)) == NULL) {
					t_log("can't alloc for client");
					t_close(fd);
					i = nap->n_cnt;
					break;
				}

				tcp->addr = nap->n_addrs[i];
				if (debug)
					fprintf(stderr, "Try <%s>\n",
					taddr2uaddr(ncp, &tcp->addr));
				if (t_connect(fd, tcp, NULL) >= 0)
					break;
				t_close(fd);
			}
			if (i >= nap->n_cnt) {
				t_log("t_connect failed");
				t_close(fd);
				continue;
			}

			/* Set up file descriptor for read/write use */
			if (ioctl(fd, I_PUSH, "tirdwr") < 0) {
				t_log("push tirdwr failed");
				t_close(fd);
				continue;
			}

			endnetpath(handle);
			return fd;
		}

		endnetpath(handle);
	}

	/* Not found - fail */
	bomb(451);
	/*NOTREACHED*/
}
#endif

#ifdef SVR4_1
int connsrv_connect(host)
char *host;
{
	register int fd;
	register void *vp;
	char *nhost;
#ifdef S17
	void *status;
#else
	int status;
	struct csopts;
#endif
	extern int lookup_mx();
	extern int cs_connect();
	extern void cs_perror();
	extern void *next_mx();

	/* Prime the list of MX hosts for "host" */
	(void) lookup_mx(host);

	vp = next_mx((void *) 0, &nhost);
	for (; vp != (void *) 0; vp = next_mx(vp, &nhost)) {
		/* Ask the connection server to make a connection for us */
#ifdef S17
		status = (void *) 0;
		fd = cs_connect(nhost, SERVNAME, &status);
#else
		fd = cs_connect(nhost, SERVNAME, (struct csopts *)NULL, &status);
#endif
		if (fd < 0) {
			/* Not found - try next MX entry */
			/* This will go in the E.* file, which is not the most useful
			   place (I'd prefer it went in the logfile) but is better
			   than nothing (we are not privy to the structure of *status)
			 */
			cs_perror("smtp", status);
			continue;
		}

		/* Set up file descriptor for read/write use */
		if (ioctl(fd, I_PUSH, "tirdwr") < 0) {
			t_log("push tirdwr failed");
			t_close(fd);
			continue;
		}
		return fd;
	}

	/* Not found - fail */
	bomb(451);
}
#endif
