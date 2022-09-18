/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:acc/priv/sum/sum.c	1.24.3.8"
#ident	"$Header: $"

#include <acc/audit/audit.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/acct.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/secsys.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define DEV  		0x1
#define FS 		0x2
#define FILE 		0x4

STATIC	void		pm_rmprivfl();
STATIC	void		pm_clrentry();
STATIC	void		pm_insprivfl();

STATIC	lpktab_t	*pm_mkprivdev();
STATIC	lpdtab_t	*pm_mkprivfs();
STATIC	lpftab_t	*pm_mkprivfl();
STATIC	lpdtab_t	*pm_rmprivfs();
STATIC	lpktab_t	*pm_rmprivdev();

STATIC	int		pm_count();
STATIC	int		pm_getprid();
STATIC	int		pm_setprid();
STATIC	int		pm_getprivfl();
STATIC	int		pm_ckallowed();

extern	uid_t	privid;

KSTATIC lpktab_t	*pm_lpktab;	/* anchor for kernel privilege table */

/*
 * This routine determines process privilege based on
 * whether or not the privilege requested is contained
 * in the working privilege set for the current process.
 */
int
pm_denied(crp, priv)
	cred_t	*crp;
	int	priv;
{
	register int ret = 0;

	ASSERT(crp != NULL);

	if (!pm_privon(crp, pm_privbit(priv))) 
		ret = EPERM;

	ADT_PRIV(priv, ret, crp);	/* audit the "use of privilege" */

	if (!ret)
		acctevt(ASU);
	return ret;
}


/*
 * This routine is called by the procpriv() system call to set, get,
 * clear, count, or put (absolute set) privileges for the current
 * process.  There are certain rules followed for the set, clear, and
 * put routines.  These rules are explained in the individual cases
 * for each command.
 *
 * In any case, the working privilege set is always a subset of the
 * maximum privilege set.
 */
