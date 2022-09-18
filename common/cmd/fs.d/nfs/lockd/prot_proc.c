/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/prot_proc.c	1.16.7.6"
#ident	"$Header:"

	/*
	 * prot_proc.c
	 * consists all local, remote, and continuation routines:
	 * local_xxx, remote_xxx, and cont_xxx.
	 */

#include <stdio.h>
#include <netdb.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <memory.h>
#include "prot_lock.h"
#include <nfs/nfs.h>
#include <nfs/nfssys.h>

remote_result nlm_result;		/* local nlm result */
remote_result *nlm_resp = &nlm_result;	/* ptr to klm result */

void slpremove();
int  slpadd();
remote_result *remote_cancel();
remote_result *remote_lock();
remote_result *remote_test();
remote_result *remote_unlock();
remote_result *local_test();
remote_result *local_lock();
remote_result *local_cancel();
remote_result *local_unlock();
remote_result *cont_test();
remote_result *cont_lock();
remote_result *cont_unlock();
remote_result *cont_cancel();
remote_result *cont_reclaim();

msg_entry *search_msg();
msg_entry *retransmitted();
struct lm_vnode *search_fh();
struct reclock *blocked_reclox(), *insflck_reclox();

extern int debug, errno;
extern int res_len;
extern lm_vnode *fh_q;
extern msg_entry *msg_q;
extern struct reclock *sleep_q;
extern bool_t obj_cmp();
extern int obj_copy();

#define FDTABLE	1000

struct {
	netobj fh;
	int fd;
} fd_table[FDTABLE];

int used_fd;

print_fdtable()
{
	int i, ii;

	if (debug)
		printf("In print_fdtable()....used_fd=%d\n", used_fd);

	for (i=0; i < FDTABLE; i++) {
		if (fd_table[i].fd) {
			printf("%d : ID=%d\n", i, fd_table[i].fd);
			for (ii = 0; ii < fd_table[i].fh.n_len; ii++) {
				printf("%02x",
					(fd_table[i].fh.n_bytes[ii] & 0xff));
			}
			printf("\n");
		}
	}
}

remove_fd(a)
	struct reclock *a;
{
	int i, ii;

	if (debug)
		printf("In remove_fd() ...\n");

	for (i=0; i < FDTABLE; i++) {
		if (fd_table[i].fh.n_len &&
			obj_cmp(&fd_table[i].fh, &a->lck.fh) &&
			fd_table[i].fd) {
			if (debug) {
				for (ii = 0; ii < a->lck.fh.n_len; ii++) {
					printf("%02x",
						(a->lck.fh.n_bytes[ii] & 0xff));
				}
				printf("\n");
			}
			(void) memset(fd_table[i].fh.n_bytes, 0,
				sizeof (fd_table[i].fh.n_bytes));
			xfree(&(fd_table[i].fh_bytes));
			fd_table[i].fh.n_len = 0;
			fd_table[i].fd = 0;
			used_fd--;
			break;
		}
	}
	if (debug)
		print_fdtable();
}

int
get_fd(a)
	struct reclock *a;
{
	int fd, cmd, i, ii;
	struct {
		char    *fh;
		int	filemode;
		int	*fd;
	} fa;

	if (debug)
		printf("enter get_fd ....\n");

	for (i=0; i < FDTABLE; i++) {
		if (obj_cmp(&(fd_table[i].fh), &(a->lck.fh)) &&
			fd_table[i].fd) {
			if (debug) {
				printf("Found fd entry : a = ");
				for (ii = 0; ii < a->lck.fh.n_len; ii++) {
					printf("%02x",
						(a->lck.fh.n_bytes[ii] & 0xff));
				}
				printf("\nfd_table[i].fh = ");
				for (ii = 0; ii < 32; ii++) {
					printf("%02x",
						(fd_table[i].fh.n_bytes[ii] & 0xff));
				}
				printf("\n");
			}
			return (fd_table[i].fd);
		}
	}
	/*
	 * convert fh to fd
	 */
	cmd = NFS_CNVT;
	fa.fh = a->lck.fh.n_bytes;
	if (debug) {
		printf("Convert fd entry : ");
		for (i = 0; i < a->lck.fh.n_len; i++) {
			printf("%02x", (a->lck.fh.n_bytes[i] & 0xff));
		}
		printf("\n");
	}
	/*
	 * lockd ALWAYS opens file read/write even when the
	 * file may be opened for read-only on the client. This
	 * is because the same fd is used by the lockd to acquire
	 * read and write locks while the actual client process
	 * may use diferrent descriptors or two different proceses
	 * may be involved. This will work as the file mode checks
	 * on the client will never let a write lock request on a
	 * file opened for read get this far. 
	 */
	fa.filemode = O_RDWR;
	fa.fd = &fd;

