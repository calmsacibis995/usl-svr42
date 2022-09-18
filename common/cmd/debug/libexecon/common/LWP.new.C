#ident	"@(#)debugger:libexecon/common/LWP.new.C	1.4.1.5"

#include "LWP.h"
#include "Program.h"
#include "Proglist.h"
#include "Process.h"
#include "Procctl.h"
#include "EventTable.h"
#include "Event.h"
#include "Interface.h"
#include "Machine.h"
#include "Manager.h"
#include "global.h"
#include "PtyList.h"
#include "Object.h"
#include "Seglist.h"
#include "str.h"
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>


// Constructor used for live processes
LWP::LWP() : instr(this)
{

	setup_new();
	state = es_none;
	goal = pg_run;
	goal2 = sg_run;
	pctl = new Proclive;
}

// Constructor used for proto and core files
// MORE - for now assume only 1 lwp
LWP::LWP( int tfd, int cfd, int pnum, const char *ename):
	instr(this)
{
	time_t		stime;
	Proccore	*core;
	Procctl		*textctl;
	int		go_past;

	setup_new();
	if (!setup_name(ename, pnum, 1, stime))
	{
		state = es_dead;
		return;
	}
	pctl = 0;
	_process = new Process(this, -1, procname);
	
	epoch = 0;
	state = es_corefile;
	seglist = new Seglist(_process);
	_process->seglist = seglist;
	if (!seglist->setup( tfd, go_past, ename ))
	{
		delete seglist;
		delete _process;
		state = es_dead;
		return;
	}
	core = new Proccore;
	textctl = new Procctl;
	if ((core->open(cfd) == 0) ||
		(textctl->open(tfd) == 0))
	{
		delete core;
		state = es_dead;
		return;
	}
	_process->core = core;
	regaccess.setup_core( core );
	regaccess.update();
	pc = regaccess.getreg( REG_PC );
	seglist->readproto( textctl, core, ename );
	top_frame = new Frame(this);
	cur_frame = top_frame;
	_process->_program = new Program(_process, ename, progname,
		core->psargs(), 0, stime, 0);
	find_cur_src();
}

// common actions from 2 constructors
void
LWP::setup_new()
{
	flags = 0;
	firstevent = 0;
	pc = lopc = hipc = 0;
	dot = 0;
	current_srcfile = 0;
	last_sym = 0;
	etable = 0;
	hw_watch = 0;
	sw_watch = 0;
	verbosity = vmode;
}

LWP::~LWP()
{
	if (pctl)
	{
		pctl->close();
		delete pctl;
	}
}

// Create pipeline
int
LWP::create(char *cmdline, int pnum, int input, 
	int output, int redir, int id, int on_exec, int ignore_fork)
{
	char		*execline = new char[ sizeof("exec ") +
				strlen(cmdline) + 3 ];
	pid_t		pid;
	PtyInfo		*child_io = 0;
	time_t		symfiltime;

	if (!setup_name(cmdline, pnum, 1, symfiltime))
	{
		delete execline;
		return 0;
	}
	strcpy( execline, "exec " );
	// if no path specified, always look in current directory
	if (strchr(ename, '/') == 0)
		strcat(execline, "./");
	strcat( execline, cmdline );
	if (redir & REDIR_PTY)
	{
		child_io = new PtyInfo;
		{
			if (child_io->is_null())
			{
				delete execline;
				return 0;
			}
		}
	}
	if ( (pid = fork()) == 0 )
	{
		// New child
		// Set up to stop on exec, set up I/O redirection
		// and exec new command.
		stop_self_on_exec();
		if (redir & REDIR_PTY)
			redirect_childio( child_io->pt_fd() );
		if (input >= 0)
		{
			if (redir & REDIR_IN)
			{
				int	fs = fcntl(0, F_GETFD, 0);
				close(0);
				fcntl(input, F_DUPFD, 0);
				if (fs == 1)
					fcntl(0, F_SETFD, 1);
			}
			close(input);
		}
		if (output >= 0)
		{
			if (redir & REDIR_OUT)
			{
				int	fs = fcntl(1, F_GETFD, 0);
				close(1);
				fcntl(output, F_DUPFD, 1);
				if (fs == 1)
					fcntl(1, F_SETFD, 1);
			}
			close(output);
		}
		// turn off lazy binding so we don't need to go through rtld
		// binding routines
		putenv("LD_BIND_NOW=1");
		execlp( "/usr/bin/sh", "sh", "-c", execline, 0 );
		delete child_io;
		_exit(1);  // use _exit to avoid flushing buffers
	}
	else if ( pid == -1 )
	{
		printe(ERR_fork_failed, E_ERROR, strerror(errno));
		state = es_dead;
		delete execline;
		return 0;
	}
	if ( pctl->open(pid, !ignore_fork) == 0 )
	{
		printe(ERR_fork_failed, E_ERROR, strerror(errno));
		kill(pid, SIGKILL);
		state = es_dead;
		delete execline;
		return 0;
	}

	exec_cnt = EXECCNT;
	if (ignore_fork)
		flags |= (L_IS_CHILD|L_IN_START|L_IGNORE_FORK);
	else
		flags |= (L_IS_CHILD|L_IN_START);

	_process = new Process(this, pid, procname);
	if (!setup_data(1))
	{
		kill(pid, SIGKILL);
		state = es_dead;
		delete execline;
		return 0;
	}
	if (on_exec)
		// stop on exec - don't go past dynamic linker
		flags &= ~L_GO_PAST_RTL;

	_process->seglist = seglist;

	// initial startup - do exec of sh and process
	_process->_program = new Program(_process, ename, progname, cmdline,
		child_io, symfiltime, id);
	while (flags & L_IN_START)
	{
		int	what, why;
		if ((pctl->wait_for(what, why) != p_stopped) ||
			(!inform(what, why)))
		{
			kill(pid, SIGKILL);
			state = es_dead;
			delete execline;
			proglist.remove_program(_process->_program);
			_process->_program = 0;
			return 0;
		}
	}
	return 1;
}

