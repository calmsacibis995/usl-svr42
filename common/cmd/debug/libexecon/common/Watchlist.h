/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Watchlist_h
#define Watchlist_h
#ident	"@(#)debugger:libexecon/common/Watchlist.h	1.1"

#include "Iaddr.h"

class HW_Wdata;	// opaque to clients
class LWP;
class Rvalue;

class HW_Watch {
	HW_Wdata	*hwdata;
public:
			HW_Watch();
			~HW_Watch();
	int		add(Iaddr, Rvalue *, LWP *, void *);
	int		remove(Iaddr, LWP *, void *);
	int		hw_fired(LWP *); // did a hw watchpoint fire?
};


#endif // end of Watchlist.h

