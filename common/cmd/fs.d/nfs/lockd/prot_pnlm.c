/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/prot_pnlm.c	1.10.7.4"
#ident	"$Header: $"

	/*
	 * prot_pnlm.c
	 * consists of all procedures called bu nlm_prog
	 */

#include <stdio.h>
#include <sys/fcntl.h>
#include <netinet/in.h>
#include "prot_lock.h"

#define DEBUG_ON        57
#define DEBUG_OFF       58
#define KILL_IT         59

int			nlm;
extern FILE 		*fp;
SVCXPRT 		*nlm_transp;	/* export nlm transport handle */

extern int 		grace_period, LockID;
extern int 		lock_len, res_len;
extern msg_entry        *msg_q; 
extern msg_entry 	*queue();
extern remote_result 	*get_res();
extern reclock 		*get_reclock();
 
extern remote_result 	res_nolock;
extern remote_result 	res_working;
extern remote_result 	res_grace;

extern int 		debug;

extern msg_entry 	*search_msg();
extern msg_entry 	*klm_msg;
extern remote_result 	*local_lock();
extern remote_result 	*local_unlock();
extern remote_result 	*local_test();
extern remote_result 	*local_cancel();
extern remote_result 	*cont_test();
extern remote_result 	*cont_lock();
extern remote_result 	*cont_unlock();
extern remote_result 	*cont_cancel();
extern remote_result 	*cont_reclaim();

extern nlm_testargs 	*get_nlm_testargs();
extern nlm_lockargs 	*get_nlm_lockargs();
extern nlm_unlockargs 	*get_nlm_unlockargs();
extern nlm_cancargs 	*get_nlm_cancargs();
extern nlm_testres	*get_nlm_testres();

proc_nlm_test(a)
	struct reclock *a;
{
	remote_result *result;

	if (debug)
		printf("proc_nlm_test(%x) \n", a);
	result = local_test(a);
	nlm_reply(NLM_TEST, result, a);
	a->rel = 1;
}

proc_nlm_lock(a)
	struct reclock *a;
{
	remote_result *result;


	if (debug)
		printf("enter proc_nlm_lock(%x) \n", a);
	result = local_lock(a);
	nlm_reply(NLM_LOCK, result, a);
	a->rel = 1;
}

proc_nlm_cancel(a)
	struct reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_cancel(%x) \n", a);
	result = local_cancel(a);
	nlm_reply(NLM_CANCEL, result, a);
	a->rel = 1;
}

proc_nlm_unlock(a)
	struct reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_unlock(%x) \n", a);
	result = local_unlock(a);
	nlm_reply(NLM_UNLOCK, result, a);
	a->rel = 1;
}

proc_nlm_test_msg(a)
	struct reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_test_msg(%x)\n", a);
	result = local_test(a);
	nlm_reply(NLM_TEST_MSG, result, a);
	a->rel = 1;
}

proc_nlm_lock_msg(a)
	struct reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_lock_msg(%x)\n", a);
	result = local_lock(a);
	nlm_reply(NLM_LOCK_MSG, result, a);
	a->rel = 1;
}

proc_nlm_cancel_msg(a)
	struct reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_cancel_msg(%x)\n", a);
	result = local_cancel(a);
	nlm_reply(NLM_CANCEL_MSG, result, a);
	a->rel = 1;
}

proc_nlm_unlock_msg(a)
	struct reclock *a;
{
	remote_result *result;

	if (debug)
		printf("enter proc_nlm_unlock_msg(%x)\n", a);
	result = local_unlock(a);
	nlm_reply(NLM_UNLOCK_MSG, result, a);
	a->rel = 1;
}

proc_nlm_test_res(reply)
	remote_result *reply;
{
	nlm_res_routine(reply, cont_test);
}

proc_nlm_lock_res(reply)
	remote_result *reply;
{
	nlm_res_routine(reply, cont_lock);
}

proc_nlm_cancel_res(reply)
	remote_result *reply;
{
	nlm_res_routine(reply, cont_cancel);
}

proc_nlm_unlock_res(reply)
	remote_result *reply;
{
	nlm_res_routine(reply, cont_unlock);
}

/*
 * return rpc calls;
 * if rpc calls, directly reply to the request;
 * if msg passing calls, initiates one way rpc call to reply!
 */
nlm_reply(proc, reply, a)
	int proc;
	remote_result *reply;
	struct reclock *a;
{
	bool_t (*xdr_reply)();
	int act;
	int nlmreply = 1;
	int newcall = 2;
	int rpc_err;
	char *name;
	int valid;

