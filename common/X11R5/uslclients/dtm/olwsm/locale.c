/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:olwsm/locale.c	1.36"

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>

#include <Xol/OpenLookP.h>
#include <Xol/AbbrevButt.h>
#include <Xol/Caption.h>
#include <Xol/Category.h>
#include <Xol/ChangeBar.h>
#include <Xol/ControlAre.h>
#include <Xol/FButtons.h>
#include <Xol/MenuShell.h>
#include <Xol/RubberTile.h>
#include <Xol/ScrolledWi.h>
#include <Xol/FList.h>
#include <Xol/StaticText.h>
#include <Xol/array.h>

#include <sys/stat.h>
#include "dirent.h"
#include "ctype.h"
#include "misc.h"
#include "node.h"
#include "list.h"
#include "exclusive.h"
#include "property.h"
#include "resource.h"
#include "wsm.h"
#include "error.h"

/*
 * Private types:
 */

static char *choice_fields[] = {
	XtNlabel,
	XtNselectProc
};

typedef struct _choice_items {
	char *	label;
	void	(*select)();
} _choice_items;

typedef struct ResourceName {
	String			name;
	XrmQuark		quark;
}			ResourceName;

typedef struct Abbreviated {
	String			name;
	String			label;
	Widget			button;
	Widget			show_current;
	Exclusive		exclusive;
}			Abbreviated;

#define EXCL(LAB, STR) \
    {									\
	False,								\
	(LAB),								\
	(STR),								\
	(ExclusiveItem *)0,						\
	(ExclusiveItem *)0,						\
	(List *)0,							\
	(void (*) OL_ARGS(( struct Exclusive * )))0,			\
	(ADDR)0,							\
	(Widget)0,							\
	True								\
    }
#define ABBREVIATED(name) \
    { name, 0, 0, 0, EXCL("menu", "menu") }

typedef struct LocaleListView {
	String			name;
	Widget			list;
	Cardinal		current;
}			LocaleListView;

typedef struct Locale {
	XrmDatabase		db;
	String			name;
	String			label;
	Cardinal		item;
}			Locale;

typedef _OlArrayStruct(String, StringArray)	StringArray;
typedef _OlArrayStruct(Locale, LocaleArray)	LocaleArray;

/*
 * Private routines:
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
static void		PopdownCB OL_ARGS((
	Widget			w,
	XtPointer		closure
));
static String		SetCurrent OL_ARGS((
	Cardinal		index,
	String			value,
	Boolean			copy
));
static void		FetchFileSearchPath OL_ARGS((
	void
));
static String		FindSubString OL_ARGS((
	String			src,
	String			substring
));
static int		StringCompare OL_ARGS((
	String *		a,
	String *		b
));
static Boolean		AccumulateFileSearchPath OL_ARGS((
	String			filename
));
static Boolean		AccumulateParentLocaleDirectories OL_ARGS((
	String			filename
));
static Boolean		IsDir OL_ARGS((
	String			parent,
	String			file
));
static void		AddDatabaseFileToLocaleList OL_ARGS((
	String			file,
	LocaleArray *		locales
));
static String		GetResource OL_ARGS((
	XrmDatabase		db,
	ResourceName *		pname
));
static String		_MultiCodesetStrchr OL_ARGS((
	String			str,
	int			c,
	int			quote
));
static void		ParseNameAndLabel OL_ARGS((
	String			value,
	String *		pname,
	String *		plabel
));
static Locale *		FindLocale OL_ARGS((
	String			locale
));
static int		LocaleCompare OL_ARGS((
	Locale *		pa,
	Locale *		pb
));
static void		CreateLocaleListView OL_ARGS((
	Locale *		locale,
	Widget			parent
));
static void		LocaleSelectCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static void		Reset OL_ARGS((
	Locale *		p,
	List *			resource_list
));
static Boolean		NewLocale OL_ARGS((
	LocaleListView *	pv,
	Locale *		p,
	OlDefine		state_change
));
static void		CreateSettingsView OL_ARGS((
	Locale *		locale,
	Widget			parent
));
static void		FetchResourceAndParse OL_ARGS((
	XrmDatabase		db,
	ResourceName *		resource,
	Exclusive *		exclusive
));
static String		InputMethodPartner OL_ARGS((
	Locale *		locale,
	String			im,
	Cardinal		partner
));
static void		FreeExclusive OL_ARGS((
	Exclusive * 		exclusive
));
static void		CreateAbbreviated OL_ARGS((
	Abbreviated *		pa,
	Widget			parent
));
static void		ShowCurrent OL_ARGS((
	Exclusive *		pe
));
static void		NextChoiceCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static Display *	OpenDisplayForNewLocale OL_ARGS((
	String			locale
));
static void		CloseDisplay OL_ARGS((
	Display *		display
));
static void		FillInLangSubs OL_ARGS((
	Substitution		subs,
	String			string
));

/*
 * Convenient macros:
 */

#define XNLLANGUAGE		0
#define INPUTLANG		1
#define DISPLAYLANG		2
#define DATETIMEFORMAT		3
#define NUMERICFORMAT		4
#define FONTGROUP		5
#define FONTGROUPDEF		6
#define INPUTMETHOD		7
#define IMSTATUS		8
#define FRONTENDSTRING		9
#define BASICLOCALE		10

#define CURRENT(x)		*(ADDR *)&(_current[x].value)
#define GLOBAL(x) \
	resource_value(&global_resources, _current[x].name)

#define ITEM(exclusive,addr)	ResourceItem(exclusive, addr)

#define MultiCodesetStrchr(s,c)           _MultiCodesetStrchr(s,c,0)
#define MultiCodesetStrchrWithQuotes(s,c) _MultiCodesetStrchr(s,c,'"')

/*
 * Public data:
 */
static Arg		locale_args[] = {
	{XtNlayoutType,		OL_FIXEDCOLS},
	{XtNmeasure,		1}
};

static OlDtHelpInfo LocaleHelp = {
	NULL, NULL, "DesktopMgr/locpref.hlp", NULL, NULL
};

Property		localeProperty = {
	"Set Locale",
	locale_args,
	XtNumber (locale_args),
	&LocaleHelp,
	'\0',
	Import,
	Export,
	Create,
	ApplyCB,
	ResetCB,
	FactoryCB,
	PopdownCB,
	0,
	0,
	0,
};

/*
 * Private data:
 */

static _choice_items choice_items[] = {
	"Next Choice", NextChoiceCB
};

static String		defaultLocaleName = "C";
static String		defaultLocale     = "\
	*xnlLanguage:    C \"American English\"\n\
	*inputLang:      C \"American English\"\n\
	*displayLang:    C \"American English\"\n\
	*timeFormat:     C \"12/31/90 5:30 PM\"\n\
	*numeric:        C \"10,000\"\n\
	*fontGroup:      \n\
	*fontGroupDef:   \n\
	*inputMethod:    \n\
	*imStatus:       False\n\
	*frontEndString: \n\
	*basicLocale:    C \"C\"\n\
";

