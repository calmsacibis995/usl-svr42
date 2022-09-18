/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:olwsm/mouse.c	1.54"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLookP.h>
#include <Xol/StaticText.h>
#include <Xol/ControlAre.h>
#include <Xol/AbbrevButt.h>
#include <Xol/Caption.h>
#include <Xol/Category.h>
#include <Xol/ChangeBar.h>
#include <Xol/MenuShell.h>
#include <Xol/FButtons.h>

#include "error.h"
#include <misc.h>
#include <node.h>
#include <list.h>
#include <menu.h>
#include <exclusive.h>
#include <nonexclu.h>
#include <property.h>
#include <resource.h>
#include <wsm.h>

extern char *			strtok();
extern NonexclusiveItem *       NonexclusiveResourceItem();

static void			Import();
static void			Export();
static void			Create();
static ApplyReturn *		ApplyCB();
static void			ResetCB();
static void			FactoryCB();

static void			change_mod();
static char *			modifier();
static char *			button();
static void			set_mod_btns(Nonexclusive *, char *);
static void			PingChangeBar(Exclusive * exclusive);
static void			SwitchNumMouseButtons(Exclusive * exc);
static void			InitNumBtnsDependentStuff(void);

static Arg			mouse_args[] = {
	{XtNlayoutType,		OL_FIXEDCOLS},
 /*	{XtNcenter,		True}, */
	{XtNvSpace,		6},
	{XtNsameSize,		OL_NONE},
};

static OlDtHelpInfo MseModHelp = {
	NULL, NULL, "DesktopMgr/moupref.hlp", NULL, NULL
};

Property			mouseProperty = {
	"Mouse Modifiers",
	mouse_args,
	XtNumber (mouse_args),
	&MseModHelp,
	'\0',
	Import,
	Export,
	Create,
	ApplyCB,
	ResetCB,
	FactoryCB,
	0,
	0,
	0,
	0,
};

/* Define indexes for each control. */
static enum {
    SELECT_INDEX, ADJUST_INDEX, MENU_INDEX, PASTE_INDEX,/* Primaries */
    TOGGLE_INDEX, EXTEND_INDEX,				/* modified SELECT */
    DUPLICATE_INDEX, LINK_INDEX, CONSTRAIN_INDEX,	/* modified SELECT */
    CUT_INDEX, COPY_INDEX, PAN_INDEX,			/* modified PASTE */
    MENUDEF_INDEX,					/* modified MENU */
    MAX_ENTRY };

/* The num-btns control gets tacked onto the end of the resources lists
   (_current, _factory).  Note that when addressing _resource_buf,
   NUM_BTNS_INDEX is divided by 2 (See GLOBAL() macro usage in Export.
*/
#define NUM_BTNS_INDEX		MAX_ENTRY*2

#define DEFAULT_NUM_MOUSE_BTNS	2
#define MOD(i)			(i)
#define BUT(i)			( (i) + MAX_ENTRY )
#define MAX_BUF			80
#define IS_3BTN_MOUSE		( _factory == &_factory_3btn )
#define IS_PRIMARY(i)		( (i) < 4 )

#define FACTORY(x)		*(ADDR *)&((*_factory)[x].value)
#define CURRENT(x)		*(ADDR *)&(_current[x].value)
#define GLOBAL(x) resource_value(&global_resources, _resource_buf[x].name)

static char		_two[] = "2";
static char		_three[] = "3";

static char		_none[] = "";
static char		_shift[] = "Shift";
static char		_ctrl[] = "Ctrl";
static char		_mod1[] = MOD1;

static char		_button1[] = "<Button1>";
static char		_button2[] = "<Button2>";
static char		_button3[] = "<Button3>";

static char		_select[] = "<selectBtn>";
static char		_adjust[] = "<adjustBtn>";
static char		_menu[] = "<menuBtn>";
static char		_paste[] = "<pasteBtn>";

