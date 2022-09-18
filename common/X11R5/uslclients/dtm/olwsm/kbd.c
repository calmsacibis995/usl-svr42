/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma	ident	"@(#)dtm:olwsm/kbd.c	1.43"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLookI.h>
#include <Xol/Category.h>
#include <Xol/DynamicP.h>
#include <Xol/ControlAre.h>
#include <Xol/ChangeBar.h>
#include <Xol/TextEdit.h>
#include <Xol/Notice.h>
#include <Xol/FButtons.h>
#include <Xol/Modal.h>

#include <FKeys.h>
#include "error.h"
#include <misc.h>
#include <node.h>
#include <list.h>
#include <notice.h>
#include <menu.h>
#include <exclusive.h>
#include <property.h>
#include <resource.h>
#include <xtarg.h>
#include <wsm.h>

extern char * NoticeMenuFields[];

/*
 * Special types:
 */

	/*
	 * MORE: Type M is treated just like K or E, except
	 * the header differs. Should allow user to enter just
	 * modifiers.
	 */
typedef enum KeyType {
	K = 0,	/* full key:    Modifier<KeySym> */
	E = 1,	/* full key:    Modifier<KeySym> */
	M = 2,	/* partial key: Modifier         */ /* MORE: see above */
	U = 3	/* unknown                       */
}			KeyType;

/* PERF: In performance improvement work (April, 1992), we changed sizes */
/*       of caption from BUFSIZE (1024) to 60 and value from 160 to 120. */
/*       "caption" is just a key redefinition caption (e.g. "up:").      */
/*       "value" stores the encoded primary and alternate key sequences, */
/*       and the longest ones I could create were still under 50 bytes.  */
/*       Even with 2-byte chars, 120 bytes should be more than enough.   */
/*       This saves a great deal of malloc'ed storage, but both are      */
/*       still too large.  Worse, a constant size allocation is always   */
/*       dangerous, no matter how big.  The RIGHT fix is to use char *   */
/*       (pointers here, then malloc the appropriate sizes as needed.    */
/*       This would require code changes beyond the scope of this        */
/*       performance effort, so the "quick and dirty" fix was used. [AS] */

typedef struct Key {
	KeyType			type;
	char			name[40];
	/* char		caption[BUFSIZ];		  /** see PERF above */
	/* char		value[160]; /* 80 2-byte chars */ /** see PERF above */
	char			caption[60];
	char			value[120];   /* 60 2-byte chars */
}			Key;

typedef struct KeyItem {
	XtArgVal		is_header;
	XtArgVal		traversal_on;
	XtArgVal		change_bar;
	XtArgVal		caption;
	XtArgVal		label;
}			KeyItem;

typedef struct _KeyList {
	Key *			keys;
	Cardinal		num_keys;
	KeyItem *		_items[2];
	Cardinal		_nitems;
	Cardinal *		items[2];
	String *		current;
	Resource *		_rlist;
	List			rlist;
	Widget			w[2];
	Boolean			check;
}			KeyList;

/*
 * Local functions:
 */

static void		Import OL_ARGS((
	XtPointer		closure
));
static void		Export OL_ARGS((
	XtPointer		closure
));
static void		SetKbdLabels OL_ARGS((
        Display *               dpy
));
static unsigned char *	FindDelimiter OL_ARGS((
	unsigned char *		str
));
#ifndef SVR4
static unsigned char *	FindSubString OL_ARGS((
	unsigned char *		src,
	unsigned char *		substring
));
#endif
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
static void		KeyChangedCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
static Cardinal		Separate OL_ARGS((
	String			value,
	String *		p1,
	String *		p2,
	Boolean			allocate
));
static String		Combine OL_ARGS((
	String			s0,
	String			s1
));
static Property *	CheckForDuplicates OL_ARGS((
	Widget			w
));
static int		CmpKeys OL_ARGS((
	const void *		_pa,
	const void *		_pb
));
static void		AbortCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
#ifdef DUP_BINDINGS_FIXED_BY_USER_IN_FUTURE
static void		ContinueCB OL_ARGS((
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
));
#endif /* DUP_BINDINGS_FIXED_BY_USER_IN_FUTURE */
static Cardinal		InsertPosition OL_ARGS((
	Widget			w
));

void			setUpKbdCaptions OL_ARGS((
        Display *               dpy
));
static void		setUpLabels OL_ARGS ((
        Display	*		dpy,
        Key **			k,
        KeyList *		kl,
        char *			str
));
static void		getInfo OL_ARGS ((
        Key *			k,
        KeyList *		kl,
        unsigned char *		str,
        int			count
));


/*
 * Convenient macros:
 */

#if	!defined(Strlen)
# define Strlen(S) (S && *S? strlen((S)) : 0)
#endif

#define GLOBAL(pk,x) \
	resource_value(&global_resources, (pk)->keys[(x)].name)

#define PROP	PropertyList[i]

/*
 * Local data:
 */

static String		fields[] = {
	XtNisHeader,
	XtNtraversalOn,
	XtNchangeBar,
	XtNcaption,
	XtNlabel,
};

