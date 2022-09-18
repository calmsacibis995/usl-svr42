#ident	"@(#)debugger:libexecon/common/Seglist.C	1.8.1.1"

#include "Process.h"
#include "LWP.h"
#include "Link.h"
#include "Seglist.h"
#include "Symtab.h"
#include "Object.h"
#include "Segment.h"
#include "Procctl.h"
#include "Interface.h"
#include "Symbol.h"
#include "Rtl_data.h"
#include "Tag.h"
#include "global.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <libgen.h>

// Segment list processing is divided between common
// and machine dependent pieces.  The machine dependencies
// have mainly to do with shared library processing.

// Symnodes are an optimization; we maintain a list of
// symbol tables per-process.  This list will be shorter
// for traversal than the list of segments, since many segments
// may map to one symtable.

class Symnode: public Link {
public:
	Symtab		sym;
	const char	*pathname;
			Symnode( const char *, Iaddr );
			~Symnode() { delete (void *)pathname; }
	Iaddr		brtbl_lo;	// static shared lib branch table 
	Iaddr		brtbl_hi;
	Symnode		*next()	{ return (Symnode*)Link::next(); }
};

Symnode::Symnode( const char * s, Iaddr ss_base )
{
	pathname = new(char[strlen(s) + 1]);
	strcpy( (char *)pathname, s );
	sym.ss_base = ss_base;
	sym.symtable = 0;
	brtbl_lo = 0;
	brtbl_hi = 0;
}

Seglist::Seglist( Process * process)
{
	proc = process;
	mru_segment = 0;
	first_segment = last_segment = 0;
	symlist = lastsym = 0;
	static_loaded = 0;
	symnode_file = symnode_global = 0;
	uses_rtld = 0;
	stack_lo = 0;
	_has_stsl = 0;
	stack_hi = 0;
	rtl_data = 0;
	start = 0;
}

Seglist::~Seglist()
{
	Segment *seg;
	Segment *nxt;

	delete rtl_data;
	for ( seg = first_segment; seg != 0; seg = nxt )
	{
		nxt = seg->next();
		seg->unlink();
		delete seg;
	}
}

// Determine if executable is dynamically linked.  If so,
// find interpreter path and create an Rtl_data structure.
int
Seglist::setup( int fd, int & rtld_used, const char *exec_name )
{
	Object	*obj;
	Iaddr	addr;
	long	size, seekpt;
	void	*interp, *tmp;

	rtld_used = 0;
	if ( fd == -1 )
	{
		return 0;
	}
	if ((obj = find_object(fd, exec_name)) == 0)
		return 0;

	if ( (obj->getsect(s_dynamic, tmp, addr, size, seekpt) == 0) ||
		(obj->getsect(s_interp, interp, addr, size, seekpt)
		== 0) )
	{
		return 1;
	}
	rtl_data = new Rtl_data((char *)interp);
	rtld_used = 1;
	uses_rtld = 1;
	return 1;
}

//
// build segments from a.out and static shared libs if there are any
//
int
Seglist::build_static( Proclive *pctl, const char *executable_name )
{
	Segment	*seg;
	int 	fd;
	Object	*obj;

	if (( (fd = pctl->open_object(0, executable_name)) == -1 ) ||
		((obj = find_object(fd, 0)) == 0) ||
		( add( fd, (Procctl *)pctl, executable_name, 0, 0L, 0 ) == 0 ) ||
		( add_static_shlib( obj, (Procctl *)pctl, 0 ) == 0 ))
	{
		if (fd >= 0)
			close(fd);
		return 0;
	}
	seg = new Segment( (Procctl *)pctl, executable_name, 0, 
		stack_lo, stack_hi - stack_lo, stack_lo, 0, 1, 0);
	if (last_segment)
		seg->append(last_segment);
	last_segment = seg;
	if (!first_segment)
		first_segment = last_segment;
	close(fd);
	return 1;
}

//
// is addr in stack segment
//
int
Seglist::in_stack( Iaddr addr )
{
	if ( addr == 0 )
		return 0;
	if ( (stack_lo == 0) && (stack_hi == 0) )
		return ( !in_text(addr) );
	else
		return ( (stack_lo <= addr) && (addr < stack_hi) );
}

//
// update stack segment boundaries
//
void
Seglist::update_stack(Proclive *pctl)
{
	if (pctl->update_stack( stack_lo, stack_hi) == 0)
		printe(ERR_sys_map, E_ERROR, strerror(errno));
}

//
// get static shared lib branch table addresses
//
int
Seglist::get_brtbl( const char * name )
{
	Symnode *symnode;
	Symbol	symbol;

	symnode = symlist;
	while ( symnode != 0) 
	{
		if (!strcmp(symnode->pathname, name))
			break;
		symnode = symnode->next();
	}
	symnode->sym.find_source("branchtab", symbol);
	if ( symbol.isnull() ) 
	{
		printe(ERR_branch_tbl, E_ERROR, name);
		return 0;
	}
	symnode->brtbl_lo = symbol.pc(an_lopc);
	symnode->brtbl_hi = symbol.pc(an_hipc);
	return 1;
}