static Resource			_factory_2btn[] = {
	{ "SELECT_MOD",		_none },
	{ "ADJUST_MOD",		_ctrl },	/* Same as Toggle */
	{ "MENU_MOD",		"Ctrl" " Shift" },	/* FIX THIS */
	{ "PASTE_MOD",		_none },
	{ "TOGGLE_MOD",		_ctrl },
	{ "EXTEND_MOD",		_shift },
	{ "DUPLICATE_MOD",	_mod1 },
	{ "LINK_MOD",		MOD1 " Ctrl" },		/* FIX THIS */
	{ "CONSTRAIN_MOD",	MOD1 " Shift" },	/* FIX THIS */
	{ "CUT_MOD",		_mod1 },
	{ "COPY_MOD",		_ctrl },
	{ "PAN_MOD",		_shift },
	{ "MENUDEF_MOD",	MOD1 " Ctrl Shift" }, /* FIX THIS */
	{ "SELECT_BUT",		_button1 },	/* Buttons begin here */
	{ "ADJUST_BUT",		_button1 },	/* (really _select) */
	{ "MENU_BUT",		_button1 },	/* (really _select) */
	{ "PASTE_BUT",		_button3 },	/* Button 3: REALLY! */
	{ "TOGGLE_BUT",		_select },
	{ "EXTEND_BUT",		_select },
	{ "DUPLICATE_BUT",	_select },
	{ "LINK_BUT",		_select },
	{ "CONSTRAIN_BUT",	_select },
	{ "CUT_BUT",		_paste },
	{ "COPY_BUT",		_paste },
	{ "PAN_BUT",		_paste },
	{ "MENUDEF_BUT",	_menu },
#if DEFAULT_NUM_MOUSE_BTNS == 2
	{ "NUM_BTNS",		_two },		/* tack it on the end */
#else
	{ "NUM_BTNS",		_three },	/* tack it on the end */
#endif
};

static Resource			_factory_3btn[] = {
	{ "SELECT_MOD",		_none },
	{ "ADJUST_MOD",		_ctrl },		/* Same as Toggle */
	{ "MENU_MOD",		_none },
	{ "PASTE_MOD",		_none },
	{ "TOGGLE_MOD",		_ctrl },
	{ "EXTEND_MOD",		_shift },
	{ "DUPLICATE_MOD",	_mod1 },
	{ "LINK_MOD",		MOD1 " Ctrl" },		/* FIX THIS */
	{ "CONSTRAIN_MOD",	MOD1 " Shift" },	/* FIX THIS */
	{ "CUT_MOD",		_mod1 },
	{ "COPY_MOD",		_ctrl },
	{ "PAN_MOD",		_shift },
	{ "MENUDEF_MOD",	_mod1 },
	{ "SELECT_BUT",		_button1 },	/* Buttons begin here */
	{ "ADJUST_BUT",		_button1 },	/* (really _select) */
	{ "MENU_BUT",		_button3 },
	{ "PASTE_BUT",		_button2 },
	{ "TOGGLE_BUT",		_select },
	{ "EXTEND_BUT",		_select },
	{ "DUPLICATE_BUT",	_select },
	{ "LINK_BUT",		_select },
	{ "CONSTRAIN_BUT",	_select },
	{ "CUT_BUT",		_paste },
	{ "COPY_BUT",		_paste },
	{ "PAN_BUT",		_paste },
	{ "MENUDEF_BUT",	_menu },
#if DEFAULT_NUM_MOUSE_BTNS == 2
	{ "NUM_BTNS",		_two },		/* tack it on the end */
#else
	{ "NUM_BTNS",		_three },	/* tack it on the end */
#endif
};

static Resource			_current[] = {
	{ "SELECT_MOD",		NULL },
	{ "ADJUST_MOD",		NULL },		/* Same as Toggle */
	{ "MENU_MOD",		NULL },
	{ "PASTE_MOD",		NULL },
	{ "TOGGLE_MOD",		NULL },
	{ "EXTEND_MOD",		NULL },
	{ "DUPLICATE_MOD",	NULL },
	{ "LINK_MOD",		NULL },
	{ "CONSTRAIN_MOD",	NULL },
	{ "CUT_MOD",		NULL },
	{ "COPY_MOD",		NULL },
	{ "PAN_MOD",		NULL },
	{ "MENUDEF_MOD",	NULL },
	{ "SELECT_BUT",		NULL },		/* Buttons begin here */
	{ "ADJUST_BUT",		NULL },		/* Same as Toggle */
	{ "MENU_BUT",		NULL },
	{ "PASTE_BUT",		NULL },
	{ "TOGGLE_BUT",		NULL },
	{ "EXTEND_BUT",		NULL },
	{ "DUPLICATE_BUT",	NULL },
	{ "LINK_BUT",		NULL },
	{ "CONSTRAIN_BUT",	NULL },
	{ "CUT_BUT",		NULL },
	{ "COPY_BUT",		NULL },
	{ "PAN_BUT",		NULL },
	{ "MENUDEF_BUT",	NULL },
	{ "NUM_BTNS",		NULL },		/* tack it on the end */
};

