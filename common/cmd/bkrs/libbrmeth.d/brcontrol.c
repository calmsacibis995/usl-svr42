/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/libbrmeth.d/brcontrol.c	1.3.5.2"
#ident  "$Header: brcontrol.c 1.2 91/06/21 $"

#include	<signal.h>
#include	<bkrs.h>

extern int brtype;
extern int bkcancel();
extern int bksuspend();

int
brcancel()
{
	switch( brtype ) {
	case BACKUP_T:
		return( bkcancel() );
	case RESTORE_T:
		return( BRSUCCESS );
	default:
		return( BRNOTINITIALIZED );
	}
}

int
brsuspend()
{
	switch( brtype ) {
	case BACKUP_T:
		return( bksuspend() );
	case RESTORE_T:
		return( BRSUCCESS );
	default:
		return( BRNOTINITIALIZED );
	}
}
