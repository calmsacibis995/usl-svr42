/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:net/lockmgr/klm_lkmgr.c	1.3.2.2"
#ident	"$Header: $"

/*
 * Kernel<->Network Lock-Manager Interface
 *
 * File- and Record-locking requests are forwarded (via RPC) to a
 * Network Lock-Manager running on the local machine.  The protocol
 * for these transactions is defined in /usr/src/protocols/klm_prot.x
 */

#include <util/types.h>
#include <util/param.h>
#include <io/uio.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <proc/cred.h>
#include <net/transport/socket.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <proc/proc.h>
#include <fs/file.h>
#include <fs/stat.h>
#include <util/sysmacros.h>
#include <svc/systm.h>
#include <svc/utsname.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <net/rpc/types.h>
#include <net/tcpip/in.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/rpc.h>
#include <net/lockmgr/lockmgr.h>
#include <net/lockmgr/klm_prot.h>
#include <fs/nfs/nfs.h>
#include <fs/nfs/nfs_clnt.h>
#include <fs/nfs/rnode.h>


#define	NC_LOOPBACK		"loopback"	/* XXX */

/*
 * store knetconfig for device to avoid lookupname
 * next time
 */
static struct knetconfig	config;

STATIC int			talk_to_lockmgr();

extern int backoff_timeout;
extern int first_retry;
extern int normal_retry;
extern int normal_timeout;
extern int working_retry;
extern int working_timeout;

#ifdef DEBUG
int lockmgrlog = 0;
#endif

/*
 * klm_lockctl - process a lock/unlock/test-lock request
 *
 * Calls (via RPC) the local lock manager to register the request.
 * Lock requests are cancelled if interrupted by signals.
 */
