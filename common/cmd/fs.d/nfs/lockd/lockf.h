/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nfs.cmds:nfs/lockd/lockf.h	1.6.5.3"
#ident	"$Header: $"

struct data_lock {
	struct filock	filocks;
	int		granted,        /* The granted flag                */
			color,          /* Used during deadlock search     */
			LockID,         /* ID of this lock                 */
			class;          /* Class of lock (FILE,IO,NET)     */
        };

#define END(l)          ((l)->l_start + (l)->l_len - 1)
 
/*
 * Is TRUE if a is completely contained within b
 */
#define WITHIN(a,b) (((a)->l_start >= (b)->l_start) && (END(a) <= END(b)))

int local_state;
