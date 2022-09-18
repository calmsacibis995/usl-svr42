/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _Proglist_h
#define _Proglist_h

#ident	"@(#)debugger:inc/common/Proglist.h	1.2"

// Access to lists of programs, processes and lwps.
// Finds a lwp, process, program structures given
// program names or process or lwp ids.
// Also parses lists of program names and process and lwp ids
// and returns a list of associated lwps.
// Allows for maitaining multiple lists at once to avoid race
// conditions with associated event commands.
// 

#include <sys/types.h>

class	LWP;
class	Process;
class	Program;
struct	Proclist;

// structure returned by proc_list
struct plist {
	int	p_type;
	LWP	*p_lwp;
};

// Object types used in proc_list;
// report type of request - a single lwp,
// all lwps in a process, all lwps in a program, etc.
#define PLWP	1
#define PPROC	2
#define PEXEC	4
#define PCORE	8


class	Proglist {
	Program		*first_program;
	Program		*last_program;
	Program		*curr_program;
	Process		*curr_process;
	LWP		*curr_lwp;
	plist		*lwp_list;
	plist		*cur_plist;
	plist		*plist_head;	// head of the current list
	int		list_size;
	int		user_list;
	int		cnt;		// number of lwps in lwp_list
	const char	*all_name;	// str versions of "all", %lwp, etc.
	const char	*proc_name;	// to make strcmps faster
	const char	*prog_name;
	const char	*lwp_name;
	Program		**protolist; 	// list of proto programs
	Program		**cur_proto_list;
	int		proto_list_size;
	int		proto_cnt;
	int		proc_num;
	void 		parse_item(char *, int proto);
	int 		parse_name(char *, const char *&, 
				const char *&);
	void 		grow_list();
	void 		grow_proto_list();
	void		setup_list();
	void		add_prog(Program *, int mode); 
	void 		add_proc(Process *, int type);
	void 		add_lwp(Process *, LWP *, const char *);
public:
			Proglist();
			~Proglist() { delete lwp_list; delete protolist; }
	plist		*proc_list(Proclist *);
	plist		*proc_list(Program *); // all lwps in given program
	plist		*proc_list(Proclist *, Program **&);
	plist		*all_live(); // all procs but no core files or proto
	plist		*all_live(int create_id); 
			// all live procs with given create id
	plist		*all_procs(Program **&); // all procs, core files, and
						// proto programs
	void		add_program(Program *);
	void		remove_program(Program *);
	void		reset_lists() { cur_plist = 0; } // forget all 
				// current lists
	void		prune();	// get rid of dead objects
	void		reset_current(int announce); // reset current lwp, proc, prog
	void		set_current(Program *, int announce);
	void		set_current(Process *, int announce);
	void		set_current(LWP *, int announce);
	Process		*find_proc(const char *);
	LWP		*find_lwp(char *);
	LWP		*find_lwp(pid_t);
	Program		*find_prog(const char *);
	plist		*make_list();
	void		add_list(LWP *);
	void		list_done() { user_list = 0; }
	//		Access functions
	Program		*prog_list() { return first_program; }
	Program		*current_program() { return curr_program; }
	Process		*current_process() { return curr_process; }
	LWP		*current_lwp()	 { return curr_lwp; }
	void		reset_prog(Program *p) { curr_program = p; }
	int		next_proc() { return ++proc_num; }
	void		dec_proc() { proc_num--; }
#ifdef DEBUG
	void		print_list();
#endif
};

extern	Proglist	proglist;

#endif	// _Proglist_h
