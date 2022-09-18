#ident	"@(#)debugger:libsymbol/common/Protoline.C	1.2"

#include	"Protoline.h"
#include	"Lineinfo.h"
#include	<string.h>
#include	<stdlib.h>

Protoline &
Protoline::add_line( Iaddr addr, long linenum )
{
	LineEntry	entry;

	entry.addr = addr;
	entry.linenum = linenum;
	vector.add(&entry,sizeof(LineEntry));
	count++;
	return *this;
}

static int 
asc_addr( const LineEntry * e1, const LineEntry * e2 )
{
	if ( e1->addr > e2->addr )
		return 1;
	else if ( e1->addr < e2->addr )
		return -1;
	else
		return 0;
}

static int 
asc_line( const LineEntry * e1, const LineEntry * e2 )
{
	if ( e1->linenum > e2->linenum )
		return 1;
	else if ( e1->linenum < e2->linenum )
		return -1;
	else if ( e1->addr > e2->addr )
		return 1;
	else if ( e1->addr < e2->addr )
		return -1;
	else
		return 0;
}

Lineinfo *
Protoline::put_line( Iaddr hiaddr )
{
	Lineinfo *	lineinfo;

	if ( !count )
	{
		return 0;
	}
	lineinfo = new Lineinfo;
	add_line( hiaddr, BIG_LINE );
	qsort( vector.ptr(), count, sizeof(LineEntry), 
		(int (*) (const void *, const void *)) asc_addr );
	lineinfo->addrpart = (LineEntry*)new(char[vector.size()]);
	memcpy((char*)lineinfo->addrpart,(char*)vector.ptr(),vector.size());
	qsort( vector.ptr(), count, sizeof(LineEntry), 
		(int (*) (const void *, const void *)) asc_line );
	lineinfo->linepart = (LineEntry*)new(char[vector.size()]);
	memcpy((char*)lineinfo->linepart,(char*)vector.ptr(),vector.size());
	lineinfo->entrycount = count;
	vector.clear();
	count = 0;
	return lineinfo;
}

