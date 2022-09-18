/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bkrs:common/cmd/bkrs/bklib.d/bkgetchars.c	1.2.5.2"
#ident  "$Header: bkgetchars.c 1.2 91/06/21 $"
#define NULL	0
#define TRUE	1
#define FALSE	0

extern int strfind();
extern unsigned int strlen();

/* Function takes a pointer to a list of device characteristics */
/* and the name of a device attribute (like dtype or dcap) and returns */
/* the value of the attribute in the caller-provided buffer. */
/* Function returns TRUE if successful, FALSE otherwise. */
int
bkgetchars( charstr, attr, valbuf )
unsigned char *charstr;
unsigned char *attr;
unsigned char valbuf[];
{
	int loc;
	unsigned char *valptr;

	loc = strfind( (char *)charstr, (char *)attr );
	if ( loc < 0 )
		return( FALSE );

	/* Skip over "attr=" string */
	valptr = charstr + loc + strlen( (char *) attr ) + 1;
	while ( (*valptr != ',') && (*valptr != (unsigned char)NULL))
		*valbuf++ = *valptr++;
	*valbuf = (unsigned char)NULL;
	return( TRUE );
}