klm_lockctl(lh, bfp, cmd, cred, clid)
	lockhandle_t *lh;
	struct flock *bfp;
	int cmd;
	struct cred *cred;
	pid_t clid;
{
	register int	error;
	char 		*args;
	klm_lockargs	klm_lockargs_args;
	klm_unlockargs  klm_unlockargs_args;
	klm_testargs    klm_testargs_args;
	klm_testrply	reply;
	u_long		xdrproc;
	xdrproc_t	xdrargs;
	xdrproc_t	xdrreply;
	int		timeid;

	LOCKMGRLOG(1, "entering klm_lockctl() : cmd %d\n", cmd);

	switch (cmd) {
	case F_SETLK:
	case F_SETLKW:
		if (bfp->l_type != F_UNLCK) {
			if (cmd == F_SETLKW)
				klm_lockargs_args.block = TRUE;
			else
				klm_lockargs_args.block = FALSE;
			if (bfp->l_type == F_WRLCK) {
				klm_lockargs_args.exclusive = TRUE;
			} else {
				klm_lockargs_args.exclusive = FALSE;
			}
			klm_lockargs_args.alock.fh.n_bytes = (char *)&lh->lh_id;
			klm_lockargs_args.alock.fh.n_len = sizeof (lh->lh_id);
			klm_lockargs_args.alock.server_name = lh->lh_servername;
			klm_lockargs_args.alock.pid = clid;
			klm_lockargs_args.alock.base = bfp->l_start;
			klm_lockargs_args.alock.length = bfp->l_len;
			klm_lockargs_args.alock.rsys = bfp->l_sysid;
			args = (char *) &klm_lockargs_args;
			xdrproc = KLM_LOCK;
			xdrargs = (xdrproc_t)xdr_klm_lockargs;
			xdrreply = (xdrproc_t)xdr_klm_stat;
		} else {
			klm_unlockargs_args.alock.fh.n_bytes = (char *)&lh->lh_id;
			klm_unlockargs_args.alock.fh.n_len = sizeof (lh->lh_id);
			klm_unlockargs_args.alock.server_name = lh->lh_servername;
			klm_unlockargs_args.alock.pid = clid;
			klm_unlockargs_args.alock.base = bfp->l_start;
			klm_unlockargs_args.alock.length = bfp->l_len;
			klm_unlockargs_args.alock.rsys = bfp->l_sysid;
			args = (char *) &klm_unlockargs_args;
			xdrreply = (xdrproc_t)xdr_klm_stat;
			xdrproc = KLM_UNLOCK;
			xdrargs = (xdrproc_t)xdr_klm_unlockargs;
		}
		break;

	case F_GETLK:
		if (bfp->l_type == F_WRLCK) {
			klm_testargs_args.exclusive = TRUE;
		} else {
			klm_testargs_args.exclusive = FALSE;
		}
		klm_testargs_args.alock.fh.n_bytes = (char *)&lh->lh_id;
		klm_testargs_args.alock.fh.n_len = sizeof (lh->lh_id);
		klm_testargs_args.alock.server_name = lh->lh_servername;
		klm_testargs_args.alock.pid = clid;
		klm_testargs_args.alock.base = bfp->l_start;
		klm_testargs_args.alock.length = bfp->l_len;
		klm_testargs_args.alock.rsys = bfp->l_sysid;
		args = (char *) &klm_testargs_args;
		xdrproc = KLM_TEST;
		xdrargs = (xdrproc_t)xdr_klm_testargs;
		xdrreply = (xdrproc_t)xdr_klm_testrply;
		break;
	}

requestloop:
	/*
	 * send the request out to the local
	 * lock-manager and wait for reply 
	 */
	error = talk_to_lockmgr(xdrproc, xdrargs, args, xdrreply, &reply, cred);
	if (error == ENOLCK || error == RPC_UDERROR) {
		error = ENOLCK;
		goto ereturn;
	}

	/*
	 * The only other possible return values are:
	 * klm_granted | klm_denied | klm_denied_nolocks | EINTR
	 */
	switch (xdrproc) {

	case KLM_LOCK:
		switch (error) {

		case klm_granted:
			error = 0;
			goto ereturn;
		case klm_denied:
			if (klm_lockargs_args.block) {
				LOCKMGRLOG(1, "blocking lock denied\n", 0);

				/*
				 * try again
				 */
				goto requestloop;
			}
			error = EACCES;
			goto ereturn;
		case klm_denied_nolocks:
			/*
			 * no resources available
			 */
			error = ENOLCK;
			goto ereturn;
		case klm_deadlck:
			/*
			 * deadlock causing lock
			 */
			error = EDEADLK;
			goto ereturn;
		case EINTR:
			/*
			 * sleep interrupted or signals
			 * were pending, cancel the lock
			 */
			goto cancel;
		}

	case KLM_UNLOCK:
		switch (error) {

		case klm_granted:
			error = 0;
			goto ereturn;
		case klm_denied:
		case EINTR:
			LOCKMGRLOG(1, "unlock denied\n", 0);
			/*
			 * may want to change something for this case
			 */
			error = EINVAL;
			goto ereturn;
		case klm_denied_nolocks:
			/*
			 * wait a while a try again
			 */
			goto nolocks_wait;
		}

	case KLM_TEST:
		switch (error) {

		case klm_granted:
			/*
			 * mark the lock available
			 */
			bfp->l_type = F_UNLCK;
			error = 0;
			LOCKMGRLOG(1, "klm_granted for test\n", 0);
			goto ereturn;
		case klm_denied:
			bfp->l_type = (reply.klm_testrply_u.holder.exclusive) ?
			    F_WRLCK : F_RDLCK;
			bfp->l_start = reply.klm_testrply_u.holder.base;
			bfp->l_len = reply.klm_testrply_u.holder.length;
			bfp->l_pid = reply.klm_testrply_u.holder.pid;
			bfp->l_sysid = reply.klm_testrply_u.holder.rsys;
			LOCKMGRLOG(1, "klm_denied for test\n", 0);
			error = 0;
			goto ereturn;
		case klm_denied_nolocks:
			/*
			 * wait a while and try again
			 */
			goto nolocks_wait;
		case EINTR:
			/*
			 * may want to take a longjmp here
			 */
			goto ereturn;
		}
	}

nolocks_wait:
	/*
	 * wait a while
	 */
	timeid = timeout(wakeup, (caddr_t)&config, (backoff_timeout * HZ));
	(void) sleep((caddr_t)&config, PZERO|PCATCH);
	untimeout(timeid);
	
	/*
	 * now try again
	 */
	goto requestloop;

cancel:
	/*
	 * If we get here, a signal interrupted a rqst that must be cancelled.
	 * Change the procedure number to KLM_CANCEL and reissue the exact same
	 * request.
	 */
	xdrproc = KLM_CANCEL;
	error = talk_to_lockmgr(xdrproc, xdrargs, args, xdrreply, &reply, cred);
	switch (error) {
	case klm_granted:
		error = 0;
		goto ereturn;
	case klm_denied:
		/*
		 * try to cancel only once
		 */
		error = EINTR;
		goto ereturn;
	case klm_deadlck:
		/*
		 * cancel causing deadlock, not possible
		 */
		error = EDEADLK;
		goto ereturn;
	case EINTR:
		/*
		 * ignire signals till cancel succeeds
		 */
		goto cancel;
	case klm_denied_nolocks:
		/*
		 * no resources
		 */
		error = ENOLCK;
		goto ereturn;
	case ENOLCK:
		LOCKMGRLOG(1, "ENOLCK on cancel request\n", 0);
		goto ereturn;
	}

ereturn:
	return (error);
}


