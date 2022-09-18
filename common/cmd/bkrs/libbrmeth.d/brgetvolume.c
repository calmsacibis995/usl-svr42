/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/libbrmeth.d/brgetvolume.c	1.4.5.2"
#ident  "$Header: brgetvolume.c 1.2 91/06/21 $"

#include <bkrs.h>

extern int brtype;

extern int bkgetvolume();

int
brgetvolume( volume, override, automated, result )
char *volume, *result;
int override, automated;
{
	switch( brtype ) {
	case BACKUP_T:
		return( bkgetvolume( volume, override, automated, result ) );
		/*NOTREACHED*/
		break;
	
	case RESTORE_T:
		break;

	default:
		return( BRNOTINITIALIZED );
	}

	return( BRSUCCESS );
}