	if ((i = _nfssys(cmd, &fa)) == -1) {
		/*
		 * Now make lockd work for read only file systems
		 * (and directories), change filemode to O_RDONLY
		 */
		if ((errno == EROFS) || (errno == EISDIR)) {
			fa.filemode = O_RDONLY;
			if ((i = _nfssys(cmd, &fa)) == -1) {
				if (debug)
				  printf("get_fd():nfssys:errno= %d\n", errno);
				if (errno == ENOLCK)
					return (-1);
				else
					return (-2);
			}
		} else {
			if (debug)
				printf("get_fd(): nfssys: errno= %d\n", errno);
			if (errno == ENOLCK)
				return (-1);
			else
				return (-2);
		}
	}

	if (debug)
		printf("_nfssys returns fd %d\n", fd);

	for (i=0; i < FDTABLE; i++) {
		if (!fd_table[i].fh.n_len) {
			obj_copy(&fd_table[i].fh, &a->lck.fh);
			fd_table[i].fd = fd;
			used_fd++;
			break;
		}
	}

	if (debug)
		print_fdtable();

	return (fd_table[i].fd);
}

/*
 * server routine to get a lock for a client
 */
remote_result *
local_lock(a)
	struct reclock *a;
{
	int fd, err, cmd;
	struct flock fld;

	if (debug)
		printf("enter local_lock()...\n");

	/*
	 * convert fh to fd
	 */
	if ((fd = get_fd(a)) < 0) {
		if (fd == -1)
			nlm_resp->lstat = nlm_denied_nolocks;
		else
			nlm_resp->lstat = nlm_denied;
		return (nlm_resp);
	}

	/*
	 * set the lock
	 */
	if (debug) {
		printf("enter local_lock...FD=%d\n", fd);
		pr_lock(a);
		(void) fflush(stdout);
	}

	a->lck.lox.lld.l_sysid = get_client_sysid(a->lck.caller);

	if (a->block)
		cmd = F_RSETLKW;
	else
		cmd = F_RSETLK;
	if (a->exclusive)
		fld.l_type = F_WRLCK;
	else
		fld.l_type = F_RDLCK;

	fld.l_whence = 0;
	fld.l_start = a->lck.lox.lld.l_start;
	fld.l_len = a->lck.lox.lld.l_len;
	fld.l_pid = a->lck.lox.lld.l_pid;
	fld.l_sysid = a->lck.lox.lld.l_sysid;
	if (debug) {
		printf("fld.l_start=%d fld.l_len=%d fld.l_sysid=%x\n",
			fld.l_start, fld.l_len, fld.l_sysid);
	}
	if ((err = fcntl(fd, cmd, &fld)) == -1) {
		if ((errno != EINTR) && debug)
			perror("fcntl");
		if (errno == EINTR) {
			nlm_resp->lstat = nlm_blocked;
			a->w_flag = 1;
		} else if (errno == EDEADLK) {
			nlm_resp->lstat = nlm_deadlck;
			a->w_flag = 0;
		} else if (errno == ENOLCK) {
			printf("rpc.lockd: out of lock.\n");
			nlm_resp->lstat = nlm_denied_nolocks;
		} else {
			if(debug)
				printf("local_lock(): could not lock. \n");
			nlm_resp->lstat = nlm_denied;
		}
	} else {
		nlm_resp->lstat = nlm_granted;
	}

	return (nlm_resp);
}

/*
 * client routine to get a lock from the server
 */