static char buffer[MAX_BUF + 1];
static char buf0[MAX_BUF];
static char buf1[MAX_BUF];
static char buf2[MAX_BUF];
static char buf3[MAX_BUF];
static char buf4[MAX_BUF];
static char buf5[MAX_BUF];
static char buf6[MAX_BUF];
static char buf7[MAX_BUF];
static char buf8[MAX_BUF];
static char buf9[MAX_BUF];
static char buf10[MAX_BUF];
static char buf11[MAX_BUF];
static char buf12[MAX_BUF];

static Resource			_resource_buf[] = {
  { "*selectBtn",		buf0 },
  { "*adjustBtn",		buf1 },		/* Same as Toggle */
  { "*menuBtn",			buf2 },
  { "*pasteBtn",		buf3 },
  { "*toggleBtn",		buf4 },
  { "*extendBtn",		buf5 },
  { "*duplicateBtn",		buf6 },
  { "*linkBtn",			buf7 },
  { "*constrainBtn",		buf8 },
  { "*cutBtn",			buf9 },
  { "*copyBtn",			buf10 },
  { "*panBtn",			buf11 },
  { "*menuDefaultBtn",		buf12 },
#if DEFAULT_NUM_MOUSE_BTNS == 2
  { "*numMouseBtns",		_two },		/* tack it on the end */
#else
  { "*numMouseBtns",		_three },	/* tack it on the end */
#endif
};
static List		resource_buf = LIST(Resource, _resource_buf);

/* Define "Number of Buttons" exclusive setting (top control) */
static ExclusiveItem _num_btns[] = {
  { (XtArgVal)"Two",	(XtArgVal)_two		},
  { (XtArgVal)"Three",	(XtArgVal)_three	},
};
static List num_btns = LIST(ExclusiveItem, _num_btns);
static Exclusive NumBtns =
  { True, "num_buttons", "Number of Mouse Buttons:",
	NULL, NULL, &num_btns, SwitchNumMouseButtons, NULL, NULL, False };

#define ALLMODS(name) \
	static NonexclusiveItem	OlConcat(_,name)[] = \
	{ \
		{ (XtArgVal)"(None)", (XtArgVal)_none }, \
		{ (XtArgVal)"Shift", (XtArgVal)_shift }, \
		{ (XtArgVal)"Ctrl", (XtArgVal)_ctrl }, \
		{ (XtArgVal) MOD1, (XtArgVal)_mod1 }, \
	}; \
	static List	name = LIST(NonexclusiveItem, OlConcat(_,name));

#define SOMEMODS(name) \
	static NonexclusiveItem	OlConcat(_,name)[] = \
	{ \
		{ (XtArgVal)"Shift", (XtArgVal)_shift }, \
		{ (XtArgVal)"Ctrl", (XtArgVal)_ctrl }, \
		{ (XtArgVal) MOD1, (XtArgVal)_mod1 }, \
	}; \
	static List	name = LIST(NonexclusiveItem, OlConcat(_,name))

SOMEMODS(modifier0);
SOMEMODS(modifier1);
SOMEMODS(modifier2);
SOMEMODS(modifier3);
SOMEMODS(modifier4);
SOMEMODS(modifier5);
SOMEMODS(modifier6);
SOMEMODS(modifier7);
SOMEMODS(modifier8);
SOMEMODS(modifier9);
SOMEMODS(modifier10);
SOMEMODS(modifier11);
SOMEMODS(modifier12);

#define NONEXCLU_MOD(lab, def, items) \
	{ False, lab, NULL, NULL, def, items, change_mod }

static Nonexclusive			mods[] = {
    NONEXCLU_MOD("SELECT",		_modifier0 + 0, &modifier0),
    NONEXCLU_MOD("ADJUST",		_modifier1 + 1, &modifier1),
    NONEXCLU_MOD("MENU",		_modifier2 + 0, &modifier2),
    NONEXCLU_MOD("PASTE",		_modifier3 + 0, &modifier3),
    NONEXCLU_MOD("Toggle",		_modifier4 + 1, &modifier4),
    NONEXCLU_MOD("Extend",		_modifier5 + 0, &modifier5),
    NONEXCLU_MOD("Duplicate",		_modifier6 + 2, &modifier6),
    NONEXCLU_MOD("Link",		_modifier7 + 1, &modifier7),
    NONEXCLU_MOD("Constrain",		_modifier8 + 0, &modifier8),
    NONEXCLU_MOD("Cut",			_modifier9 + 2, &modifier9),
    NONEXCLU_MOD("Copy",		_modifier10 + 1, &modifier10),
    NONEXCLU_MOD("Scroll by Panning",	_modifier11 + 0, &modifier11),
    NONEXCLU_MOD("Set Menu Default",	_modifier12 + 2, &modifier12),
};

