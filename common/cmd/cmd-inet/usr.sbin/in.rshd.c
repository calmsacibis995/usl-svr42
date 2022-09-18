/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.rshd.c	1.8.12.1"
#ident  "$Header: in.rshd.c 1.3 91/09/19 $"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


/*
 * remote shell server:
 *	[port]\0
 *	remuser\0
 *	locuser\0
 *	command\0
 *	data
 */
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#ifdef AUDIT
#include <sys/label.h>
#include <sys/audit.h>
#endif AUDIT

#include <netinet/in.h>

#include <arpa/inet.h>

#include <stdio.h>
#include <errno.h>
#include <pwd.h>
#ifdef AUDIT
#include <pwdadj.h>
#endif AUDIT
#include <signal.h>
#include <netdb.h>
#include <syslog.h>

#ifdef SYSV
#include <sys/resource.h>
#include <sys/filio.h>

#define	killpg(a,b)	kill(-(a),(b))
#define rindex strrchr
#define index strchr
#define	bcmp memcmp
#endif SYSV
#include "./security.h"

#define _PATH_BSHELL	"/bin/sh"
#define _PATH_NOLOGIN	"/etc/nologin"
#define _PATH_TTY	"/dev/tty"
#ifdef SYSV
#define	_PATH_DEFPATH	"PATH=:/bin:/usr/bin"
#else
#define _PATH_DEFPATH	"PATH=/bin:/usr/ucb:/usr/bin:"
#endif /* SYSV */

#ifndef F_OK
#define	F_OK	0	/* 'does file exist' for access() */
#endif /* !F_OK */

/*
 * NCARGS has become enormous, making *	it impractical to allocate arrays
 * with NCARGS / 6 entries on the stack.  We simply fall back on using
 * the old value of NCARGS.
 */
#define	AVSIZ	(10240 / 6)

#ifdef AUDIT
#define AUDITNUM sizeof(audit_argv) / sizeof(char *)
#endif AUDIT

int	errno;
int	keepalive = 1;
int	check_all = 0;
int	_check_rhosts_file = 1;
char	*index(), *rindex(), *strncat();
#ifdef AUDIT
char	*audit_argv[] = {"in.rshd", 0, 0, 0, 0, 0};
#endif AUDIT
/*VARARGS1*/
int	error();
int	sent_null;

/*ARGSUSED*/
main(argc, argv)
	int argc;
	char **argv;
{
	extern int opterr, optind;
	struct linger linger;
	int ch, on = 1, fromlen;
	struct sockaddr_in from;

	openlog("rshd", LOG_PID | LOG_ODELAY, LOG_DAEMON);

	opterr = 0;
	while ((ch = getopt(argc, argv, "aln")) != EOF)
		switch (ch) {
		case 'a':
			check_all = 1;
			break;
		case 'l':
			_check_rhosts_file = 0;
			break;
		case 'n':
			keepalive = 0;
			break;
		case '?':
		default:
			syslog(LOG_ERR, "rshd: usage: rshd [-aln]");
			break;
		}

	argc -= optind;
	argv += optind;

	fromlen = sizeof (from);
	if (getpeername(0, &from, &fromlen) < 0) {
		syslog(LOG_ERR, "rshd: getpeername: %m");
		_exit(1);
	}
	if (keepalive &&
	    setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char *)&on,
	    sizeof (on)) < 0)
		syslog(LOG_WARNING, "rshd: setsockopt (SO_KEEPALIVE): %m");
	linger.l_onoff = 1;
	linger.l_linger = 60;
	if (setsockopt(0, SOL_SOCKET, SO_LINGER, (char *)&linger,
	    sizeof (linger)) < 0)
		syslog(LOG_WARNING, "rshd: setsockopt (SO_LINGER): %m");
	doit(dup(0), &from);
	/* NOTREACHED */
}

char	logname[23] = "LOGNAME=";
char	username[20] = "USER=";
char	homedir[64] = "HOME=";
char	shell[64] = "SHELL=";
char	*envinit[] =
	    {homedir, shell, _PATH_DEFPATH, logname, username, 0};
extern char	**environ;

static char cmdbuf[AVSIZ+1];

doit(f, fromp)
	int f;
	struct sockaddr_in *fromp;
{
	char *cp;
	char locuser[16], remuser[16];

	struct passwd *pwd;
#ifdef AUDIT
	struct passwd_adjunct *apw, *getpwanam();
	audit_state_t astate;
#endif AUDIT

