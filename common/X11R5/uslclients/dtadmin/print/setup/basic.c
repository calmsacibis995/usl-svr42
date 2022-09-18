/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/basic.c	1.17"
#endif

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/ControlAre.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>
#include <Xol/FList.h>
#include <Xol/BulletinBo.h>

#include <lp.h>
#include <systems.h>
#include <printers.h>

#include "properties.h"
#include "printer.h"
#include "lpsys.h"
#include "error.h"

enum {
    Sys_V_Button, BSD_Button,
};

static void 	CreateBasic (Widget, PropertyData *);
static void 	InitBasic (Widget, PropertyData *, PRINTER *);
static void 	UpdateBasic (Widget, PropertyData *, PRINTER *);
static char	*CheckBasic (Widget, PropertyData *);
static void	ApplyBasic (Widget, PropertyData *);
static void	ResetBasic (Widget, PropertyData *);

static Widget	CreateParallelControls (Widget, PropertyCntl *);
static Widget	CreateSerialControls (Widget, PropertyCntl *);
static Widget	CreateRemoteControls (Widget, PropertyCntl *);
static void	ResetParallel (PropertyData *);
static void	ResetSerial (PropertyData *);
static void	ResetRemote (PropertyData *);

static void	ParallelButtonSelect (Widget, XtPointer, XtPointer);
static void	SerialButtonSelect (Widget, XtPointer, XtPointer);
static Cardinal	GetRemoteAccess (char *name);

static void	NameCheckCB (Widget, XtPointer, XtPointer);

static PropertyCntl	AddParControls;
static PropertyCntl	AddSerControls;
static PropertyCntl	AddRmtControls;
PropertyCntl		ChangeControls;

static uid_t	LpUid;
static gid_t	LpGid;

#if defined(CAN_DO_MODULES)
static char	*DfltList [] = {
    "default",
    0,
};
#endif

static Widget	(*CreatePortControls [])(Widget, PropertyCntl *) = {
    CreateParallelControls,
    CreateSerialControls,
    CreateRemoteControls,
};

static void	(*ResetPortControls [])(PropertyData *) = {
    ResetParallel,
    ResetSerial,
    ResetRemote,
};

static ButtonItem YesNoItems [] = {
    { (XtArgVal) True, (XtArgVal) TXT_yes, },		/* Yes */
    { (XtArgVal) False, (XtArgVal) TXT_no, },		/* No */
};

static ButtonItem ParallelItems [] = {
    { (XtArgVal) "/dev/lp0", (XtArgVal) TXT_lpt1, },	/* Lpt1 */
    { (XtArgVal) "/dev/lp1", (XtArgVal) TXT_lpt2, },	/* Lpt2 */
    { (XtArgVal) 0, (XtArgVal) TXT_other, },		/* Other */
};

static ButtonItem SerialItems [] = {
    { (XtArgVal) "/dev/tty00", (XtArgVal) TXT_com1, },	/* Com1 */
    { (XtArgVal) "/dev/tty01", (XtArgVal) TXT_com2, },	/* Com2 */
    { (XtArgVal) 0, (XtArgVal) TXT_other, },		/* Other */
};

static ButtonItem OSItems [] = {
    { (XtArgVal) S5_OS, (XtArgVal) TXT_sysv, },		/* System V */
    { (XtArgVal) BSD_OS, (XtArgVal) TXT_bsd, },		/* BSD */
};

/* InitBasicSheet
 *
 * Initial basic property sheet functions.
 */
void
InitBasicSheet (Widget widget, CategoryPage *page)
{
    page->lbl = GetStr (TXT_basic),
    page->createProc = CreateBasic;
    page->initProc = InitBasic;
    page->updateProc = UpdateBasic;
    page->checkProc = CheckBasic;
    page->applyProc = ApplyBasic;
    page->resetProc = ResetBasic;
    page->factoryProc = (void (*)()) 0;
}	/* End of InitBasicSheet () */

/* CreateBasic
 *
 * Create widgets for the basic property sheet.
 */
