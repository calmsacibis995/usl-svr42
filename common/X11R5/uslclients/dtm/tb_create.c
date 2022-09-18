/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtm:tb_create.c	1.28"
#endif

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <libgen.h>

#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <Xol/BulletinBo.h>
#include <Xol/ScrolledWi.h>
#include <Xol/StaticText.h>
#include <Xol/RubberTile.h>
#include <FIconBox.h>

#include <Dtm.h>
#include "strings.h"
#include <extern.h>

#define EXTRA_ICON_SLOTS	5
#define	FOOTERHEIGHT	15

#define XA	XtArgVal

/* Toolbox window button data */
static DmMenuItemsRec FileMenuItems[] = {
  { (XA)"Open",		(XA)'O', NULL, (XA)DmTBOpenCB,   (XA)True},
  { (XA)"Copy...",	(XA)'C', NULL, (XA)DmTBCopyCB,   (XA)True},
  { (XA)"Move...",	(XA)'M', NULL, (XA)DmTBMoveCB,   (XA)True},
  { (XA)"Create...",	(XA)'R', NULL, (XA)DmTBCreateCB, (XA)True},
  { (XA)"Properties...",(XA)'P', NULL, (XA)DmTBPropCB,   (XA)True},
  { (XA)"Delete",	(XA)'D', NULL, (XA)DmTBDeleteCB, (XA)True},
  { (XA)"Exit...",	(XA)'E', NULL, (XA)DmExitCB,     (XA)True},
};

static DmMenuItemsRec SortMenuItems[] = {
   {(XA)"Name", (XA)'N', NULL, (XA)DmTBSortItemsCB,     (XA)True},
   {(XA)"Type", (XA)'T', NULL, (XA)DmTBSortItemsCB,     (XA)True},
};

static DmMenuItemsRec ViewMenuItems[] = {
   {(XA)"Align",             (XA)'A', NULL, (XA)DmTBAlign,     		(XA)True},
   {(XA)"Sort", (XA)'S', (XA)SortMenuItems, (XA)XtNumber(SortMenuItems), (XA)True},
};

static DmMenuItemsRec HelpMenuItems[] = {
  { NULL, NULL, NULL, (XA)DmHelpSpecificCB,	(XA)True},	/* Tool Box */
  { NULL, NULL, NULL, (XA)DmHelpTOCCB,		(XA)True},
  { NULL, NULL, NULL, (XA)DmHelpDeskCB,		(XA)True},
};

DmMenuItemsRec ToolboxMenuBar[] = {
   {(XA)"File",	(XA)'F', NULL, NULL, (XA)True},
   {(XA)"View",	(XA)'V', NULL, NULL, (XA)True},
   {(XA)"Help",	(XA)'H', NULL, NULL, (XA)True},
};
#undef XA