	int s;
	struct hostent *hp;
	char *hostname;
	short port;
	pid_t pid;
	int pv[2], cc;
	char buf[BUFSIZ], sig;
	int one = 1;
	char remotehost[2 * MAXHOSTNAMELEN + 1];

	(void) signal(SIGINT, SIG_DFL);
	(void) signal(SIGQUIT, SIG_DFL);
	(void) signal(SIGTERM, SIG_DFL);
#ifdef SYSV
	(void) sigset(SIGCHLD, SIG_IGN);
#endif /* SYSV */
#ifdef DEBUG
	{ int t = open(_PATH_TTY, 2);
	  if (t >= 0) {
#ifdef SYSV
		setsid();
#else
		ioctl(t, TIOCNOTTY, (char *)0);
#endif SYSV
		(void) close(t);
	  }
	}
#endif
	fromp->sin_port = ntohs((u_short)fromp->sin_port);
	if (fromp->sin_family != AF_INET) {
		syslog(LOG_ERR, "rshd: malformed from address\n");
		exit(1);
	}
#ifdef IP_OPTIONS
      {
	u_char optbuf[BUFSIZ/3], *cp;
	char lbuf[BUFSIZ], *lp;
	int optsize = sizeof(optbuf), ipproto;
	struct protoent *ip;

	if ((ip = getprotobyname("ip")) != NULL)
		ipproto = ip->p_proto;
	else
		ipproto = IPPROTO_IP;
	if (getsockopt(0, ipproto, IP_OPTIONS, (char *)optbuf, &optsize) == 0 &&
	    optsize != 0) {
		lp = lbuf;
		for (cp = optbuf; optsize > 0; cp++, optsize--, lp += 3)
			sprintf(lp, " %2.2x", *cp);
		syslog(LOG_NOTICE,
		    "rshd: Connection received using IP options (ignored):%s", lbuf);
		if (setsockopt(0, ipproto, IP_OPTIONS,
		    (char *)NULL, &optsize) != 0) {
			syslog(LOG_ERR, "rshd: setsockopt IP_OPTIONS NULL: %m");
			exit(1);
		}
	}
      }
#endif

	if (fromp->sin_port >= (u_short)IPPORT_RESERVED ||
	    fromp->sin_port < (u_short)(IPPORT_RESERVED/2)) {
		syslog(LOG_NOTICE, "rshd: connection from %s on illegal port",
			inet_ntoa(fromp->sin_addr));
		exit(1);
	}
	(void) alarm(60);
	port = 0;
	for (;;) {
		char c;
		if ((cc = read(f, &c, 1)) != 1) {
			if (cc < 0)
				syslog(LOG_NOTICE, "rshd: read: %m");
			shutdown(f, 1+1);
			exit(1);
		}
		if (c == 0)
			break;
		port = port * 10 + c - '0';
	}
	(void) alarm(0);
	if (port != 0) {
		int lport = IPPORT_RESERVED - 1;
		s = rresvport(&lport);
		if (s < 0) {
			syslog(LOG_ERR, "rshd: can't get stderr port: %m");
			exit(1);
		}

		if (is_enhancedsecurity()) {
			fd_set_to_my_level(s);
		}

		if (port >= IPPORT_RESERVED) {
			syslog(LOG_ERR, "rshd: 2nd port not reserved\n");
			exit(1);
		}
		fromp->sin_port = htons((u_short)port);
		if (connect(s, fromp, sizeof (*fromp)) < 0) {
			syslog(LOG_INFO, "rshd: connect second port: %m");
			exit(1);
		}
	}
	dup2(f, 0);
	dup2(f, 1);
	dup2(f, 2);
	hp = gethostbyaddr((char *)&fromp->sin_addr, sizeof (struct in_addr),
		fromp->sin_family);
	if (hp) {
		/*
		 * If name returned by gethostbyaddr is in our domain,
		 * attempt to verify that we haven't been fooled by someone
		 * in a remote net; look up the name and check that this
		 * address corresponds to the name.
		 */
		if (check_all || local_domain(hp->h_name)) {
			strncpy(remotehost, hp->h_name, sizeof(remotehost) - 1);
			remotehost[sizeof(remotehost) - 1] = 0;
			hp = gethostbyname(remotehost);
			if (hp == NULL) {
				syslog(LOG_INFO,
				    "rshd: Couldn't look up address for %s",
				    remotehost);
				error("Couldn't look up address for your host\n");
				exit(1);
			}
#ifdef h_addr	/* 4.2 hack */
			for (; ; hp->h_addr_list++) {
				if (!bcmp(hp->h_addr_list[0],
				    (caddr_t)&fromp->sin_addr,
				    sizeof(fromp->sin_addr)))
					break;
				if (hp->h_addr_list[0] == NULL) {
					syslog(LOG_NOTICE,
					  "rshd: Host addr %s not listed for host %s",
					    inet_ntoa(fromp->sin_addr),
					    hp->h_name);
					error("Host address mismatch\n");
					exit(1);
				}
			}
#else
			if (bcmp(hp->h_addr, (caddr_t)&fromp->sin_addr,
			    sizeof(fromp->sin_addr))) {
				syslog(LOG_NOTICE,
				  "rshd: Host addr %s not listed for host %s",
				    inet_ntoa(fromp->sin_addr),
				    hp->h_name);
				error("Host address mismatch\n");
				exit(1);
			}
#endif
		}
		hostname = hp->h_name;
	} else
		hostname = inet_ntoa(fromp->sin_addr);
	getstr(remuser, sizeof(remuser), "remuser");
	getstr(locuser, sizeof(locuser), "locuser");
	getstr(cmdbuf, sizeof(cmdbuf), "command");
#ifdef AUDIT
    	/*
	 * store common info. for audit record 
     	 */
	audit_argv[1] = remuser; 
	audit_argv[2] = locuser; 
	audit_argv[3] = hostname; 
	audit_argv[4] = cmdbuf; 
#endif AUDIT