static void
CreateBasic (Widget parent, PropertyData *properties)
{
    Widget		prtType;
    Widget		bulletin;
    PropertyCntl	*controls;
    static Boolean	first = True;
    static char		*typeLbl;
    static char		*nameLbl;

    /* initialize all labels first time only */
    if (first)
    {
	struct passwd	*pw;

	first = False;

	typeLbl = GetStr (TXT_type);
	nameLbl = GetStr (TXT_name);

	SetButtonLbls (YesNoItems, XtNumber (YesNoItems));

	/* Get the uid and gid for lp */
	pw = getpwnam ("lp");
	if (!pw)
	    LpUid = LpGid = -1;
	else
	{
	    LpUid = pw->pw_uid;
	    LpGid = pw->pw_gid;
	}
    }

    controls = properties->controls;

    /* Use text field for printer name.  If an invalid character is typed,
     * a message is written in the window footer, so create the static text
     * field for that message.
     */
    MakeText (parent, nameLbl, &controls->prtNameCtrl, 14);
    if (properties->prtName)
	XtSetSensitive (XtParent (controls->prtNameCtrl.txt), False);

    XtAddCallback (controls->prtNameCtrl.txt, XtNmodifyVerification,
		   NameCheckCB, (XtPointer) XtParent (parent));

    /* Get the list of supported printers */
    prtType = XtVaCreateManagedWidget ("prtTypeCap", captionWidgetClass,
		parent,
		XtNlabel,	(XtArgVal) typeLbl,
		0);

    controls->listWidget = GetPrinterList (prtType, controls);

    /* Make controls for entering port for local printers or system name for
     * remote printers.  In order to do size the window correctly without
     * lots of ugly flashing, create the port controls in a bulletin board,
     * all located at the origin.  At most one of the sets of controls is
     * mapped at any time.
     */
    bulletin = XtVaCreateManagedWidget ("bulletin", bulletinBoardWidgetClass,
		parent,
		0);
    controls->mappedPort = NoPort;
    if (controls == &ChangeControls)
    {
	controls->ports [ParallelPort] =
	    CreateParallelControls (bulletin, controls);
	controls->ports [SerialPort] =
	    CreateSerialControls (bulletin, controls);
	controls->ports [RemotePort] =
	    CreateRemoteControls (bulletin, controls);
    }
    else
	controls->ports [properties->kind] =
	    (*CreatePortControls [properties->kind]) (bulletin, controls);

    /* Make sure all widgets visually reflect the correct values */
    controls->pageCreated |= 1<<Basic_Page;
    ResetBasic (parent, properties);

} /* End of CreateBasic () */

/* InitBasic
 *
 * Initialize properties structure.  If config is 0, then the properties
 * structure is initialized with default values for adding new printers.
 * The structure is assumed to have the kind of port connection already set.
 * If config is not 0, then the properties and connection type are extracted
 * from the configuration data.
 */
