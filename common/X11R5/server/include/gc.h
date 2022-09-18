#ident	"@(#)siserver:include/gc.h	1.4"

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/
/* $XConsortium: gc.h,v 1.50 89/09/12 09:27:42 rws Exp $ */

#ifndef GC_H
#define GC_H 

/* clientClipType field in GC */
#define CT_NONE			0
#define CT_PIXMAP		1
#define CT_REGION		2
#define CT_UNSORTED		6
#define CT_YSORTED		10
#define CT_YXSORTED		14
#define CT_YXBANDED		18

#define GCQREASON_VALIDATE	1
#define GCQREASON_CHANGE	2
#define GCQREASON_COPY_SRC	3
#define GCQREASON_COPY_DST	4
#define GCQREASON_DESTROY	5

#define GC_CHANGE_SERIAL_BIT        ((unsigned long)1L<<31)
#define GC_CALL_VALIDATE_BIT        ((unsigned long)1L<<30)
#define GCExtensionInterest	    ((unsigned long)1L<<29)

#define DRAWABLE_SERIAL_BITS        (~(GC_CHANGE_SERIAL_BIT))

#define MAX_SERIAL_NUM     ((unsigned long)1L<<28)
#define NEXT_SERIAL_NUMBER ((++globalSerialNumber) > MAX_SERIAL_NUM ? \
	    (globalSerialNumber  = 1): globalSerialNumber)

typedef struct _GC    *GCPtr;
extern void  ValidateGC();
extern int ChangeGC();
extern GCPtr CreateGC();
extern int CopyGC();
extern int FreeGC();

#if ! SVR4		/* funNotUsedByATT, SetGCMask */
extern void SetGCMask();
#endif	/* i386, funNotUsedByATT */

extern GCPtr GetScratchGC();
extern void  FreeScratchGC();
#endif /* GC_H */