static XrmDatabase keyDB = NULL;

/* Holders for some OlGetMessage calls */

Property *	*PropertyList;

int numKbdSheets = 0;

#define KBD_DELIMITER	";;"

static String		HeaderCaption[3] = {
	"Function",
	"Mouse Equivalents",
	"\nMouse Modifiers",
};

static String		Header[3][2] = {
	"Primary\nKey Sequence",	"Alternate\nKey Sequence",
	"Primary\nKey Sequence",	"Alternate\nKey Sequence",
	"\n ",				"\n ",
};

static NoticeItem	keyNoticeItems[] = {
#ifdef DUP_BINDINGS_FIXED_BY_USER_IN_FUTURE
	{ (XtArgVal) False, (XtArgVal) ContinueCB, (XtArgVal) "Continue", 'C' },
	{ (XtArgVal) True,  (XtArgVal) AbortCB,       (XtArgVal) "Abort", 'A' },
#else		
	{ (XtArgVal) True,  (XtArgVal) AbortCB,       (XtArgVal) "Cancel", 'C' },
#endif /* DUP_BINDINGS_FIXED_BY_USER_IN_FUTURE */
};
#ifdef DUP_BINDINGS_FIXED_BY_USER_IN_FUTURE
static Notice	notice       = { "keyComplaint", 0, &keyNoticeItems, 2 };
#else		
static Notice	notice       = { "keyComplaint", 0, &keyNoticeItems, 1 };
#endif /* DUP_BINDINGS_FIXED_BY_USER_IN_FUTURE */

static OlDtHelpInfo KeysHelp = {
	NULL, NULL, "DesktopMgr/keypref.hlp", NULL, NULL
};

/**
 ** setUpKbdCaptions()
 **/

void setUpKbdCaptions(dpy)
Display *dpy;
{
	static int keysDone = 0;
	char *sheets;
	int num_sheets, i;

	if (!keysDone) {

		SetKbdLabels(dpy);

		/* Open the keys database */
		keyDB = (XrmDatabase)OlOpenDatabase(dpy, "olwsm_keys");

		/* Get the number of sheets, allocate, then get the sheets list */
		num_sheets = atoi(OlGetMessage(dpy, NULL, BUFSIZ, "Wsmkeys", "num",
				   OleCOlClientOlwsmMsgs, "0", keyDB));
		PropertyList = (Property **)malloc(sizeof(Property *) * num_sheets);

		sheets = OlGetMessage(dpy, NULL, BUFSIZ, "Wsmkeys", "sheets",
			OleCOlClientOlwsmMsgs, NULL, keyDB);

		for (i = 0; i < num_sheets; i++) {
			KeyList *kl;
      
			PROP = (Property *)malloc(sizeof(Property));

			/* Store the name and helpfiles and update the pointer */
			PROP->name = XtMalloc(80);
			PROP->pLabel = XtMalloc(80);

			if (i == 0)
				strcpy(PROP->name, strtok(sheets, "\t"));
			else
				strcpy(PROP->name, strtok(NULL, "\t"));
			strcpy(PROP->pLabel, strtok(NULL, "\t"));
			PROP->mnemonic = (strtok(NULL, ";"))[0];
			PROP->help = &KeysHelp;

			/* Assign the fields common to all kbd property sheets */
			PROP->args = (ArgList)0;
			PROP->num_args = 0;
			PROP->import = Import;
			PROP->export = Export;
			PROP->create = Create;
			PROP->apply = ApplyCB;
			PROP->reset = ResetCB;
			PROP->factory = FactoryCB;
			PROP->popdown = PopdownCB;
			PROP->footer = OLG(initKeyHelp,footerMsg);

			PROP->closure = (XtPointer)malloc(sizeof(KeyList));
			kl = (KeyList *)PROP->closure;
			kl->_items[0] = 0;
			kl->_items[1] = 0;

			/* Get the data for each key in the property sheet */
			setUpLabels(dpy, &(kl->keys), kl, PROP->name);
		}

		/* close the key database */
		OlCloseDatabase(keyDB);
		keysDone = 1;
	}
	numKbdSheets = num_sheets;	/* for other files to use */
}

static void
#if	OlNeedFunctionPrototypes
setUpLabels (
	     Display		*dpy,
	     Key		**k,
	     KeyList		*kl,
	     char		*str
)
#else
setUpLabels(dpy, k, kl, str)
Display *dpy;
Key **k;
KeyList *kl;
char *str;
#endif  
{
  int count;
  unsigned char *tmp;

  /*
   * Get the number of keys for the sheet, allocate the space, get the
   * list of keys, then parse and store the list.
   */
  
  count = atoi(OlGetMessage(dpy, NULL, BUFSIZ,
			    str, "num",
			    OleCOlClientOlwsmMsgs,
			    "0", keyDB));

  *k = (Key *)malloc(sizeof(Key) * count);
  
  tmp = (unsigned char *)OlGetMessage(dpy, NULL, BUFSIZ, str, "keys",
				      OleCOlClientOlwsmMsgs, NULL, keyDB);

  getInfo(*k, kl, tmp, count);
}

