/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/execmdep.c	1.7"
#ident	"$Header: $"

#include <fs/buf.h>
#include <fs/file.h>
#include <fs/fstyp.h>
#include <fs/pathname.h>
#include <fs/procfs/prsystm.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/hat.h>
#include <mem/immu.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_vn.h>
#include <mem/vmparam.h>
#include <proc/acct.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/proc.h>
#include <proc/reg.h>
#include <proc/siginfo.h>
#include <proc/signal.h>
#include <proc/tss.h>
#include <proc/ucontext.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/utsname.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/fp.h>
#include <util/inline.h>
#include <util/map.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>
#include <util/weitek.h>

extern int userstack[];
STATIC int exec_initialstk = (ctob(SSIZE));

/*
 * Extract the arguments for the new process and store them
 * in a temporary area in the current process's address space.
 */
int
extractarg(args)
	struct uarg	*args;
{

	register int	error;
	u_int		fnsize;
	caddr_t		nsp;
	u_int		xargc;
	u_int		stgsize;
	u_int		actualstksz;
	u_int		bsize;
	u_int		psize;
	caddr_t		*ptrstart;
	caddr_t		cp;
	u_int		ptrdelta;
	caddr_t		argv0;
	extern int	exec_initialstk;
	u_int		vectorsz;
	struct		pathname	pn;
	int		pngotten = 0;

	/* determine total size for all argument strings */
	stgsize = (u_int)args->argsize + (u_int)args->envsize +
			(u_int)args->prefixsize + (u_int)args->auxsize;

	/* now allow for the fname prefix change to old argv0: */
	if ((xargc = args->prefixc)) {
		if (error = pn_get(args->fname, args->fnameseg, &pn))
			return error;
		pngotten = 1;
		fnsize = pn.pn_pathlen + 1;
		stgsize += fnsize;
	}
	else if (args->flags & EMULA) {
		if (error = pn_get(args->fname, args->fnameseg, &pn))
			return error;
		pngotten = 1;
		fnsize = pn.pn_pathlen + 1;
		stgsize += fnsize;
		xargc = 1;
	}

	stgsize = (stgsize + NBPW-1) & ~(NBPW-1);
	args->stringsize = stgsize;

	bsize = (actualstksz =
		(((vectorsz = (3 + args->argc + args->envc + xargc) * NBPW)
		+ stgsize + ((NBPW*2) - 1)) & ~((NBPW*2) - 1)) + (NBPW * 2))
		+ exec_initialstk;
	psize = btoc(bsize);
	bsize = ctob(psize);

	/* now find a place in the user address space to put it */
	if ((nsp = execstk_addr(bsize, &args->estkhflag)) == NULL) {
#ifdef DEBUG
		cmn_err(CE_CONT,"extractarg: execstk_addr\n");
#endif
		error = ENOMEM;
		goto out;
	}
	args->estkstart = nsp;
	args->estksize = bsize;
	args->stacklow = (addr_t) u.u_userstack + NBPW;

	/* and create it */
	if (error = as_map(u.u_procp->p_as, nsp, bsize, segvn_create, zfod_argsp)) {
#ifdef DEBUG
		cmn_err(CE_CONT,"extractarg: as_map\n");
#endif
		goto out;
	}

	/*
	 * Now fill the temporary stack frame.
	 * The order is dependent on the direction of stack growth,
	 * and so, is machine-dependent.
	 * On 80x86 the userstack grows downwards toward
	 * low addresses; the arrangement is:
	 *
	 * (low address).
	 * argc
	 * prefix ptrs (no NULL ptr)
	 * argv ptrs (with NULL ptr)
	 *	(old argv0 points to fname copy if prefix exits)
	 *	(last argv ptr points to fname copy if EMULATOR present)
	 * env ptrs (with NULL ptr)
	 * postfix values (put here later if they exist)
	 * prefix strings
	 * (fname if a prefix exists)
	 * argv strings
	 * (fname if EMULATOR exists)
	 * env strings
	 * (high address)
	 *
	 */


	ptrdelta = (u_int)args->stacklow - (u_int)(nsp += bsize);
	ptrstart = (caddr_t *)(nsp -= actualstksz);
	cp = (caddr_t) (nsp + vectorsz);
	args->stackend = (caddr_t)(args->stacklow - actualstksz);

