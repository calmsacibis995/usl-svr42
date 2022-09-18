/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86:util/subr.c	1.6"
#ident	"$Header: $"

#include <fs/vfs.h>
#include <io/conf.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define NDEV		256
#define max(a, b)	((a) > (b) ? (a) : (b))

/*
 * Routine which sets a user error; placed in
 * illegal entries in the block and character device
 * switch tables.
 */
int
nodev()
{
	return (u.u_error = ENODEV);
}

/*
 * Routine which sets a user error; placed in
 * illegal entries in the block and character device
 * switch tables.
 */
int
nxio()
{
	return (u.u_error = ENXIO);
}

/*
 * Null routine; placed in entries in the block and character
 * switch tables where no processing is required.
 */
int
nulldev()
{
	return 0;
}

/*
 * Generate an unused major device number.
 */
int
getudev()
{
	static int next = 0;

	if (next == 0)
		next = max(bdevcnt, cdevcnt);
	return(next < NDEV ? next++ : -1);
}

/*
 * The following function returns the location of the array of pointers to 
 * the file table entries for open files.  It was created to maintain driver
 * compatibility when the alignment of the ublock changes in the future.
 */
struct file **
get_ofile()
{
	return (u.u_flist.uf_ofile);
}

/*
 * C-library string functions.  Assembler versions of others
 * are in string.s.
 */

/*
 * Copy s2 to s1, truncating or null-padding to always copy n bytes.
 * Return s1.
 */
char *
strncpy(s1, s2, n)
	register char *s1, *s2;
	register size_t n;
{
	register char *os1 = s1;

	n++;
	while (--n != 0 && (*s1++ = *s2++) != '\0')
		;
	if (n != 0)
		while (--n != 0)
			*s1++ = '\0';
	return os1;
}

/*
 * Compare strings (at most n bytes): return *s1-*s2 for the last
 * characters in s1 and s2 which were compared.
 */
int
strncmp(s1, s2, n)
	register char *s1, *s2;
	register size_t n;
{
	if (s1 == s2)
		return 0;
	n++;
	while (--n != 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return 0;
	return (n == 0) ? 0 : *s1 - *--s2;
}

/* takes a numeric char, yields an int */
#define	CTOI(c)		((c) & 0xf)

/* takes an int, yields an int */
#define TEN_TIMES(n)	(((n) << 3) + ((n) << 1))

/*
 * Returns the integer value of the string of decimal numeric
 * chars beginning at **str.
 * Does no overflow checking.
 * Note: updates *str to point at the last character examined.
 */
int
stoi(str)
	register char	**str;
{
	register char	*p = *str;
	register int	n;
	register int	c;

	for (n = 0; (c = *p) >= '0' && c <= '9'; p++) {
		n = TEN_TIMES(n) + CTOI(c);
	}
	*str = p;
	return n;
}

/*
 * Simple-minded conversion of a long into a null-terminated character
 * string.  Caller must ensure there's enough space to hold the result.
 */
void
numtos(num, s)
	u_long num;
	char *s;
{
	register int i = 0;
	register u_long mm = 1000000000;
	int t;

	if (num < 10) {
		*s++ = num + '0';
		*s = '\0';
	} else while (mm) {
		t = num / mm;
		if (i || t) {
			i++;
			*s++ = t + '0';
			num -= t * mm;
		}
		mm = mm / 10;
	}
	*s = '\0';
}
 
void
fshadbad(dev, bno)
dev_t dev;
daddr_t bno;	/* for the future */
{
	register struct vfs *vfsp;

	if ((vfsp = vfs_devsearch(dev)) != NULL)
		vfsp->vfs_flag |= VFS_BADBLOCK;
}

/*
 * Overlapping bcopy (source and target may overlap arbitrarily).
 */
void
ovbcopy(from, to, count)
	register char *from;
	register char *to;
	register uint count;
{
	register int diff;

	if ((diff = from - to) < 0)
		diff = -diff;
	if (from < to && count > diff) {
		do {
			count--;
			*(to + count) = *(from + count);
		} while (count);
	} else if (from != to) {
		while (count--)
			*to++ = *from++;
	}
}