int
pm_process(cmd, rvp, crp, privp, count, newcrp)
	int cmd;
	rval_t *rvp;
	cred_t *crp;
	priv_t *privp;
	int count;
	cred_t **newcrp;
{
	enum pprivset { MAX_PSET, WKG_PSET, NP_PSET };
	register const privmax =  NPRIVS * NP_PSET;
	pvec_t	tsav,
		vectors[NP_PSET],
		pvec_buf[NPRIVS * NP_PSET];
	register int cnt;

	rvp->r_val1 = 0;

	vectors[MAX_PSET] = 0;
	vectors[WKG_PSET] = 0;

	tsav = crp->cr_savpriv;		/* initial value - may be changed later */

	*newcrp = (cred_t *)NULL;

	switch (cmd) {
	case SETPRV:		/* set the process privileges */
	case PUTPRV:		/* set (absolutely) the process privileges */
	case CLRPRV:		/* clear process privileges */
		if (count < 0 || count > privmax) 
			return EINVAL;
		if (count > 0) {
			if (copyin((caddr_t)privp, (caddr_t)pvec_buf,
			   (count * sizeof(priv_t)))) 
				return EFAULT;
			if (pm_getprid("mw", pvec_buf, count, vectors)) 
				return EINVAL;
		}
		switch (cmd) {		/* secondary switch for separate work */
		case SETPRV:		/* set the process privileges */
			vectors[MAX_PSET] = crp->cr_maxpriv;	/* used later */
			/*
			 * first union the new working set with the current
			 * working set.
			 */
			vectors[WKG_PSET] |= crp->cr_wkgpriv;
			break;
		case CLRPRV:		/* clear process privileges */
			/*
			 * intersect the inverse of the maximum set to clear
			 * with the current maximum set and assign it to
			 * the new maximum set.
			 */
			vectors[MAX_PSET] = (~vectors[MAX_PSET]
							& crp->cr_maxpriv);
			/*
			 * Make sure that the saved privilege set is
			 * always a subset of any new maximum set.
			 *
			 * The saved privilege set is the union of all
			 * privileges acquired from any fixed privileges
			 * on files.
			 */
			tsav = (crp->cr_savpriv & vectors[MAX_PSET]);
			/*
			 * intersect the inverse of new working set to clear
			 * with the current working set and assign it to
			 * the new working set.
			 */
			vectors[WKG_PSET] = (~vectors[WKG_PSET]
							& crp->cr_wkgpriv);
			break;
		case PUTPRV:		/* absolute process privilege setting */
			/*
			 * intersect the new maximum set with the current
			 * maximum set.
			 */
			vectors[MAX_PSET] &= crp->cr_maxpriv;
			/*
			 * Make sure that the saved privilege set is
			 * always a subset of any new maximum set.
			 */
			tsav = (crp->cr_savpriv & vectors[MAX_PSET]);
			break;
		}	/* end secondary switch */

		/*
		 * make sure that the working set is always a subset
		 * of the maximum set.
		 */
		vectors[WKG_PSET] &= vectors[MAX_PSET];

		/*
		 * If the resulting privilege set is not equal to the
		 * privilege set in the cred structure, duplicate the
		 * cred structure and check in the procpriv system
		 * call if the cred structure was duplicated.  If so,
		 * reassign the uarea cred structure there, not here!
		 */
		if ((vectors[WKG_PSET] != crp->cr_wkgpriv) ||
		    (vectors[MAX_PSET] != crp->cr_maxpriv)) {
			crp = crdup(crp);
			crp->cr_maxpriv = vectors[MAX_PSET];
			crp->cr_wkgpriv = vectors[WKG_PSET];
			crp->cr_savpriv = tsav;
			*newcrp = crp;
		}
		rvp->r_val1 = pm_count("mw", vectors);
		break;
	case CNTPRV:			/* count the process privileges */
		vectors[MAX_PSET] = crp->cr_maxpriv;
		vectors[WKG_PSET] = crp->cr_wkgpriv;
		rvp->r_val1 = pm_count("mw", vectors);
		break;
	case GETPRV:			/* get the process privileges */
		if (count < 0 || count > privmax) 
			return EINVAL;
		vectors[MAX_PSET] = crp->cr_maxpriv;
		vectors[WKG_PSET] = crp->cr_wkgpriv;
		cnt = pm_setprid("mw", pvec_buf, vectors);
		if (cnt) {		/* something to "copyout" */
			if (cnt > count) 
				return EINVAL;
			if (copyout((caddr_t)pvec_buf,
			   (caddr_t)privp, (sizeof(priv_t) * cnt))) 
				return EFAULT;
		}
		rvp->r_val1 = cnt;
		break;
	default:
		return EINVAL;
		break;
	}	/* end of "cmd" switch */
	return 0;
}


/*
 * This routine is the "guts" of the privilege mechanism.
 * It is where the privileges for the new process are calculated.
 * It is called only when a new process is exec'ed.  If the
 * calculated privileges differ, it creates a duplicate copy of
 * the credential structure with the new privileges and returns
 * them to the kernel exec code.
 */
