/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/extended.c	1.11"
#endif

#include <stdio.h>
#include <ctype.h>
#include <memory.h>
#include <string.h>
#include <search.h>
#include <sys/types.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/ControlAre.h>
#include <Xol/FButtons.h>
#include <Xol/TextField.h>
#include <Xol/TextEdit.h>
#include <Xol/StaticText.h>
#include <Xol/Caption.h>

#include <lp.h>
#include <printers.h>

#include "properties.h"
#include "printer.h"
#include "lpsys.h"
#include "error.h"

#define TEXTSIZE	20

enum {
    Mail_button, No_fault_button,
};

enum {
    M_cstop, M_parenb, M_parodd,
    B1200, B19200, B2400, B300, B4800, B9600,
    Cs7, Cs8, Cstop, Parenb, Parodd,
};

enum {
    B300_button, B1200_button, B2400_button, B4800_button,
    B9600_button, B19200_button,
};

enum {
    Always_button, Opt_button,
};

enum {
    Even_button, Odd_button, None_button,
};

enum {
    One_button, Two_button,
};

enum {
    Eight_button, Seven_button,
};

static void 	CreateConfiguration (Widget, PropertyData *);
static void 	CreateCommunication (Widget, PropertyData *);
static void	InitConfiguration (Widget, PropertyData *, PRINTER *);
static void	InitCommunication (Widget, PropertyData *, PRINTER *);
static void	UpdateConfiguration (Widget, PropertyData *, PRINTER *);
static void	UpdateCommunication (Widget, PropertyData *, PRINTER *);
static char	*CheckConfiguration (Widget, PropertyData *);
static char	*CheckCommunication (Widget, PropertyData *);
static void	ApplyConfiguration (Widget, PropertyData *);
static void	ApplyCommunication (Widget, PropertyData *);
static void	ResetConfiguration (Widget, PropertyData *);
static void	ResetCommunication (Widget, PropertyData *);

static char	*Tokens [] = {
    "-cstopb", "-parenb", "-parodd",
    "1200", "19200", "2400", "300", "4800", "9600",
    "cs7", "cs8", "cstopb", "parenb", "parodd",
};

static ButtonItem	BannerItems [] = {
    { (XtArgVal) BAN_ALWAYS, (XtArgVal) TXT_required, },	/* Required */
    { (XtArgVal) 0, (XtArgVal) TXT_optional, },			/* Optional */
};

static ButtonItem	AlertItems [] = {
    { (XtArgVal) "mail", (XtArgVal) TXT_yes, },			/* Mail */
    { (XtArgVal) "none", (XtArgVal) TXT_no, },			/* None */
};

static ButtonItem	BaudItems [] = {
    { (XtArgVal) "300 ", (XtArgVal) TXT_b300, },		/* 300 */
    { (XtArgVal) "1200 ", (XtArgVal) TXT_b1200, },		/* 1200 */
    { (XtArgVal) "2400 ", (XtArgVal) TXT_b2400, },		/* 2400 */
    { (XtArgVal) "4800 ", (XtArgVal) TXT_b4800, },		/* 4800 */
    { (XtArgVal) "9600 ", (XtArgVal) TXT_b9600, },		/* 9600 */
    { (XtArgVal) "19200 ", (XtArgVal) TXT_b19200, },		/* 19200 */
};

static ButtonItem	ParityItems [] = {
    { (XtArgVal) "-parodd parenb ", (XtArgVal) TXT_even, },	/* Even */
    { (XtArgVal) "parodd parenb ", (XtArgVal) TXT_odd, },	/* Odd */
    { (XtArgVal) "-parenb ", (XtArgVal) TXT_none, },		/* None */
};

static ButtonItem	StopBitItems [] = {
    { (XtArgVal) "-cstopb ", (XtArgVal) TXT_one, },		/* 1 */
    { (XtArgVal) "cstopb ", (XtArgVal) TXT_two, },		/* 2 */
};

static ButtonItem	CharSizeItems [] = {
    { (XtArgVal) "cs8 ", (XtArgVal) TXT_eight, },		/* 8 */
    { (XtArgVal) "cs7 ", (XtArgVal) TXT_seven, },		/* 7 */
};

/* InitConfigurationSheet
 *
 * Initial configuration property sheet functions.
 */
void
InitConfigurationSheet (Widget widget, CategoryPage *page)
{
    page->lbl = GetStr (TXT_configuration),
    page->createProc = CreateConfiguration;
    page->initProc = InitConfiguration;
    page->updateProc = UpdateConfiguration;
    page->checkProc = CheckConfiguration;
    page->applyProc = ApplyConfiguration;
    page->resetProc = ResetConfiguration;
    page->factoryProc = (void (*)()) 0;
}	/* End of InitConfigurationSheet () */

