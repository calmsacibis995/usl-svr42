/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rtld:common/paths.c	1.9"

/* PATH setup and search functions */


#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include "rtinc.h"

/* directory search path - linked list of directory names */
struct pathnode {
	CONST char *name;
	int len;
	struct pathnode *next;
};

#define NODENUM	10	/* number of nodes allocated at once */

static struct pathnode *rt_dir_list = (struct pathnode *)0;

/* default search directory */
static CONST struct pathnode libdirs[] = {
	{ LIBDIR, LIBDIRLEN, (struct pathnode *)0 }
};

/* library aliasing typedefs: if we ever change aliasing to read alias tables
**	from files then these typedefs will have to go into some
**	globally available "Include Files"
*/

typedef struct {
	char *name;	/* the actual name */
	int  listCnt;	/* number of names in the next field */
	char **list;	/* array of names , Null terminated */
} aliasInfo, *aliasInfoP;
 
static char *libcAliasNames[] = {"/usr/lib/libc.so.1.1", "/usr/lib/ld.so.1"};
static aliasInfo aliases[] = {
	{"/usr/lib/libc.so.1", 2, libcAliasNames},
	{"libc.so.1", 2, libcAliasNames},
	{ 0}
};

static aliasInfoP _rtGetAlias(char *libName)
{
	aliasInfoP ptr;

	for(ptr=aliases; ptr->name; ptr++){
		if(!_rtstrcmp(ptr->name, libName)) return ptr;
	}
	return 0;
}

#define M_ENullName "%s: %s: attempt to open file with null name"

/*
** Given a library name converts it into the corresponding filename.
**
** Algorithm : if library name contains a '/' returns it untouched
**		else go through  rt_dir_list and find the first
**		directory that contains a file with the same name
**		as the library and return its absolute name.
**      Note : returned  pointer points to an internal static area.
**		Caller is responsible for copying it into his own space.
*/

static char *transalateLibName(char *name, char *tmpName)
{
	char   *tmpPtr;
	int	nameLen;
	register struct pathnode *dirInfo;

	nameLen = _rtstrlen(name);

	for(tmpPtr=name; *tmpPtr; tmpPtr++){
		if(*tmpPtr == '/') {
			/*
			if(_access(name, F_OK|R_OK) != 0)
				return 0;
			*/

			_rtstrcpy(tmpName, name);
			return tmpName;
		}
	}

	for(dirInfo = rt_dir_list; dirInfo; dirInfo = dirInfo->next){
		if((nameLen + dirInfo->len + 1) > PATH_MAX) /* 1 for the '/'*/
			continue;

		(void)_rtstr3cpy(tmpName, dirInfo->name, "/", name);
		if(_access(tmpName, F_OK|R_OK) == 0){ /* file exists */
			return tmpName;
		}
	}
	return 0;
}

/*
**	Given a library name returns the number of files that need
**	to be opened and mapped in, the names of files is returned
**	in the second argument. 
*/

int _so_find(libName, names)
	CONST char *libName;
	char  ***names;	/* return array of lib names to be loaded */
{
	aliasInfoP alias;
	static char *nameArray[6]; /* static because we return pointer to
				    ** it to caller. */
	int fileCnt;	/* number of file names returned */
	char   tmpName[PATH_MAX+2];

	DPRINTF(LIST, 
		(2, "rtld: _so_find(%s, 0x%x)\n", (libName ? libName : (CONST char *)"0"), (unsigned long)names));

	if (!libName) {
		_rt_lasterr(M_ENullName, (char*) _rt_name,_proc_name);
		return -1;
	}
	if(alias = _rtGetAlias((char *)libName)){
		int index;
		fileCnt = alias->listCnt;
		for(index=0; index < fileCnt; index++){
		    char *fname = transalateLibName(alias->list[index],tmpName);
		    if(fname) {
			nameArray[index]=(char *)_rtmalloc(_rtstrlen(fname)+1);
			_rtstrcpy(nameArray[index], fname);
		    } else {
			/* Could not find some shared object */
			fileCnt = 0;
			break;
		    }
		}
	} else {
		/* just expand libName and return pointer to the 
		** expanded name */
		char *fname = transalateLibName((char *)libName,tmpName);
		if(fname){
		    nameArray[0]=(char *)_rtmalloc(_rtstrlen(fname)+1);
		    _rtstrcpy(nameArray[0], fname);
		    fileCnt = 1;
		} else {
		    fileCnt = 0;
		}
	}
	if(fileCnt == 0){
		_rt_lasterr("%s : %s : error opening %s", 
			_rt_name, _proc_name, libName);
		return 0;
	}

DPRINTF(LIST, (2, "rtld: _so_find(%s,%s) = %d", libName, nameArray[0],fileCnt));
	nameArray[fileCnt] = 0;
	*names = nameArray;
	return fileCnt;
}

