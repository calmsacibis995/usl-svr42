/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libfolio1.2:include/errors.h	1.1"
/*
 * @(#)errors.h 1.3 89/05/11
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


#define error_NOERROR       ((int8)0)
#define error_FATAL         ((int8)1)
#define error_NOTFATAL      ((int8)2)


extern	void	error_Clear();
/*
	Clears all past errors.
*/


extern	int32	error_GetCount();


extern	void	error_SetStatus(/*errptr,severity*/);
/*char	*errptr;*/
/*int8	severity;*/
/*
	To report a new error.
*/


extern	char	*error_GetNextError();
/*
	This function returns the most recent unread error message or 
	error_CLEAR if no more errors remain.
*/


extern	int8    error_GetSeverity();
/*
	This function returns the most recent unread error severity or
	error_NOERROR if no more error severities remain.
*/ 

extern	int8	error_GetNextSeverity();
/*
	This function returns the most recent unread error severity or
	error_NOERROR if no more error severities remain. It removes
	the current error from the stack of errors.
*/

