/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _UTIL_ERR_H	/* wrapper symbol for kernel use */
#define _UTIL_ERR_H	/* subject to change without notice */

#ident	"@(#)uts-x86:util/err.h	1.4"
#ident	"$Header: $"
/*
 * structure of the err buffer area
 */
#define	NESLOT	20
#define	E_LOG	01
#define	E_SLP	02

struct err {
	int		e_nslot;		/* number of errslots */
	int		e_flag;			/* state flags */
	struct errhdr	**e_org;		/* origin of buffer pool */
	struct errhdr	**e_nxt;		/* next slot to allocate */
	struct errslot {
		int	slot[8];
	} e_slot[NESLOT];			/* storage area */
	struct map	e_map[(NESLOT+3)/2];	/* free space in map */
	struct errhdr	*e_ptrs[NESLOT];	/* pointer to logged errors */
};

extern struct err err;

struct errhdr	*geteslot();
struct errhdr	*geterec();

#endif	/* _UTIL_ERR_H */
