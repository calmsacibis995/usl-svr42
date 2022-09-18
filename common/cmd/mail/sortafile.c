/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/sortafile.c	1.3.2.3"
#ident "@(#)sortafile.c	1.6 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	sortafile - sort an alias file into another file

    SYNOPSIS
	int sortafile(char *infile, char *outfile)

    DESCRIPTION
	sortafile() takes an alias file, infile, and generates
	a sorted version of it. It does this using an interative
	sort-merge method using 5 temp files. Duplicate aliases are
	removed. The sort is stable.

    RETURNS
	0 - an error occurred
	1 - all is well

    STRATEGY
	function read-sort-write(file)
		read 100 lines
		internal sort
		write file

	create temp[0]
	read-sort-write(temp[0])
	while (!eof)
		create temp[1] .. temp[4]
		for (i = 1; i < 3 && !eof; i++)
			read-sort-write(temp[i])
		for (i = 0; i < 3; i++)
			merge temp[i] into temp[4]
		close temp[0] .. temp[3]
		temp[0] = temp[4]
		close temp[4]
	copy temp[0] into outfile
	close temp[0]
*/

struct sortfile
{
    string *s;
    int fileorder;
};

static void close_thru ARGS((FILE *tmp[], int max));
static int cmpline ARGS((string *s1, string *s2));
static int merge ARGS((FILE *tmp[], int max));
static int read_sort_write ARGS((FILE *infp, FILE *outfp, struct sortfile strs[]));
static int qsfcmp ARGS((const void*, const void*));
static int sfcmp ARGS((struct sortfile *sf1, struct sortfile *sf2));
static void sfinssort ARGS((struct sortfile sortfiles[], int max));
static int write_sorted_file ARGS((FILE *infp, FILE *outfp));
static int writelong ARGS((unsigned long l, FILE *outfp));

#ifndef MAXSTRS
# define MAXSTRS 100
#endif
#ifndef MAXFILES
# define MAXFILES 4
#endif

#define RSW_OK	1
#define RSW_EOF 0
#define RSW_ERR	-1

int sortafile(infile, outfile)
char *infile;
char *outfile;
{
    struct sortfile strs[MAXSTRS];
    FILE *temp[MAXFILES+1];
    FILE *infp = fopen(infile, "r");
    FILE *outfp = 0;
    int ret = 0;
    register int i;

    if (!infp)
	return ret;

    temp[0] = tmpfile();
    if (!temp[0])
	return ret;

    for (i = 0; i < MAXSTRS; i++)
	strs[i].s = s_new();

    /* Prime the pumps */
    ret = read_sort_write(infp, temp[0], strs);
    if (ret == RSW_ERR)
	goto done;

    /* While there's more input, merge it in. */
    while (!feof(infp))
	{
	/* Do it in chunks of MAXFILES files. */
	for (i = 1; i < MAXFILES && !feof(infp); i++)
	    {
	    temp[i] = tmpfile();
	    if (!temp[i])
		{
		close_thru(temp, i-1);
		goto done;
		}
	    ret = read_sort_write(infp, temp[i], strs);
	    if (ret == RSW_ERR)
		{
		close_thru(temp, i);
		goto done;
		}
	    }

	/* Merge the sorted mini-files into temp[MAXFILES]. */
	temp[MAXFILES] = tmpfile();
	if (!temp[MAXFILES])
	    {
	    close_thru(temp, i-1);
	    goto done;
	    }

	ret = merge(temp, i);
	if (!ret)
	    {
	    close_thru(temp, i-1);
	    (void) fclose(temp[MAXFILES]);
	    goto done;
	    }

	close_thru(temp, i-1);

	/* temp[MAXFILES] is now sorted file #0 for merging. */
	temp[0] = temp[MAXFILES];
	}

    /* Copy the merged temp files into the output file */
    outfp = fopen(outfile, "w");
    if (!outfp)
	{
	(void) fclose(temp[0]);
	goto done;
	}

    rewind(temp[0]);
    ret = write_sorted_file(temp[0], outfp);
    if (!ret)
	{
	(void) unlink(outfile);
	goto done;
	}

    if (fflush(outfp) == EOF)
	{
	(void) unlink(outfile);
	goto done;
	}

    ret = 1;

done:
    if(infp)  fclose(infp);
    if(outfp) fclose(outfp);
    (void) fclose(temp[0]);
    for (i = 0; i < MAXSTRS; i++)
	s_free(strs[i].s);
    return ret;
}

