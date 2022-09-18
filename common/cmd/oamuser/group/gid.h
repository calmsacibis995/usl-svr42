/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:group/gid.h	1.1.1.2"
#ident  "$Header: gid.h 2.0 91/07/13 $"


/*
 * The gid_blk structure is used in the search for the default
 * uid.  Each gid_blk represent a range of gid(s) that are currently
 * used on the system.
*/
struct	gid_blk { 			
	struct	gid_blk	*link;
	gid_t		low;		/* low bound for this uid block */
	gid_t		high; 		/* high bound for this uid block */
};

