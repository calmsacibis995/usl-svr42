/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:proc/ipc/ipc.c	1.8.3.3"
#ident	"$Header: $"

/*
 * Common Inter-Process Communication routines.
 */

#include <acc/mac/covert.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <proc/cred.h>
#include <proc/ipc/ipc.h>
#include <proc/ipc/ipcsec.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

/* control structure for covert channel limiter */
STATIC ccevent_t cc_alloc_ipc = { CC_ALLOC_IPC, CCBITS_ALLOC_IPC };

/*
 * ipcaccess - Check message, semaphore, or shared memory access permissions.
 *
 *	Arguments:
 * 		p	- Pointer to ipc_perm structure
 * 		mode	- Desired access permissions (e.g. IPC_R or IPC_W)
 *		flags	- Desired security access (e.g. IPC_DAC or IPC_MAC)
 *		cr	- User credentials
 *
 *	Return value:
 *		0	- Access granted
 *		EACCES	- DAC access denied
 *		EINVAL	- MAC access denied
 *
 *	Notes:
 *		. p is assumed to be a valid structure pointer;
 *		  no validation checks are performed.
 *		. MAC_ACCESS() returns zero on success.
 *		. goto label nodaccess points to code which checks
 *		  for DAC privilege if normal DAC checks have failed.
 */

int
ipcaccess(p, mode, flags, cr)
	register struct ipc_perm	*p;		/* permission struct */
	register int			mode;		/* requested mode access */
	uint				flags;		/* requested sec access */
	register struct cred		*cr;		/* user credentials */
{
	int				savemode;	/* mode used for priv */
	int				ipcaclck();	/* IPC ACL checker */

	/*
	 * Match if the user has the required access privileges.
	 */

	ASSERT((mode & ~(IPC_R|IPC_W)) == 0);
	ASSERT(flags==IPC_DAC || flags==IPC_MAC || flags==(IPC_DAC|IPC_MAC));

	/*
	 * On a MAC access request, if the calling process level and
	 * the object level are the same, MAC access is always granted.
	 * If not equal, P_MACWRITE privilege can override MAC on a
	 * write request.  If a read request, calling process must
	 * dominate object.  P_MACREAD privilege can override MAC
	 * on a read request.
	 */
	if ((flags & IPC_MAC)
	&&  MAC_ACCESS(MACEQUAL, cr->cr_lid, p->ipc_secp->ipc_lid)) {
		if ((mode & IPC_W) && pm_denied(cr, P_MACWRITE))
			return EINVAL;
		if ((mode & IPC_R)
		&&  MAC_ACCESS(MACDOM, cr->cr_lid, p->ipc_secp->ipc_lid)
		&&  pm_denied(cr, P_MACREAD))
			return EINVAL;
	}

	/* return success if no DAC request */
	if ((flags & IPC_DAC) == 0)
		return 0;

	/* remember mode for privilege checks, since mode may be shifted */
	savemode = mode;

	/* check object user (USER_OBJ) access */
	if ((cr->cr_uid == p->uid) || (cr->cr_uid == p->cuid)) {
		if ((p->mode & mode) == mode)
			return 0;
		goto nodaccess;
	}

	/*
	 * At this point, check additional users, object group, and additional
	 * groups (USERS, GROUP_OBJ, GROUP) access.  If there is no ACL for
	 * the IPC object, just check for object group (GROUP_OBJ) access.
	 */
	mode >>= 3;	/* re-position desired access for extended permission */

	if (p->ipc_secp->dacp) {	/* check ACL entries */
		switch (ipcaclck(p, mode, cr)) {
		case -1:		/* no match found in ACL */
			/* go try object other (OTHER_OBJ) access */
			break;

		case 0:			/* match found, and access granted */
			return 0;

		case 1:			/* match found, but access denied */
			goto nodaccess;

		default:
			ASSERT(0);	/* panic in debug mode */
			return EACCES;
		}
	}
	else {	/* just check group (GROUP_OBJ) access */
		if (groupmember(p->gid, cr) || groupmember(p->cgid, cr)) {
			if ((p->mode & mode) == mode)
				return 0;
			goto nodaccess;
		}
	}

	/* Finally, check object other (OTHER_OBJ) access */
	mode >>= 3;	/* re-position desired access for other */

	if ((p->mode & mode) == mode)
		return 0;
nodaccess:
	/* Check for DAC privilege */
	if (((savemode & IPC_R)  &&  pm_denied(cr, P_DACREAD))
	||  ((savemode & IPC_W)  &&  pm_denied(cr, P_DACWRITE)))
		return EACCES;

	return 0;
}