// one of our subjects has forked - control new process
int
LWP::grab_fork(LWP *op, int pnum, pid_t ppid)
{
	time_t		stime;
	char		*p;
	int		what, why;

	if (pctl->open(ppid, 1) == 0)
	{
		return 0;
	}

	if ((pctl->wait_for(what, why) != p_stopped) ||
		!setup_name(0, pnum, 0, stime))
	{
		run( 0, 0, 0 );
		state = es_dead;
		return 0;
	}

	p = new(char[strlen(op->ename)+1]);
	strcpy( p, op->ename);
	ename = p;
	progname = op->progname;
	flags |= (L_IS_CHILD|L_GRABBED);
	exec_cnt = 0;
	flags &= ~L_IN_START;
	state = es_suspended;
	_process = new Process(this, ppid, procname);
	if (!setup_data(0))
	{
		run(0, 0, 0);
		state = es_dead;
		return 0;
	}
	flags &= ~L_GO_PAST_RTL;
	_process->seglist = seglist;
	regaccess.update();
	seglist->build( pctl, ename);
	seglist->update_stack( pctl );
	pc = regaccess.getreg( REG_PC );
	pc = instr.adjust_pc();
	if (!copy_et(op, E_PROGRAM))
	{
		run(0, 0, 0);
		state = es_dead;
		return 0;
	}
	_process->_program = op->_process->_program;
	_process->_program->add_proc(_process);
	etable->prog = _process->_program;
	return 1;
}

// grab already running process - not created by one of our subjects
// MORE - assume for now only 1 lwp
int
LWP::grab(int pnum, char *path, char *loadfile, int id, int ignore_fork)
{
	char		*args;
	char		*s, *p;
	time_t		symfiltime;
	pid_t		ppid;

	s = strrchr( path, '/' );
	if (s)
	{
		s++;
		ppid = (pid_t)strtol(s, &p, 10);
		if (*p)
			return 0;
		if (pctl->open(path, ppid, !ignore_fork) == 0)
			return 0;
	}
	else
	{
		ppid = (pid_t)strtol(path, &p, 10);
		if (*p)
			return 0;
		if (pctl->open(ppid, !ignore_fork) == 0)
			return 0;
	}
	if ( stop() == 0 )
		return 0;
	if (loadfile)
	{
		if (!setup_name(loadfile, pnum, 1, symfiltime))
		{
			run(0,0,0);
			return 0;
		}
	}
	else
	{
		if (((args = (char *)pctl->psargs()) == 0) ||
			(!setup_name(args, pnum, 0, symfiltime)))
		{
			run(0,0,0);
			return 0;
		}
	}
	if (ignore_fork)
		flags |= (L_GRABBED|L_IGNORE_FORK);
	else
		flags |= L_GRABBED;
	exec_cnt = 1;
	_process = new Process(this, ppid, procname);
	if (!setup_data(loadfile ? 1 : 0))
	{
		run( 0, 0, 0 );
		return 0;
	}
	flags &= ~L_GO_PAST_RTL;
	_process->seglist = seglist;
	regaccess.update();
	_process->_program = new Program(_process, ename, progname, args,
		0, symfiltime, id);
	if (!create_exec())
	{
		run( 0, 0, 0 );
		proglist.remove_program(_process->_program);
		return 0;
	}
	etable->prog = _process->_program;
	return 1;
}

