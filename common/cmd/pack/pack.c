/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)pack:pack.c	1.16.1.11"
#ident "$Header: pack.c 1.2 91/08/13 $"
/*
 *	Huffman encoding program 
 *	Usage:	pack [[ - ] filename ... ] filename ...
 *		- option: enable/disable listing of statistics
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <locale.h>
#include <pfmt.h>
#include <errno.h>
#include <string.h>

#define	END	256
#define PACKED 017436 /* <US><RS> - Unlikely value */
#define	SUF0	'.'
#define	SUF1	'z'
#define ZSUFFIX	0x40000

static const char badread[] = ":28:Read error: %s";

struct stat status, ostatus;
struct utimbuf {
	time_t actime;	/* access time */
	time_t modtime;	/* modification time */
} utimes;

/* union for overlaying a long int with a set of four characters */
union FOUR {
	struct { long int lng; } longint;
	struct { char c0, c1, c2, c3; } chars;
};

/* character counters */
long	count [END+1];
union	FOUR insize;
long	outsize;
long	dictsize;
int	diffbytes;

/* i/o stuff */
char	vflag = 0;
int	force = 0;	/* allow forced packing for consistency in directory */
char	*filename = (char *)NULL;
int	infile;		/* unpacked file */
int	outfile;	/* packed file */
char	inbuff [BUFSIZ];
union	{ long dummy; char _outbuff[BUFSIZ + 4]; } dummy; /* Force alignment */
char	*outbuff = dummy._outbuff;

/* variables associated with the tree */
int	maxlev;
int	levcount [25];
int	lastnode;
int	parent [2*END+1];

/* variables associated with the encoding process */
char	length [END+1];
long	bits [END+1];
union	FOUR mask;
long	inc;
char	*maskshuff[4];

/* the heap */
int	n;
struct	heap {
	long int count;
	int node;
} heap [END+2];
#define hmove(a,b) {(b).count = (a).count; (b).node = (a).node;}

long	errorm;		/* Error marker for eprintf() */
char	*argvk;		/* Current file under scrutiny */

/* Initialize maskshuff[]; order of maskshuff is    */
/* machine-dependent (big endian or little endian). */
void
setmask()
{
        unsigned int i = 1;
        unsigned char *p = (unsigned char *)&i;

        switch(*p) {
        case 0:
		/* big endian */
                maskshuff[0] = &(mask.chars.c0); 
                maskshuff[1] = &(mask.chars.c1); 
                maskshuff[2] = &(mask.chars.c2); 
                maskshuff[3] = &(mask.chars.c3); 
                break;
        case 1:
		/* little endian */
                maskshuff[0] = &(mask.chars.c3); 
                maskshuff[1] = &(mask.chars.c2); 
                maskshuff[2] = &(mask.chars.c1); 
                maskshuff[3] = &(mask.chars.c0); 
                break;
        }
}

/* gather character frequency statistics */
/* return 1 if successful, 0 otherwise */
input ()
{
	register int i;
	for (i=0; i<END; i++)
		count[i] = 0;
	while ((i = read(infile, inbuff, BUFSIZ)) > 0)
		while (i > 0)
			count[inbuff[--i]&0377] += 2;
	if (i == 0)
		return (1);
	eprintf (MM_ERROR, badread, strerror (errno));
	return (0);
}

/* encode the current file */
/* return 1 if successful, 0 otherwise */
output ()
{
	int c, i, inleft;
	char *inp;
	register char **q, *outp;
	register int bitsleft;
	long temp;

	/* output ``PACKED'' header */
	outbuff[0] = 037; 	/* ascii US */
	outbuff[1] = 036; 	/* ascii RS */
	/* output the length and the dictionary */
	temp = insize.longint.lng;
	for (i=5; i>=2; i--) {
		outbuff[i] =  (char) (temp & 0377);
		temp >>= 8;
	}
	outp = &outbuff[6];
	*outp++ = maxlev;
	for (i=1; i<maxlev; i++)
		*outp++ = levcount[i];
	*outp++ = levcount[maxlev]-2;
	for (i=1; i<=maxlev; i++)
		for (c=0; c<END; c++)
			if (length[c] == i)
				*outp++ = c;
	dictsize = outp-&outbuff[0];

	/* output the text */
	lseek(infile, 0L, 0);
	outsize = 0;
	bitsleft = 8;
	inleft = 0;
	do {
		if (inleft <= 0) {
			inleft = read(infile, inp = &inbuff[0], BUFSIZ);
			if (inleft < 0) {
				eprintf (MM_ERROR, badread, strerror (errno));
				return (0);
			}
		}
		c = (--inleft < 0) ? END : (*inp++ & 0377);
		mask.longint.lng = bits[c]<<bitsleft;
		q = &maskshuff[0];
		if (bitsleft == 8)
			*outp = **q++;
		else
			*outp |= **q++;
		bitsleft -= length[c];
		while (bitsleft < 0) {
			*++outp = **q++;
			bitsleft += 8;
		}
		if (outp >= &outbuff[BUFSIZ]) {
			if (write(outfile, outbuff, BUFSIZ) != BUFSIZ) {
wrerr:
				eprintf (ZSUFFIX|MM_ERROR, ":29:Write error: %s",
					strerror (errno));
				return (0);
			}
			((union FOUR *) outbuff)->longint.lng = ((union FOUR *) &outbuff[BUFSIZ])->longint.lng;
			outp -= BUFSIZ;
			outsize += BUFSIZ;
		}
	} while (c != END);
	if (bitsleft < 8)
		outp++;
	c = outp-outbuff;
	if (write(outfile, outbuff, c) != c)
		goto wrerr;
	outsize += c;
	return (1);
}

