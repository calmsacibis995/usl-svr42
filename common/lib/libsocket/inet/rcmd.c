/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libsocket:common/lib/libsocket/inet/rcmd.c	1.6.5.6"
#ident  "$Header: rcmd.c 1.2 91/06/26 $"

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

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/file.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>

#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

#ifdef SYSV
#define bcopy(s1, s2, len)	memcpy(s2, s1, len)
#define bzero(s, len)		memset(s, 0, len)
#define index(s, c)		strchr(s, c)
char	*strchr();
#else
char	*index();
#endif SYSV

extern	errno;
char	*strcpy();
static char *domain;

rcmd(ahost, rport, locuser, remuser, cmd, fd2p)
	char **ahost;
	u_short rport;
	char *locuser, *remuser, *cmd;
	int *fd2p;
{
	int s, timo = 1, retval;
	pid_t pid;
	struct sockaddr_in sin, from;
	char c;
	int lport = IPPORT_RESERVED - 1;
	struct hostent *hp;
#ifdef SYSV
	void	(*spipe)();
	sigset_t oldmask;
	sigset_t newmask;
#else
	int oldmask;
#endif SYSV
	int oerr;


	pid = getpid();
	hp = gethostbyname(*ahost);
	if (hp == 0) {
		fprintf(stderr, "%s: unknown host\n", *ahost);
		return (-1);
	}
	*ahost = hp->h_name;
#ifdef SYSV
	spipe = sigset(SIGPIPE, SIG_IGN);
	bzero((char *) &newmask, sizeof (newmask));
	(void) sigaddset(&newmask, SIGURG);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
#else
	oldmask = sigblock(sigmask(SIGURG));
#endif SYSV
	for (;;) {
		s = rresvport(&lport);
		if (s < 0) {
			if (errno == EAGAIN)
				fprintf(stderr, "socket: All ports in use\n");
			else
				perror("rcmd: socket");
#ifdef SYSV
			(void) sigset(SIGPIPE, spipe);
			(void) sigprocmask(SIG_SETMASK, &oldmask, 
					   (sigset_t *) 0);
#else
			sigsetmask(oldmask);
#endif SYSV
			return (-1);
		}
		fcntl(s, F_SETOWN, pid);
		sin.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr_list[0], (caddr_t)&sin.sin_addr, hp->h_length);
		sin.sin_port = rport;
		if (connect(s, &sin, sizeof (sin)) >= 0)
			break;

		oerr=errno;
		(void)close(s);
		errno=oerr;

		if (errno == EADDRINUSE) {
			lport--;
			continue;
		}
		if (errno == ECONNREFUSED && timo <= 16) {
			sleep(timo);
			timo *= 2;
			continue;
		}
		if (hp->h_addr_list[1] != NULL) {
			int oerrno = errno;

			fprintf(stderr,
			    "connect to address %s: ", inet_ntoa(sin.sin_addr));
			errno = oerrno;
			perror(0);
			hp->h_addr_list++;
			bcopy(hp->h_addr_list[0], (caddr_t)&sin.sin_addr,
			    hp->h_length);
			fprintf(stderr, "Trying %s...\n",
				inet_ntoa(sin.sin_addr));
			continue;
		}
		perror(hp->h_name);
#ifdef SYSV
		(void) sigset(SIGPIPE, spipe);
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *) 0);
#else
		sigsetmask(oldmask);
#endif SYSV
		return (-1);
	}
	lport--;
	if (fd2p == 0) {
		write(s, "", 1);
		lport = 0;
	} else {
		char num[8];
		int s2 = rresvport(&lport), s3;
		int len = sizeof (from);

		if (s2 < 0)
			goto bad;
		listen(s2, 1);
		(void) sprintf(num, "%d", lport);
		if (write(s, num, strlen(num)+1) != strlen(num)+1) {
			perror("write: setting up stderr");
			(void) close(s2);
			goto bad;
		}
		s3 = accept(s2, &from, &len);
		(void) close(s2);
		if (s3 < 0) {
			perror("accept");
			lport = 0;
			goto bad;
		}
		*fd2p = s3;
		from.sin_port = ntohs((u_short)from.sin_port);
		if (from.sin_family != AF_INET ||
		    from.sin_port >= IPPORT_RESERVED) {
			fprintf(stderr,
			    "socket: protocol failure in circuit setup.\n");
			goto bad2;
		}
	}
	(void) write(s, locuser, strlen(locuser)+1);
	(void) write(s, remuser, strlen(remuser)+1);
	(void) write(s, cmd, strlen(cmd)+1);
	retval = read(s, &c, 1);
	if (retval != 1) {
		if (retval == 0) {
		    fprintf(stderr,
		      "Protocol error, %s closed connection\n", *ahost);
		} else if (retval < 0) {
		    perror(*ahost);
		} else {
		    fprintf(stderr,
		      "Protocol error, %s sent %d bytes\n", *ahost, retval);
		}
		goto bad2;
	}
	if (c != 0) {
		while (read(s, &c, 1) == 1) {
			(void) write(2, &c, 1);
			if (c == '\n')
				break;
		}
		goto bad2;
	}
