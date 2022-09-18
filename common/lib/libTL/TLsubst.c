/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libTL:common/lib/libTL/TLsubst.c	1.2.5.2"
#ident  "$Header: TLsubst.c 1.2 91/06/25 $"
#include <table.h>
#include <internal.h>

extern tbl_t TLtables[];

int
TLsubst( tid, entry, fieldname, pattern, replace )
int tid;
entry_t *entry;
unsigned char *fieldname, *pattern, *replace;
{
	register rc;

	/* Initialize TLlib, if needed */
	TLinit();

	tid--;
	if( !TLt_valid( tid ) )	return( TLBADID );
	if( !entry || !fieldname ) return( TLARGS );
	if( TLe_diffformat( tid, entry ) ) return( TLDIFFFORMAT );

	if( (rc = TLs_replace( tid, entry, fieldname, pattern, replace )) == TLOK )
		TLtables[ tid ].status |= MODIFIED;
	return( rc );
}