static void
#if	OlNeedFunctionPrototypes
getInfo (
	 Key			*k,
	 KeyList		*kl,
	 unsigned char		*str,
	 int			count
)
#else
getInfo(k, kl, str, count)
Key *k;
KeyList *kl;
unsigned char *str;
int count;
#endif  
{
  int i, n;
  unsigned char *delim;

  for (n = 0; n < count; n++) {
/*    char *tmp;

    tmp = strtok((char *)*str, " ");*/

    /* assign the type */
    
    switch(str[0]) {
/*    switch(tmp[0]) {*/
    case 'K': k[n].type = K; break;
    case 'E': k[n].type = E; break;
    case 'M': k[n].type = M; break;
    case 'U': k[n].type = U; break;
    default: break;
    }

    /* copy in the name and binding.  Since the binding may have several */
    /* words, just dump the rest of the list into it, then find the ; */
    /* delimiter and cut it off there.  The list will now start at that */
    /* point (which is the key caption). */
    
    sscanf((char *)&str[2], "%s", k[n].name);
    strcpy(k[n].value, (char *)&str[3+strlen(k[n].name)]);
/*    k[n].name = strtok(NULL, " ");
    k[n].value = strtok(NULL, ";");*/

    for (i = 0; k[n].value[i] != ';'; i++);
    
    str = (unsigned char *)&str[4+strlen(k[n].name)+i];
    k[n].value[i] = '\0';
/*    *str = (unsigned char *)k[n].value[strlen(k[n].value)+1];*/

    strcpy(k[n].caption,"");

    /* Now, get the whole caption text.  It ends with the KBD_DELIMITER.
     * Since we parse the whole string, the string becomes the part after
     * the delimiter.
     */
    
    delim = FindDelimiter(str);
    delim[0] = '\0';
    strcpy(k[n].caption, (char *)str);
    str = &(delim[2]);

  } 

  /* assign the keys to the keylist */
  
  kl->keys = k;
  kl->num_keys = count;
  kl->current = NULL;
}

static unsigned char *
#if	OlNeedFunctionPrototypes
FindDelimiter (
	     unsigned char 	*str
)
#else
FindDelimiter (str)
  unsigned char *str;
#endif  
{
  unsigned char *before;		/* holds before OlGetNextStrSeg */
  unsigned char *after;			/* holds after OlGetNextStrSeg */
  int len;
  static firsttime = 1;
  static OlFontList *olfl;
  OlStrSegment olss;
  static XtResource flr[] = {
    { XtNfontGroup, XtCFontGroup, XtROlFontList, sizeof(OlFontList *),
	0, XtRString, (XtPointer)NULL },
  };

  after = str;
  
  /* Get the font list structure */

  if (firsttime) {
    firsttime = 0;
    OlGetApplicationResources(InitShell, &olfl, flr, 1, NULL, 0);
  }

  while (1) {
    unsigned char tmpbuf[BUFSIZ];
    
    before = after;			/* update string pointer */
    olss.str = tmpbuf;
    olss.len = len = 0;
    len = strlen((char *)after);
      
    if (olfl)
      OlGetNextStrSegment(olfl,&olss,&after,&len);
    else 			/* must be ascii only if NULL fontlist */
      olss.code_set = 0;
  
    if (!olss.code_set) {	/* ascii */
      unsigned char buf[BUFSIZ];
      unsigned char *delim;

      /* copy only the part used in OlGetNextStrSeg.  We need a pointer
       * to the input, not the stripeed olss.str.
       */

      if (olfl) {
	strncpy((char *)buf, (char *)before, after - before);
	buf[after - before] = '\0';
      }
      else
	strcpy((char *)buf, (char *)after);

      /* find the delimiter, if it exists */

#ifdef SVR4      
      delim = (unsigned char *)strstr((char *)buf, KBD_DELIMITER);
#else
      delim = FindSubString(buf, (unsigned char *)KBD_DELIMITER);
#endif
      /* if found it, return a pointer to the delimiter in the ORIGINAL
       * string, not from the temporary buffer.
       */
      
      if (delim)
	return &(before[delim - buf]);
    }

    /* if not ascii, cannot have delimiter, so do nothing */
  }
  /*NOTREACHED*/
  return NULL;
}

#ifndef SVR4
static unsigned char *
#if	OlNeedFunctionPrototypes
FindSubString (
	unsigned char *		src,
	unsigned char *		substring
)
#else
FindSubString (src, substring)
	unsigned char *		src;
	unsigned char *		substring;
