/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libTL:common/lib/libTL/space.c	1.3.5.2"
#ident  "$Header: space.c 1.2 91/06/25 $"
#include <table.h>
#include <internal.h>
#include <setjmp.h>

tbl_t TLtables[ TL_MAXTABLES ];
int TLinitialized = 0;
unsigned char TLinbuffer[ 2 * TL_MAXLINESZ + 1 ];
unsigned char TLgenbuf[ 2 * TL_MAXLINESZ + 1 ];
jmp_buf TLenv;