/* CreateConfiguration
 *
 * Create widgets for the configuration property sheet.
 */
static void
CreateConfiguration (Widget parent, PropertyData *properties)
{
    PropertyCntl	*controls = properties->controls;
    static Boolean	first = True;
    static char		*bannerLbl;
    static char		*alerterLbl;
    static char		*charPitchLbl;
    static char		*linePitchLbl;
    static char		*pageWidthLbl;
    static char		*pageLengthLbl;

    /* initialize all labels first time only */
    if (first)
    {
	first = False;

	bannerLbl = GetStr (TXT_skipBanner);
	alerterLbl = GetStr (TXT_alerter);
	charPitchLbl = GetStr (TXT_charPitch);
	linePitchLbl = GetStr (TXT_linePitch);
	pageWidthLbl = GetStr (TXT_pageWidth);
	pageLengthLbl = GetStr (TXT_pageLength);

	SetButtonLbls (BannerItems, XtNumber(BannerItems));
	SetButtonLbls (AlertItems, XtNumber(AlertItems));
    }

    MakeButtons (parent, bannerLbl, BannerItems, XtNumber (BannerItems),
		 &controls->bannerCtrl);

    MakeButtons (parent, alerterLbl, AlertItems, XtNumber (AlertItems),
		 &controls->alertCtrl);

    MakeScaled (parent, pageLengthLbl, &controls->pgLenCtrl);
    MakeScaled (parent, pageWidthLbl, &controls->pgWidCtrl);
    MakeScaled (parent, charPitchLbl, &controls->cpiCtrl);
    MakeScaled (parent, linePitchLbl, &controls->lpiCtrl);

    /* Make sure all widgets visually reflect the correct values */
    controls->pageCreated |= 1<<Config_Page;
    ResetConfiguration (parent, properties);
}	/* End of CreateConfiguration () */

/* InitConfiguration
 *
 * Initialize configuration properties
 */
static void
InitConfiguration (Widget page, PropertyData *properties, PRINTER *config)
{
    if (!config)
    {
	properties->banner = Always_button;
	properties->alert = 0;
	properties->cpi.val = 0;
	properties->cpi.sc = 0;
	properties->lpi.val = 0;
	properties->lpi.sc = 0;
	properties->pgLen.val = 0;
	properties->pgLen.sc = 0;
	properties->pgWid.val = 0;
	properties->pgWid.sc = 0;
    }
    else
    {
	properties->banner = (config->banner & BAN_ALWAYS) ?
	    Always_button : Opt_button;

	/* This only works if the alert type is mail or none.  We should
	 * have an "other" button, but that will have to keep for later.
	 * Unfortunately, this means that an apply of this property sheet
	 * is destructive.
	 */
	properties->alert = (strncmp(config->fault_alert.shcmd,"mail",4)==0) ?
	    Mail_button : No_fault_button;

	properties->pgLen = config->plen;
	properties->pgWid = config->pwid;
	properties->cpi = config->cpi;
	properties->lpi = config->lpi;
    }
}	/* End of InitConfiguration () */

/* UpdateConfiguration
 *
 * Populate a printer configuration structure.
 */
static void
UpdateConfiguration (Widget page, PropertyData *properties, PRINTER *config)
{
    PropertyCntl	*controls = properties->controls;

    if (!(controls->pageCreated & 1<<Config_Page))
	return;

    /* update values in the configuration from the property sheet */
    if (properties->kind != RemotePort)
    {
	config->banner = (ushort)
	    BannerItems [controls->bannerCtrl.setIndx].userData;

	/* If the fault type is "Mail", this could possibly change to whom the
	 * mail is sent.  Not the best idea, but for the moment....
	 */
	if (controls->alertCtrl.setIndx == Mail_button)
	{
	    char	buf [32];

	    sprintf (buf, "mail %s", getname ());
	    config->fault_alert.shcmd = strdup (buf);
	}
	else
	    config->fault_alert.shcmd = strdup ("none");

	/* Not really applying yet */
	ApplyScaled(&controls->cpiCtrl, &config->cpi.val, &config->cpi.sc);
	ApplyScaled(&controls->lpiCtrl, &config->lpi.val, &config->lpi.sc);
	ApplyScaled(&controls->pgLenCtrl, &config->plen.val, &config->plen.sc);
	ApplyScaled(&controls->pgWidCtrl, &config->pwid.val, &config->pwid.sc);
    }
}	/* End of UpdateConfiguration () */

