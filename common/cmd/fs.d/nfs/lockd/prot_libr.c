/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/prot_libr.c	1.6.5.3"
#ident	"$Header: $"

/*
 *	prot_libr.c, consists of routines used for initialization,
 *	mapping and debugging
 */

#include <stdio.h>
#include <memory.h>
#include <netdb.h>
#include "prot_lock.h"
#include "prot_time.h"

/*
 * used for generating oh 
 */
char hostname[MAXNAMELEN];
/*
 * id for status monitor usage
 */
int pid;
/*
 * used for generating oh 
 */
int host_len;
int lock_len;
int res_len;
int msg_len;
int grace_period;
remote_result res_nolock;
remote_result res_working;
remote_result res_grace;

/*
 * monitonically increasing number
 */
int cookie;

extern lm_vnode *fh_q;
extern struct reclock *sleep_q;
extern msg_entry *msg_q;
extern int debug, used_reclock;

char *xmalloc();

/*
 * lock manager init
 */
init()
{
	int i;

	(void) gethostname(hostname, MAXNAMELEN);

	/*
	 * used to generate owner handle
	 */
	host_len = strlen(hostname) +1;
	msg_len = sizeof (msg_entry);
	lock_len = sizeof (struct reclock);
	res_len = sizeof (remote_result);

	/*
	 * used to generate return id for status monitor
	 */
	pid = getpid();
	res_nolock.lstat = nlm_denied_nolocks;
	res_working.lstat = nlm_blocked;
	res_grace.lstat = nlm_denied_grace_period;
	grace_period = LM_GRACE;
	cancel_mon();
}

/*
 * map input (from kenel) to lock manager internal structure
 * returns -1 if cannot allocate memory;
 * returns 0 otherwise
 */
int
map_kernel_klm(a)
	reclock *a;
{
	/*
	 * common code shared between map_kernel_klm and map_klm_nlm
	 * generate op
	 */
	if (a->lck.lox.lld.l_type == F_WRLCK) {
		a->lck.op = LOCK_EX;
	} else if (a->lck.lox.lld.l_type == F_RDLCK) {
		a->lck.op = LOCK_SH;
	}
	if (a->block == FALSE)
		a->lck.op = a->lck.op | LOCK_NB;
	if (a->lck.lox.lld.l_len > MAXLEN) {
		fprintf(stderr, " len(%d) greater than max len(%d)\n",
			a->lck.lox.lld.l_len, MAXLEN);
		a->lck.lox.lld.l_len = MAXLEN;
	}

	/*
	 * generate svid holder
	 */
	if (!a->lck.lox.lld.l_pid)
		a->lck.lox.lld.l_pid = getpid();
	a->lck.svid = a->lck.lox.lld.l_pid;

	/*
	 * owner handle == (hostname, pid);
	 * cannot generate owner handle use obj_alloc
	 * because additioanl pid attached at the end
	 */
	a->lck.oh_len = host_len + sizeof (int);
	if ((a->lck.oh_bytes = xmalloc(a->lck.oh_len) ) == NULL)
		return (-1);
	(void) strcpy(a->lck.oh_bytes, hostname);
	memcpy(&a->lck.oh_bytes[host_len], (char *) &a->lck.lox.lld.l_pid,
		sizeof (int));

	/*
	 * generate cookie
	 * cookie is generated from monitonically increasing #
	 */
	cookie++;
	if (obj_alloc(&a->cookie, (char *) &cookie, sizeof (int))== -1)
		return (-1);

	/*
	 * generate clnt_name
	 */
	if ((a->lck.clnt= xmalloc(host_len)) == NULL)
		return (-1);
	(void) strcpy(a->lck.clnt, hostname);
	a->lck.caller_name = a->lck.clnt; 	/* ptr to same area */
	return (0);
}


/*
 * nlm map input from klm to lock manager internal structure
 * return -1, if cannot allocate memory!
 * returns 0, otherwise
 */
int
map_klm_nlm(a)
	reclock *a;
{
	/*
	 * common code shared between map_kernel_klm and map_klm_nlm
	 * generate op
	 */
	if (a->lck.lox.lld.l_type == F_WRLCK) {
		a->lck.op = LOCK_EX;
	} else if (a->lck.lox.lld.l_type == F_RDLCK) {
		a->lck.op = LOCK_SH;
	}
	if (a->block == FALSE)
		a->lck.op = a->lck.op | LOCK_NB;

	/*
	 * generate svid holder
	 */
	if (!a->lck.lox.lld.l_pid)
		a->lck.lox.lld.l_pid = getpid();
	a->lck.svid = a->lck.lox.lld.l_pid;

	a->lck.l_offset = a->lck.lox.lld.l_start;
	a->lck.l_len = a->lck.lox.lld.l_len;

 	/*
	 * normal klm to nlm calls
	 */
	if ((a->lck.svr = xmalloc(host_len)) == NULL) {
		return (-1);
	}
	(void) strcpy(a->lck.svr, hostname);
	a->lck.clnt = a->lck.caller_name;
	return (0);
}