/* makes a heap out of heap[i],...,heap[n] */
heapify (i)
{
	register int k;
	int lastparent;
	struct heap heapsubi;
	hmove (heap[i], heapsubi);
	lastparent = n/2;
	while (i <= lastparent) {
		k = 2*i;
		if (heap[k].count > heap[k+1].count && k < n)
			k++;
		if (heapsubi.count < heap[k].count)
			break;
		hmove (heap[k], heap[i]);
		i = k;
	}
	hmove (heapsubi, heap[i]);
}

/* return 1 after successful packing, 0 otherwise */
int packfile ()
{
	register int c, i, p;
	long bitsout;

	/* gather frequency statistics */
	if (input() == 0)
		return (0);

	/* put occurring chars in heap with their counts */
	diffbytes = -1;
	count[END] = 1;
	insize.longint.lng = n = 0;
	for (i=END; i>=0; i--) {
		parent[i] = 0;
		if (count[i] > 0) {
			diffbytes++;
			insize.longint.lng += count[i];
			heap[++n].count = count[i];
			heap[n].node = i;
		}
	}
	if (diffbytes == 1) {
		eprintf (MM_INFO, ":30:Trivial file");
		return (0);
	}
	insize.longint.lng >>= 1;
	for (i=n/2; i>=1; i--)
		heapify(i);

	/* build Huffman tree */
	lastnode = END;
	while (n > 1) {
		parent[heap[1].node] = ++lastnode;
		inc = heap[1].count;
		hmove (heap[n], heap[1]);
		n--;
		heapify(1);
		parent[heap[1].node] = lastnode;
		heap[1].node = lastnode;
		heap[1].count += inc;
		heapify(1);
	}
	parent[lastnode] = 0;

	/* assign lengths to encoding for each character */
	bitsout = maxlev = 0;
	for (i=1; i<=24; i++)
		levcount[i] = 0;
	for (i=0; i<=END; i++) {
		c = 0;
		for (p=parent[i]; p!=0; p=parent[p])
			c++;
		levcount[c]++;
		length[i] = c;
		if (c > maxlev)
			maxlev = c;
		bitsout += c*(count[i]>>1);
	}
	if (maxlev > 24) {
		/* can't occur unless insize.longint.lng >= 2**24 */
		eprintf (MM_ERROR, ":31:Huffman tree has too many levels");
		return(0);
	}

	/* don't bother if no compression results */
	outsize = ((bitsout+7)>>3)+6+maxlev+diffbytes;
	if (((float)insize.longint.lng+BUFSIZ-1)/BUFSIZ <= ((float)outsize+BUFSIZ-1)/BUFSIZ
	    && !force) {
		eprintf (MM_INFO, ":32:No saving");
		return(0);
	}

	/* compute bit patterns for each character */
	inc = 1L << 24;
	inc >>= maxlev;
	mask.longint.lng = 0;
	for (i=maxlev; i>0; i--) {
		for (c=0; c<=END; c++)
			if (length[c] == i) {
				bits[c] = mask.longint.lng;
				mask.longint.lng += inc;
			}
		mask.longint.lng &= ~inc;
		inc <<= 1;
	}
	return (output());
}

