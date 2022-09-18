#ident	"@(#)debugger:libdbgen/common/Buffer.C	1.3"
#include	"Buffer.h"
#include	"Interface.h"
#include	<stdlib.h>
#include	<string.h>

extern void	new_handler();	// declared here instead of including utility.h
				// to avoid pulling in extra stuff for the gui
				// only a problem with CC 1.2 which generates
				// references to Ptrlist vtbles even though
				// not used in this file

// BSIZ is the minimum growth of the string in bytes.
#define BSIZ	100

void
Buffer::getmemory(size_t howmuch)
{
	size_t	sz;

	sz = howmuch < BSIZ ? BSIZ : howmuch;
	if (total_bytes == 0)
	{
		total_bytes = sz;
		string = (char *)malloc(sz);
	}
	else
	{
		total_bytes = total_bytes + sz;
		string = (char *)realloc(string,total_bytes);
	}
	if (string == 0)
		new_handler();
}

// append a string to the existing string, leaving room for the
// null byte

void
Buffer::add(const char *p)
{
	size_t sz = strlen(p) + 1;

	if (sz > (total_bytes - bytes_used))
		getmemory(sz);

	if (bytes_used)		// over-write the existing null byte
		--bytes_used;
	strcpy(string + bytes_used, p);
	bytes_used += sz;
}

// append c to the string, making sure the string is
// still null-terminated

void
Buffer::add(char c)
{
	if (bytes_used)		// over-write the existing null byte
		--bytes_used;
	if (total_bytes - bytes_used < 2)
		getmemory(2);
	string[bytes_used++] = c;
	string[bytes_used++] = '\0';
}

// global scratch buffers used by various functions.
// must be used with care, since data are overwritten
// on consecutive calls

Buffer	gbuf1;
Buffer	gbuf2;
Buffer	gbuf3;
Buffer	gbuf4;
