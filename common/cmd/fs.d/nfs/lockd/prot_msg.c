/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/prot_msg.c	1.4.6.4"
#ident	"$Header: $"

	/*
	 * prot_msg.c
	 * consists all routines handle msg passing
	 */

#include <memory.h>
#include "prot_lock.h"
#include "prot_time.h"

extern int debug;
extern int grace_period;
extern int msg_len;
extern remote_result res_working;
char *xmalloc();
void xtimer();

/*
 * pointer to the last msg in msg queue
 */
msg_entry *klm_msg;

/*
 * indicator that add_reply() has already sent the reply
 * so klm_msg_routine does not need to call klm_reply()
 */
int reply_sent = 0;

/*
 * head of message queue
 */
msg_entry *msg_q;

/*
 * retransmitted search through msg_queue to determine if "a" is
 * retransmission of a previously received msg;
 * it returns the addr of the msg entry if "a" is found
 * otherwise, it returns NULL
 */
msg_entry *
retransmitted(a, proc)
	struct reclock *a;
	int proc;
{
	msg_entry *msgp;

	msgp = msg_q;
	while (msgp != NULL) {
		if (same_lock(msgp->req, &(a->lck.lox.filocks))
			|| simi_lock(msgp->req, &(a->lck.lox.filocks))) {
			/*
			 * 5 is the constant diff between rpc calls and msg
			 * passing
			 */
			if ((msgp->proc == NLM_LOCK_RECLAIM &&
			    (proc == KLM_LOCK || proc == NLM_LOCK_MSG)) ||
				msgp->proc == proc + 5 || msgp->proc == proc)
				return (msgp);
		}
		msgp = msgp->nxt;
	}

	return (NULL);
}

/*
 * match response's cookie with msg req
 * either return msgp or NULL if not found
 */
msg_entry *
search_msg(resp)
	remote_result *resp;
{
	msg_entry *msgp;
	struct reclock *req;

	msgp = msg_q;
	while (msgp != NULL) {
		req = msgp->req;
		if (obj_cmp(&req->cookie, &resp->cookie))
			return (msgp);
		msgp = msgp->nxt;
	}
	return (NULL);
}


/*
 * add a to msg queue; called from nlm_call: when rpc call is succ and reply is needed
 * proc is needed for sending back reply later
 * if case of error, NULL is returned;
 * otherwise, the msg entry is returned
 */
msg_entry *
queue(a, proc)
	struct reclock *a;
	int proc;
{
	msg_entry *msgp;

	if (debug) {
		printf("queue entered for proc = %d\n", proc);
	}

	if ((msgp = (msg_entry *) xmalloc(msg_len)) == NULL)
		return (NULL);
	(void) memset((char *) msgp, 0, msg_len);
	msgp->req = a;
	msgp->proc = proc;
	msgp->t.exp = 1;

	/* insert msg into msg queue */
	if (msg_q == NULL) {
		msgp->nxt = msgp->prev = NULL;
		msg_q = msgp;
		/* turn on alarm only when there are msgs in msg queue */
		if (grace_period == 0)
			(void) alarm(LM_TIMEOUT);
	}
	else {
		msgp->nxt = msg_q;
		msgp->prev = NULL;
		msg_q->prev = msgp;
		msg_q = msgp;
	}

	if ( proc != NLM_LOCK_RECLAIM )
		klm_msg = msgp;			/* record last msg to klm */
	return (msgp);
}

/*
 * dequeue remove msg from msg_queue;
 * and deallocate space obtained  from malloc
 * lockreq is release only if a->rel == 1;
 */
dequeue(msgp)
	msg_entry *msgp;
{
	/*
	 * First, delete msg from msg queue since dequeue(),
	 * FREELOCK() and dequeue_lock() are recursive.
	 */

	if (debug)
		printf("dequeue entered\n");

	if (msgp->prev != NULL)
		msgp->prev->nxt = msgp->nxt;
	else
		msg_q = msgp->nxt;
	if (msgp->nxt != NULL)
		msgp->nxt->prev = msgp->prev;

	if (msgp->req != NULL) {
		msgp->req->rel = 1;
		release_reclock(msgp->req);
	}
	if (msgp->reply != NULL)
		release_res(msgp->reply);

	(void) memset((char *) msgp, 0, sizeof (*msgp));
	__free((char *) msgp, "dequeue");
}

