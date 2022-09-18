/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:proc/ipc/sem.c	1.8.3.3"
#ident	"$Header: $"
/*
 * Inter-Process Communication Semaphore Facility.
 */

#include <acc/audit/audit.h>
#include <acc/dac/acl.h>
#include <acc/mac/cca.h>
#include <acc/mac/covert.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/ipc/ipc.h>
#include <proc/ipc/ipcsec.h>
#include <proc/ipc/sem.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/map.h>
#include <util/param.h>
#include <util/sysinfo.h>
#include <util/types.h>

/*
 * These externs are defined in sem.cf
 */

extern struct semid_ds	sema[];		/* semaphore data structures */
extern struct sem	sem[];		/* semaphores */
extern struct map	semmap[];	/* sem allocation map */
extern struct sem_undo	*sem_undo[];	/* undo table pointers */
extern struct sem_undo	semu[];		/* operation adjust on exit table */
extern struct seminfo seminfo;		/* param information structure */

STATIC struct sem_undo	*semunp;	/* ptr to head of undo chain */
STATIC struct sem_undo	*semfup;	/* ptr to head of free undo chain */

/* control structure for covert channel limiter */
STATIC ccevent_t cc_re_sem = { CC_RE_SEM, CCBITS_RE_SEM };

/*
 * Argument vectors for the various flavors of semsys().
 */

#define	SEMCTL	0
#define	SEMGET	1
#define	SEMOP	2

struct semsysa {
	int		opcode;
};

struct semctla {
	int		opcode;
	int		semid;
	uint		semnum;
	int		cmd;
	int		arg;
};

struct semgeta {
	int		opcode;
	key_t		key;
	int		nsems;
	int		semflg;
};

struct semopa {
	int		opcode;
	int		semid;
	struct sembuf	*sops;
	uint		nsops;
};

/*
 * semaoe - Create or update adjust on exit entry.
 */
STATIC int
semaoe(val, id, num)
	short	val;	/* operation value to be adjusted on exit */
	int	id;	/* semid */
	ushort	num;	/* semaphore number */
{
	register struct undo		*uup;	/* ptr to entry to update */
	register struct	undo		*uup2;	/* ptr to move entry */
	register struct sem_undo	*up;	/* ptr to process undo struct */
	register struct	sem_undo	*up2;	/* ptr to undo list */
	register int			i;	/* loop control */
	register int			found;	/* matching entry found flag */

	if (val == 0)
		return 0;
	if (val > seminfo.semaem || val < -seminfo.semaem)
		return ERANGE;
	if ((up = sem_undo[u.u_procp->p_slot]) == NULL)
		if ((up = semfup) == NULL) {
			cc_limiter(&cc_re_sem, u.u_cred);
			return ENOSPC;
		} else {
			semfup = up->un_np;
			up->un_np = NULL;
			sem_undo[u.u_procp->p_slot] = up;
		}
	for (uup = up->un_ent, found = i = 0;i < up->un_cnt;i++) {
		if (uup->un_id < id
		  || (uup->un_id == id && uup->un_num < num)) {
			uup++;
			continue;
		}
		if (uup->un_id == id && uup->un_num == num)
			found = 1;
		break;
	}
	if (!found) {
		if (up->un_cnt >= seminfo.semume)
			return EINVAL;
		if (up->un_cnt == 0) {
			up->un_np = semunp;
			semunp = up;
		}
		uup2 = &up->un_ent[up->un_cnt++];
		while (uup2-- > uup)
			*(uup2 + 1) = *uup2;
		uup->un_id = id;
		uup->un_num = num;
		uup->un_aoe = -val;
		return 0;
	}
	uup->un_aoe -= val;
	if (uup->un_aoe > seminfo.semaem || uup->un_aoe < -seminfo.semaem) {
		uup->un_aoe += val;
		return ERANGE;
	}
	if (uup->un_aoe == 0) {
		uup2 = &up->un_ent[--(up->un_cnt)];
		while (uup++ < uup2)
			*(uup - 1) = *uup;
		if (up->un_cnt == 0) {

			/* Remove process from undo list. */
			if (semunp == up)
				semunp = up->un_np;
			else
				for (up2 = semunp;up2 != NULL;up2 = up2->un_np)
					if (up2->un_np == up) {
						up2->un_np = up->un_np;
						break;
					}
			up->un_np = NULL;
		}
	}
	return 0;
}

