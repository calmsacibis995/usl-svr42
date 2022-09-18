/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtadmin:print/setup/printer.c	1.18"
#endif

/* Read the file of built-in printers.  The file consists of zero or
 * more printer descriptions separated by blank lines or comments.  Each
 * description is one or more lines of the form:
 *
 *		keyword: value
 *
 * Keywords are:
 *	entry -- entry name (ignored here, used by ISV commands)
 *	name -- name of printer to appear in list
 *	terminfo -- comma or space separated list of terminfo names
 *	contents -- comma or space separated list of content types
 *	interface -- path of interface program.  If the path is not absolute,
 *			it is relative to the model directory of lp.
 *	stty -- list of stty args to be used by default for serial printers.
 *	modules -- list of streams modules needed for the printer.
 *
 * String values (e.g. name) are not enclosed in quotes.  Leading and
 * trailing white space is stripped automatically.  Keywords in the input
 * file are converted to lower case before comparison.  The file may include
 * comments; comments are introduced by a # and extend to the end of line.
 * A keyword can be listed only once per printer; if it is included more than
 * once, subsequent values are ignored.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <search.h>
#include <sys/types.h>
#include <memory.h>

#include <Intrinsic.h>
#include <StringDefs.h>
#include <Xol/OpenLook.h>

#include <Xol/FList.h>
#include <Xol/ScrolledWi.h>
#include <DtI.h>

#include <lp.h>
#include <printers.h>

#include "properties.h"
#include "printer.h"
#include "error.h"

#define WHITESPACE	" \t\n"
#define PRINTER_ALLOC_SIZE	10

typedef void (*pfv)();

typedef struct {
    XtArgVal	pName;
    XtArgVal	printer;
} ListItem;

typedef struct {
    char	*key;
    pfv		proc;
} KeyWord;

static void PrinterSelectCB (Widget, XtPointer, XtPointer);

static void GetName (ListItem *, char *);
static void GetTerminfo (ListItem *, char *);
static void GetContent (ListItem *, char *);
static void GetInterface (ListItem *, char *);
static void GetModules (ListItem *, char *);
static void GetStty (ListItem *, char *);

static void LayoutIcons (IconItem *, int, int, int);

static char **GetList (char *);
static pfv Lookup (char *);
static int ItemCmp (const void *, const void *);
static int KeywordCmp (const void *, const void *);
static void RemoveSpaces (char *);
static Boolean ErrorChk (ListItem *);

DmGlyphPtr	PrtGlyph;
DmGlyphPtr	DfltPrtGlyph;

static KeyWord	keywords [] = {
    { "contents", GetContent, },
    { "entry", 0 },	/* Entry name is ignored here. */
    { "interface", GetInterface, },
    { "modules", GetModules, },
    { "name", GetName, },
    { "stty", GetStty, },
    { "terminfo", GetTerminfo, },
};

static String	listFields [] = {
    XtNformatData, XtNuserData,
};

static ListItem *ListItems;
static unsigned	ItemCnt;

ResourceRec AppResources;
static XtResource	Resources [] = {
    { "printerFile", "PrinterFile", XtRString, sizeof (String),
	  XtOffset (ResourcesPtr, file),  XtRString,
	  "desktop/PrintMgr/Printers", },
    { "printerIcon", "PrinterIcon", XtRString, sizeof (String),
	  XtOffset (ResourcesPtr, icon), XtRString,
	  "printer.icon", },
    { "dfltPrtIcon", "DfltPrinterIcon", XtRString, sizeof (String),
	  XtOffset (ResourcesPtr, dfltIcon), XtRString,
	  "dfltprt.icon", },
    { "defaultPrinter", "DefaultPrinter", XtRString, sizeof (String),
	  XtOffset (ResourcesPtr, dfltPrt), XtRString,
	  NULL, },
    { "administrator", "Administrator", XtRBoolean, sizeof (Boolean),
	  XtOffset (ResourcesPtr, administrator), XtRImmediate,
	  (XtPointer) False, },
};

Widget
GetPrinterList (Widget parent, PropertyCntl *controls)
{
    Widget		list;
    Widget		scrolledWindow;

    /* Create Scrolling List to display supported printers.  The list
     * should be a child of a scrolled window.
     */

    scrolledWindow = XtVaCreateManagedWidget ("scrolledWindow",
		scrolledWindowWidgetClass, parent,
		0);

    list = XtVaCreateManagedWidget ("printerList", flatListWidgetClass,
		scrolledWindow,
		XtNviewHeight,		(XtArgVal) 5,
		XtNexclusives,		(XtArgVal) True,
		XtNnoneSet,		(XtArgVal) False,
		XtNselectProc,		(XtArgVal) PrinterSelectCB,
		XtNclientData,		(XtArgVal) controls,
		XtNitems,		(XtArgVal) ListItems,
		XtNnumItems,		(XtArgVal) ItemCnt,
		XtNitemFields,		(XtArgVal) listFields,
		XtNnumItemFields,	(XtArgVal) XtNumber (listFields),
		XtNformat,		(XtArgVal) "%s",
		0);

    return (list);
} /* End of GetPrinterList () */

