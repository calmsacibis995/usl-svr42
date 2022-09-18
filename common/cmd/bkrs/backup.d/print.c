/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/backup.d/print.c	1.4.5.2"
#ident  "$Header: print.c 1.2 91/06/21 $"

#include	<time.h>
#include	<stdio.h>
#include	<signal.h>
#include	<backup.h>
#include	<bkmsgs.h>

extern int bklevels;

void
pr_estimate( data )
bkdata_t *data;
{
	register nvolumes = data->estimate.volumes;
	register nblocks = data->estimate.blocks;

	if( nvolumes >= 0 ) {
		if( nblocks >= 0 )
			(void) fprintf( stdout,
				"%s: %d volume%s, %d block%s\n",
					data->estimate.method_id.tag,
					nvolumes, (nvolumes == 1? "": "s"),
					nblocks, (nblocks == 1? "": "s") );
		else
			(void) fprintf( stdout,
				"%s: %d volume%s\n",
					data->estimate.method_id.tag,
					nvolumes, (nvolumes == 1? "": "s") );
	} else if( nblocks >= 0 )
		(void) fprintf( stdout,
			"%s: %d block%s\n",
				data->estimate.method_id.tag,
				nblocks, (nblocks == 1? "": "s") );
}

void
pr_failure( data )
bkdata_t *data;
{
	BEGIN_CRITICAL_REGION;
	if( data->failed.method_id.tag && data->failed.method_id.tag[0] )
		(void) fprintf( stderr, "Tag: %s: ", data->failed.method_id.tag );

	if( data->failed.errmsg[ 0 ] )
		(void) fprintf( stderr, "%s", data->failed.errmsg );

	else if( data->failed.reason )
		(void) fprintf( stderr, "Failed (%d)", data->failed.reason );

	(void) fprintf( stderr, "\n" );
	END_CRITICAL_REGION;
}