/*
 * semunrm - Undo entry remover.
 *
 * This routine is called to clear all undo entries for a set of semaphores
 * that are being removed from the system or are being reset by SETVAL or
 * SETVALS commands to semctl.
 */
STATIC void
semunrm(id, low, high)
	int	id;	/* semid */
	ushort	low;	/* lowest semaphore being changed */
	ushort	high;	/* highest semaphore being changed */
{
	register struct sem_undo	*pp;	/* ptr to predecessor to p */
	register struct	sem_undo	*p;	/* ptr to current entry */
	register struct undo		*up;	/* ptr to undo entry */
	register int			i;	/* loop control */
	register int			j;	/* loop control */

	pp = NULL;
	p = semunp;
	while (p != NULL) {

		/* Search through current structure for matching entries. */
		for (up = p->un_ent, i = 0;i < p->un_cnt;) {
			if (id < up->un_id)
				break;
			if (id > up->un_id || low > up->un_num) {
				up++;
				i++;
				continue;
			}
			if (high < up->un_num)
				break;
			for (j = i; ++j < p->un_cnt; )
				p->un_ent[j - 1] = p->un_ent[j];
			p->un_cnt--;
		}

		/* Reset pointers for next round. */
		if (p->un_cnt == 0)

			/* Remove from linked list. */
			if (pp == NULL) {
				semunp = p->un_np;
				p->un_np = NULL;
				p = semunp;
			} else {
				pp->un_np = p->un_np;
				p->un_np = NULL;
				p = pp->un_np;
			}
		else {
			pp = p;
			p = p->un_np;
		}
	}
}

/*
 * semundo - Undo work done up to finding an operation that can't be done.
 */
STATIC void
semundo(op, n, id, sp)
	register struct sembuf	*op;	/* first operation that was done ptr */
	register int		n;	/* # of operations that were done */
	register int		id;	/* semaphore id */
	register struct semid_ds *sp;	/* semaphore data structure ptr */
{
	register struct sem	*semp;	/* semaphore ptr */

	for (op += n - 1; n--; op--) {
		if (op->sem_op == 0)
			continue;
		semp = sp->sem_base + op->sem_num;
		semp->semval -= op->sem_op;
		if (op->sem_flg & SEM_UNDO)
			semaoe(-op->sem_op, id, op->sem_num);
	}
}

/*
 * semconv - Convert user supplied semid into a ptr to the associated
 * semaphore header.
 *
 * This function cannot be static because it is used in ipcdac.c.
 */
int
semconv(s, spp)
	register int	s;	/* semid */
	struct semid_ds	**spp;	/* semaphore header to be returned */
{
	register struct semid_ds	*sp;	/* ptr to associated header */

	if (s < 0)
		return EINVAL;
	sp = &sema[s % seminfo.semmni];
	if ((sp->sem_perm.mode & IPC_ALLOC) == 0
	  || s / seminfo.semmni != sp->sem_perm.seq)
		return EINVAL;
	*spp = sp;
	return 0;
}

/*
 * semctl - Semctl system call.
 */