remote_result *
remote_lock(a, proc)
	struct reclock *a;
	int proc;
{
	struct lm_vnode *fp;
	struct msg_entry *msgp;
	struct reclock  *sr, *found, *insrt = NULL;
        int i, retval;

	if (debug) {
		printf("remote_lock(): entered\n");
		pr_lock(a);
	}

	/*
	 * Create/get fh entry
	 */
	if ((fp = search_fh(a, 1)) == (struct lm_vnode *)-1) {
		/*
		 * same lock exists
		 */
		a->lck.lox.granted = 1;
		a->rel = 1;
		nlm_resp->lstat = nlm_granted;
		return (nlm_resp);
	} else {
		if ((fp->reclox == NULL) ||
			(found = blocked_reclox(fp->reclox, &(a->lck.lox.lld), 
				&insrt)) == NULL) {
			/*
			 * will not block locally so
			 * now check on server
			 */
			a->lck.lox.granted = 0;
                        if (nlm_call(NLM_LOCK_MSG, a, 0) == -1)
                        	a->rel = 1;
		} else {
			/*
			 * there exists a blocking lock
			 * held by another process locally
			 */
			if (!a->block) {
				/*
				 * fail non-blocking requests
				 * right away
				 */
				a->lck.lox.granted = 0;
				a->rel = 1;
				nlm_resp->lstat = nlm_denied;
				return (nlm_resp);
                        }

                        if (deadflck_reclox(found, &(a->lck.lox.lld))) {
				/*
				 * will cause deadlock
				 */
                                nlm_resp->lstat = nlm_deadlck;
                                a->lck.lox.granted = 0;
                                a->rel = 1;
                                return (nlm_resp);
                        }

			/*
		 	 * put in sleep queue and don't
			 * bother going to the server
		 	 */
			if (slpadd(found, a, fp->reclox, insrt)) {
                        	nlm_resp->lstat = nlm_denied_nolocks;
                        	a->lck.lox.granted = 0;
                        	a->rel = 1;
				return (nlm_resp);
			}

			a->lck.lox.granted = 0;
			a->rel = 1;
			nlm_resp->lstat = nlm_blocked;

			return (nlm_resp);
		}
	}

	/*
	 * no reply available
	 */
	return (NULL);
}

/*
 * server routine to unlock a lock for a client
 */
remote_result *
local_unlock(a)
	struct reclock *a;
{
	int fd, cmd;
	struct flock fld;

	if (debug)
		printf("enter local_unlock\n");
	/*
	 * convert fh to fd
	 */
	if ((fd = get_fd(a)) < 0) {
		if (fd == -1)
			nlm_resp->lstat = nlm_denied_nolocks;
		else
			nlm_resp->lstat = nlm_denied;
		return (nlm_resp);
	}

	/*
	 * unlock the lock
	 */
	a->lck.lox.lld.l_sysid = get_client_sysid(a->lck.caller);
	if (a->block)
		cmd = F_RSETLKW;
	else
		cmd = F_RSETLK;
	fld.l_type = F_UNLCK;
	fld.l_whence = 0;
	fld.l_start = a->lck.lox.lld.l_start;
	fld.l_len = a->lck.lox.lld.l_len;
	fld.l_pid = a->lck.lox.lld.l_pid;
	fld.l_sysid = a->lck.lox.lld.l_sysid;
	if (debug) {
		printf("fld.l_start=%d fld.l_len=%d fld.l_pid=%d fld.l_rsys=%x\n",
			fld.l_start, fld.l_len, fld.l_pid, fld.l_sysid);
	}
	if (fcntl(fd, cmd, &fld) == -1) {
		if (errno!= EINTR && debug)
			perror("local_unlock(): fcntl():");
		if (errno == EINTR) {
			nlm_resp->lstat = nlm_blocked;
			a->w_flag = 1;
		} else if (errno == ENOLCK) {
			if (debug)
				printf("lock_unlock(): out of locks.\n");
			nlm_resp->lstat = nlm_denied_nolocks;
		} else {
			if (debug)
				printf("lock_unlock(): could not unlock\n");
			nlm_resp->lstat = nlm_denied;
		}
	} else {
		nlm_resp->lstat = nlm_granted;
		/*
		 * Update fd table
		 */
		remove_fd(a);
		close(fd);
	}

	return (nlm_resp);
}

/*
 * client routine to get a lock from the server
 */
