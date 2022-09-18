/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:svc/name.c	1.6"
#ident	"$Header: $"

#include <acc/dac/acl.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/statvfs.h>
#include <fs/ustat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <proc/session.h>
#include <proc/signal.h>
#include <proc/ucontext.h>
#include <proc/unistd.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/hrtcntl.h>
#include <svc/resource.h>
#include <svc/sysconfig.h>
#include <svc/systeminfo.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/uadmin.h>
#include <svc/ulimit.h>
#include <svc/utsname.h>
#include <svc/utssys.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

STATIC char	*version = VERSION;	/* VER and REL are defined at */
STATIC char	*release = RELEASE;	/* compile time, on the command line */
STATIC char	sysinfo_buf[SYS_NMLN];	/* common sysinfo_buf used to copyin
					** strings from user space.
					*/
extern char	architecture[];		/* from name config file */
extern char	hw_provider[];		/* from name config file */
extern char	hw_serial[];		/* from name config file */
extern char     initfile[];             /* from /etc/system */
extern int	exec_ncargs;		/* from proc.cf/Space.c  */

/*
 * Initialize uname info.
 * Machine-dependent code.
 */
void
inituname()
{
	/*
	 * Get the release and version of the system.
	 */
	if (release[0] != '\0') {
		strncpy(utsname.release, release, SYS_NMLN-1);
		utsname.release[SYS_NMLN-1] = '\0';
	}
	if (version[0] != '\0') {
		strncpy(utsname.version, version, SYS_NMLN-1);
		utsname.version[SYS_NMLN-1] = '\0';
	}
}

/*
 * Provides system configuration information.
 *
 * NOTE: check the code in sco.c when making any implementation
 * changes to avoid breaking the SCO-compatible equivalent of
 * this function
 */

struct sysconfiga {
	int which;
};

int
sysconfig(uap, rvp)
	register struct sysconfiga *uap;
	rval_t *rvp;
{
	switch (uap->which) {

	default:
		return EINVAL;

	case _CONFIG_CLK_TCK:
		/*
		 * Clock frequency, ticks per second.
		 */
		rvp->r_val1 = HZ;
		break;

	case _CONFIG_NGROUPS:
		/*
		 * Maximum number of supplementary groups.
		 */
		rvp->r_val1 = ngroups_max;
		break;

	case _CONFIG_OPEN_FILES:
		/*
		 * Maxiumum number of open files (soft limit).
		 */
		rvp->r_val1 = u.u_rlimit[RLIMIT_NOFILE].rlim_cur;
		break;

	case _CONFIG_CHILD_MAX:
		/*
		 * Maxiumum number of processes.
		 */
		rvp->r_val1 = v.v_maxup;
		break;

	case _CONFIG_POSIX_VER:
		rvp->r_val1 = _POSIX_VERSION;	/* current POSIX version */
		break;
	
	case _CONFIG_PAGESIZE:
		rvp->r_val1 = PAGESIZE;
		break;

	case _CONFIG_XOPEN_VER:
		rvp->r_val1 = _XOPEN_VERSION; /* current XOPEN version */
		break;

	case _CONFIG_NACLS_MAX:
                rvp->r_val1 = acl_getmax();   /* for Enhanced Security */
                break;
	case _CONFIG_ARG_MAX:
                rvp->r_val1 = exec_ncargs; /* max length of exec args*/
                break;
	}
	
	return 0;
}

/*
 * uname system call - provides name of system.
 */

struct uname {
	struct utsname *cbuf;
};

/* ARGSUSED */
int
nuname(uap, rvp)
	register struct uname *uap;
	rval_t *rvp;
{
	register int error = 0;
        register struct utsname *buf = uap->cbuf;

	if (copyout(utsname.sysname, buf->sysname, strlen(utsname.sysname)+1)) {
		error = EFAULT;
		return error;
	}
	if (copyout(utsname.nodename, buf->nodename, strlen(utsname.nodename)+1)) {
		error = EFAULT;
		return error;
	}
	if (copyout(utsname.release, buf->release, strlen(utsname.release)+1)) {
		error = EFAULT;
		return error;
	}
	if (copyout(utsname.version, buf->version, strlen(utsname.version)+1)) {
		error = EFAULT;
		return error;
	}
	if (copyout(utsname.machine, buf->machine, strlen(utsname.machine)+1)) {
		error = EFAULT;
		return error;
	}
	rvp->r_val1 = 1;
	return error;
}

struct systeminfoa {
	int command;
	char *buf;
	long count;
};

