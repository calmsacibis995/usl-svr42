#ident	"@(#)debugger:libexecon/common/Segment.C	1.2"

#include "Object.h"
#include "Segment.h"
#include "Procctl.h"
#include "global.h"
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

Segment::Segment(Procctl *pctl, const char *path, Object *obj,
	Iaddr lo, long sz, long b, 
	long ss_base, int iswrite, int isexec)
{
	char	*p;

	access = pctl;
	object = obj;
	if ( path != 0 )
	{
		p = new(char[strlen(path) + 1]);
		strcpy( p, path );
		pathname = p;
	}
	else
		pathname = 0;
	loaddr = lo;
	hiaddr = loaddr + sz;
	base = b;
	sym.ss_base = ss_base;
	// is_write and is_exec are either 0 or positive,
	// not necessarily 1
	is_write = iswrite ? 1 : 0;
	is_exec = isexec ? 1 : 0;
	sym.symtable = 0;
}

int
Segment::read( Iaddr addr, void * buffer, int len )
{
	long	offset;

	offset = addr - loaddr + base;
	return access->read( offset, buffer, len );
}

int
Segment::write( Iaddr addr, const void * buffer, int len )
{
	long	offset;

	offset = addr - loaddr + base;
	return ((Proclive *)access)->write( offset, buffer, len );
}

static int	size[] = {	
	0,
	sizeof(Ichar),
	sizeof(Iint1),
	sizeof(Iint2),
	sizeof(Iint4),
	sizeof(Iuchar),
	sizeof(Iuint1),
	sizeof(Iuint2),
	sizeof(Iuint4),
	sizeof(Isfloat),
	sizeof(Idfloat),
	sizeof(Ixfloat),
	sizeof(Iaddr),
	sizeof(Ibase),
	sizeof(Ioffset),
	0
};

int
stype_size( Stype stype )
{
	return size[stype];
}

int
Segment::read( Iaddr addr, Stype stype, Itype & itype )
{
	long	offset;

	offset = addr - loaddr + base;
	return(access->read( offset, &itype, size[stype] ));
}

int
Segment::write( Iaddr addr, Stype stype, const Itype & itype )
{
	long	offset;

	offset = addr - loaddr + base;
	return(((Proclive *)access)->write( offset, &itype, size[stype] ));
}

int
Segment::get_symtable()
{
	if ( sym.symtable != 0 )
	{
		return 1;
	}
	if (!object)
		return 0;
	sym.symtable = object->get_symtable();
	return 1;
}
