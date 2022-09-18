/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtadmin:fontmgr/font_prop.c	1.10"

/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       font_prop.c
 */

#include <stdio.h>
#include <Intrinsic.h>
#include <StringDefs.h>

#include <OpenLook.h>

#include <Gizmos.h>
#include <PopupGizmo.h>
#include <MenuGizmo.h>
#include <NumericGiz.h>
#include <ChoiceGizm.h>
#include <CheckBox.h>
#include <LabelGizmo.h>

#include <fontmgr.h>

#define MAX_CACHE_SIZE 4000     /* unit is in Kbytes */
#define MAX_CACHE_CHARS 4
#define MAX_DEFAULT_PS_CHARS 2
#define MIN_DEFAULT_PS       6
#define MAX_DEFAULT_PS      36

/*
 * external procedures
 */
extern char *getenv();

extern Widget       app_shellW;		  /* application shell widget       */
extern Boolean secure;
extern char *xwin_home;

extern void HelpCB();
static void ChangeCatalogCB(Widget, XtPointer, XtPointer);
static void ApplyPropCB();
static void ResetPropCB();
static void CancelPropCB();


static Boolean renderer_exist[E_MAX_PROP]={ 
    TRUE };     /* set gereral item to true */

#define CACHE_INDEX  0        /* this is an index into the general_cfg array */
#define MIN_CACHE_INDEX 1
#define DERIVED_PS_INDEX 2
static config_type general_cfg[]= {
     {"cachesize", DEFAULT_CACHE_SIZE,           MIN_MATCH},
     {"mincachesize", "750",        MIN_MATCH},
     {"derived-instance-pointsizes", DEFAULT_POINT_SIZE, MIN_MATCH},
     {0}   /* terminator */
};

#ifdef old
#define FREE_RENDERER_INDEX 1
#define PREALLOC_INDEX 2
#define PRERENDER_INDEX 4
     {"free-renderer",               "f",          MIN_MATCH},
     {"preallocate-glyphs",          "0",    MIN_MATCH},
     {"prerender-glyphs",            "f",      MIN_MATCH},
#endif

/* this is an index into the renderer specific cfg array */
enum renderer_options { E_DEFAULT_PS, E_PRELOAD, E_USE };
static config_type atm_cfg[] = {
     {"defaultpoint",                DEFAULT_POINT_SIZE, MIN_MATCH},  
     {"preload-renderer",            "f",       MIN_MATCH},
     {"use-renderer",                "t",       MIN_MATCH},
     {0}   /* terminator */
};
static config_type folio_cfg[] = {
     {"defaultpoint",                DEFAULT_POINT_SIZE, MIN_MATCH},  
     {"preload-renderer",            "f",       MIN_MATCH},
     {"use-renderer",                "t",       MIN_MATCH},
     {0}   /* terminator */
};
static config_type speedo_cfg[] = {
     {"defaultpoint",                DEFAULT_POINT_SIZE, MIN_MATCH},  
     {"preload-renderer",            "f",       MIN_MATCH},
     {"use-renderer",                "t",       MIN_MATCH},
     {0}   /* terminator */
};

static string_array_type file_a;
static prop_type prop_info = { general_cfg, atm_cfg };

static HelpInfo help_property = { 0,0, HELP_PATH, TXT_HELP_PROPERTY };

static Arg caption_position_args[] = { XtNposition, OL_LEFT };

#define APPLY_BUT 0
static MenuItems prop_menu_item[] = {  
{ TRUE, TXT_APPLY, 0, 0, ApplyPropCB, (char *)&prop_info },
{ TRUE, TXT_RESET, 0, 0, ResetPropCB, (char *)&prop_info},
{ TRUE, TXT_CANCEL, 0, 0, CancelPropCB, (char *)&prop_info },
{ TRUE, TXT_HELP,   0, 0, HelpCB, (char *)&help_property },
{ NULL }
};

static MenuItems derive_menu_item[] = {  
          { TRUE, TXT_6},
          { TRUE, TXT_8},
          { TRUE, TXT_10},
          { TRUE, TXT_12},
          { TRUE, TXT_14},
          { TRUE, TXT_18},
          { TRUE, TXT_24},
          { TRUE, TXT_36},
	  { NULL }
	      };

