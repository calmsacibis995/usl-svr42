/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/cat.c	1.2.1.2"
#ident  "$Header: cat.c 1.1 91/07/03 $"
/*	@(#) cat.c 22.1 89/11/14 
 *
 *	Copyright (C) The Santa Cruz Operation, 1985.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, Microsoft Corporation
 *	and AT&T, and should be treated as Confidential.
 */

#include	<stdio.h>
#include	"dosutil.h"
#ifdef	INTL
#include	<ctype.h>						/*L002*/
#endif

/*	cat()  --  dump a DOS file into a Unix file.  If the file is not
 *		ASCII printable, there won't be <cr><lf> conversion, unless
 *		flag is set to MAP.
 *		clustno :  starting cluster of DOS file
 *		nbytes  :  length of the DOS file in bytes
 *		outfile :  file stream into which to write the file
 *		flag  	:  RAW if no <cr><lf> conversion necessary
 *		 	   MAP if <cr><lf> conversion always required
 *			   UNKNOWN if file should be checked for conversion
 *
 */

cat(clustno,nbytes,outfile,flag)
unsigned clustno;
long nbytes;
FILE *outfile;
int flag;
{
	unsigned clustsize, count;

	clustsize = frmp->f_sectclust * BPS;

	if (flag == UNKNOWN)	/* M001 */
		flag = (canprint(clustno,nbytes) ? MAP : RAW);

#ifdef DEBUG
	fprintf(stderr,"DEBUG cat() %ld bytes starting from cluster %u\t",
			nbytes,clustno);
	fprintf(stderr,"flag = %d\n",flag);
#endif

	while (goodclust(clustno)){
		if (!readclust(clustno,buffer)){
			sprintf(errbuf,"cluster %u unreadable",clustno);
			fatal(errbuf,1);
		}
		count   = min(clustsize,nbytes);

		if (flag == RAW)
			fwrite(buffer,1,count,outfile);
		else /* (flag == MAP) */
			fwrcvt(buffer,1,count,outfile);

		nbytes -= count;
		clustno = nextclust(clustno);
	}
	if (nbytes != 0)
		fatal("ERROR internal inconsistency in DOS disk",1);
}



/*	fwrcvt()  --  write out a DOS text file, stripping the CR character.
 *		inbuf    :  input buffer
 *		dummy    :  not used; for compatibility with fwrite()
 *		count    :  number of bytes in input buffer
 *		outfile  :  file stream into which to write the file
 */

static fwrcvt(inbuf,dummy,count,outfile)
char *inbuf;
int dummy;
unsigned count;
FILE *outfile;
{
	static int crseen;

	while (count-- > 0) {			/* M001 begin */
		switch(*inbuf) {
		case CR:
			if (crseen)
				putc(CR,outfile);
			else
				crseen = TRUE;
			break;
		case DOSEOF:
			if (crseen) {
				putc(CR,outfile);
				crseen = FALSE;
			}
			return;
		case '\n':
			crseen = FALSE;
			putc('\n',outfile);
			break;
		default:
			if (crseen) {
				putc(CR,outfile);
				crseen = FALSE;
			}
			putc(*inbuf,outfile);
		}				/* M001 end */
		inbuf++;
	}
}


/*	canprint()  --  returns TRUE if a DOS file is printable. Only
 *		the first cluster of the file is examined.  The last
 *		byte of the file is allowed to be DOSEOF.  Printable
 *		characters are:		0x07  -  0x0d
 *					0x20  -  0x7e
 *
 *		start :  starting cluster of the DOS file.
 *		nbytes: size of the file
 */

static canprint(start,nbytes)
unsigned start;
long nbytes;
{
	char *c;
	unsigned chkbytes;

	if (!readclust(start,buffer)){
		sprintf(errbuf,"cluster %u unreadable",start);
		fatal(errbuf,1);
	}
	chkbytes = min(frmp->f_sectclust * BPS, nbytes);

	for (c = buffer; c < buffer + chkbytes; c++){

		if (*c == DOSEOF)
			return( (c - buffer + 1) < nbytes ? FALSE : TRUE);

#ifdef INTL
		if (!isprint(*c) && !isspace(*c))		/*L002*/
#else
		if ((*c < 0x07) || (*c > 0x7e) || 
		    ((*c > 0x0d) && (*c < 0x20)))
#endif
			return(FALSE);
	}
	return(TRUE);
}
