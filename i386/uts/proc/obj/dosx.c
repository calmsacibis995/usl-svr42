/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:proc/obj/dosx.c	1.1"
#ident	"$Header: $"

#include <util/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/signal.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <fs/vnode.h>
#include <proc/mman.h>
#include <mem/kmem.h>
#include <util/sysinfo.h>
#include <mem/immu.h>
#include <proc/proc.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <proc/exec.h>

#define	DOSEXEMAGIC	0x5a4d		/* DOS .exe file */
#define DOSBLTMAGIC	0x5a4c		/* DOS builtin file */

short	dosx_magic[2] = {
	DOSEXEMAGIC,
	DOSBLTMAGIC
};

char *mrg_dos_file = "/sbin/dosexec";
extern int	merge386enable;	/* Is MERGE386 enabled or disabled */


/* ARGSUSED */
int
dosx_exec(vp, args, level, execsz, ehdp, setid)
struct vnode *vp;
struct uarg *args;
int level;
long	 *execsz;
exhda_t *ehdp;
int setid;
{
	int	error;

	if (!merge386enable)
		return ENOEXEC;

	/* If no magic number, maybe it's a .com or .bat file */
	if (u.u_execsw->exec_magic == NULL && !isdosexec(vp, ehdp))
		return ENOEXEC;
	/* CHANGE SUGGESTED BY LOCUS */
	if ((error = set0emulate(mrg_dos_file, args, execsz)) != 0)
		return error;
	return -1;	/* Special emulator return */
}


int
dosx_core()
{
	return ENOSYS;
}