static void
InitBasic (Widget widget, PropertyData *properties, PRINTER *config)
{
    if (!config)
    {
	properties->prtName = (char *) 0;
	properties->printer = properties->newPrinter =
	    GetDefaultPrinter (widget);
	properties->allowRmt = No_Button;

	switch (properties->kind) {
	case ParallelPort:
	    properties->controls = &AddParControls;
	    properties->device.parallel.port = 0;
	    properties->device.parallel.miscDev = (char *) 0;
	    break;

	case SerialPort:
	    properties->controls = &AddSerControls;
	    properties->device.serial.port = 0;
	    properties->device.serial.miscDev = (char *) 0;
	    break;

	case RemotePort:
	    properties->controls = &AddRmtControls;
	    properties->device.remote.system = (char *) 0;
	    properties->device.remote.rmtName = (char *) 0;
	    properties->device.remote.os = Sys_V_Button;
	    break;
	}
    }
    else
    {
	properties->controls = &ChangeControls;
	properties->prtName = strdup (config->name);

	/* Make your best guess as to whether it's a parallel, serial, or
	 * remote printer.  The remote case is easy--the config->remote is
	 * non-zero.  For parallel and serial, make the non-trivial assumption
	 * that parallel printers only use one of the /dev/lp? devices.  If
	 * this guess is not correct, we will say treat a parallel printer
	 * as a serial printer, but this is not a crisis; the user will just
	 * have to set a lot of confusing options that will be ignored by
	 * lp.
	 */
	if (config->remote)
	{
	    char	*separator;
	    int		len;
	    SYSTEM	*system;

	    properties->allowRmt = Yes_Button;

	    /* remote name is in the form system[!name] */
	    properties->kind = RemotePort;
	    separator = strchr (config->remote, '!');
	    if (separator)
	    {
		len = separator - config->remote;
		properties->device.remote.system = XtMalloc (len + 1);
		strncpy (properties->device.remote.system,
			 config->remote, len);
		properties->device.remote.system [len] = 0;
		properties->device.remote.rmtName =
		    strdup (config->remote + len + 1);
	    }
	    else
	    {
		properties->device.remote.system = strdup (config->remote);
		properties->device.remote.rmtName = (char *) 0;
	    }
	    if (system = getsystem (properties->device.remote.system))
		properties->device.remote.os =
		    (system->protocol == S5_PROTO) ? Sys_V_Button : BSD_Button;
	}
	else
	{
	    register	itemIndx;

	    properties->allowRmt = GetRemoteAccess (config->name);
	    if (strncmp (config->device, "/dev/lp", 7) == 0)
	    {
		/* assumes last item in table is "other" */
		properties->kind = ParallelPort;
		for (itemIndx=0;
		     itemIndx<XtNumber(ParallelItems)-1;
		     itemIndx++)
		{
		    if (strcmp (config->device,
				(char *)ParallelItems[itemIndx].userData)==0)
			break;
		}
		properties->device.parallel.port = itemIndx;
		properties->device.parallel.miscDev = strdup (config->device);
	    }
	    else
	    {
		/* assumes last item in table is "other" */
		properties->kind = SerialPort;
		for (itemIndx=0; itemIndx<XtNumber(SerialItems)-1; itemIndx++)
		{
		    if (strcmp (config->device,
				(char *) SerialItems [itemIndx].userData)==0)
			break;
		}
		properties->device.serial.port = itemIndx;
		properties->device.serial.miscDev = strdup (config->device);
	    }
	}
    }
} /* End of InitBasic () */

/* CheckBasic
 *
 * Check validity of new property sheet values.  Return pointer to error
 * text if there is a problem, else NULL.
 */
static char *
CheckBasic (Widget page, PropertyData *properties)
{
    PropertyCntl	*controls = properties->controls;
    PRINTER		*config;
    Bool		rc;
    char		*newName;
    static char		*nameMsg;
    static char		*existMsg;
    static char		*rmtBlankMsg;
    static char		*rmtNameMsg;
    static char		*deviceMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;
	nameMsg = GetStr (TXT_badName);
	existMsg = GetStr (TXT_printerExists);
	rmtBlankMsg = GetStr (TXT_blankRemoteName);
	rmtNameMsg = GetStr (TXT_badRemoteName);
	deviceMsg = GetStr (TXT_nonexistent);
    }

    if (!(controls->pageCreated & 1<<Basic_Page))
    {
	/* This can only happen when updating a printer.  Since the page
	 * has never been brought up, we can assume that all basic
	 * values in the properties structure are to be left alone.
	 */
	return (NULL);
    }

    /* Check the new name */
    if (!properties->prtName)
    {
	/* new printer */
	newName = GetText (&controls->prtNameCtrl);
	if (strlen (newName) == 0 || strcmp (newName, "all") == 0)
	    return (nameMsg);

	config = getprinter (newName);

	if (config)
	{
	    FreeConfig (config, True);
	    return (existMsg);
	}
    }

    switch (properties->kind) {
    case RemotePort:
	/* Check remote system name */
	newName = GetText (&controls->remoteCtrls.systemCtrl);
	if (strlen (newName) == 0)
	    return (rmtBlankMsg);
	if (!AddRemote (newName, (int) OSItems [controls->
				   remoteCtrls.osCtrl.setIndx].userData))
	    return (rmtNameMsg);

	/* Get remote printer name.  Any name is legit. */
	newName = GetText (&controls->remoteCtrls.rmtNameCtrl);
	break;

    case ParallelPort:
	/* If the user has specified a device, make sure it exists.  Since
	 * the spooler runs privileged, it should be sufficient to check for
	 * simple existence.
	 */
	if (!ParallelItems [controls->parallelCtrls.portCtrl.setIndx].userData)
	{
	    char	*dev;

	    dev = GetText (&controls->parallelCtrls.miscDevCtrl);
	    if (access (dev, 0) != 0)
		return (deviceMsg);
	}
	break;

    case SerialPort:
	/* If the user has specified a device, make sure it exists */
	if (!SerialItems [controls->serialCtrls.portCtrl.setIndx].userData)
	{
	    char	*dev;

	    dev = GetText (&controls->serialCtrls.miscDevCtrl);
	    if (access (dev, 0) != 0)
		return (deviceMsg);
	}
	break;
    }

    return (NULL);
}	/* End of CheckBasic () */

