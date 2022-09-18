/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)dtm:h_util.c	1.22"

/******************************file*header********************************

    Description:
     This file contains the source code for parsing a help file,
	reading a definition file and retrieving notes previously saved.
*/
                              /* #includes go here     */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>

#include "Dtm.h"
#include "dm_strings.h"
#include "extern.h"

/**************************forward*declarations***************************

    Forward Procedure definitions listed by category:
          1. Public Procedures
          2. Private  Procedures
*/
                         /* public procedures         */

extern char *strndup();

/***************************public*procedures****************************

    Public Procedures
*/

/****************************procedure*header*****************************
 * Called at the beginning of a session to initialize the help manager. 
 */
int
DmInitHelpManager()
{
	return(0);
} /* end of DmInitHelpManager */

/****************************procedure*header*****************************
 *  Call at the end of a session to close all help windows.
 */
void
DmHMExit()
{
	/* close all applications' help windows */
	DmCloseAllHelpWindows();

} /* end of DmHMExit */

/****************************procedure*header*****************************
 * This function parses lines like "^*name^value". If successful, the
 * pointer is placed at the beginning of the next line.
 */
int
Dm__GetNameValue(mp, name, value)
DmMapfilePtr mp;
char **name;
char **value;
{
	char *start = MF_GETPTR(mp);
	char *p;

	if (p = Dm__findchar(mp, '^')) {
		*name = strndup(start, p - start);
		start = ++p;
		MF_NEXTC(mp); /* skip '^' */
		if (p = Dm__findchar(mp, '\n')) {
			*value = strndup(start, p - start);
			MF_NEXTC(mp); /* skip '\n' */
			return(0);
		} else {
			*value = NULL;
			return(1); /* can't find '\n' */
		}
	}
	*value = NULL;
	return(1); /* can't find '^' between name and value */
} /* end of Dm__GetNameValue() */

/****************************procedure*header*****************************
 * This function parses lines like "^%keyword^value". If successful,
 * the keyword will be stored in a property list, and the
 * pointer is placed at the beginning of the next line.
 */
int
DmGetKeyword(mp, plistp)
DmMapfilePtr mp;
DtPropListPtr plistp;
{
	char *name;
	char *value;

	if (Dm__GetNameValue(mp, &name, &value))
		return(1);

	DtSetProperty(plistp, name, value, DM_B_KEYWORD);
	free(value);
	free(name);
	return(0);
} /* end of DmGetKeyword() */

/****************************procedure*header*****************************
 * This function parses definition lines like "^=keyword" until the end of
 * the definition "^=".
 * If successful, the keyword will be stored in a property list, and the
 * pointer is placed at the beginning of the next line.
 */
int
DmGetDefinition(mp, plistp)
DmMapfilePtr mp;
DtPropListPtr plistp;
{
	char *name = MF_GETPTR(mp);
	char *value;
	char *p;

	if (p = Dm__findchar(mp, '\n')) {
		name = strndup(name, p - name);
		MF_NEXTC(mp); /* skip '\n' */

		/* find end of definition "\n^=\n" */
		value = MF_GETPTR(mp);
		if (p = Dm__strstr(mp, "\n^=\n")) {
			value = strndup(value, p - value);
			DtSetProperty(plistp, name, value, DM_B_DEF);
			free(value);
			free(name);
			MF_NEXTC(mp); /* skip '\n' */
			MF_NEXTC(mp); /* skip '^'  */
			MF_NEXTC(mp); /* skip '='  */
			MF_NEXTC(mp); /* skip '\n' */
			return(0);
		}
		free(name);
		return(1);
	}
} /* end of DmGetDefinition() */

/****************************procedure*header*****************************
 * This function parses definition file line like "^+deffile" until the end of
 * the definition "^=".
 * If successful, the keyword will be stored in a property list, and the
 * pointer is placed at the beginning of the next line.
 */
int
DmReadDefFile(mp, plistp, help_dir)
DmMapfilePtr mp;
DtPropListPtr plistp;
char *help_dir;
{
	DmMapfilePtr def_mp;
	char *name = MF_GETPTR(mp);
	char *value;
	char *def_file;
	char *p;
	int	c;

	if (p = Dm__findchar(mp, '\n')) {
		name = strndup(name, p - name);
		MF_NEXTC(mp); /* skip '\n' */

		if (*name == '/') {
			/* Full path name specified */
			def_file = name;
		} else if (help_dir) {
			/* use hap->help_dir */
			def_file = DmMakePath(help_dir, name);
		} else {
			def_file = XtResolvePathname(DESKTOP_DISPLAY(Desktop),
						"help", name, NULL, NULL, NULL, 0, NULL);
		}
		
		if (def_file == NULL)
			return(0);

		/* map file for reading */
		if (!(def_mp = Dm__mapfile(def_file, PROT_READ, MAP_SHARED))) {
  			return(0);
		}

		/* scan the file for definitions */
		while (MF_NOT_EOF(def_mp)) {

			if (MF_PEEKC(def_mp) == '#') {
				Dm__findchar(def_mp, '\n');
				MF_NEXTC(def_mp); /* skip #ident string */

			} else if (MF_PEEKC(def_mp) == '\n') {
				MF_NEXTC(def_mp); /* skip blank line */

			} else if (MF_PEEKC(def_mp) == '^') {
				MF_NEXTC(def_mp); /* skip '^' */

				switch(c = MF_PEEKC(def_mp)) {
				case '=':
					/* definition */
					MF_NEXTC(def_mp);
					DmGetDefinition(def_mp, plistp);
					break;

				default:
					/* silently ignore other options */
					break;
				}
			}
		}
		free(name);
		Dm__unmapfile(def_mp);
		return(1);
	}
} /* end of DmReadDefFile() */

