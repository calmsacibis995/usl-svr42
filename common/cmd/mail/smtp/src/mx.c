/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/mx.c	1.8.2.7"
#ident "@(#)mx.c	1.21 'attmail mail(1) command'"

#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#ifdef SYS_INET_H
# include <sys/in.h>	/* WIN/3B */
# include <sys/inet.h>	/* WIN/3B */
#endif
#ifdef NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef NOERROR
# undef NOERROR		/* Also #defined in sys/stream.h on some systems */
#endif
#include <arpa/nameser.h>
#include "miscerrs.h"
#include "s5sysexits.h"
#include "smtp.h"

#ifdef TLI
#include <tiuser.h>
#include <netconfig.h>
#include <netdir.h>
#endif

#ifdef SVR4_1
#undef MAXLABEL		/* defined in <arpa/nameser.h> also */
#include <pfmt.h>
#endif

#if defined(SVR3) || defined(SVR4)
#include <memory.h>
#define	bzero(p, n)	memset(p, '\0', n)
#define	bcopy(f, t, n)	memcpy(t, f, n)
#endif

#ifdef BIND
#define	MAILCNFG	"/etc/mail/mailcnfg"	/* should match mail.h */
#endif

extern char *realloc proto((char *, unsigned));
extern int res_mkquery proto((int, char *, int, int, char *, int, struct rrec *, char *, int));
extern int res_send proto((char *, int, char *, int));
extern int dn_skipname proto((unsigned char *, unsigned char *));
extern int dn_expand proto((unsigned char *, unsigned char *, unsigned char *, unsigned char *, int));
extern int xsetenv proto((char *));
extern void bomb proto((int));
extern int gethostname proto((char *, int));

#ifdef BIND
/* imports */
extern int errno;
extern char *malloc(), *inet_ntoa();
extern debug;
extern char *progname;
extern int no_nameserver;

extern struct hostent *gethostbyname();
extern struct hostent *gethostbyaddr();
extern struct hostent *_gethtbyname();
#ifndef SUN41
extern unsigned long inet_addr();
#endif

/* exports */
int mxconnect();

/* private */
#define MAXMXLIST 10
static struct mxitem {
	char *host;
	u_short pref;
	u_char localflag;
} MXlist[MAXMXLIST + 1];
static char *strsave();
static int buildmxlist();
static void mxsave(), mxinsert(), mxlocal();
static struct hostent *getmxhost();

#ifdef MXMAIN

#define bomb return
#include <resolv.h>

main(argc, argv)
	char **argv;
{	int fd;
	char buf[BUFSIZ], *crlf, *index();
	struct mxitem *mxp;

	_res.options |= RES_DEBUG;
	for (;;) {
		printf("domain: ");
		if (argc > 1)
			strcpy(buf, argv[1]);
		else {
			if (fgets(buf, sizeof buf, stdin) == 0)
				break;
			buf[strlen(buf)-1] = '\0';
		}
		if ((fd = mxconnect(buf)) >= 0)
			if (read(fd, buf, 512) > 0) {
				if ((crlf = index(buf, '\r')) != 0)
					strcpy(crlf, "\n");
				puts(buf);
			} else
				perror("read");
		close(fd);
		if (argc > 1)
			break;
		for (mxp = MXlist; mxp < MXlist + MAXMXLIST + 1; mxp++)
			mxp->host = 0;
	}
	return 0;
}
#endif