/* CheckConfiguration
 *
 * Check validity of new property sheet values.
 */
static char *
CheckConfiguration (Widget page, PropertyData *properties)
{
    PropertyCntl	*controls = properties->controls;
    static char		*pgLenMsg;
    static char		*pgWidMsg;
    static char		*cpiMsg;
    static char		*lpiMsg;
    static Boolean	first = True;

    if (first)
    {
	first = False;
	pgLenMsg = GetStr (TXT_badPgLen);
	pgWidMsg = GetStr (TXT_badPgWid);
	cpiMsg = GetStr (TXT_badCpi);
	lpiMsg = GetStr (TXT_badLpi);
    }

    if (!(controls->pageCreated & 1<<Config_Page))
	return (NULL);

    /* Check the scaled numbers for validity */
    if (!CheckScaled (&controls->pgLenCtrl))
	return (pgLenMsg);
    if (!CheckScaled (&controls->pgWidCtrl))
	return (pgWidMsg);
    if (!CheckScaled (&controls->cpiCtrl))
	return (cpiMsg);
    if (!CheckScaled (&controls->lpiCtrl))
	return (lpiMsg);

    return (NULL);
}	/* End of CheckConfiguration () */

/* ApplyConfiguration
 *
 * Update property sheet to reflect the changed values.
 */
static void
ApplyConfiguration (Widget page, PropertyData *properties)
{
    PropertyCntl	*controls = properties->controls;

    if (!(controls->pageCreated & 1<<Config_Page))
	return;

    properties->banner = controls->bannerCtrl.setIndx;
    properties->alert = controls->alertCtrl.setIndx;
    ApplyScaled(&controls->cpiCtrl, &properties->cpi.val, &properties->cpi.sc);
    ApplyScaled(&controls->lpiCtrl, &properties->lpi.val, &properties->lpi.sc);
    ApplyScaled(&controls->pgLenCtrl, &properties->pgLen.val,
		&properties->pgLen.sc);
    ApplyScaled(&controls->pgWidCtrl, &properties->pgWid.val,
		&properties->pgWid.sc);
}	/* End of ApplyConfiguration () */

/* ResetConfiguration
 *
 * Reset property sheet to its original values
 */
static void
ResetConfiguration (Widget page, PropertyData *properties)
{
    PropertyCntl	*controls = properties->controls;

    if (!(controls->pageCreated & 1<<Config_Page))
	return;

    controls->bannerCtrl.setIndx = properties->banner;
    OlVaFlatSetValues (controls->bannerCtrl.btn, properties->banner,
		XtNset,			(XtArgVal) True,
		0);

    controls->alertCtrl.setIndx = properties->alert;
    OlVaFlatSetValues (controls->alertCtrl.btn, properties->alert,
		XtNset,			(XtArgVal) True,
		0);

    ResetScaled (&controls->pgLenCtrl, properties->pgLen.val,
	   properties->pgLen.sc);
    ResetScaled (&controls->pgWidCtrl, properties->pgWid.val,
	   properties->pgWid.sc);
    ResetScaled (&controls->cpiCtrl, properties->cpi.val,
	   properties->cpi.sc);
    ResetScaled (&controls->lpiCtrl, properties->lpi.val,
	   properties->lpi.sc);
}	/* End of ResetConfiguration () */

/* InitCommunicationSheet
 *
 * Initial communication property sheet functions.
 */
void
InitCommunicationSheet (Widget widget, CategoryPage *page)
{
    page->lbl = GetStr (TXT_communication),
    page->createProc = CreateCommunication;
    page->initProc = InitCommunication;
    page->updateProc = UpdateCommunication;
    page->checkProc = CheckCommunication;
    page->applyProc = ApplyCommunication;
    page->resetProc = ResetCommunication;
    page->factoryProc = (void (*)()) 0;
}	/* End of InitCommunicationSheet () */

/* CreateCommunication
 *
 * Create widgets for the configuration property sheet.
 */
