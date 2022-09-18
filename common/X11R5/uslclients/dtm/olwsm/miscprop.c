/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)dtm:olwsm/miscprop.c	1.48"

#include <stdio.h>
#include <errno.h>
#include <signal.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Category.h>
#include <Xol/ChangeBar.h>

#include <misc.h>
#include <node.h>
#include <list.h>
#include <exclusive.h>
#include <property.h>
#include <resource.h>
#include <xtarg.h>
#include <wsm.h>
#include "error.h"

	/*
	 * Define the folowing once the toolkit supports different
	 * help models.
	 */
#define	HELP_MODEL	/* */

/*
 * Local routines:
 */

static void		Import OL_ARGS((
	XtPointer		closure
));
static void		Export OL_ARGS((
	XtPointer		closure
));
static void		Create OL_ARGS((
	Widget			work,
	XtPointer		closure
));
static ApplyReturn *	ApplyCB OL_ARGS((
	Widget			w,
	XtPointer		closure
));
static void		ResetCB OL_ARGS((
	Widget			w,
	XtPointer		closure
));
static void		FactoryCB OL_ARGS((
	Widget			w,
	XtPointer		closure
));

/*
 * Global data:
 */

static Arg			misc_args[] = {
	{XtNlayoutType,		OL_FIXEDCOLS},
	{XtNalignCaptions,	True}
};

static OlDtHelpInfo MiscHelp = {
	NULL, NULL, "DesktopMgr/mispref.hlp", NULL, NULL
};