/* ApplyBasic
 *
 * Update property sheet to reflect the changed values.
 */
static void
ApplyBasic (Widget page, PropertyData *properties)
{
    PropertyCntl	*controls = properties->controls;

    if (!(controls->pageCreated & 1<<Basic_Page))
	return;

    if (!properties->prtName)
	ApplyText (&controls->prtNameCtrl, &properties->prtName);

    properties->printer = properties->newPrinter;
    properties->allowRmt = properties->newAllowRmt;

    switch (properties->kind) {
    case ParallelPort:
	properties->device.parallel.port =
	    controls->parallelCtrls.portCtrl.setIndx;

	ApplyText (&controls->parallelCtrls.miscDevCtrl,
		   &properties->device.parallel.miscDev);
	break;

    case SerialPort:
	properties->device.serial.port =
	    controls->serialCtrls.portCtrl.setIndx;

	ApplyText (&controls->serialCtrls.miscDevCtrl,
		   &properties->device.serial.miscDev);
	break;

    case RemotePort:
	ApplyText (&controls->remoteCtrls.rmtNameCtrl,
		   &properties->device.remote.rmtName);
	ApplyText (&controls->remoteCtrls.systemCtrl,
		   &properties->device.remote.system);
	properties->device.remote.os = controls->remoteCtrls.osCtrl.setIndx;
	break;
    }
}	/* End of ApplyBasic () */

/* ResetBasic
 *
 * Reset property sheet to its original values
 */
static void
ResetBasic (Widget page, PropertyData *properties)
{
    PropertyCntl	*controls = properties->controls;

    if (!(controls->pageCreated & 1<<Basic_Page))
	return;

    /* Changing the printer type has implications on the stty settings.  If
     * the type changes, revert the stty values as well.  There are some
     * subtle user interface consequences of this.  For example, if the
     * user has changed the baud rate than changed the printer type, the
     * baud rate change will be lost.
     */
    properties->newPrinter = properties->printer;
    ChangePrinter (properties, properties->config);
    
    OlVaFlatSetValues (controls->listWidget, properties->printer->indx,
		XtNset,			(XtArgVal) True,
		0);
    XtVaSetValues (controls->listWidget, 
		XtNviewItemIndex,	(XtArgVal) properties->printer->indx,
		0);

    XtVaSetValues (controls->prtNameCtrl.txt,
		XtNstring,		(XtArgVal) properties->prtName,
		0);

    (*ResetPortControls [properties->kind])(properties);
    if (controls->mappedPort != properties->kind)
    {
	if (controls->mappedPort != NoPort)
	    XtSetMappedWhenManaged (controls->ports [controls->mappedPort],
				    False);
	XtSetMappedWhenManaged (controls->ports [properties->kind], True);
	controls->mappedPort = properties->kind;
    }
}	/* End of ResetBasic () */