static MenuItems atm_boolean_menu_item[] = {  
{ TRUE, TXT_PRELOAD_RENDERER, 0,0,0, (char *)&atm_cfg[E_PRELOAD]},
{ TRUE, TXT_ENABLE_RENDERER, 0,0,0, (char *)&atm_cfg[E_USE]},
{ NULL }
};
static MenuItems folio_boolean_menu_item[] = {  
{ TRUE, TXT_PRELOAD_RENDERER, 0,0,0, (char *)&folio_cfg[E_PRELOAD]},
{ TRUE, TXT_ENABLE_RENDERER, 0,0,0, (char *)&folio_cfg[E_USE]},
{ NULL }
};
static MenuItems speedo_boolean_menu_item[] = {  
{ TRUE, TXT_PRELOAD_RENDERER, 0,0,0, (char *)&speedo_cfg[E_PRELOAD]},
{ TRUE, TXT_ENABLE_RENDERER, 0,0,0, (char *)&speedo_cfg[E_USE]},
{ NULL }
};

static MenuItems catalog_menu_item[] = {
{ TRUE, TXT_GENERAL, 0,0, ChangeCatalogCB, (char *) E_GENERAL_PROP },
{ TRUE, TXT_ATM    , 0,0, ChangeCatalogCB, (char *) E_ATM_PROP },
{ TRUE, TXT_TYPE_SCALER, 0,0, ChangeCatalogCB, (char *) E_FOLIO_PROP },
{ TRUE, TXT_SPEEDO, 0,0, ChangeCatalogCB, (char *) E_SPEEDO_PROP },
{ NULL }
};

static Setting cache_setting = { DEFAULT_CACHE_SIZE };
static Setting atm_default_ps_setting = {0,0,0, (XtPointer) DEFAULT_PS_VALUE };
static Setting folio_default_ps_setting = {0,0,0,(XtPointer)DEFAULT_PS_VALUE };
static Setting speedo_default_ps_setting ={0,0,0,(XtPointer)DEFAULT_PS_VALUE };

static Arg cache_size_args[] = { XtNmaximumSize, MAX_CACHE_CHARS };
static NumericGizmo cache_size_gizmo =
{0, "cs", TXT_CACHE_SIZE, 0, MAX_CACHE_SIZE, &cache_setting, MAX_CACHE_CHARS,
     0, cache_size_args, XtNumber(cache_size_args) };

static Arg default_ps_args[] = { XtNmaximumSize, MAX_DEFAULT_PS_CHARS};
static NumericGizmo atm_default_ps_gizmo =
{0, "cs", TXT_DEFAULT_PS,
     MIN_DEFAULT_PS,
     MAX_DEFAULT_PS,
     &atm_default_ps_setting, 2,
     0, default_ps_args, XtNumber(default_ps_args) };
static NumericGizmo folio_default_ps_gizmo =
{0, "cs", TXT_DEFAULT_PS, MIN_DEFAULT_PS, MAX_DEFAULT_PS,
     &folio_default_ps_setting, 2,
     0, default_ps_args, XtNumber(default_ps_args) };
static NumericGizmo speedo_default_ps_gizmo =
{0, "cs", TXT_DEFAULT_PS, MIN_DEFAULT_PS, MAX_DEFAULT_PS,
     &speedo_default_ps_setting, 2,
     0, default_ps_args, XtNumber(default_ps_args) };

static MenuGizmo atm_boolean_menu =
 {0, "atm_b_m", "dm", atm_boolean_menu_item, 0,0, CHK, OL_FIXEDCOLS  };
static MenuGizmo folio_boolean_menu =
 {0, "folio_b_m", "dm", folio_boolean_menu_item, 0,0, CHK, OL_FIXEDCOLS  };
static MenuGizmo speedo_boolean_menu =
 {0, "speedo_b_m", "dm", speedo_boolean_menu_item, 0,0, CHK, OL_FIXEDCOLS  };

static MenuGizmo derive_menu = 
{0, "derive_menu", "dm", derive_menu_item, 0,0, NON};

