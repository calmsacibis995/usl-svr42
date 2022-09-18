/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XKeysymStr.c	1.2"
/* $XConsortium: XKeysymStr.c,v 11.7 91/05/04 14:03:22 rws Exp $ */
/* Copyright 1990 Massachusetts Institute of Technology */

/*
*/

#include "Xlibint.h"
#include <X11/Xresource.h>
#include <X11/keysymdef.h>

#if defined(__STDC__)
#define Const const
#else
#define Const /**/
#endif

typedef unsigned long Signature;

#define NEEDVTABLE
#include "ks_tables.h"

extern XrmDatabase _XInitKeysymDB();
extern Const unsigned char _XkeyTable[];


typedef struct _GRNData {
    char *name;
    XrmRepresentation type;
    XrmValuePtr value;
} GRNData;

/*ARGSUSED*/
static Bool SameValue(db, bindings, quarks, type, value, data)
    XrmDatabase		*db;
    XrmBindingList      bindings;
    XrmQuarkList	quarks;
    XrmRepresentation   *type;
    XrmValuePtr		value;
    XPointer		data;
{
    GRNData *gd = (GRNData *)data;

    if ((*type == gd->type) && (value->size == gd->value->size) &&
	!strncmp((char *)value->addr, (char *)gd->value->addr, value->size))
    {
	gd->name = XrmQuarkToString(*quarks); /* XXX */
	return True;
    }
    return False;
}

char *XKeysymToString(ks)
    KeySym ks;
{
    register int i, n;
    int h;
    register int idx;
    Const unsigned char *entry;
    unsigned char val1, val2;
    XrmDatabase keysymdb;

    if (!ks)
	return ((char *)NULL);
    if (ks == XK_VoidSymbol)
	ks = 0;
    if (ks <= 0xffff)
    {
	val1 = ks >> 8;
	val2 = ks & 0xff;
	i = ks % VTABLESIZE;
	h = i + 1;
	n = VMAXHASH;
	while (idx = hashKeysym[i])
	{
	    entry = &_XkeyTable[idx];
	    if ((entry[0] == val1) && (entry[1] == val2))
		return ((char *)entry + 2);
	    if (!--n)
		break;
	    i += h;
	    if (i >= VTABLESIZE)
		i -= VTABLESIZE;
	}
    }

    if (keysymdb = _XInitKeysymDB())
    {
	char buf[9];
	XrmValue resval;
	XrmQuark empty = NULLQUARK;
	GRNData data;

	sprintf(buf, "%lX", ks);
	resval.addr = (XPointer)buf;
	resval.size = strlen(buf) + 1;
	data.name = (char *)NULL;
	data.type = XrmPermStringToQuark("String");
	data.value = &resval;
	(void)XrmEnumerateDatabase(keysymdb, &empty, &empty, XrmEnumAllLevels,
				   SameValue, (XPointer)&data);
	return data.name;
    }
    return ((char *) NULL);
}
