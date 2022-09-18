/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef TSClist_h
#define TSClist_h
#ident	"@(#)debugger:inc/common/TSClist.h	1.3"

#include "Ev_Notify.h"
#include "Proctypes.h"
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/procfs.h>

// special system call exits - always trapped by debug
#define	SPECIAL_EXIT(I) 	(((I) == SYS_fork) || \
				    ((I) == SYS_vfork) ||\
				    ((I) == SYS_exec) ||\
				    ((I) == SYS_execve))
class	Event;
class	List;

struct TSCevent {
	int		tsc;
	NotifyEvent	*entry_events;
	NotifyEvent	*exit_events;
	TSCevent	*next_tsc;
			TSCevent(int sys) { tsc = sys; entry_events = 0;
				exit_events = 0; next_tsc = 0; }
			~TSCevent();
};

class TSClist {
	sys_ctl		entrymask;
	sys_ctl		exitmask;
	TSCevent	*_events;
	TSCevent	*lookup(int);
	TSCevent	*find(int);
public:
			TSClist();
			~TSClist();
	int		add(int sys, int exit, Notifier, void *);
	int		remove(int sys, int exit, Notifier, void *);
	sys_ctl		*tracemask(int exit = 0);
	NotifyEvent 	*events(int sys, int exit = 0);
};

enum Systype {
	NoSType = 0,
	Entry,
	Exit,
	Entry_exit
};

struct syscalls
{
	const char	*name;
	int		entry;
};

extern syscalls systable[];

#endif
// end of TSClist.h
