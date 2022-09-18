/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/tcpip/tcpip.c	1.5.9.7"
#ident  "$Header: tcpip.c 1.3 91/09/20 $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
/*
 * TCP/IP name to address translation routines. These routines are written
 * to the getXXXbyYYY() interface that the BSD routines use. This allows
 * us to simply rewrite those routines to get various flavors of translation
 * routines. Thus while they look like they have socket dependencies (the
 * sockaddr_in structures) in fact this is simply the internal netbuf
 * representation that the TCP and UDP transport providers use.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/byteorder.h>
#include <netinet/in.h>
#include <netdb.h>
#include <tiuser.h>
#include <sys/timod.h>              /* XXX needed for work-around */
#include <netconfig.h>
#include <netdir.h>
#include <string.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/utsname.h>
#include <net/if.h>
#include <stropts.h>
#include <sys/ioctl.h>
#include <rpc/types.h>
#ifdef SYSLOG
#include <sys/syslog.h>
#else
#define	LOG_ERR 3
#endif /* SYSLOG */

#define IFCONFIGDELAY 60

extern char	*malloc(), *calloc();
static char	*inet_ntoa();
static u_long   inet_addr();
static struct in_addr inet_makeaddr();
extern int	errno;
extern int	t_errno;
extern int	_nderror;

static char 	*localaddr[] = {"\000\000\000\000", NULL};
static char     UDPDEV[] = "/dev/udp";

static struct hostent localent = {
		"Localhost",
		NULL,
		AF_INET,
		4,
		localaddr
};
#define MAXBCAST        200

#ifdef undef
/*
 * XXX: May return more than one address. INADDR_BROADCAST used here
 */
static char 	*broadaddr[] = {"\377\377\377\377", NULL};

static struct hostent broadent = {
		"broadcast",
		NULL,
		AF_INET,
		4,
		broadaddr
};
#endif

/*
 * This routine is the "internal" TCP/IP routine that will build a
 * host/service pair into one or more netbufs depending on how many
 * addresses the host has in the host table.
 * If the hostname is HOST_SELF, we return 0.0.0.0 so that the
 * binding can be contacted through all interfaces.
 * If the hostname is HOST_ANY, we return no addresses because IP doesn't
 * know how to specify a service without a host.
 * And finally if we specify HOST_BROADCAST then we ask a tli fd to tell
 * us what the broadcast addresses are for any udp interfaces on this
 * machine.
 */
struct nd_addrlist *
#ifdef PIC
_netdir_getbyname(tp, serv)
#else
#ifdef YP
nis_netdir_getbyname(tp, serv)
#else
tcp_netdir_getbyname(tp, serv)
#endif YP
#endif PIC
	struct netconfig *tp;
	struct nd_hostserv *serv;
{
	struct hostent	*he;
	struct hostent	h_broadcast;
	struct nd_addrlist *result;
	struct netbuf	*na;
	char		**t;
	struct sockaddr_in	*sa;
	int		num;
	int		server_port;
	char		*baddrlist[MAXBCAST + 1];
	struct in_addr  *inaddrs;
	struct in_addr  *getbroadcastnets();

	if (!serv || !tp) {
		_nderror = ND_BADARG;
		return (NULL);
	}
	_nderror = ND_OK;	/* assume success */

	/* NULL is not allowed, that returns no answer */
	if (! (serv->h_host)) {
		_nderror = ND_NOHOST;
		return (NULL);
	}

	/*
	 * Find the port number for the service. We look for some
	 * special cases first and on failing go into getservbyname().
	 * The special cases :
	 * 	NULL - 0 port number.
	 *	rpcbind - The rpcbind's address
	 *	A number - We don't have a name just a number so use it
	 * ifdef YP part would used by nis/Makefile that builds
	 * nis.so.
	 */
	if (!(serv->h_serv)) {
		server_port = htons(0);
	} else if (strcmp(serv->h_serv, "rpcbind") == 0) {
		server_port = htons(111);	/* Hard coded */
	} else if (strspn(serv->h_serv, "0123456789")
			== strlen(serv->h_serv)) {
		/* It's a port number */
		server_port = htons(atoi(serv->h_serv));
	} else {
		struct servent	*se;

#ifdef YP
		se = (struct servent *)_nis_getservbyname(serv->h_serv,
			(strcmp(tp->nc_proto, NC_TCP) == 0) ? "tcp" : "udp");
#else YP
		se = (struct servent *)_tcpip_getservbyname(serv->h_serv,
			(strcmp(tp->nc_proto, NC_TCP) == 0) ? "tcp" : "udp");
#endif YP

		if (!se) {
			_nderror = ND_NOSERV;
			return (NULL);
		}
		server_port = se->s_port;
	}