STATIC int
semctl(uap, rvp)
	register struct semctla *uap;
	rval_t *rvp;
{
	struct	semid_ds		*sp;	/* ptr to semaphore header */
	register struct sem		*p;	/* ptr to semaphore */
	register unsigned int		i;	/* loop control */
	register int			error;
	caddr_t				base;
	struct semid_ds			ds;	/* temporary storage */
	struct o_semid_ds		ods;
	ushort				*semvals;

	if (error = semconv(uap->semid, &sp))
		return error;

        /* 
         * Check if the Object Level Audit Criteria
         * pertains to this event and lid.
         */
	(void)adt_auditme(A_IPCCHK,ADT_SEM_CTL,sp->sem_perm.ipc_secp->ipc_lid);

	rvp->r_val1 = 0;
	switch (uap->cmd) {

	/* Remove semaphore set. */
	case IPC_O_RMID:
	case IPC_RMID:
		if (error =
		    ipcaccess(&sp->sem_perm, SEM_R|SEM_A, IPC_MAC, u.u_cred))
			return error;
                MAC_ASSERT(sp,MAC_SAME);
		
		/* must have ownership */
		if (u.u_cred->cr_uid != sp->sem_perm.uid
		  && u.u_cred->cr_uid != sp->sem_perm.cuid
		  && pm_denied(u.u_cred, P_OWNER))
			return EPERM;
		semunrm(uap->semid, 0, sp->sem_nsems);
		for (i = sp->sem_nsems, p = sp->sem_base;i--;p++) {
			p->semval = p->sempid = 0;
			if (p->semncnt) {
				wakeprocs((caddr_t)&p->semncnt, PRMPT);
				p->semncnt = 0;
			}
			if (p->semzcnt) {
				wakeprocs((caddr_t)&p->semzcnt, PRMPT);
				p->semzcnt = 0;
			}
		}
		rmfree(semmap,(long)sp->sem_nsems,(ulong)(sp->sem_base-sem)+1);
		/* SVR3 - seq number remains within SHRT_MAX range */
		if ((uap->cmd == IPC_O_RMID && (uap->semid + seminfo.semmni) >
			SHRT_MAX) || (uap->semid + seminfo.semmni) < 0)
			sp->sem_perm.seq = 0;
		else
			sp->sem_perm.seq++;
		sp->sem_perm.mode = 0;
		/*
		 * Free ACL for object; note that the argument to
		 * FRIPACL must be an unevaluated security entry.
		 */
		FRIPCACL(sp->sem_perm.ipc_secp);
		return 0;

	/* Set ownership and permissions. */
	case IPC_O_SET:
		if (error =
		    ipcaccess(&sp->sem_perm, SEM_R|SEM_A, IPC_MAC, u.u_cred))
			return error;
                MAC_ASSERT(sp,MAC_SAME);
		
		/* must have ownership */
		if (u.u_cred->cr_uid != sp->sem_perm.uid
		  && u.u_cred->cr_uid != sp->sem_perm.cuid
		  && pm_denied(u.u_cred, P_OWNER))
			return EPERM;
		if (copyin((caddr_t)uap->arg, (caddr_t)&ods,
		  sizeof(ods)))
			return EFAULT;
		if (ods.sem_perm.uid > MAXUID ||
				ods.sem_perm.gid > MAXUID)
			return(EINVAL);
		sp->sem_perm.uid = ods.sem_perm.uid;
		sp->sem_perm.gid = ods.sem_perm.gid;
		sp->sem_perm.mode = ods.sem_perm.mode & 0777 | IPC_ALLOC;
		sp->sem_ctime = hrestime.tv_sec;
		return 0;

	case IPC_SET:
		if (error =
		    ipcaccess(&sp->sem_perm, SEM_R|SEM_A, IPC_MAC, u.u_cred))
			return error;
                MAC_ASSERT(sp,MAC_SAME);
		
		/* must have ownership */
		if (u.u_cred->cr_uid != sp->sem_perm.uid
		  && u.u_cred->cr_uid != sp->sem_perm.cuid
		  && pm_denied(u.u_cred, P_OWNER))
			return EPERM;
		if (copyin((caddr_t)uap->arg, (caddr_t)&ds,
		  sizeof(ds)))
			return EFAULT;
		if (ds.sem_perm.uid < (uid_t)0 || ds.sem_perm.uid > MAXUID
				|| ds.sem_perm.gid < (gid_t)0 ||
				ds.sem_perm.gid > MAXUID)
			return(EINVAL);
		sp->sem_perm.uid = ds.sem_perm.uid;
		sp->sem_perm.gid = ds.sem_perm.gid;
		sp->sem_perm.mode = ds.sem_perm.mode & 0777 | IPC_ALLOC;
		sp->sem_ctime = hrestime.tv_sec;
		return 0;

	/* Get semaphore data structure. */
	case IPC_O_STAT:
		if (error =
		    ipcaccess(&sp->sem_perm, SEM_R, IPC_MAC|IPC_DAC, u.u_cred))
			return error;
                MAC_ASSERT(sp,MAC_DOMINATES);
		
		/* copy expanded semid_ds structure to an SVR3 semid_ds version.
		** Check whether SVR4 values are too large to store into an SVR3
		** semid_ds structure.
		*/

		if (sp->sem_perm.uid > USHRT_MAX || sp->sem_perm.gid > USHRT_MAX ||
		    sp->sem_perm.cuid > USHRT_MAX || sp->sem_perm.cgid > USHRT_MAX ||
		    sp->sem_perm.seq > USHRT_MAX)
			return EOVERFLOW;

		ods.sem_perm.uid = (o_uid_t) sp->sem_perm.uid;
		ods.sem_perm.gid = (o_gid_t) sp->sem_perm.gid;
		ods.sem_perm.cuid = (o_uid_t) sp->sem_perm.cuid;
		ods.sem_perm.cgid = (o_gid_t) sp->sem_perm.cgid;
		ods.sem_perm.mode = (o_mode_t) sp->sem_perm.mode;
		ods.sem_perm.seq = (ushort) sp->sem_perm.seq;
		ods.sem_perm.key = sp->sem_perm.key;

		ods.sem_base = NULL;	/* kernel addr */
		ods.sem_nsems = sp->sem_nsems;
		ods.sem_otime = sp->sem_otime;
		ods.sem_ctime = sp->sem_ctime;

		if (copyout((caddr_t)&ods, (caddr_t)uap->arg, sizeof(ods)))
			return EFAULT;
		return 0;

	case IPC_STAT:
		if (error =
		    ipcaccess(&sp->sem_perm, SEM_R, IPC_MAC|IPC_DAC, u.u_cred))
			return error;
                MAC_ASSERT(sp,MAC_DOMINATES);
		
		bcopy((caddr_t)sp, (caddr_t)&ds, sizeof(ds));
		ds.sem_base = (struct sem *)NULL;
		ds.sem_perm.ipc_secp = (struct ipc_sec *)NULL;

		if (copyout((caddr_t)&ds, (caddr_t)uap->arg, sizeof(ds)))
			return EFAULT;
		return 0;

	/* Get # of processes sleeping for greater semval. */
	case GETNCNT:
		if (error =
		    ipcaccess(&sp->sem_perm, SEM_R, IPC_MAC|IPC_DAC, u.u_cred))
			return error;
                MAC_ASSERT(sp,MAC_DOMINATES);
		if (uap->semnum >= sp->sem_nsems)
			return EINVAL;
		rvp->r_val1 = (sp->sem_base + uap->semnum)->semncnt;
		return 0;

	/* Get pid of last process to operate on semaphore. */
	case GETPID:
		if (error =
		    ipcaccess(&sp->sem_perm, SEM_R, IPC_MAC|IPC_DAC, u.u_cred))
			return error;
                MAC_ASSERT(sp,MAC_DOMINATES);
		if (uap->semnum >= sp->sem_nsems)
			return EINVAL;
		rvp->r_val1 = (sp->sem_base + uap->semnum)->sempid;
		return 0;

	/* Get semval of one semaphore. */
	case GETVAL:
		if (error =
		    ipcaccess(&sp->sem_perm, SEM_R, IPC_MAC|IPC_DAC, u.u_cred))
			return error;
                MAC_ASSERT(sp,MAC_DOMINATES);
		if (uap->semnum >= sp->sem_nsems)
			return EINVAL;
		rvp->r_val1 = (sp->sem_base + uap->semnum)->semval;
		return 0;

	/* Get all semvals in set. */
	case GETALL:
		if (error =
		    ipcaccess(&sp->sem_perm, SEM_R, IPC_MAC|IPC_DAC, u.u_cred))
			return error;
                MAC_ASSERT(sp,MAC_DOMINATES);
		base = (caddr_t)uap->arg;
		for (i = sp->sem_nsems, p = sp->sem_base; i--; p++) {
			if (copyout((caddr_t)&p->semval, base,
			  sizeof(p->semval)))
				return EFAULT;
			base += sizeof(p->semval);
		}
		return 0;

	/* Get # of processes sleeping for semval to become zero. */
	case GETZCNT:
		if (error =
		    ipcaccess(&sp->sem_perm, SEM_R, IPC_MAC|IPC_DAC, u.u_cred))
			return error;
                MAC_ASSERT(sp,MAC_DOMINATES);
		if (uap->semnum >= sp->sem_nsems)
			return EINVAL;
		rvp->r_val1 = (sp->sem_base + uap->semnum)->semzcnt;
		return 0;

	/* Set semval of one semaphore. */
	case SETVAL:
		if (error =
		    ipcaccess(&sp->sem_perm, SEM_A, IPC_MAC|IPC_DAC, u.u_cred))
			return error;
                MAC_ASSERT(sp,MAC_SAME);
		if (uap->semnum >= sp->sem_nsems)
			return EINVAL;
		if ((unsigned)uap->arg > seminfo.semvmx)
			return ERANGE;
		p = sp->sem_base + uap->semnum;
		if ((p->semval = uap->arg) != 0) {
			if (p->semncnt) {
				p->semncnt = 0;
				wakeprocs((caddr_t)&p->semncnt, PRMPT);
			}
		} else if (p->semzcnt) {
			p->semzcnt = 0;
			wakeprocs((caddr_t)&p->semzcnt, PRMPT);
		}
		p->sempid = u.u_procp->p_pid;
		semunrm(uap->semid, uap->semnum, uap->semnum);
		return 0;

	/* Set semvals of all semaphores in set. */
	case SETALL:
		if (error =
		    ipcaccess(&sp->sem_perm, SEM_A, IPC_MAC|IPC_DAC, u.u_cred))
			return error;
                MAC_ASSERT(sp,MAC_SAME);
		if ((semvals = (ushort *)
			kmem_alloc(sizeof(ushort) * sp->sem_nsems, KM_SLEEP))
		== (ushort *)NULL)
			return ENOMEM;
		if (copyin((caddr_t)uap->arg, (caddr_t)semvals,
		  sizeof(ushort) * sp->sem_nsems)) {
			error = EFAULT;
			goto out;
		}
		for (i = 0; i < sp->sem_nsems;)
			if (semvals[i++] > (unsigned)seminfo.semvmx) {
				error = ERANGE;
				goto out;
			}
		semunrm(uap->semid, 0, sp->sem_nsems);
		for (i = 0, p = sp->sem_base; i < sp->sem_nsems;
		  (p++)->sempid = u.u_procp->p_pid) {
			if ((p->semval = semvals[i++]) != 0) {
				if (p->semncnt) {
					p->semncnt = 0;
					wakeprocs((caddr_t)&p->semncnt, PRMPT);
				}
			} else if (p->semzcnt) {
				p->semzcnt = 0;
				wakeprocs((caddr_t)&p->semzcnt, PRMPT);
			}
		}
out:
		kmem_free(semvals, sizeof(ushort) * sp->sem_nsems);
		return error;
	default:
		return EINVAL;
	}

	/* NOTREACHED */
}

