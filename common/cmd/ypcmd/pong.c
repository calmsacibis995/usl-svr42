/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ypcmd:pong.c	1.5.9.2"
#ident  "$Header: pong.c 1.3 91/09/20 $"

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
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
#include <string.h>
#include <rpc/rpc.h>
#include <dirent.h>
#include <limits.h>
#include <netdir.h>
#include "yp_b.h"
#include "ypsym.h"
#ifndef NULL
#define NULL  0
#endif

#define BINDING "/var/yp/binding"
#define YPSERVERS "ypservers"

listofnames *names();
static int set_binding();
static bool firsttime = TRUE;

extern void free_listofnames();
extern int pipe_setdom();
extern void sysvconfig();
#ifdef __STDC__
extern void writeit();
#else
static void writeit();
#endif
extern int yp_getalias();

#define PINGTIME	10

static char *s_domain;
static struct domain *s_opaque_domain;

static listofnames *listnames;    /* private for do_broadcast and collectservers */

static bool_t
collectservers(isok, addr, netconf)
	int *isok;
	struct netbuf *addr;
	struct netconfig *netconf;
{
	struct nd_hostservlist *service;
	char *servername;
	int dofree = 0;
	int ret;

	if (! *isok) {
#ifdef DEBUG
		printf("got non-serving yp server\n");
#endif
		return(0);
	}

	if (!netdir_getbyaddr(netconf, &service, addr)) {
		servername = service->h_hostservs->h_host;
		dofree = ND_HOSTSERVLIST;
	} else {
		servername = taddr2uaddr(netconf, addr);
#ifdef DEBUG
		printf("got unknown yp server %s\n", servername);
#endif
	}

	if (listnames) {
		register listofnames *list;

		for (list = listnames; list; list = list->nextname) {
			char *name = strtok(list->name," \t\n");

			if (strcmp(servername, name) == NULL)
				break;
		 }
		 if (list == NULL) {
			ret = 0;        /* no match found */
			goto out;
		 }
	}
	ret = 1;
	(void) set_binding(netconf, addr, s_domain,
				    servername, s_opaque_domain);
out:
	if (dofree)
		netdir_free(service, ND_HOSTSERVLIST);

	return(ret);
}

/*
 * try to find a yp server by broadcasting to all nets
 */

static int
do_broadcast(servername, lin)
	char *servername;
	listofnames *lin;
{
	int isok;
	char *nettype = NULL;
	char *pname = s_domain;

	/*
	 * check for servers, if entry begins with +
	 */

	if (servername && servername[0] == '+')
		listnames = lin;
	else
		listnames = NULL;

	if (servername && servername[1] == '.')
		nettype = &servername[2];

	if (rpc_broadcast(YPPROG, YPVERS, YPPROC_DOMAIN_NONACK,
		xdr_ypdomain_wrap_string, &pname,
		xdr_int, &isok, collectservers, nettype) == RPC_SUCCESS)
		return(0);
	else
		return(-1);
}

/*
 * prefer datagram networks over vortual circuits, to avoid
 * lots of pending connections.
 */

char *nettypes[] = { "datagram_n", "circuit_n", NULL };

pong_servers(domain,opaque_domain)
char *domain;
struct domain *opaque_domain; /*to pass back*/
{
	CLIENT *clnt2;
	char *servername;
	char domain_alias[MAXNAMLEN+1];
	char outstring[YPMAXDOMAIN + 256];
	listofnames *list,*lin;
	char serverfile[MAXNAMLEN];
	struct timeval timeout;
	char *pname;
	int count=0;
	int isok, res, tried;
	enum clnt_stat clstat;

	/*
	 * We are not able to pass these names to all procedures,
	 * so save them into these static variables.
	 */

	s_domain = domain;
	s_opaque_domain = opaque_domain;

	/*
	 * get list of possible servers for this domain
	 */