	if (!strcmp(serv->h_host, HOST_SELF)) {
		he = &localent;
	} else if ((strcmp(serv->h_host, HOST_BROADCAST) == 0)) {
		int i;

		inaddrs = getbroadcastnets(tp);
		if (inaddrs == NULL)
			return (NULL);
		he = &h_broadcast;
		he->h_name = "broadcast";
		he->h_aliases = NULL;
		he->h_addrtype = AF_INET;
		he->h_length = 4;
		for (i = 0; i <= MAXBCAST && inaddrs[i].s_addr != INADDR_ANY; i++)
			baddrlist[i] = (char *)&inaddrs[i];
		baddrlist[i] = NULL;
		he->h_addr_list = baddrlist;
	} else {
#ifdef YP
		he = (struct hostent *)_nis_gethostbyname(serv->h_host);
#else YP
		he = (struct hostent *)_tcpip_gethostbyname(serv->h_host);
#endif YP
	}

	if (!he) {
		int i1, i2, i3, i4;

		/*
		 * check if it is an universal address
		 */
		if (sscanf(serv->h_host ,"%d.%d.%d.%d", &i1, &i2, &i3, &i4) != 4) {
			_nderror = ND_NOHOST;
			return (NULL);
		}

		result = (struct nd_addrlist *)(malloc(sizeof(struct nd_addrlist)));
		if (!result) {
			_nderror = ND_NOMEM;
			return (NULL);
		}
		na = _uaddr2taddr(tp, serv->h_host);
		if (!na) {
			free(result);
			return (NULL);
		}
		result->n_cnt = 1;
		result->n_addrs = na;
		((struct sockaddr_in *)na->buf)->sin_port = server_port;
		return(result);
	}

	result = (struct nd_addrlist *)(malloc(sizeof (struct nd_addrlist)));
	if (!result) {
		_nderror = ND_NOMEM;
		return (NULL);
	}

	/* Count the number of addresses we have */
	for (num = 0, t = he->h_addr_list; *t; t++, num++)
			;

	result->n_cnt = num;
	result->n_addrs = (struct netbuf *)
				(calloc(num, sizeof (struct netbuf)));
	if (!result->n_addrs) {
		_nderror = ND_NOMEM;
		return (NULL);
	}

	/* build up netbuf structs for all addresses */
	for (na = result->n_addrs, t = he->h_addr_list; *t; t++, na++) {
		sa = (struct sockaddr_in *)calloc(1, sizeof (*sa));
		if (!sa) {
			_nderror = ND_NOMEM;
			return (NULL);
		}
		/* Vendor specific, that is why it's here and hard coded */
		na->maxlen = sizeof (struct sockaddr_in);
		na->len = sizeof (struct sockaddr_in);
		na->buf = (char *)sa;
		sa->sin_family = AF_INET;
		sa->sin_port = server_port;
		sa->sin_addr = *((struct in_addr *)(*t));
	}

	return (result);
}

/*
 * This routine is the "internal" TCP/IP routine that will build a
 * host/service pair from the netbuf passed. Currently it only
 * allows one answer, it should, in fact allow several.
 */
struct nd_hostservlist *
#ifdef PIC
_netdir_getbyaddr(tp, addr)
#else
#ifdef YP
nis_netdir_getbyaddr(tp, addr)
#else
tcp_netdir_getbyaddr(tp, addr)
#endif YP
#endif PIC
	struct netconfig	*tp;
	struct netbuf		*addr;
{
	struct sockaddr_in	*sa;		/* TCP/IP temporaries */
	struct servent		*se;
	struct hostent		*he;
	struct nd_hostservlist	*result;	/* Final result		*/
	struct nd_hostserv	*hs;		/* Pointer to the array */
	int			servs, hosts;	/* # of hosts, services */
	char			**hn, **sn;	/* host, service names */
	int			i, j;		/* some counters	*/

