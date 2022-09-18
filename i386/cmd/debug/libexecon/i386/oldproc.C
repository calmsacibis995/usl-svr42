#ident	"@(#)debugger:libexecon/i386/oldproc.C	1.3.1.3"

#if OLD_PROC

#include "Procctl.h"
#include "Proctypes.h"
#include "global.h"
#include "Interface.h"
#include "utility.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/reg.h>
#include <sys/regset.h>
#include <sys/procfs.h>
#include <sys/user.h>	// for sigreturn

#include <stdio.h>

struct Procdata {
	pid_t		pid;
	pid_t		followpid;
	short		check_stat;
	short		follow_children;
	prstatus	prstat;
	prrun_t		prrun;
	char		*filename;
			Procdata() { memset(this, 0, sizeof(*this)); }
			~Procdata() { delete filename; }
	int		make_follow();
	void		follow() { if (followpid > 0)
				kill(followpid, SIGUSR1); }
	void		close_follow() { if (followpid > 0)
				kill(followpid, SIGUSR2); }
};

// create follower process
// usage is: follow /proc/subject_pid debugger_pid
int
Procdata::make_follow()
{
	char debugger[11];

	sprintf(debugger, "%d", getpid());

	if ((followpid = fork()) == 0) 
	{	// child
		execl(follow_path, "follow", filename, debugger, 0);
		printe(ERR_no_follower, E_ERROR, follow_path, strerror(errno));
		_exit(1);	// use _exit to avoid flushing buffers
	}
	else if (followpid == -1)
	{
		// fork failed
		printe(ERR_no_follower, E_ERROR, follow_path, strerror(errno));
		return 0;
	}
	// test for exec failed
	if (kill(followpid, 0) == -1)
		return 0;
	return 1;
}

Proclive::Proclive() : PROCCTL()
{
	pdata = new Procdata;
	ptype = pt_live;
}

Proclive::~Proclive()
{
	delete pdata;
}

int
Proclive::open(pid_t pid, int follow)
{
	char	path[17];
	sprintf( path, "/proc/%d", pid );
	return open(path, pid, follow);
}

int
Proclive::open(const char *path, pid_t pid, int follow)
{
	do {
		errno = 0;
		fd = debug_open( path, O_RDWR );
	} while ( errno == EINTR );
	if (errno)
		return 0;
	if (follow)
	{
		// set inherit-on-fork
		do {
			errno = 0;
			ioctl( fd, PIOCSFORK, 0);
		} while ( errno == EINTR );
		if (errno)
		{
			::close(fd);
			return 0;
		}
		pdata->follow_children = 1;
	}
	pdata->filename = new char[strlen(path) +1];
	strcpy(pdata->filename, path);
	pdata->pid = pid;
	pdata->prrun.pr_flags = PRSTRACE;
	premptyset(&(pdata->prrun.pr_trace));
	pdata->check_stat = 1;
	if (!pdata->make_follow())
	{
		close();
		return 0;
	}
	return 1;
}

int
Proclive::close()
{
	if (fd != -1)
	{
		// clear inherit-on-fork
		do {
			errno = 0;
			ioctl( fd, PIOCRFORK, 0 );
		} while ( errno == EINTR );
		do {
			errno = 0;
			::close( fd );
		} while ( errno == EINTR );
		fd = -1;
	}
	pdata->close_follow();
	return 1;
}