static MenuGizmo catalog_menu = { 0, "catalog_menu", NULL, catalog_menu_item };

static Setting derive_setting;
static ChoiceGizmo derive_gizmo = 
{0, "dg", TXT_DERIVE_PS, (Gizmo) &derive_menu, &derive_setting};

static Setting atm_boolean_setting;
static ChoiceGizmo atm_boolean_gizmo = 
{0, "cg", "", (Gizmo) &atm_boolean_menu, &atm_boolean_setting};

static Setting folio_boolean_setting;
static ChoiceGizmo folio_boolean_gizmo = 
{0, "cg", "", (Gizmo) &folio_boolean_menu, &folio_boolean_setting};

static Setting speedo_boolean_setting;
static ChoiceGizmo speedo_boolean_gizmo = 
{0, "cg", "", (Gizmo) &speedo_boolean_menu, &speedo_boolean_setting};

static GizmoRec general_gizmos[] = {
{    NumericGizmoClass, &cache_size_gizmo },
{    ChoiceGizmoClass, &derive_gizmo }
};
static GizmoRec atm_gizmos[] = {
{    NumericGizmoClass, &atm_default_ps_gizmo },
{    ChoiceGizmoClass, &atm_boolean_gizmo }
};
static GizmoRec folio_gizmos[] = {
{    NumericGizmoClass, &folio_default_ps_gizmo },
{    ChoiceGizmoClass, &folio_boolean_gizmo }
};
static GizmoRec speedo_gizmos[] = {
{    NumericGizmoClass, &speedo_default_ps_gizmo },
{    ChoiceGizmoClass, &speedo_boolean_gizmo }
};

static LabelGizmo general_label =
{0, "gl", 0, general_gizmos, XtNumber(general_gizmos), OL_FIXEDCOLS, 1,
     NULL, 0, TRUE };
static LabelGizmo atm_label =
{0, "atm_label", 0, atm_gizmos, XtNumber(atm_gizmos), OL_FIXEDCOLS, 1,
     NULL, 0, TRUE };
static LabelGizmo folio_label =
{0, "folio_label", 0, folio_gizmos, XtNumber(folio_gizmos), OL_FIXEDCOLS, 1,
     NULL, 0, TRUE };
static LabelGizmo speedo_label =
{0, "speedo_label", 0, speedo_gizmos, XtNumber(speedo_gizmos), OL_FIXEDCOLS, 1,
     NULL, 0, TRUE };

static Setting catalog_setting;
static ChoiceGizmo catalog_choice = 
{0, "catalog_choice", TXT_CATEGORY, (Gizmo) &catalog_menu, &catalog_setting};

static GizmoRec prop_gizmos[] = {
{    AbbrevChoiceGizmoClass, &catalog_choice },
{    LabelGizmoClass,   &general_label },
{    LabelGizmoClass,   &atm_label },
{    LabelGizmoClass,   &folio_label },
{    LabelGizmoClass,   &speedo_label }
};

static MenuGizmo prop_menu =
{0, "prop_menu", "dm", prop_menu_item, 0, 0, 0, 0, 0, OL_NO_ITEM };

static PopupGizmo prop_popup = {0, "prop_popup", TXT_FONT_PROPERTIES,
    (Gizmo)&prop_menu, prop_gizmos, XtNumber(prop_gizmos) };


/* returns TRUE if a keyword, value pair is found */
static Boolean
ParseKeywordValue(
    char* line,
    char* keyword,
    char* value)
{
    int len;
    char *start, *end;

    start = line;
    if (SkipSpace(&start) == FALSE)
	return FALSE;

    /* if comment then return */
    if (start[0]=='#')
	return FALSE;

    /* parse keyword */
    end = start;
    if (FindChar(&end, '=') == FALSE)
	return FALSE;
    len = end - start;
    strncpy(keyword, start, len);
    keyword[len] = 0; /* string terminator */

    /* parse value */
    start = end + 1;
    if (SkipSpace(&start) == FALSE)
	return FALSE;
    end = start;
    if (FindSpace(&end) == FALSE)
	return FALSE;
    len = end - start;
    strncpy(value, start, len);
    value[len] = 0;  /* string terminator */
    return TRUE;

} /* ParseKeywordValue */