	if (!addr || !tp) {
		_nderror = ND_BADARG;
		return (NULL);
	}
	_nderror = ND_OK; /* assume success */

	/* XXX This is a user-provided address. */
	sa = (struct sockaddr_in *)(addr->buf);

	/* first determine the host */
#ifdef YP
	he = (struct hostent *)_nis_gethostbyaddr(&(sa->sin_addr.s_addr),
			4, sa->sin_family);
#else
	he = (struct hostent *)_tcpip_gethostbyaddr(&(sa->sin_addr.s_addr),
			4, sa->sin_family);
#endif YP
	if (!he) {
		_nderror = ND_NOHOST;
		return (NULL);
	}

	/* Now determine the service */
	if (sa->sin_port == 0) {
		/*
		 * The port number 0 is a reserved port for both UDP & TCP.
		 * We are assuming that this is used to just get
		 * the host name and to bypass the service name.
		 */
		servs = 1;
		se = NULL;
	} else {
#ifdef YP
		se = (struct servent *)_nis_getservbyport(sa->sin_port,
			(strcmp(tp->nc_proto, NC_TCP) == 0) ? "tcp" : "udp");
#else
		se = (struct servent *)_tcpip_getservbyport(sa->sin_port,
			(strcmp(tp->nc_proto, NC_TCP) == 0) ? "tcp" : "udp");
#endif YP
		if (!se) {
			/* It is not a well known service */
			servs = 1;
		}
	}

	/* now build the result for the client */
	result = (struct nd_hostservlist *)
			malloc(sizeof (struct nd_hostservlist));
	if (!result) {
		_nderror = ND_NOMEM;
		return (NULL);
	}

	/*
	 * We initialize the counters to 1 rather than zero because
	 * we have to count the "official" name as well as the aliases.
	 */
	for (hn = he->h_aliases, hosts = 1; hn && *hn; hn++, hosts++)
		;

	if (se)
		for (sn = se->s_aliases, servs = 1; sn && *sn; sn++, servs++)
			;

	hs = (struct nd_hostserv *)calloc(hosts * servs,
			sizeof (struct nd_hostserv));
	if (!hs) {
		_nderror = ND_NOMEM;
		free((void *)result);
		return (NULL);
	}

	result->h_cnt	= servs * hosts;
	result->h_hostservs = hs;

	/* Now build the list of answers */

	for (i = 0, hn = he->h_aliases; i < hosts; i++) {
		sn = se ? se->s_aliases : NULL;

		for (j = 0; j < servs; j++) {
			if (! i)
				hs->h_host = strdup(he->h_name);
			else
				hs->h_host = strdup(*hn);
			if (! j) {
				if (se) {
					hs->h_serv = strdup(se->s_name);
				} else {
					/* Convert to a number string */
					char stmp[16];

					sprintf(stmp, "%d", sa->sin_port);
					hs->h_serv = strdup(stmp);
				}
			} else {
				hs->h_serv = strdup(*sn++);
			}

			if (!(hs->h_host) || !(hs->h_serv)) {
				_nderror = ND_NOMEM;
				free((void *)result->h_hostservs);
				free((void *)result);
				return (NULL);
			}
			hs ++;
		}
		if (i)
			hn++;
	}

	return (result);
}

/*
 * This internal routine will merge one of those "universal" addresses
 * to the one which will make sense to the remote caller.
 *
 * SNI:
 *          all previous implementations failed to work on
 *          multihomed hosts or when routes were involved.
 *
 *          We solve this problem by connecting to a udp socket.
 *          This causes all routing tables to be checked in order
 *          to bind the local socket to an IP provider.
 *
 *          The internet address of this endpoint should be use by
 *          our peer to connect to us.
 */

