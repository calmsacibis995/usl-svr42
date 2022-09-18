/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:f3/io.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)io.c 1.6 89/05/25";
#endif
/*
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                   PROPRIETARY NOTICE (Combined) 
**   
**            This source code is unpublished proprietary 
**            information constituting, or derived under 
**            license from AT&T's UNIX(r) System V. 
**   
**                       Copyright Notice 
**   
**            Notice of copyright on this source code 
**            product does not indicate publication. 
**   
**    Copyright (C) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
**    Copyright (C) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T
**   
**                      All rights reserved. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
**   
**                    RESTRICTED RIGHTS LEGEND: 
**    Use, duplication, or disclosure by the Government is subject 
**    to restrictions as set forth in subparagraph (c)(1)(ii) of 
**    the Rights in Technical Data and Computer Software clause at 
**    DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
**    FAR Supplement. 
**   
**    +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
*/


#include        <stdio.h>
#include        <string.h>
#include        <math.h>
#include        "cdefs.h"
#include	"frmath.h"
#include    	"f3io.h"



int32	(*f3_FontReadBINTXX)( /* FILE * */ );
int32 	(*f3_FontReadWINTXX)( /* FILE * */ );
int32 	(*f3_FontReadLINTXX)( /* FILE * */ );
fract 	(*f3_FontReadSREALXX)( /* FILE * */ );
fract 	(*f3_FontReadFREALXX)( /* FILE * */ );

void 	(*f3_FontWriteBINTXX)( /* FILE *, int32 */ );
void 	(*f3_FontWriteWINTXX)( /* FILE *, int32 */ );
void 	(*f3_FontWriteLINTXX)( /* FILE *, int32 */ );
void 	(*f3_FontWriteSREALXX)( /* FILE *, fract */ );
void 	(*f3_FontWriteFREALXX)( /* FILE *, fract */ );

long	f3_EncryptState;

void	f3_Seed(s)
int32 s;
{
	f3_EncryptState = s;
}

#define	CRYPT(b) {							\
	b ^= ((f3_EncryptState >> 24) & 0xFF);			\
	f3_EncryptState = f3_EncryptState * 77631821 + 1;	\
}

#ifndef PERF
#define	getbyteNE(fp,b)	b = getc(fp);
#define	getbyteE(fp,b) {						\
	b = getc(fp);							\
/*    	DCHECK(END_OF_FILE,b==EOF);		*/			\
	CRYPT(b)							\
}
#else
extern	unsigned char	*f3_CurFontP;

#define	getbyteNE(fp,b)							\
	b = *f3_CurFontP++;
#define	getbyteE(fp,b) {						\
	b = *f3_CurFontP++;						\
/*    	DCHECK(END_OF_FILE,b==EOF);		*/			\
	CRYPT(b)							\
}
#endif /*PERF*/

#define	putbyteNE(fp,b)	putc(b,fp);
#define putbyteE(fp,b) {						\
	char	c;							\
	c = b;								\
	CRYPT(c)							\
	putc(c,fp);							\
}

/*  	--------------------	*/
int32 	f3_FontReadBINTNE(fp)
FILE	*fp;
/*  	--------------------	*/
{   	int16 	b;

	getbyteNE(fp,b)	/*;*/
   	return(b);
}

/*  	------------------------	*/
void 	f3_FontWriteBINTNE(fp,v)
FILE	*fp;
int32	 v;
/*  	------------------------	*/
{
	putbyteNE(fp,v)	/*;*/
}

/*  	--------------------	*/
int32 	f3_FontReadWINTNE(fp)
FILE	*fp;
/*  	--------------------	*/
{   	int16	v;
	int32	b1,b2;
	

	getbyteNE(fp,b1)	/*;*/
	getbyteNE(fp,b2)	/*;*/
	v = (b1<<8) | b2;
	return((int32)v);
}

/*  	-------------------------    */
void	f3_FontWriteWINTNE(fp,v)
FILE	*fp;
int32 	 v;
/*  	-------------------------    */
{   	putbyteNE(fp,v>>8)	/*;*/
   	putbyteNE(fp,v)		/*;*/
}


/*  	----------------------	*/
int32 	f3_FontReadLINTNE(fp)
FILE	*fp;
/*  	----------------------	*/
{   	int32	v;
	int32	b1,b2,b3,b4;
	

	getbyteNE(fp,b1)	/*;*/
	getbyteNE(fp,b2)	/*;*/
	getbyteNE(fp,b3)	/*;*/
	getbyteNE(fp,b4)	/*;*/
	v = (b1<<24) | (b2<<16) | (b3<<8) | b4;
	return(v);
}


/*  	-------------------------    */
void	f3_FontWriteLINTNE(fp,v)
FILE	*fp;
int32 	 v;
/*  	-------------------------    */
{   	putbyteNE(fp,v>>24)	/*;*/
   	putbyteNE(fp,v>>16)	/*;*/
   	putbyteNE(fp,v>>8)	/*;*/
   	putbyteNE(fp,v)		/*;*/
}

/*  	-----------------------	    */
fract	f3_FontReadSREALNE(fp)
FILE	*fp;
/*  	-----------------------	    */
{    	fract	f;

    	f = f3_FontReadWINTNE(fp);
	f <<= 6;
	
    	return(f);
}

