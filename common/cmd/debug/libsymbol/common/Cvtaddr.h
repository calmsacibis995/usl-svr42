/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libsymbol/common/Cvtaddr.h	1.1"

// machine specific routines to convert a COFF symbol's value
// into a location description

void cvt_arg( Locdesc &loc, unsigned long value );
void cvt_reg( Locdesc &loc, unsigned long value );
void cvt_auto( Locdesc &loc, unsigned long value );
void cvt_extern( Locdesc &loc, unsigned long value );