static ResourceName	resources[] = {
	{ "xnlLanguage",    0 },
	{ "inputLang",      0 },
	{ "displayLang",    0 },
	{ "timeFormat",     0 },
	{ "numeric",        0 },
	{ "fontGroup",      0 },
	{ "fontGroupDef",   0 },
	{ "inputMethod",    0 },
	{ "imStatus",       0 },
	{ "frontEndString", 0 },
	{ "basicLocale",    0 },
};

static List		factory = LISTOF(Resource);

static Resource		_current[]	= {
	{ "*xnlLanguage"    },
	{ "*inputLang"      },
	{ "*displayLang"    },
	{ "*timeFormat"     },
	{ "*numeric"        },
	{ "*fontGroup"      },
	{ "*fontGroupDef"   },
	{ "*inputMethod"    },
	{ "*imStatus"       },
	{ "*frontEndString" },
	{ "*basicLocale"    },
};
static List		current		= LIST(Resource, _current);

static Widget		SettingsView	= 0;
static LocaleListView	XnlLanguage     = { "xnlLanguage" };

static Abbreviated	InputLang	= ABBREVIATED("inputLang");
static Abbreviated	DisplayLang	= ABBREVIATED("displayLang");
static Abbreviated	DateTimeFormat	= ABBREVIATED("timeFormat");
static Abbreviated	NumericFormat	= ABBREVIATED("numeric");
static Abbreviated	FontGroup	= ABBREVIATED("fontGroup");
static Abbreviated	InputMethod	= ABBREVIATED("inputMethod");

static String		FileSearchPath              = 0;

static Display *	SeparateDisplayForNewLocale = 0;
static String		SeparateLocale              = 0;

String		CurrentLocale            = 0;
String		PrevLocale               = 0;

static StringArray	ParentLocaleDirectories;
static LocaleArray	locales;

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
	DIR *			dirp;

	struct dirent *		direntp;

	Locale			locale;
	Locale *		p;

	String			dir;
	String			file;
	String			xnlLanguage;

	Cardinal		n;

	Resource		r;

	static SubstitutionRec	subs[] = {
		{ 'N' , OL_LOCALE_DEF },  /* MUST BE 0th ELEMENT */
		{ 'T' , "" },
		{ 'S' , "" },
		{ 'C' , "" },
		{ 'L' },
		{ 'l' },
		{ 't' },
		{ 'c' },
	};
/*	subs[4..7].substitution =   set below */


	/*
	 * Most of what we do in this routine is look for all the
	 * system and user defined locales.
	 */


	_OlArrayInitialize (&ParentLocaleDirectories, 10, 10, StringCompare);
	_OlArrayInitialize (&locales, 10, 10, LocaleCompare);

	/*
	 * Fetch the names of the parent directories of the locale
	 * definition files. This is tricky, because the convenient
	 * routine that knows how to parse XFILESEARCHPATH (or an
	 * implementation-specific default for it) automatically
	 * substitutes the current locale's name into the path--we
	 * don't want the directory for the current locale, we want
	 * the parent directory(s) for all possible locales. Thus we
	 * first discover XFILESEARCHPATH on our own and we don't use
	 * that convenient routine (OK, its name is XtResolvePathname).
	 */
	FetchFileSearchPath ();
	if (FileSearchPath) {
		/*
		 * We don't want to expand the standard substitutions,
		 * but instead want them to show up as themselves.
		 * XtFindFile will drop the % if it has no substitution,
		 * so we do the following. (Actually, the only one we
		 * care about is %L.)
		 *
		 * We also reconstruct the search path so it parallels the
		 * list of parent directories. This excludes path entries
		 * that, e.g., don't have a %L and would thus be locale
		 * insensitive. This is important to avoid locking in on
		 * the same locale definition file in a %L-less path, in
		 * the XtFindFile later on.
		 *
		 * See AccumulateParentLocaleDirectories() for details.
		 */
		static SubstitutionRec	same_subs[] = {
			{ 'N' , "%N" },
			{ 'T' , "%T" },
			{ 'S' , "%S" },
			{ 'C' , "%C" },
			{ 'L' , "%L" },
			{ 'l' , "%l" },
			{ 't' , "%t" },
			{ 'c' , "%c" },
		};
		String			fsp = FileSearchPath;

		FileSearchPath = 0;
		(void)XtFindFile (
			fsp,
			same_subs, XtNumber(same_subs),
			AccumulateParentLocaleDirectories
		);
		XtFree (fsp);
	}

	/*
	 * The user may have defined a private locale, so take a shot
	 * in the dark to see if we hit it. If we do, save the locale
	 * and later ignore any ``system'' locale of the same name.
	 *
	 * Note: If we were started with a valid locale (e.g. via
	 * the xnlLanguage option or resource, or via $LANG), then
	 * the locale definition file fetched below will be for that
	 * locale. That's mostly OK, the only bummer is that it may
	 * hide a locale definition file hidden in an odd place by
	 * the user.
	 */
	file = XtResolvePathname(
		DISPLAY,
		(String)0,		/* type */
		subs[0].substitution,	/* filename */
		(String)0,		/* suffix */
		(String)0,		/* use default path */
		(Substitution)0, (Cardinal)0,
		(XtFilePredicate)0	/* use default */
	);
	if (file) {
		AddDatabaseFileToLocaleList (file, &locales);
		XtFree (file);
	}

	/*
	 * Step through the content of each directory in the
	 * accumulated list of potential locale parent directories,
	 * to see which subdirectories (if any) harbor locale
	 * definition files.
	 */
	for (n = 0; n < _OlArraySize(&ParentLocaleDirectories); n++) {
		String	parent = _OlArrayElement(&ParentLocaleDirectories, n);

		if (!(dirp = opendir(parent)))
			continue;

		while ((direntp = readdir(dirp))) {
			/*
			 * Skip the obvious.
			 */
			if (
				MATCH(direntp->d_name, ".")
			     || MATCH(direntp->d_name, "..")
			     || !IsDir(parent, direntp->d_name)
			)
				continue;

			/*
			 * Found a file or sub-directory under the parent
			 * of a locale directory. Try this as a locale
			 * name. Skip it if no locale definition file is
			 * found.
			 */
			FillInLangSubs (&subs[4], direntp->d_name);
			file = XtFindFile(
				FileSearchPath,
				subs, XtNumber(subs),
				(XtFilePredicate)0 /* use default */
			);
			if (subs[5].substitution)
				/*
				 * See "FillInLangSubs" (sorry!)
				 */
				XtFree ((XtPointer)subs[5].substitution);
			if (!file)
				continue;

			/*
			 * Found a correctly named locale definition file;
			 * fetch the content into an XrmDatabase and run a
			 * sanity check on it.
			 */
			AddDatabaseFileToLocaleList (file, &locales);
			XtFree (file);
		}
		closedir (dirp);
	}

	/*
	 * Make sure we have the "C" locale, and that it's first
	 * in the list. If we don't have it yet, use its hard-wired
	 * definition.
	 *
	 * All other locales are put in alphabetical location (using
	 * "C" lexical sort).
	 */
