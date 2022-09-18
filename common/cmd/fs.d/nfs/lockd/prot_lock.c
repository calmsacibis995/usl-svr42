/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/prot_lock.c	1.11.5.4"
#ident	"$Header: $"

/*
 *	prot_lock.c, consists of low level routines that
 *	manipulate lock entries. This is the place where
 *	real locking codes reside. It is (in most cases)
 *	independent of network code
 */

#include <stdio.h>
#include "sm_inter.h"
#include "sm_res.h"
#include "prot_lock.h"
#include "priv_prot.h"

static struct priv_struct priv;

lm_vnode		*fh_q = NULL;

extern remote_result res_working;

extern int		used_me;
/*
 * pid is used by the status monitor
 */
extern int		pid;
extern int		LockID;
extern int 		debug;
extern int 		local_state;

extern char		*xmalloc();
extern char		hostname[MAXNAMELEN];
extern msg_entry 	*retransmitted();
extern msg_entry 	*msg_q;
extern struct reclock 	*sleep_q;
extern struct stat_res 	*stat_mon();

int			obj_copy();
int			obj_alloc();
int 			contact_monitor();
void			add_fh();


reclock			*search_block_lock();
bool_t			obj_cmp();
bool_t			same_op();
bool_t			same_lock();
bool_t			same_type();
bool_t			same_bound();
bool_t			remote_data();
bool_t			remote_clnt();

lm_vnode 		*search_fh();
lm_vnode		*find_me();
lm_vnode		*get_me();


reclock *
search_block_lock(a)
	reclock *a;
{
	lm_vnode *fp;
	struct reclock *t;

	if (debug)
		printf("enter search_bvlock_lock()....\n");

	for (fp = fh_q; fp; fp = fp->next) {
		if (obj_cmp(&fp->fh, &a->lck.fh)) {
			for (t = fp->reclox; t != NULL; t = t->next) {
				if (same_lock(a, &(t->lck.lox.filocks))) {
					return (t);
				}
			}
		}
	}
	return (NULL);
}

/*
 * search_fh creates fh entry and returns NULL if not found
 */
lm_vnode *
search_fh(a, choice)
	reclock *a;
	int choice;
{
	struct lm_vnode *fp, *new, *get_fh();
	struct reclock *t;

	if (debug) {
		printf("enter search_fh a->lck.lox.LockID=%d...\n",
			a->lck.lox.LockID);
		pr_all();
	}
	/*
	 * Return fh entry
	 */
	for (fp = fh_q; fp; fp = fp->next) {
		if (obj_cmp(&fp->fh, &a->lck.fh)) {
			for (t = fp->reclox; t != NULL; t = t->next) {
                                if (same_lock(a, &(t->lck.lox.filocks))) {
					if (debug)
                                                printf("search_fh(): same lock exists\n");
                                        if (choice) {
                                                return ((struct lm_vnode *) -1);                                        		} else {
                                                return (fp);
                                        }
                                }
                        }
			return (fp);
		}
	}
	if (choice) {
		/*
		 * Create new fh entry
		 */
		if ((new = get_fh()) != NULL) {
			if (!obj_copy(&new->fh, &a->lck.fh)) {
				add_fh(new);
				return (new);
			} else {
				if (debug)
				    printf("search_f(): failed obj_copy\n");
				new->rel= 1;
				release_fh(new);
				return ((struct lm_vnode *)NULL);
			}
		} else {
			return ((struct lm_vnode *)NULL);
		}
	}
	return ((struct lm_vnode *)NULL);
}


/*
 * add fh adds a to the end of fh queue fh_q
 */
void
add_fh(a)
	struct lm_vnode *a;
{
	struct lm_vnode *nl, *next;

	if (debug)
		printf("enter add_fh()....\n");

	if ((nl = fh_q) == NULL) {
		fh_q = a;
		return;
	} else {
		while (nl != NULL) {
			if (obj_cmp(&nl->fh, &a->fh)) {
				if (debug)
					printf("same fh entry already exists\n");
				nl->rel = 1;
				return;
			}
			next = nl;
			nl = nl->next;
		}
		next->next = a;
		a->prev = next;
	}
}

