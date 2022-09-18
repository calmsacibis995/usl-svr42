/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/trans.h	1.1"
/*
 * @(#)trans.h 1.4 89/05/25
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


/*---------------------------------------------------------------
    	TRANSFORMATIONS
---------------------------------------------------------------*/

/*
    Construction of transformation matrices
*/

extern	void	trans_MakeTranslation (/*dx,dy,tm*/);
/*  double   	    dx,dy;	*/
/*  trans_dTrans    *tm;	*/


extern	void	trans_MakeScale (/*sx,sy,tm*/);
/*  double	    sx,sy;	*/
/*  trans_dTrans    *tm;	*/


extern	void	trans_MakeRotation (/*ang,tm*/);
/*  double	    ang;	*/
/*  trans_dTrans    *cmt;	*/


/*---------------------------------------------------------------*/

/*
    Operations on transformation matrices
*/


extern	void	trans_Multiply (/*m1,m2,m3*/);
/*  trans_dTrans    	*m1,*m2,*m3; */


extern	bool	trans_Invert(/*cmt,res*/);
/*  trans_dTrans	*cmt,*res;  */


extern	void	trans_Concatenate (/*mt,cmt*/);
/*  trans_dTrans	*mt,*cmt;   */


extern	void	trans_Translate(/*x,y,cmt*/);
/*  double	    x,y;    */
/*  trans_dTrans    *cmt;   */


extern	void	trans_Scale(/*sx,sy,cmt*/);
/*  double	    sx,sy;  */
/*  trans_dTrans    *cmt;   */


extern	void	trans_Rotate(/*ang,cmt*/);
/*  double	    ang;    */
/*  trans_dTrans    *cmt;   */


extern	void	trans_GridFit(/*cmt*/);
/*  trans_dTrans    *cmt;   */

/*----------------------------------------------------------------*/

/*
    Operations of transformation matrices on points and distances
*/


extern void	trans_Apply(/*cmt,p*/);
/* trans_dTrans	    *cmt;	*/
/* pair_dXY   	    *p;  	*/

extern void	trans_ApplyFloating(/*cmt,p*/);
/* transdfTrans     *cmt;	*/
/* pair_dXY    	    *p;  	*/

extern void	trans_ApplyDisplacement(/*cmt,p*/);
/* trans_dTrans     *cmt;	*/
/* pair_dXY    	    *p;  	*/

extern void trans_dTofr(/*frmt,dmt*/);
/*trans_frTrans   *frmt;	*/
/*trans_dTrans    *dmt;		*/

extern void trans_frTod(/*dmt,frmt*/);
/*trans_dTrans    *dmt;		*/
/*trans_frTrans   *frmt;	*/
