/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5fontlib:font/bufio.c	1.1"
/*
 * $XConsortium: bufio.c,v 1.3 92/02/11 17:17:39 eswu Exp $
 *
 * Copyright 1991 Massachusetts Institute of Technology
 *
 * Author:  Keith Packard, MIT X Consortium
 */


#include    <fontmisc.h>
#include    <bufio.h>
#include    <errno.h>
extern int errno;

BufFilePtr
BufFileCreate (private, io, skip, close)
    char    *private;
    int	    (*io)();
    int	    (*skip)();
    int	    (*close)();
{
    BufFilePtr	f;

    f = (BufFilePtr) xalloc (sizeof *f);
    if (!f)
	return 0;
    f->private = private;
    f->bufp = f->buffer;
    f->left = 0;
    f->io = io;
    f->skip = skip;
    f->close = close;
    return f;
}

#define FileDes(f)  ((int) (f)->private)

static int
BufFileRawFill (f)
    BufFilePtr	f;
{
    int	left;

    left = read (FileDes(f), f->buffer, BUFFILESIZE);
    if (left <= 0) {
	f->left = 0;
	return BUFFILEEOF;
    }
    f->left = left - 1;
    f->bufp = f->buffer + 1;
    return f->buffer[0];
}

static int
BufFileRawSkip (f, count)
    BufFilePtr	f;
    int		count;
{
    int	    curoff;
    int	    fileoff;
    int	    todo;

    curoff = f->bufp - f->buffer;
    fileoff = curoff + f->left;
    if (curoff + count <= fileoff) {
	f->bufp += count;
	f->left -= count;
    } else {
	todo = count - (fileoff - curoff);
	if (lseek (FileDes(f), todo, 1) == -1) {
	    if (errno != ESPIPE)
		return BUFFILEEOF;
	    while (todo) {
		curoff = BUFFILESIZE;
		if (curoff > todo)
		    curoff = todo;
		fileoff = read (FileDes(f), f->buffer, curoff);
		if (fileoff <= 0)
		    return BUFFILEEOF;
		todo -= fileoff;
	    }
	}
	f->left = 0;
    }
    return count;
}

static int
BufFileRawClose (f, doClose)
    BufFilePtr	f;
{
    if (doClose)
	close (FileDes (f));
    return 1;
}

BufFilePtr
BufFileOpenRead (fd)
    int	fd;
{
    return BufFileCreate ((char *) fd, BufFileRawFill, BufFileRawSkip, BufFileRawClose);
}

static
BufFileRawFlush (c, f)
    int		c;
    BufFilePtr	f;
{
    int	cnt;

    if (c != BUFFILEEOF)
	*f->bufp++ = c;
    cnt = f->bufp - f->buffer;
    f->bufp = f->buffer;
    f->left = BUFFILESIZE;
    if (write (FileDes(f), f->buffer, cnt) != cnt)
	return BUFFILEEOF;
    return c;
}

BufFilePtr
BufFileOpenWrite (fd)
    int	fd;
{
    BufFilePtr	f;

    f = BufFileCreate ((char *) fd, BufFileRawFlush, 0, BufFileFlush);
    f->bufp = f->buffer;
    f->left = BUFFILESIZE;
    return f;
}

BufFileRead (f, b, n)
    BufFilePtr	f;
    char	*b;
    int		n;
{
    int	    c, cnt;
    cnt = n;
    while (cnt--) {
	c = BufFileGet (f);
	if (c == BUFFILEEOF)
	    break;
	*b++ = c;
    }
    return n - cnt - 1;
}

BufFileWrite (f, b, n)
    BufFilePtr	f;
    char	*b;
    int		n;
{
    int	    c, cnt;
    cnt = n;
    while (cnt--) {
	if (BufFilePut (*b++, f) == BUFFILEEOF)
	    return BUFFILEEOF;
    }
    return n;
}

int
BufFileFlush (f)
    BufFilePtr	f;
{
    if (f->bufp != f->buffer)
	(*f->io) (BUFFILEEOF, f);
}

int
BufFileClose (f, doClose)
    BufFilePtr	f;
{
    (void) (*f->close) (f, doClose);
    xfree (f);
}

int
BufFileFree (f)
    BufFilePtr	f;
{
    xfree (f);
}
