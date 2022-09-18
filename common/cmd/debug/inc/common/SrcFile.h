/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef SrcFile_h
#define SrcFile_h
#ident	"@(#)debugger:inc/common/SrcFile.h	1.1"

#include	"Vector.h"
#include	<stdio.h>

class LWP;
class Program;

class SrcFile {
	Vector		vector;	// list of file offset for the beginning of
				// lines, indexed by line numbers
	FILE		*fptr;
	Program		*progptr;	// program the source file is
					// associated with
	int		hi;		// highest line number read so far
	int		not_last;	// indicates hi is not end-of-file
	int		g_age;		// age of global path at SrcFile creation
	int		l_age;		// age of local path at SrcFile creation
	char		*name;		// file name

public:
			SrcFile(LWP *, FILE *, const char *fname);
			~SrcFile()	{ if (fptr) fclose(fptr); delete name; }

	char		*filename()	{ return name; }
	Program		*program()	{ return progptr; }
	int		global_age()	{ return g_age; }
	int		local_age()	{ return l_age; }

	char		*line(int linenum);
	long		num_lines();
};

SrcFile	*find_srcfile(LWP *, const char *fname);

#endif