#ifdef SOCKET
mxconnect(host)
	char *host;
{	int s, lport, mxfatal;
	char **addr, errbuf[256];
	struct hostent *hp;
	struct servent *sp;
	struct sockaddr_in sin;
	struct mxitem *mxp;

	mxfatal = buildmxlist(host);
	if (MXlist[0].host == 0)
		MXlist[0].host = host;
	if ((sp = getservbyname ("smtp", "tcp")) == NULL) {
#ifdef SVR4_1
		(void) pfmt(stderr, ML_ERROR, ":7:unknown service TCP/smtp\n");
#else
		(void) fprintf(stderr,"%s: unknown service TCP/smtp\n", progname);
#endif
		bomb(E_OSFILE);
	}
	(void) setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *) 0, 0);

	/* slop in the loop -- i hate the socket dance */
	for (mxp = MXlist; mxp->host; mxp++) {
		int i;

		if ((hp = getmxhost(mxp->host)) == 0) {
			if (mxfatal)
				bomb(E_NOHOST);
			continue;
		}
		for (i=0; hp->h_addr_list[i]; i++) {
			s = socket(AF_INET, SOCK_STREAM, 0);
			if (s < 0) {
#ifdef SVR4_1
				pfmt(stderr, MM_ERROR, ":51:socket: %s\n", strerror(errno));
#else
				perror("socket");
#endif
				bomb(E_CANTOPEN);
			}
			bzero((char *)&sin, sizeof(sin));
			sin.sin_port = sp->s_port;
			sin.sin_family = hp->h_addrtype;
			bcopy(hp->h_addr_list[i], (char *) &sin.sin_addr,
				hp->h_length);
			if (debug)
				fprintf(stderr, "try %s [%s]", mxp->host,
				     inet_ntoa(sin.sin_addr));
			if (connect(s, (struct sockaddr *)&sin, sizeof(sin))>=0) {
				if (debug)
					fprintf(stderr, " connected\n");
				return s;
			}
			if (debug)
				fprintf(stderr, "NG, err %d\n", errno);
			close(s);
		}
		sprintf(errbuf, "%s [%s]", mxp->host, inet_ntoa(sin.sin_addr));
#ifdef SVR4_1
		pfmt(stderr, MM_ERROR, ":52:%s: %s\n", errbuf, strerror(errno));
#else
		perror(errbuf);
#endif
		close(s);
	}

	bomb(E_TEMPFAIL);
}
#endif
#endif

#ifdef TLI

#ifdef BIND
static void addone(np, sp)
struct nd_addrlist *np;
struct sockaddr_in *sp;
{
	register struct sockaddr_in *xp;
	register struct netbuf *nbuf;

	if (np->n_cnt) {
		np->n_cnt++;
		np->n_addrs = (struct netbuf *) realloc((char *) np->n_addrs,
			np->n_cnt * sizeof(struct netbuf));
	} else {
		np->n_cnt = 1;
		np->n_addrs = (struct netbuf *) malloc(sizeof(struct netbuf));
	}
	xp = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
	if ((xp == (struct sockaddr_in *) 0) || (np->n_addrs == (struct netbuf *) 0))
		bomb(E_OSFILE);

	*xp = *sp;
	nbuf = &np->n_addrs[np->n_cnt-1];

#ifdef K17
	/* These 2 fields HAVE to be "8" sizeof(taddr_in) for load k17. */
	nbuf->maxlen = sizeof(struct taddr_in);
	nbuf->len    = sizeof(struct taddr_in);
#else
	nbuf->maxlen = sizeof(struct sockaddr_in);
	nbuf->len    = sizeof(struct sockaddr_in);
#endif
	nbuf->buf    = (char *) xp;
}
#endif

int mxnetdir(ncp, ndh, nap)
struct netconfig *ncp;
struct nd_hostserv *ndh;
struct nd_addrlist **nap;
{
#ifdef BIND
	register struct mxitem *mxp;
	int mxfatal;
	struct hostent *hp;
	struct servent *sp;
	struct sockaddr_in sin;

	/* Special case tcp to use the resolver library for MX records */
	if (strcmp(ncp->nc_proto, "tcp") == 0) {
		mxfatal = buildmxlist(ndh->h_host);
		if (MXlist[0].host == 0) {
			MXlist[0].host = ndh->h_host;
			MXlist[1].host = 0;
		}
		if ((sp = getservbyname("smtp", "tcp")) == NULL)
			return 1;
		*nap = (struct nd_addrlist *) malloc(sizeof(struct nd_addrlist));
		if (*nap == (struct nd_addrlist *) 0)
			return 1;
		(*nap)->n_cnt = 0;
		for (mxp = MXlist; mxp->host; mxp++) {
			register int i;

			if ((hp = getmxhost(mxp->host)) == 0) {
				if (mxfatal)
					return 1;
				continue;
			}
			for (i = 0; hp->h_addr_list[i]; i++) {
				bzero((char *)&sin, sizeof(sin));
				sin.sin_port = sp->s_port;
				sin.sin_family = hp->h_addrtype;
				bcopy(hp->h_addr_list[i], (char *) &sin.sin_addr,
				      hp->h_length);
				addone(*nap, &sin);
				if (debug)
					fprintf(stderr, "add %s (%s)\n", inet_ntoa(sin.sin_addr), mxp->host);
			}
		}
		if ((*nap)->n_cnt == 0)
			return 1;
		return 0;
	} else
#endif
		return netdir_getbyname(ncp, ndh, nap);
}
#endif

