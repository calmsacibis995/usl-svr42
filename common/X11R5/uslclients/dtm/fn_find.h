/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* ident	"@(#)dtm:fn_find.h	1.10" */

#include <stdio.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLookP.h>
#include <Xol/BaseWindow.h>
#include <Xol/ControlAre.h>
#include <Xol/PopupWindo.h>
#include <Xol/Form.h>
#include <Xol/FooterPane.h>
#include <Xol/Caption.h>
#include <Xol/TextField.h>
#include <Xol/StaticText.h>
#include <Xol/Text.h>
#include <Xol/Stub.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrolledWi.h>
#include <Xol/OlStrings.h>
#include <Xol/Notice.h>



void	fn_WhereSelectCB();
void	fn_ContextSelectCB();
void    fn_DismissCB();
void    fn_FindCancelCB();
void    fn_FindingCanCB();
void    fn_StopCB();
void    DmFileCB();
void    fn_EditCB();
void    DmFindHelp();
void    fnEditSelectAllCB();
void    fnEditUnselectAllCB();
void    fn_txtfocus();
char	*fn_mounted();
void    DmFindCB();

/*
 * Each member of the Where to Look: list
 * will have the following structure.
 */
typedef struct entryrec * entryptr;

typedef struct entryrec{
        char *name;
        char *dir;
        Boolean removable;
        entryptr next;
} entry;
