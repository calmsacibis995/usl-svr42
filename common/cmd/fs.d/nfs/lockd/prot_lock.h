/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/prot_lock.h	1.4.4.3"
#ident	"$Header: $"

/*
 * This file consists of all structure information used by lock manager 
 */

#include <rpc/rpc.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/fcntl.h>
#include "flock.h"
#include "signal.h"
#include "nlm_prot.h"
#include "klm_prot.h"
#include "lockf.h"

typedef struct nlm_testres remote_result;

#define	BUFSIZE		128

#define lstat stat.stat
#define lholder stat.nlm_testrply_u.holder

/*
 * Flock call.
 * used to be in /usr/include/sys/file.h
 */
#define LOCK_SH		1	/* shared lock */
#define LOCK_EX		2	/* exclusive lock */
#define LOCK_NB		4	/* don't block when locking */
#define LOCK_UN		8	/* unlock */

/*
 * Used to be in /usr/include/sys/signal.h
 */
#define	SIGLOST		29	/* resource lost (eg, record-lock lost) */

/*
 * Used to be in flock.c
 */
#define SAMEOWNER(a, b) (((a)->l_pid == (b)->l_pid) && \
                                ((a)->l_sysid == (b)->l_sysid))

#define same_proc(x, y) (obj_cmp(&x->lck.oh, &y->lck.oh))

#define NLM_LOCK_RECLAIM	16
#define MSG 	0		/* choices of comm to remote svr */
#define RPC 	1		/* choices of comm to remote svr */

#define	MAXLEN		MAXEND

#define lck 		alock
#define lld		filocks.set
#define lls		filocks.stat
#define svr		server_name
#define caller		caller_name
#define clnt		clnt_name
#define fh_bytes	fh.n_bytes
#define oh_len		oh.n_len
#define oh_bytes	oh.n_bytes
#define cookie_len	cookie.n_len
#define cookie_bytes	cookie.n_bytes

#define rpc_error	6

/*
 * warning:  struct alock consists of klm_lock and nlm_lock,
 * it has to be modified if either structure has been modified!!!
 */
struct alock {
	netobj cookie;

	/* from klm_prot.h */
        char *server_name;
        netobj fh;

	/* from lockf.h */
	struct data_lock lox;

	/* addition from nlm_prot.h */
	char *caller_name;
	netobj oh;
	int svid;
	u_int l_offset;
        u_int l_len;

	/* addition from lock manager */
	char *clnt_name;
	int op;
};


struct reclock {
	netobj cookie;
	bool_t block;
	bool_t exclusive;
	struct alock alock;
	bool_t reclaim;
	int state;
	int sleep_id;

	/* auxiliary structure */
	SVCXPRT *transp; /* transport handle for delayed response due to */ 
			 /* blocking or grace period */
	int rel;
	int w_flag;

	struct reclock *prev; 
	struct reclock *next;
};
typedef struct reclock reclock;

/*
 * Lock manager vp->v_filocks
 */
struct lm_vnode {
        char *server_name;
        netobj fh;
	struct lm_vnode *prev;
	struct lm_vnode *next;
	struct reclock *reclox;
	int    rel;
};
typedef struct lm_vnode lm_vnode;

struct timer {
	/* timer goes off when exp == curr */
	int exp;
	int curr;
};
typedef struct timer timer;

/*
 * msg passing structure
 */
struct msg_entry {
	reclock *req;
	remote_result *reply;
	timer t;
	int proc; /* procedure name that req is sent to; needed for reply purpose */
	struct msg_entry *prev;
	struct msg_entry *nxt;
};
typedef struct msg_entry msg_entry;

struct priv_struct {
	int pid;
	int *priv_ptr;
};
