/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:include/xtestqueue.h	1.3"

#ifndef _XTESTQ_H
#define _XTESTQ_H

/*
** Added for AT&T/i386 Server
*/

typedef struct XTestQueueNode
	{
		struct XTestQueueNode	*next;
		int			variant1;
		int			variant2;
		xqEvent			xqevent;
	} XTestQueueNode;

typedef struct
	{
		XTestQueueNode	*head;
		XTestQueueNode	*tail;
	} XTestQueue;

#define XTestQueueEmpty(q)	(!(q).head)
#define XTestQueueNotEmpty(q)	((q).head)

#endif