static Boolean
ParseRendererType(prop_type* info,
		  char* keyword,
		  char* value)
{
    Boolean found_renderer = FALSE;

    if (strncmp(keyword, "startrenderer", 6) == STR_MATCH) {
	/* mark which renderer we are parsing */
	LowercaseStr(value);
	if (strncmp(value, "f3b", 3) == STR_MATCH) {
	    info->cur_cfg = folio_cfg;
	    info->cur_parse_section = E_FOLIO_PROP;
	    renderer_exist[info->cur_parse_section] = TRUE;
	    found_renderer = TRUE;
	}
	else if (strncmp(value, "spd", 3) == STR_MATCH) {
	    info->cur_cfg = speedo_cfg;
	    info->cur_parse_section = E_SPEEDO_PROP;
	    renderer_exist[info->cur_parse_section] = TRUE;
	    found_renderer = TRUE;
	}
	else if (strncmp(value, "pfa", 3) == STR_MATCH) {
	    info->cur_cfg = atm_cfg;
	    info->cur_parse_section = E_ATM_PROP;
	    renderer_exist[info->cur_parse_section] = TRUE;
	    found_renderer = TRUE;
	}
	else if (strncmp(value, "pfb", 3) == STR_MATCH) {
	    info->cur_cfg = atm_cfg;
	    info->cur_parse_section = E_ATM_PROP;
	    renderer_exist[info->cur_parse_section] = TRUE;
	    found_renderer = TRUE;
	}
    }
    return found_renderer;

} /* ParseRendererType */


static config_type*
GetRendererCfg(prop_type* info,
	       char* line)
{
    char keyword[MAX_STRING], value[MAX_PATH_STRING];
    config_type *cfg;

    if (!ParseKeywordValue(line, keyword, value))
	return NULL;

    if (!ParseRendererType(info, keyword, value))
	return NULL;

    return info->cur_cfg;

} /* end of GetRendererCfg */


static Boolean
InsertConfigDB(prop_type* info,
	       char* line)
{
    char keyword[MAX_STRING], value[MAX_PATH_STRING];
    config_type *cfg;
    int i;

    if (!ParseKeywordValue(line, keyword, value))
	return FALSE;

    if (ParseRendererType(info, keyword, value))
	return FALSE;

    cfg = info->cur_cfg;
    for(i=0; *cfg[i].keyword; i++) {
	if (strncmp( keyword, cfg[i].keyword, cfg[i].match_len) ==STR_MATCH) {
	    strcpy(cfg[i].value, value);
	    return TRUE;
	}
    }
    return FALSE;

} /* InsertConfigDB */


static Boolean
IsStartRenderer(file_a, cfg, line_index, start_end_str)
    string_array_type *file_a;
    config_type *cfg;
    int line_index;
    char *start_end_str;
{
    int i;
    char new[MAX_PATH_STRING];
    char *line = file_a->strs[line_index];

    /* if comment then return FALSE*/
    if (line[0] == '#')
	return FALSE;

    /* replace keyword-value pair with value that user specified*/
    for(i=0; *cfg[i].keyword; i++) {
	if ((cfg[i].replaced == FALSE) && 
	    (strncmp(line, cfg[i].keyword, cfg[i].match_len) == STR_MATCH)) {
	    XtFree( file_a->strs[line_index]);
	    sprintf(new, "%s=%s\n", cfg[i].keyword, cfg[i].value);
	    file_a->strs[line_index] = XtNewString(new);
	    cfg[i].replaced = TRUE;
	    return FALSE;
	}
    }

    if (strncmp(line, start_end_str, MIN_MATCH)==STR_MATCH)
	return TRUE;
    return FALSE;
}


static void
WriteToFile(file, cfg, str_info, start, end)
    FILE *file;
    config_type *cfg;
    string_array_type *str_info;
    int start, end;
{
    int i;
    char str[MAX_PATH_STRING];

    for(; start <= end; start++) {
	fputs(str_info->strs[start], file);
    }

    /* output any lefted out keyword-value pair */
    for(i=0; *cfg[i].keyword; i++) {
	if (cfg[i].replaced == FALSE) {
	    sprintf(str, "%s=%s\n", cfg[i].keyword, cfg[i].value);
	    fputs(str, file);
	}
    }
} /* end of WriteToFile */
    