#if defined(SOCKET) || defined(SVR4_1)
int lookup_mx(host)
char *host;
{
#ifdef BIND
	register struct mxitem *mxp;
	register struct hostent *hp;
	int mxfatal;

	mxfatal = buildmxlist(host);
	if (MXlist[0].host == 0) {
		MXlist[0].host = host;
		MXlist[1].host = 0;
	}
	for (mxp = MXlist; mxp->host; mxp++) {
		if ((hp = getmxhost(mxp->host)) == 0) {
			if (mxfatal)
				return 0;
			continue;
		}
		if (hp->h_addr_list[0])
			return 1;
	}
	return 0;
#else
	return gethostbyname(host) != (struct hostent *) 0;
#endif
}
#endif

#ifdef SVR4_1
void *next_mx(vp, cpp)
void *vp;
char **cpp;
{
	register struct mxitem *mxp;

	if (vp == (void *) 0)
		vp = (void *) MXlist;
	mxp = (struct mxitem *) vp;
	if (mxp->host) {
		*cpp = mxp->host;
		return (void *) ++mxp;
	} else {
		*cpp = NULL;
		return (void *) 0;
	}
}
#endif

#ifdef BIND
/* return 1 for fatal MX error (authoritative NXDOMAIN), 0 o.w. */
static int
buildmxlist(host)
	char *host;
{	register HEADER *hp;
	register char *cp;
	register int n;
	char q[PACKETSZ], a[PACKETSZ];	/* query, answer */
	char *eom, *bp;
	int buflen, ancount, qdcount;
	char hostbuf[BUFSIZ+1];
	u_short preference, reclen;
	int niter = 0;
	char nhostbuf[BUFSIZ+1];

#ifdef SVR3
	/*
	 * Under WIN, if NONAMESERVER is set, we probably don't want
	 * to do MX lookups
	 */
	if (getenv("NONAMESERVER"))
		return 0;
#endif
	if ((host == NULL) || (*host == '\0'))
		return 0;
	if (no_nameserver)
		return 0;
again:
	if ((n = res_mkquery(QUERY, host, C_IN, T_MX, (char *) 0, 0, (struct rrec *) 0, q, sizeof(q))) < 0)
		return 0;
	n = res_send(q, n, a, sizeof(a));
	if (n < 0)
		return 0;
	if (debug)
		fprintf(stderr, "buildmxlist got %d\n", n);
	eom = a + n;
	hp = (HEADER *) a;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	if (debug)
		fprintf(stderr, "buildmx rcode %d ancount %d qdcount %d aa %d\n",
		 hp->rcode, ancount, qdcount, hp->aa);
	if (hp->rcode != NOERROR || ancount == 0) {
		if (hp->aa == 0)
			return 0;	/* non-authoritative in any event */
		return hp->rcode == NOERROR || hp->rcode == NXDOMAIN;
	}
	bp = hostbuf;
	buflen = sizeof(hostbuf);
	cp = a + sizeof(HEADER);
	if (debug>1) {
		int i;
		fprintf(stderr, "buildmx got:");
		for (i=0; i<n; i++)
			if (a[i]>=' '&& a[i]<0177)
				fprintf(stderr, "%c", a[i]);
			else fprintf(stderr, "\\%.3o", a[i]&0377);
		fprintf(stderr, "\n");
	}
	while (--qdcount >= 0)
		cp += dn_skipname((u_char*)cp,(u_char*)eom) + QFIXEDSZ;
	/* TODO: if type is CNAME, reissue query */
	while (--ancount >= 0 && cp < eom) {
		int type;
		cp += dn_skipname((u_char*)cp,(u_char*)eom);	/* name */
		type = _getshort((u_char*)cp);	
		cp += sizeof(u_short)	/* type */
		    + sizeof(u_short)	/* class */
		    + sizeof(u_long);	/* ttl (see rfc973) */
		reclen = _getshort((u_char*)cp);
		cp += sizeof(u_short);
		if (type==T_CNAME) {	/* canonical name */
			if (dn_expand((u_char*)a, (u_char*)eom, (u_char*)cp, (u_char*)nhostbuf, BUFSIZ) < 0)
				if (debug)
					fprintf(stderr, "CNAME? in mxexpand\n");
			host = nhostbuf;
			if (++niter>10)
				return 0;
			if (debug)
				fprintf(stderr, "CNAME, try with %s\n", host);
			goto again;
		}
		preference = _getshort((u_char*)cp);
		if ((n = dn_expand((u_char*)a, (u_char*)eom, (u_char*)(cp + sizeof(u_short)), (u_char*)bp, buflen)) < 0)
			break;
		mxsave(bp, preference);
		cp += reclen;
	}
	mxlocal();
	return 0;
}

/* NOT TODO: issue WKS query.  (just try to connect.) */