cred_t	*
pm_calcpriv(vp, vap, crp, lvl)
	vnode_t	*vp;
	vattr_t	*vap;
	cred_t	*crp;
	int	lvl;
{
	pvec_t	f_privs = 0;
	cred_t	*newcrp = crp;
	pvec_t	tmax = crp->cr_maxpriv;
	pvec_t	twkg = crp->cr_savpriv;
	pvec_t	tsav = crp->cr_savpriv;

	if (lvl > 0)		/* Only do this once for each exec */
		return newcrp;

	/*
	 * The following check is performed as a special case in
	 * exec because a process that is executing with effective
	 * UID equal to privid must be able to propagate privilege
	 * at exec time.
	 */
	if (crp->cr_uid == privid)
		twkg = crp->cr_maxpriv;
	/*
	 * If the setuid-on-exec bit is on for the file being exec'ed
	 * check the file owner UID.  If it's equal to the privid, turn
	 * all privileges in the temporary maximum and working privilege
	 * sets on.
	 *
	 * If the file owner UID is not equal to privid but the current
	 * effective UID is the privid, set the temporary working set
	 * to the current saved privilege set.  Otherwise, set the
	 * temporary maximum privilege set to the saved privilege
	 * set.  This is for compatibility with older versions of the
	 * UNIX(TM) Operating System.
	 */
	if (vap->va_mode & VSUID) {
		if (vap->va_uid == privid) {
			pm_setbits(P_ALLPRIVS, tmax);
			twkg = tmax;
		}
		else {
			if (crp->cr_uid == privid)
				twkg = crp->cr_savpriv;
			else
				tmax = crp->cr_savpriv;
		}
	}
	/*
	 * Regardless of what happened previously, check if the file
	 * being exec'ed has any fixed privileges.  If it does, union
	 * the fixed privileges with the privileges in the temporary
	 * privilege sets.
	 */
	if (pm_getprivfl(&f_privs, vp, vap)) {
		tmax |= f_privs;
		twkg |= f_privs;
		tsav |= f_privs;
	}
	/*
	 * Duplicate the credential structure and modify the privilege
	 * sets for the new process if any of the current processes
	 * privilege sets differ from the temporary privilege sets
	 * manipulated during this routine.
	 */
	if (crp->cr_maxpriv != tmax || crp->cr_wkgpriv != twkg
		|| crp->cr_savpriv != tsav) {

		newcrp = crdup(crp);
		newcrp->cr_maxpriv = tmax;	/* upper bound */
		newcrp->cr_wkgpriv = twkg;	/* effective privileges */
		newcrp->cr_savpriv = tsav;	/* see NOTE in pm_recalc() */
	}
	return newcrp;
}


/*
 * This routine is called by the filepriv() and link() system calls.  It
 * is used to set, get, or count privileges associated with executable files.
 * When setting privileges, the new privilege set must be a subset of the
 * current processes maximum privileges.
 */
int
pm_file(cmd, vp, vap, rvp, crp, privp, count)
	int	cmd;
	vnode_t	*vp;
	vattr_t	*vap;
	rval_t	*rvp;
	cred_t	*crp;
	priv_t	*privp;
	int	count;
{
	pvec_t	fixed,
		pvec_buf[NPRIVS];
	register int	cnt = 0,
			error = 0,
			privmax = NPRIVS;

	fixed = 0;
	rvp->r_val1 = 0;

	if (cmd != CNTPRV) {
		if (count < 0 || count > privmax) 
			return EINVAL;
	}

	switch (cmd) {
	case PUTPRV:		/* absolute file privilege setting */
		if (error = pm_ckallowed(vp, VWRITE, cmd, vap, crp)) 
			return error;	/* failed for privilege or access */
		if (count > 0) {
			if (copyin((caddr_t)privp, (caddr_t)pvec_buf,
			   (count * sizeof(priv_t)))) 
				return EFAULT;
			if (error = pm_getprid("f", pvec_buf, count, &fixed)) 
				return error;
		}
		/*
		 * make sure that the privilege set passed is
		 * subset of the processes maximum privileges.
		*/
		fixed &= crp->cr_maxpriv;

		if (!fixed) 
			/*
			 * there are no privileges to set. If the file
			 * is in the kernel privilege table, remove it!
			*/
			pm_rmprivfl(vp, vap);
		/*
		 * set the privileges on the named file.
		*/
		else {
			pm_insprivfl(vp, vap, fixed);
			rvp->r_val1 = pm_count("f", &fixed);
		}
		break;
	case GETPRV:		/* get the file privileges */
		if (error = pm_ckallowed(vp, VREAD, cmd, vap, crp)) 
			return error;	/* failed access "read" check */
		if (pm_getprivfl(&fixed, vp, vap)) {
			cnt = pm_setprid("f", pvec_buf, &fixed);
			if (cnt > count) 
				return EINVAL;
			if (cnt) {		/* something to "copyout" */
				if (copyout((caddr_t)pvec_buf, (caddr_t)privp,
				   (sizeof(priv_t) * cnt))) 
					return EFAULT;	/* "copyout" failed */
			}
		}
		rvp->r_val1 = cnt;
		break;
	case CNTPRV:			/* count number of privileges associated
					   with the named file */
		if (error = pm_ckallowed(vp, VREAD, cmd, vap, crp)) 
			return error;	/* failed access "read" check */
		if (pm_getprivfl(&fixed, vp, vap)) 
			rvp->r_val1 = pm_count("f", &fixed);
		break;
	default:
		error = EINVAL;
	}	/* end of "cmd" switch */
	return error;
}


