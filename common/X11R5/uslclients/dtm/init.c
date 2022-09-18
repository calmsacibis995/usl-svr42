/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:init.c	1.39"

#include <libgen.h>
#include <X11/Intrinsic.h>
#include <Xol/OpenLook.h>
#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"
#include "shortcut.icon"

static const char shortcut_glyph[] = "shortcut.icon";

/* default paths */
static const char dflt_desktopdir[] = "$HOME";
static const char dflt_fclassdb[] = "$HOME/.dtfclass";
static const char dflt_toolbox[] = "%DESKTOPDIR";
static const char dflt_iconpath[] = "";
static const char dflt_wbdir[] = "%DESKTOPDIR/.wastebasket";
static const char dflt_hdpath[] = "$XWINHOME/desktop/Help_Desk";
static const char dflt_templatedir[] = "$XWINHOME/lib/template";

/* global variable */
DmHDDataPtr hddp = NULL;

static char *
InitDesktopProp(name, dflt, expand)
char *name;
char *dflt; /* default value */
Boolean expand;
{
	char *p;
	char *ret = NULL;
	extern char *getenv();

	if (!(p = getenv(name))) {
		/*
		 * If it is already defined in the prop list, so be it.
		 * Otherwise, use the default.
		 */
		if (p = DtGetProperty(&DESKTOP_PROPS(Desktop), name, NULL))
			return(p);
		else
			p = dflt;
	}

	if (p && *p) {
		if (expand)
			p = Dm__expand_sh(p, DmDTProp, NULL);
		else
			p = strdup(p);
		ret = DtSetProperty(&DESKTOP_PROPS(Desktop), name, p, 0);
		free(p);
	}
	return(ret);
}

static char *
PreProcessRE(re)
char *re;
{
	char *ret;
	register char *p;

	if (p = ret = (char *)malloc(strlen(re) + 2)) {
		strcpy(p, re);

		/* translate all ',' to '\0' */
		for (; *p; p++) {
			if (*p == '\\') {
				/*
				 * make sure we are not
				 * escaping a null char.
				 */
				if (*++p == '\0')
					p--;
			}
			if (*p == ',')
				*p = '\0';
		}
		*++p = '\0'; /* extra null at the end */
	}
	return(ret);
}

void
DmInitFileClass(fnkp)
DmFnameKeyPtr fnkp;
{
	register char *p;

	if (p = DtGetProperty(&(fnkp->fcp->plist), PATTERN, NULL)) {
		register char *p2;

		if (fnkp->re = PreProcessRE(p)) {
			fnkp->attrs |= DM_B_REGEXP;
		}
		else
			Dm__VaPrintMsg(TXT_BAD_REGEXP, p);
	}

	if (p = DtGetProperty(&(fnkp->fcp->plist), FILETYPE, NULL)) {
		DmFmodeKeyPtr fmkp = DmStrToFmodeKey(p);

		if (fmkp) {
			fnkp->attrs |= DM_B_FILETYPE;
			fnkp->ftype = fmkp->ftype;
		}
	}

	if (p = DtGetProperty(&(fnkp->fcp->plist), FILEPATH, NULL))
		fnkp->attrs |= DM_B_FILEPATH;

	if (p = DtGetProperty(&(fnkp->fcp->plist), LPATTERN, NULL)) {
		register char *p2;

		if (fnkp->lre = PreProcessRE(p)) {
			fnkp->attrs |= DM_B_LREGEXP;
		}
		else
			Dm__VaPrintMsg(TXT_BAD_REGEXP, p);
	}

	if (p = DtGetProperty(&(fnkp->fcp->plist), LFILEPATH, NULL))
		fnkp->attrs |= DM_B_LFILEPATH;

}

static void
DmGetDesktopProperties(desktop)
DmDesktopPtr desktop;
{
	int lineno;
	int ret;
	char *path;
	FILE *f;
	char name[128];
	char value[256];

	path = DmMakePath(DmGetDTProperty(DESKTOPDIR, NULL), ".dtprops");
	if (f = fopen(path, "r")) {
		lineno = 1;
		while ((ret = fscanf(f,"%[^=]=%s\n",&name,&value)) == 2) {
			DtSetProperty(&DESKTOP_PROPS(desktop), name, value, 0);
			lineno++;
		}

		if (ret != EOF) {
			Dm__VaPrintMsg(TXT_SYNTAX, path, lineno);
		}
		fclose(f);
	}
}

