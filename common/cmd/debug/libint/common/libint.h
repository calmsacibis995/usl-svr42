/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libint/common/libint.h	1.2"

// This file contains declarations that are used by several files within libint

#include	"Link.h"
#include	<stdio.h>

// OutPut is the stack of nested (via redirection) output files
// curoutput is the top of the stack
struct	OutPut : public Link {
	FILE *		fp;
};
extern	OutPut		*curoutput;

// transport is the transport mechanism, needed only if the gui is invoked
class	Transport;
extern	Transport	*transport;

// log_msg writes the formatted message to the log file
// the output is the same for either the cli or the gui
#include "Msgtypes.h"
#include "Severity.h"
extern	void		log_msg(int length, Msg_id, Severity ...);
