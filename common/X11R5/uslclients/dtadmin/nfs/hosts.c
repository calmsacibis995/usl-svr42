/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtadmin:nfs/hosts.c	1.14"
#endif

/*
 * Module:	dtadmin:nfs   Graphical Administration of Network File Sharing
 * File:	hosts.c       get remote system names
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Intrinsic.h>
#include <DtI.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/PopupWindo.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <Xol/FList.h>
#include <Xol/ScrolledWi.h>

#include <Gizmo/Gizmos.h>
#include <Gizmo/BaseWGizmo.h>
#include <Gizmo/MenuGizmo.h>
#include <Gizmo/PopupGizmo.h>
#include <Gizmo/ListGizmo.h>
#include <Gizmo/InputGizmo.h>

#include "nfs.h"
#include "text.h"
#include "verify.h"
#include "help.h"

extern  NFSWindow *nfsw;

#define WHITESPACE	" \t\n"
#define HOST_ALLOC_SIZE	50

typedef struct {
    XtPointer	name;
} FormatData;


typedef struct {
    Widget	listWidget;
    FormatData	*list;
    unsigned	cnt;
    unsigned	allocated;
} HostList;

typedef enum _readHostsReturn { NewList, SameList, Failure } readHostsReturn;

static void	FreeHosts (HostList *);
static readHostsReturn	ReadHosts (HostList *);
static int	HostCmp ();
static void ExecuteCB();
static void SelectCB();
static void UnselectCB();
static void hostMenuCB();
static void getHosts();

static HostList	Hosts = { 0, NULL, 0, 0 };
static String SelectedHost = NULL;
static InputGizmo *HostGizmoPtr;

static ListHead currentItem = {
	(ListItem *)0, 0, 0
};

static ListHead previousItem = {
	(ListItem *)0, 0, 0
};

static ListHead initialItem = {
	(ListItem *)0, 0, 0
};
static Setting HostListSetting =
{
	"hostList",
	(XtPointer)&initialItem,
	(XtPointer)&currentItem,
	(XtPointer)&previousItem
};


static ListGizmo hostListG = {
	&HostListHelp, "hostList", "list Label", (Setting *)&HostListSetting,
	"%s", True, 8,
	NULL,
	(XtArgVal) False,		/* Don't copy this list */
	ExecuteCB, SelectCB, UnselectCB,
};

static GizmoRec hostGizmos[] =
{
   { ListGizmoClass, &hostListG },
};

typedef enum _hostMenuItemsIndex 
   { applyHost, /* newHost, */ cancelHost, hostHelp } 
   hostMenuItemIndex;

static MenuItems  hostMenuItems[] =
   {
      {True, TXT_OK,  CHR_OK },
/*      {True, TXT_New,  CHR_New }, */
      {True, TXT_Cancel, CHR_Cancel },
      {True, TXT_Help,   CHR_Help },
      { 0 }
   };

static MenuGizmo  hostMenu = { &HostPopupMenuHelp, "_X_", "_X_", hostMenuItems, hostMenuCB };

static PopupGizmo HostPopup =
   { &HostPopupHelp, "host", TXT_HostTitle, &hostMenu, hostGizmos,
	 XtNumber(hostGizmos) }; 

extern Boolean 
verifyHost2(char * host, Gizmo popup)
{
    FormatData	key;

    if (strpbrk(host, WHITESPACE) != NULL)
    {
	SetMessage(popup, TXT_MultipleHosts, Popup);
	return INVALID;
    }
    if (ReadHosts(&Hosts) == Failure || Hosts.cnt == 0)
    {
	SetMessage(popup, TXT_EtcHostOpenFailed, Popup);
	return INVALID;
    }
    key.name = (XtPointer)host;
    if (bsearch((void *)&key, (void *)Hosts.list,
		(size_t)Hosts.cnt, sizeof(key), HostCmp) == NULL)
    {
	char error_text[BUFSIZ];

	sprintf(error_text, GetGizmoText(TXT_UnknownHost), host);
	SetMessage(popup, error_text, Popup);
	return INVALID;
    }
    return VALID;
}

extern Boolean 
verifyHost(char * host, Gizmo popup)
{
    extern Boolean removeWhitespace();

    (void) removeWhitespace(host);
    if (host == NULL || *host == EOS)
    {
	SetMessage(popup, TXT_HostRequired, Popup);
	return INVALID;
    }
    return verifyHost2(host, popup);
}


/* ReadHosts
 *
 * Read /etc/hosts file.  Return True if there was no problem.
 */
static readHostsReturn
ReadHosts (HostList *hosts)
{
    FILE	  *hostFile;
    FormatData	  *remoteSys;
    FormatData	   searchkey;
    char	  *token;
    char	   buf [MEDIUMBUFSIZE];
    struct stat    statBuffer;
    static time_t  lastRead = 0;

    while(!(hostFile = fopen ("/etc/hosts", "r")) && errno == EINTR)
	/* try again */;
    if (!hostFile)
    {
	SetMessage(nfsw-> hostPopup, TXT_EtcHostOpenFailed, Popup);
	return Failure;
    }
    while (fstat(hostFile->_file, &statBuffer) < 0 && errno == EINTR)
	/* try again */;
    
    if (statBuffer.st_mtime > lastRead)
    {
	lastRead = statBuffer.st_mtime;
	FreeHosts(hosts);
    }
    else
    {
	DEBUG0("using host data from earlier call\n");
	fclose(hostFile);
	return SameList;
    }

    while (fgets (buf, MEDIUMBUFSIZE, hostFile))
    {
	/* remove comments */
	token = strchr (buf, '#');
	if (token)
	    *token = EOS;

	/* the host name is the second token */
	token = strtok (buf, WHITESPACE);
	if (token)
	{
	    while ((token = strtok (NULL, WHITESPACE)))
	    {
		if (hosts->cnt >= hosts->allocated)
		{
		    hosts->allocated += HOST_ALLOC_SIZE;
		    hosts->list = (FormatData *)
			REALLOC ((char *) hosts->list,
				   hosts->allocated * sizeof (FormatData));
		}

		hosts->list [hosts->cnt++].name = (XtPointer) strdup (token);
	    }
	}
    }
    fclose (hostFile);

    if (hosts->cnt == 0)
	return Failure;

    /* Sort the host list */
    qsort ((char *) hosts->list, hosts->cnt, sizeof (FormatData), HostCmp);

    return NewList;
}	/* End of ReadHosts () */