/*
    NAME
	write_sorted_file - write out the sorted file

    SYNOPSIS
	static int write_sorted_file(FILE *infp, FILE *outfp);

    DESCRIPTION
	Write out the sorted file. Also write out a table
	giving the offsets where each letter starts.
*/
static int write_sorted_file(infp, outfp)
FILE *infp;
FILE *outfp;
{
    long offsets[257];
    long noffset;
    int c, oldc = -1, j;
    if (fseek(outfp, 257*sizeof(long), 0) == EOF)
	return 0;

    for (;;)
	{
	/* get 1st char of line */
	c = getc(infp);
	if (c == EOF)
	    break;

	/* Fill in the offset table */
	c = tolower(c);
	if (c != oldc)
	    {
	    /* find offset of line */
	    noffset = ftell(outfp);
	    for (j = oldc + 1; j <= c; j++)
		offsets[j] = noffset;
	    oldc = c;
	    }

	/* copy the line */
	for (;;)
	    {
	    if (putc(c, outfp) == EOF)
		return 0;
	    if (c == '\n')
		break;
	    c = getc(infp);
	    if (c == EOF)
		break;
	    }
	}

    noffset = ftell(outfp);
    for (j = oldc + 1; j <= 256; j++)
	offsets[j] = noffset;

    if (fflush(outfp) == EOF)
	return 0;
    if (fseek(outfp, 0, 0) == EOF)
	return 0;
    for (j = 0; j <= 256; j++)
	if (!writelong((unsigned long)offsets[j], outfp))
	    return 0;
    if (fflush(outfp) == EOF)
	return 0;
    return 1;
}

/*
    NAME
	close_thru - close temp files

    SYNOPSIS
	static void close_thru(FILE *tmp[], int max)

    DESCRIPTION
	Close the temp files in preparation of an error exit
*/
static void close_thru(tmp, max)
FILE *tmp[];
int max;
{
    register int i;
    for (i = 0; i <= max; i++)
	(void) fclose(tmp[i]);
}

/*
    NAME
	sfinssort - insertion sort of sortfiles

    SYNOPSIS
	static void sfinssort(struct sortfile sortfiles[], int max)

    DESCRIPTION
	Sort an array of struct sortfiles using
	an insertion sort. Insertion sorts are faster
	than qsort for small arrays (< ~16).
*/
static void sfinssort(sortfiles, max)
struct sortfile sortfiles[];
int max;
{
    register int i, j, k;
    for (i = 1; i < max; i++)
	{
	struct sortfile v;
	v = sortfiles[i];
	for (j = i, k = j-1;
	     (j > 0) && (sfcmp(&sortfiles[k], &v) > 0);
	     j--, k--)
	    sortfiles[j] = sortfiles[k];
	sortfiles[j] = v;
	}
}

/*
    NAME
	merge - merge tmp files

    SYNOPSIS
	static int merge(FILE *tmp[MAXFILES+1], int max)

    DESCRIPTION
	merge tmp[0] .. tmp[MAXFILES-1]
	into tmp[MAXFILES]

    RETURNS
	0 - error writing output file tmp[MAXFILES]
	1 - all okay
*/
static int merge(tmp, max)
FILE *tmp[];
int max;
{
    FILE *outfp = tmp[MAXFILES];
    struct sortfile sortfiles[MAXFILES];
    register int i, j, fill_line;
    string *tmpstr;

    /* prime the pump */
    for (i = 0; i < max; i++)
	{
	sortfiles[i].s = s_new();
	sortfiles[i].fileorder = i;
	rewind(tmp[i]);
	tmpstr = s_getline(tmp[i], sortfiles[i].s);
	if (!tmpstr)
	    s_free(sortfiles[i].s);
	sortfiles[i].s = tmpstr;
	}

    /* Repeatedly find the lowest line and write */
    /* it out, then fill it back in. */
    for (;;)
	{
	/* Sort the lines. */
	sfinssort(sortfiles, max);
	/* qsort((char*)sortfiles, max, sizeof(sortfiles[0]), sfcmp); */


	/* If there are no more lines, quit */
	if (!sortfiles[0].s)
	    break;

	/* Check for dups. */
	if (cmpline(sortfiles[0].s, sortfiles[1].s) == 0)
	    fill_line = 1;

	else
	    {
	    /* Write out the minimal line */
	    if (fprintf(outfp, "%s\n", s_to_c(sortfiles[0].s)) == EOF)
		return 0;
	    fill_line = 0;
	    }

	/* Fill in another line from the file whose */
	/* line was just written or skipped. */
	j = sortfiles[fill_line].fileorder;
	s_restart(sortfiles[fill_line].s);
	tmpstr = s_getline(tmp[j], sortfiles[fill_line].s);
	if (!tmpstr)
	    s_free(sortfiles[fill_line].s);
	sortfiles[fill_line].s = tmpstr;
	}

    if (fflush(outfp) == EOF)
	return 0;
    return 1;
}

/*
    NAME
	read_sort_write - read a MAXSTRS lines, sort and write them

    SYNOPSIS
	static int read_sort_write(FILE *infp, FILE *outfp, string **strs)

    DESCRIPTION
	Read in up to MAXSTRS lines, sort them, and write them back out.

    RETURNS
	RSW_OK  - all went well
	RSW_EOF - end of file found immediately
	RSW_ERR - some other occurred
*/
static int read_sort_write(infp, outfp, strs)
FILE *infp;
FILE *outfp;
struct sortfile strs[];
{
    register int i, j, k;

