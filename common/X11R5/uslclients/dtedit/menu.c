/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtedit:menu.c	1.8"
#endif

/*
 * menu.c
 *
 */

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <buffutil.h>
#include <textbuff.h>

#include <OpenLook.h>

#include <Gizmos.h>
#include <MenuGizmo.h>
#include <PopupGizmo.h>
#include <ModalGizmo.h>
#include <BaseWGizmo.h>

#include <editor.h>
#include <file.h>
#include <edit.h>
#include <prop.h>
#include <view.h>
#include <help.h>
#ifdef TXT_FIND
#include <find.h>
#endif

/*
 *
 #    #  ######  #    #  #    #   ####
 ##  ##  #       ##   #  #    #  #
 # ## #  #####   # #  #  #    #   ####
 #    #  #       #  # #  #    #       #
 #    #  #       #   ##  #    #  #    #
 #    #  ######  #    #   ####    ####
 *
 */

static MenuItems  FileMenuItems[] = 
   {
   { True, TXT_NEW,          MNE_NEW     },
   { True, TXT_OPEN,         MNE_OPEN    },
   { True, TXT_SAVE,         MNE_SAVE    },
   { True, TXT_SAVE_AS,      MNE_SAVE_AS },
   { True, TXT_PRINT,        MNE_PRINT   },
   { True, TXT_EXIT,         MNE_EXIT    },
   { 0 }
   };

static MenuItems  EditMenuItems[] =
   {
   { True, TXT_UNDO,         MNE_UNDO        },
   { True, TXT_CUT,          MNE_CUT         },
   { True, TXT_COPY,         MNE_COPY        },
   { True, TXT_PASTE,        MNE_PASTE       },
   { True, TXT_DELETE,       MNE_DELETE      },
   { True, TXT_SELECTALL,    MNE_SELECTALL   },
   { True, TXT_UNSELECTALL,  MNE_UNSELECTALL },
   { 0 }
   };

static MenuItems  ViewMenuItems[] =
   {
   { True, TXT_SPLIT,        MNE_SPLIT                    },
   { True, TXT_PROPERTIES,   MNE_PROPERTIES, NULL, PropCB },
#ifdef TXT_FIND
   { True, TXT_FIND,         MNE_FIND,       NULL, FindCB },
#endif
   { 0 }
   };

static MenuItems  HelpMenuItems[] =
   {
   { True, TXT_APP_HELP,     MNE_APP_HELP     },
   { True, TXT_TOC_HELP,     MNE_TOC_HELP     },
   { True, TXT_HELPDESK,     MNE_HELPDESK     },
   { 0 }
   };

static MenuGizmo FileMenu =
   { NULL,      "filemenu",   NULL,   FileMenuItems, FileCB };
static MenuGizmo EditMenu =
   { NULL,      "editmenu",   NULL,   EditMenuItems, EditCB };
static MenuGizmo ViewMenu =
   { NULL,      "viewmenu",   NULL,   ViewMenuItems, ViewCB };
static MenuGizmo HelpMenu =
   { NULL,      "helpmenu",   NULL,   HelpMenuItems, HelpCB };

static MenuItems  BarMenuItems[] =
   {
   { True, TXT_FILE, MNE_FILE, (char *)&FileMenu },
   { True, TXT_EDIT, MNE_EDIT, (char *)&EditMenu },
   { True, TXT_VIEW, MNE_VIEW, (char *)&ViewMenu },
   { True, TXT_HELP, MNE_HELP, (char *)&HelpMenu },
   { 0 }
   };

extern MenuGizmo MenuBar =
   { 
   NULL, "menubar", NULL, BarMenuItems, NULL, NULL, CMD, OL_FIXEDROWS, 1
   };