remote_result *
remote_unlock(a, proc)
	struct reclock *a;
	int proc;
{
	if (debug)
		printf("enter remote_unlock\n");

	if (search_fh(a, 0) == (struct lm_vnode *)NULL) {
		a->rel = 1;

		/*
		 * remove from sleep queue if it is there
		 */
		slpremove(a);

		nlm_resp->lstat = nlm_granted;
		return (nlm_resp);
	} else {
		if (nlm_call(NLM_UNLOCK_MSG, a, 0) == -1)
			a->rel = 1;
		return (NULL);
	}
}


/*
 * server routine to test a lock for a client
 */
remote_result *
local_test(a)
	struct reclock *a;
{
	int fd, cmd;
	struct flock fld;

	if (debug)
		printf("enter local_test()...\n");

	/*
	 * convert fh to fd
	 */
	if ((fd = get_fd(a)) < 0) {
		if (fd == -1)
			nlm_resp->lstat = nlm_denied_nolocks;
		else
			nlm_resp->lstat = nlm_denied;
		nlm_resp->lstat = nlm_denied;
		return (nlm_resp);
	}

	/*
	 * test the lock
	 */
	cmd = F_RGETLK;
	a->lck.lox.lld.l_sysid = get_client_sysid(a->lck.caller);
	if (a->exclusive)
		fld.l_type = F_WRLCK;
	else
		fld.l_type = F_RDLCK;
	fld.l_whence = 0;
	fld.l_start = a->lck.lox.lld.l_start;
	fld.l_len = a->lck.lox.lld.l_len;
	fld.l_pid = a->lck.lox.lld.l_pid;
	fld.l_sysid = a->lck.lox.lld.l_sysid;
	if (fcntl(fd, cmd, &fld) == -1) {
		if (debug)
			perror("local_test(): fcntl():");
		if (errno == EINTR) {
			a->w_flag = 1;
			nlm_resp->lstat = nlm_blocked;
		} else if (errno == ENOLCK) {
			if (debug)
				printf("local_test(): out of locks\n");
			nlm_resp->lstat = nlm_denied_nolocks;
		} else {
			if (debug)
				printf("local_test(): could not test\n");
			nlm_resp->lstat = nlm_denied;
		}
	} else {
		if (fld.l_type == F_UNLCK) {
			nlm_resp->lstat = nlm_granted;
			a->lck.lox.lld.l_type = fld.l_type;
		} else {
			nlm_resp->lstat = nlm_denied;
			a->lck.lox.lld.l_type = fld.l_type;
			a->lck.lox.lld.l_start = fld.l_start;
			a->lck.lox.lld.l_len = fld.l_len;
			a->lck.lox.lld.l_pid = fld.l_pid;
			a->lck.lox.lld.l_sysid = fld.l_sysid;
		}
	}
	if (debug) {
		printf("fld.l_start=%d fld.l_len=%d fld.l_sysid=%x\n",
			fld.l_start, fld.l_len, fld.l_sysid);
	}
	return (nlm_resp);
}

/*
 * client routine to test a lock on the server
 */
remote_result *
remote_test(a, proc)
	struct reclock *a;
	int proc;
{
	if (debug)
		printf("enter remote_test()\n");

	if (nlm_call(NLM_TEST_MSG, a, 0) == -1)
		a->rel = 1;

	return (NULL);
}

/*
 * server routine to cancel a lock for a client
 */
remote_result *
local_cancel(a)
	struct reclock *a;
{
	int fd, cmd;
	struct flock fld;
	msg_entry *msgp;

	if (debug)
		printf("enter local_cancel(%x)\n", a);

