/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtadmin:fontmgr/message.h	1.14"

/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       message.h
 */

#ifndef __fontmgr_Message_h__
#define __fontmgr_Message_h__

/**********************************
 Gizmo strings
*******************************/

#define ClientName           "Font Setup"

#define HELP_FILE             "font.hlp"
#define HELP_PATH             "dtadmin/" HELP_FILE
#define MESS_FILE		"fontsetup"

#define TXT_6                MESS_FILE ":1"   FS "6"
#define TXT_8                MESS_FILE ":2"   FS "8"
#define TXT_10               MESS_FILE ":3"   FS "10"
#define TXT_12               MESS_FILE ":4"   FS "12"
#define TXT_14               MESS_FILE ":5"   FS "14"
#define TXT_18               MESS_FILE ":6"   FS "18"
#define TXT_24               MESS_FILE ":7"   FS "24"
#define TXT_36               MESS_FILE ":8"   FS "36"

#define TXT_ADD_APPLY        MESS_FILE ":10"  FS "Install"
#define TXT_ADD_APPLY_ALL    MESS_FILE ":11"  FS "Install All"
#define TXT_ADD_DDD          MESS_FILE ":12"  FS "Install..."
#define TXT_ADD_FINISH       MESS_FILE ":13"  FS "Installation of fonts completed."
#define TXT_ADD_GAUGE_CAPTION MESS_FILE ":14"  FS "Installation Progress"
#define TXT_ADD_HINT         MESS_FILE ":15"  FS "To install fonts, select the font names from the list."
#define TXT_ADD_LIST_CAPTION MESS_FILE ":16"  FS "Fonts on Diskette"
#define TXT_ADD_START        MESS_FILE ":17"  FS "Installing fonts; this will take some time."
#define TXT_APPLIED_DEFAULT_FONT_TO_ALL MESS_FILE ":18" FS \
"Default font will now be used in windows."
#define TXT_APPLIED_FONT_TO_ALL MESS_FILE ":19" FS "Current font will now be used in windows."
#define TXT_APPLIED_FONT_TO_XTERM MESS_FILE ":20" FS "Current font will be used in new \"Terminal\" windows."
#define TXT_APPLY            MESS_FILE ":21"  FS "Apply"
#define TXT_APPLY_ALL        MESS_FILE ":22"  FS "Apply All"
#define TXT_APPLY_DEFAULT_FONT  MESS_FILE ":23" FS "Restore Default"
#define TXT_APPLY_FONT_ALL   MESS_FILE ":24"  FS "Apply to Windows"
#define TXT_APPLY_FONT_XTERM MESS_FILE ":25"  FS "Apply to \"Terminal\" Only"
#define TXT_ATM              MESS_FILE ":26"  FS "Type 1"

#define TXT_BAD_FONT         MESS_FILE ":30"  FS "Cannot display font"
#define TXT_BAD_XTERM_FONT   MESS_FILE ":31"  FS "Cannot apply this font to \
\"Terminal\" windows."
#define TXT_BITMAP_DDD       MESS_FILE ":32"  FS "Bitmapped..."
#define TXT_BLANK            MESS_FILE ":33"  FS " "
#define TXT_CACHE_SIZE       MESS_FILE ":34"  FS "Cache Size (KBytes)"
#define TXT_CANCEL           MESS_FILE ":35"  FS "Cancel"
#define TXT_CATEGORY         MESS_FILE ":36"  FS "CATEGORY"
#define TXT_CHAR_SET         MESS_FILE ":37"  FS "\
\040\041\042\043\044\045\046\047\050\051\052\053\054\055\056\057\n\
0123456789\072\073\074\075\076\077\100\n\
ABCDEFGHIJKLMNOPQRSTUVWXYZ\n\
abcdefghijklmnopqrstuvwxyz\n\
\133\134\135\136\137\140\173\174\175\176\177\n\
\200\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\n\
\220\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\n\
\240\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\n\
\260\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\n\
\300\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\n\
\320\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\n\
\340\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\n\
\360\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377"
#define TXT_CONFIRM_MESS     MESS_FILE ":38"  FS "Deleting these fonts \
removes them permanently from your system.  If you do this, the only way to \
re-install them is to obtain the original media from which they were first \
installed and re-install them."
#define TXT_CONTINUE         MESS_FILE ":39"  FS "Continue"

#define TXT_DEFAULT_PS       MESS_FILE ":40"  FS "Default Point Size"
#define TXT_DEL_BITMAP       MESS_FILE ":41"  FS ClientName ": Delete Bitmapped Fonts"
#define TXT_DEL_OUTLINE      MESS_FILE ":42"  FS ClientName ": Delete Outline Fonts"
#define TXT_DELETE           MESS_FILE ":43"  FS "Delete"
#define TXT_DELETE_APPLY     MESS_FILE ":44"  FS "Delete"
#define TXT_DELETE_FINISH    MESS_FILE ":45"  FS "Deletion of fonts completed"
#define TXT_DELETE_START     MESS_FILE ":46"  FS "Deleting fonts..."
#define TXT_DERIVE_PS        MESS_FILE ":47"  FS "Derived Instance Point Sizes"