/*
 * semexit - Called by exit() to clean up on process exit.
 */
void
semexit()
{
	register struct sem_undo	*up;	/* process undo struct ptr */
	register struct	sem_undo	*p;	/* undo struct ptr */
	struct semid_ds			*sp;	/* semid being undone ptr */
	register int			i;	/* loop control */
	register long			v;	/* adjusted value */
	register struct sem		*semp;	/* semaphore ptr */

	if ((up = sem_undo[u.u_procp->p_slot]) == NULL)
		return;
	if (up->un_cnt == 0)
		goto cleanup;
	for (i = up->un_cnt; i--; ) {
		if (semconv(up->un_ent[i].un_id, &sp) != 0)
			continue;
		v = (long)(semp = sp->sem_base + up->un_ent[i].un_num)->semval +
		  up->un_ent[i].un_aoe;
		if (v < 0 || v > seminfo.semvmx)
			continue;
		semp->semval = (ushort) v;
		if (v == 0 && semp->semzcnt) {
			semp->semzcnt = 0;
			wakeprocs((caddr_t)&semp->semzcnt, PRMPT);
		}
		if (up->un_ent[i].un_aoe > 0 && semp->semncnt) {
			semp->semncnt = 0;
			wakeprocs((caddr_t)&semp->semncnt, PRMPT);
		}
	}
	up->un_cnt = 0;
	if (semunp == up)
		semunp = up->un_np;
	else
		for (p = semunp;p != NULL;p = p->un_np)
			if (p->un_np == up) {
				p->un_np = up->un_np;
				break;
			}
cleanup:
	up->un_np = semfup;
	semfup = up;
	sem_undo[u.u_procp->p_slot] = NULL;
}

