/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/secure/llib-llpsec.c	1.3.1.3"
#ident	"$Header: $"

/* LINTLIBRARY */

# include	"secure.h"

/**
 ** getsecure() - EXTRACT SECURE REQUEST STRUCTURE FROM DISK FILE
 **/

SECURE * getsecure ( char * file )
{
    static SECURE * _returned_value;
    return _returned_value;
}

/**
 ** putsecure() - WRITE SECURE REQUEST STRUCTURE TO DISK FILE
 **/

int putsecure ( char * file, SECURE * secbufp )
{
    static int _returned_value;
    return _returned_value;
}

void freesecure ( SECURE * secbufp )
{
}
