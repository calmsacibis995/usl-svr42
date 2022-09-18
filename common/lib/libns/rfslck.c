/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libns:common/lib/libns/rfslck.c	1.1"
#include	<stdio.h>
#include	<assert.h>
#include	<unistd.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<string.h>
#define	RFSLCKFILE	"/etc/rfs/rfslck"
#define	RFSLCKMODE	0600
#define RFSLCKMSG	"RFS administration now in progress.  Try again.\n"
static	rfslckfd = -1;
/*
 *	Return value:
 *		 0	got lock
 *		-1	did not get lock
 *		>0	some other problem
 */
int
setrfslck()
{
	int	ret;

	if( (rfslckfd = open(RFSLCKFILE, O_RDWR|O_CREAT, RFSLCKMODE)) == -1){
		ret = errno;
		(void)fprintf(stderr,"cannot open: %s: %s\n", 
			RFSLCKFILE, strerror(errno));
		return ret;
	}
		if( lockf(rfslckfd, F_TLOCK, (long)0) == -1 ){
		switch(errno) {
		case EAGAIN:
		case EACCES:
			(void)fprintf(stderr, RFSLCKMSG);
			return -1;
		default:
			ret = errno;
			(void)fprintf(stderr,"cannot lock: %s: %s\n", 
				RFSLCKFILE, strerror(errno));
			return ret;
		}
	}
	return 0;
}
void
clrrfslck()
{
	assert(rfslckfd > -1);
	(void)close(rfslckfd);
}