/*
 * Send the given request to the local lock-manager.
 * If timeout or error, go back to the portmapper to check the port number.
 * This routine loops forever until one of the following occurs:
 *	1) A legitimate (not 'klm_working') reply is returned (returns 'stat').
 *
 *	2) A signal occurs (returns EINTR).  In this case, at least one try
 *	   has been made to do the RPC; this protects against jamming the
 *	   CPU if a KLM_CANCEL request has yet to go out.
 *
 *	3) A drastic error occurs (e.g., the local lock-manager has never
 *	   been activated OR cannot create a client-handle) (returns ENOLCK).
 */
STATIC int
talk_to_lockmgr(xdrproc, xdrargs, args, xdrreply, reply, cred)
	u_long xdrproc;
	xdrproc_t xdrargs;
	char *args;
	xdrproc_t xdrreply;
	klm_testrply *reply;
	struct cred *cred;
{
	struct timeval			tmo;
        struct netbuf			netaddr;
	CLIENT				*client;
	enum clnt_stat			stat;
	struct vnode			*vp;
	int				error, timeid;
	static char			keyname[SYS_NMLN+16];

	LOCKMGRLOG(1, "entered talk_to_lockmgr()\n", 0);

	strcpy(keyname, utsname.nodename);
	netaddr.len = strlen(keyname);
	strcpy(&keyname[netaddr.len], ".lockd");
	netaddr.buf = keyname;
	netaddr.len = netaddr.maxlen = netaddr.len + 6;

        /* 
	 * filch a knetconfig structure.
         */
	if (config.knc_rdev == 0){
		if ((error = lookupname("/dev/ticlts", UIO_SYSSPACE, FOLLOW,
			NULLVPP, &vp)) != 0) {
			cmn_err(CE_CONT, "klm_lkmgr: lookupname: %d\n", error);
			return (error);
		}
		config.knc_rdev = vp->v_rdev;
		config.knc_protofmly = NC_LOOPBACK;
		VN_RELE(vp);
	}

	LOCKMGRLOG(1, "calling clnt_tli_create()\n", 0);

	/*
	 * now call the proper stuff.
	 */
	if ((error = clnt_tli_kcreate(&config, &netaddr, (u_long)KLM_PROG,
		(u_long)KLM_VERS, 0, first_retry, cred, &client)) != 0) {
		cmn_err(CE_CONT, "klm_lkmgr: clnt_tli_kcreate: %d\n", error);
		return (ENOLCK);
	}
	tmo.tv_sec = working_timeout;
	tmo.tv_usec = 0;

retryloop:
	/*
	 * retry the request until completion, timeout, or error
	 */
	for (;;) {
		error = (int) CLNT_CALL(client, xdrproc,
			xdrargs, (caddr_t)args, xdrreply,
			(caddr_t)reply, tmo);

		LOCKMGRLOG(1, "clnt_call() error = %d()\n", error);

		switch (error) {

		case RPC_SUCCESS:
			error = (int)reply->stat;
			if (error == (int)klm_working) {
				/*
				 * lock manager is working or
				 * lock has blocked
				 */
				if (ISSIG(u.u_procp, 0)) {
					/*
					 * pending signals, let upper
					 * level take action
					 */
					error = EINTR;
					goto out;
				}

				/*
				 * now retry
				 */
				continue;
			}

			/*
			 * got as legitimate answer
			 */
			goto out;

		case RPC_UDERROR:
			goto out;

		case RPC_TIMEDOUT:
			continue;

		default:
			LOCKMGRLOG(1, "RPC error = %d\n", error);

			/*
			 * on RPC error, wait a bit and try again
			 */
			timeid = timeout(wakeup, (caddr_t)&config,
			    (normal_timeout * HZ));
			error = sleep((caddr_t)&config, ((PZERO+1)|PCATCH));
			untimeout(timeid);
			if (error) {
			    error = EINTR;
			    goto out;
			}
			continue;

		} 
	} 

out:
	AUTH_DESTROY(client->cl_auth);
	CLNT_DESTROY(client);
	return (error);
}

#ifdef DEBUG

/*
 * Kernel level debugging aid. The global variable "lockmgrlog" is a bit
 * mask which allows various types of debugging messages to be printed
 * out.
 * 
 *	lockmgrlog & 1 	will cause actual failures to be printed.
 */
int
lockmgr_log(level, str, a1)
	register int	level;
	register char	*str;
	register int	a1;

{
        if (level & lockmgrlog)
                printf(str, a1);
	return(0);
}
#endif
#ifdef DEBUG
#define		LOCKMGRLOG(A, B, C) ((void)((lockmgrlog) && lockmgr_log((A), (B), (C))))
#else
#define		LOCKMGRLOG(A, B, C)

#endif