/*	-------------------------	*/
void	f3_FontWriteSREALNE(fp,v)
FILE	*fp;
fract 	 v;
/*	-------------------------	*/
{
	f3_FontWriteWINTNE(fp,v>>6);
}

/*  	------------------------	*/
fract 	f3_FontReadFREALNE(fp)
FILE	*fp;
/*  	------------------------	*/
{	return((fract)f3_FontReadLINTNE(fp));
}


/*  	---------------------------	    */
void	f3_FontWriteFREALNE(fp,v)
FILE	*fp;
fract 	 v;
/*  	---------------------------	    */
{
	f3_FontWriteLINTNE(fp,(int32)v);
}

/********************************************/


/*  	--------------------	*/
int32 	f3_FontReadBINTE(fp)
register FILE	*fp;
/*  	--------------------	*/
{   	register int16 	b;

	getbyteE(fp,b)	/*;*/
   	return(b);
}

/*  	------------------------	*/
void 	f3_FontWriteBINTE(fp,v)
FILE	*fp;
int32	 v;
/*  	------------------------	*/
{
	putbyteE(fp,v)	/*;*/
}

/*  	--------------------	*/
int32 	f3_FontReadWINTE(fp)
register FILE	*fp;
/*  	--------------------	*/
{   	register int16	v;
	register int32	b1,b2;
	

	getbyteE(fp,b1)	/*;*/
	getbyteE(fp,b2)	/*;*/
	v = (b1<<8) | b2;
	return((int32)v);
}

/*  	-------------------------    */
void	f3_FontWriteWINTE(fp,v)
FILE	*fp;
int32 	 v;
/*  	-------------------------    */
{   	putbyteE(fp,v>>8)	/*;*/
   	putbyteE(fp,v)		/*;*/
}



/*  	----------------------	*/
int32 	f3_FontReadLINTE(fp)
FILE	*fp;
/*  	----------------------	*/
{   	int32	v;
	int32	b1,b2,b3,b4;
	

	getbyteE(fp,b1)	/*;*/
	getbyteE(fp,b2)	/*;*/
	getbyteE(fp,b3)	/*;*/
	getbyteE(fp,b4)	/*;*/
	v = (b1<<24) | (b2<<16) | (b3<<8) | b4;
	return(v);
}


/*  	-------------------------    */
void	f3_FontWriteLINTE(fp,v)
FILE	*fp;
int32 	 v;
/*  	-------------------------    */
{   	putbyteE(fp,v>>24)	/*;*/
   	putbyteE(fp,v>>16)	/*;*/
   	putbyteE(fp,v>>8)	/*;*/
   	putbyteE(fp,v)		/*;*/
}

/*  	-----------------------	    */
fract	f3_FontReadSREALE(fp)
FILE	*fp;
/*  	-----------------------	    */
{    	fract	f;

    	f = f3_FontReadWINTE(fp);
	f <<= 6;
	
    	return(f);
}

/*	-------------------------	*/
void	f3_FontWriteSREALE(fp,v)
FILE	*fp;
fract 	 v;
/*	-------------------------	*/
{
	f3_FontWriteWINTE(fp,v>>6);
}

/*  	------------------------	*/
fract 	f3_FontReadFREALE(fp)
FILE	*fp;
/*  	------------------------	*/
{	return((fract)f3_FontReadLINTE(fp));
}


/*  	---------------------------	    */
void	f3_FontWriteFREALE(fp,v)
FILE	*fp;
fract 	 v;
/*  	---------------------------	    */
{
	f3_FontWriteLINTE(fp,(int32)v);
}


/*	-----------------------		*/
void	f3_SetFontEncryption(t)
bool	t;
/*	-----------------------		*/
{	if (t) {
		f3_FontReadBINTXX =	f3_FontReadBINTE;
	 	f3_FontReadWINTXX =	f3_FontReadWINTE;
	 	f3_FontReadLINTXX =	f3_FontReadLINTE;
	 	f3_FontReadSREALXX =	f3_FontReadSREALE;
	 	f3_FontReadFREALXX =	f3_FontReadFREALE;

	 	f3_FontWriteBINTXX =	f3_FontWriteBINTE;
	 	f3_FontWriteWINTXX =	f3_FontWriteWINTE;
	 	f3_FontWriteLINTXX =	f3_FontWriteLINTE;
	 	f3_FontWriteFREALXX =	f3_FontWriteFREALE;
	} else {
		f3_FontReadBINTXX =	f3_FontReadBINTNE;
	 	f3_FontReadWINTXX =	f3_FontReadWINTNE;
	 	f3_FontReadLINTXX =	f3_FontReadLINTNE;
	 	f3_FontReadSREALXX =	f3_FontReadSREALNE;
	 	f3_FontReadFREALXX =	f3_FontReadFREALNE;

	 	f3_FontWriteBINTXX =	f3_FontWriteBINTNE;
	 	f3_FontWriteWINTXX =	f3_FontWriteWINTNE;
	 	f3_FontWriteLINTXX =	f3_FontWriteLINTNE;
	 	f3_FontWriteFREALXX =	f3_FontWriteFREALNE;
	}
}