static void
CreateCommunication (Widget parent, PropertyData *properties)
{
    SerialCntl		*serialCtrls;
    static Boolean	first = True;
    static char		*baudLbl;
    static char		*parityLbl;
    static char		*stopBitsLbl;
    static char		*charSizeLbl;

    /* initialize all labels first time only */
    if (first)
    {
	first = False;

	baudLbl = GetStr (TXT_baud);
	parityLbl = GetStr (TXT_parity);
	stopBitsLbl = GetStr (TXT_stopBits);
	charSizeLbl = GetStr (TXT_charSize);

	SetButtonLbls (BaudItems, XtNumber (BaudItems));
	SetButtonLbls (ParityItems, XtNumber (ParityItems));
	SetButtonLbls (StopBitItems, XtNumber (StopBitItems));
	SetButtonLbls (CharSizeItems, XtNumber (CharSizeItems));
    }

    serialCtrls = &properties->controls->serialCtrls;
    MakeButtons (parent, baudLbl, BaudItems, XtNumber (BaudItems),
		 &serialCtrls->baudRateCtrl);

    MakeButtons (parent, parityLbl, ParityItems, XtNumber (ParityItems),
		 &serialCtrls->parityCtrl);

    MakeButtons (parent, stopBitsLbl, StopBitItems,
		 XtNumber (StopBitItems), &serialCtrls->stopBitsCtrl);

    MakeButtons (parent, charSizeLbl, CharSizeItems,
		 XtNumber (CharSizeItems), &serialCtrls->charSizeCtrl);

    /* Make sure all widgets visually reflect the correct values */
    properties->controls->pageCreated |= 1<<Comm_Page;
    ResetCommunication (parent, properties);
}	/* End of CreateCommunication () */

/* InitCommunication
 *
 * Initialize communications properties.  One oddity:  if we are modifying
 * an existing printer, preserve the stty settings that are controllable
 * by the user (baud, parity, etc.) if config is NULL.  Summarizing the
 * four cases:  1) new printer, config=0 -- using all default values.
 * 2) new printer, config!=0 -- can't happen.  3) existing printer,
 * config=0 -- use default values except for baud, etc.--leave these
 * alone.  4) existing printer, config!=0 -- use values entirely from config.
 */
static void
InitCommunication (Widget page, PropertyData *properties, PRINTER *config)
{
    SerialData		*serial = &properties->device.serial;
    Cardinal		baudRate;
    Cardinal		stopBits;
    Cardinal		charSize;
    Boolean		parenb;
    Boolean		odd;
    char		*stty;
    char		*token;

    if (properties->stty)
	XtFree (properties->stty);
    properties->stty = (char *) 0;

    if (config)
	stty = strdup (config->stty);
    else
    {
	if (properties->newPrinter->stty)
	    stty = strdup (properties->newPrinter->stty);
	else
	    stty = strdup ("");
    }

    /* Most Communications values are only relevant to serial ports */
    if (properties->kind != SerialPort)
    {
	properties->stty = (char *) stty;
	return;
    }

    if (!properties->prtName || config)
    {
	baudRate = B9600_button;
	stopBits = One_button;
	charSize = Eight_button;
	parenb = False;
	odd = True;
    }

    /* Parse the stty string to extract the relevant values. */
    properties->stty = XtMalloc (strlen (stty));
    properties->stty [0] = 0;
    for (token=strtok(stty, " "); token; token=strtok(NULL, " "))
    {
	switch (Lookup (token)) {
	case Parenb:
	    parenb = True;
	    break;
	case M_parenb:
	    parenb = False;
	    break;
	case Parodd:
	    odd = True;
	    break;
	case M_parodd:
	    odd = False;
	    break;
	case Cstop:
	    stopBits = Two_button;
	    break;
	case M_cstop:
	    stopBits = One_button;
	    break;
	case Cs8:
	    charSize = Eight_button;
	    break;
	case Cs7:
	    charSize = Seven_button;
	    break;
	case B300:
	    baudRate = B300_button;
	    break;
	case B1200:
	    baudRate = B1200_button;
	    break;
	case B2400:
	    baudRate = B2400_button;
	    break;
	case B4800:
	    baudRate = B4800_button;
	    break;
	case B9600:
	    baudRate = B9600_button;
	    break;
	case B19200:
	    baudRate = B19200_button;
	    break;
	default:
	    strcat (properties->stty, " ");
	    strcat (properties->stty, token);
	    break;
	}
    }

    if (!properties->prtName || config)
    {
	serial->stopBits = stopBits;
	serial->charSize = charSize;
	serial->baudRate = baudRate;
	if (!parenb)
	    serial->parity = None_button;
	else
	    serial->parity = odd ? Odd_button : Even_button;
    }
    
    XtFree (stty);
}	/* End of InitCommunication () */

/* UpdateCommunication
 *
 * Populate a printer configuration structure.
 */