#if	defined(LOCALEDEBUG)
printf ("[%s line %d] Test with C locale defined/not-defined externally\n", __FILE__, __LINE__);
#endif
	p = FindLocale(defaultLocaleName);
	if (p) {
		locale = *p;
		n = p - &_OlArrayElement(&locales, 0);
		if (n != 0) {
			_OlArrayDelete (&locales, n);
			_OlArrayInsert (&locales, 0, locale);
		}
	} else {
		locale.db = XrmGetStringDatabase(defaultLocale);
		xnlLanguage = GetResource(locale.db, &resources[XNLLANGUAGE]);
		if (xnlLanguage) {
			ParseNameAndLabel (
				xnlLanguage, &locale.name, &locale.label
			);
			_OlArrayInsert (&locales, 0, locale);
		}
	}

	/*
	 * The above work gave us the "C" locale in "locale".
	 *
	 * First, we add the name of the default (factory) locale
	 * (duh, must be "C"!) to the global resources. This keeps
	 * all other specific and supplementary locale resources
	 * out of the global set, except those fetched from the
	 * user's resource set (after Import). Then, in Export,
	 * we look for missing specific and supplementary resources
	 * and fill them in from the locale (either the default
	 * locale or the locale specified by the user through his/her
	 * resources).
	 *
	 * Second, we construct the balance of the factory resources,
	 * for use if the user presses the Reset to Factory button.
	 */

	r.name  = _current[XNLLANGUAGE].name;
	r.value = XtNewString(locale.name);
	list_append (&factory, &r, 1);

	/* for compatibility with Sun we need to write "basicLocale" also */
	r.name  = _current[BASICLOCALE].name;
	r.value = XtNewString(locale.name);
	list_append (&factory, &r, 1);

	merge_resources (&global_resources, &factory);

	for (n = 0; n < XtNumber(resources); n++) {
		String			value;
		String			comma;
		String			name;
		String			label;

		if (n == XNLLANGUAGE)
			continue;

		r.name = _current[n].name;
		/*
		 * The value we fetch here may be multiple-choice,
		 * so parse out the first choice as the default
		 * (``factory'') value.
		 */
		if (!(value = GetResource(locale.db, &resources[n])))
			r.value = XtNewString("");
		else {
			comma = MultiCodesetStrchrWithQuotes(value, ',');
			if (comma)
				*comma = 0;
			ParseNameAndLabel (value, &name, &label);

			/*
			 * The factory stuff never needs freeing.
			 */
			r.value = name;
			XtFree (label);

			if (comma)
				*comma = ',';
		}

		/*
		 * WARNING: Make sure the factory list gets created
		 * in the same order as the current list, so that a
		 * given index fetches the same resource value from
		 * either.
		 */
		list_append (&factory, &r, 1);
	}

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
	Locale *		locale;

	Cardinal		n;


	/*
	 * Fetch the current locale, for use below.
	 */
#if	defined(LOCALEDEBUG)
printf ("GLOBAL(%d) %s\n", XNLLANGUAGE, GLOBAL(XNLLANGUAGE));
#endif
	CurrentLocale = SetCurrent(XNLLANGUAGE, GLOBAL(XNLLANGUAGE), True);

	/*
	 * MORE: Watch out! User may have given screwy value for
	 * the locale.
	 */
#if	defined(LOCALEDEBUG)
printf ("[%s line %d] Check for unknown locale\n", __FILE__, __LINE__);
#endif
	locale = FindLocale(CurrentLocale);
#if	defined(LOCALEDEBUG)
printf ("locale %x, CurrentLocale %s\n", locale, CurrentLocale);
#endif
	/*
	 * Fetch the ``real'' current values from the RESOURCE_MANAGER
	 * database, but note that they may not be there.
	 */
	for (n = 1; n < XtNumber(resources); n++) {
		if (GLOBAL(n)) {
			/*
			 * MORE: Make sure the value is correct.
			 */
			SetCurrent (n, GLOBAL(n), True);
		} else if (n == BASICLOCALE) {
			/*
			 * For compatibility with Sun, we need to write
			 * "basicLocale".
			 */
			SetCurrent(n, CURRENT(XNLLANGUAGE), True);
		} else {
			String			value;
			String			comma;
			String			name;

			/*
			 * The value we fetch here may be multiple-choice,
			 * so parse out the first choice as the default
			 * value (this is an arbitrary choice).
			 */
			if (!(value = GetResource(locale->db, &resources[n])))
				SetCurrent (n, "", True);
			else {
				comma = MultiCodesetStrchrWithQuotes(value, ',');
				if (comma)
					*comma = 0;

				ParseNameAndLabel (value, &name, (String *)0);
				SetCurrent (n, name, False);
				if (comma)
					*comma = ',';
			}
		}
	}

	merge_resources(&global_resources, &current);

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
	Locale *		locale;


	/*
	 * Find the current locale in the list of locales.
	 */
	locale = FindLocale(CURRENT(XNLLANGUAGE));
	if (!locale)
		locale = &_OlArrayElement(&locales, 0);

	/*
	 * Keep the list from being made wider than it needs to be.
	 */
	XtVaSetValues (
		work,
		XtNcenter,   (XtArgVal)True,
		XtNsameSize, (XtArgVal)OL_NONE,
		(String)0
	);

	CreateLocaleListView (locale, work);
	CreateSettingsView (locale, work);

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
	static ApplyReturn	ret = { APPLY_RESTART };

	Locale *		p;

	String			value;


	OlVaFlatGetValues (
		XnlLanguage.list, XnlLanguage.current,
		XtNuserData, (XtArgVal)&p,
		(String)0
	);

	PrevLocale = strdup(CurrentLocale);
	CurrentLocale = SetCurrent(XNLLANGUAGE, (ADDR)(p->name), True);
	_OlSetChangeBarState (XnlLanguage.list, OL_NONE, OL_PROPAGATE);

#define _APPLY(abbrev,index) \
	if (abbrev.exclusive.items) {					\
		SetCurrent (						\
		  index, (ADDR)abbrev.exclusive.current_item->addr, True\
		);							\
		_OlSetChangeBarState (					\
		  abbrev.button, OL_NONE, OL_PROPAGATE			\
		);							\
	} else								\
		SetCurrent (index, "", True)

	_APPLY (DisplayLang,    DISPLAYLANG);
	_APPLY (InputLang,      INPUTLANG);
	_APPLY (NumericFormat,  NUMERICFORMAT);
	_APPLY (DateTimeFormat, DATETIMEFORMAT);
	_APPLY (FontGroup,      FONTGROUP);
	_APPLY (InputMethod,    INPUTMETHOD);

	if ((value = GetResource(p->db, &resources[FONTGROUPDEF])))
		SetCurrent (FONTGROUPDEF, value, True);
	else
		SetCurrent (FONTGROUPDEF, "", True);

	SetCurrent (
		IMSTATUS,
		InputMethodPartner(p, CURRENT(INPUTMETHOD), IMSTATUS),
		False
	);
	SetCurrent (
		FRONTENDSTRING,
		InputMethodPartner(p, CURRENT(INPUTMETHOD), FRONTENDSTRING),
		False
	);

	/* for compatibility with Sun, we need to write "basicLocale" also */
	SetCurrent(BASICLOCALE, CURRENT(XNLLANGUAGE), True);
	merge_resources (&global_resources, &current);

