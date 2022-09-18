/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _Manager_h
#define _Manager_h
#ident	"@(#)debugger:inc/common/Manager.h	1.1"

#include "Severity.h"
#include "Msgtypes.h"

class LWP;

// The Message Manager is the link between the user interface and
// the rest of the debugger.  The base class defines the command
// line interface, since that is always needed.  Other classes
// are derived from this to deal with other user interfaces

class MessageManager
{
public:
	virtual	int	send_msg(Msg_id, Severity ...); // from debug to ui
	virtual int	docommand();		// execute one user cmd
	virtual void	sync_request();		// sync with the ui
	virtual void	reset_context(LWP *);	// set context for new LWP's
};

extern MessageManager	*message_manager;
extern MessageManager	cli_manager;

#endif // _Manager_h
