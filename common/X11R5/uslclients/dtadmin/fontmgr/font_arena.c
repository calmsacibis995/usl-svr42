/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtadmin:fontmgr/font_arena.c	1.10"

/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_arena.c
 */

#include <stdio.h>
#include <Xatom.h>
#include <Intrinsic.h>
#include <fontmgr.h>

extern Widget       app_shellW;		  /* application shell widget       */

static char *bad_boys[] = { "open look cursor", "open look glyph" };

static slant_type slant_table[] = {
{ "R", "" },
{ "I", "Italic" },
{ "O", "Oblique" },
{ "RI", "Reverse Italic"},
{ "RO", "Reverse Oblique" }
};


/* returns FALSE if font is a derived instance font */
Boolean
ParseXLFD(String xlfd_name, xlfd_type **info)
{
    static xlfd_type xlfd_info;
    char field_str[MAX_STRING];
    Boolean done = FALSE;
    char *p;
    int f, len;
    Boolean derived = FALSE;
    String pixel_size_p;

    *info = &xlfd_info;

    /* for each field in the XLFD */
    for (f = 0, p = xlfd_name; (f<RELEVANT_FIELD_COUNT) && !done; f++) {
	char *fieldP;

	p = (char *) GetNextField( DELIM, p, &fieldP, &len);
	strncpy(field_str, fieldP, len);
	field_str[len]=0;  /* string terminator */

	switch(f) {
	case 1: /* family name */
	    CapitalizeStr(field_str);
	    strcpy(xlfd_info.family, field_str);
	    break;
	case 2: /* weight */
	    strcpy(xlfd_info.weight, field_str);
	    CapitalizeStr(xlfd_info.weight);
	    break;
	case 3: /* slant */
	    strcpy(xlfd_info.slant, field_str);
	    break;
	case 4: /* setwidth name */
	    strcpy(xlfd_info.set_width, field_str);
	    CapitalizeStr(xlfd_info.set_width);
	    if (strcmp(xlfd_info.set_width, "Normal")==STR_MATCH)
		xlfd_info.set_width[0]=0;
	    break;
	case 5: /* add style name */
	    strcpy(xlfd_info.add_style, field_str);
	    CapitalizeStr(xlfd_info.add_style);
	    break;
	case 6: /* pixel size */
	    pixel_size_p = fieldP;
	    break;
	case 7: /* point size */
	    field_str[len-1]=0; /*unit is in tenths of a point so convert it */
	    if (atoi(field_str) == 0) {
		/* outline font, cut short the string */
		strncpy(xlfd_info.truncated_xlfd, xlfd_name,
			pixel_size_p-xlfd_name);
		xlfd_info.truncated_xlfd[pixel_size_p-xlfd_name] = 0;
		xlfd_info.bitmap = FALSE;
	    }
	    else
		/* bitmap font */
		xlfd_info.bitmap = TRUE;
	    strcpy( xlfd_info.size, field_str);
	    break;
	case 10: /* spacing */
	    strcpy(xlfd_info.spacing, field_str);
	    break;
	case 11: /* average width */
	    if ((atoi(field_str) == 0) && xlfd_info.bitmap)
		derived = TRUE;
	    done = TRUE;
	    break;
	} /* switch */
    } /* for f */
    return !derived;

} /* end of ParseXLFD */


static int
FamilyInfoCmp (pA, pB)
    family_info * pA;
    family_info * pB;
{
	return(strcmp(pA->n, pB->n));
}  /*  end of FamilyInfoCmp() */


static int
PSInfoCmp (pA, pB)
    ps_info *pA;
    ps_info *pB;
{
	int a = atoi(pA->ps);
	int b = atoi(pB->ps);

	if (a < b)
		return(-1);
	else
		return(!(a == b));

}  /*  end of PSInfoCmp() */


static int
LookInfoCmp (pA, pB)
    look_info *pA;
    look_info *pB;
{
	return(strcmp(pA->look_name, pB->look_name));
}  /*  end of LookInfoCmp() */


static Boolean _IsXLFDFontName(fontName)
    String fontName;
{
    int f;
    for (f = 0; *fontName;) if (*fontName++ == DELIM) f++;
    return (f == FIELD_COUNT);
} /* end of _IsXLFDFontName() */


static _OlArenaType(LookArena) *
	InitLookArena ()
{
    _OlArenaType(LookArena) * look_arena;
    
    look_arena = XtNew(_OlArenaType(LookArena));
    _OlArenaInitialize(look_arena, 4, 4, LookInfoCmp);
    return(look_arena);
    
}  /* end of InitLookArena() */


