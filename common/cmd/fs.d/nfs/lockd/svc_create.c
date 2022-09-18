/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/svc_create.c	1.1.8.4"
#ident	"$Header: $"

#include <stdio.h>
#include <rpc/rpc.h>
#include <errno.h>
#ifdef SYSLOG
#include <sys/syslog.h>
#else
#define LOG_ERR 3
#endif				/* SYSLOG */
#include <rpc/nettype.h>
#include <netconfig.h>
#include <netdir.h>

extern int errno;
extern int t_errno;
extern char *t_errlist[];

extern char *strdup();

/*
 * The highest level interface for server creation.
 * It tries for all the nettokens in that particular class of token
 * and returns the number of handles it can create and/or find.
 *
 * It creates a link list of all the handles it could create.
 * If svc_create_local_service() is called multiple times, it uses the handle
 * created earlier instead of creating a new handle every time.
 */
int
svc_create_local_service(dispatch, prognum, versnum, nettype, servname)
	void (*dispatch) ();	/* Dispatch function */
	u_long prognum;			/* Program number */
	u_long versnum;			/* Version number */
	char *nettype;			/* Networktype token */
	char *servname;
{
	struct xlist {
		SVCXPRT *xprt;	/* Server handle */
		struct xlist *next;	/* Next item */
	}    *l;
	static struct xlist *xprtlist;	/* A link list of all the handles */
	int num = 0;
	SVCXPRT *xprt;
	struct netconfig *nconf;
	struct t_bind *bind_addr;
	int fd;
	struct nd_hostserv ns;
	struct nd_addrlist *nas;
	struct t_info tinfo;
	void *net;


	if ((net = _rpc_setconf(nettype)) == NULL) {
		(void) syslog(LOG_ERR, "svc_create_local_service: unknown protocol");
		return (0);
	}
	while (nconf = _rpc_getconf(net)) {
		if (strcmp(nconf->nc_protofmly, NC_LOOPBACK)
#ifndef LOCAL_ONLY
		    && strcmp(nconf->nc_protofmly, NC_INET)
#endif
		    )
			continue;

		for (l = xprtlist; l; l = l->next) {
			if (strcmp(l->xprt->xp_netid, nconf->nc_netid) == 0) {
				/* Found an  old  one,  use  it
				*/
				(void) rpcb_unset(prognum, versnum, nconf);
				if (svc_reg(l->xprt, prognum, versnum,
					dispatch, nconf) == FALSE)
					(void) syslog(LOG_ERR,
					    "svc_create_local_service: could not register prog %d vers %d on %s",
					    prognum, versnum, nconf->nc_netid);
				else
					num++;
				break;
			}
		}
		/* It was not found.
		 * Now create a new one */
		if (l == (struct xlist *) NULL) {
			if ((fd = t_open(nconf->nc_device, O_RDWR, &tinfo))
			    < 0) {
				syslog(LOG_ERR,
				    "svc_create_local_service: %s: cannot open connection: %s",
				    nconf->nc_netid, t_errlist[t_errno]);
				continue;
			}
			bind_addr = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR);
			if ((bind_addr == NULL)) {
				(void) syslog(LOG_ERR, "svc_create_local_service: t_alloc failed\n");
				continue;
			}
			ns.h_host = HOST_SELF;
			ns.h_serv = servname;
			if (!netdir_getbyname(nconf, &ns, &nas)) {
				/* Copy the address */
				bind_addr->addr.len =
				    nas->n_addrs->len;
				memcpy(bind_addr->addr.buf,
				    nas->n_addrs->buf, (int) nas->n_addrs->len);
				netdir_free((char *) nas, ND_ADDRLIST);
			} else {
				/*
				 * We need a well-known address for ticlts for
				 * the kernel, but not for any other transport.
				 */
				if (!strcmp(nconf->nc_netid, "ticlts")) {
					syslog(LOG_ERR,
					    "NFS lockd:: no well known address on transport %s\n",
					    nconf->nc_netid);
					fprintf(stderr,
					    "NFS lockd:: warning: no well known address on transport %s\n",
					    nconf->nc_netid);
				}
				(void) t_free(bind_addr, T_BIND);
				bind_addr = NULL;
			}


			xprt = svc_tli_create(fd, nconf, bind_addr, 0, 0);
			if (bind_addr)
				(void) t_free(bind_addr), T_BIND;
			if (xprt) {

				/* Made  a  new  one,  use  it */
				(void) rpcb_unset(prognum, versnum, nconf);
				if (svc_reg(xprt, prognum, versnum,
					dispatch, nconf) == FALSE)
					(void) syslog(LOG_ERR,
					    "svc_create_local_service: could not register prog %d vers %d on %s",
					    prognum, versnum, nconf->nc_netid);
				l = (struct xlist *) malloc(sizeof(struct xlist));
				if (l == (struct xlist *) NULL) {
					(void) syslog(LOG_ERR,
					    "svc_create_local_service: no memory");
					return (0);
				}
				l->xprt = xprt;
				l->next = xprtlist;
				xprtlist = l;
				num++;
				if (strcmp(nconf->nc_protofmly, NC_LOOPBACK)
				    == 0)
					_rpc_negotiate_uid(xprt->xp_fd);
			}
		}
	}
	_rpc_endconf(net);
	/* In case of num == 0; the
	 * error messages are generated
	 * by the underlying layers;
	 * and hence not needed here. */
	return (num);
}