#undef	_APPLY
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
	Reset (FindLocale(CURRENT(XNLLANGUAGE)), &current);
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
	Reset (FindLocale(defaultLocaleName), &factory);
	return;
} /* FactoryCB */

/**
 ** PopdownCB()
 **/

static void
#if	OlNeedFunctionPrototypes
PopdownCB (
	Widget			w,
	XtPointer		closure
)
#else
PopdownCB (w, closure)
	Widget			w;
	XtPointer		closure;
#endif
{
	FreeExclusive (&DisplayLang.exclusive);
	FreeExclusive (&InputLang.exclusive);
	FreeExclusive (&NumericFormat.exclusive);
	FreeExclusive (&DateTimeFormat.exclusive);
	FreeExclusive (&FontGroup.exclusive);
	FreeExclusive (&InputMethod.exclusive);

	/*
	 * Do this to (1) clean up stuff we may not need for a long
	 * time and (2) to force a re-open when we need it again.
	 *
	 * CAUTION:
	 * (2) is necessary for now, since the dynamic resource handling
	 * in Xol doesn't support multiple displays.
	 */
	if (SeparateDisplayForNewLocale)
		CloseDisplay (SeparateDisplayForNewLocale);
	
	/*
	 * Remove reference to the view widget.  It is
	 * destroyed when it is Popped down.
	 */
	SettingsView = 0;

	return;
} /* PopdownCB */

/**
 ** SetCurrent()
 **/

static String
#if	OlNeedFunctionPrototypes
SetCurrent (
	Cardinal		index,
	String			value,
	Boolean			copy
)
#else
SetCurrent (index, value, copy)
	Cardinal		index;
	String			value;
	Boolean			copy;
#endif
{
	if (CURRENT(index))
		XtFree (CURRENT(index));
	return (CURRENT(index) = (copy? XtNewString(value) : value));
} /* SetCurrent */

/**
 ** FetchFileSearchPath()
 **/

static void
#if	OlNeedFunctionPrototypes
FetchFileSearchPath (
	void
)
#else
FetchFileSearchPath ()
#endif
{
	static String		language_spec_format = "%l_%t.%c";

	Display *		display;

	Cardinal		language_spec_format_length;

	String			p;

	extern char *		getenv();


	/*
	 * We need: The given or default value for XFILESEARCHPATH.
	 * We can't use XtResolvePathname(), because it provides
	 * a language substitution for %L and %l that we can't
	 * override. We need to call XtFindFile() directly, and
	 * this requires knowing the correct path.
	 *
	 * Problem: We don't know the correct path to pass to
	 * XtFindFile(). If XFILESEARCHPATH is defined in the
	 * environment, great; but if it isn't, the Intrinsics
	 * have an implementation-specific default that isn't
	 * publically known.
	 *
	 * Solution: We deduce the default value by probing the
	 * Intrinsics with repeated calls to XtResolvePathname().
	 */

	if (FileSearchPath)
		return;
	else if ((FileSearchPath = getenv("XFILESEARCHPATH"))) {
		FileSearchPath = XtNewString(FileSearchPath);
		return;
	}

	/*
	 * Reopen the current display under a new application
	 * context, but provide a bogus language specification
	 * through the argv/argc parameters. Because of the bogus
	 * values for the language and the application class, the
	 * resource database creation overhead should be minimal.
	 */
	display = OpenDisplayForNewLocale(language_spec_format);
	XtResolvePathname (
		display,
		"%T", "%N", "%S",	/* type, filename, suffix */
		(String)0,		/* use default path */
		(Substitution)0, (Cardinal)0,
		AccumulateFileSearchPath
	);
	CloseDisplay (display);

	p = FileSearchPath;
	language_spec_format_length = strlen(language_spec_format);
	while ((p = FindSubString(p, language_spec_format))) {
		/*
		 * Each time we find %l_%t.%c (language_spec_format)
		 * in the path, we replace it--in place--with %L.
		 * Since the %L is shorter than %l_%t.%c, we simply
		 * overlay %L on top of %l and collapse the rest of
		 * the path over the _%t.%c.
		 */
		sprintf (p, "%s%s", "%L", p + language_spec_format_length);
		p += 2;	/* skip the %L */
	}

	return;
} /* FetchFileSearchPath */

/**
 ** FindSubString()
 **/

static String
#if	OlNeedFunctionPrototypes
FindSubString (
	String			src,
	String			substring
)
#else
FindSubString (src, substring)
	String			src;
	String			substring;
#endif
{
	Cardinal		n	= strlen(substring);


	while (*src) {
		if (strncmp(src, substring, n) == 0)
			return (src);
		src++;
	}
	return (0);
} /* FindSubString */

/**
 ** StringCompare()
 **/

static int
#if	OlNeedFunctionPrototypes
StringCompare (
	String *		a,
	String *		b
)
#else
StringCompare (a, b)
	String *		a;
	String *		b;
#endif
{
	return (strcmp(*a, *b));
} /* StringCompare */

/**
 ** AccumulateFileSearchPath()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
AccumulateFileSearchPath (
	String			filename
)
#else
AccumulateFileSearchPath (filename)
	String			filename;
#endif
{
	String			newpath;

	Cardinal		curlen;


	if (!FileSearchPath)
		FileSearchPath = XtNewString(filename);
	else {
		curlen = strlen(FileSearchPath);
		FileSearchPath = XtRealloc(
			FileSearchPath, curlen + 1 + strlen(filename) + 1
		);
		sprintf (FileSearchPath + curlen, ":%s", filename);
	}

	return (False);	/* keep the ``search'' going */
} /* AccumulateFileSearchPath */

/**
 ** AccumulateParentLocaleDirectories()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
AccumulateParentLocaleDirectories (
	String			filename
)
#else
AccumulateParentLocaleDirectories (filename)
	String			filename;
#endif
{
	String			locale;


	if ((locale = FindSubString(filename, "%L"))) {
		*locale = 0;
		/*
		 * Can't use _OlArrayUniqueAppend below, because we need
		 * to malloc a copy of filename once we know we should add
		 * it to the list.
		 */
		if (
			IsDir((String)0, filename)
		     && _OlArrayFind(&ParentLocaleDirectories, filename)
				== _OL_NULL_ARRAY_INDEX
		) {
			String f = XtNewString(filename);
			_OlArrayAppend (&ParentLocaleDirectories, f);
			*locale = '%';
			AccumulateFileSearchPath (filename);
		} else
			*locale = '%';
	}

	return (False);	/* keep the ``search'' going */
} /* AccumulateParentLocaleDirectories */

/**
 ** IsDir()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
IsDir (
	String			parent,
	String			file
)
#else
IsDir (parent, file)
	String			parent;
	String			file;
#endif
{
	char			buffer[128];

	String			name = 0;

	struct stat		status;

	Boolean			ret;


	if (parent) {
		Cardinal len = strlen(parent) + 1 + strlen(file) + 1;
		name = len <= sizeof(buffer)? buffer : XtMalloc(len);
		sprintf (name, "%s/%s", parent, file);
	} else
		name = file;

	ret = (
		access(name, (R_OK|X_OK)) == 0
	     && stat(name, &status) == 0
	     && (status.st_mode & S_IFDIR) == S_IFDIR
	);

	if (name != file && name != buffer)
		XtFree (name);

	return (ret);
} /* IsDir */

