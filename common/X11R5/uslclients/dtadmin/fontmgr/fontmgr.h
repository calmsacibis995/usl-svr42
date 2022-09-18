/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtadmin:fontmgr/fontmgr.h	1.9"

/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       fontmgr.h
 */

#include <arena.h>
#include <message.h>

#define AFM_DIR "afm"

#define MAX_PATH_STRING 512
#define MAX_STRING 128
#define STR_MATCH 0
#define STR_ALLOC 50
#define MIN_MATCH 5

#define FIELD_COUNT 14
#define DELIM '-'
#define PROPORTIONAL_SPACING 'p'
#define RELEVANT_FIELD_COUNT 12
#define NUM_FAMILY_ITEMS 10
#define NUM_LIST_ITEMS  4

#define FAMILY 0
#define LOOK 1
#define POINT 2

#define MIN_PS_VALUE  6
#define MAX_PS_VALUE  100

#define DEFAULT_CACHE_SIZE "800"
#define DEFAULT_POINT_SIZE "12"
#define DEFAULT_PS_VALUE    12

#define DEFAULT_FAMILY "Lucida"
#define DEFAULT_LOOK   "Bold   "

/* this must be updated when catalog_menu_item[] is updated */
enum prop_constant { E_GENERAL_PROP, E_ATM_PROP, E_FOLIO_PROP,
			 E_SPEEDO_PROP, E_MAX_PROP };

/*
 * user defined types
 */
typedef struct _string_array_type {
  char **strs;
  int n_strs;
  int alloc_strs;
} string_array_type;

typedef struct _config_type {
    char keyword[MAX_STRING];
    char default_value[MAX_STRING];
    int  match_len;
    char value[MAX_PATH_STRING];
    Boolean replaced;
    Boolean renderer_exist;
} config_type;

typedef struct add_db {
    char file_name[MAX_STRING];
    int    pfb_disk;
    int    afm_disk;
} add_db;

typedef struct _add_type {             /* donot rearrange the order of the
					  fields because they get initialize
					  in the declaration */
    string_array_type *font_name; 
    string_array_type *disk_label;
    char device[PATH_MAX];
    Widget popup;
    Widget prompt;
    Widget font_list;
    Widget gauge;
    add_db *db;
    int font_cnt;
    int select_cnt;
    int slider_val;
    int disk_num;
    Boolean adobe_foundry;
} add_type;

typedef struct _prop_type {
    config_type *general;
    config_type *cur_cfg;
    Widget popup;
    int cur_prop;
    int cur_parse_section;
    char filename[MAX_PATH_STRING];
} prop_type;

typedef struct _font_type {
    char * xlfd_name;
    char *truncated_xlfd;
    Boolean bitmap;
} font_type;

typedef struct _family_info {
    char * n;
    _OlArenaType(LookArena) * l;
} family_info;
_OlArenaStruct(family_info, FamilyArena);

typedef struct _look_info {
    char * look_name;          /* XtNlabel */
    _OlArenaType(PSArena) * l;
} look_info;
_OlArenaStruct(look_info, LookArena);

typedef struct _ps_info {
    char * ps;
    font_type *l;              /* XtNuserData */
} ps_info;
_OlArenaStruct(ps_info, PSArena);

typedef struct _delete_type {             /* donot rearrange the order of the
					  fields because they get initialize
					  in the declaration */
    string_array_type *font_name;
    string_array_type *xlfd;
    string_array_type *selected_xlfd;
    string_array_type *file_name;
    string_array_type *selected_dir;
    Widget popup;
    Widget font_list;
    Widget confirm;
    Boolean bitmap;
} delete_type;

typedef struct _view_type {              /* donot rearrange the order of the
					  fields because they get initialize
					  in the declaration */
    _OlArenaType(FamilyArena) *family_arena;
    Widget form;
    Widget upper;
    Widget family_caption;
    Widget family_exclusive;
    Widget look_caption;
    Widget look_exclusive;
    Widget size_caption;
    Widget size_exclusive;
    Widget sample_text;
    Widget footer_text;
    Widget size_window;
    Widget ps_text;
    int font_state[3];
    String outline_xlfd;
    Boolean bitmap;
    char cur_family[MAX_STRING];
    char cur_look[MAX_STRING];
    int  cur_size;
    char cur_xlfd[MAX_PATH_STRING];
    char prev_xlfd[MAX_PATH_STRING];
    XtIntervalId timer_id;
    _OlArenaType(PSArena) * ps_arena;
} view_type;

typedef struct _slant_type {
    String code;
    String translation;
} slant_type;

typedef struct _xlfd_type {
    char family[MAX_STRING];
    char weight[40];
    char slant[40];
    char set_width[40];
    char add_style[40];
    char size[40];
    char spacing[40];
    Boolean bitmap;
    char truncated_xlfd[MAX_PATH_STRING];
} xlfd_type;