/* NameCheckCB
 *
 * Printer Name Check callback.  As characters are types, check for white
 * space an non-printable characters.  Throw these out.  Also disallow
 * the wild-card characters * and ?.  Lp is not this restrictive, but it
 * is asking for trouble to allow these characters. call_data is a pointer
 * to OlTextModifyCallData.  client_data is the category widget to display
 * the message.
 */
static void
NameCheckCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    OlTextModifyCallDataPointer		pTextData;
    char	*cp;
    register	i;
    static	first = True;
    static char	*errorMsg;

    if (first)
    {
	first = False;

	errorMsg = GetStr (TXT_invalidChar);
    }

    pTextData = (OlTextModifyCallDataPointer) call_data;
    for (cp=pTextData->text, i=pTextData->text_length; --i>=0; cp++)
    {
	if (!isgraph (*cp) || *cp == '?' || *cp == '*')
	{
	    char	msg [256];
	    sprintf (msg, errorMsg, *cp);
	    _OlBeepDisplay (widget, 1);
	    FooterMsg ((Widget) client_data, msg);

	    pTextData->ok = False;
	    return;
	}
    }
}	/* End of NameCheckCB () */

/* CreateParallelControls
 *
 * Create widgets for parallel printer specific controls
 */
static Widget
CreateParallelControls (Widget parent, PropertyCntl *controls)
{
    Widget		cntlArea;
    ButtonItem		*allowItems;
    static Boolean	first = True;
    static char		*portLbl;
    static char		*otherLbl;
    static char		*allowRmtLbl;

    /* initialize all labels first time only */
    if (first)
    {
	first = False;

	portLbl = GetStr (TXT_port);
	otherLbl = GetStr (TXT_device);
	allowRmtLbl = GetStr (TXT_allowRemote);

	SetButtonLbls (ParallelItems, XtNumber (ParallelItems));
    }

    cntlArea = XtVaCreateManagedWidget ("cntlArea",
		controlAreaWidgetClass, parent,
		XtNmappedWhenManaged,		(XtArgVal) False,
		XtNallowChangeBars,		(XtArgVal) True,
		XtNalignCaptions,		(XtArgVal) True,
		XtNlayoutType,			(XtArgVal) OL_FIXEDCOLS,
		XtNshadowThickness,		(XtArgVal) 0,
		0);

    MakeButtons (cntlArea, portLbl, ParallelItems, XtNumber (ParallelItems),
		 &controls->parallelCtrls.portCtrl);
    XtVaSetValues (controls->parallelCtrls.portCtrl.btn,
		XtNselectProc,	(XtArgVal) ParallelButtonSelect,
		XtNclientData,	(XtArgVal) &controls->parallelCtrls,
		0);

    MakeText (cntlArea, otherLbl, &controls->parallelCtrls.miscDevCtrl, 25);
    XtSetMappedWhenManaged (XtParent (controls->parallelCtrls.miscDevCtrl.txt),
			    False);

    allowItems = (ButtonItem *) XtMalloc (sizeof (YesNoItems));
    memcpy (allowItems, YesNoItems, sizeof (YesNoItems));
    MakeNoneSetButtons (cntlArea, allowRmtLbl, allowItems,
			XtNumber (YesNoItems),
			&controls->parallelCtrls.allowRmtCtrl);
    return (cntlArea);
}	/* End of CreateParallelControls () */

/* CreateSerialControls
 *
 * Create widgets for serial printer specific controls.
 */