	switch (proc) {
	case NLM_TEST:
		xdr_reply = xdr_nlm_testres;
		act = nlmreply;
		break;
	case NLM_LOCK:
	case NLM_CANCEL:
	case NLM_UNLOCK:
		xdr_reply = xdr_nlm_res;
		act = nlmreply;
		break;
	case NLM_TEST_MSG:
		xdr_reply = xdr_nlm_testres;
		act = newcall;
		proc = NLM_TEST_RES;
		name = a->lck.clnt;
		if (a->lck.lox.lld.l_type == F_UNLCK) {
			reply->lstat = nlm_granted;
		} else {
			reply->lstat = nlm_denied;
			reply->stat.nlm_testrply_u.holder.svid = a->lck.lox.lld.l_pid;
			reply->stat.nlm_testrply_u.holder.l_offset =
				a->lck.lox.lld.l_start;
			reply->stat.nlm_testrply_u.holder.l_len =
				a->lck.lox.lld.l_len;
			if (a->lck.lox.lld.l_type == F_WRLCK)
				reply->stat.nlm_testrply_u.holder.exclusive = TRUE;
			else
				reply->stat.nlm_testrply_u.holder.exclusive = FALSE;
		}
		if (debug) {
			printf("NLM_REPLY : stat=%d svid=%d l_offset=%d l_len=%d\n",
				reply->lstat,
				reply->stat.nlm_testrply_u.holder.svid,
				reply->stat.nlm_testrply_u.holder.l_offset,
				reply->stat.nlm_testrply_u.holder.l_len);
		}
		break;
	case NLM_LOCK_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_LOCK_RES;
		name = a->lck.clnt;
		break;
	case NLM_CANCEL_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_CANCEL_RES;
		name = a->lck.clnt;
		break;
	case NLM_UNLOCK_MSG:
		xdr_reply = xdr_nlm_res;
		act = newcall;
		proc = NLM_UNLOCK_RES;
		name = a->lck.clnt;
		break;
	default:
		printf("unknown nlm_reply proc value: %d\n", proc);
		return;
	}
	if (act == nlmreply) { /* reply to nlm_transp */
		if (debug)
			printf("rpc nlm_reply %d: %d\n", proc, reply->lstat);
		if (!svc_sendreply(nlm_transp, xdr_reply, &reply))
			svcerr_systemerr(nlm_transp);
		return;
	}
	else {
		if (debug)
			printf("nlm_reply: (%s, %d), result = %d\n",
			name, proc, reply->lstat);
		/*
		 * issue a one way rpc call to reply
		 */
		reply->cookie_len = a->cookie_len;
		reply->cookie_bytes = a->cookie_bytes;
		valid = 1;

		if ((rpc_err = rpc_call(name, NLM_PROG, NLM_VERS, proc,
                        xdr_reply, reply, xdr_void, NULL, "visible", 0, 0))
                        != (int) RPC_TIMEDOUT && rpc_err != (int) RPC_CANTSEND) {
	                /*
			 * in case of error, print out error msg
			 */
                        clnt_perrno(rpc_err);
                        fprintf(stderr, "\n");
                } else if (rpc_err == RPC_CANTSEND) {
                        if ((rpc_err = rpc_call(name, NLM_PROG, NLM_VERS, proc,
                                xdr_reply, reply, xdr_void, NULL, "visible", 0, 0))
                                != (int) RPC_TIMEDOUT && rpc_err != (int) RPC_CANTSEND) {
                                /*
				 * in case of error, print out error msg
				 */
                                clnt_perrno(rpc_err);
                                fprintf(stderr, "\n");
                        }
                }
	}
}

/*
 * common routine shared by all nlm routines that expects replies from svr nlm:
 * nlm_lock_res, nlm_test_res, nlm_unlock_res, nlm_cancel_res
 * private routine "cont" is called to continue local operation;
 * reply is match with msg in msg_queue according to cookie
 * and then attached to msg_queue;
 */
nlm_res_routine(reply, cont)
	remote_result *reply;
	remote_result *(*cont)();
{
	msg_entry *msgp;
	remote_result *resp;
	struct reclock *a;
        struct lm_vnode *fp;

	if (debug) {
		printf("enter nlm_res_routine...\n");
		pr_all();
		(void) fflush(stdout);
	}

	if ((msgp = search_msg(reply)) != NULL) {
		if (debug)
			printf("nlm_res_routine(): message found\n");
		/*
		 * message found
		 */
		if (msgp->reply != NULL) {
			if (debug)
				printf("nlm_res_routine(): reply exists!\n");
			/*
			 * reply already exists
			 */
			msgp->reply->lstat = reply->lstat;
			release_res(reply);
 
			/*
			 * if blocked, remove msg from queue ao that another
			 * request from kernel can go to the server lock
			 * manager, otherwise leave it there and it
			 * will be dequeued in klm_msg_routine()
			 */
                        if (msgp->reply->lstat == nlm_blocked)
                                dequeue(msgp);
			return;
		}
		/*
		 * continue process req according to remote reply
		 */
		if (msgp->proc == NLM_LOCK_RECLAIM) {
			/*
			 * reclaim response
			 */
			resp = cont_reclaim(msgp->req, reply);
		} else {
			/*
			 * normal response
			 */
			resp = cont(msgp->req, reply);
		}

		/*
		 * only add reply if not blocked
		 * else dequeue message so that another
		 * request from the kernel can go to
		 * the server lock manager
		 */
		if (resp) {
                	if (resp->lstat != nlm_blocked)
				add_reply(msgp, resp);
			else
                        	dequeue(msgp);
		} else {
                        dequeue(msgp);
			release_res(reply);
		}
	} else {
		/*
		 * discard this reply
		 */
		release_res(reply);
	}
}

