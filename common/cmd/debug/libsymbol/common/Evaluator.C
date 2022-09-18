#ident	"@(#)debugger:libsymbol/common/Evaluator.C	1.5"
#include	"Evaluator.h"
#include	"Syminfo.h"
#include	"Machine.h"
#include	"Object.h"
#include	"ELF.h"
#include	"Coff.h"
#include	"global.h"
#include	<sys/types.h>
#include	<unistd.h>
#include	<string.h>
#include	<signal.h>
#include	"Interface.h"

#define NOMORE	(-1)	/* for file offsets */

static Attribute * find_attr(Attribute *, Attr_name);

Evaluator::Evaluator( int fd, Object *obj )
{
	fdesc = fd;
	first_record = 0;
	elf_record = 0;
	next_disp = 0;
	elf_disp = 0;
	no_elf_syms = 1;
	current_entry = 0;
	dwarfbuild = 0;
	elfbuild = 0;
	coffbuild = 0;
	switch( file_type = obj->file_format() ) {
	case ff_elf:
		dwarfbuild = new Dwarfbuild((ELF *)obj);
		elfbuild = new Elfbuild((ELF *)obj);
		break;
	case ff_coff:
		coffbuild = new Coffbuild((Coff *)obj);
		elf_disp = NOMORE;
		break;
	}
}

Attribute *
Evaluator::first_file()
{
	long		first_offset;
	long		elf_offset;

	if ( first_record != 0 )
	{
		return first_record;
	}
	if ( file_type == ff_coff )
	{
		first_offset = coffbuild->first_symbol();
		first_record = coffbuild->make_record(first_offset, WANT_FILE);
		return first_record;
	}
	else if ( file_type != ff_elf )
	{
		return 0;
	}

	if ((elf_offset = elfbuild->first_symbol()) != 0 )
	{
		elf_record = elfbuild->make_record( elf_offset );
	}
	if ((first_offset = dwarfbuild->first_file()) != 0 )
	{
		first_record = dwarfbuild->make_record( first_offset );
	}
	else
		first_record = elf_record;
	return first_record;
}

Attribute *
Evaluator::arc( Attribute * attrlist, Attr_name attrname )
{
	Attribute *	a;
	Build *build = coffbuild ? (Build *)coffbuild : (Build *)dwarfbuild;

	if ( (a = find_attr( attrlist, attrname )) == 0 )
	{
		return 0;
	}
	else if ( a->form == af_symbol )
	{
		return a;
	}
	else if ( a->name == an_child )
	{
		return add_children( a, attrlist );
	}
	else if ( a->name == an_parent )
	{
		return add_parent(attrlist,
				 build->make_record(a->value.word, 0));
	}
	else
	{
		return add_node( a );
	}
}

Attribute *
Evaluator::add_node( Attribute * a )
{
	long	offset = a->value.word;

	switch ( a->form ) {
	case af_coffrecord:
		a->value.symbol = coffbuild->make_record( offset );
		break;
	case af_cofffile:
		a->value.symbol = coffbuild->make_record( offset, WANT_FILE );
		break;
	case af_dwarfoffs:
		a->value.symbol = dwarfbuild->make_record( offset );
		// hook elf records onto end of dwarf source file list,
		// so that debug can find static symbols even if file was
		// compiled with -g
		if (!a->value.symbol && a->name == an_sibling
			&& offset >= dwarfbuild->last_entry())
			a->value.symbol = elf_record;
		break;
	case af_elfoffs:
		a->value.symbol = elfbuild->make_record( offset );
		break;
	default:
		return a;
	}
	a->form = af_symbol;
	return a;
}

Attribute *
Evaluator::add_parent( Attribute * b, Attribute * ancestor )
{
	Attribute *	p;

	if( ancestor == 0 )
	{
		return 0;
	}

	if ( (p = find_attr( b, an_parent )) != 0 )
	{
		p->value.symbol = ancestor;
		p->form = af_symbol;
	}
	return p;
}

