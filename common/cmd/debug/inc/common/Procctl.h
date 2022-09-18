/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Procctl_h
#define Procctl_h
#ident	"@(#)debugger:inc/common/Procctl.h	1.2.1.1"

// Operating-system level control of processes and lwps
// Common interface handles old /proc, new /proc, ptrace.
// The mechanism for following multiple processes
// is hidden in the interface-specific pieces.
//
// The same interface is provided for live processes, core files
// and static object files.
// 
// The "read" function is virtual - reads from an object file, live process
// address space and core file data region all use the same interface.

#include "Iaddr.h"
#include "Machine.h"
#include "Proctypes.h"
#include "Reg.h"
#include <sys/types.h>
#include <sys/procfs.h>

enum	Procstat {
	p_unknown = 0,
	p_dead,
	p_core,
	p_stopped,
	p_running,
};

enum	Proctype {
	pt_unknown = 0,
	pt_object,
	pt_core,
	pt_live,
};

struct	Procdata;	// opaque to clients
struct	CoreData;	// opaque to clients
struct	Elf_Phdr;

class Procctl  {
protected:
	int		fd;
	Proctype	ptype;
public:
			Procctl() { fd = -1; ptype = pt_unknown;}
			~Procctl() {}
	int		open(int fd);
	int		close();
	virtual int	read(Iaddr from, void *to, int len);
	int		get_fd() { return fd; }
	Proctype	get_type() { return ptype; }
};

class Proclive : public Procctl {
	Procdata	*pdata;
	int		err_handle(int);
public:
			Proclive();
			~Proclive();
	int		open(pid_t, int follow);
	int		open(const char *path, pid_t, int follow);
				// path might not be in local /proc
				// directory
	int		close();
	const char	*psargs();
	int		stop();
	Procstat	status(int &why, int &what);
	Procstat	wait_for(int &why, int &what);
	int		kill(int sig);
	int		cancel_sig(int sig);
	int		cancel_all_sig();
	int		pending_sigs();
	int		run(int sig);
	int		step(int sig);
	int		sys_entry(sys_ctl *);
	int		sys_exit(sys_ctl *);
	int		trace_sigs(sig_ctl *);
	int		trace_traps(flt_ctl *);
	int		update_stack(Iaddr &low, Iaddr &hi);
	int		open_object(Iaddr, const char *);
	Iaddr		sigreturn();
	int		read_greg(greg_ctl *);
	int		write_greg(greg_ctl *);
	int		read_fpreg(fpreg_ctl *);
	int		write_fpreg(fpreg_ctl *);
	int		read_dbreg(dbreg_ctl *);
	int		write_dbreg(dbreg_ctl *);
	int		read(Iaddr from, void *to, int len);
	int		write(Iaddr to, const void *from, int len);
	int		numsegments();
	int		seg_map(map_ctl *);
};

class Proccore: public Procctl {
	CoreData	*coredata;
public:
			Proccore();
			~Proccore();
	int		open(int fd);
	int		close();
	const char	*psargs();
	int		read_greg(greg_ctl *);
	int		read_fpreg(fpreg_ctl *);
	Procstat 	status(int &why, int &what);
	void		core_state();	// why it dumped core
	int		numsegments(); // includes all segments,
				// whether actually present in core
				// file or not
	Elf_Phdr	*segment(int which); 
				// which = [0..numsemgents() -1]
};

// cfront 2.1 requires base class name in derived class constructor,
// 1.2 forbids it
#ifdef __cplusplus
#define PROCCTL	Procctl
#else
#define PROCCTL
#endif

extern int		stop_self_on_exec();
extern int		release_child(pid_t);

#ifdef OLD_PROC
extern int		live_proc(int fd);
#else
extern int		live_proc(const char *path);
#endif

#endif