static void
mxsave(host, pref)
	char *host;
	u_short pref;
{	struct mxitem *mxp;
	int localflag;
	static char thishost[64];
	static char thishost2[80];

	if (*thishost == 0) {
		gethostname(thishost, sizeof(thishost));
		thishost2[0] = '\0';
		if (strchr(thishost, '.') == NULL) {
			extern char *maildomain();
			(void) xsetenv(MAILCNFG);
			(void) strcpy(thishost2, thishost);
			(void) strcat(thishost2, maildomain());
		}
	}
	if (debug)
		fprintf(stderr, "MXsave %s\n", host);

	if (MXlist[MAXMXLIST].host)
		return;				/* full */

	localflag = (strcmp(thishost, host) == 0);
	localflag |= (strcmp(thishost2, host) == 0);

	/* insertion sort */
	for (mxp = MXlist; mxp < MXlist + MAXMXLIST; mxp++) {
		if (mxp->host == 0) {
			mxinsert(mxp, host, pref, localflag);
			return;
		}
		if (pref < mxp->pref) {
			mxinsert(mxp, host, pref, localflag);
			return;
		}
		if (pref == mxp->pref) {
			if (mxp->localflag)
				return;
			if (localflag) {
				mxp->host = strsave(host);
				mxp->pref = pref;
				mxp->localflag = localflag;
				(++mxp)->host = 0;
				return;
			}
			mxinsert(mxp, host, pref, localflag);
			return;
		}
	}
}

static void
mxinsert(mxlistp, host, pref, localflag)
	struct mxitem *mxlistp;
	char *host;
	u_short pref;
{	register struct mxitem *mxp;

	for (mxp = MXlist + MAXMXLIST - 1; mxp > mxlistp; --mxp)
		*mxp = mxp[-1];
	mxp->host = strsave(host);
	mxp->pref = pref;
	mxp->localflag = localflag;
}

static char *
strsave(str)
	register char *str;
{	register char *rval;

	if ((rval = malloc(strlen(str) + 1)) == 0) {
#ifdef SVR4_1
		pfmt(stderr, MM_ERROR, ":53:malloc: %s\n", strerror(errno));
#else
		perror("malloc");
#endif
		bomb(-EX_SOFTWARE);
	}
	strcpy(rval, str);
	return rval;
}

static void
mxlocal()
{
	register struct mxitem *mxp;

	for (mxp = MXlist; mxp->host; mxp++) {
		if (mxp->localflag) {
			mxp->host = 0;
			break;
		}
	}
}

static struct hostent *
getmxhost(host)
	char *host;
{	struct hostent *hp;
	static struct in_addr ia;
	static char *hlist[2] = {0, 0};
	static struct hostent he = {0, 0, AF_INET, sizeof(struct in_addr), hlist};

	if (!host)
		return 0;

	/* If no_nameserver is not set, use the dynamic lookup. */
	if (!no_nameserver && (hp = gethostbyname(host)) != 0)
		return hp;

	/* If the dynamic lookup failed, try a static table lookup. */
	if ((hp = _gethtbyname(host)) != 0)
		return hp;

	if ((ia.s_addr = inet_addr(host)) != -1) {
		he.h_addr = (char *)&ia;
		return &he;
	}
	if (debug)
		(void) fprintf(stderr, "Error looking up Host \"%s\".\n", host);
	return 0;
}
#endif /* BIND */

/*
 * The following two routines are duplicated from the resolver library
 * because some ports do not contain these functions.
 */
#ifdef	NEED_SKIPNAME
/*
 * Skip over a compressed domain name. Return the size or -1.
 */
dn_skipname(comp_dn, eom)
	u_char *comp_dn, *eom;
{
	register u_char *cp;
	register int n;

	cp = comp_dn;
	while (cp < eom && (n = *cp++)) {
		/*
		 * check for indirection
		 */
		switch (n & INDIR_MASK) {
		case 0:		/* normal case, n == len */
			cp += n;
			continue;
		default:	/* illegal type */
			return (-1);
		case INDIR_MASK:	/* indirection */
			cp++;
		}
		break;
	}
	return (cp - comp_dn);
}
#endif

#ifdef NEED_GETSHORT
/*
 * Routines to insert/extract short/long's. Must account for byte
 * order and non-alignment problems. This code at least has the
 * advantage of being portable.
 *
 * used by sendmail.
 */

u_short
_getshort(msgp)
	u_char *msgp;
{
	register u_char *p = (u_char *) msgp;
#ifdef vax
	/*
	 * vax compiler doesn't put shorts in registers
	 */
	register u_long u;
#else
	register u_short u;
#endif

	u = *p++ << 8;
	return ((u_short)(u | *p));
}
#endif