	if (suword((int *)ptrstart, args->argc + xargc)) {
#ifdef DEBUG
		cmn_err(CE_CONT,"extractarg: arguments\n");
#endif
err:
		as_unmap(u.u_procp->p_as, args->estkstart, bsize);
		error = EFAULT;
		goto out;
	}
	ptrstart++;

	args->auxaddr = cp + ptrdelta;
	cp += args->auxsize;

	if (args->prefixc) {
		if (copyarglist(args->prefixc, args->prefixp, ptrdelta,
				ptrstart, cp, 1) != args->prefixsize)  {
#ifdef DEBUG
			cmn_err(CE_CONT,"extractarg: arguments\n");
#endif
			goto err;
		}
		ptrstart += xargc;
		cp += args->prefixsize;

		bcopy(pn.pn_path, cp, fnsize);
		cp += fnsize;
	}

	args->argaddr = cp + ptrdelta;
	if (copyarglist(args->argc, args->argp, ptrdelta, ptrstart, cp, 0)
			!= args->argsize) {
#ifdef DEBUG
		cmn_err(CE_CONT,"extractarg: arguments\n");
#endif
		goto err;
	}
	if (args->prefixc) {
		if (suword((int *)ptrstart, (int)(cp + ptrdelta - fnsize))) {
#ifdef DEBUG
			cmn_err(CE_CONT,"extractarg: arguments\n");
#endif
			goto err;
		}
	}
	ptrstart += args->argc + 1;
	cp += args->argsize;

	if (args->flags & EMULA) {
		if (copyarglist(1, (caddr_t *) &args->fname, ptrdelta, ptrstart - 1,
				cp, 1) != fnsize) {
#ifdef DEBUG
			cmn_err(CE_CONT,"extractarg: arguments\n");
#endif
			goto err;
		}
		++ptrstart;
		cp += fnsize;
	}

	if (copyarglist(args->envc, args->envp, ptrdelta, ptrstart, cp, 0)
			!= args->envsize) {
#ifdef DEBUG
		cmn_err(CE_CONT,"extractarg: arguments\n");
#endif
		goto err;
	}
	ASSERT((caddr_t)(ptrstart + args->envc + 1) == (args->auxaddr - ptrdelta));
	ASSERT((addr_t)((((uint)cp + args->envsize + (NBPW*2 - 1))
			& ~(NBPW*2 -1)) + (NBPW*2)) ==
				(addr_t)args->estkstart + args->estksize);

	error = 0;
out:
	if (pngotten)
		pn_free(&pn);
	return error;
}

/*
 * Machine-dependent final setup code goes in setregs().
 */
int
setregs(args)
	register struct uarg *args;
{
	register int i;
	register char *sp;
	register struct proc *p = u.u_procp;

        /*
         * Do psargs.
	 */
	sp = u.u_psargs;
	i = min(args->argsize, PSARGSZ -1);
	if (copyin(args->argaddr, sp, i))
		return(EFAULT);
	while (i--) {
		if (*sp == '\0')
			*sp = ' ';	/* convert NULLs to blanks */
		sp++;
	}
	*sp = '\0';			/* set last byte to NULL */

	p->p_stksize = args->estksize;		/* init stack size */
	p->p_stkbase = (caddr_t)u.u_userstack;	/* init stack base address */

	u.u_sub = (ulong)(u.u_userstack - p->p_stksize + sizeof(int));

	ASSERT((u_int)args->stackend >= (u_int)u.u_sub);
	u.u_ar0[UESP] = (int)args->stackend;
 	u.u_ar0[EIP] = (int)u.u_exdata.ux_entloc;
 	u.u_ar0[CS] = USER_CS;
 	u.u_ar0[DS] = u.u_ar0[ES] = u.u_ar0[SS] = USER_DS;
 	u.u_ar0[FS] = u.u_ar0[GS] = 0;
 	u.u_ar0[EFL] &= ~PS_D;		/* clear direction flag */

 	/*
 	** If this process is being traced, come up with the single-step
 	** flag set.
 	*/
 	if (u.u_procp->p_flag & STRC)
 		u.u_ar0[EFL] |= PS_T;

 	u.u_debugon = 0;
 
 	/* Clear the user return address for signals */
 	u.u_sigreturn = (void (*)()) NULL;

	return 0;
}