#endif
{
	Cardinal		n	= strlen((char *)substring);

	while (*src) {
		if (strncmp((char *)src, (char *)substring, n) == 0)
			return (src);
		src++;
	}
	return (0);
} /* FindSubString */
#endif /* SVR4 */
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
	KeyList *		pk = (KeyList *)closure;

	Cardinal		i;


	pk->_rlist = ARRAY(Resource, pk->num_keys);
	pk->rlist.entry = (ADDR)pk->_rlist;
	pk->rlist.size  = sizeof(Resource);
	pk->rlist.count = pk->num_keys;
	pk->rlist.max   = 0;
	pk->check       = True;

	for (i = 0; i < pk->num_keys; ++i) {
		pk->_rlist[i].name  = pk->keys[i].name;
		pk->_rlist[i].value = pk->keys[i].value;
	}

	merge_resources (&global_resources, &pk->rlist);

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
	KeyList *		pk = (KeyList *)closure;

	Cardinal		i;


	if (!pk->current)
		pk->current = ARRAY(String, pk->num_keys);
	for (i = 0; i < pk->num_keys; ++i)
		pk->current[i] = STRDUP(GLOBAL(pk, i));

	return;
} /* Export */

static void
#if	OlNeedFunctionPrototypes  
SetKbdLabels(
	     Display *dpy
)
#else
SetKbdLabels(dpy)
  Display *dpy;
#endif
{
  Cardinal n = 0;
  static short setupDone = 0;

  if (!setupDone) {
    HeaderCaption[0] = OLG(function,fixedString);
    HeaderCaption[1] = OLG(mouseEq,fixedString);
    HeaderCaption[2] = OLG(mouseMod,fixedString);
    Header[0][0] = Header[1][0] = OLG(primary,fixedString);
    Header[0][1] = Header[1][1] = OLG(alternate,fixedString);
    setupDone = 1;
  }
}

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
	KeyList *		pk = (KeyList *)closure;

	KeyType			type;

	Cardinal		i;
	Cardinal		n;

	String			string0;
	String			string1;

        Screen *		screen	  = XtScreenOfObject(work);
	Display *		dpy = XtDisplay(work);
	String temp;
	static Bool firsttime = TRUE;

#ifdef DUP_BINDINGS_FIXED_BY_USER_IN_FUTURE
	if (firsttime) {
	  notice.items[0].string = (XtArgVal)OLG(continue,fixedString);
	  temp = OLG(continue,mnemonic);
	  notice.items[0].mnemonic = (XtArgVal)temp[0];
	  notice.items[1].string = (XtArgVal)OLG(abort,fixedString);
	  temp = OLG(abort,mnemonic);
	  notice.items[1].mnemonic = (XtArgVal)temp[0];
	  firsttime = FALSE;
	}
#else
	if (firsttime) {
	  notice.items[0].string = (XtArgVal)OLG(cancel,fixedString);
	  temp = OLG(cancel,mnemonic);
	  notice.items[0].mnemonic = (XtArgVal)temp[0];
	  firsttime = FALSE;
	}
#endif /* DUP_BINDINGS_FIXED_BY_USER_IN_FUTURE */

	/*
	 * Count how many flat items we need. This will be more
	 * than the number of keys on the property sheet, as we
	 * include the headers in the count.
	 *
	 * Once counted, allocate two arrays, one for primary,
	 * one for alternate.
	 */
	for (type = U, pk->_nitems = i = 0; i < pk->num_keys; ++i) {
		if (pk->keys[i].type != type) {
			type = pk->keys[i].type;
			pk->_nitems++;
		}
		pk->_nitems++;
	}
	pk->_items[0] = ARRAY(KeyItem, pk->_nitems);
	pk->_items[1] = ARRAY(KeyItem, pk->_nitems);

	/*
	 * Fill the arrays. Again, headers are included as ``items''.
	 * Keep track of indexes to real items in another array, for
	 * easier access after this.
	 */
	pk->items[0] = ARRAY(Cardinal, pk->num_keys);
	pk->items[1] = ARRAY(Cardinal, pk->num_keys);
	for (type = U, n = i = 0; i < pk->num_keys; ++i) {

		if (pk->keys[i].type != type) {
			type = pk->keys[i].type;
#define HEADER(C) pk->_items[C][n]

			HEADER(0).is_header    = (XtArgVal)True;
			HEADER(0).traversal_on = (XtArgVal)False;
			HEADER(0).change_bar   = (XtArgVal)OL_NONE;
			HEADER(0).caption      =
				   (XtArgVal)HeaderCaption[type];
			HEADER(0).label	       =
				   (XtArgVal)STRDUP(Header[type][0]);

			HEADER(1).is_header    = (XtArgVal)True;
			HEADER(1).traversal_on = (XtArgVal)False;
			HEADER(1).change_bar   = (XtArgVal)OL_NONE;
			HEADER(1).caption      = (XtArgVal)0;
			HEADER(1).label		=
				   (XtArgVal)STRDUP(Header[type][1]);
#undef HEADER
			n++;
		}

#define ITEM(C) pk->_items[C][n]
		pk->items[0][i] = n;
		pk->items[1][i] = n;

		Separate (pk->current[i], &string0, &string1, True);

		ITEM(0).is_header    = (XtArgVal)False;
		ITEM(0).traversal_on = (XtArgVal)True;
		ITEM(0).change_bar   = (XtArgVal)OL_NONE;
		ITEM(0).caption      = (XtArgVal)pk->keys[i].caption;
		ITEM(0).label        = (XtArgVal)string0;

		ITEM(1).is_header    = (XtArgVal)False;
		ITEM(1).traversal_on = (XtArgVal)True;
		ITEM(1).change_bar   = (XtArgVal)OL_NONE;
		ITEM(1).caption      = (XtArgVal)0;
		ITEM(1).label        = (XtArgVal)string1;
#undef ITEM
		n++;
	}