void
remove_fh(a)
	struct lm_vnode *a;
{
	if (debug)
		printf("remove_fh(): entered\n");
	if (a->prev == NULL)
		fh_q = a->next;
	else
		a->prev->next = a->next;
	if (a->next != NULL)
		a->next->prev =a->prev;

	a->rel = 1;
}


void
wakeup(a)
	struct reclock *a;
{
	msg_entry *msgp;
	remote_result *resp, *get_res();
	int ID = 0;
	struct reclock *sr;

	if (debug) {
		printf("enter wakeup()....\n");
		(void) fflush(stdout);
	}

	if (a == NULL) {
		printf("wakeup(): null reclock, ignored\n");
		return;
	}

	for (sr = sleep_q; sr != NULL; sr = sr->next) {
		if (sr->sleep_id == a->sleep_id) {
			msgp = msg_q;
			while (msgp != NULL) {
				if (msgp->req->lck.lox.LockID ==
				   sr->lck.lox.LockID && msgp->reply != NULL
				   && msgp->reply->lstat == nlm_blocked) {
					break;
				}
				msgp = msgp->nxt;
			}
			if (msgp != NULL) {
				resp->lstat = nlm_granted;
				msgp->reply->lstat = nlm_granted;
				printf("wakeup(): adding reply\n");
				add_reply(msgp, resp);
			} else
				release_res(resp);
		}
	}
}


bool_t
obj_cmp(a, b)
	struct netobj *a, *b;
{
	if (a->n_len != b->n_len)
		return (FALSE);
	if (a && b && memcmp(&a->n_bytes[0], &b->n_bytes[0], a->n_len) != 0)
		return (FALSE);
	else
		return (TRUE);
}

/*
 * duplicate b in a;
 * return -1, if malloc error;
 * returen 0, otherwise;
 */
int
obj_alloc(a, b, n)
	netobj *a;
	char *b;
	u_int n;
{
	if (debug)
		printf("enter obj_alloc()....\n");

	a->n_len = n;
	if ((a->n_bytes = xmalloc(n)) == NULL) {
		return (-1);
	}
	else
		memcpy(a->n_bytes, b, a->n_len);
	return (0);
}

/*
 * copy b into a
 * returns 0, if succeeds
 * return -1 upon error
 */
int
obj_copy(a, b)
	netobj *a, *b;
{
	if (debug)
		printf("enter obj_copy()....\n");

	if (b == NULL) {
		/*
		 * trust a is already NULL
		 */
		if (debug)
			printf(" obj_copy(a = %x, b = NULL), a\n", a);
		return (0);
	}

	return (obj_alloc(a, b->n_bytes, b->n_len));
}

bool_t
same_op(a, b)
	reclock *a, *b;
{
	if (debug)
		printf("enter same_op()....\n");

	if (((a-> lck.op & LOCK_EX) && (b-> lck.op & LOCK_EX)) ||
		((a-> lck.op & LOCK_SH) && (b-> lck.op & LOCK_SH)))
		return (1);
	else
		return (0);

}

bool_t
same_bound(a, b)
	struct flock *a, *b;
{
	if (debug)
		printf("enter same_bound()....\n");

	if ((a->l_start == b->l_start) &&
		(a->l_len == b->l_len))
		return (1);
	else
		return (0);
}

bool_t
same_type(a, b)
	struct flock *a, *b;
{
	if (debug)
		printf("enter same_type()...\n");

	if (a->l_type == b->l_type)
		return (1);
	else
		return (0);
}

bool_t
same_lock(a, b)
	reclock *a;
	struct filock *b;
{
	if (debug)
		printf("enter same_lock()....\n");

	if (same_type(&(a->lck.lox.lld), &b->set) &&
		same_bound(&(a->lck.lox.lld), &b->set) &&
		SAMEOWNER(&(a->lck.lox.lld), &b->set))
		return (TRUE);
	else
		return (FALSE);
}