static void
LookUpSlant( slant)
    String slant;
{
    int i;

    UppercaseStr( slant);
    for (i=0; i<XtNumber(slant_table); i++)
	if (strcmp( slant, slant_table[i].code)==STR_MATCH) {
	    strcpy( slant, slant_table[i].translation);
	    break;
	}
} /* end of LookUpSlant */


static void
ConcatLook(String look, String cat)
{
    if (*cat) {
	strcat(look, cat);
	strcat(look, " ");
    }    
} /* end of ConcatLook */


static void
GetLook(xlfd_type *info, String look_str)
{
    int look_len;

    LookUpSlant( info->slant);
    *look_str = 0;
    ConcatLook(look_str, info->set_width);
    ConcatLook(look_str, info->add_style);
    ConcatLook(look_str, info->weight);
    ConcatLook(look_str, info->slant);
    look_len = strlen(look_str);
    if (look_len && look_str[look_len-1] == ' ')
	look_str[look_len-1] = 0; /* remove trailing space */

} /* end of GetLook */

static _OlArenaType(PSArena) *
AddLook (xlfd_type *info, _OlArenaType(LookArena) *look_arena)
{
    char look_str[MAX_STRING];
    look_info tmp;
    int spot, hint;

    GetLook(info, look_str);
    tmp.look_name = look_str;
    if ((spot = _OlArenaFindHint(look_arena, &hint, tmp)) ==
	_OL_NULL_ARENA_INDEX)  {
	tmp.look_name = XtNewString(look_str);
	tmp.l = XtNew(_OlArenaType(PSArena));
	_OlArenaInitialize(tmp.l, 10, 10, PSInfoCmp);
	spot = _OlArenaHintedOrderedInsert(look_arena, hint, tmp);
    }
    return _OlArenaElement(look_arena, spot).l;
}  /*  end of AddLook() */


static void
AddPointSize (point_str, ps_arena, xlfd_name, bitmap, truncated_xlfd)
    String point_str;
    _OlArenaType(PSArena) * ps_arena;
    String xlfd_name;
    Boolean bitmap;
    String truncated_xlfd;
{
    ps_info tmp;
    int hint;

    tmp.ps = point_str;
    if (_OlArenaFindHint(ps_arena, &hint, tmp) == _OL_NULL_ARENA_INDEX)  {
	tmp.ps = XtNewString(point_str);
	tmp.l = XtNew(font_type);
	tmp.l->xlfd_name = XtNewString(xlfd_name);
	tmp.l->bitmap = bitmap;
	tmp.l->truncated_xlfd = XtNewString(truncated_xlfd);
	_OlArenaHintedOrderedInsert(ps_arena, hint, tmp);
    }
}  /* end of AddPointSize() */



/* add family */
static _OlArenaType(LookArena) *
AddFamily(family_str, family_arena)
    char *family_str;
    _OlArenaType(FamilyArena) *family_arena;
{
    family_info tmp;
    int hint;
    int spot;

    tmp.n = family_str;
    if ((spot = _OlArenaFindHint(family_arena, &hint,
				 tmp)) == _OL_NULL_ARENA_INDEX)  {
	tmp.n = XtNewString(family_str);
	tmp.l = InitLookArena();
	spot = _OlArenaHintedOrderedInsert(family_arena, hint, tmp);
    }
    return _OlArenaElement(family_arena, spot).l;
} /* end of AddFamily */


static Boolean
ValidFamily(String str)
{
    int i;

    for (i=0; i<XtNumber(bad_boys); i++)
      if (caseless_strcmp(str, bad_boys[i], 0) == STR_MATCH)
          return FALSE;

    return TRUE;
}


static void
FillArena( family_arena, xlfd_name)
    _OlArenaType(FamilyArena) *family_arena;
    char *xlfd_name;
{
    _OlArenaType(LookArena) *look_arena;
    _OlArenaType(PSArena) *ps_arena;
    xlfd_type *info;

    if (ParseXLFD(xlfd_name, &info) && ValidFamily(info->family)) {
	look_arena = AddFamily(info->family, family_arena);
	ps_arena = AddLook(info, look_arena);
	AddPointSize(info->size, ps_arena, xlfd_name, info->bitmap,
		     info->truncated_xlfd);
    }

} /* end of FillArena */


String GetFontName( String xlfd)
{
    char font_name[MAX_PATH_STRING];
    xlfd_type *info;

    ParseXLFD( xlfd, &info);
    LookUpSlant( info->slant);
    sprintf( font_name, "%s %s %s %s %s %s", info->family, 
	    info->set_width, info->add_style, info->weight,
	    info->slant, info->size);
    return font_name;

} /* end of GetFontName */


