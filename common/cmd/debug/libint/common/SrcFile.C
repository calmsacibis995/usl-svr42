#ident	"@(#)debugger:libint/common/SrcFile.C	1.2"
#include "SrcFile.h"
#include "Interface.h"
#include "Vector.h"
#include "global.h"
#include "utility.h"
#include "LWP.h"
#include "Process.h"
#include "Program.h"
#include "Language.h"
#include <libgen.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>

int	pathage;
char	*global_path;


// The vector.adds in the contructor set up the 0 and 1 entries
// in the array of offsets.  0 is unused, but is there for
// correct indexing.  The offset of the first line (vector[1])
// is always zero.  That works even for an empty file
// program, global_age, and local_age are needed to differentiate
// between files with the same name in different directories

SrcFile::SrcFile(LWP *lwp, FILE *fd, const char *fname)
{
	long	number = 0;

	fptr = fd;
	name = new(char[strlen(fname)+1]);
	strcpy(name, fname);
	hi = 1;
	not_last = 1;
	progptr = lwp->process()->program();
	g_age = pathage;
	l_age = (int)progptr->pathage();

	vector.add( &number, sizeof( long ) );
	vector.add( &number, sizeof( long ) );
}

// print a warning if the source file has a more recent
// modification date than the object file.
// keeping a linked list of all the source files for which a
// messages has already been printed allows it to avoid printing
// the message for any file more than once.

static void
check_newer(LWP *p, const char *fnpath)
{
	typedef struct	fileid {
		ino_t		fino;
		dev_t		fent;
		struct fileid	*next;
	} fileid;
	static fileid	*fhead = (fileid *)0;

	struct stat 	stbuf;

	if (!fnpath || !(*fnpath))
		return;

	if (stat(fnpath,&stbuf) == -1) 
	{
		printe(ERR_no_access, E_ERROR, fnpath);
		return;
	}

	if (stbuf.st_mtime > p->process()->program()->symfiltime()) 
	{
		for (fileid* ptr = fhead; ptr; ptr = ptr->next) 
		{
			if (ptr->fent == stbuf.st_dev
				&& ptr->fino == stbuf.st_ino)
				break;
		}
		if (!ptr)
		{
			fileid *nfile = new fileid;

			printe(ERR_newer_file, E_WARNING, fnpath, p->exec_name());
			nfile->fino = stbuf.st_ino;
			nfile->fent = stbuf.st_dev;
			nfile->next = fhead;
			fhead = nfile;
		}
	}
	return;
}

// walk the path, and for each subpath, try to open file name
// in that directory; :: is equivalent to :.:

static SrcFile *
search_path(LWP *lwp, const char *path, const char *fname)
{
	size_t	len;
	char	buf[PATH_MAX];
	FILE	*fptr;
	SrcFile	*file;

	while (path && *path)
	{
		len = strcspn(path, ":");
		if (len)
		{
			strncpy(buf, path, len);
			buf[len] = '/';
			buf[len+1] = '\0';
		}
		else	// ::
			strcpy(buf, "./");
		strcat(buf, fname);

		if ((fptr = debug_fopen(buf, "r")) != NULL)
		{
			file = new SrcFile(lwp, fptr, buf);
			check_newer(lwp, buf);
			return file;
		}
		else
			path += (len + 1);
	}
	// try just relative to current dir
	if ((fptr = debug_fopen(fname, "r")) != NULL)
	{
		file = new SrcFile(lwp, fptr, fname);
		check_newer(lwp, fname);
		return file;
	}
	return 0;
}

// open_srcfile tries to find the file named name in 4 steps:
// 1) if the current language is C++ and the name ends in "..c",
//     replace the "..c" with ".C"
// 2) create the colon-separated list of directories from the
//	program-specific path and the global path, and search
//	for name in that list.  If that doesn't work, then
// 3) if name is actually a path name (has a / in it),
//	search the list of directories again with just the last
//	component of the name
// 4) if the current language is C++ and if the name ends in ".c",
//      replace the suffix with '.C' and try again
//    else if current language is C++ and the name ends in "..c",
//      try again with ".c"


