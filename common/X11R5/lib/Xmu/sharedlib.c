/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5Xmu:sharedlib.c	1.3"
/*
 * $XConsortium: sharedlib.c,v 1.6 91/07/31 21:12:30 keith Exp $
 * 
 * Copyright 1991 Massachusetts Institute of Technology
 *
 *
 */

#if defined(SUNSHLIB) && !defined(SHAREDCODE)

#include "Atoms.h"

struct _AtomRec {
    char *name;
    struct _DisplayRec* head;
};

#if defined(__STDC__) && !defined(UNIXCPP)
#define DeclareAtom(atom) \
extern struct _AtomRec __##atom; \
AtomPtr _##atom = &__##atom;
#else
#define DeclareAtom(atom) \
extern struct _AtomRec __/**/atom; \
AtomPtr _/**/atom = &__/**/atom;
#endif

DeclareAtom(XA_ATOM_PAIR)
DeclareAtom(XA_CHARACTER_POSITION)
DeclareAtom(XA_CLASS)
DeclareAtom(XA_CLIENT_WINDOW)
DeclareAtom(XA_CLIPBOARD)
DeclareAtom(XA_COMPOUND_TEXT)
DeclareAtom(XA_DECNET_ADDRESS)
DeclareAtom(XA_DELETE)
DeclareAtom(XA_FILENAME)
DeclareAtom(XA_HOSTNAME)
DeclareAtom(XA_IP_ADDRESS)
DeclareAtom(XA_LENGTH)
DeclareAtom(XA_LIST_LENGTH)
DeclareAtom(XA_NAME)
DeclareAtom(XA_NET_ADDRESS)
DeclareAtom(XA_NULL)
DeclareAtom(XA_OWNER_OS)
DeclareAtom(XA_SPAN)
DeclareAtom(XA_TARGETS)
DeclareAtom(XA_TEXT)
DeclareAtom(XA_TIMESTAMP)
DeclareAtom(XA_USER)

#endif /* SUNSHLIB */
