/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/prot_free.c	1.3.4.4"
#ident	"$Header: $"

	/*
	 * prot_freeall.c consists of subroutines that implement the
	 * DOS-compatible file sharing services for PC-NFS
	 */

#include <stdio.h>
/*
#include <sys/file.h>
*/
#include "prot_lock.h"

extern int debug;
extern int grace_period;
extern char *xmalloc();
extern void xfree();
extern void zap_all_locks_for();
extern bool_t obj_cmp();
extern lm_vnode *fh_q;
char *malloc();

void *
proc_nlm_freeall(Rqstp, Transp)
	struct svc_req *Rqstp;
	SVCXPRT *Transp;
{
	nlm_notify	req;
/*
 * Allocate space for arguments and decode them
 */

	req.name = NULL;
	if (!svc_getargs(Transp, xdr_nlm_notify, &req)) {
		svcerr_decode(Transp);
		return;
	}

	if (debug) {
		printf("proc_nlm_freeall from %s\n",
			req.name);
	}
	destroy_client_shares(req.name);
	zap_all_locks_for(req.name);

	__free(req.name, "prot_nlm_freeall");
	svc_sendreply(Transp, xdr_void, NULL);
}

void
zap_all_locks_for(client)
	char *client;
{
	reclock *le;
	struct lm_vnode *fp;

	if (debug)
		printf("zap_all_locks_for %s\n", client);

	for (fp = fh_q; fp; fp = fp->next) {
		for (le = fp->reclox; le; le = le->next) {
			if (strcmp(le->alock.clnt_name, client) == 0) {
				if (debug)
					printf("...zapping: le@0x%x\n", le);

				delflck_reclox(&fp->reclox, le);
				le->rel = 1;
				release_reclock(le);
			}
		}
	}
	if (debug)
		printf("DONE zap_all_locks_for %s\n", client);
}