/*
 * This routine should only be called ONCE from the main routine of the
 * kernel.
 */
void
pm_init()
{
	pm_lpktab = (lpktab_t *)0;	/* set kernel priv table to NULL */

	/* set both the maximum and working privilege sets for process 0 */

	pm_setbits(P_ALLPRIVS, sys_cred->cr_maxpriv);
	pm_setbits(P_ALLPRIVS, sys_cred->cr_wkgpriv);

}


/*
 * This routine removes an entry (if found) from the privilege
 * list along with all files contained on that device that have
 * privilege associated with them whenever the filesystem is
 * unmounted.
 */
void
pm_clrdev(vfp)
	vfs_t	*vfp;
{
	lpktab_t	*mdev, *omdev;

	if (!pm_lpktab)			/* Privilege table not initialized */
		return;

	omdev = (lpktab_t	*)0;
	for(mdev = pm_lpktab; mdev; mdev = mdev->lpk_next) {
		if (mdev->lpk_dev == vfp->vfs_dev) {
			if (!omdev)
				(void) pm_rmprivdev(mdev);
			else
				omdev->lpk_next = pm_rmprivdev(mdev);
			return;
		}
		omdev = mdev;
	}
	return;
}


/*
 * Used to insert a new entry in the kernel privilege table with the
 * value of pvecp passed to pm_file() from the filepriv() system call.
 */
STATIC	void
pm_insprivfl(vp, vap, f_privs)
	vnode_t	*vp;
	vattr_t	*vap;
	pvec_t	f_privs;
{
	lpktab_t	*mdev,
			*tmdev;		/* ptrs to mounted device list */
	lpdtab_t	*fs,
			*tfs;		/* ptrs to file system list */
	lpftab_t	*file,
			*tfile,
			**prefile;	/* ptrs to file list */
	u_char		flag = 0,
			found = 0;

	tfs = (lpdtab_t *)NULL;
	tmdev = (lpktab_t *)NULL;
	tfile = (lpftab_t *)NULL;

	do {
		/*
	 	 * Find the right list to search.
		*/
		for (mdev = pm_lpktab; mdev && (mdev->lpk_dev != vp->v_vfsp->vfs_dev);
		     mdev = mdev->lpk_next)
			;
		if (!mdev) {
			/* Allocate the necessary data structure */
			if (!tmdev) {
				tmdev = pm_mkprivdev(vp);
				if (!tfs)
					tfs = pm_mkprivfs(vap);
				if (!tfile)
					tfile = pm_mkprivfl(vap);
				continue;	/* do while loop */
			}
			tmdev->lpk_next = pm_lpktab;
			pm_lpktab = tmdev;
			mdev = tmdev;		/* set to head of device list */
			tmdev = (lpktab_t *)NULL;
		}
		else if (tmdev) 
			flag |= DEV;
	
		for (fs = mdev->lpk_list; fs && (fs->lpd_fsid != vap->va_fsid);
		     fs = fs->lpd_next)
			;
		if (!fs) {
			if (!tfs) {
				tfs = pm_mkprivfs(vap);
				if (!tfile)
					tfile = pm_mkprivfl(vap);
				continue;	/* do while loop */
			}
			tfs->lpd_next = mdev->lpk_list;
			mdev->lpk_list = tfs;
			fs = tfs;	/* set to head of file system list */
			tfs = (lpdtab_t *)NULL;
		}
		else if (tfs) 
			flag |= FS;
	
		/*
		 * search the list.
		*/
	
		file = fs->lpd_list;
		prefile = &fs->lpd_list;
	
		while (file) {
			if (file->lpf_nodeid > vap->va_nodeid) 
				break;
			else if (file->lpf_nodeid == vap->va_nodeid) {
				found = 1;
				break;
			}
			prefile = &file->lpf_next;
			file = file->lpf_next;
		}
		if (!found) {
			if (!tfile) {
				tfile = pm_mkprivfl(vap);
				continue;	/* do while loop */
			}
			found = 1;
			*prefile = tfile;
			tfile->lpf_next = file;
			tfile->lpf_fixpriv = f_privs;
		}
		else {			/* file was already in the table */
			if (tfile) 
				flag |= FILE;
			file->lpf_fixpriv = f_privs;
			file->lpf_validity = vap->va_ctime.tv_sec;
		}
	} while (!found);

	if (flag & FILE) 
		kmem_free(tfile, sizeof (lpftab_t));
	if (flag & FS) 
		kmem_free(tfs, sizeof (lpdtab_t));
	if (flag & DEV) 
		kmem_free(tmdev, sizeof (lpktab_t));
	return;
}