#define _4HorzPoints	OlScreenPointToPixel(OL_HORIZONTAL,4,screen)
#define _4VertPoints	OlScreenPointToPixel(OL_VERTICAL,4,screen)
#define _2VertPoints	OlScreenPointToPixel(OL_VERTICAL,2,screen)

	pk->w[0] = XtVaCreateManagedWidget(
		"keys1",
		flatKeysWidgetClass,
		work,
		XtNitemFields,      (XtArgVal)fields,
		XtNnumItemFields,   (XtArgVal)XtNumber(fields),
		XtNitems,           (XtArgVal)pk->_items[0],
		XtNnumItems,        (XtArgVal)pk->_nitems,
		XtNallowChangeBars, (XtArgVal)True,
		XtNgravity,	    (XtArgVal)EastGravity,
		XtNlayoutWidth,	    (XtArgVal)OL_MAXIMIZE,
		XtNlayoutHeight,    (XtArgVal)OL_MAXIMIZE,
		XtNvPad,	    (XtArgVal)_4VertPoints,
		XtNhPad,	    (XtArgVal)_4HorzPoints,
		XtNvSpace,	    (XtArgVal)_2VertPoints,
		(String)0
	);
	pk->w[1] = XtVaCreateManagedWidget(
		"keys2",
		flatKeysWidgetClass,
		work,
		XtNitemFields,      (XtArgVal)fields,
		XtNnumItemFields,   (XtArgVal)XtNumber(fields),
		XtNitems,           (XtArgVal)pk->_items[1],
		XtNnumItems,        (XtArgVal)pk->_nitems,
		XtNallowChangeBars, (XtArgVal)False,
		XtNgravity,	    (XtArgVal)WestGravity,
		XtNlayoutWidth,	    (XtArgVal)OL_MAXIMIZE,
		XtNlayoutHeight,    (XtArgVal)OL_MAXIMIZE,
		XtNvPad,	    (XtArgVal)_4VertPoints,
		XtNhPad,	    (XtArgVal)_4HorzPoints,
		XtNvSpace,	    (XtArgVal)_2VertPoints,
		(String)0
	);
	XtAddCallback (pk->w[0], XtNkeyChanged, KeyChangedCB, (XtPointer)pk);
	XtAddCallback (pk->w[1], XtNkeyChanged, KeyChangedCB, (XtPointer)pk);

	/*
	 * Since we're being created, either we've never existed before
	 * or we have but we were destroyed (e.g. the user pulled the
	 * pin!) The latter case is important, as we may have residual
	 * things to clean up.
	 *
	 * MORE: Add a Cleanup method to each property sheet that will
	 * do this at destroy time!
	 */

	notice.w = 0;	/* DestroyPropertyPopup already killed it */
/*	if (notice.items) {
	  if (notice.items[0])
	    XtFree(notice.items[0].string);
	  if (notice.items[1])
	    XtFree(notice.items[1].string);
	}*/

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
	KeyList *		pk = (KeyList *)closure;

	Property *		p;

	Cardinal		i;

	static ApplyReturn	ret;


	/*
	 * Look for duplicate bindings. The checking routine returns
	 * the first sheet that has a duplicate binding, or NULL if
	 * no duplicates.
	 */
	if (pk->check) {
		p = CheckForDuplicates(w);
		if (p) {
			ret.bad_sheet = p;
			ret.reason    = APPLY_NOTICE;
			ret.u.notice  = &notice;
			return (&ret);
		}
	}
	pk->check = True;

#define LABEL(C,I) (String)(pk->_items[C][pk->items[C][I]].label)

	for (i = 0; i < pk->num_keys; ++i) {
		if (pk->current[i])
			FREE (pk->current[i]);

		pk->_rlist[i].value =
		pk->current[i] = Combine(LABEL(0,i), LABEL(1,i));

		_OlFlatSetChangeBarState (
			pk->w[0], pk->items[0][i], OL_NONE, OL_PROPAGATE
		);
	}
	merge_resources (&global_resources, &pk->rlist);
	ret.reason = APPLY_OK;

#undef LABEL

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
	KeyList *		pk = (KeyList *)closure;

	Cardinal		i;

	String			string0;
	String			string1;


	for (i = 0; i < pk->num_keys; ++i) {
		Separate (pk->current[i], &string0, &string1, True);

		OlVaFlatSetValues (
			pk->w[0],
			pk->items[0][i],
			XtNlabel,     (XtArgVal)string0,
			(String)0
		);
		OlVaFlatSetValues (
			pk->w[1],
			pk->items[1][i],
			XtNlabel,     (XtArgVal)string1,
			(String)0
		);
		_OlFlatSetChangeBarState (
			pk->w[0], pk->items[0][i], OL_NONE, OL_PROPAGATE
		);
	}

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
	KeyList *		pk = (KeyList *)closure;

	Cardinal		i;

	String			string0;
	String			string1;


	for (i = 0; i < pk->num_keys; ++i) {
		Separate (pk->keys[i].value, &string0, &string1, True);

		OlVaFlatSetValues (
			pk->w[0],
			pk->items[0][i],
			XtNlabel,     (XtArgVal)string0,
			(String)0
		);
		OlVaFlatSetValues (
			pk->w[1],
			pk->items[1][i],
			XtNlabel,     (XtArgVal)string1,
			(String)0
		);
		_OlFlatSetChangeBarState (
			pk->w[0], pk->items[0][i], OL_NORMAL, OL_PROPAGATE
		);

	}

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
	KeyList *		pk = (KeyList *)closure;

	Cardinal		n;