Attribute *
Evaluator::add_children( Attribute * a, Attribute * ancestor )
{
	long		offset, limit, scansize;
	Attribute	*b, *x;

	if ( (b = find_attr( ancestor, an_scansize )) == 0 )
	{
		return 0;
	}
	offset = a->value.word;
	scansize = b->value.word;
	limit = offset + scansize;
	x = a;
	Build *build = coffbuild ? (Build *)coffbuild : (Build *)dwarfbuild;
	while ( x != 0 )
	{
		offset = x->value.word;
		if ( (offset < limit)
			&& (x->form == af_dwarfoffs
				|| x->form == af_coffrecord
				|| x->form == af_elfoffs))
		{
			if (x->form == af_elfoffs)
				build = (Build *)elfbuild;
			b = build->make_record( offset );
			(void)add_parent( b, ancestor );
			x->value.symbol = b;
			x->form = af_symbol;
			x = find_attr( b, an_sibling );
		}
		else
		{
			x->value.symbol = 0;
			x->form = af_symbol;
			break;
		}
	}
	return a;
}

Attribute *
Evaluator::attribute( Attribute * attrlist, Attr_name attr_name )
{
	Attribute *	a;

	if ( (a = find_attr(attrlist,attr_name)) == 0 )
		return 0;
	switch (a->form)
	{
		case af_dwarfoffs:
			a->value.symbol = dwarfbuild->make_record(a->value.word);
			a->form = af_symbol;
			break;
		case af_dwarfline:
			a->value.lineinfo = dwarfbuild->line_info(a->value.word);
			a->form = af_lineinfo;
			break;
         	case af_coffrecord:
                        a->value.symbol = coffbuild->make_record( a->value.word,0 );
                        a->form = af_symbol;
                        break;
		case af_coffline:
                        a->value.lineinfo = coffbuild->line_info(a->value.word);
                        a->form = af_lineinfo;
                        break;
		case af_coffpc:
			printe(ERR_debug_entry, E_WARNING);
			break;
		case af_elfoffs:
			a->value.symbol = elfbuild->make_record(a->value.word);
			a->form = af_symbol;
			break;
	}
	return a;
}

NameEntry *
Evaluator::get_global( const char * name )
{
	char *		nom;
	Syminfo		syminfo;
	NameEntry *	n;
	Build *		build = coffbuild ? (Build *)coffbuild : (Build *)dwarfbuild;
	Attr_form	form = coffbuild ? af_coffrecord : af_dwarfoffs;

	if ( (next_disp != NOMORE) )
	{
		if ( next_disp == 0 ) next_disp = build->globals_offset();
		while ( build->get_syminfo( next_disp, syminfo ) != 0 )
		{
			if (interrupt & sigbit(SIGINT))
				return 0;
			nom = (char*)syminfo.name;
			if ( syminfo.type == st_file )
			{
				next_disp = syminfo.child;
				continue;
			}
			if ( syminfo.bind != sb_global ||
						 !syminfo.resolved )
			{
				next_disp = syminfo.sibling;
				continue;
			}
			n = namelist.add( nom, next_disp, form );
			next_disp = syminfo.sibling;
			if (!name || n->cmpName(name) == 0)
			{
				return n;
			}
		}
		next_disp = NOMORE;
	}

	// not found in COFF or DWARF, try ELF (if an ELF file)
	if ( (file_type == ff_elf) && (elf_disp != NOMORE) )
	{
		if ( elf_disp == 0 ) elf_disp = elfbuild->first_symbol();
		while ( elfbuild->get_syminfo( elf_disp, syminfo ) != 0 )
		{
			if (interrupt & sigbit(SIGINT))
				return 0;
			nom = elfbuild->get_name( syminfo.name );
			if ( ((syminfo.bind != sb_weak) &&
				(syminfo.bind != sb_global)) ||
				((syminfo.type != st_object) &&
				(syminfo.type != st_func)) ||
				(!syminfo.resolved) )
			{
				elf_disp = syminfo.sibling;
				continue;
			}
			n = namelist.add( nom, elf_disp, af_elfoffs );
			elf_disp = syminfo.sibling;
			if (!name || n->cmpName(name) == 0)
			{
				if ( n != 0 ) return n;
			}
		}
		elf_disp = NOMORE;
	}
	return 0;
}

NameEntry *	
Evaluator::first_global()
{
	if (next_disp != NOMORE || elf_disp != NOMORE)
	{
		// first time through, create the NameEntry table
		while (get_global(0))
			;
	}

	current_entry = (NameEntry*)namelist.tfirst();
	return current_entry;
}

NameEntry *
Evaluator::next_global()
{
	if (next_disp != NOMORE || elf_disp != NOMORE)
	{
		// first time through, create the NameEntry table
		while (get_global(0))
			;
	}
	if ( current_entry == 0 )
	{
		return 0;
	}
	current_entry = (NameEntry*)(current_entry->next());
	return current_entry;
}