bool_t
simi_lock(a, b)
	reclock *a, *b;
{
	if (same_proc(a, b) && same_op(a, b) &&
	    WITHIN(&b->lck.lox.lld, &a->lck.lox.lld))
		return (TRUE);
	else
		return (FALSE);
}


bool_t
remote_data(a)
	reclock *a;
{
	if (strcmp(a->lck.svr, hostname) == 0)
		return (FALSE);
	else
		return (TRUE);
}


bool_t
remote_clnt(a)
	reclock *a;
{
	if (strcmp(a->lck.clnt, hostname) == 0)
		return (FALSE);
	else
		return (TRUE);
}

/*
 * translate monitor calls into modifying monitor chains
 * returns 0, if success
 * returns -1, in case of error
 */
int
add_mon(a, i)
	reclock *a;
	int i;
{
	if (debug)
		printf("Enter add_mon ........\n");
	if (strcmp(a->lck.svr, a->lck.clnt) == 0)
		/* local case, no need for monitoring */
		return (0);
	if (remote_data(a)) {		/* client */
		if (strlen(hostname) &&
			mond(hostname, PRIV_RECOVERY, i) == -1)
			return (-1);
		if (strlen(a->lck.svr) &&
			mond(a->lck.svr, PRIV_RECOVERY, i) == -1)
			return (-1);
	} else {			/* server */
		if (strlen(a->lck.clnt) &&
			mond(a->lck.clnt, PRIV_CRASH, i) == -1)
			return (-1);
	}
	return (0);
}

/*
 * mond set up the monitor ptr;
 * it return -1, if no more free mp entry is available when needed
 * or cannot contact status monitor
 */
int
mond(site, proc, i)
	char *site;
	int proc;
	int i;
{
	struct lm_vnode *new;

	if (debug)
		printf("enter mond()....\n");

	if (i == 1) {		/* insert! */
		if ((new = find_me(site)) == NULL) { /* not found */
			if (( new = get_me()) == NULL) /* no more me entry */
				return (-1);
			else {	/* create a new mp */
				if ((new->svr = xmalloc(strlen(site)+1)) == NULL) {
					used_me--;
					if ((char *) new != NULL) 
						__free((char *) new, "mond");
					return (-1);
				}

				(void) strcpy(new->svr, site);
				/* contact status monitor */
				if (contact_monitor(proc, new, 1) == -1) {
					used_me--;
					if (new->svr)
						xfree(&new->svr);
					if (new)
						__free((char *) new, "mond");
					return (-1);
				} else {
					insert_me(new);
				}
			}
		}
		return (0);
	} else { /* i== 0; delete! */
		if ((new = find_me(site)) == NULL)
			return (0);   /* happen due to call back */
	}
}

int
contact_monitor(proc, new, i)
	int proc;
	struct lm_vnode *new;
	int i;
{
	struct stat_res *resp;
	int priv_size;
	int func;

	if (debug)
		printf("enter contact_monitor ....\n");
	switch (i) {
	case 0:
		func = SM_UNMON;
		break;
	case 1:
		func = SM_MON;
		break;
	default:
		fprintf(stderr, "unknown contact monitor (%d)\n", i);
		abort();
	}

	priv.pid = pid;
	priv.priv_ptr = (int *) new;
	if ((priv_size = sizeof (struct priv_struct)) > 16) /* move to init */
		fprintf(stderr, "contact_mon: problem with private data size (%d) to status monitor\n",
			priv_size);

	if ( !strcmp(new->svr, hostname) ) {
		return (0);
	}
	resp = stat_mon(new->svr, hostname, PRIV_PROG, PRIV_VERS,
		proc, func, priv_size, &priv);
	if (resp->res_stat == stat_succ) {
		if (resp->sm_stat == stat_succ) {
			local_state = resp->sm_state; /* update local state */
			return (0);
		} else {
			fprintf(stderr,
				"lockd: site %s does not subscribe to status monitor service \n", new->svr);
			return (-1);
		}
	} else {
		fprintf(stderr, "lockd: cannot contact local statd\n");
		return (-1);
	}
}