static Widget
CreateSerialControls (Widget parent, PropertyCntl *controls)
{
    Widget		cntlArea;
    ButtonItem		*allowItems;
    static Boolean	first = True;
    static char		*portLbl;
    static char		*otherLbl;
    static char		*allowRmtLbl;

    /* initialize all labels first time only */
    if (first)
    {
	first = False;

	portLbl = GetStr (TXT_port),
	otherLbl = GetStr (TXT_device);
	allowRmtLbl = GetStr (TXT_allowRemote);

	SetButtonLbls (SerialItems, XtNumber(SerialItems));
    }

    cntlArea = XtVaCreateManagedWidget ("cntlArea",
		controlAreaWidgetClass, parent,
		XtNmappedWhenManaged,		(XtArgVal) False,
		XtNallowChangeBars,		(XtArgVal) True,
		XtNalignCaptions,		(XtArgVal) True,
		XtNlayoutType,			(XtArgVal) OL_FIXEDCOLS,
		XtNshadowThickness,		(XtArgVal) 0,
		0);

    MakeButtons (cntlArea, portLbl, SerialItems, XtNumber (SerialItems),
		 &controls->serialCtrls.portCtrl);
    XtVaSetValues (controls->serialCtrls.portCtrl.btn,
		XtNselectProc,	(XtArgVal) SerialButtonSelect,
		XtNclientData,	(XtArgVal) &controls->serialCtrls,
		0);

    MakeText (cntlArea, otherLbl,  &controls->serialCtrls.miscDevCtrl, 25);
    XtSetMappedWhenManaged (XtParent (controls->serialCtrls.miscDevCtrl.txt),
			    False);

    allowItems = (ButtonItem *) XtMalloc (sizeof (YesNoItems));
    memcpy (allowItems, YesNoItems, sizeof (YesNoItems));
    MakeNoneSetButtons (cntlArea, allowRmtLbl, allowItems,
			XtNumber (YesNoItems),
			&controls->serialCtrls.allowRmtCtrl);
    return (cntlArea);
}	/* End of CreateSerialControls () */

/* CreateRemoteControls
 *
 * Create widgets for remote printer specific controls.
 */
static Widget
CreateRemoteControls (Widget parent, PropertyCntl *controls)
{
    Widget		cntlArea;
    static Boolean	first = True;
    static char		*systemLbl;
    static char		*rmtNameLbl;
    static char		*osLbl;

    /* initialize all labels first time only */
    if (first)
    {
	first = False;

	systemLbl = GetStr (TXT_system);
	rmtNameLbl = GetStr (TXT_rmtName);
	osLbl = GetStr (TXT_os);

	SetButtonLbls (OSItems, XtNumber(OSItems));
    }

    cntlArea = XtVaCreateManagedWidget ("cntlArea",
		controlAreaWidgetClass, parent,
		XtNmappedWhenManaged,		(XtArgVal) False,
		XtNallowChangeBars,		(XtArgVal) True,
		XtNalignCaptions,		(XtArgVal) True,
		XtNlayoutType,			(XtArgVal) OL_FIXEDCOLS,
		XtNshadowThickness,		(XtArgVal) 0,
		0);

    MakeText (cntlArea, systemLbl, &controls->remoteCtrls.systemCtrl, 8);
    MakeText (cntlArea, rmtNameLbl, &controls->remoteCtrls.rmtNameCtrl, 8);
    MakeButtons (cntlArea, osLbl, OSItems, XtNumber (OSItems),
		 &controls->remoteCtrls.osCtrl);
    return (cntlArea);
}	/* End of CreateRemoteControls () */

/* ParallelButtonSelect
 *
 * When a parallel ports select button is pressed, check to see if the user
 * is selecting the "other" button.  In this case, map the text field for
 * the other device.  If the "other" button is not selected, unmap the device
 * button.  client_data is a pointer to a ParallelCntl struct.
 */
static void
ParallelButtonSelect(Widget widget, XtPointer client_data, XtPointer call_data)
{
    ParallelCntl	*parallel = (ParallelCntl *) client_data;
    OlFlatCallData	*flatData = (OlFlatCallData *) call_data;
    Boolean		map;

    map = (((ButtonItem *)flatData->items)[flatData->item_index].userData==0);
    XtSetMappedWhenManaged (XtParent (parallel->miscDevCtrl.txt), map);

    ButtonSelectCB (widget, &parallel->portCtrl, call_data);
}	/* End of ParallelButtonSelect () */

