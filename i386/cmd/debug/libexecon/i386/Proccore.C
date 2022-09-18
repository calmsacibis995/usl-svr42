#ident	"@(#)debugger:libexecon/i386/Proccore.C	1.4.1.1"

// provides access to core files,
// both old and new (ELF) format
//
// If old format, fake new format coredata as best we can.

#include "Procctl.h"
#include "Proctypes.h"
#include "Interface.h"
#include "Machine.h"
#include "Reg1.h"
#include "global.h"
#include "Parser.h"
#include "ELF.h"
#include "utility.h"
#include <elf.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/procfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

struct CoreData {
	char		*notes;		// copy of NOTE segment
	Elf_Phdr	*phdr;		// copy of program header array
	int		numphdr;	// how many phdrs?
	ELF		*elf;		// object access
	prstatus_t	*status;	// points into notes
	fpregset_t	*fpregs;	// ditto, or 0
	char 		*psinfo;	// ditto, or 0

			CoreData() { memset( (char *)this, 0, 
						sizeof(CoreData) ); }
			~CoreData();
};

CoreData::~CoreData()
{
	if (elf)
		delete elf;
	else
	{
		if (phdr)
			free(phdr);
		if (status)
			free(status);
		if (psinfo)
			free(psinfo);
		if (fpregs)
			free(fpregs);
	}
}

#ifdef __cplusplus
extern "C" {
#endif
extern int fake_ELF_core( int corefd, off_t sz, Elf_Phdr **, 
	int *phdrnm, prstatus_t **, fpregset_t **, char **psinfo );
#ifdef __cplusplus
}
#endif

Proccore::Proccore() : PROCCTL()
{
	coredata = 0;
	ptype = pt_core;
}

int
Proccore::open( int cfd )
{
	char 		magic[SELFMAG];
	Elf_Phdr	*phdrp;
	int		phnum;
	int		begin = 0;
	struct stat	stbuf;
	off_t		size;
	Iaddr		addr;
	long		off;
	char		*p;

	// These must be declared here because of the use of gotos.
	long  tempsize; // for C++ 2.0 workaround below
	void* tempdata;

	if ((fd = debug_dup(cfd)) == -1)
		return 0;
	if (fstat(fd, &stbuf) == -1)
		return 0;
	size = stbuf.st_size;
	coredata = new CoreData;
	if ((lseek(fd, 0, SEEK_SET) == -1) ||
		(::read(fd, magic, sizeof magic) != sizeof magic))
	{
		printe(ERR_core_access, E_ERROR, strerror(errno));
		return 0;
	}

	// DEL E L F == ELF file
	if ( strncmp(magic, ELFMAG, SELFMAG) != 0 ) 
	{	
		char		*psinfo;
		Elf_Phdr	*phdr;
		fpregset_t	*fpregs;
		prstatus_t	*pstat;
		int		pnum;
		int		err;
		// old style
		if ((err = fake_ELF_core( fd, size, &phdr, &pnum,
			&pstat, &fpregs, &psinfo)) != 0)
			 
		{
			if (err == (int)ERR_internal)
				new_handler();
			delete coredata;
			coredata = 0;
			printe((Msg_id)err, E_ERROR);
			return 0;
		} 
		coredata->numphdr = pnum;
		coredata->phdr = phdr;
		coredata->status = pstat;
		coredata->fpregs = fpregs;
		coredata->psinfo = psinfo;
		return 1;
	}
	coredata->elf = new ELF(fd, stbuf.st_dev, stbuf.st_ino, stbuf.st_mtime);
	if ((coredata->elf->file_format() != ff_elf) || !coredata->elf->is_core())
	{
		printe(ERR_core_format, E_ERROR);
		goto err;
	} 

	if (!coredata->elf->get_phdr(coredata->numphdr, coredata->phdr))
	{
		printe(ERR_core_format, E_ERROR);
		goto err;
	}

	// check for truncated core file
	phdrp = coredata->phdr;
	phnum = coredata->numphdr;
	for(; phnum > 0; phnum--, phdrp++)
	{
		if ((phdrp->p_offset + phdrp->p_filesz) > size)
		{
			printe(ERR_core_truncated, E_ERROR);
			goto err;
		}
	}

	// Get core NOTE section.
	// The status and register information for ELF
	// core files is contained in the NOTE section.
	// The format of a NOTE section entry is:
	//
	// namesz	# int: number of bytes in name
	// descsz	# int: number of bytes in description
	// type		# int: type of entry
	// name		# namesz bytes (null-terminated)
	// description	# descsz bytes
	// padding	# to make next entry 4-byte aligned

	tempsize = size;
	tempdata = coredata->notes;
	// C++ 2.0 uses temporaries when types don't match exactly.
	if (!coredata->elf->getsect(s_notes, tempdata, addr, tempsize, off))
	{
		printe(ERR_core_format, E_ERROR);
		goto err;
	}
	size = tempsize;
	coredata->notes = (char*)tempdata;

	int namesz, descsz, type;
	p = coredata->notes;
	while ( size > 0 ) 
	{
		namesz = *(int *)p; p += sizeof(int);
		descsz = *(int *)p; p += sizeof(int);
		type   = *(int *)p; p += sizeof(int);
		size -= 3 * sizeof(int) + namesz + descsz;
		p += namesz;
		switch( type ) 
		{
		default:
			break;
		case 1:
			coredata->status = (prstatus_t *)p;
			break;
		case 2:
			coredata->fpregs = (fpregset_t *)p;
			break;
		case 3:			// psinfo
			coredata->psinfo = ((prpsinfo_t *)p)->pr_psargs;
			break;
		}
		p += descsz;
		int mod = (int)p % sizeof(int);
		if (mod)
			p += sizeof(int) - mod;
	}
	if ( !coredata->status )
	{
		printe(ERR_core_format, E_ERROR);
		goto err;
	}
	return 1;
err:
	delete coredata;
	coredata = 0;
	return 0;
}

