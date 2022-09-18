#ident	"@(#)debugger:libdbgen/common/Wordstack.C	1.1"

#include	"Wordstack.h"

void
Wordstack::push( unsigned long word )
{
	vector.add(&word,sizeof(unsigned long));
	++count;
}

unsigned long
Wordstack::pop()
{
	unsigned long *	p;
	unsigned long 	top_item = 0;

	if ( count > 0 )
	{
		p = (unsigned long *)vector.ptr();
		--count;
		top_item = p[count];
		vector.drop(sizeof(unsigned long));
	}
	return top_item;
}