/*
 * Get message, semaphore, or shared memory structure.
 *
 * This routine searches for a matching key based on the given flags
 * and returns, in *ipcpp, a pointer to the appropriate entry. 
 * A structure is allocated if the key doesn't exist and the flags call
 * for it.  The arguments must be set up as follows:
 * 	key - Key to be used
 * 	flag - Creation flags and access modes
 * 	base - Base address of appropriate facility structure array
 * 	cnt - # of entries in facility structure array
 * 	size - sizeof(facility structure)
 * 	status - Pointer to status word: set on successful completion
 * 		only:	0 => existing entry found
 * 			1 => new entry created
 * Ipcget returns 0 on success, or an appropriate non-zero errno on failure.
 *
 * A MAC check is performed here to get a unique identifier based on
 * the key and the object's level.  No privilege MAC check is
 * performed.
 */
int
ipcget(key, flag, base, cnt, size, status, ipcpp)
	key_t				key;
	int				flag;
	register struct ipc_perm	*base;
	int				cnt;
	int				size;
	int				*status;
	struct ipc_perm			**ipcpp;
{
	register struct ipc_perm	*a;	/* ptr to available entry */
	register struct ipc_perm	*sbase = NULL;
	register int			i;	/* loop control */

	if (key == IPC_PRIVATE) {
		for (i = 0; i++ < cnt;
		  /* LINTED pointer alignment */
		  base = (struct ipc_perm *)(((char *)base) + size)) {
			if (base->mode & IPC_ALLOC)
				continue;
			if (mac_installed
			&&  base->ipc_secp->ipc_lid
			&&  MAC_ACCESS(MACEQUAL, u.u_cred->cr_lid,
						base->ipc_secp->ipc_lid)) {
				if (sbase == NULL)
					sbase = base;
				continue;
			}
			goto init;
		}
		if (sbase == NULL)
			return ENOSPC;
		base = sbase;
	} else {
		for (i = 0, a = NULL; i++ < cnt;
		  /* LINTED pointer alignment */
		  base = (struct ipc_perm *)(((char *)base) + size)) {
			if (base->mode & IPC_ALLOC) {
				if (base->key == key) {
					if (MAC_ACCESS(MACEQUAL,
							u.u_cred->cr_lid,
							base->ipc_secp->ipc_lid))
						continue;
					if ((flag & (IPC_CREAT | IPC_EXCL)) ==
					  (IPC_CREAT | IPC_EXCL)) {
						return EEXIST;
					}
					if ((flag & 0777) & ~base->mode)
						return EACCES;
					*status = 0;
					*ipcpp = base;
					return 0;
				}
				continue;
			}
			if (mac_installed
			&&  base->ipc_secp->ipc_lid
			&&  MAC_ACCESS(MACEQUAL, u.u_cred->cr_lid,
						base->ipc_secp->ipc_lid)) {
				if (sbase == NULL)
					sbase = base;
				continue;
			}
			if (a == NULL)
				a = base;
		}
		if (!(flag & IPC_CREAT))
			return ENOENT;
		if (a == NULL) {
			if (sbase == NULL)
				return ENOSPC;
			base = sbase;
		} else
			base = a;
	}
init:
	*status = 1;
	base->mode = IPC_ALLOC | (flag & 0777);
	base->key = key;
	base->cuid = base->uid = u.u_cred->cr_uid;
	base->cgid = base->gid = u.u_cred->cr_gid;
	/* the object's level is assigned the user's level */
	base->ipc_secp->ipc_lid = u.u_cred->cr_lid;
	*ipcpp = base;
	if (base == sbase)
		cc_limiter(&cc_alloc_ipc, u.u_cred);
	return 0;
}