/*
 * print net object
 */
pr_oh(a)
	netobj *a;
{
	int i;
	int j;
	unsigned p = 0;

	if (a->n_len - sizeof (int) > 4 )
		j = 4;
	else
		j = a->n_len - sizeof (int);

	/*
	 * only print out part of oh
	 */
	for (i = 0; i< j; i++) {
		printf("%c", a->n_bytes[i]);
	}
	for (i = a->n_len - sizeof (int); i< a->n_len; i++) {
		p = (p << 8) | (((unsigned)a->n_bytes[i]) & 0xff);
	}
	printf("%u", p);
}

/*
 * print file handle
 */
pr_fh(a)
	netobj *a;
{
	int i;

	for (i = 0; i< a->n_len; i++) {
		printf("%02x", (a->n_bytes[i] & 0xff));
	}
}


/*
 * print a lock
 */
pr_lock(a)
	reclock *a;
{
	if (a != NULL) {
		printf("(%x), oh= ", a);
		pr_oh(&a->lck.oh);
		if (a->lck.svr)
			printf(", svr= %s, fh = ", a->lck.svr);
		else
			printf(", svr= NULL, fh = ");
		pr_fh(&a->lck.fh);
		if (a->block)
			printf(" block=TRUE ");
		else
			printf(" block=FALSE ");
		if (a->exclusive)
			printf(" exclusive=TRUE ");
		else
			printf(" exclusive=FALSE ");
		printf(" rel=%d w_flag=%d type=%d pid=%d class=%d granted=%d rsys=%x LockID=%d ",
			a->rel, a->w_flag, a->lck.lox.lld.l_type,
			a->lck.lox.lld.l_pid, a->lck.lox.class, a->lck.lox.granted,
			a->lck.lox.lld.l_sysid, a->lck.lox.LockID);
		printf(", op=%d, ranges= [%d, %d)\n",
 			a->lck.op,
			a->lck.lox.lld.l_start, a->lck.lox.lld.l_start + a->lck.lox.lld.l_len);
	} else {
		printf("pr_lock(): RECLOCK is NULL.\n");
	}
}


pr_all()
{
	struct reclock *tf;
	struct lm_vnode *fl;
	struct filock *ff;
	msg_entry *msgp;
	int i, ii;

	if (debug < 2)
		return;

	/*
	 * print msg queue
	 */
	if (msg_q != NULL) {
		printf("***** MSG QUEUE *****\n");
		msgp= msg_q;
		while (msgp != NULL) {
			printf("(%x) : ", msgp->req);
			printf(" (%x, ", msgp->req);
			if (msgp->reply != NULL)
				printf(" lstat =%d, proc =%d), ",
					msgp->reply->lstat, msgp->proc);
			else
				printf(" NULL), ");
			msgp = msgp->nxt;
		}
		printf("\n");
	}
	else
		printf("***** NO MSG IN MSG QUEUE *****\n");

	/*
	 * print fh_q
	 */
	if (fh_q != NULL) {
		printf("\n***** FILE HANDLE LIST *****");
		for (fl = fh_q; fl; fl = fl->next) {
			if (fl->reclox != NULL) {
				printf("\n***** RECLOX LOCKS : %x *****\n", fl);
				for (tf = fl->reclox; tf; tf = tf->next) {
					pr_lock(tf);
				}
			} else
				printf("\n***** NO RECLOX LOCKS : %x\n****",
					fl);
		}
	}
	else
		printf("\n***** NO FILE HANDLE STRUCT ****\n");

	/*
         * print sleep_q
         */
        if (sleep_q != NULL) {
                printf("\n***** SLEEP LOCKS LIST *****\n");
                for (tf = sleep_q; tf; tf = tf->next) {
                        pr_lock(tf);
                }
        } else
                printf("\n***** NO SLEEP LOCKS: %x****\n", sleep_q);

	printf("used_reclock=%d\n", used_reclock);
	(void) fflush(stdout);
}

up(x)
	int x;
{
	return ((x % 2 == 1) || (x %2 == -1));
}

kill_process(a)
	reclock *a;
{
	fprintf(stderr, "kill process (%d)\n", a->lck.lox.lld.l_pid);
	(void) kill(a->lck.lox.lld.l_pid, SIGLOST);
}

pr_sleepq()
{
	struct reclock *tf;

        if (sleep_q != NULL) {
                printf("\n***** SLEEP LOCKS LIST *****\n");
                for (tf = sleep_q; tf; tf = tf->next) {
                        pr_lock(tf);
                }
        } else
                printf("\n***** NO SLEEP LOCKS: %x****\n", sleep_q);
	(void) fflush(stdout);
}