/*
 * Find a reclock and dequeue it.  But do not actually free reclock here.
 */
void
dequeue_lock(a)
	struct reclock *a;
{
	msg_entry *msgp;

	if (debug)
		printf("dequeue_lock  entered\n");

	msgp = msg_q;
	while (msgp != NULL) {
		if (a == msgp->req) {
			msgp->req = NULL;  /* don't free here; caller does it */
			dequeue(msgp);
			dequeue_lock(a); /* is there another ? */
			return;
		}
		msgp = msgp->nxt;
	}
}

add_reply(msgp, resp)
	msg_entry *msgp;
	remote_result *resp;
{
	if (debug)
		printf("enter add_reply ...\n");

	/*
	 * reset reply indicator
	 */
	reply_sent = 0;

	if ( resp != NULL) {
		if (debug) {
			if (klm_msg && klm_msg->req)
				printf("klm_msg->req=%x\n", klm_msg->req);
			if (msgp && msgp->req)
				printf("msgp->req=%x\n", msgp->req);
		}

		/*
		 * reset timer counter to record old msg
		 */
		msgp->t.curr = 0;
		msgp->reply = resp;
		if (klm_msg == msgp) {
			/*
			 * reply immediately
			 */
			klm_reply(msgp->proc, resp);
			reply_sent = 1;
			/*
			 * prevent timer routine reply "working" to already
			 * replied req
			 */
			klm_msg = NULL;
			if (resp->lstat != nlm_blocked)
				dequeue(msgp);
		}
	} else {
		/*
		 * res == NULL, used by xtimer
		 */
		if (debug)
			printf("resp == NULL  klm_msg->req=%x msgp->req=%x\n",
				klm_msg->req, msgp->req);
		if (klm_msg == msgp) {
			if (debug)
				printf("xtimer reply to (%x): ", msgp->req);
			klm_reply(msgp->proc, &res_working);
			klm_msg = NULL;
			dequeue(msgp);
		}
	}
}

/*
 * signal handler: wakes up periodically to check retransmiting
 * status
 */
void
xtimer()
{
	msg_entry *msgp, *next;
	struct reclock *sr;
	int sleeping;

	if (debug)
		printf("\nalarm! enter xtimer:\n");

	(void) signal(SIGALRM, SIG_IGN);
	if (grace_period > 0) {
		/*
		 * reduce the remaining grace period
		 */
		grace_period--;
		if (grace_period == 0) {
			if (debug) {
				printf("**********end of grace period\n");
				pr_all();
			}
			/*
			 * remove proc == klm_xxx in msg queue
			 */
			next = msg_q;
			while ((msgp = next) != NULL) {
				next = msgp->nxt;
				if (msgp->proc == KLM_LOCK ||
				    msgp->proc == KLM_UNLOCK ||
				    msgp->proc == KLM_TEST ||
				    msgp->proc == KLM_CANCEL) {
					if (debug)
						printf("remove grace period msg (%x) from msg queue\n", msgp);
					dequeue(msgp);
				}
			}
		}
	}

	next = msg_q;
	while ((msgp = next) != NULL) {
		next = msgp->nxt;
		if (msgp->reply == NULL) {
			/*
			 * have not yet recieved reply from server
			 * see if we need to call again
			 */
			if (msgp->proc != KLM_LOCK) {
				/*
				 * KLM_LOCK is for local blocked locks 
				 */
				if (msgp->t.exp == msgp->t.curr) {
					/*
					 * retransmit to server
					 */
					if (debug)
						printf("xtimer retransmit: klm_msg = %x",
							klm_msg);
					(void) nlm_call(msgp->proc, msgp->req, 1);
					msgp->t.curr = 0;
					msgp->t.exp = 2 * msgp->t.exp;
					/*
					 * double timeout period
					 */
					if (msgp->t.exp > MAX_LM_TIMEOUT_COUNT)
						msgp->t.exp = MAX_LM_TIMEOUT_COUNT;
				} else {
					msgp->t.curr++;
				}
			}
		} else {
			if (msgp->reply->lstat != nlm_blocked) {
				/*
				 * assume reply is sitting there too long
				 */
				klm_reply(msgp->proc, msgp->reply);
				dequeue(msgp);
			}
		}
	}

	(void) signal(SIGALRM, xtimer);
	if (grace_period != 0 || msg_q != NULL) {
		(void) alarm(LM_TIMEOUT);
	}
}