#define TXT_EDIT             MESS_FILE ":50"  FS "Font"
#define TXT_ENABLE_RENDERER  MESS_FILE ":51"  FS "Enable Renderer"
#define TXT_EXIT             MESS_FILE ":53"  FS "Exit"

#define TXT_FILE             MESS_FILE ":60"  FS "Actions"
#define TXT_FONT_DELETE      MESS_FILE ":62"  FS ClientName ": Delete"
#define TXT_FONT_ADD         MESS_FILE ":63"  FS ClientName ": Install Type 1 Fonts from Diskette"
#define TXT_FONT_NAME_MENU_LABEL MESS_FILE ":64"  FS "Font Name"
#define TXT_FONT_PROPERTIES  MESS_FILE ":65"  FS ClientName ": Properties"
#define TXT_FREE_RENDERER    MESS_FILE ":66"  FS "Free Renderer"
#define TXT_FULL_SAMPLE      MESS_FILE ":67"  FS "Character Set"
#define TXT_GENERAL          MESS_FILE ":68"  FS "General"

#define TXT_HELP             MESS_FILE ":70"  FS "Help"
#define TXT_HELP_ABOUT       MESS_FILE ":71"  FS "Font Setup..."
#define TXT_HELP_DESK        MESS_FILE ":72"  FS "Help Desk..."
#define TXT_HELP_DDD         MESS_FILE ":73"  FS "Help..."
#define TXT_HELP_TOC         MESS_FILE ":74"  FS "Table of Contents..."

#define TXT_ICON_TITLE       MESS_FILE ":80"  FS "Font_Setup"
#define TXT_INSERT_DISK      MESS_FILE ":81"  FS "Insert DOS diskette containing\n\
 Type 1 outline fonts into the drive and click \"Continue\"."
#define TXT_INSERT_DOS_DISK  MESS_FILE ":82"  FS "Diskette is not recognizable as a\n\
DOS diskette.  Insert another and click \"Continue\"."
#define TXT_INSERT_ATM_DISK  MESS_FILE ":83"  FS "Diskette does not contain Type 1\n\
 fonts in a recognizable format.  Insert another and click \"Continue\"."
#define TXT_INTEGRITY        MESS_FILE ":84"  FS "Integrity Check"

#define TXT_NO               MESS_FILE ":180" FS "No"
#define TXT_NO_DELETABLE_BITMAP MESS_FILE ":181" FS "There are no deletable bitmapped fonts."
#define TXT_NO_DELETABLE_OUTLINE MESS_FILE ":182" FS "There are no deletable outline fonts."
#define TXT_NONE_ADD_SEL     MESS_FILE ":183" FS "No fonts were selected for \
installation."
#define TXT_NONE_DELETE      MESS_FILE ":184" FS "No fonts were selected for deletion."
#define TXT_NORMAL           MESS_FILE ":185" FS "Normal"
#define TXT_NOTICE_TITLE     MESS_FILE ":186" FS ClientName " : Notice"

#define TXT_OFF              MESS_FILE ":190" FS "Off"
#define TXT_OK               MESS_FILE ":191" FS "OK"
#define TXT_ON               MESS_FILE ":192" FS "On"
#define TXT_OUTLINE_DDD      MESS_FILE ":193" FS "Outline..."

#define TXT_POINT_SIZE       MESS_FILE ":200" FS "Point Size"
#define TXT_POINT_SIZE_NOT_IN_RANGE MESS_FILE ":201" FS "Point size not in valid range"
#define TXT_PREALLOCATE_GLYPHS MESS_FILE ":202" FS "Preallocate Glyphs (%)"
#define TXT_PRELOAD_RENDERER MESS_FILE ":203" FS "Preload Renderer"
#define TXT_PRERENDER_GLYPHS MESS_FILE ":204" FS "Prerender Glyphs"
#define TXT_PROMPT_TITLE     MESS_FILE ":205" FS ClientName " : Notice"
#define TXT_PROPERTIES       MESS_FILE ":206" FS "Properties"
#define TXT_RESET            MESS_FILE ":207" FS "Reset"
#define TXT_RESTART_X        MESS_FILE ":208" FS "Properties will take affect \
at next desktop session"

#define TXT_SAMPLE_MENU_LABEL MESS_FILE ":220" FS "Display"
#define TXT_SAMPLE_TEXT      MESS_FILE ":221" FS "Type in here"
#define TXT_SHORT_NAME       MESS_FILE ":222" FS "Short"
#define TXT_SHORT_SAMPLE     MESS_FILE ":223" FS "Phrase"
#define TXT_SPEEDO           MESS_FILE ":224" FS "Speedo"
#define TXT_STYLE            MESS_FILE ":225" FS "Style"