	/*
	 * convert fh to fd
	 */
	if ((fd = get_fd(a)) < 0) {
		if (fd == -1)
			nlm_resp->lstat = nlm_denied_nolocks;
		else
			nlm_resp->lstat = nlm_granted;
		return (nlm_resp);
	}
	/*
	 * unlock the lock
	 */
	a->lck.lox.lld.l_sysid = get_client_sysid(a->lck.caller);
	if (a->block)
		cmd = F_RSETLKW;
	else
		cmd = F_RSETLK;
	fld.l_type = F_UNLCK;
	fld.l_whence = 0;
	fld.l_start = a->lck.lox.lld.l_start;
	fld.l_len = a->lck.lox.lld.l_len;
	fld.l_pid = a->lck.lox.lld.l_pid;
	fld.l_sysid = a->lck.lox.lld.l_sysid;
	if (debug) {
		printf("fld.l_start=%d fld.l_len=%d fld.l_pid=%d fld.l_rsys=%x\n",
			fld.l_start, fld.l_len, fld.l_pid, fld.l_sysid);
	}
	if (fcntl(fd, cmd, &fld) == -1) {
		if (errno!= EINTR && debug)
			perror("local_cancel(): fcntl():");
		if (errno == EINTR) {
			nlm_resp->lstat = nlm_blocked;
			a->w_flag = 1;
		} else if (errno == ENOLCK) {
			if (debug)
				printf("lock_cancel(): out of locks.\n");
			nlm_resp->lstat = nlm_denied_nolocks;
		} else {
			if (debug)
				printf("lock_cancel(): could not cancel\n");
			nlm_resp->lstat = nlm_denied;
		}
	} else {
		nlm_resp->lstat = nlm_granted;
		/*
		 * Update fd table
		 */
		remove_fd(a);
		close(fd);
	}

	return (nlm_resp);
}

/*
 * client routine to cancel a lock
 */
remote_result *
remote_cancel(a, proc)
	struct reclock *a;
	int proc;
{
	msg_entry *msgp;

	if (debug)
		printf("enter remote_cancel(%x)\n", a);

	if (search_fh(a, 0) == (struct lm_vnode *)NULL) {
		a->rel = 1;

		/*
		 * remove from sleep queue if it is there
		 */
		slpremove(a);

		nlm_resp->lstat = nlm_granted;
		return (nlm_resp);
	} else {
		if (nlm_call(NLM_CANCEL_MSG, a, 0) == -1)
			a->rel = 1;
		return (NULL);
	}
}

/*
 * client routine to continue locking when the server
 * comes back with a reply
 */
remote_result *
cont_lock(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	struct lm_vnode *lox;
	struct reclock *sr, *found_reclox;
	int i, retval;

	if (debug) {
		printf("enter cont_lock (%x) ID=%d \n", a, a->lck.lox.LockID);
	}
	switch (resp->lstat) {
	case nlm_granted:
		a->rel = 0;
		/*
		 * Update fh struct table
		 */
		if ((lox = search_fh(a, 1)) == (struct lm_vnode *)NULL) {
			a->rel = 1;
			release_res(resp);
			if (debug)
				printf("cont_lock(): unable to allocate fh\n");
			return (NULL);
		}
                retval = flckadj_reclox(&(lox->reclox), (struct reclock *)NULL, a);

		/*
		 * remove from sleeping queue if it is there
		 */
		slpremove(a);

		if (debug) {
			printf("cont_lock(): after updating table :\n");
			pr_all();
		}
		if (add_mon(a, 1) == -1)
			printf("cont_lock(): add_mon failed\n");
		return (resp);

	case nlm_denied:
	case nlm_denied_nolocks:
		/*
		 * Update fh struct table
		 */
		if ((lox = search_fh(a, 1)) == (struct lm_vnode *)NULL) {
			a->rel = 1;
			release_res(resp);
			if (debug)
				printf("cont_lock(): unable to alloc fh\n");
			return (NULL);
		}
		a->rel = 1;
		a->block = FALSE;
		a->lck.lox.granted = 0;
		if (debug) {
                        printf("cont_lock(): after updating table\n");
                        pr_all();
                }

		/*
		 * remove from sleeping queue if it is there
		 */
		slpremove(a);

		return (resp);
	case nlm_deadlck:
		/*
		 * Update fh struct table
		 */
		if ((lox = search_fh(a, 1)) == (struct lm_vnode *)NULL) {
			a->rel = 1;
			release_res(resp);
			printf("cont_lock(): unable to alloc fh\n");
			return (NULL);
		}
		a->rel = 1;
		a->block = TRUE;
		a->w_flag = 0;
		a->lck.lox.granted = 0;

		if (debug) {
                        printf("cont_lock(): after updating table\n");
                        pr_all();
                }

		/*
		 * remove from sleeping queue if it is there
		 */
		slpremove(a);

		return (resp);
	case nlm_blocked:
		/*
                 * Update fh struct table
                 */
                if ((lox = search_fh(a, 1)) == (struct lm_vnode *)NULL) {
                        a->rel = 1;
                        release_res(resp);
                        printf("cont_lock(): unable to alloc fh\n");
                        return (NULL);
                }
		a->rel = 0;
		a->w_flag = 1;
		a->block = TRUE;

		/*
		 * put in sleep queue if not there
		 */
		if (slpadd((struct reclock *)NULL, a, lox,
				(struct reclock *)NULL)) {
                        nlm_resp->lstat = nlm_denied_nolocks;
                        a->lck.lox.granted = 0;
                        a->rel = 1;
			return (nlm_resp);
		}

		return (resp);
	case nlm_denied_grace_period:
		a->rel = 0;

		/*
		 * remove from sleeping queue if it is there
		 */
		slpremove(a);

		release_res(resp);
		return (NULL);
	default:
		a->rel = 1;
		release_res(resp);
		printf("cont_lock(): unknown lock return: %d\n",
						resp->lstat);
		return (NULL);
	}
}