/*
 * semget - Semget system call.
 */
STATIC int
semget(uap, rvp)
	register struct semgeta *uap;
	rval_t *rvp;
{
	struct semid_ds	*sp;			/* semaphore header ptr */
	register ulong	i;			/* temp */
	int		s;			/* ipcget status return */
	register int	error;

	if (error = ipcget(uap->key, uap->semflg, (struct ipc_perm *)sema,
	  seminfo.semmni, sizeof(*sp), &s, (struct ipc_perm **)&sp))
		return error;

        /* 
         * Check if the Object Level Audit Criteria
         * pertains to this event and lid.
         */
	(void)adt_auditme(A_IPCCHK,ADT_SEM_GET,sp->sem_perm.ipc_secp->ipc_lid);

        MAC_ASSERT(sp,MAC_SAME);
	if (s) {
		/* This is a new semaphore set.  Finish initialization. */
		if (uap->nsems <= 0 || uap->nsems > seminfo.semmsl) {
			sp->sem_perm.mode = 0;
			return EINVAL;
		}
		if ((i = rmalloc(semmap, (long)uap->nsems)) == NULL) {
			cc_limiter(&cc_re_sem, u.u_cred);
			sp->sem_perm.mode = 0;
			return ENOSPC;
		}
		sp->sem_base = sem + (i - 1);
		sp->sem_nsems = uap->nsems;
		sp->sem_ctime = hrestime.tv_sec;
		sp->sem_otime = 0;
		{
			/* init reserve area */
		int i;
			for(i=0;i<IPC_PERM_PAD;i++)
				sp->sem_perm.pad[i] = 0;
			sp->sem_pad1 = 0;
			sp->sem_pad2 = 0;
			for(i=0;i<SEM_PAD;i++)
				sp->sem_pad3[i] = 0;
		}
				
	} else if (uap->nsems && sp->sem_nsems < (unsigned)uap->nsems)
		return EINVAL;

	rvp->r_val1 = sp->sem_perm.seq * seminfo.semmni + (sp - sema);
	return 0;
}

