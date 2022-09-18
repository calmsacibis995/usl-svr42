/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/aclsort.c	1.4"

#ifdef __STDC__
	#pragma weak aclsort = _aclsort
#endif

#include "synonyms.h"
#include <sys/types.h>
#include <acl.h>

static void		isort();
static int		aclcmpfcn();

/*
 * aclsort()	- Sort a buffer of ACL entries
 *
 *	Arguments:
 *		nentries	- number of ACL entries
 *		calclass	- indicates whether to recalculate class entry
 *		aclbufp		- buffer of ACL entries
 *
 *	Return value:
 *		0		- success
 *		-1		- no mandatory entry or invalid ACL type
 *		other		- position of duplicate entry
 *
 *	Notes:
 *		1. Use insertion sort isort() to perform the sort.
 *		2. Combine sort by type and id.
 *		3. The assumption on types is that they are defined
 *		   low to high so that entries will be sorted in the
 *		   right order.
 *		4. If nentries is larger than the allocated buffer,
 *		   the caller is on its own.
 *		5. Even on error, an attempt is made to sort the ACL
 *		   with the available information.
 *		6. Validation of user and group ids is not performed.
 *
 */

int
aclsort(nentries, calclass, aclbufp)
	int			nentries;
	int			calclass;
	struct acl		*aclbufp;
{
	register struct acl	*aclp;
	register int		last_id;
	int			i;
	int			user_obj = 0;
	int			users = 0;
	int			group_obj = 0;
	int			groups = 0;
	int			class_obj = 0;
	int			other_obj = 0;
	int			def_user_obj = 0;
	int			def_users = 0;
	int			def_group_obj = 0;
	int			def_groups = 0;
	int			def_class_obj = 0;
	int			def_other_obj = 0;
	mode_t			class_perms = 0;
	mode_t			group_perms;
	mode_t			def_class_perms;
	mode_t			def_group_perms;

	/* quickly sort the ACL entries with no validation checks whatsoever */
	isort(aclbufp, nentries);

	/* check for duplicate entries */
	for (i = nentries, aclp = aclbufp; i > 0; i--, aclp++) {
		switch (aclp->a_type) {
		case USER_OBJ:
			/* check for duplicate object user */
			if (user_obj)
				return(nentries-i);
			user_obj++;
			break;
		
		case USER:
			if (!users)
				last_id = -1;	/* reset last user id */
			/* check for duplicate user ids */
			if (aclp->a_id == last_id)
				return(nentries-i);
			last_id = aclp->a_id;	/* set last user id */
			if (calclass)
				class_perms |= aclp->a_perm;
			users++;
			break;

		case GROUP_OBJ:
			/* check for duplicate object group */
			if (group_obj)
				return(nentries-i);
			if (calclass)
				class_perms |= aclp->a_perm;
			group_obj++;
			group_perms = aclp->a_perm;
			break;

		case GROUP:
			if (!groups)
				last_id = -1;	/* reset last group id */
			/* check for duplicate group ids */
			if (aclp->a_id == last_id)
				return(nentries-i);
			last_id = aclp->a_id;	/* set last group id */
			if (calclass)
				class_perms |= aclp->a_perm;
			groups++;
			break;

		case CLASS_OBJ:
			/* check for duplicate class entry */
			if (class_obj)
				return(nentries-i);
			if (calclass)
				aclp->a_perm = class_perms;
			else
				class_perms = aclp->a_perm;
			class_obj++;
			break;

		case OTHER_OBJ:
			/* check for duplicate other entry */
			if (other_obj)
				return(nentries-i);
			other_obj++;
			break;

		case DEF_USER_OBJ:
			/* check for duplicate def object user */
			if (def_user_obj)
				return(nentries-i);
			def_user_obj++;
			break;
		
		case DEF_USER:
			if (!def_users)
				last_id = -1;	/* reset last def user id */
			/* check for duplicate def user ids */
			if (aclp->a_id == last_id)
				return(nentries-i);
			last_id = aclp->a_id;	/* set last def user id */
			def_users++;
			break;

		case DEF_GROUP_OBJ:
			/* check for duplicate def object group */
			if (def_group_obj)
				return(nentries-i);
			def_group_obj++;
			def_group_perms = aclp->a_perm;
			break;

		case DEF_GROUP:
			if (!def_groups)
				last_id = -1;	/* reset last def group id */
			/* check for duplicate def group ids */
			if (aclp->a_id == last_id)
				return(nentries-i);
			last_id = aclp->a_id;	/* set last def group id */
			def_groups++;
			break;

		case DEF_CLASS_OBJ:
			/* check for duplicate def class entry */
			if (def_class_obj)
				return(nentries-i);
			def_class_obj++;
			def_class_perms = aclp->a_perm;
			break;

		case DEF_OTHER_OBJ:
			/* check for duplicate def other entry */
			if (def_other_obj)
				return(nentries-i);
			def_other_obj++;
			break;

		default:
			return(-1);		/* no such type */
			/*NOTREACHED*/
			break;
		}
	} /* end for loop */

	/* check for mandatory entries */
	if ((user_obj + group_obj + class_obj + other_obj) != NACLBASE)
		return(-1);
	/*
	 * Check file owning group & class permissions with only
	 * mandatory entries.
	 */
	if (!(users + groups) && group_perms != class_perms)
		return(-1);
	/* check default file owning group & class combination */
	if (def_group_obj && !(def_users + def_groups)) {
		if (!def_class_obj)
			return (-1);
		if (def_group_perms != def_class_perms)
			return (-1);
	}

	return(0);
}

/*
 * isort()	- insertion sort for ACL entries
 *
 *	Arguments:
 *		aclbufp		- ptr to ACL entries
 *		nentries	- number of ACL entries
 *
 */

static void
isort(aclbufp, nentries)
	struct acl		*aclbufp;
	int			nentries;
{
	register int		i;
	register int		j;
	register struct acl	*aclp;
	register struct acl	*low_aclp;
	struct acl		*start_aclp;
	struct acl		tmp_acl;

	if (nentries > 1) {
		for (i = nentries; i > 1; i--) {
			start_aclp = aclbufp + (nentries - i);
			low_aclp = start_aclp;
			aclp = start_aclp + 1;
			for (j = i; j > 1; j--, aclp++)
				if (aclcmpfcn(aclp, low_aclp) < 0)
					low_aclp = aclp;
			if (low_aclp != start_aclp) {
				tmp_acl = *start_aclp;
				*start_aclp = *low_aclp;
				*low_aclp = tmp_acl;
			}
		}
	}
}

/*
 * aclcmpfcn()	- ACL comparison function used by isort()
 *
 *	Arguments:
 *		e1	- ptr to ACL entry 1
 *		e2	- ptr to ACL entry 2
 *
 *	Return value:
 *		neg	- ACL entry 1 must be ahead of ACL entry 2
 *		zero	- duplicate entries
 *		pos	- ACL entry 1 must be behind ACL entry 2
 *
 *	Algorithm:
 *		If the types are the same
 *			sort by id
 *		Else
 *			sort by type
 *
 *	Notes:
 *		1. Although the number of checks are comparable with two
 *		   separate sorts, the exchanges are less.
 *
 */

static int
aclcmpfcn(e1, e2)
	register struct acl	*e1;
	register struct acl	*e2;
{
	return(e1->a_type == e2->a_type
		? e1->a_id - e2->a_id
		: e1->a_type - e2->a_type);
}