static void
Dm__InitToolboxStrings()
{

	int i;
	char buf[4];

	XtArgVal *buttonLabelPointers[] = {
	&ToolboxMenuBar[0].label /* file */,
	&ToolboxMenuBar[1].label /* view */,
	&ToolboxMenuBar[2].label /* help */,
	&FileMenuItems[0].label ,/*(open)*/
	&FileMenuItems[1].label ,/*(copy)*/
	&FileMenuItems[2].label ,/*(move)*/
	&FileMenuItems[3].label ,/*(create)*/
	&FileMenuItems[4].label ,/*(prop)*/
	&FileMenuItems[5].label ,/*(del)*/
	&FileMenuItems[6].label ,/*(ext)*/
	&ViewMenuItems[0].label ,/* (align)*/
	&ViewMenuItems[1].label ,/* (sort)*/
	&SortMenuItems[0].label ,/* (name)*/
	&SortMenuItems[1].label ,/* (type)*/
	&HelpMenuItems[0].label ,/*tool box */
	&HelpMenuItems[1].label ,/* table of contents */
	&HelpMenuItems[2].label ,/* help desk */
	};

	XtArgVal *mnemonicPointers[] = {
	&ToolboxMenuBar[0].mnemonic /* file */,
 	&ToolboxMenuBar[1].mnemonic /* view */,
	&ToolboxMenuBar[2].mnemonic /* help */,
	&FileMenuItems[0].mnemonic ,/*(open)*/
	&FileMenuItems[1].mnemonic ,/*(copy)*/
	&FileMenuItems[2].mnemonic ,/*(move)*/
	&FileMenuItems[3].mnemonic ,/*(create)*/
	&FileMenuItems[4].mnemonic ,/*(prop)*/
	&FileMenuItems[5].mnemonic ,/*(del)*/
	&FileMenuItems[6].mnemonic ,/*(ext)*/
	&ViewMenuItems[0].mnemonic ,/* (align)*/
	&ViewMenuItems[1].mnemonic ,/* (sort)*/
	&SortMenuItems[0].mnemonic ,/* (name)*/
	&SortMenuItems[1].mnemonic ,/* (type)*/
	&HelpMenuItems[0].mnemonic ,/*tool box*/
	&HelpMenuItems[1].mnemonic ,/*table of contents*/
	&HelpMenuItems[2].mnemonic ,/*help desk*/
	};
	
	char *addons[] = {
	"tb_file",
	"tb_view",
	"tb_help",
	"tb_open",
	"tb_copy",
	"tb_move",
	"tb_create",
	"tb_prop",
	"tb_del",
	"tb_ext",
	"tb_align",
	"tb_sort",
	"tb_name",
	"tb_type",
	"tb_tool_box",
	"tb_toc",
	"tb_help_desk",
	};
	
	for (i=0; i < XtNumber(addons); i++) 
	    {
	    *buttonLabelPointers[i] = (XtArgVal)OlGetMessage(
					XtDisplay(Desktop->init_shell), NULL,
                                        0, OleNlabel, addons[i],
                                        OleCOlClientDtmMsgs,
                                        toolbox_labels[i],
                                        (XrmDatabase)NULL);

	    *mnemonicPointers[i] = (XtArgVal) *OlGetMessage(
					XtDisplay(Desktop->init_shell),
                                        NULL, 0, OleNmnemonic, addons[i],
                                        OleCOlClientDtmMsgs,
                                        toolbox_mnemonics[i],
                                        (XrmDatabase)NULL);
	    }

	for(i=0; i < (TB_LASTMSG); i++)
	    {
	    sprintf(buf, "%d", Dm__TBErrorMessages[i].type);
	    Dm__TBErrorMessages[i].value = OlGetMessage(
						XtDisplay(Desktop->init_shell),
						NULL, 0,
						OleNfileextern, 
						buf,
						OleCOlClientDtmMsgs,
						toolbox_msgs[i],
						(XrmDatabase)NULL);
	    }

}    /* end of Dm__InitFolderStrings() */