	setpwent();
	pwd = getpwnam(locuser);
	if (pwd == NULL) {
		audit_write(1, "Login incorrect"); 
		error("Login incorrect.\n");
		exit(1);
	}
	endpwent();
#ifdef AUDIT
	CLR_WORKPRIVS_NON_ADMIN(setauid(pwd->pw_uid));
	/*
	 * get audit flags for user and set for all 
	 * processes owned by this uid 
	 */
	if ((apw = getpwanam(locuser)) != NULL) {
		astate.as_success = 0;
		astate.as_failure = 0;

		if ((getfauditflags(&apw->pwa_au_always, 
		  &apw->pwa_au_never, &astate)) != 0) {
			/*
             		* if we can't tell how to audit from the flags, audit
             		* everything that's not never for this user.
			*/
            		astate.as_success = apw->pwa_au_never.as_success ^ (-1);
            		astate.as_failure = apw->pwa_au_never.as_success ^ (-1);
		}
	}
	else {
		astate.as_success = -1;
		astate.as_failure = -1;
	}

	if (issecure())
		setuseraudit(pwd->pw_uid, &astate);
#endif AUDIT

	if (chdir(pwd->pw_dir) < 0) {
		(void) chdir("/");
#ifdef notdef
		error("No remote directory.\n");
		exit(1);
#endif
	}
	if (pwd->pw_passwd != 0 && *pwd->pw_passwd != '\0' &&
	    ruserok(hostname, pwd->pw_uid == 0, remuser, locuser) < 0) {
		audit_write(1, "Permission denied"); 
		error("Permission denied.\n");
		exit(1);
	}

	if (pwd->pw_uid && !access(_PATH_NOLOGIN, F_OK)) {
		error("Logins currently disabled.\n");
		exit(1);
	}

	(void) write(2, "\0", 1);
	sent_null = 1;

	if (port) {
		if (pipe(pv) < 0) {
			audit_write(1, "Can't make pipe."); 
			error("Can't make pipe.\n");
			exit(1);
		}
		pid = fork();
		if (pid == (pid_t)-1)  {
			audit_write(1, "Error in fork()."); 
			error("Try again.\n");
			exit(1);
		}

#ifndef MAX
#define MAX(a,b) (((u_int)(a) > (u_int)(b)) ? (a) : (b))
#endif /* MAX */

		if (pid) {
			int width = MAX(s, pv[0]) + 1;
			fd_set ready;
			fd_set readfrom;

			(void) close(0); (void) close(1); (void) close(2);
			(void) close(f); (void) close(pv[1]);
			FD_ZERO (&ready);
			FD_ZERO (&readfrom);
			FD_SET (s, &readfrom);
			FD_SET (pv[0], &readfrom);
			if (ioctl(pv[0], FIONBIO, (char *)&one) == -1)
				syslog (LOG_INFO, "rshd: ioctl FIONBIO: %m");
			/* should set s nbio! */
			do {
				ready = readfrom;
				if (select(width, &ready, (fd_set *)0,
				    (fd_set *)0, (struct timeval *)0) < 0)
					break;
				if (FD_ISSET (s, &ready)) {
					if (read(s, &sig, 1) <= 0)
						FD_CLR (s, &readfrom);
					else
						killpg(pid, sig);
				}
				if (FD_ISSET (pv[0], &ready)) {
					errno = 0;
					cc = read(pv[0], buf, sizeof (buf));
					if (cc <= 0) {
						shutdown(s, 1+1);
						FD_CLR (pv[0], &readfrom);
					} else
						(void) write(s, buf, cc);
				}
			} while (FD_ISSET (s, &readfrom) || 
				 FD_ISSET (pv[0], &readfrom));
			exit(0);
		}
		setpgrp(0, getpid());
		(void) close(s); (void) close(pv[0]);
		dup2(pv[1], 2);
		(void) close(pv[1]);
	}
	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = _PATH_BSHELL;
	(void) close(f);
	if (  setgid((gid_t)pwd->pw_gid) < 0 ) {
		error("Invalid gid.\n");
		exit(1);
	}
	initgroups(pwd->pw_name, pwd->pw_gid);

