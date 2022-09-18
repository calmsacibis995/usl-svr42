/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _Msgtab_h
#define _Msgtab_h
#ident	"@(#)debugger:inc/common/Msgtab.h	1.1"

#include "Msgtypes.h"
#include "Signature.h"
#include "Msgtable.h"

class Message_table
{
	struct Msgtab	*msgtab;

public:
			Message_table();
			~Message_table(){}

	const char	*format(Msg_id);
#ifndef NOCHECKS
	Signature	signature(Msg_id);
	Msg_class	msg_class(Msg_id);
#else
	Signature	signature(Msg_id m)	{ return msgtab[m].signature; }
	Msg_class	msg_class(Msg_id m)	{ return msgtab[m].mclass; }
#endif	// NOCHECKS
};

extern Message_table	Mtable;

#endif // _Msgtab_h
