/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/rmbuffer.h	1.1"
/*
 * @(#)rmbuffer.h 1.2 89/03/10
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
    A RMBUFFER contains the definition of a sequence of rook moves derived
    from a segment between two points. Points, distances and displacements
    are all measured in grid coordinates. Each rook move sequence is restricted
    within the four cartesian quadrants. Given a strating point, ending point and
    one of the four possible directions the sequences will resemble the following
    depiction.
    

              end                   end
                |                    | 
                 --              ----
           UPLEFT  |            |   UPRIGHT
                    ---  start
                         |  |
           DOWNLEFT      |   ----- DOWNRIGHT
                    -----         |
                   |             end
                  end      
    	    	    	    
    The actual sequence is a series of moves along the X or the Y axis.
    In the case of the UPRIGHT direction, an X move is a move to the right
    and a Y move is a move upward etc. Using the convention of '0' for an 
    XMOVE and '1' for a YMOVE the rook move sequence for the UPLEFT example 
    above can be coded as '0001001' and the DOWNRIGHT example is '1000001'

*/  	    	     

#define rm_BUFFERAREASIZE   (4*1024)
byte rm_bufferArea[rm_BUFFERAREASIZE];	    

#define rm_BUFFERAREAEND    (rm_bufferArea + rm_BUFFERAREASIZE)
byte	*rm_bufferFreep;	    /* points to free section of buffer */

/* Rook move directions */
#define rm_UPRIGHT  	1
#define rm_UPLEFT   	2
#define rm_DOWNLEFT 	3
#define rm_DOWNRIGHT	4

#define rm_XMOVE 0
#define rm_YMOVE 1

/*
    	Rook move Buffer Header definition 

        Given the starting and ending positions of the rook move sequence
        the bounding box can be calculated. Nevertheless, the bounding box
    	is stored in rook move buffer definition for ease of access
*/  

typedef struct _rm_bufferHeaderType{
    pair_iXY	startpos;   /* starting position of rook move sequence */
    pair_iXY	endpos;     /* ending position of rook move sequence */
    bbox_iBBox	bbox;	    /* bounding box for rook move sequence */
    int32   direction;	    /* one of the above directions */
} rm_bufferHeaderType, 
  *rm_bufferHandle;

#define rm_BUFFERHEADERSIZE 	sizeof(rm_bufferHeaderType)

/*
 * Macros to PUT and GET information out of rook move buffers
 *
 */
#define rm_StartPos(bhdl)  	    ((bhdl)->startpos)
#define rm_EndPos(bhdl)  	    ((bhdl)->endpos)
#define rm_Direction(bhdl)  	    ((bhdl)->direction)
#define rm_BBox(bhdl)	    	    ((bhdl)->bbox)
#define rm_Size(bhdl)    	    ( ((bhdl)->bbox.hix - (bhdl)->bbox.lox) +   \
       	    	    	    	      ((bhdl)->bbox.hiy - (bhdl)->bbox.loy) )

/*  Convert endpoints of rook move to direction */
#define rm_EndpointsToDirection(direction,startp,endp)      	\
    	    	    {	    	    	    	    	    	\
    	    	    	if ( ((endp)->x >  (startp)->x) &&  	\
    	    	    	     ((endp)->y >= (startp)->y) )	\
    	    	    	    	direction = rm_UPRIGHT;	    	\
    	    	    	else if (((endp)->x <=  (startp)->x) &&	\
    	    	    	     	 ((endp)->y > (startp)->y))	\
    	    	    	    	direction = rm_UPLEFT;	    	\
    	    	    	else if (((endp)->x <  (startp)->x) &&	\
    	    	    	     	 ((endp)->y <= (startp)->y))	\
    	    	    	    	direction = rm_DOWNLEFT;    	\
    	    	    	else 	    	    	    	    	\
    	    	    	    	direction = rm_DOWNRIGHT;   	\
    	    	    }

/* Convert endpoints of rook move to bounding box */
#define rm_EndpointsToBBox(bboxp,startp,endp)                       \
    	    	    {	(bboxp)->lox = min((startp)->x,(endp)->x);  \
    	    	     	(bboxp)->loy = min((startp)->y,(endp)->y);  \
    	    	      	(bboxp)->hix = max((startp)->x,(endp)->x);  \
    	    	      	(bboxp)->hiy = max((startp)->y,(endp)->y);  \
    	    	    }	

    
/* initialize byte pointer <*movesp> to sequence of rook moves */
#define rm_InitMovesp(bhdl,movesp)  (movesp=((byte *)bhdl + rm_BUFFERHEADERSIZE))


#define rm_PutX(movesp)	    	    (*(movesp)++ = rm_XMOVE)
#define rm_PutY(movesp)	    	    (*(movesp)++ = rm_YMOVE)
#define rm_GetMove(movesp)	    (*(movesp)++)
#define rm_IsFirstMove(bhdl,movesp) ((movesp)==((byte *)(bhdl) + rm_BUFFERHEADERSIZE))
#define rm_PeekPrevMove(movesp)	    (*(movesp-1))



/*
    Allocates one buffer including a header for storing rook move
    sequences in one of the four possible directions. 
    Returns pointer to allocated buffer header,
    or returns NULL if required <size> is too large for remaining buffer area
 */
extern rm_bufferHandle rm_AllocateBuffer(/* size */);
/*  int32   size;   */

/*
    Initialize buffer header
*/
extern void	rm_bufferInitHeader(/*bhdl,startp,endp*/);
/*  rm_bufferHandle bhdl;   	*/
/*  pair_iXY    *startp,*endp;	*/


/*
    Releases all buffers used for storing rook move sequences
 */
extern void rm_ClearBuffers();


/*
    Boot time intialization for rook move buffer manipulation
*/
extern void rm_BootInit();