/* set up directory search path: rt_dir_list
 * consists of directories (if any) from run-time list
 * in a.out's dynamic, followed by directories (if any)
 * in environment string LD_LIBRARY_PATH, followed by LIBDIR;
 * if we are running setuid or setgid, no directories from LD_LIBRARY_PATH
 * are permitted
 * returns 1 on success, 0 on error
 */
int _rt_setpath(envdirs, rundirs)
CONST char *envdirs, *rundirs;
{
	int elen = 0, rlen = 0, ndx, flen;
	register char *rdirs, *edirs;
	register struct pathnode *p1, *p2 = (struct pathnode *)0;

	DPRINTF(LIST,(2, "rtld: rt_setpath(%s, %s)\n",envdirs ? envdirs :
		(CONST char *)"0", rundirs ? rundirs : (CONST char *)"0"));



	/* allocate enough space for rundirs, envdirs and first
	 * chunk of pathnode structs - we allocate space for
	 * twice the size of envdirs and rundirs to allow for
	 * extra nulls at the end of each directory (foo::bar
	 * becomes foo\0.\0bar\0); this is overkill, but allows
	 * for the worst case and is faster than malloc'inc space
	 * for each directory individually
	 */
	if (envdirs)
		elen = _rtstrlen(envdirs);
	if (rundirs)
		rlen = _rtstrlen(rundirs);
	if ((rlen + elen) > 0) {
		if ((p1 = rt_dir_list = (struct pathnode *)_rtmalloc((2 * elen) + (2 * rlen) + (NODENUM 
			* sizeof(struct pathnode)))) == 0) 
			return 0;
		rdirs = (char *)p1 + (NODENUM * sizeof(struct pathnode));
		edirs = rdirs + (2 * rlen);
		ndx = 0;
		if (rundirs) {
			while (*rundirs) {
				p1->name = rdirs;
				p2 = p1;
				ndx++;
				if (ndx >= NODENUM) {
				/* allocate another set of pathnodes */
					if ((p1->next = (struct pathnode *)
						_rtmalloc(NODENUM * sizeof(struct 
						pathnode))) == 0) 
						return 0;
					ndx = 0;
				}
				else p1->next = p1 + 1;
				p1 = p1->next;
				if (*rundirs == ':') {
					*rdirs++ = '.';
					flen = 1;
				}
				else {
					flen = 0;
					while (*rundirs != ':' && *rundirs) {
						*rdirs++ = *rundirs++;
						flen++;
					}
				}
				*rdirs++ = '\0';
				p2->len = flen;
				if (*rundirs) 
					rundirs++;
			} /* end while (*rundirs) */
		} /* end if (rundirs) */
		/* envdirs is of form [PATH1][;PATH2] */
		if (envdirs && /* see if we are running secure */
			(_getuid() == _geteuid())  &&
			(_getgid() == _getegid())
			) {

			if (*envdirs == ';')
				++envdirs;
			while (*envdirs) {
				p1->name = edirs;
				p2 = p1;
				ndx++;
				if (ndx >= NODENUM) {
				/* allocate another set of pathnodes */
					if ((p1->next = (struct pathnode *)
						_rtmalloc(NODENUM * sizeof(struct 
						pathnode))) == 0) 
						return 0;
					ndx = 0;
				}
				else p1->next = p1 + 1;
				p1 = p1->next;
				if (*envdirs == ':') {
					*edirs++ = '.';
					flen = 1;
				}
				else {
					flen = 0;
					while (*envdirs != ':' && *envdirs != ';' && *envdirs) {
						*edirs++ = *envdirs++;
						flen++;
					}
				}
				*edirs++ = '\0';
				p2->len = flen;
				if (*envdirs) 
					envdirs++;
			} /* while (*envdirs) */
		} /*if (envdirs) */
	} 

	/* add LIBDIR to end of list */
	if (!p2)
		rt_dir_list = (struct pathnode *)libdirs;
	else 
		p2->next = (struct pathnode *)libdirs;
#ifdef DEBUG
	if (_debugflag & LIST) {
		p1 = rt_dir_list;
		while(p1) {
			_rtfprintf(2, "%s\n",p1->name);
			p1 = p1->next;
		}
	}
#endif
	return(1);
}
