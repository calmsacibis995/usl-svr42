/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/segment.h	1.1"
/*
 * @(#)segment.h 1.2 89/03/10
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
    The tracing routines assume that the arcs they are given are "traceable".
    By that it is meant is that (i) they are contained in a quadrant and
    that (ii) they are contained in a bbox small enough not to overflow
    the 32-bit integer arithmetic used to trace the arcs. The first
    of the two conditions is intrinsic in the arc. The second depends
    on the implementation of the tracing code. The following two
    constants are used to determine what is small enough to be
    traceable... and they will change with different implementations
    of the segment handler.
*/

#define	seg_MAXLINEDIMENSION	2000
#define seg_MAXCONICDIMENSION	97

/*
    Given a <line> segment and a handle <bhdl> to a large enough rook move 
    buffer, traces the line and  fills the buffer accordingly.
*/
extern void	seg_TraceLine(/* line, bhdl */);
/* arc_frLine   	    *line;  */
/* rm_bufferHandle  bhdl;   */


/*
    Given a <conic> segment that is already in a quadrant and a handle
    <bhdl> to a large enough rook move buffer, traces the conic and
    fills the buffer accordingly.
    Assumes that the conic is traceable.
*/
extern void	seg_TraceConic(/* conic, bhdl */);
/*  arc_frConic    	    *conic; */ 	
/*  rm_bufferHandle	    bhdl;   */
