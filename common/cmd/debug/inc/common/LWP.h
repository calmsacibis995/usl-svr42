/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef LWP_h
#define LWP_h
#ident	"@(#)debugger:inc/common/LWP.h	1.5.1.1"

// Light-Weight Process control.
// Each LWP points to its containing process and is
// a member of the list of LWPs for that process.
//
// Some operations happen at the LWP level.  Some are
// simply passed up to the process level.
//
// Some program and process-specific data is duplicated at the LWP level
// for efficiency.

#include "Iaddr.h"
#include "Link.h"
#include "Stmt.h"
#include "Instr.h"
#include "Breaklist.h"
#include "RegAccess.h"
#include "Symbol.h"
#include "TSClist.h"
#include "Frame.h"
#include "Ev_Notify.h"
#include "Process.h"
#include <sys/types.h>

struct	EventTable;
class	Event;
class	HW_Watch;
class	NameEntry;
class	Proclive;
class	PtyInfo;
class	Rvalue;
class	Seglist;
class	Symtab;

enum Execstate {
	es_none,
	es_corefile,
	es_dead,
	es_stepping,
	es_running,
	es_stepped,
	es_suspended,
	es_signalled,
	es_breakpoint,
	es_syscallent,
	es_syscallxit,
	es_watchpoint,
};

// Primary and secondary goals.  Primary goals are the
// result of some user request, like stmt step.  Secondary
// goals may be different actions taken to achieve primary
// goals, like stepping over a breakpoint in order to run.

enum Goal1 {
	pg_run,
	pg_stmt_step,
	pg_instr_step,
	pg_stmt_step_over,
	pg_instr_step_over,
};

enum Goal2 {
	sg_run,
	sg_step,
	sg_stepbkpt,
};

// definitions for stepping
#define STEP_INF_ANNOUNCE	-1	// step forever and announce
#define STEP_INF_QUIET		-2	// step forever quietly
#define	STEP_INTO		0
#define	STEP_OVER		1
#define STEP_STMT		0
#define STEP_INSTR		1

// definitions for create I/O redirection
#define	REDIR_IN	1
#define	REDIR_OUT	2
#define	REDIR_PTY	4

// definitions for proto process creation
#define P_KILL		0
#define P_RELEASE	1
#define P_EXEC		2

// returned by set_watchpoint
#define WATCH_FAIL	0
#define WATCH_SOFT	1
#define WATCH_HARD	2

// mode parameter to state_check()
#define	E_DEAD		1
#define E_CORE		2
#define E_RUNNING	4

// flags
#define	L_IS_CHILD	0x1
#define	L_GRABBED	0x2
#define	L_GO_PAST_RTL	0x4
#define	L_IN_START	0x8
#define	L_CHECK		0x10
#define	L_WAITING	0x20
#define	L_IGNORE_EVENTS	0x40
#define	L_IGNORE_FORK	0x80

class LWP : public Link {
	char		exec_cnt; // number of times we go through exec
	char		verbosity;
	short		flags;
	short		sw_watch;
	short		ecount;	  // number of single steps requested

#ifdef THREADS
	const char	*lname;
#endif
	const char	*procname;	// really process specific
	const char	*progname;	// really program specific
	const char	*ename; 	// really program specific

	Proclive	*pctl;
	EventTable 	*etable;
	Process		*_process;
	Seglist		*seglist; // really process specific

	Iaddr		pc;	// current location, constant while stopped
	Iaddr		dot;	// used by dis, may change
	Iaddr		lopc;	// keep track of current function
	Iaddr		hipc;
	Iaddr		startaddr;	// first location in a stmt
	Symtab		*last_sym;
	Symbol		last_src;
	Goal1		goal;
	Goal2		goal2;
	Breakpoint 	*latestbkpt;
	int		latestsig;
	int		latesttsc;
	int		latestflt;
	unsigned long	epoch;	// keep track of whether state has changed
	char		*latestexpr;
	Iaddr		retaddr;
	Instr		instr;
	Stmt		startstmt;
	Stmt		currstmt;
	Breakpoint	hoppt;	// for step over calls
	Breakpoint	destpt;	// destination of run
	Breakpoint	dynpt;	// dynamic linker debug trap
	RegAccess	regaccess;
	Frame 		*cur_frame;
	Frame 		*top_frame;
	HW_Watch	*hw_watch;
	char 		*current_srcfile;
	Execstate	state;

	Event		*firstevent;  // first assigned for this LWP

	int		inform_startup(int why);
	int		inform_run();
	int		inform_step();
	int		inform_stepbkpt();

	int		start(Goal2);
	int		restart();

	int		check_stmt_step();
	int		check_instr_step();
	int		check_watchpoints();
	void		check_onstop();

	int		respond_to_sig();
	int		respond_to_sus();
	int		respond_to_bkpt();
	int		respond_to_tsc();
	int		respond_to_dynpt();

	int		set( Breakpoint &, Iaddr );
	int		remove( Breakpoint & );
	int		insert_bkpt( Breakpoint * );
	int		lift_bkpt( Breakpoint *, int other_lwp = 0 );
	int		lift_all_bkpt(Breakpoint *, int other_lwp);
	int		insert_all_bkpt(Breakpoint *);
	void		remove_all_bkpt(Breakpoint *);

	int		default_traps();

	int		find_stmt( Stmt &, Iaddr );
	int		find_cur_src();
	void		stateinfo(int level, const char *entryname, 
				const char *filename);

