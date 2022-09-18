/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/rpc.c	1.3.6.3"
#ident	"$Header: $"

/*
 * this file consists of routines to support call_rpc();
 * client handles are cached in a hash table;
 * clntudp_create is only called if (site, prog#, vers#) cannot
 * be found in the hash table;
 * a cached entry is destroyed, when remote site crashes
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <string.h>
#include <sys/param.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

extern char *calloc();
extern char *malloc();
extern int t_errno, errno;

static struct rpc_call_private {
	int	valid;			/* Is this entry valid ? */
	CLIENT	*client;		/* Client handle */
	u_long	prognum, versnum;	/* Program, version */
	char	host[MAXHOSTNAMELEN];	/* Servers host */
	char	nettype[32];		/* Network type */
} *rpc_call_private;


#define MAX_HASHSIZE 100

char *malloc();
char *xmalloc();
extern int debug;
extern int HASH_SIZE;
extern void nlm_prog(), klm_prog(), priv_prog();

struct cache {
	char *host;
	int prognum;
	int versnum;
	int sock;
	CLIENT *client;
	struct cache *nxt;
};

struct cache *table[MAX_HASHSIZE];
int cache_len = sizeof (struct cache);

hash(name)
	char *name;
{
	int len;
	int i, c;

	c = 0;
	len = strlen(name);
	for (i = 0; i< len; i++) {
		c = c +(int) name[i];
	}
	c = c %HASH_SIZE;
	return (c);
}

void
delete_hash(host)
	char *host;
{
	struct cache *cp;
	struct cache *cp_prev = (struct cache *)NULL;
	struct cache *next;
	int h;

	if (debug)
		printf("enter delete_hash() ...\n");

	/*
	 * if there is more than one entry with same host name;
	 * delete has to be recurrsively called
	 */

	h = hash(host);
	next = table[h];
	while ((cp = next) != (struct cache *)NULL) {
		next = cp->nxt;
		if (strcmp(cp->host, host) == 0) {
			if (cp_prev == (struct cache *)NULL) {
				table[h] = cp->nxt;
			}
			else {
				cp_prev->nxt = cp->nxt;
			}
			if (debug)
				printf("delete hash entry (%x), %s \n", cp, host);
			if (cp->client)
				clnt_destroy(cp->client);
			if (cp->host != NULL) free(cp->host);
			if (cp != NULL) free(cp);
		}
		else {
			cp_prev = cp;
		}
	}
}


/*
 * This is the simplified interface to the client rpc layer.
 * The client handle is not destroyed here and is reused for
 * the future calls to same prog, vers, host and nettype combination.
 *
 * The total time available is 25 seconds.
 */
enum clnt_stat
rpc_call(host, prognum, versnum, procnum, inproc, in, outproc, out, nettype,
	 sec, usec)
	char *host;			/* host name */
	u_long prognum;			/* program number */
	u_long versnum;			/* version number */
	u_long procnum;			/* procedure number */
	xdrproc_t inproc, outproc;	/* in/out XDR procedures */
	char *in, *out;			/* recv/send data */
	char *nettype;			/* nettype */
	int sec;
	int usec;
{
	register struct rpc_call_private *rcp = rpc_call_private;
	enum clnt_stat clnt_stat;
	struct timeval tottimeout;

	if (rcp == (struct rpc_call_private *)NULL) {
		rcp = (struct rpc_call_private *)calloc(1, sizeof (*rcp));
		if (rcp == (struct rpc_call_private *)NULL) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			rpc_createerr.cf_error.re_errno = errno;
			return (rpc_createerr.cf_stat);
		}
		rpc_call_private = rcp;
	}
	if ((nettype == NULL) || (nettype[0] == NULL))
		nettype = "netpath";
	if (!(rcp->valid && rcp->prognum == prognum
		&& rcp->versnum == versnum
		&& (!strcmp(rcp->host, host))
		&& (!strcmp(rcp->nettype, nettype)))) {
		int fd;
		struct t_info tinfo;

		rcp->valid = 0;
		if (rcp->client)
			CLNT_DESTROY(rcp->client);
		/*
		 * Using the first successful transport for that type
		 */
		rcp->client = clnt_create(host, prognum, versnum, nettype);
		if (rcp->client == (CLIENT *)NULL)
			return (rpc_createerr.cf_stat);
		(void) CLNT_CONTROL(rcp->client, CLGET_FD, &fd);
		if (t_getinfo(fd, &tinfo) != -1) {
			if (tinfo.servtype == T_CLTS) {
				struct timeval timeout;

				/*
				 * Set time outs for connectionless case
				 */
				timeout.tv_usec = 0;
				timeout.tv_sec = 5;
				(void) CLNT_CONTROL(rcp->client,
					CLSET_RETRY_TIMEOUT, &timeout);
			}
		} else {
			rpc_createerr.cf_stat = RPC_TLIERROR;
			rpc_createerr.cf_error.re_terrno = t_errno;
			return (rpc_createerr.cf_stat);
		}
		rcp->prognum = prognum;
		rcp->versnum = versnum;
		(void) strcpy(rcp->host, host);
		(void) strcpy(rcp->nettype, nettype);
		rcp->valid = 1;
	}

	tottimeout.tv_sec = sec;
	tottimeout.tv_usec = usec;
	clnt_stat = CLNT_CALL(rcp->client, procnum, inproc, in, outproc,
				out, tottimeout);
	/* 
	 * if call failed, empty cache
	 */
	if (clnt_stat != RPC_SUCCESS)
		rcp->valid = 0;
	return (clnt_stat);
}
