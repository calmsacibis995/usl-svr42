/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ifndef PLACE_H
#define PLACE_H
#ident	"@(#)debugger:inc/common/Place.h	1.2"

#include "Iaddr.h"
#include "Reg.h"

/*
// NAME
//	Place (generalized address class)
//
// ABSTRACT
//	Place provides a generalized way to specify the location
//	in memory or register of a particular data item.
//
// DATA
//	kind		one of (pRegister, pAddress, pDebugvar)
//	(union)		the actual register number or address 
//
*/

class Debug_var;

enum PlaceMark { // Note: enum values determine order of respective kinds.
    pRegister,
    pAddress,
    pDebugvar,
    pUnknown
};

struct Place {
    PlaceMark kind;
    union {
	RegRef reg;
	Iaddr  addr;
	Debug_var*  var;
    };
    void null()		{ kind = pUnknown; }
    int  isnull()	{ return kind == pUnknown; }
    Place()  		{ null(); }
};

#endif
