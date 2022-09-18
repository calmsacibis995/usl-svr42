/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/acct.c	1.6.3.3"
#ident	"$Header: $"

#include <util/types.h>
#include <acc/priv/privilege.h>
#include <util/param.h>
#include <svc/systm.h>
#include <proc/acct.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <util/debug.h>
#include <svc/resource.h>
#include <io/uio.h>

STATIC struct acct	acctbuf;
STATIC struct vnode	*acctvp;
STATIC int		aclock;

#define ACLOCKED        0x01
#define ACWANT          0x02

#define ACLOCK() { \
        while (aclock & ACLOCKED) { \
                aclock |= ACWANT; \
                sleep((caddr_t)&aclock, PZERO); \
        } \
        aclock |= ACLOCKED; \
}

#define ACUNLOCK() { \
        ASSERT(aclock & ACLOCKED); \
        aclock &= ~ACLOCKED; \
        if (aclock & ACWANT) { \
                aclock &= ~ACWANT; \
                wakeprocs((caddr_t)&aclock, PRMPT); \
        } \
}

/*
 * Perform process accounting functions.
 */

struct accta {
	char	*fname;
};

/* ARGSUSED */
int
sysacct(uap, rvp)
	register struct accta *uap;
	rval_t *rvp;
{
	struct vnode *vp;
	int error = 0;

	if (pm_denied(u.u_cred, P_SYSOPS))
		return EPERM;
	ACLOCK();
	if (uap->fname == NULL) {
		if (acctvp) {
			if (error = VOP_CLOSE(acctvp, FWRITE, 1, 0, u.u_cred))
				goto out;
			VN_RELE(acctvp);
			acctvp = NULL;
		}
	} else {
		if (error = vn_open(uap->fname, UIO_USERSPACE, FWRITE,
		  0, &vp, (enum create)0)) {
			/* SVID  compliance */
			if (error == EISDIR)
				error = EACCES;
			goto out;
		}
		if (acctvp && VN_CMP(acctvp, vp)) {
			error = EBUSY;
			goto closevp;
		}
		if (vp->v_type != VREG) {
			error = EACCES;
			goto closevp;
		}
		if (acctvp) {
			if (error = VOP_CLOSE(acctvp, FWRITE, 1, 0, u.u_cred))
				goto closevp;
			VN_RELE(acctvp);
		}
		acctvp = vp;
	}
	goto out;
closevp:
	(void)VOP_CLOSE(vp, FWRITE, 1, 0, u.u_cred);
	VN_RELE(vp);
out:
	ACUNLOCK();
	return error;
}

/*
 * Produce a pseudo-floating point representation
 * with 3 bits base-8 exponent, 13 bits fraction.
 */

STATIC int
compress(t)
	register time_t t;
{
	register exp = 0, round = 0;

	while (t >= 8192) {
		exp++;
		round = t&04;
		t >>= 3;
	}
	if (round) {
		t++;
		if (t >= 8192) {
			t >>= 3;
			exp++;
		}
	}
	return (exp<<13) + t;
}

/*
 * On exit, write a record on the accounting file.
 */

void
#ifdef __STDC__
acct(char st)
#else
acct(st)
	char st;
#endif
{
	register struct vnode *vp;

	ACLOCK();
	if ((vp = acctvp) == NULL) {
		ACUNLOCK();
		return;
	}
	bcopy(u.u_comm, acctbuf.ac_comm, sizeof(acctbuf.ac_comm));
	acctbuf.ac_btime = u.u_start;
	acctbuf.ac_utime = compress(u.u_procp->p_utime);
	acctbuf.ac_stime = compress(u.u_procp->p_stime);
	acctbuf.ac_etime = compress(lbolt - u.u_ticks);
	acctbuf.ac_mem = compress(u.u_mem);
	acctbuf.ac_io = compress(u.u_ioch);
	acctbuf.ac_rw = compress(u.u_ior+u.u_iow);
	acctbuf.ac_uid = u.u_cred->cr_ruid;
	acctbuf.ac_gid = u.u_cred->cr_rgid;
	acctbuf.ac_tty = cttydev(u.u_procp);
	/* XENIX Support */
	if (VIRTUAL_XOUT)
		acctbuf.ac_stat = (char)(st >> 8);
	else
		acctbuf.ac_stat = st;
	/* End XENIX Support */
	acctbuf.ac_flag = (u.u_acflag | AEXPND);
	(void) vn_rdwr(UIO_WRITE, vp, (caddr_t) &acctbuf,
	  sizeof(acctbuf), 0, UIO_SYSSPACE, IO_APPEND,
	  RLIM_INFINITY, u.u_cred, (int *)NULL);
	ACUNLOCK();
}

/*
 * As a result of making acctvp STATIC,
 * other modules (e.g.,AUDIT) need a 
 * functional interface to determine
 * when the process accounting is enabled.
 */
int
accton()
{
 	return(acctvp!=NULL);
}