/****************************procedure*header*****************************
 * This routine will convert a reference string ("file^sect_tag")
 * to a DmHelpLoc structure and return a pointer to this static structure.
 * The original string is not modified in anyway.
 */
DmHelpLocPtr
DmHelpRefToLoc(str)
char	*str;
{
	char *save = strdup(str);
	register char *p = save;
	static DmHelpLocRec loc = { NULL };
	char *array[2] = { NULL };
	char **pp = array;
	int  i;

	/* free previous strings */
	if (loc.file) {
		free(loc.file);
		loc.file = NULL;
	}
	if (loc.sect_tag) {
		free(loc.sect_tag);
		loc.sect_tag = NULL;
	}

	if (*p != '^') {
		/* file_name is specified */ 
		*pp = p;

		if (p = strchr(p, '^')) {
			*p++ = '\0';

			/* assign the rest of the string to sect_tag */
			/* it could be NULL */
			*(++pp) = p;
		} else {
			/* assume str only contains file_name */
			*(++pp) = NULL;
		}
	} else {
		*pp = NULL;
		*p++;
		*(++pp) = p;
	}

	if (array[0])
		loc.file = strdup(array[0]);
	if (array[1])
		loc.sect_tag = strdup(array[1]);

	free(save);
	return(&loc);
} /* end of DmHelpRefToLoc() */

/****************************procedure*header*****************************
 */
void
DmReadNotes(hwp, cur_tag)
DmHelpWinPtr hwp;
char *cur_tag; /* tag of current section */
{
	DmHelpNotesPtr	np;

	XtSetArg(Dm__arg[0], XtNcursorPosition, 0);
	XtSetArg(Dm__arg[1], XtNselectStart, 0);
	XtSetArg(Dm__arg[2], XtNselectEnd, 0);

	for (np = hwp->hfp->notesp; np; np = np->next) {
		if (np->sect_name && !strcmp(cur_tag, np->sect_name) ||
			(np->sect_tag && !strcmp(cur_tag, np->sect_tag))) {

			XtSetArg(Dm__arg[3], XtNsource, np->notes_file);
			XtSetArg(Dm__arg[4], XtNsourceType, OL_DISK_SOURCE);
			XtSetValues(hwp->notes_te, Dm__arg, 5);

			/* make Delete button sensitive */
			OlVaFlatSetValues(hwp->notes_btns, 1, XtNsensitive, True, NULL);
			return;
		}
	}
	XtSetArg(Dm__arg[3], XtNsource, "");
	XtSetArg(Dm__arg[4], XtNsourceType, OL_STRING_SOURCE);
	XtSetValues(hwp->notes_te, Dm__arg, 5);
	/* make Delete button insensitive */
	OlVaFlatSetValues(hwp->notes_btns, 1, XtNsensitive, False, NULL);
	return;

} /* end of DmReadNotes */

/****************************procedure*header*****************************
 * This routine returns a section tag given a section name.
 * alias is a flag to indicate whether the alias section name
 * should be used in locating the section.
 */
char *
DmGetSectTag(DmHelpFilePtr hfp, char *sect_name, Boolean alias)
{
	DmHelpSectPtr hsp;
	int  i;
	char *tag = NULL;

	for (hsp = hfp->sections, i = hfp->num_sections; i; i--, hsp++) {
		if (hsp->alias && !strcmp(hsp->alias, sect_name))
			return(hsp->tag);
		else if (!strcmp(hsp->name, sect_name))
			return(hsp->tag);
	}
	return(NULL);

} /* end of DmGetSectTag */

/****************************procedure*header*****************************
 * This routine returns a section name given a section tag.
 */
char *
Dm__GetSectName(hfp, sect_tag)
DmHelpFilePtr hfp;
char *sect_tag;
{
	DmHelpSectPtr hsp;
	int  i;
	char *name = NULL;

	for (hsp = hfp->sections, i = hfp->num_sections; i; i--, hsp++)
		if (!strcmp(hsp->tag, sect_tag))
			return(hsp->name);

	return(NULL);

} /* end of DmGetSectTag */

/****************************procedure*header*****************************
 * Called when a new section is displayed to activate/deactivate the
 * Previous Topic, Next Topic, Backtrack, Bookmark and Notes buttons
 * depending on whether a "regular" section or a Table of Contents is
 * being displayed.  Also deactivates/activates buttons in Bookmark
 * and Notes window if they're up when a Table of Contents and a
 * "regular" section is displayed, respectively.
 */
void
DmChgHelpWinBtnState(DmHelpWinPtr hwp, Boolean skip_menubar, Boolean sensitive)
{
	XtSetArg(Dm__arg[0], XtNsensitive, sensitive);

	if (skip_menubar == False) {
		OlFlatSetValues(hwp->menubar, 0, Dm__arg, 1);
		OlFlatSetValues(hwp->menubar, 1, Dm__arg, 1);
		OlFlatSetValues(hwp->menubar, 2, Dm__arg, 1);
		OlFlatSetValues(hwp->menubar, 4, Dm__arg, 1);
		OlFlatSetValues(hwp->menubar, 6, Dm__arg, 1);
	}

	/* if Bookmark window is up, deactivate/activate its buttons */
	if (hwp->bmark_shell)
		XtSetValues(hwp->bmark_btns, Dm__arg, 1);

	/* if Notes window is up, deactivate/activate its buttons */
	if (hwp->notes_shell)
		XtSetValues(hwp->notes_btns, Dm__arg, 1);

} /* end of DmChgHelpWinBtnState */