#define LABEL(C,N) pk->_items[C][N].label
	for (n = 0; n < pk->_nitems; n++) {
		if (LABEL(0,n))
			FREE (LABEL(0,n));
		if (LABEL(1,n))
			FREE (LABEL(1,n));
	}
#undef LABEL
	FREE (pk->_items[0]);
	FREE (pk->_items[1]);
	FREE (pk->items[0]);
	FREE (pk->items[1]);
	pk->_items[0] = 0;
	pk->_items[1] = 0;
	pk->items[0]  = 0;
	pk->items[1]  = 0;

	/*
	 * "->current" and "->_rlist" are not freed, as they were
	 * permanently allocated in the Import and Export procedures.
	 */

	return;
} /* PopdownCB */

/**
 ** KeyChangedCB()
 **/

static void
#if	OlNeedFunctionPrototypes
KeyChangedCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
KeyChangedCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	KeyList *		pk = (KeyList *)client_data;

	OlFlatKeyChanged *	cd = (OlFlatKeyChanged *)call_data;

	OlDefine		changebar;


	switch (cd->change) {

	case OL_FLATKEY_UNDONE:
		FooterMessage (w, 0, OL_LEFT, False);
		FooterMessage (w, 0, OL_RIGHT, False);
		return;

	case OL_FLATKEY_CHANGING:
		FooterMessage (w, Dm__gettxt(TXT_footerMsg_nowPress),
			       OL_LEFT, False);
		FooterMessage (w, Dm__gettxt(TXT_footerMsg_quoting),
			       OL_RIGHT, False);
		changebar = OL_DIM;
		break;

	case OL_FLATKEY_DELETED:
		/*
		 * Don't let the user remove a Primary Key Sequence.
		 */
		if (w == pk->w[0]) {
			FooterMessage (w, Dm__gettxt(TXT_footerMsg_deletePrime),
				OL_LEFT,
				True
			);
			FooterMessage (w, 0, OL_RIGHT, False);
			cd->ok = False;
			return;
		}
		/*FALLTHROUGH*/

	case OL_FLATKEY_CHANGED:
	case OL_FLATKEY_ABORTED:
		FooterMessage (w, 0, OL_LEFT, False);
		FooterMessage (w, 0, OL_RIGHT, False);
		changebar = OL_NORMAL;
		break;
	}

	_OlFlatSetChangeBarState (
		pk->w[0], cd->index, changebar, OL_PROPAGATE
	);

	return;
} /* KeyChangedCB */

/**
 ** Separate()
 **/

static Cardinal
#if	OlNeedFunctionPrototypes
Separate (
	String			value,
	String *		p1,
	String *		p2,
	Boolean			allocate
)
#else
Separate (value, p1, p2, allocate)
	String			value;
	String *		p1;
	String *		p2;
	Boolean			allocate;
#endif
{
	/*
	 * MORE: Allow for this input:
	 *
	 *	Modifiers<,>,Modifiers<Key>
	 */

	String			comma = strchr(value, ',');

	Cardinal		n;


	if (!comma) {
		n = Strlen(value);
		*p1 = (allocate? STRDUP(value) : value);
		*p2 = 0;
	} else {
		n = comma - value;
		if (allocate) {
			*p1 = strncpy(MALLOC(n+1), value, n);
			(*p1)[n] = 0;
			*p2 = STRDUP(comma + 1);
		} else {
			*p1 = value;
			*p2 = comma + 1;
		}
	}

	return (n);
} /* Separate */

/**
 ** Combine()
 **/

static String
#if	OlNeedFunctionPrototypes
Combine (
	String			s0,
	String			s1
)
#else
Combine (s0, s1)
	String			s0;
	String			s1;
#endif
{
	Cardinal		len;

	String			ret;


	len = Strlen(s0) + 1;		/* +1 for comma or null */
	if (s1)
		len += Strlen(s1) + 1;	/* +1 for null */
	ret = MALLOC(len);
	strcpy (ret, s0);
	if (s1) {
		strcat (ret, ",");
		strcat (ret, s1);
	}

	return (ret);
} /* Combine */

/**
 ** CheckForDuplicates()
 **/

typedef struct KItem {
	String			binding;
	unsigned char		sheet;
	unsigned char		key;
	unsigned char		length;
}			KItem;

static Property *
#if	OlNeedFunctionPrototypes
CheckForDuplicates (
	Widget			w
)
#else
CheckForDuplicates (w)
	Widget			w;
