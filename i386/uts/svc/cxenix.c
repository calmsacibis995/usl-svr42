/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */
#ident	"@(#)uts-x86:svc/cxenix.c	1.6"
#ident	"$Header: $"

#include <proc/reg.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/types.h>

/* Enhanced Application Compatibility Support */
#include <io/termios.h>
#include <svc/sco.h>
/* End Enhanced Application Compatibility Support */

int	nosys();  
int	chsize(); 
int	creatsem();
int	execseg();
int	ftime();    
int	locking();   
int	nap(); 
int	nbwaitsem();
int	opensem();
int	proctl(); 
int	rdchk(); 
int	sdenter(); 
int	xsdfree();    
int	sdget();   
int	sdgetv(); 
int	sdleave();  
int	sdwaitv();
int	sigsem();
int	unexecseg(); 
int	waitsem();


/* Enhanced Application Compatibility Support */
/* The functions required ACP package are declared in sco.h */
/* Enhanced Application Compatibility Support */

/*
#define Xdebug 1
*/

/*
 * XENIX-special system calls.  In order to save space in the system
 * call table, and to minimize conflicts with other unix systems,
 * all custom XENIX calls are done via the cxenix call.
 * The cxentry table is the switch used to transfer
 * to the appropriate routine for processing a cxenix sub-type system call.
 * Each row contains the number of arguments expected,
 * a switch that tells systrap() in trap.c whether a setjmp() is not necessary,
 * and a pointer to the routine.  Note that all cxenix system calls will
 * do the setjmp() in systrap(), for XENIX compatibility.
 */

struct sysent cxentry[] = {
	0, 0, nosys,			/* 0 = obsolete (XENIX shutdown) */
	3, 0, locking,			/* 1 = XENIX file/record lock */
	2, 0, creatsem,			/* 2 = create XENIX semaphore */
	1, 0, opensem,			/* 3 = open XENIX semaphore */
	1, 0, sigsem,			/* 4 = signal XENIX semaphore */
	1, 0, waitsem,			/* 5 = wait on XENIX semaphore */
	1, 0, nbwaitsem,		/* 6 = nonblocking wait on XENIX sem */
	1, 0, rdchk,			/* 7 = read check */
	0, 0, nosys,			/* 8 = obsolete (XENIX stkgrow) */
	0, 0, nosys,			/* 9 = obsolete (XENIX ptrace) */
	2, 0, chsize,			/* 10 = change file size */
	1, 0, ftime,			/* 11 = V7 ftime*/
	1, 0, nap,			/* 12 = nap */
	4, 0, sdget,			/* 13 = create/attach XENIX shdata */
	1, 0, xsdfree,			/* 14 = free XENIX shdata */
					/*    N.B. changed name from sdfree
					 *    because it conflicted with nudnix.
					 */
	2, 0, sdenter,			/* 15 = enter XENIX shdata */
	1, 0, sdleave,			/* 16 = leave XENIX shdata */
	1, 0, sdgetv,			/* 17 = get XENIX shdata version */
	2, 0, sdwaitv,			/* 18 = wait for XENIX shdata version */
	0, 0, nosys,			/* 19 = obsolete (XENIX brkctl) */
	0, 0, nosys,			/* 20 = unused (reserved for XENIX) */
	0, 0, nosys,			/* 21 = unused reserved (XENIX nfs_sys) */
	0, 0, nosys,			/* 22 = obsolete (XENIX msgctl) */
	0, 0, nosys,			/* 23 = obsolete (XENIX msgget) */
	0, 0, nosys,			/* 24 = obsolete (XENIX msgsnd) */
	0, 0, nosys,			/* 25 = obsolete (XENIX msgrcv) */
	0, 0, nosys,			/* 26 = obsolete (XENIX semctl) */
	0, 0, nosys,			/* 27 = obsolete (XENIX semget) */
	0, 0, nosys,			/* 28 = obsolete (XENIX semop) */
	0, 0, nosys,			/* 29 = obsolete (XENIX shmctl) */
	0, 0, nosys,			/* 30 = obsolete (XENIX shmget) */
	0, 0, nosys,			/* 31 = obsolete (XENIX shmat) */
	3, 0, proctl,			/* 32 = proctl */
	0, 0, execseg,			/* 33 = execseg */
	0, 0, unexecseg,		/* 34 = unexecseg */
	0, 0, nosys,			/* 35 = obsolete (XENIX swapadd) */