#ifdef SYSV
	(void) sigset(SIGPIPE, spipe);
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *) 0);
#else
	sigsetmask(oldmask);
#endif SYSV
	return (s);
bad2:
	if (lport)
		(void) close(*fd2p);
bad:
	(void) close(s);
#ifdef SYSV
	(void) sigset(SIGPIPE, spipe);
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *) 0);
#else
	sigsetmask(oldmask);
#endif SYSV
	return (-1);
}

rresvport(alport)
	int *alport;
{
	struct sockaddr_in sin;
	int s;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
		return (-1);
	for (;;) {
		sin.sin_port = htons((u_short)*alport);
		if (bind(s, (caddr_t)&sin, sizeof (sin)) >= 0)
			return (s);
		if (errno != EADDRINUSE) {
			(void) close(s);
			return (-1);
		}
		(*alport)--;
		if (*alport == IPPORT_RESERVED/2) {
			(void) close(s);
			errno = EAGAIN;		/* close */
			return (-1);
		}
	}
}

ruserok(rhost, superuser, ruser, luser)
	char *rhost;
	int superuser;
	char *ruser, *luser;
{
	FILE *hostf;
	char fhost[MAXHOSTNAMELEN];
	register char *sp, *p;
	int baselen = -1;

	struct stat sbuf;
	struct passwd *pwd;
	char pbuf[MAXPATHLEN];
	uid_t euid = (uid_t)-1;

	sp = rhost;
	p = fhost;
	while (*sp) {
		if (*sp == '.') {
			if (baselen == -1)
				baselen = sp - rhost;
			*p++ = *sp++;
		} else {
			*p++ = isupper(*sp) ? tolower(*sp++) : *sp++;
		}
	}
	*p = '\0';

	/* check /etc/hosts.equiv */
	if (!superuser) {
		if ((hostf = fopen("/etc/hosts.equiv", "r")) != NULL) {
			if (!_validuser(hostf, fhost, luser, ruser, baselen, 0)) {
				(void) fclose(hostf);
				return(0);
		        }
			(void) fclose(hostf);
		}
	}

	/* check ~/.rhosts */

	if ((pwd = getpwnam(luser)) == NULL)
       		return(-1);
	(void)strcpy(pbuf, pwd->pw_dir);
	(void)strcat(pbuf, "/.rhosts");

	/* 
	 * Read .rhosts as the local user to avoid NFS mapping the root uid
	 * to something that can't read .rhosts.
	 */
	euid = geteuid();
	(void) seteuid (pwd->pw_uid);
	if ((hostf = fopen(pbuf, "r")) == NULL) {
		if (euid != (uid_t)-1)
	    		(void) seteuid (euid);
	  	return(-1);
	}
	(void)fstat(fileno(hostf), &sbuf);
	if (sbuf.st_uid && sbuf.st_uid != pwd->pw_uid) {
	  	fclose(hostf);
		if (euid != (uid_t)-1)
		  	(void) seteuid (euid);
		return(-1);
	}

	if (!_validuser(hostf, fhost, luser, ruser, baselen, 1)) {
	  	(void) fclose(hostf);
		if (euid != (uid_t)-1)
			(void) seteuid (euid);
		return(0);
	}

	(void) fclose(hostf);
	if (euid != (uid_t)-1)
       		(void) seteuid (euid);
	return (-1);
}