static void
InitCfgReplace(cfg)
    config_type *cfg;
{
    int i;

    for(i=0; *cfg[i].keyword; i++) {
	cfg[i].replaced = FALSE;
    }
}

static void
InitCfg(cfg)
    config_type *cfg;
{
    int i;

    for(i=0; *cfg[i].keyword; i++) {
	strcpy(cfg[i].value, cfg[i].default_value);
    }
    InitCfgReplace(cfg);
} /* end of InitCfg */


static char *
ParseDerivedPS(derive_str)
    char *derive_str;
{
    char *fieldP;
    int len, i;

#define DERIVE_DELIM ','
    derive_str = (char *) GetNextField(DERIVE_DELIM, derive_str, &fieldP, &len);
    if (len > 0)
	for(i=0; derive_menu_item[i].label; i++)
	    if (strncmp( derive_menu_item[i].label, fieldP, len) == STR_MATCH) {
		OlVaFlatSetValues(derive_menu.child, i, XtNset,
			  TRUE, (String) 0);
		break;
	    }

    return derive_str;

} /* end of ParseDerivedPS */


static void
ResetBoolean(Widget w, MenuItems* menu_item)
{
    int i;
    config_type *cfg;
    
    for(i=0; menu_item[i].label; i++) {
	cfg = (config_type *) menu_item[i].client_data;
	switch(toupper(*cfg->value)) {
	case 'T':
	case 'Y':
	case '1':
	    OlVaFlatSetValues(w, i, XtNset, TRUE, (String) 0);
	    break;
	default:
	    OlVaFlatSetValues(w, i, XtNset, FALSE, (String) 0);
	}
    }
} /* end of ResetBoolean */


static void
GetBoolean( menu_item)
    MenuItems *menu_item;
{
    int i;
    config_type *cfg;
    
    for(i=0; menu_item[i].label; i++) {
	cfg = (config_type *) menu_item[i].client_data;
	sprintf( cfg->value, "%s", menu_item[i].set ? "y" : "n");
    }

} /* end of GetBoolean */


static Boolean
ReadCfg( prop_info, clean_up)
    prop_type *prop_info;
    Boolean clean_up;
{
    FILE *file;
    char *dir;
    char line[MAX_PATH_STRING];

    /* open file */
    if ((dir = getenv("XWINFONTCONFIG"))==NULL) {
	sprintf(prop_info->filename, "%s/defaults/Xwinfont", xwin_home);
    }
    else 
	strcpy(prop_info->filename, dir);
    
    file = fopen(prop_info->filename, "r");
    if (!FileOK(file)) {
	sprintf(line, "Can't open config file: %s\n", prop_info->filename);
	InformUser(line);
	return FALSE;
    }
    
    InitCfg(general_cfg);
    InitCfg(atm_cfg);
    InitCfg(folio_cfg);
    InitCfg(speedo_cfg);

    prop_info->cur_parse_section = E_GENERAL_PROP;
    prop_info->cur_cfg = general_cfg;
    while (fgets(line, MAX_PATH_STRING, file) != NULL) {
	InsertStringDB(&file_a, line);
	InsertConfigDB(prop_info, line);
    }

    fclose(file);

    if (clean_up)
	DeleteStringsDB(&file_a);
    return TRUE;

} /* end of ReadCfg */


String
GetDerivedPS() 
{
    if (ReadCfg(&prop_info, TRUE))
	return general_cfg[DERIVED_PS_INDEX].value;
    else
	return NULL;

} /* end of GetDerivedPS */