/*
 * client routine to continue unlocking when the server
 * comes back with a reply
 */
remote_result *
cont_unlock(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	struct lm_vnode *lox;
	struct reclock *sr;
	int retval;

	if (debug)
		printf("enter cont_unlock\n");

	a->rel = 1;
	switch (resp->lstat) {
		case nlm_granted:
			/*
			 * Update fh struct table
			 */
			if ((lox = search_fh(a, 0)) == (struct lm_vnode *)NULL) {
				if (debug)
					printf("cont_unlock(): fh gone\n");
				/*
				 * file handle missing, should not happpen
				 * but don't sweat, we're unlocking!
				 */
				a->rel = 1;
				resp->lstat = nlm_granted;
				return (resp);
			}

			/*
			 * adjust locks so that all locks over the unlocked
			 * area are removed.
			 */
			retval = flckadj_reclox(&(lox->reclox), 
					(struct reclock *)NULL, a);

			if (!lox->reclox) {
				remove_fh(lox);
                                release_fh(lox);
                        }
			resp->lstat = nlm_granted;
			if (debug) {
                                printf("cont_unlock(): after updating the tables\n");
                                pr_all();
                        }
			return (resp);
		case nlm_denied:
			/*
			 * this cannot happen
			 */
		case nlm_denied_nolocks:
			return (resp);
		case nlm_blocked:
			/*
			 * this cannot happen
			 */
			a->w_flag = 1;
			return (resp);
		case nlm_denied_grace_period:
			a->rel = 0;
			release_res(resp);
			return (NULL);
		default:
			a->rel = 0;
			release_res(resp);
			printf("cont_unlock(): unkown return %d\n",
				resp->lstat);
			return (NULL);
	}
}

/*
 * client routine to continue testing when the server
 * comes back with a reply
 */
remote_result *
cont_test(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	if (debug)
		printf("enter cont_test\n");

	a->rel = 1;
	switch (resp->lstat) {
	case nlm_denied_grace_period:
		a->rel = 0;
		release_res(resp);
		return (NULL);
	case nlm_granted:
	case nlm_denied:
		if (debug)
			printf("lock blocked by %d, (%d, %d)\n",
				resp->lholder.svid, resp->lholder.l_offset,
				resp->lholder.l_len);
		return (resp);
	case nlm_denied_nolocks:
		return (resp);
	case nlm_blocked:
		a->w_flag = 1;
		return (resp);
	default:
		printf("cont_test(): unknown return: %d\n",
			resp->lstat);
		release_res(resp);
		return (NULL);
	}
}

/*
 * client routine to continue caceling when the server
 * comes back with a reply
 */
