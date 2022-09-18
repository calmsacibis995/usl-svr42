/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/error.h	1.1"
/*
 * @(#)error.h 1.2 89/03/10
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


extern VOID 	    	    f3_Error(	/* code */ );


#define	    CHECK(code,cond)	if (cond) f3_Error(code)

#if 	RIFFDEBUG
#define	   DCHECK(code,cond)	if (cond) f3_Error(code)
#else
#define	   DCHECK(code,cond)
#endif

#define	X_OFLO	    	1
#define	X_UFLO	    	2
#define	S_OFLO	    	3
#define	S_UFLO	    	4
#define	GADDX_LO   	5
#define	GADDX_HI  	6
#define	G_OFLO	    	7
#define	LADDX_LO   	8
#define	LADDX_HI  	9
#define	L_OFLO	    	10
#define	END_OF_FILE 	11
#define	INV_OPERAND 	12
#define	INV_FONT 	13
#define	NO_FONT	    	14
#define	NO_SYMBOL   	15
#define	NO_SPACE    	16