int
Proclive::err_handle(int err)
{
	if (!err)
		return 1;
	if (err == EAGAIN)
	{
		int	nfd;

		/* Can't control process;
		 * it may have exec'd a set-uid or set-gid program.
		 * Try re-opening proc file in case we are running
		 * as super-user and can regain control.
		 * Order is important here: must try to re-open
		 * proc file before closing old file descriptor.
		 * If re-open fails, close will take process out of
		 * our control and set it running, before we
		 * have a chance to kill it.
		 */

		do {
			errno = 0;
			nfd = debug_open(pdata->filename, O_RDWR);
		} while (errno == EINTR);
		if (nfd == -1)
		{
			printe(ERR_proc_setid, E_ERROR, pdata->pid);
			return 0;
		}
		::close(fd);
		fd = nfd;
		if (pdata->follow_children)
		{
			// set inherit-on-fork
			do {
				errno = 0;
				ioctl( fd, PIOCSFORK, 0);
			} while ( errno == EINTR );
			if (errno)
			{
				close();
				printe(ERR_proc_setid, E_ERROR, pdata->pid);
				return 0;
			}
		}
		return 1;
	}
	switch(err)
	{
	case EINTR:
	case ENOENT:	
			break;
	case EIO: /*FALLTHROUGH*/
	case EFAULT:	
			printe(ERR_proc_io, E_ERROR, pdata->pid);
			break;
	default:
			printe(ERR_proc_unknown, E_ERROR, pdata->pid);
	}
	return 0;
}

