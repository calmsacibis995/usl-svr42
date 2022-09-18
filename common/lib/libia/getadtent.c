/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libia:common/lib/libia/getadtent.c	1.1.3.2"
#ident  "$Header: getadtent.c 1.2 91/06/21 $"

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <ia.h>
#include <string.h>
#include <errno.h>
#include <audit.h>

static FILE *amf = NULL ;	/* auditmask file (amf) */
static char line[BUFSIZ+1] ;
static struct adtuser adtuser;

void
setadtent()
{
	if(amf == NULL) {
		amf = fopen(AUDITMASK, "r") ;
		/* ERROR CHECKING? */
	}
	else
		rewind(amf) ; /* called by getadtnam() */
}

void
endadtent()
{
	if(amf != NULL) {
		(void) fclose(amf) ;
		amf = NULL ;
	}
}

static char *
adtskip(p)
register char *p ;
{
	while(*p && *p != ':' && *p != '\n')
		++p ;
	if(*p == '\n')
		*p = '\0' ;
	else if(*p)
		*p++ = '\0' ;
	return(p) ;
}

/* 	The getadtent function will return a NULL for an end of 
	file indication or a bad entry
*/
int
getadtent(adtptr)
struct adtuser *adtptr;
{
	extern int fgetadtent() ;

	if(amf == NULL) {
		if((amf = fopen(AUDITMASK, "r")) == NULL)
			return (-1) ;
	}
	return (fgetadtent(amf,adtptr)) ;
}

int
fgetadtent(f,adtptr)
FILE *f;
struct	adtuser	*adtptr;
{
	if(fread(adtptr, sizeof(struct adtuser), 1, f) != 1) {
		if(ferror(f))
			return(ferror(f));
		if(feof(f))
			return(-1);
	}
	return(0) ;
}

int
getadtnam(name,adtptr)
char	*name ;
struct adtuser *adtptr;
{

	int p;
	setadtent() ;
	while ( (p = getadtent(adtptr)) != -1 && strcmp(name, adtptr->ia_name) )
		;
	endadtent() ;
	return (p) ;
}

int
putadtent(p, fp)
register struct adtuser *p;
register FILE *fp;
{
	fwrite(p, sizeof(struct adtuser), 1, fp);
	fflush(fp);
	return(ferror(fp));
}

/*
** Convert the hexadecimal representation of the auditmask supplied
** in 'in' to a binary form, and place the result in 'out'.
*/
void
cnvxmask(out,in)
unsigned long	*out;
char		*in;
{
	int i;

	for (i=0; i<ADT_EMASKSIZE; ++i) {
		sscanf(in,"%8x",out);
		++out;
		in += 8;
	}
}

/* Strict atoi conversion returns a negative number if non-digits in string */
int
satoi(s)
char *s;
{
	register int n;
	for (n=0; *s; s++) {
		if (*s < '0' || *s > '9')
			return(-1);
		n = 10*n + (*s - '0');
	}
	return(n);
}

