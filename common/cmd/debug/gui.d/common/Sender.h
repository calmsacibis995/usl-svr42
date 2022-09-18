/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SENDER_H
#define	_SENDER_H
#ident	"@(#)debugger:gui.d/common/Sender.h	1.2"

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define COMMAND_SENDER	Command_sender
#else
#define COMMAND_SENDER
#endif

class Message;
class Window_set;

// Command_sender is the base class of any framework object that
// sends a command to the debugger.  When the debugger responds,
// the Dispatcher uses get_window_set and de_message to direct
// the message to the right place.  de_message may be called more
// than once per command.  The Dispatcher will call cmd_complete
// when it receives the final message (MSG_cmd_complete)

class Command_sender
{
protected:
	Window_set	*window_set;

public:
			Command_sender(Window_set *ws)	{ window_set = ws; }
			~Command_sender() {}

			// access functions
	Window_set	*get_window_set()	{ return window_set; }

			// handle the incoming message
	virtual void	de_message(Message *);
	virtual void	cmd_complete();
};

#endif	// _SENDER_H