#define TXT_TYPE_SCALER      MESS_FILE ":230" FS "F3"
#define TXT_TYPEFACE_FAMILY  MESS_FILE ":231" FS "Typeface Family"
#define TXT_VIEW             MESS_FILE ":232" FS "View"
#define TXT_WINDOW_TITLE     MESS_FILE ":233" FS ClientName

#define TXT_XLFD_NAME        MESS_FILE ":270" FS "XLFD"

#define TXT_YES              MESS_FILE ":280" FS "Yes"


#define TXT_HELP_INSERT_DISK     HELP_FILE ":301" FS "Notice: Insert a DOS Diskette"
#define TXT_HELP_INSERT_ATM_DISK HELP_FILE ":302" FS "Notice: No Recognizable Type 1 Fonts"
#define TXT_HELP_INSERT_DOS_DISK HELP_FILE ":303" FS "Notice: Not a DOS Diskette"
#define TXT_HELP_MAIN_MENU       HELP_FILE ":304" FS "Font Setup"
#define TXT_MAIN_HELP_SECTION    HELP_FILE ":305" FS "Font Setup"
#define TXT_HELP_DEL_BITMAP      HELP_FILE ":306" FS "Font Setup: Delete Bitmapped/Outline Fonts Window"
#define TXT_HELP_DEL_BITMAP_WARN HELP_FILE ":307" FS "Delete Notice Window"
#define TXT_HELP_DEL_OUTLINE     HELP_FILE ":308" FS "Font Setup: Delete Bitmapped/Outline Fonts Window"
#define TXT_HELP_DEL_OUTLINE_WARN HELP_FILE ":309" FS "Delete Notice Window"
#define TXT_HELP_PROPERTY        HELP_FILE ":310" FS "Font Setup: Properties Window"
#define TXT_HELP_INSTALL         HELP_FILE ":311" FS "Font Setup: Install Type 1 Fonts from Diskette Window"

#define ACCEL_BASE_FILE                  MESS_FILE ":400" FS "A"
#define ACCEL_BASE_VIEW                  MESS_FILE ":401" FS "V"
#define ACCEL_BASE_EDIT                  MESS_FILE ":402" FS "F"
#define ACCEL_BASE_HELP                  MESS_FILE ":403" FS "H"

#define ACCEL_FILE_ADD                   MESS_FILE ":404" FS "I"
#define ACCEL_FILE_DELETE                MESS_FILE ":405" FS "D"
#define ACCEL_FILE_INTEGRITY             MESS_FILE ":406" FS "N"
#define ACCEL_FILE_EXIT                  MESS_FILE ":407" FS "E"

#define ACCEL_FILE_DELETE_BITMAP         MESS_FILE ":408" FS "B"
#define ACCEL_FILE_DELETE_OUTLINE        MESS_FILE ":409" FS "O"

#define ACCEL_VIEW_FONT                  MESS_FILE ":410" FS "F"
#define ACCEL_VIEW_DISPLAY      	 MESS_FILE ":411" FS "D"

#define ACCEL_VIEW_FONT_SHORT            MESS_FILE ":412" FS "S"
#define ACCEL_VIEW_FONT_XLFD    	 MESS_FILE ":413" FS "X"

#define ACCEL_VIEW_DISPLAY_PHASE         MESS_FILE ":414" FS "P"
#define ACCEL_VIEW_DISPLAY_CHAR     	 MESS_FILE ":415" FS "C"

#define ACCEL_EDIT_DEFAULT               MESS_FILE ":416" FS "R"
#define ACCEL_EDIT_ALL                   MESS_FILE ":417" FS "A"
#define ACCEL_EDIT_TERMINAL     	 MESS_FILE ":418" FS "T"

#define ACCEL_HELP_FONT                  MESS_FILE ":450" FS "F"
#define ACCEL_HELP_TABLE        	 MESS_FILE ":451" FS "T"
#define ACCEL_HELP_DESK           	 MESS_FILE ":452" FS "H"

#define ACCEL_ADD_APPLY                  MESS_FILE ":453" FS "I"
#define ACCEL_ADD_APPLY_ALL		 MESS_FILE ":454" FS "L"
#define ACCEL_ADD_CANCEL        	 MESS_FILE ":455" FS "C"
#define ACCEL_ADD_HELP     		 MESS_FILE ":456" FS "H"

#define ACCEL_DELETE_APPLY               MESS_FILE ":460" FS "D"
#define ACCEL_DELETE_RESET      	 MESS_FILE ":461" FS "R"
#define ACCEL_DELETE_CANCEL		 MESS_FILE ":462" FS "C"
#define ACCEL_DELETE_HELP  		 MESS_FILE ":463" FS "H"

#define ACCEL_PROMPT_CONTINUE      	 MESS_FILE ":465" FS "C"
#define ACCEL_PROMPT_CANCEL     	 MESS_FILE ":466" FS "A"
#define ACCEL_PROMPT_HELP    		 MESS_FILE ":467" FS "H"

#define ACCEL_NOTICE_OK          	 MESS_FILE ":468" FS "O"

#endif /* __fontmgr_Message_h__ */  
