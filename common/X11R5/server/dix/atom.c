/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:dix/atom.c	1.1"
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved



******************************************************************/

/* $XConsortium: atom.c,v 1.28 89/08/02 09:10:50 rws Exp $ */

#include "X.h"
#include "Xatom.h"
#include "misc.h"
#include "resource.h"

#define InitialTableSize 100

typedef struct _Node {
    struct _Node   *left,   *right;
    Atom a;
    unsigned int fingerPrint;
    char   *string;
} NodeRec, *NodePtr;

static Atom lastAtom = None;
static NodePtr atomRoot = (NodePtr)NULL;
static unsigned long tableLength;
static NodePtr *nodeTable;

Atom 
MakeAtom(string, len, makeit)
    char *string;
    unsigned len;
    Bool makeit;
{
    register    NodePtr * np;
    unsigned i;
    int     comp;
    register unsigned int   fp = 0;

    np = &atomRoot;
    for (i = 0; i < (len+1)/2; i++)
    {
	fp = fp * 27 + string[i];
	fp = fp * 27 + string[len - 1 - i];
    }
    while (*np != (NodePtr) NULL)
    {
	if (fp < (*np)->fingerPrint)
	    np = &((*np)->left);
	else if (fp > (*np)->fingerPrint)
	    np = &((*np)->right);
	else
	{			       /* now start testing the strings */
	    comp = strncmp(string, (*np)->string, (int)len);
	    if ((comp < 0) || ((comp == 0) && (len < strlen((*np)->string))))
		np = &((*np)->left);
	    else if (comp > 0)
		np = &((*np)->right);
	    else
		return(*np)->a;
	    }
    }
    if (makeit)
    {
	register NodePtr nd;

	nd = (NodePtr) xalloc(sizeof(NodeRec));
	if (!nd)
	    return BAD_RESOURCE;
	if (lastAtom < XA_LAST_PREDEFINED)
	{
	    nd->string = string;
	}
	else
	{
	    nd->string = (char *) xalloc(len + 1);
	    if (!nd->string) {
		xfree(nd);
		return BAD_RESOURCE;
	    }
	    strncpy(nd->string, string, (int)len);
	    nd->string[len] = 0;
	}
	if ((lastAtom + 1) >= tableLength) {
	    NodePtr *table;

	    table = (NodePtr *) xrealloc(nodeTable,
					 tableLength * (2 * sizeof(NodePtr)));
	    if (!table) {
		if (nd->string != string)
		    xfree(nd->string);
		xfree(nd);
		return BAD_RESOURCE;
	    }
	    tableLength <<= 1;
	    nodeTable = table;
	}
	*np = nd;
	nd->left = nd->right = (NodePtr) NULL;
	nd->fingerPrint = fp;
	nd->a = (++lastAtom);
	*(nodeTable+lastAtom) = nd;
	return nd->a;
    }
    else
	return None;
}

ValidAtom(atom)
    Atom atom;
{
    return (atom != None) && (atom <= lastAtom);
}

char *
NameForAtom(atom)
    Atom atom;
{
    NodePtr node;
    if (atom > lastAtom) return 0;
    if ((node = nodeTable[atom]) == (NodePtr)NULL) return 0;
    return node->string;
}

AtomError()
{
    FatalError("initializing atoms");
}

InitAtoms()
{
    FreeAllAtoms();
    tableLength = InitialTableSize;
    nodeTable = (NodePtr *)xalloc(InitialTableSize*sizeof(NodePtr));
    if (!nodeTable)
	AtomError();
    nodeTable[None] = (NodePtr)NULL;
    MakePredeclaredAtoms();
    if (lastAtom != XA_LAST_PREDEFINED)
	AtomError ();
}

FreeAllAtoms()
{
    if(atomRoot == (NodePtr)NULL)
	return;
    FreeAtom(atomRoot);
    atomRoot = (NodePtr)NULL;
    xfree(nodeTable);
    nodeTable = (NodePtr *)NULL;
    lastAtom = None;
}

FreeAtom(patom)
    NodePtr patom;
{
    if(patom->left)
	FreeAtom(patom->left);
    if(patom->right)
	FreeAtom(patom->right);
    if (patom->a > XA_LAST_PREDEFINED)
	xfree(patom->string);
    xfree(patom);
}
    