static char *
_netdir_mergeaddr(tp, ruaddr, uaddr)
	struct netconfig	*tp;	/* the transport provider */
	char			*ruaddr; /* remote uaddr of the caller */
	char			*uaddr;	/* the address */
{
	char                    *ret, *retuaddr, *portptr;
	struct netbuf           *rtaddr;
	int                     j;
	int                     fd;
	struct t_call           tcall;

	_nderror = ND_OK;
	if (!uaddr || !ruaddr || !tp) {
		_nderror = ND_BADARG;
		return ((char *)NULL);
	}
	if (strncmp(ruaddr, "0.0.0.0.", strlen("0.0.0.0.")) == 0)
		/* thats me: return the way it is */
		return (strdup(uaddr));

	fd = t_open(UDPDEV, 2, NULL);
	if (fd < 0) {
		_nderror = ND_OPEN;
		return(NULL);
	}

	if (!(rtaddr = _uaddr2taddr(tp, ruaddr))) {
		t_close(fd);
		return (NULL);
	}
	rtaddr->maxlen = rtaddr->len;

	tcall.addr = *rtaddr;
	tcall.opt.len = tcall.opt.maxlen = 0;
	tcall.udata.len = tcall.udata.maxlen = 0;


	/*
	 * XXX
	 * Work-around: UDP is capable of a T_CON_REQ, but
	 * TLI gives an error for a connectionless transport service,
	 * so we lie about the transport service.
	 */
	{
		extern struct _ti_user *_ti_user;

		_ti_user[fd].ti_servtype = T_COTS;
	}
	/*
	 * XXX end of work-around
	 */

	if (t_bind(fd, NULL, NULL)      < 0 ||
	    t_connect(fd, &tcall, NULL) < 0 ||
	    t_getprotaddr(fd, rtaddr, NULL) < 0) {

		netdir_free(rtaddr, ND_ADDR);
		t_close(fd);
		_nderror = ND_FAILCTRL;
		return(NULL);
	}
	t_close(fd);

	/*
	 * here we have got an address which may be used to
	 * access the remote host. It is the same address, that
	 * the remote host may use to access us.
	 */

	retuaddr = _taddr2uaddr(tp, rtaddr);
	netdir_free(rtaddr, ND_ADDR);
	if (!retuaddr)
		return(NULL);

	/* Get the host part */
	for (portptr = retuaddr, j = 0; j < 4; j++) {
		portptr = strchr(portptr, '.');
		portptr++;
	}
	*portptr = '\0';

	/* Get the port number */
	for (portptr = uaddr, j = 0; j < 4; j++) {
		portptr = strchr(portptr, '.');
		portptr++;
	}

	ret = malloc(strlen(portptr) + strlen(retuaddr) + 1);
	if (!ret) {
		free(retuaddr);
		_nderror = ND_NOMEM;
		return(NULL);
	}
	(void) strcpy(ret, retuaddr);
	(void) strcat(ret, portptr);
	free(retuaddr);
	return (ret);
}