/*
 * Service routine for systeminfo().
 */
static int
strout(str, uap, rvp)
        char *str;
        register struct systeminfoa *uap;
        rval_t *rvp;
{
        register int strcnt, getcnt;

        getcnt = ((strcnt = strlen(str)) >= uap->count)? uap->count : strcnt+1;

        if (copyout(str, uap->buf, getcnt))
                return EFAULT;

        if (strcnt >= uap->count && subyte(uap->buf+uap->count-1, 0) < 0)
                return EFAULT;

        rvp->r_val1 = strcnt + 1;
        return 0;
}

/*
 * sysinfo system call - get and set system information.
 */
/* ARGSUSED */
int
systeminfo(uap, rvp)
	register struct systeminfoa *uap;
	rval_t *rvp;
{
	register int error;

	switch (uap->command) {

	case SI_SYSNAME:
		error = strout(utsname.sysname, uap, rvp);
		break;

	case SI_HOSTNAME:
		error = strout(utsname.nodename, uap, rvp);
		break;

	case SI_RELEASE:
		error = strout(utsname.release, uap, rvp);
		break;

	case SI_VERSION:
		error = strout(utsname.version, uap, rvp);
		break;

	case SI_MACHINE:
		error = strout(utsname.machine, uap, rvp);
		break;

	case SI_ARCHITECTURE:
		error = strout(architecture, uap, rvp);
		break;

	case SI_HW_SERIAL:
		error = strout(hw_serial, uap, rvp);
		break;

	case SI_INITTAB_NAME:
                error = strout(initfile, uap, rvp);
                break;

	case SI_HW_PROVIDER:
		error = strout(hw_provider, uap, rvp);
		break;

	case SI_SRPC_DOMAIN:
		error = strout(srpc_domain, uap, rvp);
		break;

	case SI_SET_HOSTNAME:
		error = setnodename(uap->buf, &rvp->r_val1);
		break;

	case SI_SET_SRPC_DOMAIN:
	{
		size_t len;

		if ((error = cpyin_sysinfo(uap->buf, &sysinfo_buf, &len)) != 0)
			break;
		strcpy(srpc_domain, sysinfo_buf);
		rvp->r_val1 = len;
		break;
	}
	case SI_SET_HW_SERIAL:
	{
		size_t len;

		if ((error = cpyin_sysinfo(uap->buf, &sysinfo_buf, &len)) != 0)
			break;
		strcpy(hw_serial, sysinfo_buf);
		rvp->r_val1 = len;
		break;
	}
	case SI_SET_HW_PROVIDER:
	{
		size_t len;

		if ((error = cpyin_sysinfo(uap->buf, &sysinfo_buf, &len)) != 0)
			break;
		strcpy(hw_provider, sysinfo_buf);
		rvp->r_val1 = len;
		break;
	}

	default:
		error = EINVAL;
		break;
	}

	return error;
}

/*
 * Fetch a new nodename from user space, validate it,
 * and set utsname.nodename.  On success, set *lenp
 * (if non-NULL) to the length of the new nodename and
 * return 0; on failure, return an appropriate ERRNO.
 */
int
setnodename(uaddr, lenp)
	char	*uaddr;
	size_t	*lenp;
{
	size_t	len;
	int	error;
	char 	name[SYS_NMLN];

	if (pm_denied(u.u_cred, P_SYSOPS))
		return EPERM;

	if ((error = copyinstr(uaddr, name, SYS_NMLN, &len)) != 0)
		return error;

	/* 
	 * Must be non-NULL string and string
	 * must be less than SYS_NMLN chars.
	 */
	if (len < 2 || (len == SYS_NMLN && name[SYS_NMLN-1] != '\0'))
		return EINVAL;

	/*
	 * Copy the name into the global utsname structure.
	 */
	strcpy(utsname.nodename, name);

	if (lenp != NULL)
		*lenp = len;

	return 0;
}

int
cpyin_sysinfo(uaddr, p, len)
	char	*uaddr;
	char	*p;
	size_t	*len;
{
	int	error;

	if (pm_denied(u.u_cred, P_SYSOPS))
		return EPERM;

	if ((error = copyinstr(uaddr, p, SYS_NMLN, len)) != 0)
		return error;

	/* 
	 * Must be non-NULL string and string
	 * must be less than SYS_NMLN chars.
	 */
	if (*len == SYS_NMLN && p[SYS_NMLN-1] != '\0')
		return EINVAL;

	return 0;
}