#endif
{
#define FORMAT	"\n    %.*s\t%.*s\t%.*s"

	/*
	 * We show a limited number of errors, to avoid generating huge
	 * notices. When we don't show all errors, we include a message
	 * to that effect. This message takes up NTOTAL lines, and we
	 * include that count in the number of errors listed. This avoids
	 * embarassing messages that list N errors followed by a two-line
	 * statement that there is one more error not shown.
	 */
#define ERRBUFSIZ	700
#define MAX_ERRORS	7
#define NTOTAL		2

	Cardinal		n;	/* indexs PropertyList    */
	Cardinal		i;	/* indexs items           */
	Cardinal		j;	/* indexs keys            */
	Cardinal		k;	/* indexs and counts keys */
	Cardinal		nerrors;
	Cardinal		nlines;

	KeyList *		pk;

	Property *		ret	= 0;

	char			buf[ERRBUFSIZ];

	String			p;

        String Follow, Total, Preface;

	int			size;	/* "int", so that it can be <0 */

	static KItem *		keys	= 0;

        Display *		dpy = XtDisplay(w);
        Screen *		screen	  = XtScreenOfObject(w);


#define KEYLIST(N)	((KeyList *)(PropertyList[N]->closure))

	/*
	 * Count the total number of keys and allocate space.
	 * The following method of counting is too generous, as
	 * it assumes every key has both primary and alternate
	 * bindings. But it's also a static count, so we only
	 * have to do this once.
	 */
	if (!keys) {
		Cardinal		nkeys = 0;

		for (n = 0; n < numKbdSheets; n++)
			nkeys += 2 * KEYLIST(n)->num_keys;
		keys = ARRAY(KItem, nkeys);
	}

#define ITEM(PK,C,I) (PK)->_items[C][(PK)->items[C][I]]
#define LABEL(C,I)   (String)(ITEM(pk,C,I).label)

	k = 0;
        for (n = 0; n < numKbdSheets; n++) {
		pk = KEYLIST(n);
		for (i = 0; i < pk->num_keys; ++i) {
			String			string0;
			String			string1;
			Cardinal		len0;

			/*
			 * If the sheet hasn't been created yet, grab the
			 * strings from the resource db.
			 */
			if (!pk->_items[0]) {
				len0 = Separate(
					pk->current[i],
					&string0,
					&string1,
					False
				);
			} else {
				string0 = LABEL(0,i);
				string1 = LABEL(1,i);
				len0 = Strlen(string0);
			}

			keys[k].sheet   = (unsigned char)n;
			keys[k].key     = (unsigned char)i;
			keys[k].binding = string0;
			keys[k].length  = (unsigned char)len0;
			k++;
			if (string1) {
				keys[k].sheet   = (unsigned char)n;
				keys[k].key     = (unsigned char)i;
				keys[k].binding = string1;
				keys[k].length  = (unsigned char)Strlen(string1);
				k++;
			}
		}
	}

	qsort ((XtPointer)keys, k, sizeof(KItem), CmpKeys);

	/*
	 * Display as many duplicates as will fit in ERRBUFSIZ bytes
	 * minus the PREFACE, the TOTAL (if shown), and the FOLLOW.
	 */

        Preface = OLG(preface,dupMsg);
        Total = OLG(total,dupMsg);
        Follow = OLG(follow,dupMsg); 

	size = ERRBUFSIZ - sizeof(Preface) - sizeof(Total) - sizeof(Follow);

#if	OlNeedFunctionPrototypes
# define CMP(a,b) CmpKeys((const void *)(a), (const void *)(b))
#else
# define CMP(a,b) CmpKeys((char *)(a), (char *)(b))
#endif

#define NAME(J)  KEYLIST(keys[J].sheet)->keys[keys[J].key].caption

#if	defined(sun)
# define SPRINTF(X) (Strlen(sprintf X))
#else
# define SPRINTF(X) (sprintf X)
#endif

	p = buf;
	nerrors = 0;
	nlines  = 0;
	for (j = 1; j < k; j++)
		if (CMP(&keys[j-1], &keys[j]) == 0) {
			if (!ret) {
				ret = PropertyList[keys[j-1].sheet];
				p += SPRINTF((p, "%s", Preface));
				nlines += 4;
			}
			size -= sizeof(FORMAT)
			     +  keys[j].length
			     +  strcspn(NAME(j-1),":")
			     +  strcspn(NAME(j),":")
			     +  1;	/* term. null, to be safe */
			if (size >= 0) {
				p += SPRINTF((
					p,
					FORMAT,
					keys[j].length, keys[j].binding,
					strcspn(NAME(j-1),":"),	NAME(j-1),
					strcspn(NAME(j),":"),	NAME(j)
				));
				nlines++;
			}
			nerrors++;
		}

	if (nerrors) {
		static Position		tabs[3] = { 0, 0, 0 };
		Widget			st;
		Widget			te;
		Widget			ca;
		Pixel			bg1;

		/*
		 * "size < 0" means we couldn't display at least one
		 * error, so let the user know there are some not seen.
		 */
		if (size < 0) {
			p += SPRINTF((p, Total, nerrors));
			nlines += 2;
		}
		p += SPRINTF((p, "%s", Follow));
		nlines += 3;

		if (notice.w)
			XtDestroyWidget (notice.w);
		notice.string = "foo";

		/* Use Modal Shell for the conflict notice */
		notice.w =  XtVaCreatePopupShell(
                notice.name,
                modalShellWidgetClass,
                w, XtNtitle, OLG(keycomplaint,fixedString),
                (String)0
        	);

#define _10HorzPoints	OlScreenPointToPixel(OL_HORIZONTAL,10,screen)
#define _30VertPoints	OlScreenPointToPixel(OL_VERTICAL,30,screen)
#define _540HorzPoints	OlScreenPointToPixel(OL_HORIZONTAL,540,screen)

			te = XtVaCreateManagedWidget(
			"textEdit",
			textEditWidgetClass,
			notice.w,
			XtNblinkRate,	 (XtArgVal)0,
			XtNeditType,	 (XtArgVal)OL_TEXT_READ,
			XtNwrapMode,	 (XtArgVal)OL_WRAP_WHITE_SPACE,
			XtNtraversalOn,	 (XtArgVal)True,
			XtNbottomMargin, (XtArgVal)_30VertPoints,
			XtNtopMargin,	 (XtArgVal)_30VertPoints,
                        XtNleftMargin,	 (XtArgVal)_10HorzPoints,
                        XtNrightMargin,	 (XtArgVal)_10HorzPoints,
                        XtNsource,       (XtArgVal)buf,
			XtNsourceType,   (XtArgVal)OL_STRING_SOURCE,
			XtNtabTable,     (XtArgVal)tabs,
			XtNlinesVisible, (XtArgVal)nlines,
			(String)0
		);

	        XtVaCreateManagedWidget(
                	notice.name,
                	flatButtonsWidgetClass,
                	notice.w,
                	XtNitemFields,          (XtArgVal)NoticeMenuFields,
                	XtNnumItemFields,       (XtArgVal)NUM_FIELDS,
                	XtNitems,               (XtArgVal)notice.items,
                	XtNnumItems,            (XtArgVal)notice.numitems,
                	(String)0
        	);


	}

#undef	KEYLIST
#undef	ITEM
#undef	LABEL
#undef	NAME
	return (ret);
}