/* Define buttons for PRIMARY mouse controls */

#define LEFT	{ (XtArgVal)"L", (XtArgVal)_button1 }
#define MIDDLE	{ (XtArgVal)"M", (XtArgVal)_button2 }
#define RIGHT	{ (XtArgVal)"R", (XtArgVal)_button3 }
static ExclusiveItem _two_btns[][2] = {
  { LEFT, RIGHT },
  { LEFT, RIGHT },
  { LEFT, RIGHT },
  { LEFT, RIGHT },
};
static List two_btns[] = {
    LIST(NonexclusiveItem, _two_btns[0]),
    LIST(NonexclusiveItem, _two_btns[1]),
    LIST(NonexclusiveItem, _two_btns[2]),
    LIST(NonexclusiveItem, _two_btns[3]),
};

static ExclusiveItem _three_btns[][3] = {
  { LEFT, MIDDLE, RIGHT },
  { LEFT, MIDDLE, RIGHT },
  { LEFT, MIDDLE, RIGHT },
  { LEFT, MIDDLE, RIGHT },
};
static List three_btns[] = {
    LIST(NonexclusiveItem, _three_btns[0]),
    LIST(NonexclusiveItem, _three_btns[1]),
    LIST(NonexclusiveItem, _three_btns[2]),
    LIST(NonexclusiveItem, _three_btns[3]),
};
#undef LEFT
#undef MIDDLE
#undef RIGHT

#if DEFAULT_NUM_MOUSE_BTNS == 2
#define BTN_ITEMS(num) &two_btns[num]
#else
#define BTN_ITEMS(num) &three_btns[num]
#endif
#define EXCL_BTN(name, num) \
  { True, name, name, NULL, NULL, BTN_ITEMS(num), \
	PingChangeBar, (ADDR)(num), NULL, False }

static Exclusive			btns[] = {
    EXCL_BTN("SELECT:",	0),
    EXCL_BTN("ADJUST:",	1),
    EXCL_BTN("MENU:",	2),
    EXCL_BTN("PASTE:",	3),
};

static Widget			caps[MAX_ENTRY];
static Resource			(*_factory)[] =
#if DEFAULT_NUM_MOUSE_BTNS == 2
	&_factory_2btn;
#else
	&_factory_3btn;
#endif

#undef ALLMODS
#undef SOMEMODS
#undef BTN_ITEMS
#undef EXCL_BTN


static void
ResetCB(w, closure)
	Widget			w;
	XtPointer		closure;
{
    int			i;

    trace("ResetCB");

    /* Don't reset num-btns control */

    for (i = 0; i < MAX_ENTRY; ++i)
    {
	if (IS_PRIMARY(i))
	    SetExclusive(&btns[i],
			 ResourceItem(&btns[i], CURRENT(BUT(i))), OL_NONE);

	UnsetAllNonexclusiveItems(&mods[i]);
	ReadSavedItems(&mods[i]);
	change_mod(&mods[i]);
	XtVaSetValues (caps[i], XtNchangeBar, (XtArgVal)OL_NONE, (String)0);
    }
}

static void
FactoryCB(w, closure)
	Widget			w;
	XtPointer		closure;
{
    int			i;

    trace("FactoryCB");

    /* Don't reset num-btns control */

    for (i = 0; i < MAX_ENTRY; ++i)
    {
	if (IS_PRIMARY(i))
	    SetExclusive(&btns[i],
			 ResourceItem(&btns[i], FACTORY(BUT(i))), OL_NORMAL);

	/* Unset all the modifier-menu items, then set them according to the
	   Factory setting and set corresponding label.
	*/
	UnsetAllNonexclusiveItems(&mods[i]);
	set_mod_btns(&mods[i], FACTORY(MOD(i)));
	change_mod(&mods[i]);
	XtVaSetValues(caps[i], XtNchangeBar, (XtArgVal)OL_NORMAL, (String)0);
    }
}