/**
 ** AddDatabaseFileToLocaleList()
 **/

static void
#if	OlNeedFunctionPrototypes
AddDatabaseFileToLocaleList (
	String			file,
	LocaleArray *		locales
)
#else
AddDatabaseFileToLocaleList (file, locales)
	String			file;
	LocaleArray *		locales;
#endif
{
	String			xnlLanguage;

	Locale			locale;


	/*
	 * A valid locale definition file looks like an XrmDatabase.
	 */
	locale.db = XrmGetFileDatabase(file);
	if (!locale.db)
		return;

	/*
	 * Found a real database; skip it if it doesn't
	 * contain at least a basicLocal resource.
	 */
	xnlLanguage = GetResource(locale.db, &resources[XNLLANGUAGE]);
	if (!xnlLanguage)
		return;

	/*
	 * We're on a roll! Found a real locale database;
	 * skip it if it identifies the same locale we
	 * already know about.
	 */
	ParseNameAndLabel (xnlLanguage, &locale.name, &locale.label);
	if (FindLocale(locale.name)) {
		XtFree (locale.name);
		XtFree (locale.label);
	} else
		_OlArrayOrderedInsert (locales, locale);

	return;
} /* AddDatabaseFileToLocaleList */

/**
 ** GetResource()
 **/

static String
#if	OlNeedFunctionPrototypes
GetResource (
	XrmDatabase		db,
	ResourceName *		pname
)
#else
GetResource (db, pname)
	XrmDatabase		db;
	ResourceName *		pname;
#endif
{
	static XrmQuark		QString  = 0;
	static XrmQuark		class[2] = { 0 , 0 };
	static XrmQuark		name[2]  = { 0 , 0 };

	XrmQuark		type;

	XrmValue		value;


	if (!pname->quark)
		pname->quark = XrmStringToQuark(pname->name);
	if (!QString)
		QString = XrmStringToQuark("String");
	if (class[0] == 0)
		class[0] = XrmUniqueQuark();

	name[0] = pname->quark;
	XrmQGetResource (db, name, class, &type, &value);

	if (type != QString)
		return (0);	/* Huh? */
	else
		return ((String)value.addr);
} /* GetResource */

/**
 ** _MultiCodesetStrchr()
 **/

static String
#if	OlNeedFunctionPrototypes
_MultiCodesetStrchr (
	String			str,
	int			c,
	int			quote
)
#else
_MultiCodesetStrchr (str, c, quote)
	String			str;
	int			c;
	int			quote;
#endif
{
	String			p;

	Boolean			within_quotes	= False;


#if	defined(LOCALEDEBUG)
printf ("[%s line %d] Need to use code-set segment parser!\n", __FILE__, __LINE__);
#endif
	for (p = str; *p; p++)
		if (*p == '\\')
			if (!*++p)
				break;
			else
				continue;
		else if (*p == quote)
			within_quotes = !within_quotes;
		else if (!within_quotes && *p == c)
			return (p);

	return (c? 0 : p);
} /* _MultiCodesetStrchr */

/**
 ** ParseNameAndLabel()
 **/

static void
#if	OlNeedFunctionPrototypes
ParseNameAndLabel (
	String			value,
	String *		pname,
	String *		plabel
)
#else
ParseNameAndLabel (value, pname, plabel)
	String			value;
	String *		pname;
	String *		plabel;
#endif
{
	String			end;
	String			quote;

	Cardinal		len;


	/*
	 * Syntax is
	 *
	 *	name "xxx...xx"
	 *
	 * where the stuff in double quotes is the label. Note,
	 * though, that the ``stuff'' is in an arbitrary encoding!
	 */

	/*
	 * Strip leading spaces. Here's some induction for you,
	 * in an argument why we needn't worry about non-C codesets
	 * here: If very first character matches a space, then we
	 * can increment the character pointer by one byte and
	 * loop again. Next byte and subsequent bytes can be compared
	 * to whitespace and can be skipped if they match.
	 */
	if (value)
		while (isspace(*value))
			value++;
	if (!value || !*value) {
#if	defined(YIKES)
		*pname  = XtNewString("");
		if (plabel)
			*plabel = XtNewString("");
#else
		*pname = XtMalloc(1);
		(*pname)[0] = 0;
		if (plabel) {
			*plabel = XtMalloc(1);
			(*plabel)[0] = 0;
		}
#endif
		return;
	}

	/*
	 * Find the 1st double quote:
	 */
	if ((quote = MultiCodesetStrchr(value, '"'))) {
		/*
		 * Find the 2nd double quote.
		 */
		end = MultiCodesetStrchr(quote+1, '"');

		/*
		 * If just one double quote was found, treat this as if
		 * a quote was found at the end.
		 */
		if (!end)
			end = MultiCodesetStrchr(quote+1, 0);

		/*
		 * Label starts just past the first quote
		 * and ends just before the last quote.
		 */
		len = end - (quote+1);
		if (plabel) {
			*plabel = memcpy(XtMalloc(len+1), quote+1, len);
			(*plabel)[len] = 0;
		}
	} else {
		/*
		 * No quote, therefore the label is the same as
		 * the name.
		 *
		 * Set things up so it looks as if a quote was
		 * found at the very end, so we can reuse the code
		 * that follows.
		 */
		if (plabel)
			*plabel = 0; /* see below */
		quote = MultiCodesetStrchr(value, 0);
	}

	/*
	 * Name ends at space(s) before the 1st quote (or before
	 * trailing space(s) if no quote.)
	 */
	while (isspace(quote[-1]) && quote > value)
		quote--;
	len = quote - value;
	*pname  = memcpy(XtMalloc(len+1), value, len);
	(*pname)[len] = 0;
	if (plabel && !*plabel)
		*plabel = XtNewString(*pname);

	return;
} /* ParseNameAndLabel */

/**
 ** FindLocale()
 **/

static Locale *
#if	OlNeedFunctionPrototypes
FindLocale (
	String			locale
)
#else
FindLocale (locale)
	String			locale;
#endif
{
	Cardinal		n;


	/*
	 * Can't use _OlArrayFind, because the "natural" compare function
	 * compares labels.
	 */
	for (n = 0; n < _OlArraySize(&locales); n++)
		if (MATCH(locale, _OlArrayElement(&locales, n).name))
			return (&_OlArrayElement(&locales, n));
	return (0);
} /* FindLocale */

/**
 ** LocaleCompare()
 **/

static int
#if	OlNeedFunctionPrototypes
LocaleCompare (
	Locale *		pa,
	Locale *		pb
)
#else
LocaleCompare (pa, pb)
	Locale *		pa;
	Locale *		pb;
#endif
{
	return (strcmp(pa->label, pb->label));
} /* LocaleCompare */

/**
 ** CreateLocaleListView()
 **/

static void
#if	OlNeedFunctionPrototypes
CreateLocaleListView (
	Locale *		locale,
	Widget			parent
)
#else
CreateLocaleListView (locale, parent)
	Locale *		locale;
	Widget			parent;