	int		use_et( EventTable * );
	int		copy_et(LWP *, int mode);
	int		cleanup_et(int mode, int delete_events);
	
	int		grab_fork(LWP *, int procnum, pid_t);
	int		proc_exec();
	int		proc_fork();
	int		create_exec();
	int		setup_name(const char *, int procnum, 
				int use_obj, time_t &, char *old = 0);
	int		setup_data(int use_obj);
	void		setup_new();
public:
			LWP();
			LWP(int tfd, int cfd, int pnum, const char *ename);
			~LWP();

	LWP 		*next()	{ return (LWP*)Link::next(); }
	LWP	 	*prev()	{ return (LWP*)Link::prev(); }

	//		Access functions
	Iaddr		pc_value()	{ return pc; }
	unsigned long	p_epoch()	{ return epoch; }
#ifdef THREADS
	const char	*lwp_name()	{ return lname; }
#else
	const char	*lwp_name()	{ return procname; }
#endif
	const char	*proc_name()	{ return procname; }
	const char	*prog_name()	{ return progname; }
	const char	*exec_name()	{ return ename; }
	int		is_grabbed()	{ return (flags & L_GRABBED); }
	int		is_child()	{ return (flags & L_IS_CHILD); }
	Process		*process()	{ return _process; }
	Proclive	*proc_ctl()	{ return pctl; }
	Execstate	get_state() 	{ return state; }
	Event		*first_event() 	{ return firstevent; }
	Iaddr		dot_value()	{ return dot; }
	EventTable	*events()	{ return etable; }
	char		*curr_src()	{ return current_srcfile; }
	Symbol		current_source(){ return last_src; }
	int		check()		{ return (flags & L_CHECK); }
	int		waiting()	{ return (flags & L_WAITING); }
	Instr		*instruct()	{ return &instr; }
	void		rename(const char *s) { progname = s; }
	void		set_dot(Iaddr val) { dot = val; }
	void		set_check() { flags |= L_CHECK; }
	void		clear_check() { flags &= ~L_CHECK; }
	void		set_wait() { flags |= L_WAITING; }
	void		clear_wait() { flags &= ~L_WAITING; }
	void		markdead()  { state = es_dead; 
				_process->lwp_cnt--; }
	void		set_expr(char *e) { latestexpr = e; }
	void		set_ignore()  { flags |= L_IGNORE_EVENTS; }
	void		clear_ignore()  { flags &= ~L_IGNORE_EVENTS; }

	int		create(char *, int pnum, int in, int out, 
				int redir, int id, int on_exec = 0,
				int ignore_fork = 0);
	int		grab(int pnum, char *, char *loadfile, int id, 
				int ignore_fork = 0);
	int		make_proto(int);

	int		inform(int what, int why);
	int		state_check(int mode);
	int		run( int clearsig, Iaddr dest, int talk_ctl);
	int		drop_run();
	int		stop();
	int		stop_for_event(int);
	int		instr_step( int where, int clearsig, 
				Iaddr dest, int cnt, int talk_ctl );
	int		stmt_step( int where, int clearsig, 
				Iaddr dest, int cnt, int talk_ctl );
	int		resume( int clearsig);

	int		setframe( Frame * );
	Frame 		*curframe();
	Frame 		*topframe();

	int		read( Iaddr, int len, char *);
	int		write( Iaddr, void *, int len );
	int		read( Iaddr, Stype, Itype & );
	int		write( Iaddr, Stype, const Itype & );

	int		disassemble( Iaddr, int symbolic, Iaddr * next);

	int		show_current_location( int srclevel, int show);

	Symtab 		*find_symtab( Iaddr );
	Symtab 		*find_symtab( const char * );
	int		find_source( const char *, Symbol & );
	Symbol		find_entry( Iaddr );
	Symbol		find_symbol( Iaddr );
	Symbol		find_scope( Iaddr );
	Iaddr		first_stmt(Iaddr);
	Symbol		find_global( const char * );
	Symbol		next_global();
	Symbol		prev_global();
	Symbol		current_function();
	Symbol		first_file();
	Symbol		next_file();
	const char	*object_name( Iaddr );

	int		set_current_stmt( const char *, long line);
	long		current_line();

	void		add_event(Event *);
	void		remove_event(Event *);

	char 		*text_nobkpt( Iaddr );
	Breakpoint	*set_bkpt( Iaddr, Notifier , void *);
	int		remove_bkpt( Iaddr, Notifier, void *);

	int		set_watchpoint(Iaddr, Rvalue *, Notifier, void *);
	int		remove_watchpoint(int hw, Iaddr, Notifier, void *);
	void		disable_soft() { if (sw_watch > 0) sw_watch--; }
	void		enable_soft() { sw_watch++; }

	int		set_sig_catch( sigset_t, Notifier, void * );
	int		remove_sig( sigset_t, Notifier, void * );
	int		cancel_sig( int sig );
	int		cancel_sig_all();
	int		pending_sigs();

	int		set_sys_trace( int call, Systype, Notifier, void * );
	int		remove_sys_trace( int call, Systype, Notifier, void * );

	Iaddr		getreg( RegRef );
	int		readreg( RegRef, Stype, Itype & );
	int		writereg( RegRef, Stype, Itype & );
	int		display_regs(Frame *);
	int		set_pc(Iaddr);

	int		print_map();

	char 		*symbol_name( Symbol );	// get symbol name

	int		in_stack( Iaddr );
	int		in_text( Iaddr );
};

#endif
// end of LWP.h