int
#ifdef PIC
_netdir_options(tp, opts, fd, par)
#else
#ifdef YP
nis_netdir_options(tp, opts, fd, par)
#else
tcp_netdir_options(tp, opts, fd, par)
#endif YP
#endif PIC
	struct netconfig *tp;
	int opts;
	int fd;
	char *par;
{
	struct t_optmgmt *options;
	struct t_optmgmt *optionsret;
	struct nd_mergearg *ma;

	struct sochdr {
		struct opthdr opthdr;
		long value;
	} sochdr;
	int retval = 0;

	switch (opts) {
	case ND_SET_BROADCAST:
	/* 
	 * The handling of return values and the _nderror global variable in
	 * the code for ND_SET_BROADCAST is inconsistent with the description
	 * on the netdir(3N) man page.  Following the TLI t_optmgmt() example,
	 * netdir_options() should return -1 on failure and set _nderror to
	 * the appropriate value (the value now being returned).  That is
	 * not how this code for works, so it may be changed in a
	 * later release.
	 */
		/* enable for broadcasting */
		options = (struct t_optmgmt *)t_alloc(fd, T_OPTMGMT, 0);
		if (options == (struct t_optmgmt *) NULL)
			return (ND_NOMEM);
		optionsret = (struct t_optmgmt *)t_alloc(fd, T_OPTMGMT, T_OPT);
		if (optionsret == (struct t_optmgmt *) NULL) {
			(void) t_free((char *) options, T_OPTMGMT);
			return (ND_NOMEM);
		}
		sochdr.opthdr.level = SOL_SOCKET;
		sochdr.opthdr.name = SO_BROADCAST;
		sochdr.opthdr.len = 4;
		sochdr.value = 1;		/* ok to broadcast */
		options->opt.maxlen = sizeof (sochdr);
		options->opt.len = sizeof (sochdr);
		options->opt.buf =  (char *) &sochdr;
		options->flags = T_NEGOTIATE;
		if (t_optmgmt(fd, options, optionsret) == -1) {
			/*
			 *	Should we return an error here, or ignore it
			 *	in case the provider allows broadcasting but
			 *	doesn't know about this option?  For now, we
			 *	silently ignore the error.
			 */
		}
		options->opt.buf = (char *) NULL;
		(void) t_free((char *)options, T_OPTMGMT);
		(void) t_free((char *)optionsret, T_OPTMGMT);
		return (_nderror);
	case ND_CLEAR_BROADCAST:
		/* disable for broadcasting */
		options = (struct t_optmgmt *)t_alloc(fd, T_OPTMGMT, 0);
		if (options == (struct t_optmgmt *) NULL) {
			_nderror = ND_NOMEM;
			return (-1);
		}
		optionsret = (struct t_optmgmt *)t_alloc(fd, T_OPTMGMT, T_OPT);
		if (optionsret == (struct t_optmgmt *) NULL) {
			t_free(options, T_OPTMGMT);
			_nderror = ND_NOMEM;
			return (-1);
		}
		sochdr.opthdr.level = SOL_SOCKET;
		sochdr.opthdr.name = SO_BROADCAST;
		sochdr.opthdr.len = 4;
		sochdr.value = 0;		/* not ok to broadcast */
		options->opt.maxlen = sizeof (sochdr);
		options->opt.len = sizeof (sochdr);
		options->opt.buf =  (char *) &sochdr;
		options->flags = T_NEGOTIATE;
		if (t_optmgmt(fd, options, optionsret) == -1) {
			retval = -1;
			_nderror = ND_FAILCTRL;
		}
		options->opt.buf = (char *) NULL;
		(void) t_free((char *)options, T_OPTMGMT);
		(void) t_free((char *)optionsret, T_OPTMGMT);
		return (retval);
	case ND_SET_REUSEADDR:
		/* enable address reuse */
		options = (struct t_optmgmt *)t_alloc(fd, T_OPTMGMT, 0);
		if (options == (struct t_optmgmt *) NULL) {
			_nderror = ND_NOMEM;
			return (-1);
		}
		optionsret = (struct t_optmgmt *)t_alloc(fd, T_OPTMGMT, T_OPT);
		if (optionsret == (struct t_optmgmt *) NULL) {
			t_free(options, T_OPTMGMT);
			_nderror = ND_NOMEM;
			return (-1);
		}
		sochdr.opthdr.level = SOL_SOCKET;
		sochdr.opthdr.name = SO_REUSEADDR;
		sochdr.opthdr.len = 4;
		sochdr.value = 1;		/* ok to reuse address */
		options->opt.maxlen = sizeof (sochdr);
		options->opt.len = sizeof (sochdr);
		options->opt.buf =  (char *) &sochdr;
		options->flags = T_NEGOTIATE;
		if (t_optmgmt(fd, options, optionsret) == -1) {
			retval = -1;
			_nderror = ND_FAILCTRL;
		}
		options->opt.buf = (char *) NULL;
		(void) t_free((char *)options, T_OPTMGMT);
		(void) t_free((char *)optionsret, T_OPTMGMT);
		return (retval);
	case ND_CLEAR_REUSEADDR:
		/* disable address reuse */
		options = (struct t_optmgmt *)t_alloc(fd, T_OPTMGMT, 0);
		if (options == (struct t_optmgmt *) NULL) {
			_nderror = ND_NOMEM;
			return (-1);
		}
		optionsret = (struct t_optmgmt *)t_alloc(fd, T_OPTMGMT, T_OPT);
		if (optionsret == (struct t_optmgmt *) NULL) {
			t_free(options, T_OPTMGMT);
			_nderror = ND_NOMEM;
			return (-1);
		}
		sochdr.opthdr.level = SOL_SOCKET;
		sochdr.opthdr.name = SO_REUSEADDR;
		sochdr.opthdr.len = 4;
		sochdr.value = 0;		/* not ok to reuse address */
		options->opt.maxlen = sizeof (sochdr);
		options->opt.len = sizeof (sochdr);
		options->opt.buf =  (char *) &sochdr;
		options->flags = T_NEGOTIATE;
		if (t_optmgmt(fd, options, optionsret) == -1) {
			retval = -1;
			_nderror = ND_FAILCTRL;
		}
		options->opt.buf = (char *) NULL;
		(void) t_free((char *)options, T_OPTMGMT);
		(void) t_free((char *)optionsret, T_OPTMGMT);
		return (retval);
	case ND_SET_RESERVEDPORT:	/* bind to a resered port */
		return (bindresvport(fd, (struct netbuf *)par));
	case ND_CHECK_RESERVEDPORT:	/* check if reserved prot */
		return (checkresvport((struct netbuf *)par));
	case ND_MERGEADDR:	/* Merge two addresses */
		ma = (struct nd_mergearg *)(par);
		ma->m_uaddr = _netdir_mergeaddr(tp, ma->c_uaddr, ma->s_uaddr);
		return (_nderror);
	default:
		return (ND_NOCTRL);
	}
}