#endif
{
	LocaleListView *	pv	= &XnlLanguage;

	Locale *		p;

	Display *		dpy = XtDisplayOfObject(parent);

	Widget			sw;

	Cardinal		nitems;

	typedef struct It {
		XtArgVal		label;
		XtArgVal		user_data;
		XtArgVal		set;
	}			It;

	static It *		items = 0;

	static String		fields[] = {
		XtNlabel, XtNuserData, XtNset
	};


	items = (It *)XtRealloc((void *)items,
				(_OlArraySize(&locales) * sizeof(It)));

	for (nitems = 0; nitems < _OlArraySize(&locales); nitems++) {
		Locale *	p = &_OlArrayElement(&locales, nitems);

		items[nitems].label = (XtArgVal)p->label;
		items[nitems].user_data = (XtArgVal)p;
                if (p == locale) {
                        items[nitems].set = True;
                        pv->current = nitems;
                }
                else
                        items[nitems].set = False;
                p->item = nitems;
	}

	sw = XtVaCreateManagedWidget(
		"sw", scrolledWindowWidgetClass,
		CreateCaption(pv->name, OLG(basicSet,fixedString), parent),
		(String)0
	);

	pv->list = XtVaCreateManagedWidget(
		"list", flatListWidgetClass, sw,
		XtNitems,         (XtArgVal)items,
		XtNnumItems,      (XtArgVal)nitems,
		XtNitemFields,    (XtArgVal)fields,
		XtNnumItemFields, (XtArgVal)XtNumber(fields),
		XtNviewHeight,    (XtArgVal)3,
		XtNselectProc,    (XtArgVal)LocaleSelectCB,
		XtNclientData,    (XtArgVal)pv,
		(String)0
	);

	return;
} /* CreateLocaleListView */

/**
 ** LocaleSelectCB()
 **/

static void
#if	OlNeedFunctionPrototypes
LocaleSelectCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
LocaleSelectCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	LocaleListView *	pv  = (LocaleListView *)client_data;

	OlFlatCallData *	cd = (OlFlatCallData *)call_data;

	Locale *		locale;


	OlVaFlatGetValues (
		w, cd->item_index,
		XtNuserData, (XtArgVal)&locale,
		(String)0
	);
	NewLocale (pv, locale, OL_NORMAL);

	return;
} /* LocaleSelectCB */

/**
 ** Reset()
 **/

static void
#if	OlNeedFunctionPrototypes
Reset (
	Locale *		p,
	List *			resource_list
)
#else
Reset (p, resource_list)
	Locale *		p;
	List *			resource_list;
#endif
{
	if (!p)
		return;

	/*
	 * Attempt to switch to this locale; if the attempt ``failed''
	 * (i.e. we're already looking at this locale), then reset
	 * the specific and supplemental settings. (If the attempt
	 * ``succeeded'' then it already switched to default specific
	 * and supplemental settings.)
	 */
	if (!NewLocale(&XnlLanguage, p, OL_NONE)) {

#define VALUE(index) \
		((Resource *)list_element(resource_list, index))->value
#define _RESET(abbrev,index) \
		if (abbrev.exclusive.items) {				\
			SetExclusive (					\
				&abbrev.exclusive,			\
				ITEM(&abbrev.exclusive, VALUE(index)),	\
				OL_NONE					\
			);						\
			ShowCurrent (&abbrev.exclusive);		\
		} else

		_RESET (DisplayLang,    DISPLAYLANG);
		_RESET (InputLang,      INPUTLANG);
		_RESET (NumericFormat,  NUMERICFORMAT);
		_RESET (DateTimeFormat, DATETIMEFORMAT);
		_RESET (FontGroup,      FONTGROUP);
		_RESET (InputMethod,    INPUTMETHOD);

#undef	_RESET
#undef	VALUE
	}

	return;
} /* Reset */

/**
 ** NewLocale()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
NewLocale (
	LocaleListView *	pv,
	Locale *		p,
	OlDefine		change_state
)
#else
NewLocale (pv, p, change_state)
	LocaleListView *	pv;
	Locale *		p;
	OlDefine		change_state;
#endif
{
	Cardinal		new     = p->item;
	Cardinal		current = pv->current;


	_OlSetChangeBarState (pv->list, change_state, OL_PROPAGATE);

	if (current == new)
		return (False);

	OlVaFlatSetValues (
		pv->list, new,
		XtNset, (XtArgVal)True,
		(String)0
	);

	pv->current = new;

	/*
	 * Get rid of the old settings view and create a new one.
	 */
	CreateSettingsView (p, (Widget)0);

	return (True);
} /* NewLocale */

/**
 ** CreateSettingsView()
 **/

static void
#if	OlNeedFunctionPrototypes
CreateSettingsView (
	Locale *		locale,
	Widget			parent
)
#else
CreateSettingsView (locale, parent)
	Locale *		locale;
	Widget			parent;
#endif
{

        Display *		dpy;
	XFontStruct *		fs;

	/*
	 * On being called the first time, we save the view's widget ID.
	 * On being called again, we destroy the previous view and
	 * recreate it, using the same parent as before.
	 */
	if (SettingsView) {
		/*
		 * The XtUnmanageChild() keeps the old view from
		 * interfering with the geometry of the new view,
		 * XtDestroyWidget() just marks the widget for
		 * destruction, but does not remove it from a
		 * composite's list until we return to the main loop.
		 * Many composites (our own included, alas) don't ignore
		 * widgets being-destroyed when computing layout.
		 */
		parent = XtParent(SettingsView);
		XtUnmanageChild (SettingsView);
		XtDestroyWidget (SettingsView);
	}

	dpy = XtDisplayOfObject(parent);
	
	/*
	 * Populate the menus with the data from this locale.
	 */

#define _FETCH(abbrev,index) \
	FetchResourceAndParse (						\
		locale->db, &resources[index], &abbrev.exclusive	\
	);								\
	if (abbrev.exclusive.items)					\
		SetCurrent (						\
		  index, (ADDR)abbrev.exclusive.current_item->addr, True\
		)

	_FETCH (DisplayLang,    DISPLAYLANG);
	_FETCH (InputLang,      INPUTLANG);
	_FETCH (NumericFormat,  NUMERICFORMAT);
	_FETCH (DateTimeFormat, DATETIMEFORMAT);
	_FETCH (FontGroup,      FONTGROUP);
	_FETCH (InputMethod,    INPUTMETHOD);
#undef	_FETCH

	/*
	 * The pane that holds the view starts out unmanaged, to
	 * avoid silly base-window bounce as widgets get added.
	 */
	SettingsView = XtVaCreateWidget(
		"settingsView",
		controlAreaWidgetClass,
		parent,
		XtNshadowThickness,	(XtArgVal)0,
		XtNlayoutType,		(XtArgVal)OL_FIXEDCOLS,
		XtNallowChangeBars,	(XtArgVal)True,
		XtNalignCaptions,	(XtArgVal)True,
		(String)0
	);

	XtVaGetValues(XtParent(parent),
		      XtNcategoryFont, &fs,
		      (String) 0);

	XtVaCreateManagedWidget(
		"specificSettings",
		staticTextWidgetClass,
		SettingsView,
		XtNrecomputeSize,	(XtArgVal) False,
		XtNgravity,		(XtArgVal) WestGravity,
		XtNstring,		(XtArgVal) OLG(specSetting,fixedString),
		XtNfont,		(XtArgVal)fs,			
		(String)0
	);


	DisplayLang.label    = OLG(dispLang,fixedString);
	InputLang.label      = OLG(inputLang,fixedString);
	NumericFormat.label  = OLG(numFormat,fixedString);
	DateTimeFormat.label = OLG(dateTime,fixedString);
	
	CreateAbbreviated (&DisplayLang, SettingsView);
	CreateAbbreviated (&InputLang, SettingsView);
	CreateAbbreviated (&NumericFormat, SettingsView);
	CreateAbbreviated (&DateTimeFormat, SettingsView);

	if (FontGroup.exclusive.items || InputMethod.exclusive.items) {
		XtVaCreateManagedWidget(
			"supplementarySettings", staticTextWidgetClass,
			SettingsView, 
			XtNstring, (XtArgVal) OLG(suppSetting,fixedString),
			XtNfont,   (XtArgVal)fs,
			(String)0
		);
		if (FontGroup.exclusive.items) {
		        FontGroup.label = OLG(fontGroup,fixedString);
			CreateAbbreviated (&FontGroup, SettingsView);
		      }
		if (InputMethod.exclusive.items) {
		        InputMethod.label = OLG(inputMethod,fixedString);
			CreateAbbreviated (&InputMethod, SettingsView);
		      }
	}

	/*
	 * Now let the user see the view.
	 */
	XtManageChild (SettingsView);

	return;
} /* CreateSettingsView */

