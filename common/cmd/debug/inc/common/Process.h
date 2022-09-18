/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Process_h
#define Process_h
#ident	"@(#)debugger:inc/common/Process.h	1.1"

#include "Link.h"
#include "Machine.h"
#include <sys/types.h>

class	LWP;
class	Proccore;
class	Procctl;
class	Program;
class	Seglist;

// Process control.
// Handles operations common to a single address space.
// Operations specific to a thread-of-control within
// the address space are handled by LWPs.

class Process : public Link {
	short		next_lwp; 
	short		lwp_cnt;  // number of live lwps
	pid_t		ppid;
	Seglist		*seglist;
	Proccore 	*core;
	Procctl		*textctl;
	LWP		*_first_lwp;
	LWP		*last_lwp;
	Program		*_program;
	const char	*procname;

	friend class	LWP;
public:
			Process( LWP *, pid_t, const char *);
			~Process();
	Process 	*next()	{ return (Process*)Link::next(); }
	Process 	*prev()	{ return (Process*)Link::prev(); }
	void		add_lwp(LWP *);
	void		remove_lwp(LWP *);
	LWP		*first_lwp();
	//		Access functions
	pid_t		pid()		{ return ppid; }
	LWP		*lwp_list()	{ return _first_lwp; }
	Program		*program()	{ return _program; }
	const char	*proc_name()	{ return procname; }
	int		count()	{ return lwp_cnt; }
	int		is_core() { return ((ppid == -1) &&
					    (core != 0)); }
};

#endif

// end of Process.h