/*
 * Remove a file entry (and possibly its header node) based on nodeid and
 * fsid.
 */
STATIC	void
pm_rmprivfl(vp, vap)
	vnode_t	*vp;
	vattr_t	*vap;
{
	lpktab_t	*mdev, *omdev;
	lpdtab_t	*fs, *ofs;
	lpftab_t	*file, *ofile;

	if (!pm_lpktab)			/* Privilege table not initialized */
		return;
	omdev = (lpktab_t *)0;
	for(mdev = pm_lpktab; mdev && (mdev->lpk_dev != vp->v_vfsp->vfs_dev); mdev = mdev->lpk_next)
		omdev = mdev;
	if (!mdev)			/* No such device */
		return;
	ofs = (lpdtab_t *)0;
	for(fs = mdev->lpk_list; fs && (fs->lpd_fsid != vap->va_fsid); fs = fs->lpd_next)
		ofs = fs;
	if (!fs)			/* No such file system */
		return;
	ofile = (lpftab_t *)0;
	for(file = fs->lpd_list; file && (file->lpf_nodeid != vap->va_nodeid); file = file->lpf_next)
		ofile = file;
	if (!file)			/* No such file entry */
		return;
	pm_clrentry(mdev, omdev, fs, ofs, file, ofile);	/* remove the entry */
}


/*
 * Allocate and populate a file system list header.
 */
STATIC	lpdtab_t	*
pm_mkprivfs(vap)
	vattr_t	*vap;
{
	lpdtab_t	*ret;

	ret = (lpdtab_t *)kmem_zalloc(sizeof(lpdtab_t), KM_SLEEP);
	if (!ret) {
		cmn_err(CE_WARN, "pm_mkprivfs - Out of kernel memory\n");
		return (lpdtab_t *)0;
	}
	ret->lpd_fsid = vap->va_fsid;

	return ret;
}


/*
 * Allocate and populate a device list header.
 */
STATIC	lpktab_t	*
pm_mkprivdev(vp)
	vnode_t	*vp;
{
	lpktab_t	*ret;

	ret = (lpktab_t *)kmem_zalloc(sizeof(lpktab_t), KM_SLEEP);
	if (!ret) {
		cmn_err(CE_WARN, "pm_mkprivdev - Out of kernel memory\n");
		return (lpktab_t *)0;
	}
	ret->lpk_dev = vp->v_vfsp->vfs_dev;

	return ret;
}