static ApplyReturn *
ApplyCB(w, closure)
	Widget			w;
	XtPointer		closure;
{
    int			i, j;
    char *		msg = NULL;
    static ApplyReturn	ret;
    Display *		dpy = XtDisplay(w);

    for (i = 0; (i < MAX_ENTRY - 1) && (msg == NULL); i++)
	/* Check for non-distinct mouse button assignments */
	if (IS_PRIMARY(i+1))
	{
	    for (j = i+1; IS_PRIMARY(j); ++j)
		if ((btns[i].current_item->addr ==
		     btns[j].current_item->addr) &&
		    MATCH(mods[i].current_label, mods[j].current_label))
		{
		    msg = OLG(distinct,errorMsg);
		    break;
		}

	} else if (!IS_PRIMARY(i))
	{
	    /* Check for modifier "collision" */
	    if ((MATCH(CURRENT(BUT(i)), _select) &&
		 MATCH(mods[SELECT_INDEX].current_label,
		       mods[i].current_label)) ||
		(MATCH(CURRENT(BUT(i)), _adjust) &&
		 MATCH(mods[ADJUST_INDEX].current_label,
		       mods[i].current_label)) ||
		(MATCH(CURRENT(BUT(i)), _menu) &&
		 MATCH(mods[MENU_INDEX].current_label,
		       mods[i].current_label)) ||
		(MATCH(CURRENT(BUT(i)), _paste) &&
		 MATCH(mods[PASTE_INDEX].current_label,
		       mods[i].current_label)))
	    {
		msg = OLG(dupModifier,errorMsg);


	    } else
		for (j = i + 1; j < MAX_ENTRY; j++)
		    if (MATCH(CURRENT(BUT(i)), CURRENT(BUT(j))) &&
			MATCH(mods[i].current_label, mods[j].current_label))
		    {
			msg = OLG(dupModifier,errorMsg);
			break;
		    }
	}

    if (msg != NULL)
    {
	ret.reason = APPLY_ERROR;
	ret.u.message = msg;
	return (&ret);
    }

    /* Things are verified, so update resources */

    /* num-btns control doesn't need sprintf processing */
    _resource_buf[NUM_BTNS_INDEX/2].value =
	CURRENT(NUM_BTNS_INDEX) = (ADDR)NumBtns.current_item->addr;
    _OlSetChangeBarState(NumBtns.w, OL_NONE, OL_PROPAGATE);


    for (i = 0; i < MAX_ENTRY; ++i)
    {
	if (IS_PRIMARY(i))
	{
	    CURRENT(BUT(i)) = (ADDR)btns[i].current_item->addr;
	}
	CURRENT(MOD(i)) = (ADDR)mods[i].current_label;

	debug((stderr, "CURRENT(MOD(i)) = %s\n", CURRENT(MOD(i))));

	if (MATCH(mods[i].current_label, OLG(none,fixedString)))
	{
	    CURRENT(MOD(i)) = NULL;
	    (void)sprintf(_resource_buf[i].value, "%s", CURRENT(BUT(i)));
	} else
	{
	    (void)sprintf(_resource_buf[i].value,
			  "%s%s", CURRENT(MOD(i)), CURRENT(BUT(i)));
	}

	SetSavedItems(&mods[i]);
	XtVaSetValues (caps[i], XtNchangeBar, (XtArgVal)OL_NONE, NULL);
    }
    merge_resources(&global_resources, &resource_buf);

    ret.reason = APPLY_OK;
    return (&ret);
}

static void
Import(closure)
	XtPointer		closure;
{
    List res_buf;

    /* The num-btns value is statically defined in _resource_buf */

    /* This is a hack: only "import" the num-btns resource.  Have all mouse
       button mappings follow suit after merge during Export.
    */
    res_buf.entry	= (ADDR)&_resource_buf[NUM_BTNS_INDEX/2];
    res_buf.size	= sizeof(Resource);
    res_buf.count	= 1;
    res_buf.max		= 0;

    merge_resources(&global_resources, &res_buf);
}