/*
 * This internal routine will convert a TCP/IP internal format address
 * into a "universal" format address. In our case it prints out the
 * decimal dot equivalent. h1.h2.h3.h4.p1.p2 where h1-h4 are the host
 * address and p1-p2 are the port number.
 */
char *
#ifdef PIC
_taddr2uaddr(tp, addr)
#else
#ifdef YP
nis_taddr2uaddr(tp, addr)
#else
tcp_taddr2uaddr(tp, addr)
#endif YP
#endif PIC
	struct netconfig	*tp;	/* the transport provider */
	struct netbuf		*addr;	/* the netbuf struct */
{
	struct sockaddr_in	*sa;	/* our internal format */
	char			tmp[32];
	unsigned short		myport;

	if (!addr || !tp) {
		_nderror = ND_BADARG;
		return (NULL);
	}
	sa = (struct sockaddr_in *)(addr->buf);
	myport = ntohs(sa->sin_port);
	sprintf(tmp, "%s.%d.%d", inet_ntoa(sa->sin_addr),
			myport >> 8, myport & 255);
	return (strdup(tmp));	/* Doesn't return static data ! */
}

/*
 * This internal routine will convert one of those "universal" addresses
 * to the internal format used by the Sun TLI TCP/IP provider.
 */

struct netbuf *
#ifdef PIC
_uaddr2taddr(tp, addr)
#else
#ifdef YP
nis_uaddr2taddr(tp, addr)
#else
tcp_uaddr2taddr(tp, addr)
#endif YP
#endif PIC
	struct netconfig	*tp;	/* the transport provider */
	char			*addr;	/* the address */
{
	struct sockaddr_in	*sa;
	unsigned long		inaddr;
	unsigned short		inport;
	int			h1, h2, h3, h4, p1, p2;
	struct netbuf		*result;

	if (!addr || !tp) {
		_nderror = ND_BADARG;
		return (0);
	}
	result = (struct netbuf *) malloc(sizeof (struct netbuf));
	if (!result) {
		_nderror = ND_NOMEM;
		return (0);
	}

	sa = (struct sockaddr_in *)calloc(1, sizeof (*sa));
	if (!sa) {
		free((void *)result);
		_nderror = ND_NOMEM;
		return (0);
	}
	result->buf = (char *)(sa);
	result->maxlen = sizeof (struct sockaddr_in);
	result->len = sizeof (struct sockaddr_in);

	/* XXX there is probably a better way to do this. */
	sscanf(addr, "%d.%d.%d.%d.%d.%d", &h1, &h2, &h3, &h4, &p1, &p2);

	/* convert the host address first */
	inaddr = (h1 << 24) + (h2 << 16) + (h3 << 8) + h4;
	sa->sin_addr.s_addr = htonl(inaddr);

	/* convert the port */
	inport = (p1 << 8) + p2;
	sa->sin_port = htons(inport);

	sa->sin_family = AF_INET;

	return (result);
}



/* (#)inet_ntoa.c 1.7 88/02/08 SMI";  from UCB 4.1 83/06/12 */

/*
 * Convert network-format internet address
 * to base 256 d.d.d.d representation.
 */
static char *
inet_ntoa(in)
	struct in_addr in;
{
	static char b[18];
	register char *p;

	p = (char *)&in;
#define	UC(b)	(((int)b)&0xff)
	sprintf(b, "%d.%d.%d.%d", UC(p[0]), UC(p[1]), UC(p[2]), UC(p[3]));
	return (b);
}