/*
 * Allocate and populate a privileged file table entry.
 */
STATIC	lpftab_t	*
pm_mkprivfl(vap)
	vattr_t	*vap;
{
	lpftab_t	*ret;

	ret = (lpftab_t *)kmem_zalloc(sizeof(lpftab_t), KM_SLEEP);
	if (!ret) {
		cmn_err(CE_WARN, "pm_mkprivfl - Out of kernel memory\n");
		return (lpftab_t *)0;
	}
	ret->lpf_nodeid = vap->va_nodeid;
	ret->lpf_validity = vap->va_ctime.tv_sec;

	return ret;
}


/*
 * Locate the requested file in the kernel privilege table.
 * If the file is found, return 1. Otherwise, return 0.
 */
STATIC	int
pm_getprivfl(f_privs, vp, vap)
	pvec_t	*f_privs;
	vnode_t	*vp;
	vattr_t	*vap;
{
	lpktab_t	*mdev;
	lpdtab_t	*fs;
	lpftab_t	*file;
	time_t		ltime;

	/*
	 * set the time variable used to check validity to
	 * the value contained in the current attributes.
	*/

	ltime = vap->va_ctime.tv_sec;

	/*
	 * Find the right device list to search.
	*/
	for (mdev = pm_lpktab; mdev && (mdev->lpk_dev != vp->v_vfsp->vfs_dev);
	     mdev = mdev->lpk_next)
		;
	if (mdev) {
		/*
		 * search the file system list.
		*/
		for (fs = mdev->lpk_list; fs && (fs->lpd_fsid != vap->va_fsid);
		     fs = fs->lpd_next)
			;
		if (fs) {
			for (file = fs->lpd_list; file; file = file->lpf_next) {
				if (vap->va_nodeid == file->lpf_nodeid) {
					if (file->lpf_validity != ltime) {
						pm_rmprivfl(vp, vap);
						return 0;
					}
					*f_privs = file->lpf_fixpriv;
					return 1;
				}
				else if (file->lpf_nodeid > vap->va_nodeid) {
					return 0;
				}
			}
		}
	}
	return 0;
}


/*
 * This routine removes all the files found under a particular
 * file system entry.  It is intended to be called when ALL files
 * must be removed.
 */
STATIC	lpdtab_t	*
pm_rmprivfs(fs)
	lpdtab_t	*fs;
{
	lpftab_t	*file;

	/*
	 * lint complains about the following ``while'' statement
	 * but it's OK so leave it alone.
	*/
	while (file = fs->lpd_list) {
		fs->lpd_list = fs->lpd_list->lpf_next;
		kmem_free(file, sizeof(lpftab_t));
	}
	return fs->lpd_next;
}


/*
 * This routine removes all the files found under a particular
 * device entry.  It is intended to be called when ALL files
 * must be removed.
 */
STATIC	lpktab_t	*
pm_rmprivdev(mdev)
	lpktab_t	*mdev;
{
	lpdtab_t	*fs;
	lpktab_t	*omdev;

	/*
	 * lint complains about the following ``while'' statement
	 * but it's OK so leave it alone.
	*/
	while (fs = mdev->lpk_list) {
		mdev->lpk_list = pm_rmprivfs(mdev->lpk_list);
		kmem_free(fs, sizeof(lpdtab_t));
	}
	if (mdev == pm_lpktab)
		pm_lpktab = mdev->lpk_next;

	omdev = mdev->lpk_next;
	kmem_free(mdev, sizeof(lpktab_t));

	return omdev;
}

/*
 * This routine counts the number of bits set to 1 in the
 * argument passed.
 *
 * NOTE:	This routine will need to be modified if the
 *		number of privileges ever exceeds 32.
 */
