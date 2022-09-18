/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */


#ident	"@(#)uts-comm:proc/cred.c	1.6.3.4"
#ident	"$Header: $"

#include <acc/mac/cca.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/types.h>

struct credlist {
	union {
		struct cred cru_cred;
		struct credlist *cru_next;
	} cl_U;
#define	cl_cred	cl_U.cru_cred
#define	cl_next	cl_U.cru_next
};

/*
 * declare a pointer for the "crfreelist" and set it to NULL
 */
STATIC struct credlist *crfreelist = NULL;
STATIC size_t crsize = 0;	/* for performance only */
STATIC int cractive = 0;	/* initialize cractive to 0 */

/*
 * Global system credential structure.
*/
struct	cred	*sys_cred;

/*
 * Initialize credentials data structures.  (Could be compile-time
 * initializations except that "mkunix" on the 3B2 builds a unix
 * after the initial values have already been changed.)
 */

void
cred_init()
{
	cractive = 0;
	crfreelist = NULL;
}

/*
 * Allocate a zeroed cred structure and crhold() it.
 */
struct cred *
crget()
{
	register struct cred *cr;

	if (crsize == 0)
		(void)crgetsize();	/* crsize set in crgetsize() */

	/*
	 * if there is a freed cred structure on the
	 * crfreelist use it instead of allocating a brand new one.
	 */
	if (crfreelist) {
		cr = &crfreelist->cl_cred;
		crfreelist = ((struct credlist *)cr)->cl_next;
	} else
		cr = (struct cred *)kmem_alloc(crsize, KM_SLEEP);
	/*
	 * regardless of whether the cred structure was taken off
	 * the freelist or just allocated, zero out the contents
	 * so there's no used or "bad" data in the cred structure.
	 */
	struct_zero((caddr_t)cr, crsize);
        /*
         * At this point, we can assert that the cred structure is at the
         * same level as that of the process.
         */
        MAC_ASSERT (cr, MAC_SAME);
	crhold(cr);
	cractive++;
	return cr;
}

/*
 * Free a cred structure.  Return it to the freelist when the reference
 * count drops to 0.
 */
void
crfree(cr)
	register struct cred *cr;
{
	register int s = splhi();

	/*
	 * Reset the priority and return if the credential
	 * structure is still in use.
	 */
	if (--cr->cr_ref != 0) {
		(void) splx(s);
		return;
	}
	/*
	 * Otherwise, put the credential structure on the crfreelist,
	 * indicate its no longer active, and reset the priority.
	 */
	((struct credlist *)cr)->cl_next = crfreelist;
	crfreelist = (struct credlist *)cr;
	cractive--;

	(void) splx(s);
}


/*
 * The code for crcopy() and crdup() are exactly the
 * same except for the "crfree()" done by crcopy().
 * This duplication is for performance reasons.
 */


/*
 * Copy a cred structure to a new one and free the old one.
 */
struct cred *
crcopy(cr)
	register struct cred *cr;
{
	register struct cred *newcr;

	newcr = crget();
	/*
	 * copy the contents of the "old" credential structure
	 * to the "new" credential structure.
	 */
	bcopy((caddr_t)cr, (caddr_t)newcr, crsize);
	crfree(cr);
	newcr->cr_ref = 1;

	return newcr;
}

/*
 * Dup a cred struct to a new held one.
 */
struct cred *
crdup(cr)
	struct cred *cr;
{
	register struct cred *newcr;

	newcr = crget();
	/*
	 * copy the contents of the "old" credential structure
	 * to the "new" credential structure.
	 */
	bcopy((caddr_t)cr, (caddr_t)newcr, crsize);
	newcr->cr_ref = 1;

	return newcr;
}

/*
 * Return the (held) credentials for the current running process.
 */
struct cred *
crgetcred()
{
	crhold(u.u_cred);
	return u.u_cred;
}

/*
 * Return the size of the credentials.
 *
 * Note that setting crsize here instead of crget() allows
 * crgetsize() to be called before a crget() call.
 * Further note that for performance, crsize is defined
 * as a static to the file.
 */
size_t
crgetsize()
{
	if (crsize == 0) {
		crsize = sizeof(struct cred) + sizeof(uid_t)*(ngroups_max-1);
		/* Make sure it's word-aligned. */
		crsize = (crsize+sizeof(int)-1) & ~(sizeof(int)-1);
	}
	return(crsize);
}


/*
 * Determine whether the supplied group id is a member of the group
 * described by the supplied credentials.
 */
int
groupmember(gid, cr)
	register gid_t gid;
	register struct cred *cr;
{
	register gid_t *gp, *endgp;

	/*
	 * if the supplied gid is equal to the "effective"
	 * gid in the credential structure, return success (1).
	 */
	if (gid == cr->cr_gid)
		return 1;
	/*
	 * find the end of the supplemental groups
	 */
	endgp = &cr->cr_groups[cr->cr_ngroups];
	/*
	 * search through the supplemental group list.
	 * If a match is found, return success (1).
	 */
	for (gp = cr->cr_groups; gp < endgp; gp++)
		if (*gp == gid)
			return 1;
	return 0;		/* no match */
}

/*
 * This function is called to check whether the credentials set
 * "scrp" has permission to act on credentials set "tcrp".  It enforces the
 * permission requirements needed to send a signal to a process.
 * The same requirements are imposed by other system calls, however.
 */

int
hasprocperm(tcrp, scrp)
	register cred_t	*tcrp;
	register cred_t	*scrp;
{
	return !pm_denied(scrp, P_OWNER)
	  || scrp->cr_uid  == tcrp->cr_ruid
	  || scrp->cr_ruid == tcrp->cr_ruid
	  || scrp->cr_uid  == tcrp->cr_suid
	  || scrp->cr_ruid == tcrp->cr_suid;
}