/**
 ** FetchResourceAndParse()
 **/

static void
#if	OlNeedFunctionPrototypes
FetchResourceAndParse (
	XrmDatabase		db,
	ResourceName *		resource,
	Exclusive *		pe
)
#else
FetchResourceAndParse (db, resource, pe)
	XrmDatabase		db;
	ResourceName *		resource;
	Exclusive *		pe;
#endif
{
	String			value;
	String			end;
	String			comma;
	String			name;
	String			label;

	ExclusiveItem		item;


	FreeExclusive (pe);

	if (!(value = GetResource(db, resource)) || !*value)
		goto Return;

	pe->items = alloc_List(sizeof(ExclusiveItem));

	do {
		comma = MultiCodesetStrchrWithQuotes(value, ',');
		if (comma)
			*comma = 0;
		ParseNameAndLabel (value, &name, &label);

		item.name       = (XtArgVal)label;
		item.addr       = (XtArgVal)name;
		list_append (pe->items, &item, 1);

		if (comma) {
			*comma = ',';
			value = comma + 1;
		}
	} while (comma);

Return:
	if (pe->items) {
		/*
		 * No particular rule for which is the current and the
		 * default, so make them the first item in the exclusive.
		 */
		pe->current_item =
		pe->default_item = (ExclusiveItem *)pe->items->entry;
	} else {
		pe->current_item =
		pe->default_item = 0;
	}

	return;
} /* FetchResourceAndParse */

/**
 ** InputMethodPartner()
 **/

static String
#if	OlNeedFunctionPrototypes
InputMethodPartner (
	Locale *		locale,
	String			im,
	Cardinal		partner
)
#else
InputMethodPartner (locale, im, partner)
	Locale *		locale;
	String			im;
	Cardinal		partner;
#endif
{
	String			im_xrm;
	String			im_partner_xrm;
	String			im_comma;
	String			im_partner_comma;
	String			ret	= 0;


	im_xrm         = GetResource(locale->db, &resources[INPUTMETHOD]);
	im_partner_xrm = GetResource(locale->db, &resources[partner]);
	if (!im_xrm || !im_partner_xrm)
		return (0);

	do {
		String			name;

#define NULL_COMMA(comma,value) \
		if ((comma = MultiCodesetStrchrWithQuotes(value, ',')))	\
			*comma = 0;
#define	SKIP_COMMA(comma,value) \
		if (comma) {						\
			*comma = ',';					\
			value = comma + 1;				\
		}

		NULL_COMMA (im_comma, im_xrm);
		NULL_COMMA (im_partner_comma, im_partner_xrm);

		ParseNameAndLabel (im_xrm, &name, (String *)0);
		if (MATCH(name, im))
			ret = XtNewString(im_partner_xrm);
		XtFree (name);

		SKIP_COMMA (im_comma, im_xrm);
		SKIP_COMMA (im_partner_comma, im_partner_xrm);

#undef	NULL_COMMA
#undef	SKIP_COMMA
	} while (im_comma && !ret);

	return (ret);
} /* InputMethodPartner */

/**
 ** FreeExclusive()
 **/

static void
#if	OlNeedFunctionPrototypes
FreeExclusive (
	Exclusive * 		exclusive
)
#else
FreeExclusive (exclusive)
	Exclusive * 		exclusive;
#endif
{
	ExclusiveItem *		p;

	list_ITERATOR		I;


	if (exclusive->items) {
		I = list_iterator(exclusive->items);
		while ((p = (ExclusiveItem *)list_next(&I))) {
			if (p->name)
				XtFree ((String)p->name);
			if (p->addr)
				XtFree ((String)p->addr);
		}
		free_List (exclusive->items);
		exclusive->items = 0;
	}

	return;
} /* FreeExclusive */

/**
 ** CreateAbbreviated()
 **/

static void
#if	OlNeedFunctionPrototypes
CreateAbbreviated (
	Abbreviated *		pa,
	Widget			parent
)
#else
CreateAbbreviated (pa, parent)
	Abbreviated *		pa;
	Widget			parent;