static int
_validuser(hostf, rhost, luser, ruser, baselen, isrhosts)
char *rhost, *luser, *ruser;
FILE *hostf;
int baselen;
int isrhosts;
{
	char *user;
	char ahost[MAXHOSTNAMELEN];
	int hostmatch, usermatch;
	register char *p;

#ifdef YP
	if (domain == NULL) {
                (void) usingypmap(&domain, NULL);
        }
#endif YP

	while (fgets(ahost, sizeof (ahost), hostf)) {
		hostmatch = usermatch = 0 ;
		p = ahost;
		while (*p != '\n' && *p != ' ' && *p != '\t' && *p != '\0') {
			*p = isupper(*p) ? tolower(*p) : *p;
			p++;
		}
		if (*p == ' ' || *p == '\t') {
			*p++ = '\0';
			while (*p == ' ' || *p == '\t')
				p++;
			user = p;
			while (*p != '\n' && *p != ' ' && *p != '\t' &&
			       *p != '\0')
				p++;
		} else
			user = p;
		*p = '\0';
		if (ahost[0] == '+' && ahost[1] == 0)
			hostmatch = 1;
#ifdef YP
		else if (ahost[0] == '+' && ahost[1] == '@')
			hostmatch = innetgr(ahost + 2, rhost,
			    NULL, domain);
		else if (ahost[0] == '-' && ahost[1] == '@') {
			if (innetgr(ahost + 2, rhost, NULL, domain))
				break;
		}
#endif YP
		else if (ahost[0] == '-') {
			if (_checkhost(rhost, ahost+1, baselen))
				break;
		}
		else
			hostmatch = _checkhost(rhost, ahost, baselen);

		if ( !hostmatch )
			continue ;

		if (user[0]) {
		    if (isrhosts) {
			if (user[0] == '+' && user[1] == 0)
				usermatch = 1;
#ifdef YP
			else if (user[0] == '+' && user[1] == '@')
				usermatch = innetgr(user+2, NULL,
				    ruser, domain);
			else if (user[0] == '-' && user[1] == '@') {
				if (innetgr(user+2, NULL, ruser, domain))
					break;
			}
#endif YP
			else if (user[0] == '-') {
				if (!strcmp(user+1, ruser))
					break;
				else	continue;
			}
			else
				usermatch = !strcmp(user, ruser);
		    } else {
			/*
			 * is /etc/hosts.equiv -- don't map
			 * the remote user to an arbitrary local user,
			 */
			if (user[0] == '-') {
				if (!strcmp(user+1, ruser))
					break;
				else	continue;
			}
			usermatch = ( !strcmp(user, ruser) &&
				      !strcmp(user, luser) );
		    }
		}
		else
			usermatch = !strcmp(ruser, luser);

		if ( usermatch )
			return (0);
	}
	return (-1);
}


/* _Dname_read(file_to_search, buffer_to_fill, size_of_buffer)
 *
 * This code is intended to get the default domain name
 * from the indicated file in the spirit of code in
 * common/cmd/cmd-inet/usr.sbin/in.named/ns_init.c, since
 * we would rather not go to the network for it.
 */

static
_Dname_read(Config_file, Default_Dname, Size_Dname)
char *Config_file, *Default_Dname;
uint Size_Dname;
{
	register FILE *fp;
	register char *cp;
	char buf[BUFSIZ];
	extern char *strncpy();
	uint dotchars;

	getdomainname( buf, BUFSIZ);
	if (buf[0] == '+')
		buf[0] = '.';
	cp = index(buf, '.');
	if (cp == NULL)
		strncpy(Default_Dname, buf,  Size_Dname);
	else 
		strncpy(Default_Dname, cp+1, Size_Dname);

	if ((fp = fopen(Config_file, "r")) != NULL) {
			/* read the config file */
		while (fgets(buf, BUFSIZ, fp) != NULL) {
		        	/* read default domain name */
			if((cp=index(Default_Dname,';'))!=NULL)
				*cp = '\0';
			if (!strncmp(buf, "domain", sizeof("domain") - 1)) {
				cp = buf + sizeof("domain") - 1;
				while (*cp == ' ' || *cp == '\t')
					cp++;
				if (*cp != '\0') {
					(void)strncpy(Default_Dname,cp,Size_Dname);
					Default_Dname[Size_Dname - 1] = '\0';
					if((cp=index(Default_Dname,'\n'))!=NULL)
						*cp = '\0';
					break;
				}
			}
		}
		(void) fclose(fp);
	}
		/* Since this is an administrator edited file, they
		 * may have trailing "." for the root domain.
		 * Delete trailing '.'.
		 */
	dotchars = strlen(Default_Dname);
	while ((dotchars>0) && ('.'==Default_Dname[--dotchars])) 
		Default_Dname[dotchars]=(char) 0;
}


