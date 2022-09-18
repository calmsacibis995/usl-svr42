/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libTL:common/lib/libTL/TLclose.c	1.4.5.2"
#ident  "$Header: TLclose.c 1.2 91/06/25 $"
#include <table.h>
#include <internal.h>

extern tbl_t TLtables[];
void TLt_free();

int
TLclose( tid )
int tid;
{
	void TLt_unget();
	/* Initialize TLlib, if needed */
	TLinit();

	tid--;
	if( !TLt_valid( tid ) )	return( TLBADID );

	/* Write out new table */
	/*
	if( rc = TLt_sync( tid ) ) return( rc );
	*/

	/* Free memory associated with table structure */
	TLt_free( tid );

	/* Free table structure slot */
	TLt_unget( tid );

	return( TLOK );
}