/*
 * rpc msg passing calls to nlm msg procedure;
 * used by local_lock, local_test, local_cancel and local_unloc;
 * proc specifis the name of nlm procedures;
 * retransmit indicate whether this is retransmission;
 * rpc_call return -1 if rpc call is not successful, clnt_perrno is printed out;
 * rpc_call return 0 otherwise
 */
nlm_call(proc, a, retransmit)
	int proc;
	struct reclock *a;
	int retransmit;
{
	int 		rpc_err;
	bool_t 		(*xdr_arg)();
	char 		*name, *args;
	int 		func;
	int 		valid;
	struct msg_entry *msgp;
	nlm_testargs 	*nlm_testargs_a;
	nlm_lockargs 	*nlm_lockargs_a;
	nlm_cancargs 	*nlm_cancargs_a;
	nlm_unlockargs 	*nlm_unlockargs_a;

	func = proc;		/* this is necc for NLM_LOCK_RECLAIM */
	if (retransmit == 0)
		valid = 1;	/* use cache value for first time calls */
	else
		valid = 0;	/* invalidate cache */
	switch (proc) {
	case NLM_TEST_MSG:
		xdr_arg = xdr_nlm_testargs;
		name = a->lck.svr;
		if ((nlm_testargs_a = get_nlm_testargs()) == NULL) {
			printf("rpc.lockd: unable to allocate nlm_testargs.\n");
			return (-1);
		}
		reclocktonlm_testargs(a, nlm_testargs_a);
		args = (char *)nlm_testargs_a;
		break;

	case NLM_LOCK_RECLAIM:
		func = NLM_LOCK_MSG;
		valid = 0;	/* turn off udp cache */

	case NLM_LOCK_MSG:
		xdr_arg = xdr_nlm_lockargs;
		name = a->lck.svr;
		if ((nlm_lockargs_a = get_nlm_lockargs()) == NULL) {
			printf("rpc.lockd: unable to allocate nlm_lockargs.\n");
			return (-1);
		}
		reclocktonlm_lockargs(a, nlm_lockargs_a);
		args = (char *)nlm_lockargs_a;
		break;

	case NLM_CANCEL_MSG:
		xdr_arg = xdr_nlm_cancargs;
		name = a->lck.svr;
		if ((nlm_cancargs_a = get_nlm_cancargs()) == NULL) {
			printf("rpc.lockd: unable to allocate nlm_cancargs.\n");
			return (-1);
		}
		reclocktonlm_cancargs(a, nlm_cancargs_a);
		args = (char *)nlm_cancargs_a;
		break;

	case NLM_UNLOCK_MSG:
		xdr_arg = xdr_nlm_unlockargs;
		name = a->lck.svr;
		if ((nlm_unlockargs_a = get_nlm_unlockargs()) == NULL) {
			printf("rpc.lockd: unable to allocate nlm_unlockargs.\n");
			return (-1);
		}
		reclocktonlm_unlockargs(a, nlm_unlockargs_a);
		args = (char *)nlm_unlockargs_a;
		break;

	default:
		printf("%d not supported in nlm_call\n", proc);
		return (-1);
	}

	if (debug)
		printf("nlm_call to (%s, %d) op=%d, (%d, %d); retran = %d, valid = %d\n",
			name, proc, a->lck.op, a->lck.lox.lld.l_start,
			a->lck.lox.lld.l_len, retransmit, valid);
	if (proc == NLM_LOCK_MSG || proc == NLM_UNLOCK_MSG ||
		proc == NLM_TEST_MSG || proc == NLM_CANCEL_MSG ||
		proc == NLM_LOCK_RECLAIM ) {
		/*
		 * call is a one way rpc call to simulate msg passing
		 * no timeout nor reply is specified;
		 */
		if ((rpc_err = rpc_call(name, NLM_PROG, NLM_VERS, func, xdr_arg,
			args, xdr_void, NULL, "visible", 0, 0)) == (int)RPC_TIMEDOUT ) {
			/*
			 * if rpc call is successful, add msg to msg_queue
			 */
			if (retransmit == 0) {
				/*
				 * first time calls
				 */
				if ((msgp = queue(a, proc)) == NULL) {
					return (-1);
				}
			}
			release_nlmargs(proc, args);
			return (0);
		} else {
			if (debug) {
				clnt_perrno(rpc_err);
				fprintf(stderr, "\n");
			}
			release_nlmargs(proc, args);
			return (-1);
		}
	}
}

