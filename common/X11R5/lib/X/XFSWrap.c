/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:XFSWrap.c	1.1"
/*
 * $XConsortium: XFSWrap.c,v 11.6 91/06/05 08:53:49 rws Exp $
 */

/*
 * Copyright 1991 by the Massachusetts Institute of Technology
 * Copyright 1991 by the Open Software Foundation
 *
 *
 * OPEN SOFTWARE FOUNDATION AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD TO
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * 
 *		 M. Collins		OSF  
 */				

#include "Xlibint.h"
#include "Xlcint.h"
#include <ctype.h>
#include <X11/Xos.h>


#define	MAXLIST	256

char **
_XParseBaseFontNameList(str, num)
    char           *str;
    int            *num;
{
    char           *plist[MAXLIST];
    char          **list;
    char           *ptr;

    *num = 0;
    if (!str || !*str) {
	return (char **)NULL;
    }
    while (*str && isspace(*str))
	str++;
    if (!*str)
	return (char **)NULL;

    if (!(ptr = Xmalloc((unsigned)strlen(str) + 1))) {
	return (char **)NULL;
    }
    strcpy(ptr, str);

    while (1) {
	char	*back;

	plist[*num] = ptr;
	if ((ptr = index(ptr, ','))) {
	    back = ptr;
	} else {
	    back = plist[*num] + strlen(plist[*num]);
	}
	while (isspace(*(back - 1)))
	    back--;
	*back = '\0';
	(*num)++;
	if (!ptr)
	    break;
	ptr++;
	while (*ptr && isspace(*ptr))
	    ptr++;
	if (!*ptr)
	    break;
    }
    if (!(list = (char **) Xmalloc((unsigned)sizeof(char *) * (*num + 1)))) {
	Xfree(ptr);
	return (char **)NULL;
    }
    bcopy((char *)plist, (char *)list, sizeof(char *) * (*num));
    *(list + *num) = NULL;

    return list;
}

#if NeedFunctionPrototypes
XFontSet
XCreateFontSet (
    Display        *dpy,
    _Xconst char   *base_font_name_list,
    char         ***missing_charset_list,
    int            *missing_charset_count,
    char          **def_string)
#else
XFontSet
XCreateFontSet (dpy, base_font_name_list, missing_charset_list,
	        missing_charset_count, def_string)
    Display        *dpy;
    char           *base_font_name_list;
    char         ***missing_charset_list;
    int            *missing_charset_count;
    char          **def_string;
#endif
{
    XLCd lcd = _XlcCurrentLC();
    char *base_name;
    char **name_list;
    int count;
    XFontSet font_set;

    *missing_charset_list = NULL;
    *missing_charset_count = 0;
    if (!lcd)
	return NULL;
    base_name = (char *)Xmalloc(strlen(base_font_name_list) + 1);
    if (!base_name)
	return NULL;
    strcpy(base_name, base_font_name_list);
    name_list = _XParseBaseFontNameList(base_name, &count);
    if (!name_list) {
	Xfree(base_name);
        return NULL;
    }
    font_set = (*lcd->methods->create_fontset) (lcd, dpy, base_name,
						name_list, count,
						missing_charset_list,
						missing_charset_count);
    if (!font_set) {
	XFreeStringList(name_list);
	Xfree(base_name);
    } else if (def_string) {
	*def_string = font_set->core.default_string;
	if (!*def_string)
	    *def_string = "";
    }
    return font_set;
}

int
XFontsOfFontSet(font_set, font_struct_list, font_name_list)
    XFontSet        font_set;
    XFontStruct  ***font_struct_list;
    char         ***font_name_list;
{
    *font_name_list   = font_set->core.font_name_list;
    *font_struct_list = font_set->core.font_struct_list;
    return font_set->core.num_of_fonts;
}

char *
XBaseFontNameListOfFontSet(font_set)
    XFontSet        font_set;
{
    return font_set->core.base_name_list;
}

char *
XLocaleOfFontSet(font_set)
    XFontSet        font_set;
{
    return font_set->core.lcd->core.name;
}

extern Bool XContextDependentDrawing(font_set)
    XFontSet        font_set;
{
    return font_set->core.context_dependent;
}

XFontSetExtents *
XExtentsOfFontSet(font_set)
    XFontSet        font_set;
{
    return &font_set->core.font_set_extents;
}

void
XFreeFontSet(dpy, font_set)
    Display        *dpy;
    XFontSet        font_set;
{
    int i;

    (*font_set->methods->free) (dpy, font_set);
    for (i = 0; i < font_set->core.num_of_fonts; i++) {
	if (font_set->core.font_struct_list[i]) {
	    if (font_set->core.font_struct_list[i]->fid)
		XFreeFont(dpy, font_set->core.font_struct_list[i]);
	    else
		Xfree((char *)font_set->core.font_struct_list[i]);
	}
    }
    Xfree((char *) font_set->core.font_struct_list);
    Xfree(font_set->core.base_name_list);
    XFreeStringList(font_set->core.font_name_list);
    Xfree ((char *) font_set);
}