static struct in_addr *
getbroadcastnets(tp)
	struct netconfig *tp;
{
	static struct in_addr *addrs = NULL;
	static time_t           nexttime;
	time_t                  thistime;
	struct ifreq *ifr;
	int fd;
	int n, i, len, naddrs;
	char buf[8192];

	if (nexttime < time(&thistime) || addrs == NULL) {
		nexttime = thistime + IFCONFIGDELAY;
		fd = open(tp->nc_device, O_RDONLY);
		if (fd < 0) {
			(void) syslog(LOG_ERR,
			"broadcast: ioctl (get interface configuration): %m");
			_nderror = ND_OPEN;
			return (NULL);
		}
		if ((len = ifioctl(fd, SIOCGIFCONF, buf, sizeof(buf))) < 0) {
			(void) syslog(LOG_ERR,
			"broadcast: ioctl (get interface configuration): %m");
			close(fd);
			_nderror = ND_FAILCTRL;
			return (NULL);
		}
		naddrs = len/sizeof(struct ifreq) + 1;

		if (addrs)
			addrs = (struct in_addr *) realloc(addrs, naddrs*sizeof(struct in_addr));
		else
			addrs = (struct in_addr *) malloc(naddrs*sizeof(struct in_addr));

		if (addrs == NULL) {
			close(fd);
			_nderror = ND_NOMEM;
			return(NULL);
		}

		i = 0;
		for (ifr = (struct ifreq *)buf;
		     ifr < (struct ifreq *)(buf + len);
		     ifr++) {
			if (ifr->ifr_addr.sa_family != AF_INET)
				continue;
			addrs[i] = ((struct sockaddr_in *)&ifr->ifr_addr)->sin_addr;
			if (ifioctl(fd, SIOCGIFFLAGS, (char *)ifr, 0) < 0) {
				(void) syslog(LOG_ERR,
				"broadcast: ioctl (get interface flags): %m");
				continue;
			}
			if ((ifr->ifr_flags & IFF_BROADCAST) &&
			    (ifr->ifr_flags & IFF_UP)) {
				if (ifioctl(fd, SIOCGIFBRDADDR, (char *)ifr, 0) < 0) {
					/* May not work with other implementation */
					addrs[i] = inet_makeaddr(
					   inet_netof(addrs[i]), INADDR_ANY);
				} else {
					addrs[i] = ((struct sockaddr_in*)
							&ifr->ifr_addr)->sin_addr;
				}
				i++;
			}
		}
		close(fd);
		addrs[i].s_addr = INADDR_ANY;
	}
	return(addrs);
}

/* "@(#)inet_makeaddr.c 1.8 88/02/08 SMI"; from UCB 4.4 85/06/02 */

/*
 * Formulate an Internet address from network + host.  Used in
 * building addresses stored in the ifnet structure.
 */
static struct in_addr
inet_makeaddr(net, host)
	int net, host;
{
	u_long addr;

	if (net < 128)
		addr = (net << IN_CLASSA_NSHIFT) | (host & IN_CLASSA_HOST);
	else if (net < 65536)
		addr = (net << IN_CLASSB_NSHIFT) | (host & IN_CLASSB_HOST);
	else
		addr = (net << IN_CLASSC_NSHIFT) | (host & IN_CLASSC_HOST);
	addr = htonl(addr);
	return (*(struct in_addr *)&addr);
}

static
ifioctl(s, cmd, arg, len)
	char *arg;
{
	struct strioctl ioc;

	ioc.ic_cmd = cmd;
	ioc.ic_timout = 0;
	if (len)
		ioc.ic_len = len;
	else
		ioc.ic_len = sizeof (struct ifreq);
	ioc.ic_dp = arg;
	return (ioctl(s, I_STR, (char *) &ioc) < 0 ? -1 : ioc.ic_len);
}

static
bindresvport(fd, addr)
	int fd;
	struct netbuf *addr;
{
	int res;
	static short port;
	struct sockaddr_in myaddr;
	struct sockaddr_in *sin;
	int i;
	struct t_bind *tbind, *tres;

#define	STARTPORT 600
#define	ENDPORT (IPPORT_RESERVED - 1)
#define	NPORTS	(ENDPORT - STARTPORT + 1)