#endif
{
	Widget			menu_pane;
	Widget			rubber_tile;
	Screen *		screen	= XtScreenOfObject(parent);
	Display *		dpy     = XtDisplayOfObject(parent);


	rubber_tile = XtVaCreateManagedWidget(
		"rubberTile",
		rubberTileWidgetClass,
		CreateCaption(pa->name, pa->label, parent),
		XtNshadowThickness,	(XtArgVal)0,
		XtNorientation,		(XtArgVal)OL_HORIZONTAL,
		(String)0
	);

	menu_pane = XtVaCreatePopupShell (
		"pane",
		popupMenuShellWidgetClass,
		InitShell,
		(String)0
	);
	pa->button = XtVaCreateManagedWidget(
	      "abbrev",
	      abbreviatedButtonWidgetClass,
	      rubber_tile,
	      XtNpopupWidget,	(XtArgVal) menu_pane,
	      (String)0
	);
#define _7HorzPoints	OlScreenPointToPixel(OL_HORIZONTAL,7,screen)
	pa->show_current = XtVaCreateManagedWidget(
	      "showCurrent",
	      staticTextWidgetClass,
	      rubber_tile,
	      XtNrefName,	(XtArgVal) "abbrev",
	      XtNrefSpace,	(XtArgVal) _7HorzPoints,
	      (String)0
	);

	if (!pa->exclusive.items)
	    XtSetSensitive (pa->button, False);
	else {
	    Widget			w;
	    Dimension		width;

	    XtVaSetValues (
		    pa->button,
		    XtNpreviewWidget, pa->show_current,
		    (String)0
	    );

	    pa->exclusive.addr = (ADDR)pa;
	    pa->exclusive.f    = ShowCurrent;

	    CreateExclusive (menu_pane, &pa->exclusive, True);
	    choice_items[0].label = OLG(nextChoice,fixedString),

	    w = XtVaCreateManagedWidget(
		    "nextChoice",
		    flatButtonsWidgetClass,
		    menu_pane,
		    XtNitemFields,	(XtArgVal)choice_fields,
		    XtNnumItemFields,	(XtArgVal)XtNumber(choice_fields),
		    XtNitems,		(XtArgVal)choice_items,
		    XtNnumItems,	(XtArgVal)XtNumber(choice_items),
		    XtNdefault,		(XtArgVal)True,
		    (String)0
	    );

	    /*
	     * The menu looks a bit silly when the exclusives are
	     * narrower than the next-choice button.
	     */
	    XtVaGetValues (w, XtNwidth, (XtArgVal)&width, (String)0);
	    XtVaSetValues (
		    pa->exclusive.w,
		    XtNitemMinWidth, (XtArgVal)width,
		    (String)0
	    );

	    ShowCurrent (&pa->exclusive);
	}

	return;
} /* CreateAbbreviated */

/**
 ** ShowCurrent()
 **/

static void
#if	OlNeedFunctionPrototypes
ShowCurrent (
	Exclusive *		pe
)
#else
ShowCurrent (pe)
	Exclusive *		pe;
#endif
{
	Abbreviated *		pa = (Abbreviated *)pe->addr;

	ExclusiveItem *		pi = pe->current_item;


	XtVaSetValues (
		pa->show_current,
		XtNstring, (XtArgVal)(pi? (String)pi->name : ""),
		(String)0
	);

	return;
} /* ShowCurrent */

/**
 ** NextChoiceCB()
 **/

static void
#if	OlNeedFunctionPrototypes
NextChoiceCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
NextChoiceCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Abbreviated *		pa = (Abbreviated *)client_data;

	Exclusive *		pe = &(pa->exclusive);

	Cardinal		next;


	/*
	 * Selecting the ``Next choice'' button is identical to
	 * selecting the choice after the current choice, so use
	 * the same code-path. Note that when activating a sub-item,
	 * the indexes go from 1 to n, not 0 to n-1; so add 1 to
	 * what we think is the next item's index.
	 */
	next = (pe->current_item - (ExclusiveItem *)pe->items->entry) + 1;
	if (next >= pe->items->count)
		next = 0;
	next++;
	OlActivateWidget (pe->w, OL_SELECTKEY, (XtPointer)next);

	return;
} /* NextChoiceCB */

/**
 ** OpenDisplayForNewLocale()
 **/

static String
#if	OlNeedFunctionPrototypes
LanguageProc (
	Display *		display,
	String			language,
	XtPointer		client_data
)
#else
LanguageProc (display, language, client_data)
	Display *		display;
	String			language;
	XtPointer		client_data;
#endif
{
	return (language);
} /* LanguageProc */

static Display *
#if	OlNeedFunctionPrototypes
OpenDisplayForNewLocale (
	String			locale
)
#else
OpenDisplayForNewLocale (locale)
	String			locale;
#endif
{
	int			argc;

	String			name;
	String			class;
	String			argv[8];

	XtAppContext		app = XtDisplayToApplicationContext(DISPLAY);

#if	defined(XtSpecificationRelease) && XtSpecificationRelease >= 5
	XtLanguageProc		proc;
#endif


	if (MATCH(CurrentLocale, locale))
		return (DISPLAY);

	if (SeparateDisplayForNewLocale) {
		if (MATCH(SeparateLocale, locale))
			return (SeparateDisplayForNewLocale);
		CloseDisplay (SeparateDisplayForNewLocale);
	}

	/*
	 * DON'T pass display string to XtOpenDisplay() as the second parameter.
	 * instead, put the display string in the arg list. This will force
	 * Xt to parse the arg list first, and so will set the language
	 * correctly.
	 */
	XtGetApplicationNameAndClass (DISPLAY, &name, &class);
	argc = (int)XtNumber(argv)-1;
	argv[0] = name;
	argv[1] = "-xnllanguage";
	argv[2] = locale;
	argv[3] = "-xrm";
	argv[4] = "*customization:%C";
	argv[5] = "-display";
	argv[6] = DisplayString(DISPLAY);
	argv[7] = 0;
	SeparateDisplayForNewLocale = XtOpenDisplay(
		app, NULL,
		name, class,
		(XrmOptionDescRec *)0, (Cardinal)0,
		&argc, argv
	);
	SeparateLocale = XtNewString(locale);

	return (SeparateDisplayForNewLocale);
} /* OpenDisplayForNewLocale */

/**
 ** CloseDisplay()
 **/

static void
#if	OlNeedFunctionPrototypes
CloseDisplay (
	Display *		display
)
#else
CloseDisplay (display)
	Display *		display;
#endif
{
	if (display != DISPLAY) {
		XtCloseDisplay (display);
		if (display == SeparateDisplayForNewLocale) {
			SeparateDisplayForNewLocale = 0;
			XtFree (SeparateLocale);
		}
	}

	return;
} /* CloseDisplay */

/**
 ** FillInLangSubs()
 **
 ** Stolen from Xt/Intrinsic.c
 **/

static void
#if	OlNeedFunctionPrototypes
FillInLangSubs (
	Substitution		subs,
	String			string
)
#else
FillInLangSubs (subs, string)
	Substitution		subs;
	String			string;
#endif
{
	Cardinal		len;

	String			p1;
	String			p2;
	String			p3;
	String			ch;
	String *		rest;


	if (!string || !string[0]) {
		subs[0].substitution =
		subs[1].substitution =
		subs[2].substitution =
		subs[3].substitution = 0;
		return;
	}

	len = strlen(string) + 1;
	subs[0].substitution = string;
	p1 = subs[1].substitution = XtMalloc(3*len);
	p2 = subs[2].substitution = subs[1].substitution + len;
	p3 = subs[3].substitution = subs[2].substitution + len;

	/*
	 * Everything up to the first "_" goes into p1.
	 * From "_" to "." in p2.  The rest in p3.
	 * If no delimiters, all goes into p1.
	 * We assume p1, p2, and p3 are large enough.
	 */
	*p1 = *p2 = *p3 = 0;

	ch = strchr(string, '_');
	if (ch) {
		len = ch - string;
		(void) strncpy(p1, string, len);
		p1[len] = 0;
		string = ch + 1;
		rest = &p2;
	} else
		rest = &p1;

	/*
	 * "rest" points to where we put the first part.
	 */
	ch = strchr(string, '.');
	if (ch) {
		len = ch - string;
		strncpy(*rest, string, len);
		(*rest)[len] = '\0';
		(void) strcpy(p3, ch+1);
	} else
		(void) strcpy(*rest, string);

	return;
} /* FillInLangSubs */