// Subject process execs new program.
// MORE - get rid of sibling threads
int
LWP::proc_exec()
{
	char		*args;
	time_t		stime;
	Program		*oprog;
	PtyInfo		*child_io;
	int		what, why;

	exec_cnt = 1;
	flags |= L_IN_START;
	flags &= ~L_GRABBED;
	if ((args = (char *)pctl->psargs()) == 0)
		return 0;
	delete seglist;
	if (!make_proto(P_EXEC) ||
		!setup_name(args, -1, 0, stime, (char *)ename) ||
		!setup_data(0))
	{
		run(0, 0, 0);
		state = es_dead;
		return 0;
	}
	// initial startup - do exec of process
	while(flags & L_IN_START)
	{
		if ((pctl->wait_for(what, why) != p_stopped) ||
			(!inform(what, why)))
		{
			run(0, 0, 0);
			state = es_dead;
			return 0;
		}
	}
	printm(MSG_proc_exec, procname, ename, progname);
	_process->seglist = seglist;
	oprog = _process->_program;
	oprog->remove_proc(_process, 1);
	if (child_io = oprog->childio())
		child_io->bump_count();
	_process->_program = new Program(_process, ename, progname, args,
		child_io, oprog->symfiltime(), 
		oprog->create_id());
	if (_process == proglist.current_process())
		proglist.reset_prog(_process->_program);
	check_watchpoints();
	return 1;
}

// one of our subjects has forked - setup new process
int
LWP::proc_fork()
{
	pid_t		npid;
	LWP 	*lwp;

	npid = SYS_RETURN_VAL();

	lwp = new LWP();
	if (!lwp->grab_fork(this, proglist.next_proc(), npid))
	{
		printe(ERR_fork_child_lost, E_WARNING, procname, 
			strerror(errno));
		proglist.dec_proc();
		delete lwp;
		return 0;
	}
	message_manager->reset_context(lwp);
	printm(MSG_proc_fork, procname, lwp->procname);
	if (latesttsc == SYS_vfork)
		printe(ERR_vfork_restart, E_WARNING);
	message_manager->reset_context(0);
	check_watchpoints();
	find_cur_src();
	return 1;
}


// common routine to setup data for exec'd processes
int
LWP::create_exec()
{
	// set up new process just exec'd in pipeline
	Iaddr		addr;
	int		fd;
	EventTable	*et;
	char		*path;

	if ((addr = seglist->rtl_addr( pctl )) != 0)
		set( dynpt, addr );
	if ((fd = pctl->open_object( 0, ename )) == -1)
		return 0;
	seglist->build( pctl, ename );
	addr = seglist->start_addr();
	if ((flags & L_GO_PAST_RTL) && (pc != addr))
	{
		// skip past dynamic linker
		int	what, why;

		set(destpt, addr);
		start(sg_run);
		while(pc != addr)
		{
			if ((pctl->wait_for(what, why) != p_stopped) ||
				(!inform(what, why)))
			{
				return 0;
			}
		}
	}
	et = find_et(fd, path);
	close( fd );
	if (!et)
	{
		return 0;
	}
	if (path && _process->_program)
		_process->_program->set_path(path);
	use_et(et);
	find_cur_src();
	flags &= ~L_IN_START;
	flags &= ~L_CHECK;
	return 1;
}

// Proto programs are used to keep track of event tables
// for processes that have died; these event tables are reused
// if the process is re-created.
// We create a proto program for this process only if its lwp count
// has dropped to 0.