	_nderror = ND_SYSTEM;
	if ((i = t_getstate(fd)) != T_UNBND) {
		if (t_errno == TBADF)
			errno = EBADF;
		if (i != -1)
			errno = EISCONN;
		return (-1);
	}
	if (addr == NULL) {
		sin = &myaddr;
		(void)memset((char *)sin, 0, sizeof (*sin));
		sin->sin_family = AF_INET;
	} else {
		sin = (struct sockaddr_in *)addr->buf;
		if (sin->sin_family != AF_INET) {
			errno = EPFNOSUPPORT;
			return (-1);
		}
	}
	if (port == 0)
		port = (getpid() % NPORTS) + STARTPORT;
	res = -1;
	/* Transform sockaddr_in to netbuf */
	tbind = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR);
	if (tbind == NULL) {
		if (t_errno == TBADF)
			errno = EBADF;
		_nderror = ND_NOMEM;
		return (-1);
	}
	tres = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR);
	if (tres == NULL) {
		(void) t_free((char *)tbind, T_BIND);
		_nderror = ND_NOMEM;
		return (-1);
	}

	tbind->qlen = 0; /* Always 0; user should change if he wants to */
	(void) memcpy(tbind->addr.buf, (char *)sin, (int)tbind->addr.maxlen);
	tbind->addr.len = tbind->addr.maxlen;
	sin = (struct sockaddr_in *)tbind->addr.buf;

        for (i = 0; i < NPORTS; i++) {                             
                sin->sin_port = htons(port++);                     
                if (port > ENDPORT)                                
                        port = STARTPORT;                          
                res = t_bind(fd, tbind, tres);                     
                if (res < 0)                                       
                        break;                                     
                else if (res == 0) {                               
                        if (memcmp(tbind->addr.buf, tres->addr.buf,
                                        (int) tres->addr.len) == 0)
                                break;                             
                        else {                                     
                                res = -1;                          
                                (void) t_unbind(fd);               
                        }                                          
                }                                                  
        }                                                          

	(void) t_free((char *)tbind, T_BIND);
	(void) t_free((char *)tres, T_BIND);
	if (i != NPORTS) {
		_nderror = ND_OK;
	} else {
		_nderror = ND_FAILCTRL;
		res = 1;
	}
	return (res);
}

static
checkresvport(addr)
	struct netbuf *addr;
{
	struct sockaddr_in *sin;

	if (addr == NULL) {
		_nderror = ND_FAILCTRL;
		return (-1);
	}
	sin = (struct sockaddr_in *)addr->buf;
	if (ntohs(sin->sin_port) < IPPORT_RESERVED)
		return (0);
	return (1);
}

static u_long
inet_addr(cp)
	register char *cp;
{
	register u_long val, base, n;
	register char c;
	u_long parts[4], *pp = parts;

again:
	/*
	 * Collect number up to ``.''.
	 * Values are specified as for C:
	 * 0x=hex, 0=octal, other=decimal.
	 */
	val = 0; base = 10;
	if (*cp == '0') {
		if (*++cp == 'x' || *cp == 'X')
			base = 16, cp++;
		else
			base = 8;
	}
	while (c = *cp) {
		if (isdigit(c)) {
			if ((c - '0') >= base)
			    break;
			val = (val * base) + (c - '0');
			cp++;
			continue;
		}
		if (base == 16 && isxdigit(c)) {
			val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
			cp++;
			continue;
		}
		break;
	}
	if (*cp == '.') {
		/*
		 * Internet format:
		 *	a.b.c.d
		 *	a.b.c	(with c treated as 16-bits)
		 *	a.b	(with b treated as 24 bits)
		 */
		if (pp >= parts + 4)
			return (-1);
		*pp++ = val, cp++;
		goto again;
	}
	/*
	 * Check for trailing characters.
	 */
	if (*cp && !isspace(*cp))
		return (-1);
	*pp++ = val;
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts;
	switch (n) {

	case 1:				/* a -- 32 bits */
		val = parts[0];
		break;

	case 2:				/* a.b -- 8.24 bits */
		val = (parts[0] << 24) | (parts[1] & 0xffffff);
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
			(parts[2] & 0xffff);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
		      ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
		break;

	default:
		return (-1);
	}
	val = htonl(val);
	return (val);
}