/* PrinterSelectCB
 *
 * Printer Select callback.  When the selected printer changes, this can
 * change the stty settings, which, in turn, can change things like baud
 * rate, usw.  Therefore, we must reinitialize the communications settings.
 */
static void
PrinterSelectCB (Widget widget, XtPointer client_data, XtPointer call_data)
{
    PropertyCntl	*controls = (PropertyCntl *) client_data;
    OlFlatCallData	*pFlatData = (OlFlatCallData *) call_data;
    Printer		*newPrinter;

    newPrinter = (Printer *) ((ListItem *) pFlatData->items +
			      pFlatData->item_index)->printer;

    if (newPrinter != controls->owner->newPrinter)
    {
	controls->owner->newPrinter = newPrinter;
	ChangePrinter (controls->owner, (PRINTER *) 0);
    }
} /* End of PrinterSelectCB () */

/* Get Default Printer */
Printer *
GetDefaultPrinter (Widget widget)
{
    return ((Printer *) ListItems [0].printer);
} /* End of GetDefaultPrinter () */

/* GetName
 *
 * Get the printer name to appear in the list
 */
static void
GetName (ListItem *item, char *value)
{
    char	*desc;
    char	*caret;
    extern char	*gettxt (char *, char *);

    if (item->pName)
	return;

    caret = strchr (value, '^');
    if (caret)
    {
	*caret = 0;
	desc = gettxt (caret+1, value);
	if (desc == value)
	    desc = strdup (value);
    }
    else
	desc = strdup (value);

    ((Printer *) item->printer)->desc = desc;
    item->pName = (XtArgVal) &((Printer *) item->printer)->desc;
}	/* End of GetName () */

/* GetTerminfo
 *
 * Get terminfo list--a comma or space separated list of terminfo names
 */
static void
GetTerminfo (ListItem *item, char *value)
{
    Printer	*printer = (Printer *) item->printer;

    if (!printer->terminfo)
	printer->terminfo = GetList (value);
} /* End of GetTerminfo () */

/* GetContent
 *
 * Get Content type list--a comma or space separated list of input types.
 * Allow a null list to be created by specifying the "content:" keyword with
 * no list.
 */
static void
GetContent (ListItem *item, char *value)
{
    Printer	*printer = (Printer *) item->printer;

    if (!printer->contentTypes)
	printer->contentTypes = GetList (value);
} /* End of GetContent () */

/* GetInterface
 *
 * Get the interface program name.  Paths are relative to the lp model
 * directory.  Characters after the path name are ignored.
 */
static void
GetInterface (ListItem *item, char *value)
{
    Printer	*printer = (Printer *) item->printer;

    if (!printer->interface)
    {
	value = strtok (value, WHITESPACE);
	if (*value == '/')
	    printer->interface = strdup (value);
	else
	{
	    printer->interface = (char *) XtMalloc (strlen (Lp_Model) +
						    strlen (value) + 2);
	    strcpy (printer->interface, Lp_Model);
	    strcat (printer->interface, "/");
	    strcat (printer->interface, value);
	}
    }
} /* End of GetInterface () */

/* GetModules
 *
 * Get modules list--a comma or space separated list of streams modules names.
 */
static void
GetModules (ListItem *item, char *value)
{
    Printer	*printer = (Printer *) item->printer;

    if (!printer->modules)
	printer->modules = GetList (value);
} /* End of GetModules () */

/* GetStty
 *
 * Get stty setting--this is a string of space separated values (but not made
 * into a list).  No error checking is done here.
 */
static void
GetStty (ListItem *item, char *value)
{
    Printer	*printer = (Printer *) item->printer;

    if (!printer->stty)
	printer->stty = strdup (value);
} /* End of GetStty () */

/* InitSupportedPrinters
 *
 * Read the list of supported printers for a file.
 */