int
LWP::make_proto(int mode)
{
	Program	*protop;
	int	noproto = 0;

	// do not create a proto process if we have sibling
	// LWPs or sibling processes
	if (_process->lwp_cnt > 1)	// this lwp
		noproto = 1;
	else
	{
		Process *p = _process->_program->proclist();
		for (; p; p = p->next())
		{
			if ((p != _process) && (p->lwp_cnt > 0))
				break;
		}
		if (p)
			noproto = 1;
	}
	if (!noproto)
	{
		protop = new Program(etable, progname, ename, mode);
		protop->set_path(_process->_program->src_path());
		etable->prog = protop;
	}
	if (!cleanup_et(mode, noproto))
	{
		if (!noproto)
			proglist.remove_program(protop);
		return 0;
	}
}

int
LWP::setup_data(int use_obj)
{
	int	fd;
	int	go_past;

	seglist = new Seglist(_process);
	if (use_obj)
	{
		if ((fd = open( ename, O_RDONLY )) == -1)
			return 0;
	}
	else
	{
		if ((fd = pctl->open_object( 0, ename )) == -1)
			return 0;
	}
	if (!seglist->setup( fd, go_past, ename ))
	{
		close(fd);
		return 0;
	}
	if (go_past)
		flags |= L_GO_PAST_RTL;
	regaccess.setup_live(pctl);
	top_frame = new Frame(this);
	cur_frame = top_frame;
	close(fd);

	if (!default_traps())
	{
		return 0;
	}

	return 1;
}

int
LWP::setup_name(const char *name, int pnum, int use_obj, 
	time_t &symfiltime, char *oldname)
{
	char		buf[PATH_MAX];
	char		*p;
	size_t		sz;
	Program		*prog;
	struct stat	stbuf;

#ifdef THREADS
	if (pnum > 0) 
	{
		// set up process and lwp name
		char	n[14];  // chars in MAX_LONG 
				// + 'p', ".1" + null
		sprintf(n, "p%d" , pnum);
		procname = str(n);
		strcat(n, ".1");
		lname = str(n);
	}
	else if (pnum == 0)
	{
		// new lwp in existing process
		char	n[23];  // 2 * chars in MAX_LONG 
				// + 'p', "." + null
		_process->next_lwp++;
		sprintf(n, "%s.%d" , procname, _process->next_lwp);
		lname = str(n);
	}
#else
	if (pnum > 0)
	{
		char	n[12];  // chars in MAX_LONG 
			// + 'p', + null
		sprintf(n, "p%d" , pnum);
		procname = str(n);
	}
#endif
	if (!name)
		return 1;
	if ( (p = strchr( name, ' ' )) != 0 )
		sz = p - (char *)name;
	else
		sz = strlen(name);
	strncpy( buf, name, sz );
	buf[sz] = '\0';
	ename = str(buf);
	// check for conflicts
	p = basename((char *)ename);
	progname = str(p);
	for(prog = proglist.prog_list(); prog; prog = prog->next())
	{
		if (progname == prog->prog_name())
		{
			Process	*proc;
			if (!prog->proclist() || prog->events())
				continue;
			else
			{
				for (proc = prog->proclist();
					proc; proc = proc->next())
					if (proc->count() > 0)
						break;
			}
			if (proc)
				break;
		}
	}
	if (prog)
	{
		sprintf(buf, "%s.%d", progname, prog->name_cnt());
		printe(ERR_used_name, E_WARNING, progname, buf);
		progname = str(buf);
	}
	if (use_obj)
	{
		if (stat(ename, &stbuf) == -1)
		{
			printe(ERR_no_access, E_ERROR, ename);
			return 0;
		}
	}
	else
	{
		int	fd;

		// pathname for args from psargs isn't directly
		// available - if we have an oldname (process exec'd)
		// try to create a new name from that
		char	*d;
		if ((access(ename, R_OK) == -1) && oldname &&
			((d = strrchr(oldname, (int)'/')) != 0))
		{
			
			sz = d - oldname + 1;
			strncpy( buf, oldname, sz );
			strcpy(buf + sz, ename);
			if (access(buf, R_OK) == 0)
				ename = str(buf);
		}
		fd = pctl->open_object( 0, ename );
		if (fstat(fd, &stbuf) == -1)
		{
			printe(ERR_no_access, E_ERROR, ename);
			return 0;
		}
		close(fd);
	}
	symfiltime = stbuf.st_mtime;
	return 1;
}