remote_result *
cont_cancel(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	msg_entry *msgp;
	register struct reclock  *sr;

	if (debug)
		printf("enter cont_cancel\n");

	a->rel = 1;
	switch (resp->lstat) {
	case nlm_granted:
		/*
		 * remove from sleeping queue if it is there
		 */
		slpremove(a);

		if (search_fh(a, 0) == (struct lm_vnode *)NULL) {
			if (debug)
				printf("cont_cancel(): fh gone\n");
			a->rel = 1;
			return (resp);
		}

		if (debug) {
			printf("cont_lock(): after updating table :\n");
			pr_all();
		}

		return (resp);
	case nlm_blocked:
		/*
		 * this cannot happen
		 */
		a->w_flag = 1;
	case nlm_denied_nolocks:
		return (resp);
	case nlm_denied_grace_period:
		a->rel = 0;
		release_res(resp);
		return (NULL);
	case nlm_denied:
		/*
		 * remove from sleeping queue if it is there
		 */
		slpremove(a);

		if ((search_fh(a, 0) != (struct lm_vnode *)NULL)
				&& (a->w_flag == 1)) {
			a->w_flag = 0;
			a->rel = 1;
			return (resp);
		} else if (a != NULL && a->w_flag == 0) {
			/*
			 * remote and local lock tbl inconsistent
			 */
			if (debug)
				printf("cont_cancel(): inconsistency present\n");
			release_res(resp);
			return (NULL);
		} else {
			return (resp);
		}
	default:
		if (debug)
			printf("cont_cancel(): bad reply\n");
		release_res(resp);
		return (NULL);
	}
}

/*
 * client routine to continue reclaiming when the server
 * comes back with a reply
 */
remote_result *
cont_reclaim(a, resp)
	struct reclock *a;
	remote_result *resp;
{
	remote_result *local;

	if (debug)
		printf("enter cont_reclaim\n");
	switch (resp->lstat) {
	case nlm_granted:
	case nlm_denied:
	case nlm_denied_nolocks:
	case nlm_blocked:
		local = resp;
		break;
	case nlm_denied_grace_period:
		if (a->reclaim)
			printf("cont_reclaim(): reclaim lock req(%x) is returned due to grace period, impossible\n", a);
		local = NULL;
		break;
	default:
		printf("cont_reclaim(): unknown return: %d\n", resp->lstat);
		local = NULL;
		break;
	}

	if (local == NULL)
		release_res(resp);
	return (local);
}

int
get_client_sysid(name)
	char *name;
{
	struct hostent *hp;
	char buf[BUFSIZE];
	int sysid;

	if ((hp = gethostbyname(name)) == NULL) {
		printf( "lockd: gethostbyname failed.\n");
	}
	sprintf(buf, "%02x%02x%02x%02x",
		(u_char) hp->h_addr[0], (u_char) hp->h_addr[1],
		(u_char) hp->h_addr[2], (u_char) hp->h_addr[3]);
	sscanf(buf,"%x", &sysid);
	return (sysid);
}

void
slpremove(a)
	struct reclock *a;
{
	struct reclock *sr;
	int sleeping = 0;

	if (debug)
		printf("slpremove() entered\n");

	for (sr = sleep_q; sr != NULL; sr = sr->next) {
		if (same_lock(a, &(sr->lck.lox.filocks))) {
			if (debug)
				printf("slpremove(): slp lock found\n");
			sleeping = 1;
			break;
		}
	}

	if (sleeping) {
		sleeping = 0;
		delflck_reclox(&sleep_q, sr);
		if (debug)
			printf("slpremove(): slp lock removed\n");
	}
}

int
slpadd(found, a, lox, insrt)
	struct reclock *found, *a, *insrt;
	struct lm_vnode *lox;
{
	struct reclock *sr;
	int sleeping = 0;

	if (debug)
		printf("entered slpadd\n");

	for (sr = sleep_q; sr != NULL; sr = sr->next) {
		if (same_lock(a, &(sr->lck.lox.filocks))) {
			if (debug)
				printf("slpadd(): slp lock found\n");
			sleeping = 1;
			break;
		}
	}

	if (!sleeping) {
		if (debug)
			printf("slpaddd:found is = %x\n", found);
		if ((sr=insflck_reclox(&sleep_q, a, (struct reclock *)NULL))
				== (struct reclock *)NULL)
					return(1);
		if (debug)
			printf("slpaddd:found is = %x\n", found);
		if (found) {
                        sr->lck.lox.lls.wakeflg = found->lck.lox.lls.wakeflg++;
                        sr->lck.lox.lls.blk.pid = found->lck.lox.lld.l_pid;
                        sr->lck.lox.lls.blk.sysid = found->lck.lox.lld.l_sysid;
			if (debug)
				printf("slpadd:pid %d, sysid %\n",
				sr->lck.lox.lls.blk.pid, sr->lck.lox.lls.blk.sysid);
		}
	}

	return(0);
}