/**
 ** CmpKeys()
 **/

static int
#if	OlNeedFunctionPrototypes
CmpKeys (
	const void *		_pa,
	const void *		_pb
)
#else
CmpKeys (_pa, _pb)
	char *			_pa;
	char *			_pb;
#endif
{
	register KItem *	pa = (KItem *)_pa;
	register KItem *	pb = (KItem *)_pb;

	/*
	 * We don't care about getting these in perfect order, we
	 * just want the identical strings together. Thus we fudge
	 * the return value if the lengths aren't the same (saves
	 * time).
	 */
	if (pa->length == pb->length)
		return (strncmp(pa->binding, pb->binding, pa->length));
	else if (pa->length < pb->length)
		return (-1);
	else
		return (1);
} /* CmpKeys */

/**
 ** AbortCB()
 **/

static void
#if	OlNeedFunctionPrototypes
AbortCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
AbortCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	XtPopdown((Widget)_OlGetShellOfWidget(w));
}

#ifdef DUP_BINDINGS_FIXED_BY_USER_IN_FUTURE
/**
 ** ContinueCB()
 **/

static void
#if	OlNeedFunctionPrototypes
ContinueCB (
	Widget			w,
	XtPointer		client_data,
	XtPointer		call_data
)
#else
ContinueCB (w, client_data, call_data)
	Widget			w;
	XtPointer		client_data;
	XtPointer		call_data;
#endif
{
	Cardinal		n;

	KeyList *		pk;


#define KEYLIST(N)	((KeyList *)(PropertyList[N]->closure))

	/*
	 * The method property.c uses to apply all sheets is to step
	 * through each. The duplicate-key check, though, looks across
	 * ALL keyboard sheets, the first time one of the sheets is
	 * applied. Thus, to be here means we've hit just the first
	 * sheet, and we have all other (created) sheets to go.
	 * Therefore, we'll clear the ``check for duplicates'' for all
	 * (created) sheets; as each is applied, it will clear its
	 * flag.
	 */
        for (n = 0; n < numKbdSheets; n++) {
		pk = KEYLIST(n);
		if (pk->_items[0])
			pk->check = False;
	}

	XtPopdown((Widget)_OlGetShellOfWidget(w));
	PropertyApplyCB (w, client_data, call_data);

#undef	KEYLIST
	return;
} /* ContinueCB */
#endif /* DUP_BINDINGS_FIXED_BY_USER_IN_FUTURE */

/**
 ** InsertPosition()
 **/

static Cardinal
#if	OlNeedFunctionPrototypes
InsertPosition (
	Widget			w
)
#else
InsertPosition (w)
	Widget			w;
#endif
{
	return (0);
} /* InsertPosition */