void
InitSupportedPrinters (Widget widget)
{
    FILE	*printerFile;
    ListItem	*newItem;
    char	*keyword;
    char	*value;
    unsigned	allocated;
    void	(*proc)();
    char	*XwinHome;
    char	buf [256];
    register	i;

    XtGetApplicationResources (widget, &AppResources,
			       Resources, XtNumber (Resources), NULL, 0);

    ListItems = (ListItem *) 0;
    ItemCnt = 0;

    if (AppResources.file [0] != '/')
    {
	char	*dir;

	XwinHome = getenv ("XWINHOME");
	if (!XwinHome)
	    XwinHome = "/usr/X";

	dir = XtMalloc (strlen (XwinHome) + 1 + strlen(AppResources.file) + 1);
	sprintf (dir, "%s/%s", XwinHome, AppResources.file);
	AppResources.file = strdup (dir);
    }

    if (!(printerFile = fopen (AppResources.file, "r")))
    {
	if (!(printerFile = fopen ("/usr/X/desktop/PrintMgr/Printers", "r")))
	{
	    Printer	*newPrinter;
	    static char	*desc;

	    /* Create a default entry. */
	    newPrinter = (Printer *) XtCalloc (1, sizeof (*newPrinter));
	    ListItems = (ListItem *) XtMalloc (sizeof (*ListItems));
	    ItemCnt = 1;

	    desc = GetStr (TXT_otherDesc);
	    ListItems [0].pName = (XtArgVal) &desc;
	    ListItems [0].printer = (XtArgVal) newPrinter;
	    (void) ErrorChk (ListItems);

	    return;
	}
    }

    newItem = (ListItem *) 0;
    ItemCnt = allocated = 0;
    while (fgets (buf, 256, printerFile))
    {
	/* remove comments and leading white space and trailing newline */
	keyword = strchr (buf, '#');
	if (keyword)
	    *keyword = 0;
	keyword = buf + strlen (buf) - 1;
	if (keyword >= buf && *keyword == '\n')
	    *keyword = 0;
	keyword = buf + strspn (buf, WHITESPACE);

	/* process the keyword */
	value = strchr (keyword, ':');
	if (value)
	{
	    *value++ = 0;
	    proc = Lookup (keyword);

	    /* If proc is NULL, either the keyword is ignored here, or there
	     * is an error in the input file that we'll just quietly ignore.
	     */
	    if (proc)
	    {
		if (!newItem)
		{
		    Printer	*newPrinter;

		    newPrinter = (Printer *) XtCalloc (1, sizeof(*newPrinter));

		    if (ItemCnt >= allocated)
		    {
			allocated += PRINTER_ALLOC_SIZE;
			ListItems = (ListItem *) XtRealloc ((char *) ListItems,
			       allocated * sizeof (*ListItems));
		    }
		    newItem = &ListItems [ItemCnt++];

		    newItem->pName = (XtArgVal) 0;
		    newItem->printer = (XtArgVal) newPrinter;
		}

		(*proc) (newItem, value + strspn (value, WHITESPACE));
	    }
	}
	else
	{
	    /* Line is either blank or is invalid (just ignore the error).
	     * Check previous printer for errors.
	     */
	    if (newItem && !ErrorChk (newItem))
	    {
		/* Error found--remove the item */
		XtFree ((char *) newItem->printer);
		ItemCnt--;
	    }
	    newItem = (ListItem *) 0;
	}
    }

    /* End of file.  Check the printer for errors. */
    if (newItem && !ErrorChk (newItem))
    {
	/* Error found--remove the item */
	XtFree ((char *) newItem->printer);
	ItemCnt--;
    }
    fclose (printerFile);

    /* Sort the printer list */
    qsort ((char *) ListItems, ItemCnt, sizeof (*ListItems), ItemCmp);
    for (i=0; i<ItemCnt; i++)
	((Printer *) ListItems [i].printer)->indx = i;
}	/* End of InitSupportedPrinters () */

/* RemoveSpaces
 *
 * Remove trailing white space from a string.
 */
static void
RemoveSpaces (char *str)
{
    int		strLen;
    int		spaceCnt;

    /* Leading space is already assumed to be gone. */
    strLen = strcspn (str, WHITESPACE);
    spaceCnt = 0;
    while (str [strLen])
    {
	spaceCnt = strspn (str + strLen, WHITESPACE);
	strLen += spaceCnt;
	strLen += strcspn (str + strLen, WHITESPACE);
    }
    str [strLen - spaceCnt] = 0;
} /* End of RemoveSpaces () */

/* GetList
 *
 * Create an array of pointers to chars to each comma or space separated
 * item in a string.
 */
