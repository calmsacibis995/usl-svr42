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
#ifndef TRIGGERITEM_H
#define TRIGGERITEM_H

#ident	"@(#)debugger:inc/common/TriggItem.h	1.2"

// TriggerItem is used to pass information about event expression
// between the expression evaluator and the event handler
// When a triggerItem is created, Place and Rvalue structures
// are allocated if possible.

#include "Place.h"
#include "Rvalue.h"
#include "Frame.h"
#include "ParsedRep.h"
#include "Iaddr.h"

#define NULL_SCOPE ((Iaddr)-1)

class LWP;

class TriggerItem
{
	ParsedRep *node;	// ptr to root of expression (sub) tree ...
				//   ...describing this triggerItem
	int reinitLval;

    public:
	Iaddr	scope;		// if local variable,start address of ...
				//   ... enclosing routine; otherwise null
	LWP	*lwp;		// item's process
	FrameId	frame;		// stack frame if automatic and ...
				//   ... active (on the stack); otherwise null
	TriggerItem()
		{	scope = NULL_SCOPE;
			lwp = 0;
			frame.null();
			node = 0;
			reinitLval = 0;
		}
	TriggerItem(TriggerItem &t)
		{	lwp = t.lwp;
			scope = t.scope;
			frame = t.frame;
			node = t.node;
			reinitLval = t.reinitLval ;
		}

	~TriggerItem() {}

	void copyContext(TriggerItem &ti)
		{
			lwp = ti.lwp;
			frame = ti.frame;
			scope = ti.scope;
		}
	void setGlobalContext(LWP *l)
		{
			lwp = l;
			frame.null();
			scope = NULL_SCOPE;
		}
	ParsedRep* getNode()
		{ return node; }
	void setNode(ParsedRep *e)
		{ node = e; }
	int  reinitOnChange()
		{ return reinitLval; }
	void setReinit()
		{ reinitLval++; }
	int getTriggerLvalue( Place& lval)
		{ return node->getTriggerLvalue(lval); }
	int getTriggerRvalue(Rvalue& rval)
		{ return node->getTriggerRvalue(rval); }

};

#endif /* TRIGGERITEM_H */
	
