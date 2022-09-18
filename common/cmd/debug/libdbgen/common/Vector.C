#ident	"@(#)debugger:libdbgen/common/Vector.C	1.4"
#include	"Vector.h"
#include	"Interface.h"
#include	<string.h>

extern void	new_handler();	// declared here instead of including utility.h
				// to avoid pulling in extra stuff for the gui
				// only a problem with CC 1.2 which generates
				// references to Ptrlist vtbles even though
				// not used in this file

// BSIZ is the minimum growth of the vector in bytes.
#define BSIZ	100

void
Vector::getmemory(size_t howmuch)	// clients assume word alignment!
{
	size_t	sz;

	sz = howmuch < BSIZ ? BSIZ : howmuch;
	if (total_bytes == 0)
	{
		vector = (char *)malloc(sz);
		total_bytes = sz;
	}
	else
	{
		total_bytes = total_bytes + sz;
		vector = (char *)realloc(vector,total_bytes);
	}
	check();
}

Vector::Vector(Vector & v)
{
	vector = (char *)malloc(v.total_bytes);
	check();
	memcpy(vector,v.vector,v.bytes_used);
	total_bytes = v.total_bytes;
	bytes_used = v.bytes_used;
}

Vector &
Vector::add(void * p, size_t sz)
{
	if (sz > (total_bytes - bytes_used))
	{
		getmemory(sz);
	}
	memcpy((char*)vector + bytes_used, (char *)p, sz);
	bytes_used += sz;
	return *this;
}

Vector &
Vector::operator=(Vector & v)
{
	if (this != &v)
	{
		if (vector) free (vector);
		if (v.total_bytes > 0) // check fails for 0
		{
			vector = (char *)malloc(v.total_bytes);
			check();
			memcpy(vector,v.vector,v.bytes_used);
		}
		total_bytes = v.total_bytes;
		bytes_used = v.bytes_used;
	}
	return *this;
}

#ifdef DEBUG

Vector &
Vector::report(char * msg)
{
	if (msg)
		printf("%s\n",msg);
	printf("\ttotal bytes : %d (%#x)\n",total_bytes,total_bytes);
	printf("\tbytes used : %d (%#x)\n",bytes_used,bytes_used);
	printf("\tvector : (%#x) >%s<\n",vector,vector);
	if (bytes_used != 0)
		printf("\tvector[%d] : %c (%#x)\n",bytes_used-1,
			((char *)vector)[bytes_used-1],
			((long *)vector)[bytes_used-1]);
	printf("\tvector[%d] : %c (%#x)\n",bytes_used,
		((char *)vector)[bytes_used],((long *)vector)[bytes_used]);
	return *this;
}
#endif

void
Vector::check()
{
	if (!vector)
		new_handler();
}