int
Seglist::build( Proclive *pctl, const char *exec_name )
{
	update_stack(pctl);
	if (!static_loaded)
	{
		// build static builds segment list for a.out
		// and static shared libraries
		if ( build_static( pctl, exec_name ) == 0 )
		{
			return 0;
		}
		else
			static_loaded = 1;
	}
	return build_dynamic( pctl );
}

int
Seglist::add_static_shlib( Object *obj, Procctl *pctl, int text_only )
{
	Iaddr	addr;
	long	size, seekpt;
	int	i;
	char	**stsl_names;
	int	fd;
	void	*tmp;

	if (!obj)
		return 0;
	// if object has a .lib section, read list of shlib pathnames
	if (obj->getsect( s_lib, tmp, addr, size, seekpt ) == 0)
	{
		return 1;
	}
	else if ( (stsl_names = obj->get_stsl_names()) == 0 )
	{
		return 0;
	}
	for ( i = 0; stsl_names[i] != 0; ++i )
	{
		Procctl	*nctl;
		if ((fd = open(stsl_names[i], O_RDONLY)) < 0)
		{
			printe(ERR_shlib_open, E_ERROR, stsl_names[i],
				strerror(errno));
			return 0;
		}
		if (pctl->get_type() == pt_live)
			nctl = pctl;
		else
		{
			nctl = new Procctl;
			if (!nctl->open(fd))
				return 0;
		}
		if (!add( fd, nctl, stsl_names[i], text_only, 0, 0 ))
			return 0;
		if (!get_brtbl( stsl_names[i] ))
			return 0;
		_has_stsl++;
		close(fd);
	}
	return 1;
}
//
// read proto
// get segments from a.out, static shared libs ( if any )
// and core ( if it exists. )
//
int
Seglist::readproto( Procctl *txtctl, Proccore *core,
	const char *executable_name )
{
	Segment 	*seg;
	int		i;
	Elf_Phdr 	*phdr;
	Object		*obj;
	int		textfd;

	if ((textfd = txtctl->get_fd()) < 0)
	{
		return 0;
	}
	if ((obj = find_object(textfd, executable_name)) == 0)
		return 0;

	if ( !core )
	{
		printe(ERR_internal, E_ERROR, 
			"Seglist::readproto", __LINE__);
		return 0;
	}

	i = 0;
	for ( phdr = core->segment(i) ; phdr ;
			phdr = core->segment(++i) )
	{
		// add segment descriptors for data dumped to core file
		if ( phdr->p_memsz && phdr->p_filesz )
		{
			seg = new Segment( core, 0, 0, phdr->p_vaddr,
				phdr->p_filesz, phdr->p_offset,
				phdr->p_vaddr,(int)(phdr->p_flags & PF_W),
				(int)(phdr->p_flags & PF_X));
			if (last_segment)
				seg->append(last_segment);
			last_segment = seg;
			if (!first_segment)
				first_segment = last_segment;
		}
	}
	// add segment descriptors for a.out and static shlib text
	if (!add( textfd, txtctl, executable_name, 1, 0, 0 ))
		return 0;
	if (!add_static_shlib( obj, txtctl, 1 ))
		return 0;
	return add_dynamic_text( txtctl, executable_name );
}

Segment *
Seglist::find_segment( Iaddr addr )
{
	Segment	*seg;

	if ( ( mru_segment != 0 ) && ( mru_segment->loaddr <= addr ) &&
		( mru_segment->hiaddr > addr ) )
	{
		return mru_segment;
	}
	for ( seg = first_segment ; seg != 0 ; seg = seg->next() )
	{
		if ( (addr >= seg->loaddr) && ( addr < seg->hiaddr) )
		{
			mru_segment = seg;
			return seg;
		}
	}
	return 0;
}

Symtab *
Seglist::find_symtab( Iaddr addr )
{
	Segment *seg;

	if ( ( mru_segment != 0 ) && ( mru_segment->loaddr <= addr ) &&
		( mru_segment->hiaddr > addr ) )
	{
		seg = mru_segment;
	}
	else
	{
		for (seg = first_segment; seg != 0; seg = seg->next())
		{
			if ((addr >= seg->loaddr) && 
				(addr < seg->hiaddr))
			{
				break;
			}
		}
	}
	if (!seg)
		return 0;
	seg->get_symtable();
	return &seg->sym;
}