extern DmToolboxWinPtr 
DmOpenToolboxWindow(char *path,
		    DtAttrs options,
		    char *geom_str,
		    Boolean iconic)
{
	static int first = 1;
	static int DefaultWidth;
	static int DefaultHeight;
	static Widget file_menu;
	static Widget help_menu;
	DmToolboxWinPtr	window;
	Widget	controlarea, menubar, rubbertile;
	DmObjectPtr op;
	DmItemPtr ip;
	int i;
	int calc_pos = 0;

	if (first) {
		/*
		 * Build sub menus that are shared by all toolbox windows.
		 * The minus one in the next statement is to skip the
		 * "Exit" button in the "File" menu. Later in the code, a
		 * special "File" menu (with the "Exit" button) is created
		 * for the toplevel toolbox window.
		 */

		/* initialize strings used in the toolbox */
		Dm__InitToolboxStrings();
		ToolboxMenuBar[0].popup_menu = (XtArgVal)
				DmCreatePopupMenu(DESKTOP_SHELL(Desktop),
				FileMenuItems, XtNumber(FileMenuItems) - 1,
				NULL, NULL);
		ToolboxMenuBar[1].popup_menu = (XtArgVal)
				DmCreatePopupMenu(DESKTOP_SHELL(Desktop),
				ViewMenuItems, XtNumber(ViewMenuItems),
				NULL, NULL);
		ToolboxMenuBar[2].popup_menu = (XtArgVal)
				DmCreatePopupMenu(DESKTOP_SHELL(Desktop),
				HelpMenuItems, XtNumber(HelpMenuItems),
				NULL, NULL);
		DefaultWidth  = TOOLBOX_WINDOW_WIDTH(Desktop->init_shell);
		DefaultHeight = TOOLBOX_WINDOW_HEIGHT(Desktop->init_shell);
	}

	if (window = DmQueryToolboxWindow(path)) {
		XtMapWidget(window->shell);
		if (OlCanAcceptFocus(window->box, CurrentTime) != False) {
                	XRaiseWindow(XtDisplay(window->shell),
				     XtWindow(window->shell));
			OlSetInputFocus(window->box,RevertToNone,CurrentTime);
		}
		return(window);
	}

	window = (DmToolboxWinPtr)calloc(1, sizeof(DmToolboxWinRec));

	if ((window->cp = DmGetToolboxInfo(path)) == NULL) {
		free(window);
		fprintf(stderr,"Can't open toolbox :%s:\n", path);
		return(NULL);
	}

	window->attrs = DM_B_TOOLBOX_WIN; /* identify it as a Toolbox window */

	/* add it to the list of toolbox windows */
	DmAddWindow((DmWinPtr *)&(Desktop->twp), (DmWinPtr)window);

	XtSetArg(Dm__arg[0], XtNwidth,  DefaultWidth);
	XtSetArg(Dm__arg[1], XtNheight, DefaultHeight);
	XtSetArg(Dm__arg[2], XtNtitle,  path);
	XtSetArg(Dm__arg[3], XtNiconic, iconic);
	XtSetArg(Dm__arg[4], XtNgeometry, geom_str);
	window->shell = XtCreateApplicationShell(path, 
				topLevelShellWidgetClass, Dm__arg, 5);

	for (op=window->cp->op; op; op=op->next) {
		char *p;

		if (p = DtGetProperty(&(op->plist), REAL_PATH, NULL)) {
			DmObjectPtr real_op;

			if (real_op = DmFileToObject(p)) {
				DmInitObjType(window->shell, real_op);
				op->objectdata = real_op;

				/*
				 * Use this to distinguish shortcut objects
				 * from real objects.
				 */
				op->attrs |= DM_B_SHORTCUT;

				/*
				 * Copy object info from the real object
				 * to the shortcut!
				 */
				op->fcp = real_op->fcp;
				op->ftype = real_op->ftype;
				if ((op->x == 0) && (op->y == 0))
					calc_pos++;
			}
			else {
				/* no such file */
				op->attrs |= DM_B_HIDDEN;
				op->objectdata = NULL;
			}
			
		}
		else {
			/* A toolbox container */
			op->fcp = DmFtypeToFmodeKey(DM_FTYPE_TOOLBOX)->fcp;
			op->ftype = DM_FTYPE_TOOLBOX;
			DmInitObjType(window->shell, op);
			if ((op->x == 0) && (op->y == 0))
				calc_pos++;
		}
	}

	XtSetArg(Dm__arg[0], XtNorientation, OL_VERTICAL);
	rubbertile = XtCreateManagedWidget(
					   "rubbertile",
					   rubberTileWidgetClass,
					   window->shell,
					   Dm__arg,
					   1);
	XtSetArg(Dm__arg[0], XtNtraversalManager, True);
	XtSetArg(Dm__arg[1], XtNweight, 0);
	controlarea = XtCreateManagedWidget(
					"controlarea",
					controlAreaWidgetClass,
					rubbertile,
					Dm__arg,
					2);

	if (first) {
		DmMenuItemsPtr menubar_items;

		/* create a special menubar for toplevel toolbox */
		menubar_items = (DmMenuItemsPtr)malloc(sizeof(ToolboxMenuBar));
		memcpy(menubar_items, ToolboxMenuBar, sizeof(ToolboxMenuBar));
		menubar_items[0].popup_menu = (XtArgVal)
				DmCreatePopupMenu(DESKTOP_SHELL(Desktop),
				FileMenuItems, XtNumber(FileMenuItems),
				NULL, NULL);

		menubar = DmCreateMenuBar(controlarea, menubar_items,
						XtNumber(ToolboxMenuBar),
						NULL, (XtPointer)window);
		first = 0;
	}
	else {
		DmMenuItemsPtr menubar_items;

		/* make a copy of the menubar struct */
		menubar_items = (DmMenuItemsPtr)malloc(sizeof(ToolboxMenuBar));
		memcpy(menubar_items, ToolboxMenuBar, sizeof(ToolboxMenuBar));
		menubar = DmCreateMenuBar(controlarea, menubar_items,
						XtNumber(ToolboxMenuBar),
						NULL, (XtPointer)window);
	}

	XtSetArg(Dm__arg[0], XtNdropProc,	Dm__ToolboxDropProc);
	XtSetArg(Dm__arg[1], XtNmenuProc,	DmIconMenuProc);
	XtSetArg(Dm__arg[2], XtNclientData,	window);
	XtSetArg(Dm__arg[3], XtNdblSelectProc,	Dm__ToolboxSelect2Proc);
	XtSetArg(Dm__arg[4], XtNdrawProc,	DmDrawLinkIcon);
	XtSetArg(Dm__arg[5], XtNfont,		DESKTOP_FONT(Desktop));
	XtSetArg(Dm__arg[6], XtNtriggerMsgProc, DmTBTriggerNotify);
	window->nitems = window->cp->num_objs + EXTRA_ICON_SLOTS;
	window->box = DmCreateIconContainer(rubbertile,
					DM_B_CALC_SIZE | DM_B_NO_INIT, 
					Dm__arg, 7,
			  		window->cp->op, 
					window->cp->num_objs,
			  		&(window->itp), 
					window->nitems,
			  		&(window->swin),
					DESKTOP_FONTLIST(Desktop),
					DESKTOP_FONT(Desktop),
					DESKTOP_LABELHT(Desktop));

	/* make the iconbox the initial focus widget */
	XtSetArg(Dm__arg[0], XtNfocusWidget, window->box);
	XtSetValues(window->shell, Dm__arg, 1);

	if (calc_pos) {
		int x, y;

		for (ip=window->itp, i=window->nitems; i; i--, ip++)
			if ((ITEM_MANAGED(ip) != False) &&
			    (ITEM_X(ip) == 0) && (ITEM_Y(ip) == 0)) {
				DmGetDefaultXY(&x, &y, window->itp,
						window->nitems, ip,
						(int)DefaultWidth,
						(int)ICON_GRID_WIDTH(Desktop),
						(int)ICON_GRID_HEIGHT(Desktop));
				ip->x = (XtArgVal)(Dimension)x;
				ip->y = (XtArgVal)(Dimension)y;
			}
		XtSetArg(Dm__arg[0], XtNitemsTouched, True);
		XtSetValues(window->box, Dm__arg, 1);
	}

	XtSetArg(Dm__arg[0], XtNwidth , DefaultWidth);
	XtSetArg(Dm__arg[1], XtNheight, FOOTERHEIGHT);
	XtSetArg(Dm__arg[2], XtNstring, "  ");  /* manish, change "  " */
	XtSetArg(Dm__arg[3], XtNgravity, WestGravity);
	XtSetArg(Dm__arg[4], XtNtraversalOn, False);
	XtSetArg(Dm__arg[5], XtNweight, 0);
	window->footer = XtCreateManagedWidget(
   				       	"footer",
				       	staticTextWidgetClass,
				       	rubbertile,
				       	Dm__arg,
				       	6);
	OlAddCallback(window->shell, XtNwmProtocol, ToolboxWinWMCB,
				(XtPointer)window);
	XtRealizeWidget(window->shell);

	XSetIconName(XtDisplay(window->shell), XtWindow(window->shell), path);

	return(window);
} /* end of DmOpenToolboxWindow */

