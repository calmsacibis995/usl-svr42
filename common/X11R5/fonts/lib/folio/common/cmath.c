/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:common/cmath.c	1.1"

#ifndef lint
static char sccsid[] = "@(#)cmath.c 1.2 89/03/10";
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


#include "cdefs.h"
#include "common.h"

/*
    These routines are collected here for portability reasons
    and should be examined very carefully when moving the
    code to a different machine. The math functions of the
    raw machine should NOT be used, only the routines below.
*/

/* 
    rounding of a fixed point number <n> with <f> bits of fraction 
    returns result.
*/

int32	math_RoundFixedPoint(n, f)
int32	n, f;
{	    	    	    	    	    	    	    
    if ( (f) == 0 )
    	return( n );
    else if ( (f) == 1 ) {	    	    	    	    
    	if ( (n) > 0 ) 
    	    return( (((n)+1)&(~1)) );
    	else  
    	    return( ((n)&(~1)) );
    } else     	    	    	    	  
    	    return( (((n)+(1<<((f)-1)))&(~((1<<(f))-1))) );
}

int32	math_RoundFixedPointShort(n, f) 
int32	n, f;
{
    if ( (f) == 0 )
    	return( n );
    else if ( (f) == 1 ) {
    	if ( (n) > 0 )
    	     return( ((n)&(~1)) );
    	else  return( (((n)+1)&(~1)) );	  
    } else     	    	    	  
    	return( (((n)+((1<<((f)-1))-1))&(~((1<<(f))-1))) );
}

int32	math_FloorFixedPoint(n, f)
int32	n, f;
{ 	    	    	    	   
    if ( (f) == 0 )
    	return( n );
    else 
    	return( ((n)&(~((1<<(f))-1))) );
}  

int32 math_CeilingFixedPoint(n, f)  
int32	n, f;
{ 	    	    	    	    	
    if ( (f) == 0 )
    	return( n );
    else 
    	return( (((n)+((1<<(f))-1))&(~((1<<(f))-1))) );
}  