reclocktonlm_lockargs(from, to)
	struct reclock *from;
	nlm_lockargs   *to;
{
	if ( from->cookie.n_len )
		obj_copy(&to->cookie, &from->cookie);
	if (from->block)
		to->block = TRUE;
	else
		to->block = FALSE;
	if (from->lck.lox.lld.l_type == F_WRLCK)
		to->exclusive = TRUE;
	else
		to->exclusive = FALSE;
	to->alock.caller_name = from->alock.caller_name;
	if ( from->alock.fh.n_len )
		obj_copy(&to->alock.fh, &from->alock.fh);
	else
		to->alock.fh.n_len = 0;
	if ( from->alock.oh.n_len )
		obj_copy(&to->alock.oh, &from->alock.oh);
	else
		to->alock.oh.n_len = 0;
	to->alock.svid = from->alock.lox.lld.l_pid;
	to->alock.l_offset = from->alock.lox.lld.l_start;
	to->alock.l_len = (from->alock.lox.lld.l_len == 0) ? 0x7fffffff : from->alock.lox.lld.l_len;
	to->reclaim = from->reclaim;
	to->state = from->state;
}

reclocktonlm_cancargs(from, to)
	struct reclock *from;
	nlm_cancargs   *to;
{
	if ( from->cookie.n_len )
		obj_copy(&to->cookie, &from->cookie);
	if (from->block)
		to->block = TRUE;
	else
		to->block = FALSE;
	if (from->lck.lox.lld.l_type == F_WRLCK)
		to->exclusive = TRUE;
	else
		to->exclusive = FALSE;
	to->alock.caller_name = from->alock.caller_name;
	if ( from->alock.fh.n_len )
		obj_copy(&to->alock.fh, &from->alock.fh);
	else
		to->alock.fh.n_len = 0;
	if ( from->alock.oh.n_len )
		obj_copy(&to->alock.oh, &from->alock.oh);
	else
		to->alock.oh.n_len = 0;
	to->alock.svid = from->alock.lox.lld.l_pid;
	to->alock.l_offset = from->alock.lox.lld.l_start;
	to->alock.l_len = (from->alock.lox.lld.l_len == 0) ? 0x7fffffff : from->alock.lox.lld.l_len;
}

reclocktonlm_unlockargs(from, to)
	struct reclock *from;
	nlm_unlockargs *to;
{
	if ( from->cookie.n_len )
		obj_copy(&to->cookie, &from->cookie);
	to->alock.caller_name = from->alock.caller_name;
	if ( from->alock.fh.n_len )
		obj_copy(&to->alock.fh, &from->alock.fh);
	else
		to->alock.fh.n_len = 0;
	if ( from->alock.oh.n_len )
		obj_copy(&to->alock.oh, &from->alock.oh);
	else
		to->alock.oh.n_len = 0;
	to->alock.svid = from->alock.lox.lld.l_pid;
	to->alock.l_offset = from->alock.lox.lld.l_start;
	to->alock.l_len = (from->alock.lox.lld.l_len == 0) ? 0x7fffffff : from->alock.lox.lld.l_len;
}

reclocktonlm_testargs(from, to)
	struct reclock *from;
	nlm_testargs   *to;
{
	if ( from->cookie.n_len )
		obj_copy(&to->cookie, &from->cookie);
	if (from->lck.lox.lld.l_type == F_WRLCK)
		to->exclusive = TRUE;
	else
		to->exclusive = FALSE;
	to->alock.caller_name = from->alock.caller_name;
	if ( from->alock.fh.n_len )
		obj_copy(&to->alock.fh, &from->alock.fh);
	else
		to->alock.fh.n_len = 0;
	if ( from->alock.oh.n_len )
		obj_copy(&to->alock.oh, &from->alock.oh);
	else
		to->alock.oh.n_len = 0;
	to->alock.svid = from->alock.lox.lld.l_pid;
	to->alock.l_offset = from->alock.lox.lld.l_start;
	to->alock.l_len = (from->alock.lox.lld.l_len == 0) ? 0x7fffffff : from->alock.lox.lld.l_len;
}