STATIC	int
pm_count(fmt, vecs)
	register char	*fmt;
	register pvec_t	*vecs;
{
	register	int	j;
	register	int	cnt = 0;
	register	pvec_t	tmp;

	for (j = 0; *fmt; ++j) {
		tmp = vecs[j];
		while (tmp) {
			tmp = tmp & (tmp - 1);
			++cnt;
		}
		++fmt;
	}
	return cnt ;
}

/*
 * Scan the bufp argument and read the PRIDs contained in it.
 * Create privilege sets for the types of PRIDs being read
 * in.
 *
 * NOTE:	This routine will need to be modified if the
 *		number of privileges ever exceeds 32.
 */
STATIC	int
pm_getprid(fmt, bufp, count, vecs)
	register char	*fmt;
	priv_t	*bufp;
	int	count;
	pvec_t	*vecs;
{
	register	int	i, j, l;
	register	char	*f = fmt;

	l = 0;
	while (*f++ != '\0') ++l;

	for (i = 0; i < count; ++i) {
		for (j = 0; j < l; ++j) {
			if (pm_pridc(pm_type(bufp[i])) == fmt[j]) {
				if (pm_invalid(pm_pos(bufp[i])))
					/* invalid privilege */
					return EINVAL;
				else {
					pm_setbits(pm_pos(bufp[i]), vecs[j]);
					break;
				}
			}
		}
	}
	return 0;
}


/*
 * This routine clears an entry from the kernel privilege table.
 * If necessary, it can also remove the file system entry and re-links
 * the file system table.  It can also remove the device entry and
 * re-links the device table.
 */
STATIC	void
pm_clrentry(mdev, omdev, fs, ofs, file, ofile)
	lpktab_t	*mdev, *omdev;
	lpdtab_t	*fs, *ofs;
	lpftab_t	*file, *ofile;
{

	if (!ofile) {			/* File is first in list */
		ofile = fs->lpd_list;
		fs->lpd_list = ofile->lpf_next;
		kmem_free(ofile, sizeof(lpftab_t));
		if (!fs->lpd_list) {
			if (!ofs) {	/* File System is first one */
				ofs = mdev->lpk_list;
				mdev->lpk_list = ofs->lpd_next;
				kmem_free(ofs, sizeof(lpdtab_t));
				if (!mdev->lpk_list) {
					if (!omdev)	/* Device is first one */
						pm_lpktab = pm_lpktab->lpk_next;
					else		/* Not first device */
						omdev->lpk_next = mdev->lpk_next;
					kmem_free(mdev, sizeof(lpktab_t));
				}
			}
			else {		/* Not first file system in list */
				ofs->lpd_next = fs->lpd_next;
				kmem_free(fs, sizeof(lpdtab_t));
			}
		}
	}
	else {				/* Not first file in list */
		ofile->lpf_next = file->lpf_next;
		kmem_free(file, sizeof(lpftab_t));
	}
}


/*
 * This routine creates PRIDS based on the type argument and
 * the pvec_t passed.  These PRIDS are stored in a kernel buffer
 * and passed back to the user when calling filepriv() or procpriv()
 * with the GET command.
 *
 * NOTE:	This routine will need to be modified if the
 *		number of privileges ever exceeds 32.
 */
STATIC	int
pm_setprid(fmt, bufp, vecs)
	register char	*fmt;
	priv_t	*bufp;
	pvec_t	*vecs;
{
	register	int	i, j;
	register	int	cnt = 0;
	register	pvec_t	tmp;

	for (j = 0; *fmt; ++j) {
		tmp = vecs[j];
		for (i = 0; tmp; ++i) {
			if (tmp & 1) {
				bufp[cnt++] = i | pm_pridt(*fmt);
			}
			tmp >>= 1;
		}
		++fmt;
	}
	return cnt;
}


/*
 * This routine is used by the pm_file() routine to check for the
 * appropriate privilege and the appropriate access required.
 */
