/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/cdefs.h	1.1"
/*
 * @(#)cdefs.h 1.3 89/05/24
 *
 */
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

/*
 *
 */

/*  	The following definition (edu,5/6/88) is intended as a replacement
    	for 'void' which doesn't seem to work in situations such as:

    	void	(*f)();
    	void	g() { ... }

    	... if (f==g) ...   incompatible types
*/
#define	VOID	int

#define	int32	long
#define	int16	short
#define	int8	char
#define	bool	char
#define	uint32	unsigned long
#define	uint16	unsigned short
#define	uint8	unsigned char
#define byte	unsigned char

#define	Rint32	register long
#define	Rint16	register short
#define	Rint8	register char
#define	Rbool	register char
#define	Ruint32	register unsigned long
#define	Ruint16	register unsigned short
#define	Ruint8	register unsigned char
#define Rbyte	register unsigned char

#define	TRUE	1
#define	true	1
#define	FALSE	0
#define	false	0
#define ON	1
#define OFF	0

#ifndef EOF
#define	EOF	-1
#endif	/*EOF*/

#ifndef NIL
#define	NIL	-1
#endif	/*NIL*/

#ifndef	NULL
#define	NULL	0
#endif	/*NULL*/

#ifndef M_PI
#define	M_PI	3.14159265358979323846
#endif

#define	DEGTORAD(d) 	(((double)(d)/180)*M_PI)
#define	RADTODEG(r) 	(((double)(r)*180)/M_PI)

#ifndef abs
#define abs(a)		(((a) >= 0) ? (a) : -(a))
#endif
#ifndef max
#define max(a, b)       (((a) >= (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)       (((a) >= (b)) ? (b) : (a))
#endif

#ifndef sq
#define	sq(n)		((n) * (n))
#endif

/*	The following round function should be used instead of the
 *	<rint> provided in the math library, which only works if your program
 *	is compiled with the -f68881 option.  And furthermore, there are
 *	inconsistencies in the result.		may 5/11/88
 */
#define round(v)        (((v) > 0) ? floor((v)+0.5) : ceil((v)-0.5))