/*
 * return values:	 0	success, but previous session was not started.
 *			 1	success, previous session was started.
 *			-1	failed.
 */
int
DmOpenDesktop()
{
    char *		p;
    char *		q;
    DmFnameKeyPtr	fnkp;
    int			ret;

    /* If $HOME is '/', then DESKTOPDIR will be "//desktop" which will
       confuse things like the Folder menu.
    */
    p = InitDesktopProp(DESKTOPDIR, dflt_desktopdir, True);
    DESKTOP_DIR(Desktop) = ((p[0] == '/') && (p[1] == '/')) ? p + 1 : p;

    p = InitDesktopProp(ICONPATH, dflt_iconpath, False);
    DmSetIconPath(p);		/* pass it to libDtI */

    /* Load in desktop properties from the previous session first.
     * If an environment variable is set, it has higher precedence than
     * the old desktop property values.
     */
    DmGetDesktopProperties(Desktop);

    p = InitDesktopProp(WBDIR, dflt_wbdir, True);
    p = InitDesktopProp(HDPATH, dflt_hdpath, True);
    p = InitDesktopProp(TEMPLATEDIR, dflt_templatedir, True);

    DmInitDfltFileClasses(DESKTOP_SHELL(Desktop));

    p = InitDesktopProp(FILEDB_PATH, dflt_fclassdb, True);

    /* if user is root, remove the extra '/' */
    q = (p[0] == '/' && p[1] == '/') ? p + 1 : p;

    if ( (DESKTOP_FNKP(Desktop) = DmReadFileClassDB(q)) == NULL)
    {
	Dm__VaPrintMsg(TXT_FILE_CLASSDB, q);
	return(-1);
    }

    /* Optimization: preprocess all regular expressions and
     * set flags for typing properties.
     */
    for (fnkp = DESKTOP_FNKP(Desktop); fnkp != NULL; fnkp = fnkp->next)
    {
	if (!(fnkp->attrs & DM_B_CLASSFILE))
	    DmInitFileClass(fnkp);
    }

    /* get shortcut glyph */
    DESKTOP_SHORTCUT(Desktop) = DmGetPixmap(DESKTOP_SCREEN(Desktop),
					    (char *)shortcut_glyph);
    if (DESKTOP_SHORTCUT(Desktop) == NULL)
	DESKTOP_SHORTCUT(Desktop) =
	    DmCreateBitmapFromData(DESKTOP_SCREEN(Desktop),
				   "\n/default shortcut icon\n",
				   (unsigned char *)shortcut_bits,
				   shortcut_width, shortcut_height);

    /* restart all the things that were in the previous session */
    p = DmMakePath(DESKTOP_DIR(Desktop), ".lastsession");
    ret = DmRestartSession(p);

#ifdef TOOLBOX
    /* open toolbox */
    DESKTOP_TOOLBOX(Desktop)=DmOpenToolboxWindow(TOOLBOXROOT,0,NULL,False);
#endif

    /* open folder onto desktop dir */
    DESKTOP_TOP_FOLDER(Desktop) = DmQueryFolderWindow(DESKTOP_DIR(Desktop));
    if (DESKTOP_TOP_FOLDER(Desktop) == NULL)
	DESKTOP_TOP_FOLDER(Desktop) =
	    DmOpenFolderWindow(DESKTOP_DIR(Desktop), 0, NULL, False);

	/* Allocate structure to store help highlighting color and Help Desk
	 * information.  This has to be done here because the Help Desk is
	 * only initialized if it gets a request to add or remove an icon
	 * but the help highlighting color can be changed at any time.
	 * This may have been done already if Help Desk was saved from a
	 * previous session.
	 */
	if (hddp == NULL) {
		if ((hddp = (DmHDDataPtr)CALLOC(1, sizeof(DmHDDataRec))) == NULL) {
			Dm__VaPrintMsg(TXT_MEM_ERR);
			return(-1);
		}
		hddp->key_color = DESKTOP_OPTIONS(Desktop).help_key_color;
	}
    return(ret);
}