/* SerialButtonSelect
 *
 * When a serial ports select button is pressed, check to see if the user
 * is selecting the "other" button.  In this case, map the text field for
 * the other device.  If the "other" button is not selected, unmap the device
 * button.  client_data is a pointer to a SerialCntl struct.
 */
static void
SerialButtonSelect(Widget widget, XtPointer client_data, XtPointer call_data)
{
    SerialCntl		*serial = (SerialCntl *) client_data;
    OlFlatCallData	*flatData = (OlFlatCallData *) call_data;
    Boolean		map;

    map = (((ButtonItem *)flatData->items)[flatData->item_index].userData==0);
    XtSetMappedWhenManaged (XtParent (serial->miscDevCtrl.txt), map);

    ButtonSelectCB (widget, &serial->portCtrl, call_data);
}	/* End of SerialButtonSelect () */

/* ResetParallel
 *
 * Reset parallel port widgets to their original values.
 */
static void
ResetParallel (PropertyData *properties)
{
    ParallelCntl	*parallelCtrls = &properties->controls->parallelCtrls;
    Boolean		map;

    parallelCtrls->portCtrl.setIndx = properties->device.parallel.port;
    OlVaFlatSetValues (parallelCtrls->portCtrl.btn,
		properties->device.parallel.port,
		XtNset,			(XtArgVal) True,
		0);

    XtVaSetValues (parallelCtrls->miscDevCtrl.txt,
		XtNstring,	(XtArgVal) properties->device.parallel.miscDev,
		0);
    map = (ParallelItems [properties->device.parallel.port].userData == 0);
    XtSetMappedWhenManaged (XtParent (parallelCtrls->miscDevCtrl.txt), map);

    ResetNoneSetButton (&parallelCtrls->allowRmtCtrl, properties->allowRmt);
}	/* End of ResetParallel () */

/* ResetSerial
 *
 * Reset serial port widgets to their original values.
 */
static void
ResetSerial (PropertyData *properties)
{
    SerialCntl	*serialCtrls = &properties->controls->serialCtrls;
    Boolean	map;

    serialCtrls->portCtrl.setIndx = properties->device.serial.port;
    OlVaFlatSetValues (serialCtrls->portCtrl.btn,
		properties->device.serial.port,
		XtNset,			(XtArgVal) True,
		0);

    XtVaSetValues (serialCtrls->miscDevCtrl.txt,
		XtNstring,	(XtArgVal) properties->device.serial.miscDev,
		0);
    map = (SerialItems [properties->device.serial.port].userData == 0);
    XtSetMappedWhenManaged (XtParent (serialCtrls->miscDevCtrl.txt), map);

    ResetNoneSetButton (&serialCtrls->allowRmtCtrl, properties->allowRmt);
}	/* End of ResetSerial () */

/* ResetRemote
 *
 * Reset remote port widgets to their original values.
 */
static void
ResetRemote (PropertyData *properties)
{
    RemoteCntl	*remoteCtrls = &properties->controls->remoteCtrls;

    XtVaSetValues (remoteCtrls->systemCtrl.txt,
		XtNstring,	(XtArgVal) properties->device.remote.system,
		0);

    XtVaSetValues (remoteCtrls->rmtNameCtrl.txt,
		XtNstring,	(XtArgVal) properties->device.remote.rmtName,
		0);

    remoteCtrls->osCtrl.setIndx = properties->device.remote.os;
    OlVaFlatSetValues (remoteCtrls->osCtrl.btn,
		properties->device.remote.os,
		XtNset,			(XtArgVal) True,
		0);
}	/* End of ResetRemote () */

/* UpdateBasic
 *
 * Populate a printer configuration structure with the basic attributes.
 */