static void
Export(closure)
	XtPointer		closure;
{
    ExclusiveItem *	ei;
    int			i;
    Display *		dpy = XtDisplay(InitShell);

    /* Get the resource setting for the number of Mouse buttons and set
       "factory" and "button" pointers.
    */
    ei = ResourceItem(&NumBtns, GLOBAL(NUM_BTNS_INDEX/2)); /* see note above */
    CURRENT(NUM_BTNS_INDEX) = _resource_buf[NUM_BTNS_INDEX/2].value =
	ei ? (ADDR)ei->addr : FACTORY(NUM_BTNS_INDEX);


    /* Set NumBtns.current_item now so num-btn stuff can be initialized */
    NumBtns.current_item = ResourceItem(&NumBtns, CURRENT(NUM_BTNS_INDEX));
    InitNumBtnsDependentStuff();

    for (i = 0; i < MAX_ENTRY; ++i)
    {
	char * value;

	value = GLOBAL(i);	/* get resource value.  (May be NULL) */

	if (IS_PRIMARY(i))
	{
	    /* Use th button values if valid else factory */
	    ei = ResourceItem(&btns[i], button(value));
	    CURRENT(BUT(i)) = ei ? (ADDR)ei->addr : FACTORY(BUT(i));

	} else
	{
	    CURRENT(BUT(i)) = FACTORY(BUT(i));
	}
	debug((stderr, "name is %s\n", _resource_buf[i].name));

	/* Set the buttons in the modifier menus and set corresponding label */
	set_mod_btns(&mods[i], value ? modifier(value) : FACTORY(MOD(i)));
	mods[i].current_label = (String)MALLOC(MAX_BUF);
	change_mod(&mods[i]);

	SetSavedItems(&mods[i]);

	if (MATCH(mods[i].current_label, OLG(none,fixedString)))
	{
	    CURRENT(MOD(i)) = FACTORY(MOD(i));
	    (void)sprintf(_resource_buf[i].value, "%s", CURRENT(BUT(i)));

	} else
	{
	    CURRENT(MOD(i)) = mods[i].current_label;
	    (void)sprintf(_resource_buf[i].value,
			  "%s%s", CURRENT(MOD(i)), CURRENT(BUT(i)));
	}
    }

    merge_resources(&global_resources, &resource_buf);

}					/* end of Export */