nlm_lockargstoreclock(from, to)
	nlm_lockargs   *from;
	struct reclock *to;
{
	if ( from->cookie.n_len )
		obj_copy(&to->cookie, &from->cookie);
	if (from->block)
		to->block = TRUE;
	else
		to->block = FALSE;
	if (from->exclusive) {
		to->exclusive = TRUE;
		to->alock.lox.lld.l_type = F_WRLCK;
	} else {
		to->exclusive = FALSE;
		to->alock.lox.lld.l_type = F_RDLCK;
	}
	to->alock.caller_name = from->alock.caller_name;
	if ( from->alock.fh.n_len )
		obj_copy(&to->alock.fh, &from->alock.fh);
	else
		to->alock.fh.n_len = 0;
	if ( from->alock.oh.n_len )
		obj_copy(&to->alock.oh, &from->alock.oh);
	else
		to->alock.oh.n_len = 0;
	to->alock.lox.lld.l_pid = from->alock.svid;
	to->alock.lox.lld.l_start = from->alock.l_offset;
	to->alock.lox.lld.l_len =
		(from->alock.l_len == 0) ? 0x7fffffff : from->alock.l_len;
	/* XXX - inet address is at second word in buf */
	to->alock.lox.lld.l_sysid =
		(nlm_transp->xp_rtaddr.len >= 8) ?
			*((long *)(nlm_transp->xp_rtaddr.buf) + 1) :
			*(long *)(nlm_transp->xp_rtaddr.buf);
	to->alock.lox.class = 0;
	if (debug)
		printf("nlm_transp->xp_rtaddr.buf=%s %x\n",(long) nlm_transp->xp_rtaddr.buf, (long) nlm_transp->xp_rtaddr.buf);
}

nlm_unlockargstoreclock(from, to)
	nlm_unlockargs *from;
	struct reclock *to;
{
	if ( from->cookie.n_len )
		obj_copy(&to->cookie, &from->cookie);
	to->alock.caller_name = from->alock.caller_name;
	if ( from->alock.fh.n_len )
		obj_copy(&to->alock.fh, &from->alock.fh);
	else
		to->alock.fh.n_len = 0;
	if ( from->alock.oh.n_len )
		obj_copy(&to->alock.oh, &from->alock.oh);
	else
		to->alock.oh.n_len = 0;
	to->alock.lox.lld.l_pid = from->alock.svid;
	to->alock.lox.lld.l_start = from->alock.l_offset;
	to->alock.lox.lld.l_len =
		(from->alock.l_len == 0) ? 0x7fffffff : from->alock.l_len;
	/* XXX - inet address is at second word in buf */
	to->alock.lox.lld.l_sysid =
		(nlm_transp->xp_rtaddr.len >= 8) ?
			*((long *)(nlm_transp->xp_rtaddr.buf) + 1) :
			*(long *)(nlm_transp->xp_rtaddr.buf);
	to->alock.lox.class = 0;
}

nlm_cancargstoreclock(from, to)
	nlm_cancargs   *from;
	struct reclock *to;
{
	if ( from->cookie.n_len )
		obj_copy(&to->cookie, &from->cookie);
	to->block = from->block;
	if (from->exclusive) {
                to->exclusive = TRUE;
                to->alock.lox.lld.l_type = F_WRLCK; 
        } else {
                to->exclusive = FALSE;
                to->alock.lox.lld.l_type = F_RDLCK; 
        }
	to->alock.caller_name = from->alock.caller_name;
	if ( from->alock.fh.n_len )
		obj_copy(&to->alock.fh, &from->alock.fh);
	else
		to->alock.fh.n_len = 0;
	if ( from->alock.oh.n_len )
		obj_copy(&to->alock.oh, &from->alock.oh);
	else
		to->alock.oh.n_len = 0;
	to->alock.lox.lld.l_pid = from->alock.svid;
	to->alock.lox.lld.l_start = from->alock.l_offset;
	to->alock.lox.lld.l_len =
		(from->alock.l_len == 0) ? 0x7fffffff : from->alock.l_len;
	/* XXX - inet address is at second word in buf */
	to->alock.lox.lld.l_sysid =
		(nlm_transp->xp_rtaddr.len >= 8) ?
			*((long *)(nlm_transp->xp_rtaddr.buf) + 1) :
			*(long *)(nlm_transp->xp_rtaddr.buf);
	to->alock.lox.class = 0;
}

nlm_testargstoreclock(from, to)
	nlm_testargs   *from;
	struct reclock *to;
{
	if ( from->cookie.n_len )
		obj_copy(&to->cookie, &from->cookie);
	if (from->exclusive) {
		to->exclusive = TRUE;
		to->alock.lox.lld.l_type = F_WRLCK;
	} else {
		to->exclusive = FALSE;
		to->alock.lox.lld.l_type = F_RDLCK;
	}
	to->alock.caller_name = from->alock.caller_name;
	if ( from->alock.fh.n_len )
		obj_copy(&to->alock.fh, &from->alock.fh);
	else
		to->alock.fh.n_len = 0;
	if ( from->alock.oh.n_len )
		obj_copy(&to->alock.oh, &from->alock.oh);
	else
		to->alock.oh.n_len = 0;
	to->alock.lox.lld.l_pid = from->alock.svid;
	to->alock.lox.lld.l_start = from->alock.l_offset;
	to->alock.lox.lld.l_len =
		(from->alock.l_len == 0) ? 0x7fffffff : from->alock.l_len;
	/* XXX - inet address is at second word in buf */
	to->alock.lox.lld.l_sysid =
		(nlm_transp->xp_rtaddr.len >= 8) ?
			*((long *)(nlm_transp->xp_rtaddr.buf) + 1) :
			*(long *)(nlm_transp->xp_rtaddr.buf);
	to->alock.lox.class = 0;
}