STATIC	int
pm_ckallowed(vp, mode, cmd, vap, crp)
	vnode_t	*vp;
	register int	mode;
	register int	cmd;
	vattr_t	*vap;
	cred_t	*crp;
{
	register	int	error = 0;

	switch (cmd) {
	case PUTPRV:
		if (!pm_denied(crp, P_SETSPRIV))
			return error;
		/*
		 * check if the process has the privilege to set file
		 * privileges on behalf of users.  If not, it's an error.
		*/
		if (pm_denied(crp, P_SETUPRIV) != 0) {
			return EPERM;
		}

		if ((error = VOP_ACCESS(vp, mode, 0, crp)) != 0)
			return error;		/* failed DAC check */
		/* FALLTHROUGH */
	case CNTPRV:
	case GETPRV:
		if ((error = MAC_VACCESS(vp, mode, crp)) != 0)
			return error;		/* failed MAC check */
		break;
	}
	return error;
}


/*
 * The pm_recalc() routine is required to maintain compatibility
 * in the SUM privilege mechanism with the super-user mechanism
 * where user ID 0 is considered privileged.  This routine is called
 * whenever the effective user ID for a process changes (currently,
 * only from setuid(), seteuid() and access()).
 *
 * NOTE:
 *
 *	The only privileges removed from the processes privilege
 *	sets are those that were acquired via the setuid mechanism.
 *	Any privileges acquired from the file (e.g., fixed privileges)
 *	are maintained.  The procpriv(2) system call is the interface
 *	that should be used to manipulate ALL of the process privileges.
 */
void
pm_recalc(crp)
	cred_t	*crp;
{
	/*
	 * If none of the uid fields in the credential structure
	 * are equal to the privid, set the working and maximum
	 * privilege sets to the saved privilege set.
	 */
	if (crp->cr_ruid != privid && crp->cr_suid != privid &&
	    crp->cr_uid != privid) {
		crp->cr_maxpriv = crp->cr_savpriv;
		crp->cr_wkgpriv = crp->cr_savpriv;
	}
	else if (crp->cr_uid == privid) /* effective UID was privid */
		crp->cr_wkgpriv = crp->cr_maxpriv;
	else 
		/*
		 * Since the new effective user ID isn't
		 * the privid, set the working privilege
		 * set to the saved privilege set.
		 */
		crp->cr_wkgpriv = crp->cr_savpriv;
}


/*
 * This routine is called by the secsys() system call.  It
 * is intended to return useful information regarding this
 * particular privilege mechanism type.
 */
int
pm_secsys(cmd, rvp, arg)
	int	cmd;
	rval_t	*rvp;
	caddr_t	arg;
{
	long	pm_flag = PM_ULVLINIT | PM_UIDBASE;

	register int	error = 0;

	static	const	setdef_t	sdefs[] = {
		{ PS_FIX,	NPRIVS,	"fixed",	PS_FILE_OTYPE },
		{ PS_MAX,	NPRIVS,	"max",		PS_PROC_OTYPE },
		{ PS_WKG,	NPRIVS,	"work",		PS_PROC_OTYPE },
	};

	rvp->r_val1 = 0;

	switch (cmd) {
	case ES_PRVSETCNT:	/* return the  count of the privilege */
				/* mechanism defined sets */
		rvp->r_val1 = (sizeof(sdefs) / sizeof(setdef_t));
		break;
	case ES_PRVINFO:	/* return the privilege information in */
				/* "arg".  This corresponds the flags */
		if (error = copyout((caddr_t)&pm_flag, arg, sizeof(pm_flag)))
			error = EFAULT;
		break;
	case ES_PRVSETS:	/* return the array of supported privilege */
				/* sets for this privilege mechanism */
		if (error = copyout((caddr_t)sdefs, arg, sizeof(sdefs)))
			error = EFAULT;
		break;
	case ES_PRVID:		/* return the value of privid */
		rvp->r_val1 = privid;
		break;
	default:
		error = EINVAL;
	}
	return error;
}