	/*
	 * write audit record before making uid switch  
	 */
	audit_write(0, "authorization successful"); 

	if ( CLR_WORKPRIVS_NON_ADMIN(setuid((uid_t)pwd->pw_uid)) < 0 ) {
		error("Invalid uid.\n");
		exit(1);
	}
	environ = envinit;
	strncat(homedir, pwd->pw_dir, sizeof(homedir)-6);
	strncat(shell, pwd->pw_shell, sizeof(shell)-7);
	strncat(logname, pwd->pw_name, sizeof(logname)-9);
	strncat(username, pwd->pw_name, sizeof(username)-6);
	cp = rindex(pwd->pw_shell, '/');
	if (cp)
		cp++;
	else
		cp = pwd->pw_shell;
	endpwent();
	signal(SIGCHLD, SIG_DFL);       /* re-enable so shell */
					/* thats created gets the signal */
	if (pwd->pw_uid == 0)
		syslog(LOG_INFO|LOG_AUTH, "rshd: ROOT shell from %s@%s, comm: %s\n",
		    remuser, hostname, cmdbuf);
	CLR_MAXPRIVS_FOR_EXEC execle(pwd->pw_shell, cp, "-c", cmdbuf,
					(char *)0, environ);
	perror(pwd->pw_shell);
	audit_write(1, "can't exec");
	exit(1);
}

/*
 * Report error to client.
 * Note: can't be used until second socket has connected
 * to client, or older clients will hang waiting
 * for that connection first.
 */
/*VARARGS1*/
error(fmt, a1, a2, a3)
	char *fmt;
	int a1, a2, a3;
{
	char buf[BUFSIZ], *bp = buf;

	if (sent_null == 0)
		*bp++ = 1;
	(void) sprintf(bp, fmt, a1, a2, a3);
	(void) write(2, buf, strlen(buf));
}

getstr(buf, cnt, err)
	char *buf;
	int cnt;
	char *err;
{
	char c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);
		*buf++ = c;
		if (--cnt == 0) {
			error("%s too long\n", err);
			exit(1);
		}
	} while (c != 0);
}

audit_write(val, message)
int val;
char *message;
{
#ifdef AUDIT
	audit_argv[5] = message;
	audit_text(AU_LOGIN, val, val, AUDITNUM, audit_argv);
#endif AUDIT
}

/*
 * Check whether host h is in our local domain,
 * defined as sharing the last two components of the domain part,
 * or the entire domain part if the local domain has only one component.
 * If either name is unqualified (contains no '.'),
 * assume that the host is local, as it will be
 * interpreted as such.
 */
local_domain(h)
	char *h;
{
	char localhost[MAXHOSTNAMELEN];
	char *p1, *p2, *topdomain();

	localhost[0] = 0;
	(void) gethostname(localhost, sizeof(localhost));
	p1 = topdomain(localhost);
	p2 = topdomain(h);
	if (p1 == NULL || p2 == NULL || !strcasecmp(p1, p2))
		return(1);
	return(0);
}

char *
topdomain(h)
	char *h;
{
	register char *p;
	char *maybe = NULL;
	int dots = 0;

	for (p = h + strlen(h); p >= h; p--) {
		if (*p == '.') {
			if (++dots == 2)
				return (p);
			maybe = p;
		}
	}
	return (maybe);
}

#ifdef SYSV
/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static char charmap[] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
	'\300', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\333', '\334', '\335', '\336', '\337',
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

static
strcasecmp(s1, s2)
	register char *s1, *s2;
{
	register char *cm = charmap;

	while (cm[*s1] == cm[*s2++])
		if (*s1++ == '\0')
			return(0);
	return(cm[*s1] - cm[*--s2]);
}
#endif /* SYSV */