nlm_testrestoremote_result(from, to)
	nlm_testres   *from;
	remote_result *to;
{
	if (debug)
		printf("enter nlm_testrestoremote_result..\n");

	if ( from->cookie.n_len )
		obj_copy(&to->cookie, &from->cookie);
	to->stat.stat = from->stat.stat;
	if (from->stat.nlm_testrply_u.holder.exclusive)
		to->stat.nlm_testrply_u.holder.exclusive = TRUE;
	else
		to->stat.nlm_testrply_u.holder.exclusive = FALSE;
	to->stat.nlm_testrply_u.holder.svid =
		from->stat.nlm_testrply_u.holder.svid;
	obj_copy(&to->stat.nlm_testrply_u.holder.oh,
		&from->stat.nlm_testrply_u.holder.oh);
	to->stat.nlm_testrply_u.holder.l_offset =
		from->stat.nlm_testrply_u.holder.l_offset;
	to->stat.nlm_testrply_u.holder.l_len =
		from->stat.nlm_testrply_u.holder.l_len;
}

nlm_restoremote_result(from, to)
	nlm_res *from;
	remote_result *to;
{
	if ( from->cookie.n_len )
		obj_copy(&to->cookie, &from->cookie);
	to->stat.stat = from->stat.stat;
}

void
nlm_prog(Rqstp, Transp)
	struct svc_req *Rqstp;
	SVCXPRT *Transp;
{
	bool_t 			(*xdr_Argument)(), (*xdr_Result)();
	char 			*(*Local)();
	extern nlm_lockargs	*get_nlm_lockargs();
	extern nlm_unlockargs	*get_nlm_unlockargs();
	extern nlm_testargs	*get_nlm_testargs();
	extern nlm_cancargs	*get_nlm_cancargs();
	extern nlm_testres	*get_nlm_testres();
	extern nlm_res		*get_nlm_res();
	int			monitor_this_lock = 1;
	char 			*nlm_req = NULL, *nlm_rep = NULL;
	nlm_lockargs 		*nlm_lockargs_a;
	nlm_unlockargs 		*nlm_unlockargs_a;
	nlm_testargs 		*nlm_testargs_a;
	nlm_cancargs 		*nlm_cancargs_a;
	nlm_testres		*nlm_testres_a;
	nlm_res			*nlm_res_a;
	struct reclock  	*req;
	remote_result 		*reply;
	int 			oldmask;

	if (debug) {
		printf("\nNLM_PROG+++ version %d proc %d\n",
			Rqstp->rq_vers, Rqstp->rq_proc);
		pr_all();
	}

	oldmask = sigblock (1 << (SIGALRM -1));
	/*
	 * export the transport handle
	 */
	nlm_transp = Transp;
	switch (Rqstp->rq_proc) {
	case NULLPROC:
		svc_sendreply(Transp, xdr_void, NULL);
		(void) sigsetmask(oldmask);
		return;

	case DEBUG_ON:
		debug = 2;
		svc_sendreply(Transp, xdr_void, NULL);
		(void) sigsetmask(oldmask);
		return;

	case DEBUG_OFF:
		debug = 0;
		svc_sendreply(Transp, xdr_void, NULL);
		(void) sigsetmask(oldmask);
		return;

	case NLM_TEST:
		xdr_Argument = xdr_nlm_testargs;
		xdr_Result = xdr_nlm_testres;
		Local = (char *(*)()) proc_nlm_test;
		if ((nlm_testargs_a = get_nlm_testargs()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_req.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_req = (char *) nlm_testargs_a;
		break;

	case NLM_LOCK:
		xdr_Argument = xdr_nlm_lockargs;
		xdr_Result = xdr_nlm_res;
		Local = (char *(*)()) proc_nlm_lock;
		if ((nlm_lockargs_a = get_nlm_lockargs()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_req.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_req = (char *) nlm_lockargs_a;
		break;

	case NLM_CANCEL:
		xdr_Argument = xdr_nlm_cancargs;
		xdr_Result = xdr_nlm_res;
		Local = (char *(*)()) proc_nlm_cancel;
		if ((nlm_cancargs_a = get_nlm_cancargs()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_req.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_req = (char *) nlm_cancargs_a;
		break;

	case NLM_UNLOCK:
		xdr_Argument = xdr_nlm_unlockargs;
		xdr_Result = xdr_nlm_res;
		Local = (char *(*)()) proc_nlm_unlock;
		if ((nlm_unlockargs_a = get_nlm_unlockargs()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_req.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_req = (char *) nlm_unlockargs_a;
		break;

	case NLM_TEST_MSG:
		xdr_Argument = xdr_nlm_testargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_test_msg;
		if ((nlm_testargs_a = get_nlm_testargs()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_req.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_req = (char *) nlm_testargs_a;
		break;

	case NLM_LOCK_RECLAIM:
	case NLM_LOCK_MSG:
		xdr_Argument = xdr_nlm_lockargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_lock_msg;
		if ((nlm_lockargs_a = get_nlm_lockargs()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_req.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_req = (char *) nlm_lockargs_a;
		break;

	case NLM_CANCEL_MSG:
		xdr_Argument = xdr_nlm_cancargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_cancel_msg;
		if ((nlm_cancargs_a = get_nlm_cancargs()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_req.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_req = (char *) nlm_cancargs_a;
		break;

	case NLM_UNLOCK_MSG:
		xdr_Argument = xdr_nlm_unlockargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_unlock_msg;
		if ((nlm_unlockargs_a = get_nlm_unlockargs()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_req.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_req = (char *) nlm_unlockargs_a;
		break;

	case NLM_TEST_RES:
		xdr_Argument = xdr_nlm_testres;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_test_res;
		if ((nlm_testres_a = get_nlm_testres()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_res.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_rep = (char *) nlm_testres_a;
		break;

	case NLM_LOCK_RES:
		xdr_Argument = xdr_nlm_res;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_lock_res;
		if ((nlm_res_a = get_nlm_res()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_res.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_rep = (char *) nlm_res_a;
		break;

	case NLM_CANCEL_RES:
		xdr_Argument = xdr_nlm_res;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_cancel_res;
		if ((nlm_res_a = get_nlm_res()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_res.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_rep = (char *) nlm_res_a;
		break;

	case NLM_UNLOCK_RES:
		xdr_Argument = xdr_nlm_res;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_unlock_res;
		if ((nlm_res_a = get_nlm_res()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_res.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_rep = (char *) nlm_res_a;
		break;

	case NLM_SHARE:
	case NLM_UNSHARE:
		if (Rqstp->rq_vers != NLM_VERSX)  {
			svcerr_noproc(Transp);
			(void) sigsetmask(oldmask);
			return;
		}
		proc_nlm_share(Rqstp, Transp);
		(void) sigsetmask(oldmask);
		return;

	case NLM_NM_LOCK:
		if (Rqstp->rq_vers != NLM_VERSX)  {
			svcerr_noproc(Transp);
			(void) sigsetmask(oldmask);
			return;
		}
		Rqstp->rq_proc = NLM_LOCK; /* fake it */
		monitor_this_lock = 0;
		xdr_Argument = xdr_nlm_lockargs;
		xdr_Result = xdr_void;
		Local = (char *(*)()) proc_nlm_lock;
		if ((nlm_lockargs_a = get_nlm_lockargs()) == NULL) {
			printf("rpc.lockd: unable to allocate space for nlm_req.\n");
			(void) sigsetmask(oldmask);
			return;
		}
		nlm_req = (char *) nlm_lockargs_a;
		break;

	case NLM_FREE_ALL:
		if (Rqstp->rq_vers != NLM_VERSX)  {
			svcerr_noproc(Transp);
			(void) sigsetmask(oldmask);
			return;
		}
		proc_nlm_freeall(Rqstp, Transp);
		(void) sigsetmask(oldmask);
		return;

	default:
		svcerr_noproc(Transp);
		(void) sigsetmask(oldmask);
		return;
	}

	if ( Rqstp->rq_proc != NLM_LOCK_RES &&
		Rqstp->rq_proc != NLM_CANCEL_RES &&
		Rqstp->rq_proc != NLM_UNLOCK_RES &&
		Rqstp->rq_proc != NLM_TEST_RES ) {
		/* lock request */
		if ((req = get_reclock()) != NULL) {
			if (!svc_getargs(Transp, xdr_Argument, nlm_req)) {
				svcerr_decode(Transp);
				release_nlmargs(Rqstp->rq_proc, nlm_req);
				(void) sigsetmask(oldmask);
				return;
			}
			switch (Rqstp->rq_proc) {
			case NLM_TEST:
			case NLM_TEST_MSG:
				nlm_testargstoreclock((struct nlm_testargs *)nlm_req, req);
				break;
			case NLM_LOCK:
			case NLM_LOCK_MSG:
			case NLM_LOCK_RECLAIM:
			case NLM_NM_LOCK:
				nlm_lockargstoreclock((struct nlm_lockargs *)nlm_req, req);
				break;
			case NLM_CANCEL:
			case NLM_CANCEL_MSG:
				nlm_cancargstoreclock((struct nlm_cancargs *)nlm_req, req);
				break;
			case NLM_UNLOCK:
			case NLM_UNLOCK_MSG:
				nlm_unlockargstoreclock((struct nlm_unlockargs *)nlm_req, req);
				break;
			}
			if (debug == 3) {
				if (fwrite(&nlm, sizeof (int), 1, fp) == 0)
					fprintf(stderr, "fwrite nlm error\n");
				if (fwrite(&Rqstp->rq_proc,
					sizeof (int), 1, fp) == 0)
					fprintf(stderr,
						"fwrite nlm_proc error\n");
				printf("range[%d, %d] \n", req->lck.lox.lld.l_start,
					req->lck.lox.lld.l_len);
				(void) fflush(fp);
			}

			if (map_klm_nlm(req) == -1)
				goto abnormal;

			if (grace_period >0 && !(req->reclaim)) {
				if (debug)
					printf("during grace period, please retry later\n");
				nlm_reply(Rqstp->rq_proc, &res_grace, req);
				req->rel = 1;
				release_reclock(req);
				release_nlmargs(Rqstp->rq_proc, nlm_req);
				(void) sigsetmask(oldmask);
				return;
			}
			if (grace_period >0 && debug)
				printf("accept reclaim request(%x)\n", req);
			if (monitor_this_lock &&
				(Rqstp->rq_proc == NLM_LOCK ||
				 Rqstp->rq_proc == NLM_LOCK_MSG ||
				 Rqstp->rq_proc == NLM_LOCK_RECLAIM))
				if (add_mon(req, 1) == -1) {
					req->rel = 1;
					release_reclock(req);
					release_nlmargs(Rqstp->rq_proc, nlm_req);

					fprintf(stderr,
						"req discard due status monitor problem\n");
					(void) sigsetmask(oldmask);
					return;
				}
			(*Local)(req);

			release_reclock(req);
			release_nlmargs(Rqstp->rq_proc, nlm_req);
		} else {
			/*
			 * malloc error, return no lock
			 */
			nlm_reply((int) Rqstp->rq_proc, &res_nolock, req);
			release_nlmargs(Rqstp->rq_proc, nlm_req);
		}
	} else {
		/*
		 * msg reply
		 */
		if ((reply = get_res()) != NULL) {
			if (!svc_getargs(Transp, xdr_Argument, nlm_rep)) {
				svcerr_decode(Transp);
				release_nlmargs(Rqstp->rq_proc, nlm_rep);
				(void) sigsetmask(oldmask);
				return;
			}
			switch (Rqstp->rq_proc) {
			case NLM_TEST_RES:
				nlm_testrestoremote_result(nlm_rep, reply);
				break;
			case NLM_LOCK_RES:
			case NLM_CANCEL_RES:
			case NLM_UNLOCK_RES:
				nlm_restoremote_result(nlm_rep, reply);
				break;
			}
			if (debug == 3) {
				if (fwrite(&nlm, sizeof (int), 1, fp) == 0)
					fprintf(stderr,
						"fwrite nlm_reply error\n");
				if (fwrite(&Rqstp->rq_proc, sizeof (int),
					1, fp) == 0)
					fprintf(stderr,
						"fwrite nlm_reply_proc error \n");
				(void) fflush(fp);
			}
			if (debug)
				printf("msg reply(%d) to procedure(%d)\n",
					reply->lstat, Rqstp->rq_proc);
			(*Local)(reply);
		} else {
			/* malloc failure, do nothing */
			release_nlmargs(Rqstp->rq_proc, nlm_rep);
		}
	}
	(void) sigsetmask(oldmask);
	if (debug)
		printf("EXITING FROM NLM_PROG...\n\n");
	return;

abnormal:
	/*
	 * malloc error, release allocated space and error return
	 */
	nlm_reply((int) Rqstp->rq_proc, &res_nolock, req);
	req->rel = 1;
	release_reclock(req);
	if (nlm_req) {
		release_nlmargs(Rqstp->rq_proc, nlm_req);
	} else if (nlm_rep) {
		release_nlmargs(Rqstp->rq_proc, nlm_rep);
	}
	(void) sigsetmask(oldmask);
	return;
}

release_nlmargs(proc, req)
int proc;
caddr_t req;
{
	switch (proc) {
	case NLM_TEST:
	case NLM_TEST_MSG:
		release_nlm_testargs(req);
		break;
	case NLM_LOCK:
	case NLM_LOCK_MSG:
	case NLM_LOCK_RECLAIM:
	case NLM_NM_LOCK:
		release_nlm_lockargs(req);
		break;
	case NLM_CANCEL:
	case NLM_CANCEL_MSG:
		release_nlm_cancargs(req);
		break;
	case NLM_UNLOCK:
	case NLM_UNLOCK_MSG:
		release_nlm_unlockargs(req);
		break;
	case NLM_LOCK_RES:
	case NLM_CANCEL_RES:
	case NLM_UNLOCK_RES:
		release_nlm_res(req);
		break;
	case NLM_TEST_RES:
		release_nlm_testres(req);
		break;
	}
}