String GetFontNameAndType( String xlfd, Boolean *bitmap)
{
    char font_name[MAX_PATH_STRING];
    xlfd_type *info;

    ParseXLFD( xlfd, &info);
    LookUpSlant( info->slant);
    *bitmap = info->bitmap;
    sprintf( font_name, "%s %s %s %s %s %s", info->family, 
	    info->set_width, info->add_style, info->weight,
	    info->slant, info->size);
    return font_name;

} /* end of GetFontNameAndType */


static void
AddFontFamilies (display, family_arena)
    Display * display;
    _OlArenaType(FamilyArena) *family_arena;
{
  char ** font_list;
  String pattern = "*";
  int font_list_count;
  int max_names = 32767;
  int i;

  font_list =  XListFonts(display, pattern, max_names,
			  &font_list_count);

  /* for each XLFD */
  for (i = 0; i < font_list_count; i++)  { 
    if (!_IsXLFDFontName(font_list[i])) {
      continue;
    }

    FillArena( family_arena, font_list[i]);
  } /* for i */

  XFreeFontNames(font_list);

}  /* end of AddFontFamilies() */


/* create and init family_arena */
void
CreateFontDB( family_arena)
    _OlArenaType(FamilyArena) *family_arena;
{
    _OlArenaInitialize(family_arena, 15, 15, FamilyInfoCmp);
    AddFontFamilies(XtDisplay(app_shellW), family_arena);

} /* end of CreateFontDB */


/*  destroy the look and size arenas and the associated strings.  */
void
DeleteFontDB( family_arena)
    _OlArenaType(FamilyArena) *family_arena;
{
    int i, j, k;
    _OlArenaType(PSArena) * ps_arena;
    _OlArenaType(LookArena) * look_arena;
    font_type *font_info;

    for (i = 0; i < _OlArenaSize(family_arena); i++)  {
	XtFree(_OlArenaElement(family_arena, i).n);
	look_arena = _OlArenaElement(family_arena, i).l;
	for (j = 0; j < _OlArenaSize(look_arena); j++)  {
	    XtFree(_OlArenaElement(look_arena, j).look_name);
	    ps_arena = _OlArenaElement(look_arena, j).l;
	    for (k = 0; k < _OlArenaSize(ps_arena); k++)  {
		XtFree(_OlArenaElement(ps_arena, k).ps);
		font_info = _OlArenaElement(ps_arena, k).l;
		XtFree( font_info->xlfd_name);
		XtFree( (char *) font_info);
	    }
	    _OlArenaFree(ps_arena);
	}
	_OlArenaFree(look_arena);
    }
    _OlArenaFree(family_arena);

} /* end of DeleteFontDB */

#define FONT_KEY "*font:"
ResetToDefaultFont(view_type *view)
{
    int i, num_props;
    XFontProp *props;
    char buf[MAX_PATH_STRING], *p;
    FILE *file;
    XFontStruct *font;
    xlfd_type *info;
    int key_len = strlen(FONT_KEY);

    *view->cur_xlfd = 0;

    /* first try to get fontname from .Xdefaults */
    sprintf(buf, "%s/.Xdefaults", getenv("HOME"));
    file = fopen(buf, "r");
    if (FileOK(file)) {
	while (fgets(buf, MAX_PATH_STRING, file) != NULL) {
	    if (strncmp(buf, FONT_KEY, key_len) == STR_MATCH) {
		if ((p = strchr(buf, DELIM)) == NULL)
			continue;
		strcpy(view->cur_xlfd, p);
		view->cur_xlfd[strlen(view->cur_xlfd)-1]=0; /* get rid of \n */
		break;
	    }
	}
	fclose(file);
    }

    /* try to get name from X property */
    if (*view->cur_xlfd == 0) {
	XtVaGetValues(view->sample_text, XtNfont, &font, NULL);
	props = font->properties;
	num_props = font->n_properties;
	for (i = 0; i < num_props; i++, props++)  {
	    if (props->name == XA_FONT) {
		p = XGetAtomName(XtDisplay(app_shellW),props->card32);
		if (p)
		    strcpy(view->cur_xlfd, p);
		break;
	    }
	}
    }
    if (*view->cur_xlfd) {
	ParseXLFD(view->cur_xlfd, &info);
	strcpy(view->cur_family, info->family);
	GetLook(info, view->cur_look);
	view->cur_size = atoi(info->size);
    }
} /* end of ResetToDefaultFont */