	/* get alias for domain */
	sysvconfig();
	if (yp_getalias(domain, domain_alias, MAXNAMLEN) < 0)
		pname = domain;
	else
		pname = domain_alias;
	sprintf(serverfile,"%s/%s/%s",BINDING, pname, YPSERVERS);
	list=names(serverfile, count);
	lin=list;
	if (list == NULL) {
#if BROADCAST_IF_NO_BINDINGFILE
		/*
		 * if binding file cannot be found, we try broadcast
		 */
		return(do_broadcast(NULL, NULL));
#else
		fprintf(stderr, "NIS: service not installed, use ypinit -c\n");
		return(-1);
#endif
	}
	for (tried = count; list; list = list->nextname, tried--){
		servername=strtok(list->name," \t\n");
		if (servername == NULL) continue;
		if (tried == 0) {
			/*
			 * After ypbind is started up it will not be bound
			 * immediately.  This is normal, no error message
			 * is needed
			 */
			if (firsttime == TRUE) {
				firsttime = FALSE;
			} else {
				sprintf(outstring,
				    "yp: server not responding for domain %s; still trying.\n", domain);
				writeit(outstring);
			}
			tried = count;
		}
		pname=domain;
		if (servername[0] == '*' || servername[0] == '+') {
			if (do_broadcast(servername, lin) == 0) {
				free_listofnames(lin);
				return(0);
			}
		} else {
		    char **typ;

		    /*
		     * for all network types (datagram, cots)
		     */
		    for (typ = nettypes; *typ; typ++) {
			timeout.tv_sec=PINGTIME;
			timeout.tv_usec=0;

			clnt2 = clnt_create(servername, YPPROG, YPVERS, *typ);
			if (clnt2 == NULL) {
				perror(servername);
				clnt_pcreateerror("ypbind:");
				continue;
			}

			if  (clnt_call(clnt2,
			     YPPROC_DOMAIN, xdr_ypdomain_wrap_string, &pname,
			     xdr_int, &isok, timeout) == RPC_SUCCESS) {
			    if (isok) {
				struct netconfig *netconf;
				struct netbuf servaddr;

				netconf = getnetconfigent(clnt2->cl_netid);
				clnt_control(clnt2, CLGET_SVC_ADDR, &servaddr);
				res = set_binding(netconf, &servaddr, domain,
						  servername, opaque_domain);
				clnt_destroy(clnt2);
				free_listofnames(lin);
				return(res);
			    } else {
				fprintf(stderr,
				    "server %s doesn't serve domain %s\n",
				    servername, domain);
			    }
			} else {
				clnt_perror(clnt2,servername);
			}
			clnt_destroy(clnt2);
		    }
		}
	}
	free_listofnames(lin);
	return(-2);
}


/*if it pongs ok*/
static int
set_binding(setnc, setua, domain, servername, opaque_domain)
struct netconfig *setnc;
struct netbuf *setua;
char *domain;
char *servername;
struct domain *opaque_domain;
{
	ypbind_binding setb;
	ypbind_setdom setd;

	setb.ypbind_nconf= setnc;
	setb.ypbind_svcaddr= setua;
	setb.ypbind_servername=servername;
	setb.ypbind_hi_vers=0;
	setb.ypbind_lo_vers=0; /*system will figure this out*/
	setd.ypsetdom_bindinfo= & setb;
	setd.ypsetdom_domain=domain;
#ifdef DEBUG
	printf("pong: saving server settings , supports versions %d thru %d\n",
	    setb.ypbind_lo_vers, 
	    setb.ypbind_hi_vers);
	printf("\t\t nc_lookups %s proto %s protofmly %s\n",
	    *setb.ypbind_nconf->nc_lookups,
	    setb.ypbind_nconf->nc_proto, 
	    setb.ypbind_nconf->nc_protofmly);
#endif

	if (pipe_setdom(&setd, opaque_domain) < 0 ) {
#ifdef DEBUG
		printf("YPBIND pipe_setdom failed to server %s\n",
		servername);
#endif
		return(-1);
	}
#ifdef DEBUG
	else printf("YPBIND OK-set to server %s\n", servername);
#endif
	return(0);
}

static void
writeit(s)
char *s;
{
	FILE *f;

	if ((f = fopen("/dev/console", "w")) != NULL) {
		(void) fprintf(f, "%s.\n", s);
		(void) fclose(f);
	}
}