static char **
GetList (char *str)
{
    char	**list;
    char	*tmpList [20];
    int		cnt = 0;

    str = strtok (str, " ,");
    while (str)
    {
	tmpList [cnt++] = strdup (str);
	str = strtok (NULL, " ,");
    }
    tmpList [cnt++] = (char *) 0;

    list = (char **) XtMalloc (cnt * sizeof (*list));
    (void) memcpy (list, tmpList, cnt * sizeof (*list));
    return (list);
} /* End of GetList () */

/* ErrorChk
 *
 * Check an printer item for missing required fields.  Return True if Ok.
 */
static Boolean
ErrorChk (ListItem *item)
{
    Printer	*printer;

    if (!item->pName)
	return False;
    printer = (Printer *) item->printer;
    GetInterface (item, "standard");
    GetContent (item, "simple");
    GetTerminfo (item, "unknown");
    return True;
} /* End of ErrorChk () */

/* Comparison function for Printer List sorter */
static int
ItemCmp (const void *i1, const void *i2)
{
    return (strcoll (*(char **) ((ListItem *)i1)->pName,
		     *(char **) ((ListItem *)i2)->pName));
} /* End of ItemCmp () */

/* Comparison function for keyword list */
static int
KeywordCmp (const void *k1, const void *k2)
{
    return (strcmp (((KeyWord *)k1)->key, ((KeyWord *)k2)->key));
} /* End of KeywordCmp () */

/* Lookup
 *
 * Use a binary search to find a keyword.  Return the procedure used to
 * process that word.
 */
static pfv
Lookup (char *word)
{
    char	*pc;
    KeyWord	*keyWord;
    KeyWord	searchWord;

    /* Convert to lower case. */
    for (pc = word; *pc; pc++)
	*pc = tolower (*pc);

    searchWord.key = word;
    keyWord = (KeyWord *) bsearch (&searchWord, keywords,
				   sizeof (keywords) / sizeof (*keywords),
				   sizeof (searchWord), KeywordCmp);
    return (keyWord ? keyWord->proc : (pfv) 0);
} /* End of Lookup () */

/* GetActivePrinters
 *
 * Create the list of flat items for the currently defined printers.
 */
void
GetActivePrinters (Widget parent, IconItem **pItems, int *pItemCnt,
		   Dimension width, Dimension height)
{
    IconItem		*items;
    IconItem		*newItem;
    ListItem		*supportedItem;
    PropertyData	*properties;
    PRINTER		*config;
    char		*desc;
    int			cnt;
    int			allocated;
    register		i;

    PrtGlyph = DmGetPixmap (XtScreen (parent), AppResources.icon);
    if (!PrtGlyph)
	PrtGlyph = DmGetPixmap (XtScreen (parent), NULL);
    DfltPrtGlyph = DmGetPixmap (XtScreen (parent), AppResources.dfltIcon);
    if (!DfltPrtGlyph)
	DfltPrtGlyph = DmGetPixmap (XtScreen (parent), NULL);

    cnt = allocated = 0;
    items = (IconItem *) 0;
    while (config = getprinter ("all"))
    {
	if (cnt >= allocated)
	{
	    allocated += PRINTER_ALLOC_SIZE;
	    items = (IconItem *) XtRealloc ((char *) items,
					    allocated * sizeof (*items));
	}
	newItem = &items [cnt++];

	newItem->lbl = (XtArgVal) strdup (config->name);
	newItem->selected = (XtArgVal) False;
	newItem->glyph = (XtArgVal)
	    ((strcmp (config->name, AppResources.dfltPrt) == 0) ?
		DfltPrtGlyph :  PrtGlyph);

	/* Try to match the printer with one of the known printers in the
	 * database.  This matching uses just the description of the printer,
	 * so it is possible for this to get the wrong printer if someone
	 * has been administering printers manually.
	 */
	desc = (config->description) ? config->description :
	    *config->printer_types;
	supportedItem = ListItems;
	for (i=ItemCnt; --i>=0; supportedItem++)
	{
	    if (!strcmp (desc, ((Printer *) supportedItem->printer)->desc))
		break;
	}

	if (i < 0)
	{
	    Printer	*newPrinter;

	    /* No match was found.  Create a new printer type. */
	    newPrinter = (Printer *) XtCalloc (1, sizeof (*newPrinter));
	    newPrinter->indx = ItemCnt;
	    newPrinter->desc = strdup (desc);
	    newPrinter->terminfo = config->printer_types;
	    newPrinter->contentTypes = config->input_types;
	    newPrinter->interface = config->interface;

	    ListItems = (ListItem *) XtRealloc ((char *) ListItems,
			       ++ItemCnt * sizeof (*ListItems));
	    supportedItem = &ListItems [ItemCnt - 1];

	    supportedItem->pName = (XtArgVal) &newPrinter->desc;
	    supportedItem->printer = (XtArgVal) newPrinter;
	}

	/* Create a new properties structure for the printer. */
	newItem->properties = (XtArgVal) XtCalloc (1, sizeof (PropertyData));
	properties = (PropertyData *) newItem->properties;
	properties->config = (PRINTER *) XtMalloc (sizeof (PRINTER));
	*properties->config = *config;
	InitProperties (parent, properties, config);
	properties->printer = properties->newPrinter =
	    (Printer *) supportedItem->printer;
    }

    LayoutIcons (items, cnt, width, height);

    *pItems = items;
    *pItemCnt = cnt;
}	/* End of GetActivePrinters () */