	/* Enhanced Application Compatibility Support */
	5, 0, select_sco,		/* 36 = SCO select */
	2, 0, eaccess_sco,		/* 37 = SCO eaccess*/
	0, 0, nosys,			/* 38 = SCO RESERVED */
	3, 0, sigaction_sco,		/* 39 = SCO sigaction */
	3, 0, sigprocmask_sco,		/* 40 = SCO sigprocmask */
	1, 0, sigpending_sco,		/* 41 = SCO sigpending */
	1, 0, sigsuspend_sco,		/* 42 = SCO sigsuspend */
	2, 0, getgroups_sco,		/* 43 = SCO getgroups */
	2, 0, setgroups_sco,		/* 44 = SCO setgroups */
	1, 0, sysconf_sco,		/* 45 = SCO sysconf */
	2, 0, pathconf_sco,		/* 46 = SCO pathconf */
	2, 0, fpathconf_sco,		/* 47 = SCO fpathconf */
	2, 0, rename_sco,		/* 48 = SCO rename */
	0, 0, nosys,			/* 49 = Not Used */
	2, 0, scoinfo,			/* 50 = SCO scoinfo */
	0, 0, nosys,			/* 51 = SCO/ALTOS Reserved */
	0, 0, nosys,			/* 52 = SCO/ALTOS Reserved */
	0, 0, nosys,			/* 53 = SCO/ALTOS Reserved */
	0, 0, nosys,			/* 54 = SCO/ALTOS Reserved */
	0, 0, nosys,			/* 55 = SCO TBD */
	0, 0, nosys,			/* 56 = SCO TBD */

	0, 0, nosys,			/* 57 = SCO TBD */
	0, 0, nosys,			/* 58 = SCO TBD */
	/* End Enhanced Application Compatibility Support */
};

/* number of cxenix subfunctions */	
int ncxentry = sizeof(cxentry)/sizeof(struct sysent);

#ifdef Xdebug
int Xdbg = 0;
#endif

/*
 *      cxenix - XENIX custom system call dispatcher
 */

/* ARGSUSED */
int
cxenix(uap, rvp)
char *uap;
rval_t *rvp;
{
	register int subfunc;
	register struct user *uptr = &u;
	register struct sysent *callp;
	register int *ap;
	register u_int i;
	int ret;

	ap = (int *)u.u_ar0[UESP];
	ap++;			/* ap points to the return addr on the user's
				 * stack. bump it up to the actual args.  */
	subfunc = (u.u_syscall >> 8) & 0xff;
	if (subfunc >= ncxentry)
		return EINVAL;
	callp = &cxentry[subfunc];
#ifdef Xdebug
	Xdbprt(subfunc);
#endif
	/* get cxenix arguments in U block */
	for (i = 0; i < callp->sy_narg; i++){
		uptr->u_arg[i] = lfuword(ap++);
#ifdef Xdebug
		if (Xdbg)printf ("%x  ", uptr->u_arg[i] );
#endif
	}
#ifdef Xdebug
	if (Xdbg)
		printf ("\n");
#endif
	uptr->u_ap = uptr->u_arg;

	/* do the system call */
	ret = (*callp->sy_call)(uptr->u_ap, rvp);
	return ret;

}