Property			miscProperty = {
	"Miscellaneous",
	misc_args,
	XtNumber (misc_args),
	&MiscHelp,
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

/*
 * Convenient macros:
 */
#define ITEM(exclusive, addr)	ResourceItem(exclusive, addr)

#define BEEP			0
#define LAYERING		1
#define SELECT_BUTTON		2
#define INPUT_AREA		3
#define MNE			4
#define ACC			5
#if	defined(HELP_MODEL)
#undef	HELP_MODEL
#define HELP_MODEL		6
#endif

#define FACTORY(x)		*(ADDR *)&(_factory[x].value)
#define CURRENT(x)		*(ADDR *)&(_current[x].value)
#define GLOBAL(x) \
	resource_value(&global_resources, _factory[x].name)

/*
 * Local data:
 */

static char		_never           [] = "never";
static char		_noticesOnly     [] = "notices";
static char		_always          [] = "always";

static char		_individually    [] = "false";
static char		_asGroup         [] = "true";

static char		_displaysDefault [] = "true";
static char		_displaysMenu    [] = "false";

static char		_realEstate      [] = "true";
static char		_clickToType     [] = "false";

static char		_off             [] = "inactive";
static char		_onUnderline     [] = "underline";
static char		_onHighlight     [] = "highlight";
static char		_onDontShow      [] = "none";
static char		_onShow          [] = "display";

#if	defined(HELP_MODEL)
static char		_pointer         [] = "pointer";
static char		_inputFocus      [] = "inputfocus";
#endif

static Resource		_factory[]	= {
	{ "*beep",              _always       },
	{ "*windowLayering",    _asGroup      },
	{ "*selectDoesPreview", _displaysMenu },
	{ "*pointerFocus",      _clickToType  },
	{ "*showMnemonics",     _onUnderline  },
	{ "*showAccelerators",  _onShow       },
#if	defined(HELP_MODEL)
	{ "*helpModel",         _pointer      },
#endif
};
static List		factory		= LIST(Resource, _factory);

static Resource		_current[]	= {
	{ "*beep",              NULL },
	{ "*windowLayering",    NULL },
	{ "*selectDoesPreview", NULL },
	{ "*pointerFocus",      NULL },
	{ "*showMnemonics",     NULL },
	{ "*showAccelerators",  NULL },
#if	defined(HELP_MODEL)
	{ "*helpModel",         NULL },
#endif
};
static List		current		= LIST(Resource, _current);

static ExclusiveItem	_acc[]		= {
	{ (XtArgVal)"Off",            (XtArgVal)_off        },
	{ (XtArgVal)"On- Show",       (XtArgVal)_onShow     },
	{ (XtArgVal)"On- Don't Show", (XtArgVal)_onDontShow },
};
static List	 acc = LIST(ExclusiveItem, _acc);
static Exclusive Acc = EXCLUSIVE("accelerators", "Accelerators:",&acc);

static ExclusiveItem	_mne[]		= {
	{ (XtArgVal)"Off", (XtArgVal)_off },
	{ (XtArgVal)"On- Underline", (XtArgVal)_onUnderline },
	{ (XtArgVal)"On- Highlight", (XtArgVal)_onHighlight },
	{ (XtArgVal)"On- Don't Show", (XtArgVal)_onDontShow },
};
static List	 mne = LIST(ExclusiveItem, _mne);
static Exclusive Mne = EXCLUSIVE("mnemonics", "Mnemonics:", &mne);

static ExclusiveItem	_inputArea[]	= {
	{ (XtArgVal)"Click SELECT", (XtArgVal)_clickToType },
	{ (XtArgVal)"Move Pointer", (XtArgVal)_realEstate  },
};
static List	 inputArea = LIST(ExclusiveItem, _inputArea);
static Exclusive InputArea = EXCLUSIVE("inputArea",
				       "Set Input Area:",
				       &inputArea);

static ExclusiveItem	_beeping[]	= {
	{ (XtArgVal)"Always",       (XtArgVal)_always      },
	{ (XtArgVal)"Notices Only", (XtArgVal)_noticesOnly },
	{ (XtArgVal)"Never",        (XtArgVal)_never       },
};
static List	 beeping = LIST(ExclusiveItem, _beeping);
static Exclusive Beeping = EXCLUSIVE("beep", "Beep:", &beeping);

static ExclusiveItem	_layering[] = {
	{ (XtArgVal)"Individually", (XtArgVal)_individually },
	{ (XtArgVal)"As a Group",   (XtArgVal)_asGroup      },
};
static List	 layering = LIST(ExclusiveItem, _layering);
static Exclusive Layering = EXCLUSIVE("windowLayering",
				      "Window Layering:",
				      &layering);

static ExclusiveItem	_select[]	= {
 	{ (XtArgVal)"Displays Default", (XtArgVal)_displaysDefault },
 	{ (XtArgVal)"Displays Menu",     (XtArgVal)_displaysMenu   },
};
static List	 select	= LIST(ExclusiveItem, _select);
static Exclusive Select = EXCLUSIVE("selectMousePress",
				    "SELECT Mouse Press:",
				    &select);

#if	defined(HELP_MODEL)
static ExclusiveItem	_helpmodel[]	= {
	{ (XtArgVal)"Input Focus", (XtArgVal)_inputFocus },
	{ (XtArgVal)"Pointer",     (XtArgVal)_pointer    },
};
static List	 helpmodel = LIST(ExclusiveItem, _helpmodel);
static Exclusive HelpModel = EXCLUSIVE("helpModel",
				       "Help Model:",
				       &helpmodel);
#endif

/**
 ** Import()
 **/

static void
#if	OlNeedFunctionPrototypes
Import (
	XtPointer		closure
)
#else
Import (closure)
	XtPointer		closure;
#endif
{
	merge_resources (&global_resources, &factory);
	return;
} /* Import */

/**
 ** Export()
 **/

static void
#if	OlNeedFunctionPrototypes
Export (
	XtPointer		closure
)
#else
Export (closure)
	XtPointer		closure;
#endif
{
	ExclusiveItem *		p;


#define _EXPORT(exclusive, index) \
	p = ITEM(exclusive, GLOBAL(index));				\
	CURRENT(index) = p ? (ADDR)p->addr : FACTORY(index)

	_EXPORT (&Beeping,   BEEP);
	_EXPORT (&Layering,  LAYERING);
	_EXPORT (&Select,    SELECT_BUTTON);
	_EXPORT (&InputArea, INPUT_AREA);
	_EXPORT (&Mne,       MNE);
	_EXPORT (&Acc,       ACC);
#if	defined(HELP_MODEL)
	_EXPORT (&HelpModel, HELP_MODEL);
#endif

	/*
	 *	Enforce our notion of valid resource values ("re-Import").
	 */
	merge_resources (&global_resources, &current);

#undef	_EXPORT
	return;
} /* Export */

/**
 ** Create()
 **/

static void
#if	OlNeedFunctionPrototypes
Create (
	Widget			work,
	XtPointer		closure
)
#else
Create (work, closure)
	Widget			work;
	XtPointer		closure;
#endif
{
        Display *dpy = XtDisplayOfObject(work);
  
	Beeping.current_item   = ITEM(&Beeping,   CURRENT(BEEP));
	Beeping.string	       = OLG(beep,fixedString);
	Layering.current_item  = ITEM(&Layering,  CURRENT(LAYERING));
	Layering.string	       = OLG(layering,fixedString);
	Select.current_item    = ITEM(&Select,    CURRENT(SELECT_BUTTON));
	Select.string	       = OLG(mouseSelect,fixedString);
	InputArea.current_item = ITEM(&InputArea, CURRENT(INPUT_AREA));
	InputArea.string       = OLG(inputArea,fixedString);
	Mne.current_item       = ITEM(&Mne,       CURRENT(MNE));
	Mne.string	       = OLG(mneSetting,fixedString);
	Acc.current_item       = ITEM(&Acc,       CURRENT(ACC));
	Acc.string	       = OLG(accel,fixedString);
#if	defined(HELP_MODEL)
	HelpModel.current_item = ITEM(&HelpModel, CURRENT(HELP_MODEL));
	HelpModel.string       = OLG(helpModel,fixedString);
#endif

	_beeping[0].name       = OLG(beepalways,fixedString);
	_beeping[1].name       = OLG(Notices,fixedString);
	_beeping[2].name       = OLG(Never,fixedString);

	_layering[0].name      = OLG(individual,fixedString);
	_layering[1].name      = OLG(group,fixedString);

	_select[0].name      = OLG(defaultdis,fixedString);
	_select[1].name      = OLG(menudis,fixedString);

	_helpmodel[0].name      = OLG(inputfoc,fixedString);
	_helpmodel[1].name      = OLG(pointerfoc,fixedString);

	_inputArea[0].name	= OLG(clicksel,fixedString);
	_inputArea[1].name	= OLG(movepoint,fixedString);

	_mne[0].name		= OLG(Off,fixedString);
	_mne[1].name		= OLG(Underline,fixedString);
	_mne[2].name		= OLG(Highlight,fixedString);
	_mne[3].name		= OLG(dontshow,fixedString);

	_acc[0].name		= OLG(Off,fixedString);
	_acc[1].name		= OLG(Onshow,fixedString);
	_acc[2].name		= OLG(dontshow,fixedString);

	CreateExclusive(work, &Beeping,   TRUE);
	CreateExclusive(work, &Layering,  TRUE);
	CreateExclusive(work, &Select,    TRUE);
#if	defined(HELP_MODEL)
	CreateExclusive(work, &HelpModel, TRUE);
#endif
	CreateExclusive(work, &InputArea, TRUE);
	CreateExclusive(work, &Mne,       TRUE);
	CreateExclusive(work, &Acc,       TRUE);

	return;
} /* Create */

/**
 ** ApplyCB()
 **/

static ApplyReturn *
#if	OlNeedFunctionPrototypes
ApplyCB (
	Widget			w,
	XtPointer		closure
)
#else
ApplyCB (w, closure)
	Widget			w;
	XtPointer		closure;
#endif
{
	static ApplyReturn	ret;

#define _APPLY(exclusive, index) \
	CURRENT(index) = (ADDR)(exclusive).current_item->addr;		\
	_OlSetChangeBarState ((exclusive).w, OL_NONE, OL_PROPAGATE)

	_APPLY (Beeping,   BEEP);
	_APPLY (Layering,  LAYERING);
	_APPLY (Select,    SELECT_BUTTON);
	_APPLY (InputArea, INPUT_AREA);
	_APPLY (Mne,       MNE);
	_APPLY (Acc,       ACC);
#if	defined(HELP_MODEL)
	_APPLY (HelpModel, HELP_MODEL);
#endif

#undef	_APPLY

	merge_resources(&global_resources, &current);
	return (&ret);
} /* ApplyCB */

/**
 ** ResetCB()
 **/

static void
#if	OlNeedFunctionPrototypes
ResetCB (
	Widget			w,
	XtPointer		closure
)
#else
ResetCB (w, closure)
	Widget			w;
	XtPointer		closure;
#endif
{
#define _RESET(exclusive, index) \
	SetExclusive(exclusive, ITEM(exclusive, CURRENT(index)), OL_NONE)

	_RESET (&Beeping,   BEEP);
	_RESET (&Layering,  LAYERING);
	_RESET (&Select,    SELECT_BUTTON);
	_RESET (&InputArea, INPUT_AREA);
	_RESET (&Mne,       MNE);
	_RESET (&Acc,       ACC);
#if	defined(HELP_MODEL)
	_RESET (&HelpModel, HELP_MODEL);
#endif

#undef	_RESET
	return;
} /* ResetCB */

/**
 ** FactoryCB()
 **/

static void
#if	OlNeedFunctionPrototypes
FactoryCB (
	Widget			w,
	XtPointer		closure
)
#else
FactoryCB (w, closure)
	Widget			w;
	XtPointer		closure;
#endif
{
#define _FACTORY(exclusive, index) \
	SetExclusive(exclusive, ITEM(exclusive,FACTORY(index)), OL_NORMAL)

	_FACTORY (&Beeping,   BEEP);
	_FACTORY (&Layering,  LAYERING);
	_FACTORY (&Select,    SELECT_BUTTON);
	_FACTORY (&InputArea, INPUT_AREA);
	_FACTORY (&Mne,       MNE);
	_FACTORY (&Acc,       ACC);
#if	defined(HELP_MODEL)
	_FACTORY (&HelpModel, HELP_MODEL);
#endif

#undef	_FACTORY
	return;
} /* Factory */
