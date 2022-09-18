/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Breaklist_h
#define Breaklist_h
#ident	"@(#)debugger:inc/common/Breaklist.h	1.1"

#include "Iaddr.h"
#include "Ev_Notify.h"
#include "Machine.h"

class Breakpoint;

// A Breaklist is maintained as a simple singly linked list
// sorted in order by address.

class Breaklist {
	Breakpoint	*root;
	void		dispose(Breakpoint *);
public:
			Breaklist() { root = 0; }
			~Breaklist();
	Breakpoint	*add( Iaddr, Notifier , void *);
	Breakpoint 	*remove( Iaddr, Notifier, void *); // remove just event
	int 		remove( Iaddr );
	Breakpoint	*lookup( Iaddr );
	Breakpoint	*first() { return root; }
};


class Breakpoint {
	Breakpoint	*_left;
	Breakpoint	*_right;
	Iaddr		_addr;
	char		_oldtext[BKPTSIZE];
	int		_flags;
	NotifyEvent 	*_events;
	friend class	Breaklist;
	friend class	LWP;
public:
			Breakpoint();
			Breakpoint( Iaddr , Notifier, void *);
			~Breakpoint();
	void		copy(Breakpoint &);
			// Access functions
	Iaddr		addr()    { return _addr; }
	char *		oldtext() { return _oldtext; }
	NotifyEvent	*events() { return _events; }
	Breakpoint	*left()   { return _left; }
	Breakpoint	*right()  { return _right; }
};

#endif


// end of Breaklist.h