Symtab *
Seglist::find_symtab( const char *name )
{
	Symnode *symnode;

	for (symnode = symlist; symnode != 0; symnode = symnode->next())
	{
		if ((strcmp(name, symnode->pathname) == 0)
			|| (strcmp(name, 
			basename((char *)symnode->pathname)) == 0))
			return &symnode->sym;
			
	}
	return 0;
}

const char *
Seglist::object_name(Iaddr addr)
{
	Segment	*seg;

	for (seg = first_segment; seg != 0; seg = seg->next())
	{
		if ((addr >= seg->loaddr) && 
			(addr < seg->hiaddr))
		{
			break;
		}
	}
	if (!seg)
		return 0;
	return(seg->name());
}

int
Seglist::find_source( const char * name, Symbol & symbol )
{
	Symnode *symnode;

	for (symnode = symlist; symnode != 0; symnode = symnode->next())
	{
		if ( symnode->sym.find_source( name, symbol ) != 0 )
		{
			return 1;
		}
	}
	return 0;
}

NameEntry *
Seglist::first_global()
{
	Symnode		*symnode;
	NameEntry	*first;

	symnode = symlist;
	while ( symnode != 0 )
	{
		if ( (first = symnode->sym.first_global()) != 0 )
		{
			symnode_global = symnode;
			return first;
		}
		else
		{
			symnode = symnode->next();
		}
	}
	symnode_global = 0;
	return 0;
}

Symbol
Seglist::next_global()
{
	Symnode		*symnode;
	NameEntry	*next;
	Symbol		sym;

	symnode = symnode_global;
	if ( symnode == 0 )
	{
		return sym;
	}
	if ( (next = symnode->sym.next_global()) != 0 )
	{
		return( symnode->sym.global_symbol(next));
	}
	symnode = symnode->next();
	while ( symnode != 0 )
	{
		if ( (next = symnode->sym.first_global()) != 0 )
		{
			symnode_global = symnode;
			return( symnode->sym.global_symbol(next));
		}
		else
		{
			symnode = symnode->next();
		}
	}
	symnode_global = 0;
	return sym;
}

Symbol
Seglist::prev_global()
{
	Symnode		*symnode;
	NameEntry	*prev;
	Symbol		sym;

	symnode = symnode_global;
	if ( symnode == 0 )
	{
		return sym;
	}
	if ( (prev = symnode->sym.prev_global()) != 0 )
	{
		return( symnode->sym.global_symbol(prev));
	}
	symnode = symnode->next();
	while ( symnode != 0 )
	{
		if ( (prev = symnode->sym.first_global()) != 0 )
		{
			symnode_global = symnode;
			return( symnode->sym.global_symbol(prev));
		}
		else
		{
			symnode = symnode->next();
		}
	}
	symnode_global = 0;
	return sym;
}

Symbol
Seglist::first_file()
{
	Symnode *symnode;
	Symbol	first;

	symnode = symlist;
	while ( symnode != 0 )
	{
		first = symnode->sym.first_symbol();
		if ( !first.isnull() && (first.tag() == t_sourcefile))
		{
			symnode_file = symnode;
			current_file = first;
			return first;
		}
		else
		{
			symnode = symnode->next();
		}
	}
	symnode_file = 0;
	current_file.null();
	return first;
}

Symbol
Seglist::next_file()
{
	Symnode *symnode;
	Symbol	next;

	symnode = symnode_file;
	if ( symnode == 0 )
	{
		return next;
	}
	next = current_file.arc( an_sibling );
	if ( !next.isnull() )
	{
		current_file = next;
		return next;
	}
	symnode = symnode->next();
	while ( symnode != 0 )
	{
		next = symnode->sym.first_symbol();
		if ( !next.isnull() && (next.tag() == t_sourcefile))
		{
			symnode_file = symnode;
			current_file = next;
			return next;
		}
		else
		{
			symnode = symnode->next();
		}
	}
	symnode_file = 0;
	current_file.null();
	return next;
}

Symbol
Seglist::find_global( const char * name )
{
	Symnode *symnode;
	Symbol	symbol;

	symnode = symlist;
	while ( symnode != 0 )
	{
		symbol = symnode->sym.find_global(name);
		if ( symbol.isnull() )
		{
			symnode = symnode->next();
			continue;
		}
		if ( symnode->brtbl_lo > 0 ) 
		{
			//
			// static shared library
			//
			
			Iaddr addr = symbol.pc(an_lopc);
			if ( (addr >= symnode->brtbl_lo) &&
			    ( addr < symnode->brtbl_hi) ) 
			{
				LWP	*l;
				// find any live lwp to use
				// to get at instructions
				for(l = proc->lwp_list(); l; 
					l = l->next())
				{
					if (l->get_state() != es_dead)
						break;
				}
				addr = l->instruct()->brtbl2fcn(addr);
				symbol = symnode->sym.find_entry(addr);
			}
		}
		symnode_global = symnode;
		return symbol;
	}
	return symbol;
}