static void
Create(work, closure)
	Widget			work;
	XtPointer		closure;
{
    XFontStruct *	fs;
    Widget		controls;
    int			i;
    Display *		dpy = XtDisplay(work);
    Screen *		screen	= XtScreenOfObject(work);
    Dimension		preview_width =
	OlScreenPointToPixel(OL_HORIZONTAL, 74, screen);

    for (i = 0; IS_PRIMARY(i); i++)
    {
	_two_btns[i][0].name	= (XtArgVal)OLG(left,mnemonic);
	_two_btns[i][1].name	= (XtArgVal)OLG(right,mnemonic);
	_three_btns[i][0].name	= (XtArgVal)OLG(left,mnemonic);
	_three_btns[i][1].name	= (XtArgVal)OLG(middle,mnemonic);
	_three_btns[i][2].name	= (XtArgVal)OLG(right,mnemonic);
    }

    mods[0].string	= OLG(select,fixedString);
    mods[1].string	= OLG(adjust,fixedString);;
    mods[2].string	= OLG(menu,fixedString);
    mods[3].string	= OLG(paste,fixedString);
    mods[4].string	= OLG(Toggle,fixedString);
    mods[5].string	= OLG(Extend,fixedString);
    mods[6].string	= OLG(duplicate,fixedString);
    mods[7].string	= OLG(Link,fixedString);
    mods[8].string	= OLG(constrain,fixedString);
    mods[9].string	= OLG(cut,fixedString);
    mods[10].string	= OLG(copy,fixedString);
    mods[11].string	= OLG(scrollPan,fixedString);
    mods[12].string	= OLG(setMenuDef,fixedString);

    btns[0].string	= OLG(select,fixedString);
    btns[1].string	= OLG(adjust,fixedString);;
    btns[2].string	= OLG(menu,fixedString);
    btns[3].string	= OLG(paste,fixedString);
 
    NumBtns.string	= OLG(NumBtns,fixedString);

    _num_btns[0].name	= OLG(numbtns_two,fixedString);
    _num_btns[1].name	= OLG(numbtns_three,fixedString);

    /* Create num-btns control before creating controls below */
    NumBtns.current_item = ResourceItem(&NumBtns, CURRENT(NUM_BTNS_INDEX));
    CreateExclusive(work, &NumBtns, False);

    InitNumBtnsDependentStuff();

    /* Establish settings for "menu button" controls */
    for (i = 0; IS_PRIMARY(i); ++i)
	btns[i].current_item = ResourceItem(&btns[i], CURRENT(BUT(i)));

    /* Create the control area where the controls will reside */
    controls = XtVaCreateManagedWidget ("controls",
			controlAreaWidgetClass, work,
			XtNallowChangeBars,	(XtArgVal)True,
			XtNshadowThickness,	(XtArgVal)0,
			XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
			XtNmeasure,		(XtArgVal)3,
			XtNalignCaptions,	(XtArgVal)True,
			XtNcenter,		(XtArgVal)True,
			XtNhPad,		(XtArgVal)0,
			XtNvPad,		(XtArgVal)0,
			XtNhSpace,		(XtArgVal)6,
			XtNvSpace,		(XtArgVal)1,
			XtNsameSize,		(XtArgVal)OL_NONE,
			NULL);

    /* Create headings and mouse modifier controls */

    XtVaGetValues(XtParent(work), XtNcategoryFont, &fs, NULL);

    (void)XtVaCreateManagedWidget("heading1", staticTextWidgetClass, controls,
				  XtNstring,	(XtArgVal)"",
				  XtNvSpace,	(XtArgVal)0,
				  NULL);

    (void)XtVaCreateManagedWidget("heading2", staticTextWidgetClass, controls,
				  XtNfont,	(XtArgVal)fs,
				  XtNvSpace,	(XtArgVal)0,
				  XtNstring,
				  (XtArgVal)OLG(modifier,fixedString),
				  NULL);

    (void)XtVaCreateManagedWidget("heading3", staticTextWidgetClass, controls,
				  XtNfont,	(XtArgVal)fs,
				  XtNvSpace,	(XtArgVal)0,
				  XtNstring,
				  (XtArgVal)OLG(mouseBtn,fixedString),
				  NULL);

    for (i = 0; i < MAX_ENTRY; ++i)
    {
	Widget w, popup;

	popup = XtCreatePopupShell("pane", popupMenuShellWidgetClass,
				   controls, NULL, 0);

	w = XtVaCreateManagedWidget("abbrev", abbreviatedButtonWidgetClass,
				    (caps[i] = CreateCaption(mods[i].name,
							     mods[i].string,
							     controls)),
				    XtNpopupWidget, popup,
				    NULL);

	mods[i].addr = (ADDR)
	    XtVaCreateManagedWidget("modifiers", staticTextWidgetClass,
				    controls,
				    XtNvSpace, (XtArgVal)0,
				    XtNstring, (XtArgVal)mods[i].current_label,
				    XtNwidth, (XtArgVal)preview_width,
				    XtNrecomputeSize, (XtArgVal)False,
				    XtNwrap, (XtArgVal)False,
				    XtNgravity, (XtArgVal)WestGravity,
				    NULL);

	XtVaSetValues (w, XtNpreviewWidget, (XtArgVal)mods[i].addr, NULL);

	/* Make all modifiers (left column) non-exclusive */
	CreateNonexclusive(popup, &mods[i], True);

	/* Make mouse button controls */
	if (IS_PRIMARY(i))
	    CreateExclusive(controls, &btns[i], False);
	else
	{
	    String		modifies;
	    int			len;

	    if (MATCH(CURRENT(BUT(i)), _select))
		modifies = OLG(select,fixedString);
	    else if (MATCH(CURRENT(BUT(i)), _paste))
		modifies = OLG(paste,fixedString);
	    else
		modifies = OLG(menu,fixedString);

	    len = strlen(modifies);

	    /* Cannot modify string returned from OLG() */
	    modifies = strdup(modifies);
	    if (modifies[len - 1] == ':')
		modifies[len - 1] = '\0';

	    (void)XtVaCreateManagedWidget("modifies", staticTextWidgetClass,
					  controls,
					  XtNstring,	(XtArgVal)modifies,
					  XtNvSpace,	(XtArgVal)0,
					  NULL);
	    free(modifies);
	}
    }
}					/* end of Create */

static void
set_mod_btns(Nonexclusive * nonexclu, char * modifiers)
{
    char * s1;
    char * dup_mods;

    if ((modifiers == NULL) || (modifiers[0] == '\0'))
	return;

    dup_mods = strdup(modifiers);

    /* For each token in modifier, run through list of nonexclu items looking
       for a MATCH.  If MATCH, set it.
    */
    for (s1 = strtok(dup_mods, " "); s1 != NULL; 
	 s1 = strtok(NULL, " "))	/* Get next token until done */
    {
	NonexclusiveItem *	ni;
	list_ITERATOR		I = list_iterator(nonexclu->items);

	while ( (ni = (NonexclusiveItem *)list_next(&I)) != NULL )
	    if (MATCH(s1, (String)ni->addr))
	    {
		ni->is_set = True;
		break;
	    }
    }
    FREE(dup_mods);
}