int 
Proclive::run(int sig)
{
	pdata->check_stat = 1;
	pdata->prrun.pr_flags |= PRCFAULT;
	pdata->prrun.pr_flags &= ~PRSTEP;
	if (sig == 0)
		pdata->prrun.pr_flags |= PRCSIG;
	else
		pdata->prrun.pr_flags &= ~PRCSIG;
	do {
		errno = 0;
		ioctl(fd, PIOCRUN, &pdata->prrun);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (!errno)
	{
		pdata->follow();
		return 1;
	}
	return 0;
}

int 
Proclive::step(int sig)
{
	pdata->check_stat = 1;
	pdata->prrun.pr_flags |= PRCFAULT|PRSTEP;
	if (sig == 0)
		pdata->prrun.pr_flags |= PRCSIG;
	else
		pdata->prrun.pr_flags &= ~PRCSIG;
	do {
		errno = 0;
		ioctl(fd, PIOCRUN, &pdata->prrun);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (!errno)
	{
		pdata->follow();
		return 1;
	}
	return 0;
}

int 
Proclive::stop()
{
	do {
		errno = 0;
		ioctl(fd, PIOCSTOP, &pdata->prstat);
	} while(errno && (errno != EINTR) && err_handle(errno));
	if (!errno)
	{
		pdata->check_stat = 0;
		return 1;
	}
	if (errno == EINTR)
	{
		printe(ERR_stop_intr, E_WARNING, pdata->pid);
	}
	pdata->check_stat = 1;
	return 0;
}

Procstat 
Proclive::wait_for(int &what, int &why)
{
	do {
		errno = 0;
		ioctl(fd, PIOCWSTOP, &pdata->prstat);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (errno == 0)
	{
		what = pdata->prstat.pr_what;
		why = pdata->prstat.pr_why;
		pdata->check_stat = 0;
		return p_stopped;
	}
	pdata->check_stat = 1;
	return p_dead;
}

Procstat 
Proclive::status(int &what, int &why)
{
	if (pdata->check_stat)
	{
		do {
			errno = 0;
			ioctl(fd, PIOCSTATUS, &pdata->prstat);
		} while(errno == EINTR);
		if (errno)
			return p_dead;
	}
	if (pdata->prstat.pr_flags & PR_STOPPED)
	{
		what = pdata->prstat.pr_what;
		why = pdata->prstat.pr_why;
		pdata->check_stat = 0;
		return p_stopped;
	}
	return p_running;
}

int 
Proclive::kill(int signo)
{
	pdata->check_stat = 1;
	do {
		errno = 0;
		ioctl(fd, PIOCKILL, &signo);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
Proclive::cancel_sig( int signo )
{
	int	what, why;

	pdata->check_stat = 1;
	if (status(what, why) == p_dead)
		return 0;
	if (pdata->prstat.pr_cursig == signo)
	{
		do {
			errno = 0;
			ioctl( fd, PIOCSSIG, 0 );
		} while(errno && ((errno == EINTR) || 
			err_handle(errno)));
	}
	else
	{
		do {
			errno = 0;
			ioctl( fd, PIOCUNKILL, &signo );
		} while(errno && ((errno == EINTR) || 
			err_handle(errno)));
	}
	pdata->check_stat = 1;
	return ( errno == 0 );
}

// cancel all pending signals
int
Proclive::cancel_all_sig()
{

	int	what, why;

	// clear current signal
	pdata->check_stat = 1;
	do {
		errno = 0;
		ioctl( fd, PIOCSSIG, 0 );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	if (errno)
		return 0;

	if (status(what, why) == p_dead)
		return 0;

	pdata->check_stat = 1;
	for (int i=1; i < NSIG; i++)
	{
		if (prismember(&pdata->prstat.pr_sigpend,i))
		{
			do {
				errno = 0;
				ioctl( fd, PIOCUNKILL, &i );
			} while(errno && ((errno == EINTR) || 
				err_handle(errno)));
		}
		if (errno)
			return 0;
	}
	return 1;
}

int
Proclive::pending_sigs()
{
	int	what, why;
	int	mask = 0;

	if (status(what, why) == p_dead)
		return 0;
	if (pdata->prstat.pr_cursig)
		mask |= (1 << (pdata->prstat.pr_cursig - 1));
	for (int i=1; i < NSIG; i++)
	{
		if (prismember(&pdata->prstat.pr_sigpend,i))
		{
			mask |= (1 << (i - 1));
		}
	}
	return mask;
}

int
Proclive::trace_sigs(sig_ctl *sigs)
{
	pdata->check_stat = 1;
	pdata->prrun.pr_trace = sigs->signals;
	do {
		errno = 0;
		ioctl(fd, PIOCSTRACE, &sigs->signals);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
Proclive::trace_traps(flt_ctl *flt)
{
	pdata->check_stat = 1;
	do {
		errno = 0;
		ioctl(fd, PIOCSFAULT, &flt->faults);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
Proclive::sys_entry(sys_ctl *sys)
{
	pdata->check_stat = 1;
	do {
		errno = 0;
		ioctl(fd, PIOCSENTRY, &sys->scalls);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
Proclive::sys_exit(sys_ctl *sys)
{
	pdata->check_stat = 1;
	do {
		errno = 0;
		ioctl(fd, PIOCSEXIT, &sys->scalls);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

const char *
Proclive::psargs()
{
	static prpsinfo_t	psinfo;
	do {
		errno = 0;
		ioctl(fd, PIOCPSINFO, &psinfo);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return((errno == 0) ? psinfo.pr_psargs : 0);
}

Iaddr
Proclive::sigreturn()
{
	char	ub[USIZE * PAGESIZE];
	do {
		errno = 0;
		ioctl(fd, PIOCGETU, ub);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return((errno == 0) ? (Iaddr)(((struct user *)ub)->u_sigreturn) : (Iaddr)-1);
}

int
Proclive::read(Iaddr from, void *to, int len)
{
	int	result;
	do {
		errno = 0;
		lseek(fd, from, SEEK_SET);
	} while(errno == EINTR);
	if (errno)
		return -1;
	do {
		errno = 0;
		result = ::read(fd, to, len);
	} while(errno == EINTR);
	return result;
}

int
Proclive::write(Iaddr to, const void *from, int len)
{
	int	result;
	do {
		errno = 0;
		lseek(fd, to, SEEK_SET);
	} while(errno == EINTR);
	if (errno)
		return -1;
	do {
		errno = 0;
		result = ::write(fd, from, len);
	} while(errno == EINTR);
	return result;
}

int
Proclive::update_stack(Iaddr &lo, Iaddr &hi)
{
	int	num; // number of mapped segments
	int 	i;
	prmap_t	*pmap;

	do {
		errno = 0;
		ioctl( fd, PIOCNMAP, &num );
	} while ( errno == EINTR );
	if (errno)
		return 0;
	pmap = new(prmap_t[num + 1]);
	do {
		errno = 0;
		ioctl( fd, PIOCMAP, pmap );
	} while ( errno == EINTR );
	if (errno)
	{
		delete pmap;
		return 0;
	}
	
	for(i = 0; i <= num; i++)
		if (pmap[i].pr_mflags & MA_STACK)
			break;
	
	if (i >= num)
	{
		delete pmap;
		return 0;
	}
	else
	{
		lo = (Iaddr)(pmap[i].pr_vaddr);
		hi = (Iaddr)(pmap[i].pr_vaddr + pmap[i].pr_size);
	}
	delete pmap;
	return 1;
}

int
Proclive::open_object(Iaddr addr, const char *)
{
	int	fdobj;
	caddr_t	*p;
	p = ( addr == 0 ) ? 0 : (caddr_t*)&addr;
	do {
		errno = 0;
		fdobj = ioctl( fd, PIOCOPENM, p );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return fdobj;
}

int
Proclive::read_greg(greg_ctl *greg)
{
	int	what, why;

	if (status(what, why) != p_stopped)
		return 0;
	memcpy((char *)&greg->gregs, (char *)&pdata->prstat.pr_reg, sizeof(gregset_t));
	return 1;
}

int
Proclive::write_greg(greg_ctl *greg)
{
	pdata->check_stat = 1;
	do {
		errno = 0;
		ioctl( fd, PIOCSREG, &greg->gregs );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
Proclive::read_fpreg(fpreg_ctl *fpreg)
{
	do {
		errno = 0;
		ioctl( fd, PIOCGFPREG, &fpreg->fpregs );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
Proclive::write_fpreg(fpreg_ctl *fpreg)
{
	do {
		errno = 0;
		ioctl( fd, PIOCSFPREG, &fpreg->fpregs );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
Proclive::read_dbreg(dbreg_ctl *dbreg)
{
	do {
		errno = 0;
		ioctl( fd, PIOCGDBREG, &dbreg->dbregs );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
Proclive::write_dbreg(dbreg_ctl *dbreg)
{
	do {
		errno = 0;
		ioctl( fd, PIOCSDBREG, &dbreg->dbregs );
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
Proclive::numsegments()
{
	int	num;
	do {
		errno = 0;
		ioctl( fd, PIOCNMAP, &num);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno ? 0 : num);
}

int
Proclive::seg_map(map_ctl *map)
{
	do {
		errno = 0;
		ioctl( fd, PIOCMAP, map);
	} while(errno && ((errno == EINTR) || err_handle(errno)));
	return (errno == 0);
}

int
stop_self_on_exec()
{
	pid_t		pid;
	sysset_t	sxset;
	char		filename[17];
	int		fd;
	int		err;

	do {
		errno = 0;
		pid = getpid();
	} while ( errno == EINTR );
	if (errno)
		return 0;
	sprintf( filename, "/proc/%d", pid);
	do {
		errno = 0;
		fd = ::open( filename, O_RDWR );
	} while ( errno == EINTR );
	if (errno)
		return 0;
	premptyset( &sxset);
	praddset( &sxset, SYS_exec );
	praddset( &sxset, SYS_execve );
	do {
		errno = 0;
		ioctl(fd, PIOCSEXIT, &sxset);
	} while(errno == EINTR);
	err = errno;
	close(fd);
	return (err == 0);
}

int
live_proc(int fd)
{
	prstatus	p;

	do {
		errno = 0;
		ioctl(fd, PIOCSTATUS, &p);
	} while(errno == EINTR);
	return (errno == 0);
}

int
release_child(pid_t pid)
{
	char		filename[17];
	int		fd;
	int		err;

	sprintf( filename, "/proc/%d", pid);
	do {
		errno = 0;
		fd = ::open( filename, O_RDWR );
	} while ( errno == EINTR );
	if (errno)
		return 0;
	
	do {
		errno = 0;
		ioctl(fd, PIOCSRLC);
	} while(errno == EINTR);
	err = errno;
	close(fd);
	return (err == 0);
}

#endif