static Boolean
WriteCfg( prop_info)
    prop_type *prop_info;
{
    FILE *file;
    int i;
    char line[MAX_PATH_STRING];
    int write_start;
    config_type *cfg;

    file = fopen(prop_info->filename, "w");
    if (!FileOK(file)) {
	sprintf(line, "Can't open config file: %s for writing\n", prop_info->filename);
	InformUser(line);
	return FALSE;
    }

    cfg = (config_type *) &general_cfg;
    write_start = 0;
    for(i=0; i<file_a.n_strs; i++) {
	if (IsStartRenderer(&file_a, cfg, i, "startrenderer")) {
	    WriteToFile(file, cfg, &file_a, write_start, i-1);
	    write_start = i;
	    cfg = GetRendererCfg(prop_info, file_a.strs[i]);
#ifdef old
	    InitCfgReplace(cfg);
#endif
	}
    }
    /* write out last renderer */
    WriteToFile(file, cfg, &file_a, write_start, i-1);
    fclose(file);
    
    DeleteStringsDB(&file_a);
    return TRUE;

} /* end of WriteCfg */


static void
ManageCurProp(prop_type *info)
{
    /* change the preview label */
    XtVaSetValues(catalog_choice.previewWidget, XtNstring,
		  catalog_menu_item[info->cur_prop].label, 0);

    switch (info->cur_prop) {
    case E_GENERAL_PROP:
	XtUnmanageChild(atm_label.controlWidget);
	XtUnmanageChild(folio_label.controlWidget);
	XtUnmanageChild(speedo_label.controlWidget);
	XtManageChild(general_label.controlWidget);
	break;
    case E_ATM_PROP:
	XtUnmanageChild(general_label.controlWidget);
	XtUnmanageChild(folio_label.controlWidget);
	XtUnmanageChild(speedo_label.controlWidget);
	XtManageChild(atm_label.controlWidget);
	break;
    case E_FOLIO_PROP:
	XtUnmanageChild(general_label.controlWidget);
	XtUnmanageChild(atm_label.controlWidget);
	XtUnmanageChild(speedo_label.controlWidget);
	XtManageChild(folio_label.controlWidget);
	break;
    case E_SPEEDO_PROP:
	XtUnmanageChild(general_label.controlWidget);
	XtUnmanageChild(atm_label.controlWidget);
	XtUnmanageChild(folio_label.controlWidget);
	XtManageChild(speedo_label.controlWidget);
	break;
    }
} /* ManageCurProp */


void
FontSetupPropCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    config_type *general = prop_info.general;
    int i;
    char *derive_str;
    char line[MAX_STRING];
    Widget upper_w;

    prop_info.cur_prop = (int) client_data;

    if (ReadCfg(&prop_info, TRUE) == FALSE)
	return;

    /* reset the Gizmo values so we don't get warnings */
    cache_setting.previous_value = (XtPointer)atoi(general[CACHE_INDEX].value);
    atm_default_ps_setting.previous_value = (XtPointer) atoi(atm_cfg[E_DEFAULT_PS].value);
    folio_default_ps_setting.previous_value = (XtPointer) atoi(folio_cfg[E_DEFAULT_PS].value);
    speedo_default_ps_setting.previous_value = (XtPointer) atoi(speedo_cfg[E_DEFAULT_PS].value);

    if (prop_info.popup == 0) {
	prop_menu_item[APPLY_BUT].sensitive = !secure;
	for(i=0; i<E_MAX_PROP; i++)
	    catalog_menu_item[i].sensitive = renderer_exist[i];
	prop_info.popup = CreateGizmo(w, PopupGizmoClass,
				  &prop_popup, NULL, 0);
    }
    else {
	/* reset the widget values */
	XtVaSetValues(cache_size_gizmo.textFieldWidget, XtNvalue, 
		      atoi(general[CACHE_INDEX].value), 0);
	XtVaSetValues(atm_default_ps_gizmo.textFieldWidget, XtNvalue, 
		      atoi(atm_cfg[E_DEFAULT_PS].value), 0);
	XtVaSetValues(folio_default_ps_gizmo.textFieldWidget, XtNvalue, 
		      atoi(folio_cfg[E_DEFAULT_PS].value), 0);
	XtVaSetValues(speedo_default_ps_gizmo.textFieldWidget, XtNvalue, 
		      atoi(speedo_cfg[E_DEFAULT_PS].value), 0);
    }

    /* reset the 'set' field of the derive_menu_item */
    for(i=0; derive_menu_item[i].label; i++)
	OlVaFlatSetValues(derive_menu.child, i, XtNset, FALSE, (String) 0);

    /* fill in the 'set' field of the derive_menu_item */
    sprintf( line, ",%s", general[DERIVED_PS_INDEX].value);
    derive_str = line;
    while (*derive_str)
	derive_str = ParseDerivedPS(derive_str);

    ResetBoolean( atm_boolean_menu.child, atm_boolean_menu_item);
    ResetBoolean( folio_boolean_menu.child, folio_boolean_menu_item);
    ResetBoolean( speedo_boolean_menu.child, speedo_boolean_menu_item);
    ManageCurProp(&prop_info);
    MapGizmo(PopupGizmoClass, &prop_popup);

} /* end if FontSetupPropCB */