/* AddPrinter
 *
 * Add a new printer to the icon box.
 */
void
AddPrinter (PropertyData *properties)
{
    IconItem		*items;
    IconItem		*newItem;
    Cardinal		cnt;
    Dimension		width, height;
    extern Widget	IconBox;

    /* Allocate space for the new printer */
    XtVaGetValues (IconBox,
		XtNitems,		(XtArgVal) &items,
		XtNnumItems,		(XtArgVal) &cnt,
		XtNwidth,		(XtArgVal) &width,
		XtNheight,		(XtArgVal) &height,
		0);
    items = (IconItem *) XtRealloc ((char *) items, ++cnt * sizeof(*items));
    newItem = items + cnt - 1;

    newItem->lbl = (XtArgVal) strdup (properties->prtName);
    newItem->glyph = (XtArgVal) PrtGlyph;
    newItem->selected = (XtArgVal) False;
    newItem->properties = (XtArgVal) properties;

    LayoutIcons (items, cnt, width, height);
    XtVaSetValues (IconBox,
		XtNitems,		(XtArgVal) items,
		XtNnumItems,		(XtArgVal) cnt,
		XtNitemsTouched,	(XtArgVal) True,
		0);
}	/* End of AddPrinter () */

/* DelPrinters
 *
 * Delete printers from the icon box.  Printers to be deleted are marked by
 * having a null properties structure.  Move printers from the end of the
 * items list up to fill the gap.
 */
void
DelPrinters (void)
{
    IconItem		*items;
    Cardinal		cnt;
    Cardinal		selectedCnt;
    Boolean		isSerial;
    register Cardinal	last;
    register Cardinal	i;
    extern Widget	IconBox;
    extern void		SetButtonState (Cardinal, Boolean);

    XtVaGetValues (IconBox,
		XtNitems,		(XtArgVal) &items,
		XtNnumItems,		(XtArgVal) &cnt,
		0);

    /* While we're removing the dead wood, note how many items are selected. */
    selectedCnt = 0;
    last = 0;
    for (i=0; i<cnt; i++)
    {
	if (items [i].properties)
	{
	    if (items [i].selected)
	    {
		selectedCnt++;
		isSerial = ((PropertyData *) items [i].properties)->kind ==
		    SerialPort;
	    }
	    if (last != i)
		items [last] = items [i];
	    last++;
	}
	else
	    XtFree ((char *) items [i].lbl);
    }

    if (last != cnt)
    {
	XtVaSetValues (IconBox,
		XtNitems,		(XtArgVal) items,
		XtNnumItems,		(XtArgVal) last,
		XtNitemsTouched,	(XtArgVal) True,
		0);
	SetButtonState (selectedCnt, isSerial);
    }
}	/* End of DelPrinters () */

/* LayoutIcons
 *
 * Layout the items in an icon box in an orderly fashion.  Put as many items
 * on a row as will fit.  Height is ignored.
 */
static void
LayoutIcons (IconItem *items, int itemCnt, int width, int height)
{
    IconItem	*current;
    int		x, y;
    register	i;

    x = y = 0;
    for (current=items, i=itemCnt; --i>=0; current++)
    {
	if (x >= width)
	{
	    x = 0;
	    y += BOGUS_HEIGHT;
	}
	current->x = (XtArgVal) x;
	current->y = (XtArgVal) y;
	current->width = (XtArgVal) BOGUS_WIDTH;
	current->height = (XtArgVal) BOGUS_HEIGHT;
	x += BOGUS_WIDTH;
    }
}	/* End of LayoutIcons () */

/* IsAdmin
 *
 * Determine if the user has administrative permissions.
 */
Boolean
IsAdmin (void)
{
    static Boolean	first = True;
    static Boolean	admin;

    if (first)
    {
	first = False;
	admin = (AppResources.administrator) ? True : is_user_admin ();
    }

    return (admin);
}	/* End of IsAdmin () */