static SrcFile *
open_srcfile(LWP *lwp, const char *name)
{
	SrcFile		*file = 0;
	char		*npath = 0;
	char		*p, *q;
	int		len = strlen(name);
	char		*name2 = 0;
	int		cplus = 0;
	int		count = 0;

	npath = set_path(lwp->process()->program()->src_path(),
		global_path);

	if (current_language(0) == CPLUS
		|| current_language(lwp) == CPLUS)
		cplus = 1;
	if ((strcmp(name+(len-3), "..c") == 0) && cplus)
	{
		name2 = new char[len+1];
		strcpy(name2, name);
		name2[len-2] = 'C';
		name2[len-1] = 0;
		p = name2;
	}
	else
		p = (char *)name;
	while(1)
	{
		if ((file = search_path(lwp, npath, p)) == 0)
		{
			if ((q = strrchr(p, '/')) != 0)
				file = search_path(lwp, npath, q);
		}
		if (!count && !file && 
			(strcmp(name+(len-2), ".c") == 0) && cplus)
		{
			if (name2)
				delete name2;
			name2 = new char[len+1];

			strcpy(name2, name);
			if (name2[len-3] == '.')
			{
				// "..c"
				name2[len-2] = 'c';
				name2[len-1] = 0;
			}
			else
			{
				name2[len-1] = 'C';
			}
			p = name2;
			count++;
		}
		else
			break;
	}
	delete name2;
	delete npath;
	return file;
}

// Search through the list of saved source files for fname.
// If there is no match, or if the either the global path
// or the pre-program path has changed since the file was
// opened, call open_srcfile to do a path search

#define NUMSAVED	10

SrcFile *
find_srcfile(LWP *lwp, const char *fname)
{
	static SrcFile	*ftab[NUMSAVED];
	static int	nextslot = 0;
	SrcFile		*file;
	char		*bname = basename((char *)fname);

	if (!bname)
		return 0;

	for ( register int i = 0 ; i < NUMSAVED ; i++ ) 
	{
		file = ftab[i];
		if (file && file->program() == lwp->process()->program()
			&& strcmp(bname, basename(file->filename())) == 0)
		{
			if (file->global_age() != pathage
				|| file->local_age()
					!= lwp->process()->program()->pathage())
				ftab[i] = file = open_srcfile(lwp, fname);
			return file;
		}
	}

	if (ftab[nextslot]) 
		delete ftab[nextslot];
	file = ftab[nextslot] = open_srcfile(lwp, fname);
	if (++nextslot >= NUMSAVED)
		nextslot = 0;

	return file;
}

#define SBSIZE	513

// read a line, up to SBSIZE bytes, from the source file
// if the line is longer than SBSIZE bytes, throw the rest away
// (nobody will want to see all that on their screen anyway)
// the newline is NOT included in the string returned

static char *
readline(FILE *fptr)
{
	static char	buf[SBSIZE];
	int 		c, len;

	if (fgets(buf, SBSIZE, fptr) == 0)
		return 0;

	len = strlen(buf);
	if (len == (SBSIZE-1) && buf[SBSIZE-2] != '\n')
	{
		while ((c = getc(fptr)) != EOF && c != '\n')
			;
	}
	else
	{
		buf[len - 1] = 0;
	}
	return buf;
}

// read line num from the source file.  if we haven't done a num_lines(),
// or read up to this line before from this file, we have to walk through
// each line in the file, but line and num_lines build a list of line
// offsets, so the next time line is called, it can seek there directly
// the newline is NOT included in the string returned

char *
SrcFile::line(int num)
{
	long 	*array;
	long	offset;
	char	*ptr;

	if (num <= 0)
		return 0;

	array = (long*) vector.ptr();
	if (num <= hi)	// already know the offset, just go there and read
	{
		if (fseek(fptr, array[num], 0) != 0)
			return 0;
		return readline(fptr);
	}
	if (fseek(fptr, array[hi], 0) != 0)
		return 0;

	while (hi <= num)
	{
		if ((ptr = readline(fptr)) == 0)
		{
			not_last = 0;
			--hi;
			return 0;
		}
		else
		{
			offset = ftell( fptr );
			vector.add( &offset, sizeof( long ) );
			++hi;
		}
	}
	return ptr;
}

// determine the total number of lines in the file

long
SrcFile::num_lines()
{
	long	*array;
	long	offset;

	array = (long*) vector.ptr();
	if (fseek(fptr, array[hi], 0) != 0)
		return 0;

	while ( not_last )
	{
		if (!readline(fptr))
		{
			not_last = 0;
			--hi;
		}
		else
		{
			offset = ftell( fptr );
			vector.add( &offset, sizeof( long ) );
			++hi;
		}
	}
	return hi;
}
