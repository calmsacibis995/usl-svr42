/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)libDtI:mapfile.c	1.5"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "mapfile.h"

DmMapfilePtr
Dm__mapfile(char *filename, int prot, int flags)
{
    int			fd;
    char *		fp;
    struct stat		st;
    DmMapfilePtr	mp;
    int			oflag;

    if (!filename)
	return(NULL);

    oflag = (prot == PROT_READ) ?  O_RDONLY : O_RDWR;

    errno = 0;
    if ((fd = open(filename, oflag)) == -1)
    {
	return(NULL);
    }

    /* get file size */
    if (fstat(fd, &st)) {
    err1:
	if (close(fd))
	    fprintf(stderr, "close() error (%d)", errno);
	return(NULL);
    }
	
    if (st.st_size == 0) {
	/* Not really an error, just nothing to map. */
	errno = 0;
	goto err1;
    }

    if ((fp = mmap((caddr_t)0, st.st_size, prot, flags, fd, 0)) == (char *)-1)
    {
	fprintf(stderr, "mmap error (%d)", errno);
	(void)close(fd);
	return(NULL);
    }
    if (close(fd))
	fprintf(stderr, "close() error (%d)", errno);

    if ((mp = (DmMapfilePtr)malloc(sizeof(DmMapfileRec))) == NULL)
    {
	fprintf(stderr, "Out of memory");
	return(NULL);
    }

    /* initialize header */
    mp->filesize = st.st_size;
    mp->endptr = fp + st.st_size;
    mp->curptr = mp->mapptr  = fp;
    mp->line = 1;
    return(mp);
}

void
Dm__unmapfile(mp)
DmMapfilePtr mp;
{
	if (mp) {
		munmap(mp->mapptr, mp->filesize);
		free(mp);
	}
}

/*
 * Look for a specific character, starting from the current position.
 */
char *
Dm__findchar(mp, ch)
DmMapfilePtr mp;
int ch;
{
	while (MF_NOT_EOF(mp) && (MF_PEEKC(mp) != ch))
		MF_NEXTC(mp);
	return(MF_GETPTR(mp));
}

/*
 * Look for any of many possible characters, starting from the current position.
 */
char *
Dm__strchr(mp, str)
DmMapfilePtr mp;
char *str;
{
	while (MF_NOT_EOF(mp) && !strchr(str, MF_PEEKC(mp)))
		MF_NEXTC(mp);
	return(MF_GETPTR(mp));
}

/*
 * Look for a substring.
 */
char *
Dm__strstr(mp, str)
DmMapfilePtr mp;
char *str;
{
	char *p;
	char *q;

	while (MF_NOT_EOF(mp)) {
		p = str;
		q = MF_GETPTR(mp);
	 	while (*p && *q == *p) {
			p++;
			if (q != MF_EOFPTR(mp))
				q++;
			else
				break;
		}

		if (!*p)
			/* found it */
			return(MF_GETPTR(mp));

		MF_NEXTC(mp);
	}
	return(NULL);
}