static void
InRange(NumericGizmo* gizmo,
	Setting*      setting)
{
    if ((int)setting->current_value < gizmo->min)
	setting->current_value = (XtPointer) gizmo->min;
    else if ((int)setting->current_value > gizmo->max)
	setting->current_value = (XtPointer) gizmo->max;

} /* end of InRange */


void
ApplyPropCB(w, client, call )
Widget	     w;
XtPointer    client;
XtPointer    call;
{
    prop_type *prop_info = (prop_type *) client;
    int value;
    config_type *general = prop_info->general;
    int i;
    char *p;
    Boolean first_time = TRUE;

    /* read the file again in case some user modified it manually
       after our last read */
    ReadCfg( prop_info, FALSE);

    XtVaGetValues(cache_size_gizmo.textFieldWidget, XtNvalue, 
		  &cache_setting.current_value, 0);
    XtVaGetValues(atm_default_ps_gizmo.textFieldWidget, XtNvalue, 
		  &atm_default_ps_setting.current_value, 0);
    XtVaGetValues(folio_default_ps_gizmo.textFieldWidget, XtNvalue, 
		  &folio_default_ps_setting.current_value, 0);
    XtVaGetValues(speedo_default_ps_gizmo.textFieldWidget, XtNvalue, 
		  &speedo_default_ps_setting.current_value, 0);

    sprintf(atm_cfg[E_DEFAULT_PS].value, "%d", 
	    atm_default_ps_setting.current_value);
    sprintf(folio_cfg[E_DEFAULT_PS].value, "%d",
	    folio_default_ps_setting.current_value);
    sprintf(speedo_cfg[E_DEFAULT_PS].value, "%d",
	    speedo_default_ps_setting.current_value);
    value = (int) cache_setting.current_value;
    sprintf(general[CACHE_INDEX].value, "%d", value);
    value *= 0.9;
    sprintf(general[MIN_CACHE_INDEX].value, "%d", value);

    p = general[DERIVED_PS_INDEX].value;
    *p = 0;      /* init string */
    for(i=0; derive_menu_item[i].label; i++) {
	if (derive_menu_item[i].set) {
	    if (first_time)
		first_time = FALSE;
	    else
		strcat(p, ",");
	    strcat(p, derive_menu_item[i].label);
	}
    }

    GetBoolean( atm_boolean_menu_item);
    GetBoolean( folio_boolean_menu_item);
    GetBoolean( speedo_boolean_menu_item);

    if (WriteCfg( prop_info))
	InformUser(TXT_RESTART_X);
    XtPopdown( prop_info->popup);

} /* end of ApplyPropCB */


void
ResetPropCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    prop_type *info = (prop_type *) client_data;

    FontSetupPropCB((Widget)0, (XtPointer) info->cur_prop, (XtPointer)0);

} /* end if ResetPropCB */


void
CancelPropCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    prop_type *prop_info = (prop_type *) client_data;

    XtPopdown( prop_info->popup);

} /* end of CancelPropCB */


static void 
ChangeCatalogCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    prop_info.cur_prop = (int) client_data;
    ManageCurProp(&prop_info);

} /* end of ChangeCatalogCB */


void
SetRendererSensitivity(MenuItems* menu_item)
{
    int i;

    if (ReadCfg(&prop_info, TRUE)) {
	for(i=0; i<E_MAX_PROP; i++)
	    menu_item[i].sensitive = renderer_exist[i];
    }

} /* end of CheckRenderer */
