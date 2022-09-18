/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:user/uidage.h	1.1.7.2"
#ident  "$Header: uidage.h 2.0 91/07/13 $"


#define	ADD	 	  1
#define	CHECK	 	  2
#define	REM	 	  3
#define	UID_MIN		100
#define	BUFSIZE 	256

#define UIDAGEF		"/etc/security/ia/ageduid"
#define UATEMP		"/etc/security/ia/autemp"
#define OUIDAGEF	"/etc/security/ia/oageduid"

struct uidage {
	uid_t	uid;	/* uid being aged */
	long	age;	/* date when uid becomes available */
	};


/*
 * The uid_blk structure is used in the search for the default
 * uid.  Each uid_blk represent a range of uid(s) that are currently
 * used on the system.
*/
struct	uid_blk { 			
	struct	uid_blk	*link;
	uid_t		low;		/* low bound for this uid block */
	uid_t		high; 		/* high bound for this uid block */
};