/*
 * seminit - Called by main() to initialize the semaphore map.
 */
void
seminit()
{
	register i;
	register struct semid_ds        *sp;
	register struct ipc_sec         *ipc_secp;  /* ptr to sec structs */

	mapinit(semmap, seminfo.semmap);
	rmfree(semmap, (long)seminfo.semmns, 1L);

	semfup = semu;
	for (i = 0; i < seminfo.semmnu - 1; i++) {
		semfup->un_np =
		  (struct sem_undo *)((uint)semfup+seminfo.semusz);
		semfup = semfup->un_np;
	}
	semfup->un_np = NULL;
	semfup = semu;

	/* initialize DAC entries in semaphore sets */
	if ((ipc_secp = (struct ipc_sec *)
		kmem_zalloc(seminfo.semmni*sizeof(struct ipc_sec), KM_NOSLEEP))
	==  (struct ipc_sec *)NULL)
		cmn_err(CE_PANIC, "Can't allocate sem security struct.\n");

	for (i = seminfo.semmni, sp = sema; i > 0; i--, sp++, ipc_secp++)
		sp->sem_perm.ipc_secp = ipc_secp;
}

/*
 * semop - Semop system call.
 */
STATIC int
semop(uap, rvp)
	register struct semopa *uap;
	rval_t *rvp;
{
	register struct sembuf		*op;	/* ptr to operation */
	register int			i;	/* loop control */
	struct semid_ds			*sp;	/* ptr to associated header */
	register struct sem		*semp;	/* ptr to semaphore */
	int again, error;
	struct sembuf			*semops;/* temporary */

	sysinfo.sema++;			/* bump semaphore operation count */
	if (error = semconv(uap->semid, &sp))
		return error;

        /* 
         * Check if the Object Level Audit Criteria
         * pertains to this event and lid.
         */
	(void)adt_auditme(A_IPCCHK,ADT_SEM_OP,sp->sem_perm.ipc_secp->ipc_lid);

	if (uap->nsops > seminfo.semopm)
		return E2BIG;
	if ((semops = (struct sembuf *)
		kmem_alloc(uap->nsops * sizeof(*op), KM_SLEEP)) == NULL)
		return ENOMEM;
	if (copyin((caddr_t)uap->sops, (caddr_t)semops,
	  uap->nsops * sizeof(*op))) {
		error = EFAULT;
		goto out;
	}

	/* Verify that sem #s are in range and permissions are granted. */
	for (i = 0, op = semops; i++ < uap->nsops; op++) {
		/*
		 * Write MAC access is needed for read since a read
		 * operation modifies the object as well.
		 */
		if ((error = ipcaccess(&sp->sem_perm, op->sem_op ?
			SEM_A : SEM_R|SEM_A, IPC_MAC, u.u_cred))
		||  (error = ipcaccess(&sp->sem_perm, op->sem_op ?
			SEM_A : SEM_R, IPC_DAC, u.u_cred)))
			goto out;
		if (op->sem_num >= sp->sem_nsems) {
			error = EFBIG;
			goto out;
		}
	}
        MAC_ASSERT(sp,MAC_SAME);
	again = 0;
check:
	/*
	 * Loop waiting for the operations to be satisfied atomically.
	 * Actually, do the operations and undo them if a wait is needed
	 * or an error is detected.
	 */
	if (again) {
		struct semid_ds *junk;

		/* Verify that the semaphores haven't been removed. */
		if (semconv(uap->semid, &junk) != 0) {
			error = EIDRM;
			goto out;
		}
		/* Copy in user operation list after sleep. */
		if (copyin((caddr_t)uap->sops, (caddr_t)semops,
		  uap->nsops * sizeof(*op))) {
			error = EFAULT;
			goto out;
		}
	}
	again = 1;

	for (i = 0, op = semops; i < uap->nsops; i++, op++) {
		semp = sp->sem_base + op->sem_num;
		if (op->sem_op > 0) {
			if (op->sem_op + (long)semp->semval > seminfo.semvmx
			  || (op->sem_flg & SEM_UNDO
			    && (error = semaoe(op->sem_op, uap->semid,
			      op->sem_num)))) {
				if (i)
					semundo(semops, i,
					  uap->semid, sp);
				if (error == 0)
					error = ERANGE;
				goto out;
			}
			semp->semval += op->sem_op;
			if (semp->semncnt) {
				semp->semncnt = 0;
				wakeprocs((caddr_t)&semp->semncnt, PRMPT);
			}
			if (semp->semzcnt && !semp->semval) {
				semp->semzcnt = 0;
				wakeprocs((caddr_t)&semp->semzcnt, PRMPT);
			}
			continue;
		}
		if (op->sem_op < 0) {
			if (semp->semval >= (unsigned)(-op->sem_op)) {
				if (op->sem_flg & SEM_UNDO
				  && (error = semaoe(op->sem_op, uap->semid,
				    op->sem_num))) {
					if (i)
						semundo(semops, i,
						  uap->semid, sp);
					goto out;
				}
				semp->semval += op->sem_op;
				if (semp->semzcnt && !semp->semval) {
					semp->semzcnt = 0;
					wakeprocs((caddr_t)&semp->semzcnt,
					    PRMPT);
				}
				continue;
			}
			if (i)
				semundo(semops, i, uap->semid, sp);
			if (op->sem_flg & IPC_NOWAIT) {
				error = EAGAIN;
				goto out;
			}
			semp->semncnt++;
			if (sleep((caddr_t)&semp->semncnt, PCATCH|PSEMN)) {
				if ((semp->semncnt)-- <= 1) {
					semp->semncnt = 0;
					wakeprocs((caddr_t)&semp->semncnt,
					    PRMPT);
				}
				error = EINTR;
				goto out;
			}
			goto check;
		}
		if (semp->semval) {
			if (i)
				semundo(semops, i, uap->semid, sp);
			if (op->sem_flg & IPC_NOWAIT) {
				error = EAGAIN;
				goto out;
			}
			semp->semzcnt++;
			if (sleep((caddr_t)&semp->semzcnt, PCATCH|PSEMZ)) {
				if ((semp->semzcnt)-- <= 1) {
					semp->semzcnt = 0;
					wakeprocs((caddr_t)&semp->semzcnt,
					    PRMPT);
				}
				error = EINTR;
				goto out;
			}
			goto check;
		}
	}

	/* All operations succeeded.  Update sempid for accessed semaphores. */
	for (i = 0, op = semops; i++ < uap->nsops;
	  (sp->sem_base + (op++)->sem_num)->sempid = u.u_procp->p_pid)
		;
	sp->sem_otime = hrestime.tv_sec;
	rvp->r_val1 = 0;
out:
	kmem_free(semops, uap->nsops * sizeof(*op));
	return error;
}

/*
 * semsys - System entry point for semctl, semget, and semop system calls.
 */
int
semsys(uap, rvp)
	register struct semsysa *uap;
	rval_t *rvp;
{
	register int error;

	switch (uap->opcode) {
	case SEMCTL:
		error = semctl((struct semctla *)uap, rvp);
		break;
	case SEMGET:
		error = semget((struct semgeta *)uap, rvp);
		break;
	case SEMOP:
		error = semop((struct semopa *)uap, rvp);
		break;
	default:
		error = EINVAL;
		break;
	}

	return error;
}