#ifdef Xdebug
struct Xdbgtab{
	int args;
	int ljmp;
	char *name;
} Xdbgt[]={
	0, 1, "nosys",			/* 0 = obsolete (XENIX shutdown) */
	3, 0, "locking",		/* 1 = XENIX file/record lock */
	2, 0, "creatsem",		/* 2 = create XENIX semaphore */
	1, 0, "opensem",		/* 3 = open XENIX semaphore */
	1, 0, "sigsem",			/* 4 = signal XENIX semaphore */
	1, 0, "waitsem",		/* 5 = wait on XENIX semaphore */
	1, 0, "nbwaitsem",		/* 6 = nonblocking wait on XENIX sem */
	1, 0, "rdchk",			/* 7 = read check */
	0, 1, "nosys",			/* 8 = obsolete (XENIX stkgrow) */
	0, 1, "nosys",			/* 9 = obsolete (XENIX ptrace) */
	2, 0, "chsize",			/* 10 = change file size */
	1, 0, "ftime",			/* 11 = V7 ftime*/
	1, 0, "nap",			/* 12 = nap */
	4, 0, "sdget",			/* 13 = create/attach XENIX shdata */
	1, 0, "xsdfree",		/* 14 = free XENIX shdata */
	2, 0, "sdenter",		/* 15 = enter XENIX shdata */
	1, 0, "sdleave",		/* 16 = leave XENIX shdata */
	1, 0, "sdgetv",			/* 17 = get XENIX shdata version */
	2, 0, "sdwaitv",		/* 18 = wait for XENIX shdata version */
	0, 1, "nosys",			/* 19 = obsolete (XENIX brkctl) */
	0, 1, "nosys",			/* 20 = unused (reserved for XENIX) */
	0, 1, "nosys",			/* 21 = obsolete (XENIX nfs_sys) */
	0, 1, "nosys",			/* 22 = obsolete (XENIX msgctl) */
	0, 1, "nosys",			/* 23 = obsolete (XENIX msgget) */
	0, 1, "nosys",			/* 24 = obsolete (XENIX msgsnd) */
	0, 1, "nosys",			/* 25 = obsolete (XENIX msgrcv) */
	0, 1, "nosys",			/* 26 = obsolete (XENIX semctl) */
	0, 1, "nosys",			/* 27 = obsolete (XENIX semget) */
	0, 1, "nosys",			/* 28 = obsolete (XENIX semop) */
	0, 1, "nosys",			/* 29 = obsolete (XENIX shmctl) */
	0, 1, "nosys",			/* 30 = obsolete (XENIX shmget) */
	0, 1, "nosys",			/* 31 = obsolete (XENIX shmat) */
	3, 0, "proctl",			/* 32 = proctl */
	0, 0, "execseg",		/* 33 = execseg */
	0, 0, "unexecseg",		/* 34 = unexecseg */
	0, 1, "nosys",			/* 35 = obsolete (XENIX swapadd) */

	/* Enhanced Application Compatibility Support */
	5, 0, "select_sco",		/* 36 = SCO select */
	2, 0, "eaccess_sco",		/* 37 = SCO eaccess */
	0, 0, "nosys",			/* 38 = SCO TBD */
	3, 0, "sigaction_sco",		/* 39 = SCO sigaction */
	3, 0, "sigprocmask_sco",	/* 40 = SCO sigprocmask */
	1, 0, "sigpending_sco",		/* 41 = SCO sigpending */
	1, 0, "sigsuspend_sco",		/* 42 = SCO sigsuspend */
	2, 0, "getgroups_sco",		/* 43 = SCO getgroups */
	2, 0, "setgroups_sco",		/* 44 = SCO setgroups */
	0, 0, "sysconf_sco",		/* 45 = SCO sysconf */
	0, 0, "pathconf_sco",		/* 46 = SCO pathconf */
	0, 0, "fpathconf_sco",		/* 47 = SCO fpathconf */
	0, 0, "rename_sco",		/* 48 = SCO rename */
	0, 0, "nosys",			/* 49 = Not Used */
	2, 0, "scoinfo",			/* 50 = SCO scoinfo */
	0, 0, "nosys",			/* 51 = SCO/ALTOS Reserved */
	0, 0, "nosys",			/* 52 = SCO/ALTOS Reserved */
	0, 0, "nosys",			/* 53 = SCO/ALTOS Reserved */
	0, 0, "nosys",			/* 54 = SCO/ALTOS Reserved */
	0, 0, "nosys",			/* 55 = SCO TBD */
	0, 0, "nosys",			/* 56 = SCO TBD */

	0, 0, "nosys",			/* 57 = SCO TBD */
	0, 0, "nosys",			/* 58 = SCO TBD */

	/* End Enhanced Application Compatibility Support */
};
Xdbprt(callno)
{
	if(Xdbg) 
		printf ( "xenix call %d: %s, nargs =%d; ",
			callno, Xdbgt[callno].name, Xdbgt[callno].args);
}
#endif