static void
UpdateBasic (Widget page, PropertyData *properties, PRINTER *config)
{
    Printer		*newPrinter;
    PropertyCntl	*controls = properties->controls;

    if (!(controls->pageCreated & 1<<Basic_Page))
	return;

    newPrinter = properties->newPrinter;

    /* update values in the configuration from the property sheet */
    if (!properties->prtName)
    {
	ignprinter = 0;
	config->name = controls->prtNameCtrl.setText;

#if defined(CAN_DO_MODULES)
	if (newPrinter->modules)
	    config->modules = newPrinter->modules;
	else
	    config->modules = DfltList;
#endif

#if defined(SVR4_1ES)
	config->hilevel = PR_DEFAULT_HILEVEL;
	config->lolevel = PR_DEFAULT_LOLEVEL;
#endif
    }
    else
    {
	if (newPrinter != properties->printer)
	{
	    ignprinter = 0;
#if defined(CAN_DO_MODULES)
	    if (newPrinter->modules)
		config->modules = newPrinter->modules;
	    else
		config->modules = DfltList;
#endif
	}
	else
	    ignprinter = BAD_INTERFACE;
    }
    config->description = newPrinter->desc;
    config->input_types = newPrinter->contentTypes;
    config->printer_types = newPrinter->terminfo;

    if (properties->kind != RemotePort)
    {
	config->interface = newPrinter->interface;

	if (properties->kind == ParallelPort)
	{
	    config->device = (char *)
		ParallelItems [controls->parallelCtrls.portCtrl.setIndx].
		    userData;
	    if (!config->device)
		config->device = controls->parallelCtrls.miscDevCtrl.setText;

	    properties->newAllowRmt =
		controls->parallelCtrls.allowRmtCtrl.setIndx;
	}
	else
	{
	    config->device = (char *)
		SerialItems [controls->serialCtrls.portCtrl.setIndx].userData;
	    if (!config->device)
		config->device = controls->serialCtrls.miscDevCtrl.setText;

	    properties->newAllowRmt =
		controls->serialCtrls.allowRmtCtrl.setIndx;
	}

	/* The device should be owned by lp and be readable and writable only
	 * by lp.  The exception is if the device is /dev/null.  In this case,
	 * leave the permissions alone.  Ignore errors.
	 */
	if (strcmp (config->device, "/dev/null") != 0)
	{
	    if (chown (config->device, LpUid, LpGid) == 0)
		(void) chmod (config->device, S_IRUSR | S_IWUSR);
	}
    }
    else
    {
	char	*sys;
	char	*name;

	properties->newAllowRmt = properties->allowRmt;

	/* Create a name of the form "system!name", but be careful if name
	 * is zero length--that is, remove the '!'.
	 */
	sys = controls->remoteCtrls.systemCtrl.setText;
	name = controls->remoteCtrls.rmtNameCtrl.setText;

	XtFree (config->remote);
	if (strlen (name) == 0)
	    config->remote = strdup (sys);
	else
	{
	    config->remote = XtMalloc (strlen (sys) + strlen (name) + 2);
	    strcpy (config->remote, sys);
	    strcat (config->remote, "!");
	    strcat (config->remote, name);
	}
    }
} /* End of UpdateBasic () */

/* GetRemoteAccess
 *
 * Determine if remote users can print on local printers.  Returns Yes_Button
 * or No_Button if user access was can be determined; returns OL_NO_ITEM if
 * the administrator has set user permissions to anything else.
 */
static Cardinal
GetRemoteAccess (char *name)
{
    char		**allowList;
    char		**denyList;
    Cardinal		rc;

    load_userprinter_access (name, &allowList, &denyList);

    /* Everybody is allowed to use the printer if the allow list contains
     * "all!all".  Only local users are permitted if the list contains "all".
     * Anything else indicates the the administrator has tailored the list
     * outside of the graphical interface.
     */
    rc = OL_NO_ITEM;
    if (allowList)
    {
	/* As of p11, the allowList should never be empty if it exists.
	 * Nonetheless, for safety's sake, check anyway.
	 */
	if (*allowList)
	{
	    if (strcmp (*allowList, "all!all") == 0)
		rc = Yes_Button;
	    else if (strcmp (*allowList, "all") == 0)
		rc = No_Button;
	}
    }

    if (allowList)
	freelist (allowList);
    if (denyList)
	freelist (denyList);

    return (rc);
}	/* End of GetRemoteAccess () */