main(argc, argv)
int argc; char *argv[];
{
	register int i;
	register char *cp;
	int k, sep;
	int fcount =0; /* count failures */
	long name_max; /* limit on file name length */
	long path_max; /* limit on path length */
	long name_len;  /* length of name argument */

	/* Initialize maskshuff[]. */
	(void)setmask();

	(void)setlocale (LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:pack");

	errorm = -1;
	for (k=1; k<argc; k++) {
		/* Free filename from previous malloc, if needed. */
		if (filename != (char *)NULL) {
			free(filename);
			filename = (char *)NULL;
		}

		if (argv[k][0] == '-' && argv[k][1] == '\0') {
			vflag = 1 - vflag;
			continue;
		}
		if (argv[k][0] == '-' && argv[k][1] == 'f') {
			force++;
			continue;
		}
		fcount++; /* increase failure count - expect the worst */
		argvk = argv[k];
		if (errorm != -1) {
			(void) fputc ('\n', stderr);
			errorm = -1;
		}

		/* Get path length limit. */
		if ((path_max = pathconf(argvk, _PC_PATH_MAX)) == -1) {
			pfmt (stderr, MM_NOGET, "%s: %s\n", argvk, strerror(errno));
			continue;
		}

		/* Get length of name argument. */
		name_len = strlen(argvk);

		/* Check if path name will be too long with ".z" appended. */
		if ((name_len + 2) > path_max) {
			eprintf (MM_ERROR, ":66:Path name too long");
			continue;
		}

		/*
		 * Allocate space for filename. (Add 3 to name_len:
		 * 1 for terminating NULL, 2 for ".z".)
		 */
		filename = (char *)malloc(name_len + 3);
		if (filename == (char *)NULL) {
			pfmt(stderr, MM_ERROR|MM_NOGET, "%s\n", strerror(errno));
			exit(1);
		}

		cp = filename;
		sep = -1;  

		for (i=0; (*cp = argv[k][i]); i++)
			if (*cp++ == '/') sep = i;

		if (cp[-1]==SUF1 && cp[-2]==SUF0) {
			eprintf (MM_ERROR, ":33:Already packed");
			continue;
		}

		/* Get file name length limit. */
		if ((name_max = pathconf(argvk, _PC_NAME_MAX)) == -1) {
			pfmt (stderr, MM_NOGET, "%s: %s\n", argvk, strerror(errno));
			continue;
		}

		if ((i - sep) >= name_max) {
			eprintf (MM_ERROR, "uxsyserr:81:File name too long");
			continue;
		}

		if ((infile = open (filename, 0)) < 0) {
			eprintf (MM_ERROR, ":64:Cannot open: %s", strerror (errno));
			continue;
		}

		fstat(infile,&status);
		if (status.st_mode&S_IFDIR) {
			eprintf (MM_ERROR, ":35:Cannot pack a directory");
			(void) close (infile);
			continue;
		}
		if (status.st_size == 0) {
			eprintf (MM_ERROR, ":36:Cannot pack a zero length file");
			(void) close (infile);
			continue;
		}
		if( status.st_nlink != 1 ) {
			eprintf (MM_ERROR, ":37:File has links");
			(void) close (infile);
			continue;
		}
		*cp++ = SUF0;  *cp++ = SUF1;  *cp = '\0';
		if( stat(filename, &ostatus) != -1) {
			eprintf (ZSUFFIX|MM_ERROR, ":38:Already exists");
			(void) close (infile);
			continue;
		}
		if ((outfile = creat (filename, status.st_mode)) < 0) {
			eprintf (ZSUFFIX|MM_ERROR, ":39:Cannot create: %s",
				strerror (errno));
			(void) close (infile);
			continue;
		}
		if (packfile()) {
			if (unlink(argv[k]) != 0)
				eprintf (MM_WARNING, ":40:Cannot unlink: %s",
					strerror (errno));
			fcount--;  /* success after all */
			pfmt (stdout, MM_INFO, ":65:%s: %.1f%% Compression\n",
				argvk, ((double)(-outsize+(insize.longint.lng))/(double)insize.longint.lng)*100);
			errorm = -1;	/* Done the "\n" */
			/* output statistics */
			if (vflag) {
				pfmt(stdout, MM_INFO, 
					":42:\tFrom %ld to %ld bytes\n", insize.
					longint.lng, outsize);
				pfmt(stdout, MM_NOSTD, 
					":43:\tHuffman tree has %d levels below root\n",
					maxlev);
				pfmt(stdout, MM_NOSTD, 
					":44:\t%d distinct bytes in input\n", 
					diffbytes);
				pfmt(stdout, MM_NOSTD, 
					":45:\tDictionary overhead = %ld bytes\n",
					dictsize);
				pfmt(stdout, MM_NOSTD, 
					":46:\tEffective  entropy  = %.2f bits/byte\n",
					((double) outsize / (double) insize.
					longint.lng) * 8 );
				pfmt(stdout, MM_NOSTD, 
					":47:\tAsymptotic entropy  = %.2f bits/byte\n",
					((double) (outsize-dictsize) / (double)
					insize.longint.lng) * 8 );
			}
			utimes.actime = status.st_atime;
			utimes.modtime = status.st_mtime;
			if(utime(filename, &utimes)!=0) {
				eprintf(ZSUFFIX|MM_WARNING, ":49:Cannot change times: %s",
					strerror (errno));
			}
			if (chmod (filename, status.st_mode) != 0) {
				eprintf(ZSUFFIX|MM_WARNING, ":50:Cannot change mode to %o: %s",
					status.st_mode, strerror (errno));
			}
			chown (filename, status.st_uid, status.st_gid);
		}
		else
		{       eprintf (MM_INFO, ":48:File unchanged");
			unlink(filename);
		}
		(void) close (outfile);
		(void) close (infile);
	}
	if (errorm != -1) {
		(void) fputc ('\n', stderr);
		errorm = -1;
	}
	return (fcount);
}

eprintf (flag, s, a1, a2)
	int  flag;
	char *s, *a1, *a2;
{
	int loc_flag = flag & ~ZSUFFIX;
	if (errorm == -1 || errorm != flag) {
		if (errorm != -1)
			fprintf(stderr, "\n");
		errorm = flag;
		pfmt(stderr, (loc_flag | MM_NOGET),
			flag & ZSUFFIX ? "%s.z" : "%s", argvk);
	}
	pfmt(stderr, MM_NOSTD, "uxsyserr:2:: ");
	pfmt(stderr, MM_NOSTD, s, a1, a2);
}