/*
 * _get_domain(buffer, size_of_buffer)
 *
 * Search several DNS files for the domain name.
 * Since we are being called from _checkhost, we
 * will be called with the buffer gethostname used,
 * and will slipping the DNS path into place after the
 * name in the buffer.
 */

#ifndef	CONFFILE
#define	CONFFILE	"/etc/resolv.conf"
#endif

static char *
_get_domain(local_name, local_size)
char *local_name;
int local_size;
{
	char	*workstring,
		*datafiles[] = {CONFFILE,
				"/etc/inet/named.boot",
				"/etc/named.boot",
				NULL };
	uint	len, i;

	if ((NULL==local_name) ||
	    (1 > (len = strlen(local_name))) ||
	    (3 > (local_size-len)) ||		/*need some room for DNS*/
	    ('.' == *(local_name+len-1)))	/*dots here are awkward*/
		return NULL;	/* strange string or no room */
	workstring=local_name+len+1;

	for (i=0; datafiles[i]; i++) {
		/* extract domain name into correct place for it to be
		 * appended to uname string in local_name.  Don't put
		 * in the "." as a connector till after the domain name
		 * is there, so in abort case we won't have to mess with
		 * the string.  Make the domain part null terminated before
		 * we start.
		 */
		*workstring = (char) 0;

		_Dname_read(datafiles[i], workstring, local_size-len-1);
		if (((char) 0   != *workstring) &&
		    ((char) '.' != *workstring) &&
		    (index(workstring, '.'))) {
				/* put the dot after the uname and before
				 * the domain name, joining the strings.
				 * _checkhost needs us to return at this
				 * preceding '.'
				 */
			*(workstring-1) = '.';
			return (workstring-1);
		}
	}
	return (NULL);
}


static
_checkhost(rhost, lhost, len)
char *rhost, *lhost;
int len;
{
	static char *domainp = NULL;
	static int nodomain  = 0;
	register char *cp;

	/* the remote host is not in DNS format? */
	if (len == -1)
		return(!strcmp(rhost, lhost)); /*ret true if hosts =*/
	/* does the host portion of the DNS name MISmatch? */
	if (strncmp(rhost, lhost, len))
		return(0);
	/* does the file's host have the FULL domain name, and it matches? */
	if (!strcmp(rhost, lhost))
		return(1);
	/* past the host portion must therefore be null-terminated */
	if (*(lhost + len) != '\0')
		return(0);
	/* have we tried and failed to get the default domain name? */
	if (nodomain)
		return(0);
	/* is this our first try to get the default domain name? */
	if (!domainp) {
		char *ldomain = NULL;

		if (ldomain == NULL) {
			ldomain = (char *)malloc(MAXHOSTNAMELEN+1);
			if (ldomain == NULL) {
				/*don't set nodomain=1, may have mem next time*/
				return (0);
			}
		}

		/*
		 * "domainp" points after the first dot in the host name
		 */
		if (gethostname(ldomain, MAXHOSTNAMELEN) == -1) {
			nodomain = 1;
			free(ldomain);
			ldomain = NULL;
			return(0);
		}
		ldomain[MAXHOSTNAMELEN] = (char) 0;
		if ((domainp = index(ldomain, '.')) == (char *)NULL) {
			/*last hope: get the domain name from resolver files*/
			if ((domainp=_get_domain(ldomain,MAXHOSTNAMELEN))
				    == NULL) {
				/* _get_domain failed */
				nodomain = 1;
				free(ldomain);
				ldomain = NULL;
				return(0);
			}
		}
		domainp++;
		cp = domainp;
		while (*cp) {
			*cp = isupper(*cp) ? tolower(*cp) : *cp;
			cp++;
		}
	}
	return(!strcmp(domainp, rhost + len +1));
}
