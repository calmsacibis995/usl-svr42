/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wctype/loadtab.c	1.1.1.2"
#ident  "$Header: loadtab.c 1.2 91/06/26 $"

/*
* loadtab - set character table for multi-byte characters
*/
#include <locale.h>
#include "_locale.h"
#include <fcntl.h>
#include <widec.h>
#include <wctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#define	SZ	524

/* pointers to multi-byte character table */
struct	_wctype	*_wcptr[3] = {0, 0, 0}; 

/* _flag=1 if multi-byte character table is updated */
int _lflag = 1; 

int
_loadtab()
{
	register int fd, ret;
	struct stat f_stat;
	static char *ptr = 0;
	char *malloc();
	int i;
	unsigned lsize;

	ret = -1;
	if (ptr != 0) {
		free(ptr);
		ptr = (char *)NULL;
	}
	if ((fd = open(_fullocale(_cur_locale[LC_CTYPE],"LC_CTYPE"), O_RDONLY)) == -1) {
		return -1;
	} else if (fstat(fd, &f_stat) == 0) {
		if (f_stat.st_size > SZ && lseek(fd, SZ, 0) != -1) {
			lsize = f_stat.st_size - SZ;
			if ((ptr = malloc(lsize)) != 0) {
				if (read(fd, ptr, lsize) == lsize) {
					for (i=0; i<3; i++) {
						_wcptr[i] = (struct _wctype *)((int)ptr + (int)((sizeof(struct _wctype)) * i));
						if (_wcptr[i]->index != 0)
							_wcptr[i]->index = (unsigned char *)((int)ptr + (int)_wcptr[i]->index);
						if (_wcptr[i]->type != 0)
							_wcptr[i]->type = (unsigned *)((int)ptr + (int)_wcptr[i]->type);
						if (_wcptr[i]->code != 0)
							_wcptr[i]->code = (wchar_t *)((int)ptr + (int)_wcptr[i]->code);
					}
					ret = 0;
				} else {
					free(ptr);
					ptr = (char *)NULL;
				}
			}
		}
	}
	(void)close(fd);
	return ret;
}
