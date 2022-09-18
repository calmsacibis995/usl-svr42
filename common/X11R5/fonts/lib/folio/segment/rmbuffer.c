/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:segment/rmbuffer.c	1.1"
#ifndef lint
static char sccsid[] = "@(#)rmbuffer.c 1.2 89/03/10";
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


#include    "cdefs.h"
#include    "common.h"
#include    "rmbuffer.h"


/*
    Allocates one buffer including a header for storing rook move
    sequences in one of the four possible directions. 
    Returns pointer to allocated buffer header,
    or returns NULL if required size is too large for remaining buffer area
 */
rm_bufferHandle rm_AllocateBuffer( size )
int32   size;
{
    byte   *newbufferp, *nextbufferp;
    rm_bufferHandle bhdl;

    newbufferp = rm_bufferFreep;
    nextbufferp = newbufferp + rm_BUFFERHEADERSIZE + size;
    if ( nextbufferp > rm_BUFFERAREAEND )
    	return( NULL );
    else {
    	rm_bufferFreep = nextbufferp;
    	bhdl = (rm_bufferHandle)newbufferp;
    	return( bhdl );
    }
}

/*
    Initialize buffer header
*/
void	rm_bufferInitHeader(bhdl,startp,endp)
rm_bufferHandle bhdl;
pair_iXY    *startp,*endp;
{
    rm_StartPos(bhdl) = *startp;
    rm_EndPos(bhdl) = *endp;
    rm_EndpointsToDirection(rm_Direction(bhdl),startp,endp);
    rm_EndpointsToBBox(&rm_BBox(bhdl),startp,endp);
}


/*
    Releases all buffers used for storing rook move sequences
 */
void	rm_ClearBuffers( )
{
    rm_bufferFreep = rm_bufferArea;
}


/*
    Boot time intialization for rook move buffer manipulation
*/
void	rm_BootInit()
{
    rm_ClearBuffers();
}