DmToolboxWinPtr
DmContainerToToolboxWindow(cp)
DmContainerPtr cp;
{
	register DmToolboxWinPtr twp;

	for (twp = Desktop->twp; twp != NULL; twp = twp->next)
		if (cp == twp->cp)
			return(twp);
	return(NULL);
}	/* end of DmQueryToolboxWindow */

DmToolboxWinPtr
DmQueryToolboxWindow(path)
char *path;
{
	register DmToolboxWinPtr twp;

	for (twp = Desktop->twp; twp != NULL; twp = twp->next)
		if (!strcmp(path, twp->cp->path))
			return(twp);
	return(NULL);
}	/* end of DmQueryToolboxWindow */

extern void
DmCloseToolboxWindow(twp)
DmToolboxWinPtr twp;
{
	DmToolboxWinPtr p = Desktop->twp;
	DmObjectPtr op;
	DmContainerPtr cp;

	/* remove it from the DmToolboxWindow list */
	if (twp == Desktop->twp)
		Desktop->twp = twp->next;
	else {
		while (p->next != twp)
			p = p->next;
		p->next = twp->next;
	}
		   
	DmSaveXandY(twp->itp, twp->nitems);

	/* close the real object containers */
	for (op=twp->cp->op; op; op=op->next) {
		if ((op->ftype != DM_FTYPE_TOOLBOX) && op->objectdata) {
			cp = ((DmObjectPtr)(op->objectdata))->container;
			if (cp)
				DmCloseContainer(cp, DM_B_NO_FLUSH);
		}

		/* erase info copied from the real object */
		op->fcp = NULL;
		op->objectdata = NULL;
	}

	DmDestroyIconContainer(twp->shell, twp->box, twp->itp, twp->nitems);

	XtFree(twp->copy_prompt.current);
	XtFree(twp->move_prompt.current);
	XtFree(twp->create_prompt.current);
	/* NEED TO FREE LOTS OF STUFF HERE, INTERFACE !!!! */
} /* end of DmCloseToolboxWindow */

void
DmCloseToolboxWindows()
{
	register DmContainerPtr cp;
	register DmToolboxWinPtr twp;

	for (twp=DESKTOP_TWP(Desktop); twp; twp=twp->next)
		DmCloseToolboxWindow(twp);

	/* flush all the container info */
	for (cp=DESKTOP_TCP(Desktop); cp; cp=cp->next)
		DmWriteDtInfo(cp, TBINFO(cp)->path, DM_B_PERM);
}