    /* Read in up-to MAXSTRS lines */
    for (i = 0; i < MAXSTRS && (strs[i].s = s_getline(infp, s_restart(strs[i].s))) != 0; i++)
	strs[i].fileorder = i;

    if (i == 0)
	return RSW_EOF;

    /* Sort them */
    qsort((char*)strs, (unsigned) i, sizeof(strs[0]), qsfcmp);

    /* Write out the lines. Omit duplicate aliases */
    if (fprintf(outfp, "%s\n", s_to_c(strs[0].s)) == EOF)
	return RSW_ERR;
    for (j = 0, k = 1; k < i; k++)
	{
	if (cmpline(strs[j].s, strs[k].s) != 0)
	    {
	    j = k;
	    if (fprintf(outfp, "%s\n", s_to_c(strs[k].s)) == EOF)
		return RSW_ERR;
	    }
	}

    if (fflush(outfp) == EOF)
	return RSW_ERR;

    return RSW_OK;
}

/*
    NAME
	sfcmp - compare two sortfile structures

    SYNOPSIS
	static int sfcmp(struct sortfile *sf1, struct sortfile *sf2)

    DESCRIPTION
	Sortfile structure comparison routine to be used by inssort().
	A null sortfile compares higher than anything else. After that,
	the comparison is on the strings.
*/
static int sfcmp(sf1, sf2)
struct sortfile *sf1;
struct sortfile *sf2;
{
    if (!sf1->s)
	return (!sf2->s) ? 0 : 1;

    else if (!sf2->s)
	return -1;

    else
	{
	int ret = cmpline(sf1->s, sf2->s);
	if (ret == 0)
	    ret = sf1->fileorder < sf2->fileorder ? -1 : 1;
	return ret;
	}
}

/*
    NAME
	qsfcmp - compare two struct sortfiles

    SYNOPSIS
	static int qsfcmp(const void *v1, const void *v2)

    DESCRIPTION
	Struct sortfile comparison routine to be used by qsort.
	It just casts the void* pointers into struct sortfile
	pointers and calls sfcmp().
*/
#ifdef __STDC__
static int qsfcmp(const void *v1, const void *v2)
#else
static int qsfcmp(v1, v2)
char *v1;
char *v2;
#endif
{
    struct sortfile *s1 = (struct sortfile*)v1;
    struct sortfile *s2 = (struct sortfile*)v2;
    return sfcmp(s1, s2);
}

/*
    NAME
	cmpline - compare 1st token on two lines

    SYNOPSIS
	static int cmpline(string *s1, string *s2)

    DESCRIPTION
	Take a look at the first token of each string
	and do case-independent comparison on those token.
	A token ends with white-space.
*/
static int cmpline(s1, s2)
string *s1;
string *s2;
{
    if (!s1)
	return -1;

    else if (!s2)
	return 1;

    else
	{
	register char *p1 = s_to_c(s1);
	register char *p2 = s_to_c(s2);
	register int rv;
	char c1, c2;

	for (;;p1++, p2++)
	    {
	    c1 = *p1;
	    c2 = *p2;
	    if (!c1 || isspace(c1))
		{
		if (!c2 || isspace(c2))
		    return 0;
		else
		    return -1;
		}

	    else if (!c2 || isspace(c2))
		return 1;

	    else
		{
		rv = tolower(c1) - tolower(c2);
		if (rv)
		    return rv;
		}
	    }
	}
}

/*
    NAME
	writelong - write a long to an output FILE* in a portable manner

    SYNOPSIS
	int writelong(long l, FILE *outfp)

    DESCRIPTION
	The indicated long is written out to the file in a semi portable manner
	so that alias.c can read the longs back in even across a heterogeneous
	filesystem.
*/
static int writelong(l, outfp)
unsigned long l;
FILE *outfp;
{
    register int i;
    unsigned char buf[sizeof(long)];
    for (i = 0; i < sizeof(long); i++)
	{
	buf[i] = l & 0xFF;
	l >>= 8;
	}

    if (fwrite(buf, sizeof(buf), 1, outfp) != 1)
	return 0;
    return 1;	
}

#ifdef TEST
main(argc, argv)
char **argv;
{
    if (argc != 3)
	(void) fprintf (stderr, "Usage: %s infile outfile\n", argv[0]);
    return sortafile(argv[1], argv[2]);
}

FILE *tmpfile()
{
    char *fn = tempnam("/usr/tmp/", "NS");
    if (fn)
	{
	FILE *ret = fopen(fn, "w+");
	free(fn);
	return ret;
	}
    else
	return 0;
}
#endif
