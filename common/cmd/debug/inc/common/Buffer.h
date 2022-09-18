/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Buffer_h
#define Buffer_h

#ident	"@(#)debugger:inc/common/Buffer.h	1.2"

#include <stdlib.h>

// null terminated string that grows as necessary (never shrinks)

class Buffer
{
	size_t	bytes_used;	// includes null byte
	size_t	total_bytes;
	char	*string;
	void	getmemory(size_t);

public:
		 Buffer()	{ bytes_used = total_bytes = 0;
				  string = 0;
				}
		~Buffer()	{ if (string) free(string); }

	void	add(const char *);
	void	add(char);
	void	clear()		{ bytes_used = 0; }
	size_t	size()		{ return bytes_used; } // includes null byte
	operator char *()	{ return bytes_used ? string : ""; }
};

// global scratch buffers used by various functions.
// must be used with care, since data are overwritten
// on consecutive calls
extern Buffer	gbuf1;
extern Buffer	gbuf2;
extern Buffer	gbuf3;
extern Buffer	gbuf4;

#endif // Buffer_h