int
Proccore::close()
{
	::close(fd);
	fd = -1;
	return 1;
}

Proccore::~Proccore()
{
	delete coredata;
}

int
Proccore::numsegments()
{
	if (coredata)
		return coredata->numphdr;
	return 0;
}

Elf_Phdr *
Proccore::segment( int which )
{
	if ( coredata && coredata->phdr && which >= 0 && which < coredata->numphdr )
		return coredata->phdr + which;
	else
		return 0;
}

Procstat
Proccore::status(int &what, int &why)
{
	if (coredata)
	{
		what = coredata->status->pr_what;
		why = coredata->status->pr_why;
		return p_core;
	}
	return p_unknown;
}

// return name of signal that caused core file to be generated
// and faulting address, if applicable
void
Proccore::core_state()
{
	char		*sname;
	int		sig;
	Iaddr		fltaddr;
	int		addr_needed = 0;
	char		signal_name[14]; // big enough for any name

	if (!coredata->status)
		return;
	sig = coredata->status->pr_info.si_signo;
	if (sig < 1 || sig >= NSIG)
		return;
	sname = (char *)signame(sig);
	switch(sig)
	{
		case SIGILL:
		case SIGTRAP:
		case SIGFPE:
		case SIGEMT:
		case SIGBUS:
		case SIGSEGV:
			fltaddr = (Iaddr)coredata->status->pr_info._data._fault._addr;
			addr_needed = 1;
			break;
		default:
			break;
	}
	sprintf(signal_name, "sig%s", sname);
	if (addr_needed)
		printm(MSG_core_state_addr, signal_name, fltaddr);
	else
		printm(MSG_core_state, signal_name);
}

int
Proccore::read_greg(greg_ctl *greg)
{
	if (coredata)
	{
		memcpy((char *)&greg->gregs, (char *)coredata->status->pr_reg,
			sizeof(gregset_t));
		return 1;
	}
	return 0;
}

int
Proccore::read_fpreg(fpreg_ctl *fpreg)
{
	if (coredata)
	{
		memcpy((char *)&fpreg->fpregs, (char *)&coredata->fpregs, 
			sizeof(fpregset_t));
		return 1;
	}
	return 0;
}

const char *
Proccore::psargs()
{
	if (coredata)
		return coredata->psinfo;
	return 0;
}