// Add a segment entry for each loadable segment - if text_only
// set, skip writeable segments
int
Seglist::add( int fd, Procctl *pctl, const char * name, int text_only, 
	Iaddr base, Segment * segment )
{
	Iaddr	addr;
	Segment	*seg;
	int	shared;
	Seginfo	*seginfo;
	int	i, count;
	Object	*obj;

	if ( fd < 0 )
	{
		return 0;
	}
	if (((obj = find_object(fd, 0)) == 0) ||
		( (seginfo = obj->get_seginfo( count, shared )) == 0 ))
	{
		return 0;
	}
	if ( !shared )
	{
		start = obj->start_addr();
	}
	for ( i = 0; i < count ; ++i )
	{
		addr = seginfo[i].vaddr;
		if ( text_only && (seginfo[i].seg_flags & SEG_WRITE) ) 
		{
			continue;
		}
		else if ( shared )
		{
			addr += base;
		}
		if ( seginfo[i].seg_flags & SEG_LOAD )
		{
			// if segment is not writable, assume it's text
			seg = new Segment(pctl, name, obj, addr, 
				seginfo[i].mem_size, 
				seginfo[i].offset, base,
				(seginfo[i].seg_flags & SEG_WRITE),
				(seginfo[i].seg_flags & SEG_EXEC));
			if (segment)
				seg->prepend(segment);
			else
			{
				if (last_segment)
					seg->append(last_segment);
				last_segment = seg;
				if (!first_segment)
					first_segment = last_segment;
			}
		}
	}
	return add_symnode( obj, name, base );
}

int
Seglist::add_symnode( Object *obj, const char * name, Iaddr base )
{
	int	found;
	Symnode	*symnode;

	found = 0;
	symnode = symlist;
	if (name == 0)
	{
		printe(ERR_internal, E_ERROR, 
			"Seglist::add_symnode", __LINE__);
		return 0;
	}
	for(symnode = symlist; symnode; symnode = symnode->next())
	{
		if ( !strcmp( name, symnode->pathname ) )
		{
			found = 1;
			break;
		}
	}
	if ( !found )
	{
		symnode = new Symnode( name, base );
		if (lastsym)
			symnode->append(lastsym);
		lastsym = symnode;
		if (!symlist)
			symlist = symnode;
		symnode->sym.symtable = obj->get_symtable();
	}
	return 1;
}

static const char *modes[2][2] = {
	"R__", "R_X",
	"RW_","RWX"
};

// macros to truncate to previous page or round to next page
// /proc maintains segment list on page boundaries
#define PTRUNC(X) ((X) & ~(pagesize - 1))
#define PROUND(X) (((X) + pagesize - 1) & ~(pagesize - 1))

int
Seglist::print_map(Procctl *pctl)
{
	Segment 	*seg;
	const char	*path;
	int		num;
	map_ctl		*map, *save;

	if (!pctl)
		return 0;
	if (pctl->get_type() == pt_core)
	{
		for (seg = first_segment; seg; seg = seg->next())
		{
			path = seg->pathname ? seg->pathname : "";
			printm(MSG_map, (Word) seg->loaddr, (Word) seg->hiaddr,
				(Word) seg->hiaddr - seg->loaddr,
				modes[seg->is_write][seg->is_exec], path);
		}
		return 1;
	}
	if (!pagesize)
		pagesize = sysconf(_SC_PAGESIZE);
	if ((num = ((Proclive *)pctl)->numsegments()) == 0)
		return 0;
	// /proc returns an extra null entry
	save = map = (map_ctl *)new map_ctl[num+1];

	if (((Proclive *)pctl)->seg_map(map) == 0)
		return 0;

	for(int i = 0; i < num; i++, map++)
	{
		int	is_write, is_exec;
		Iaddr	lo, hi;

		lo = (Iaddr)map->pr_vaddr;
		hi = (Iaddr)map->pr_vaddr + map->pr_size;
		is_write = ((map->pr_mflags & MA_WRITE) != 0);
		is_exec = ((map->pr_mflags & MA_EXEC) != 0);
		for(seg = first_segment; seg; seg = seg->next())
		{
			Iaddr	lotrunc = PTRUNC(seg->loaddr);
			Iaddr	hiround = PROUND(seg->hiaddr);
			if (lo >= stack_lo && hi <= stack_hi)
			{
				path = "[STACK]";
				break;
			}
			else if (lo >= lotrunc && lo < hiround && 
				hi <= hiround)
			{
				path = seg->pathname ?
					seg->pathname : "";
				
				break;
			}
		}
		if (!seg)
		{
			path = "";
		}
		printm(MSG_map, (Word)lo, (Word)hi, (Word)map->pr_size,
			modes[is_write][is_exec], path);
	}
	delete save;
	return 1;
}