NameEntry *
Evaluator::prev_global()
{
	if (next_disp != NOMORE || elf_disp != NOMORE)
	{
		// first time through, create the NameEntry table
		while (get_global(0))
			;
	}
	if ( current_entry == 0 )
	{
		return 0;
	}
	current_entry = (NameEntry*)(current_entry->prev());
	return current_entry;
}

Attribute *
Evaluator::evaluate( NameEntry * n )
{
	Attribute *	a;

	if ( n == 0 )
	{
		return 0;
	}
	if ( n->form == af_dwarfoffs )
	{
		a = dwarfbuild->make_record( n->value.word );
	}
	else if ( n->form == af_coffrecord )
	{
		a = coffbuild->make_record( n->value.word );
	}
	else if ( n->form == af_elfoffs )
	{
		a = elfbuild->make_record( n->value.word );
	}
	else
	{
		return n->value.symbol;
	}
	n->value.symbol = a;
	n->form = af_symbol;
	return a;
}

// Search for a NameEntry with the specified name
Attribute *
Evaluator::find_global( const char * name )
{
	NameEntry *	n;
	NameEntry	node;

	node.namep = name;
	n = (NameEntry*)namelist.tlookup(node);
	if ( n != 0 || (n = get_global(name)) != 0 )
	{
		current_entry = n;
		return evaluate(n);
	}
	return 0;
}
		
// Search for a AddrEntry containing a specified address
// Assumes graph has already been searched; uses only addrlist
Attribute *
Evaluator::lookup_addr( Iaddr addr )
{
	AddrEntry	node;
	AddrEntry *	a;
	long		offset;
	Syminfo		syminfo;
	Build *		build = coffbuild ? (Build *)coffbuild : (Build *)elfbuild;
	Attr_form	form = coffbuild ? af_coffrecord : af_elfoffs;

	node.loaddr = addr;
	node.hiaddr = addr;
	if ( no_elf_syms )
	{
		no_elf_syms = 0;
		offset = build->globals_offset();
		while ( build->get_syminfo( offset, syminfo ) != 0 )
		{
			if (syminfo.bind == sb_local &&
				(syminfo.type == st_object ||
				 syminfo.type == st_func) &&
				 syminfo.resolved)
                        {
                                addrlist.add( syminfo.lo, syminfo.hi, offset, form );
			}
			// allows undefined global functions if
			// their low pc is non-zero == plt entries
			else if ( syminfo.bind == sb_global &&
				(((syminfo.type == st_object) 
				&& syminfo.resolved) ||
				((syminfo.type == st_func) &&
				(syminfo.resolved || 
				(syminfo.lo != 0)))) )
                        {
                                addrlist.add( syminfo.lo, syminfo.hi, offset, form );
			}
			offset = syminfo.sibling;
		}
		addrlist.complete();
	}
	a = (AddrEntry*)addrlist.tlookup(node);
	if ( a == 0 )
	{
		return 0;
	}
	if ( a->form == af_symbol )
	{
		return a->value.symbol;
	}
	if ( a->form == form )
	{
		Attribute	*attr, *list;
		a->value.symbol = list =
			build->make_record(a->value.word);
		a->form = af_symbol;

		// for functions, check for lowpc == hipc;
		// this means we have no size information;
		// if this is true use hiaddr from addrlist
		// for the hi pc
		attr = attribute(list, an_tag);
		if (attr)
		{
			if (attr->value.tag == t_subroutine ||
				attr->value.tag == t_global_sub)
			{
				Attribute	*lowpc, *hipc;
				lowpc = attribute(list, an_lopc);
				hipc = attribute(list, an_hipc);
				if (lowpc && hipc)
				{
					if (lowpc->value.addr ==
						hipc->value.addr)
					{
						hipc->value.addr =
							a->hiaddr;
					}
				}
			}
		}
		return list;
	}
	return 0;
}

static Attribute *
find_attr(Attribute * a, Attr_name name)
{
	register Attribute *	p;
	Attribute *		rec;
	static long		last;

	if ( a != 0 )
	{
		p = rec = a;
		last = p->value.word - 1;
		rec[last].name = name;
		while (p->name != name) p++ ;
		rec[last].name = an_nomore;
		return (p == rec+last) ? 0 : p ;
	}
	else
		return 0;
}
