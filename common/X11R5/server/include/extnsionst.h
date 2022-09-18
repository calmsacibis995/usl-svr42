#ident	"@(#)siserver:include/extnsionst.h	1.4"

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* $XConsortium: extnsionst.h,v 1.9 89/08/31 18:41:12 rws Exp $ */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
                        All Rights Reserved
******************************************************************/

#ifndef EXTENSIONSTRUCT_H
#define EXTENSIONSTRUCT_H 
#include "extension.h"
typedef struct _ExtensionEntry {
    int index;
    void (* CloseDown)();	/* called at server shutdown */
    char *name;               /* extension name */
    int base;                 /* base request number */
    int eventBase;            
    int eventLast;
    int errorBase;
    int errorLast;
    int num_aliases;
    char **aliases;
    pointer extPrivate;
    unsigned short (* MinorOpcode)();	/* called for errors */
} ExtensionEntry;

extern void (* EventSwapVector[128]) ();

typedef void (* ExtensionLookupProc)();

typedef struct _ProcEntry {
    char *name;
    ExtensionLookupProc proc;
} ProcEntryRec, *ProcEntryPtr;

typedef struct _ScreenProcEntry {
    int num;
    ProcEntryPtr procList;
} ScreenProcEntry;

#define    SetGCVector(pGC, VectorElement, NewRoutineAddress, Atom)    \
    pGC->VectorElement = NewRoutineAddress;

#define    GetGCValue(pGC, GCElement)    (pGC->GCElement)

extern void InitExtensions();
extern int ProcQueryExtension();
extern int ProcListExtensions();
extern ExtensionEntry *AddExtension();
extern unsigned short MinorOpcodeOfRequest();
extern unsigned short StandardMinorOpcode();

#if ! SVR4 /* funNotUsedByATT, AddExtensionAlias, LookupProc, RegisterProc, RegisterScreenProc */
extern Bool AddExtensionAlias();
extern ExtensionLookupProc LookupProc();
extern Bool RegisterProc();
extern Bool RegisterScreenProc();
#endif

#endif /* EXTENSIONSTRUCT_H */