static void
UpdateCommunication (Widget page, PropertyData *properties, PRINTER *config)
{
    char	*sttyStr;

    if (!(properties->controls->pageCreated & 1<<Comm_Page) ||
	properties->kind != SerialPort)
    {
	config->stty = properties->stty ? strdup (properties->stty) : 0;
	return;
    }

    /* update values in the configuration from the property sheet */
    sttyStr = XtMalloc (40 + strlen (properties->stty));
    sttyStr [0] = 0;
    if (properties->kind == SerialPort)
    {
	SerialCntl	*serialCtrls = &properties->controls->serialCtrls;
	
	strcat (sttyStr, (char *)
		BaudItems [serialCtrls->baudRateCtrl.setIndx].userData);
	strcat (sttyStr, (char *)
		ParityItems [serialCtrls->parityCtrl.setIndx].userData);
	strcat (sttyStr, (char *)
		StopBitItems [serialCtrls->stopBitsCtrl.setIndx].userData);
	strcat (sttyStr, (char *)
		CharSizeItems[serialCtrls->charSizeCtrl.setIndx].userData);
    }
    strcat (sttyStr, properties->stty);
    XtFree (config->stty);
    config->stty = sttyStr;
}	/* End of UpdateCommunication () */

/* CheckCommunication
 *
 * Check validity of new property sheet values.
 */
static char *
CheckCommunication (Widget page, PropertyData *properties)
{
    /* As all the controls are button controls, there is nothing to check! */
    return (NULL);
}	/* End of CheckCommunication () */

/* ApplyCommunication
 *
 * Update property sheet to reflect the changed values.
 */
static void
ApplyCommunication (Widget page, PropertyData *properties)
{
    SerialCntl	*serialCtrls = &properties->controls->serialCtrls;
    SerialData	*serial = &properties->device.serial;

    if (!(properties->controls->pageCreated & 1<<Comm_Page) ||
	properties->kind != SerialPort)
	return;

    serial->baudRate = serialCtrls->baudRateCtrl.setIndx;
    serial->parity = serialCtrls->parityCtrl.setIndx;
    serial->charSize = serialCtrls->charSizeCtrl.setIndx;
    serial->stopBits = serialCtrls->stopBitsCtrl.setIndx;
}	/* End of ApplyCommunication () */

/* ResetCommunication
 *
 * Reset property sheet to its original values
 */
static void
ResetCommunication (Widget page, PropertyData *properties)
{
    SerialCntl	*serialCtrls = &properties->controls->serialCtrls;
    SerialData	*serial = &properties->device.serial;

    if (!(properties->controls->pageCreated & 1<<Comm_Page) ||
	properties->kind != SerialPort)
	return;

    serialCtrls->baudRateCtrl.setIndx = serial->baudRate;
    OlVaFlatSetValues (serialCtrls->baudRateCtrl.btn, serial->baudRate,
		XtNset,			(XtArgVal) True,
		0);

    serialCtrls->parityCtrl.setIndx = serial->parity;
    OlVaFlatSetValues (serialCtrls->parityCtrl.btn, serial->parity,
		XtNset,			(XtArgVal) True,
		0);

    serialCtrls->charSizeCtrl.setIndx = serial->charSize;
    OlVaFlatSetValues (serialCtrls->charSizeCtrl.btn, serial->charSize,
		XtNset,			(XtArgVal) True,
		0);

    serialCtrls->stopBitsCtrl.setIndx = serial->stopBits;
    OlVaFlatSetValues (serialCtrls->stopBitsCtrl.btn, serial->stopBits,
		XtNset,			(XtArgVal) True,
		0);

}	/* End of ResetCommunication () */

/* ChangePrinter
 *
 * When the user changes the printer type, the stty string can potentially
 * change as well.  Update the properties based on the new printer
 * configuration and make sure relevant widgets are updated.  If config
 * is zero, use default values; otherwise, use values from config.
 */
void
ChangePrinter (PropertyData *properties, PRINTER *config)
{
    Widget	sheet = properties->controls->sheets [Comm_Page];

    InitCommunication (sheet, properties, config);
}	/* End of ChangePrinter () */

/* SttyCmp
 *
 * Comparison function for stty tokens.
 */
static int
SttyCmp (const void *k1, const void *k2)
{
    return (strcmp (*(char **)k1, *(char **)k2));
}	/* End of SttyCmp () */

/* Lookup
 *
 * Lookup a token in the list of know stty values.  Return the index into
 * the Tokens table, -1 if not found.
 */
static int
Lookup (char *word)
{
    char	**s;

    s = bsearch (&word, Tokens, XtNumber (Tokens), sizeof (char *), SttyCmp);
    if (s)
	return (s - Tokens);
    else
	return (-1);

}	/* End of Lookup () */