/* FreeHosts
 *
 * Free all old host data.  The list itself is kept around to reduce mallocing
 * on later calls.
 */
static void
FreeHosts (HostList *hosts)
{
    register	i;

    if (hosts == NULL)
	return;

    for (i=hosts->cnt; --i>=0; )
    {
	if (hosts->list [i].name != NULL)
	    FREE (hosts->list [i].name);
    }

    hosts->cnt = 0;
}	/* End of FreeHosts () */


/* HostCmp
 *
 * Comparison function for Host List sorter.
 */
static int
HostCmp (FormatData *h1, FormatData *h2)
{
    return (strcoll (h1->name, h2->name));
}	/* End of HostCmp () */



extern void
HostCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    Arg             arg[10];

    HostGizmoPtr = (InputGizmo *)client_data;
    if (nfsw->hostPopup == NULL)
    {
	nfsw->hostPopup = &HostPopup;
	CreateGizmo(w, PopupGizmoClass, nfsw->hostPopup, NULL, 0);
        XtSetArg(arg[0], XtNnoneSet, True);
	XtSetValues(hostListG.flatList, arg, 1);
	OlVaFlatSetValues(hostMenu.child, applyHost,
			  XtNsensitive, False, NULL);
    }
    getHosts();
    MapGizmo(PopupGizmoClass, nfsw->hostPopup);
    return;
}

static void
getHosts()
{
    int index;
    ListHead	*headPtr;
    readHostsReturn retval;

    if ((retval = ReadHosts(&Hosts)) == Failure)
    {
	ManipulateGizmo(ListGizmoClass, &hostListG, ResetGizmoValue); 
	return;
    }
    /*  headPtr-> numfields will be zero if /etc/hosts was read to     */
    /*  validate a host before it was read to post the scrolling list. */
    headPtr = (ListHead *) (hostListG.settings-> current_value);
    if (retval == NewList || headPtr-> numFields == 0)
    {
	headPtr = (ListHead *) (hostListG.settings-> previous_value);
	if (headPtr->list)
	    FREE((char *)headPtr->list);
	headPtr->numFields = 1;
	headPtr->size = Hosts.cnt;
	headPtr->list = (ListItem *)MALLOC(sizeof(ListItem) *headPtr->size);
	for (index =  Hosts.cnt - 1; index >= 0; index--)
	{
	    headPtr-> list[index].set = False;
	    headPtr-> list[index].clientData = NULL;
	    headPtr-> list[index].fields = (XtArgVal)&Hosts.list[index].name;
	}
	ManipulateGizmo(ListGizmoClass, &hostListG, ResetGizmoValue);
    }
}


static void 
hostMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    OlFlatCallData * p          = (OlFlatCallData *)call_data;
    NFSWindow *     nfsw        = FindNFSWindow(w);
    PopupGizmo *     popup      = nfsw-> hostPopup;
    Widget	    shell	= GetPopupGizmoShell(popup);

    SetMessage(nfsw-> hostPopup, "", Popup);
    switch (p-> item_index)
    {
    case applyHost:
	ManipulateGizmo(PopupGizmoClass, popup, GetGizmoValue);
	BringDownPopup(shell);
	setInitialValue(HostGizmoPtr->settings, SelectedHost);
	ManipulateGizmo(InputGizmoClass, HostGizmoPtr,
			ReinitializeGizmoValue);
	setInitialValue(HostGizmoPtr->settings, "");
	if (HostGizmoPtr-> verify != (void (*)())NULL)
	    HostGizmoPtr-> verify(HostGizmoPtr->textFieldWidget, 0, NULL);
	return;
	break;
/*
    case newHost:
	SetMessage(nfsw-> hostPopup, "Feature Not Implemented", Popup);
	break;
*/
    case cancelHost:
	SetWMPushpinState(XtDisplay(shell), XtWindow(shell), WMPushpinIsOut);
	XtPopdown(shell);
	break;
    case hostHelp:
	PostGizmoHelp(nfsw-> baseWindow-> shell, &HostWindowHelp);
	break;
    default:
	DEBUG0("default in hostMenuCB taken!!!\n");
    }
}				/* end of hostMenuCB */


static void 
SelectCB(Widget wid, XtPointer client_data, OlFlatDropCallData *call_data)
{
    Cardinal  item_index = call_data->item_data.item_index;
    char **   fields = (char **)GetListField(&hostListG, item_index);

    SelectedHost = fields[0];
    OlVaFlatSetValues(hostMenu.child, applyHost,
		      XtNsensitive, True, NULL);
    
    return;
}


static void 
UnselectCB(Widget wid, XtPointer client_data, XtPointer call_data)
{
    SelectedHost = NULL;
    return;
}


static void 
ExecuteCB(Widget wid, XtPointer client_data, XtPointer pcall_data)
{
    return;
}