static void
change_mod(nonexclu)
	Nonexclusive *		nonexclu;
{
    Widget		widget = (Widget)nonexclu->addr;
    list_ITERATOR	I;
    NonexclusiveItem *	ni;
    Display *		dpy = XtDisplay(InitShell);

    /* In this nonexclu, for every item that is set, concat to
       nonexclu->current_label
    */
    nonexclu->current_label[0] = '\0';
                                
    I = list_iterator(nonexclu->items);
    while ( (ni = (NonexclusiveItem *)list_next(&I)) != NULL )
    {
	if (ni->is_set)
	{
	    strcat(nonexclu->current_label, (String)ni->addr);
	    strcat(nonexclu->current_label, " ");
	}
    } 
    if (nonexclu->current_label[0] == '\0')
	strcpy(nonexclu->current_label, OLG(none,fixedString));

    if ( (widget = (Widget)nonexclu->addr) != NULL)	/* preview widget */
	XtVaSetValues (widget,
		       XtNstring, (XtArgVal)nonexclu->current_label,
		       NULL);

    if ( (widget = caps[nonexclu - mods]) != NULL)	/* caption */
	OlSetChangeBarState(widget, OL_NORMAL, OL_PROPAGATE);
}

static char *
modifier(value)
	char *			value;
{
	char *			p = strchr(value, '<');
	int			n;

	if (!p || (n = p - value) > MAX_BUF) {
		return NULL;
	}
	(void)strncpy(buffer, value, n);
	buffer[n] = 0;
	debug((stderr, " in modifier= %s\n", buffer));
	return buffer;
}

static char *
button(value)
	char *			value;
{
	char *			p = strchr(value, '<');
	int			n;

	if (!p || (n = strlen(p)) > MAX_BUF) {
		return NULL;
	}
	(void)strncpy(buffer, p, n);
	buffer[n] = 0;
	return buffer;
}

/***************************************************************************
  InitNumBtnsDependentStuff-
	Point to new set of factory resources and point the mouse button
	controls at the right items.

	The case when Export calls this is handled by NumBtns.current_item == NULL

*/
static void
InitNumBtnsDependentStuff()
{
    int		i;

    if (MATCH((char *)NumBtns.current_item->addr, _two))
    {
	_factory = &_factory_2btn;
	for (i = 0; IS_PRIMARY(i); i++)
	    btns[i].items = &two_btns[i];
    } else
    {
	_factory = &_factory_3btn;
	for (i = 0; IS_PRIMARY(i); i++)
	    btns[i].items = &three_btns[i];
    }
}

/***************************************************************************
    SwitchNumMouseButtons-
*/
static void
SwitchNumMouseButtons(Exclusive * exc)
{
    int			i;
    Resource		(*new_settings)[];

    if (exc->current_item == NULL)
	return;		/* current_item is NULL on UnselectCB: ignore */

    new_settings = MATCH((char *)exc->current_item->addr, _two) ?
	&_factory_2btn : &_factory_3btn;

    if (_factory == new_settings)	/* Shouldn't be */
	return;

    InitNumBtnsDependentStuff();

    for (i = 0; IS_PRIMARY(i); i++)
    {
	/* FactoryCB (below) will unset current_item so make sure it points
	   to an item.
	*/
	btns[i].current_item =
	    (ExclusiveItem *)(btns[i].items->entry);	/* ie. the 1st one */

	XtVaSetValues(btns[i].w,
		      XtNitems,		btns[i].items->entry,
		      XtNnumItems,	btns[i].items->count,
		      XtNitemsTouched,	True,
		      NULL);
    }

    /* re-establish factory settings */
    FactoryCB(exc->w, NULL);

    _OlSetChangeBarState(exc->w, OL_NORMAL, OL_PROPAGATE);
}

/**
 ** PingChangeBar()
 **/

static void
#if	OlNeedFunctionPrototypes
PingChangeBar (
	Exclusive *		exclusive
)
#else
PingChangeBar (exclusive)
	Exclusive *		exclusive;
#endif
{
    _OlSetChangeBarState (caps[(int)exclusive->addr], OL_NORMAL, OL_PROPAGATE);
}
