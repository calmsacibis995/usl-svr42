/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtedit:editor.h	1.22"
#endif

/*
 * editor.h
 *
 */

#ifndef _editor_h
#define _editor_h

#include <sys/types.h>
#include <sys/stat.h>

#define UNTITLED_BUFFER  (1L<<0)

#define ERROR_MESSAGE     1
#define STATUS_MESSAGE    0
#define CLEAR_MESSAGE    -1

extern char * print_command;

#define DEFAULT_PRINT_COMMAND        "/usr/X/bin/wrap | /usr/X/bin/PrtMgr"
#define PRINT_COMMAND                print_command

#define HELPPATH             "dtedit" "/" "edit.hlp"

#define FormalClientName     "dtedit:1" FS "Text Editor"
#define TXT_CLIENT_NAME      "dtedit:2" FS "Text Editor: "
#define TXT_ICON_NAME        NULL

#define TXT_CLEAR            "dtedit:9" FS "Clear"

#define TXT_NEW              "dtedit:10" FS "New..."
#define TXT_OPEN             "dtedit:11" FS "Open..."
#define TXT_SAVE             "dtedit:12" FS "Save"
#define TXT_SAVE_AS          "dtedit:13" FS "Save As..."
#define TXT_PRINT            "dtedit:14" FS "Print"
#define TXT_EXIT             "dtedit:15" FS "Exit"

#define TXT_UNDO             "dtedit:20" FS "Undo"
#define TXT_CUT              "dtedit:21" FS "Cut"
#define TXT_COPY             "dtedit:22" FS "Copy"
#define TXT_PASTE            "dtedit:23" FS "Paste"
#define TXT_DELETE           "dtedit:24" FS "Delete"
#define TXT_SELECTALL        "dtedit:25" FS "Select All"
#define TXT_UNSELECTALL      "dtedit:26" FS "Unselect All"

#define TXT_SPLIT            "dtedit:30" FS "Another..."
#define TXT_PROPERTIES       "dtedit:31" FS "Properties..."

#define TXT_FILE             "dtedit:50" FS "File"
#define TXT_EDIT             "dtedit:51" FS "Edit"
#define TXT_VIEW             "dtedit:52" FS "View"
#define TXT_HELP             "dtedit:53" FS "Help"

#define TXT_APPLY            "dtedit:60" FS "Apply"
#define TXT_SET_DEFAULT      "dtedit:61" FS "Set"
#define TXT_RESET            "dtedit:62" FS "Reset"
#define TXT_RESET_TO_FACTORY "dtedit:63" FS "Reset to Factory"
#define TXT_CANCEL           "dtedit:64" FS "Cancel"
#define TXT_HELP_DDD         "dtedit:65" FS "Help..."

#define TXT_WORDS            "dtedit:70" FS "Words"
#define TXT_CHARS            "dtedit:71" FS "Characters"
#define TXT_NO_WRAP          "dtedit:72" FS "No Wrapping"
#define TXT_LEFT             "dtedit:80" FS "Left"
#define TXT_RIGHT            "dtedit:81" FS "Right"
#define TXT_NO_NUMBER        "dtedit:82" FS "No Numbering"

#define TXT_WRAPPING         "dtedit:90" FS "Wrap at:"
#define TXT_NUMBERING        "dtedit:91" FS "Number on:"

#define TXT_PROP_TITLE       "dtedit:100" FS "Text Editor: Properties"
#define TXT_OPEN_TITLE       "dtedit:101" FS "Text Editor: Open"
#define TXT_SAVE_TITLE       "dtedit:102" FS "Text Editor: Save"
#define TXT_OVERWRITE_TITLE  "dtedit:103" FS "Text Editor: Overwrite"
#define TXT_EXIT_TITLE       "dtedit:104" FS "Text Editor: Exit"
#define TXT_CLIENT_UNTITLED  "dtedit:105" FS "Text Editor: (Untitled)"
#define TXT_UNTITLED_BUFFER  "dtedit:106" FS "New document."
#define TXT_FILE_OPENED      "dtedit:107" FS "Document opened."

#define TXT_BROWSE           "dtedit:110" FS "Browse..."
#define TXT_DISCARD          "dtedit:111" FS "Discard"
#define TXT_FILENAME         "dtedit:112" FS "Filename:"
#define TXT_CONTINUE         "dtedit:113" FS "Continue"
#define TXT_OVERWRITE        "dtedit:114" FS "Overwrite"
#define TXT_HELPDDD          "dtedit:115" FS "Help..."

#define TXT_OPEN_NOTICE      "dtedit:120" FS "The document has been changed.\nDiscard Changes?"
#define TXT_SAVE_NOTICE      "dtedit:121" FS "The document could not be saved."
#define TXT_OVERWRITE_NOTICE "dtedit:122" FS "The file already exists.\nOverwrite it?"
#define TXT_CLEAR_MESSAGE    "dtedit:123" FS "Document cleared."
#define TXT_SAVED_MESSAGE    "dtedit:124" FS "Document saved."
#define TXT_OPENED_MESSAGE   "dtedit:125" FS "Document opened."
#define TXT_OPEN_CANCEL      "dtedit:126" FS "Open cancelled."
#define TXT_NO_SAVE_MESSAGE  "dtedit:127" FS "Document not saved!"
#define TXT_SAVE_CANCEL      "dtedit:128" FS "Save cancelled."
#define TXT_OVER_MESSAGE     "dtedit:129" FS "Document overwritten!"
#define TXT_NO_OVER_MESSAGE  "dtedit:130" FS "Document not overwritten!"
#define TXT_OVER_CANCEL      "dtedit:131" FS "Overwrite cancelled."
#define TXT_NEW_DOC_MESSAGE  "dtedit:132" FS "New document."
#define TXT_EXIT_NOTICE      "dtedit:133" FS "The document has not been saved.\nExit anyway?"
#define TXT_EXIT_CANCEL      "dtedit:134" FS "Exit cancelled."

#define TXT_PROP_POPUP       "dtedit:140" FS "Property window mapped."
#define TXT_PROP_APPLIED     "dtedit:141" FS "Properties applied."
#define TXT_PROP_RESET       "dtedit:142" FS "Properties reset to previous settings."
#define TXT_PROP_FACTORY     "dtedit:143" FS "Properties reset to the factory settings."
#define TXT_PROP_CANCEL      "dtedit:144" FS "Property window cancelled."

#define TXT_CANT_SAVE        "dtedit:150" FS "Document could not be saved."
#define TXT_BROWSE_MESSAGE   "dtedit:151" FS "Browsing..."
#define TXT_NEEDS_OVERWRITE  "dtedit:152" FS "Document exists.  Overwrite?"
#define TXT_OPENED_ANOTHER   "dtedit:153" FS "Another window into document opened."
#define TXT_NEW_VIEW         "dtedit:154" FS "New window into document opened."
#define TXT_DISCARD_CHANGES  "dtedit:155" FS "Document has been changed.  Discard changes?"

#define TXT_CANT_PRINT       "dtedit:160" FS "Document cannot be printed."
#define TXT_NOTHING_TO_PRINT "dtedit:161" FS "Document is empty.  Nothing to print."
#define TXT_DOC_PRINTED      "dtedit:162" FS "Document has been printed."

#define TXT_HELP_POSTED      "dtedit:170" FS "Help posted."
#define TXT_OPEN_HELP_TITLE  "dtedit:171" FS "Opening a Document"
#define TXT_OPEN_HELP_SECT   "50"
#define TXT_SAVE_HELP_TITLE  "dtedit:173" FS "Saving a Document"
#define TXT_SAVE_HELP_SECT   "60"
#define TXT_SNOTE_HELP_TITLE "dtedit:175" FS "Save Errors"
#define TXT_SNOTE_HELP_SECT  "10"
#define TXT_ONOTE_HELP_TITLE "dtedit:177" FS "Overwrite Procedure"
#define TXT_ONOTE_HELP_SECT  "10"
#define TXT_ENOTE_HELP_TITLE "dtedit:179" FS "Exiting the Text Editor"
#define TXT_ENOTE_HELP_SECT  "90"

#define TXT_APP_HELP         "dtedit:181" FS "Text Editor..."
#define TXT_TOC_HELP         "dtedit:182" FS "Table of Contents..."
#define TXT_HELPDESK         "dtedit:183" FS "Help Desk..."
#define TXT_TOC_HELP_TITLE   "dtedit:184" FS "Table of Contents"
#define TXT_HELPDESK_TITLE   "dtedit:185" FS "Help Desk"
#define TXT_HELPDESK_SECT    "HelpDesk"
#define TXT_MAIN_HELP_TITLE  "dtedit:187" FS "Application"
#define TXT_MAIN_HELP_SECT   "10"
#define TXT_PROP_HELP_TITLE  "dtedit:189" FS "Properties"
#define TXT_PROP_HELP_SECT   "210"

#define TXT_UNDO_PERFORMED   "dtedit:201" FS "Undo performed"
#define TXT_CUT_PERFORMED    "dtedit:202" FS "Selected text cut to clipboard"
#define TXT_COPY_PERFORMED   "dtedit:203" FS "Selected text copied to clipboard"
#define TXT_PASTE_PERFORMED  "dtedit:204" FS "Clipboard pasted into document"
#define TXT_CLEAR_PERFORMED  "dtedit:205" FS "Selected text deleted"
#define TXT_SELECT_PERFORMED "dtedit:206" FS "Entire document selected"
#define TXT_UNSELECT_PERFORMED "dtedit:207" FS "Selection cleared"

#define TXT_FIND             "dtedit:210" FS "Find..."

#define TXT_FIND_CANCEL      "dtedit:211" FS "Cancel find"
#define TXT_FIND_APPLIED     "dtedit:212" FS "Find performed"
#define TXT_FIND_POPUP       "dtedit:213" FS "Find popup posted"
#define TXT_FIND_PREV        "dtedit:214" FS "Previous"
#define TXT_FIND_NEXT        "dtedit:215" FS "Next"
#define TXT_FIND_PROMPT      "dtedit:216" FS "Text:"
#define TXT_NULL_STRING      ""
#define TXT_FIND_TITLE       "dtedit:217" FS "Text Editor: Find"
#define TXT_FIND_HELP_TITLE  "dtedit:218" FS "Find Help"
#define TXT_FIND_HELP_SECT   "220"
#define TXT_FIND_FAILED      "dtedit:220" FS "Not found"
#define TXT_FIND_WRAPPED     "dtedit:221" FS "Wrapped"
#define TXT_FIND_FOUND       "dtedit:222" FS "Found"
#define TXT_FIND_NOTHING     "dtedit:223" FS "Nothing to find"
#define TXT_FIXED_FONT       "dtedit:224" FS "-*-lucidatypewriter-medium-r-*-*-*-*-*-*-*-*-iso8859-1"

#define MNE_APPLY            "dtedit:300" FS "A"
#define MNE_APP_HELP         "dtedit:301" FS "H"
#define MNE_CANCEL           "dtedit:302" FS "C"
#define MNE_CHARS            "dtedit:303" FS "C"
#define MNE_CONTINUE         "dtedit:304" FS "C"
#define MNE_COPY             "dtedit:305" FS "C"
#define MNE_CUT              "dtedit:306" FS "T"
#define MNE_DELETE           "dtedit:307" FS "D"
#define MNE_EDIT             "dtedit:308" FS "E"
#define MNE_EXIT             "dtedit:309" FS "E"
#define MNE_FILE             "dtedit:310" FS "F"
#define MNE_FIND             "dtedit:311" FS "F"
#define MNE_FIND_NEXT        "dtedit:312" FS "N"
#define MNE_FIND_PREV        "dtedit:313" FS "P"
#define MNE_HELP             "dtedit:314" FS "H"
#define MNE_HELPDDD          "dtedit:315" FS "H"
#define MNE_HELPDESK         "dtedit:316" FS "D"
#define MNE_HELP_DDD         "dtedit:317" FS "H"
#define MNE_LEFT             "dtedit:318" FS "L"
#define MNE_NEW              "dtedit:319" FS "N"
#define MNE_NO_NUMBER        "dtedit:320" FS "N"
#define MNE_NO_WRAP          "dtedit:321" FS "N"
#define MNE_OPEN             "dtedit:322" FS "O"
#define MNE_OVERWRITE        "dtedit:323" FS "O"
#define MNE_PASTE            "dtedit:324" FS "P"
#define MNE_PRINT            "dtedit:325" FS "P"
#define MNE_PROPERTIES       "dtedit:326" FS "P"
#define MNE_RESET            "dtedit:327" FS "R"
#define MNE_RESET_TO_FACTORY "dtedit:328" FS "F"
#define MNE_RIGHT            "dtedit:329" FS "R"
#define MNE_SAVE             "dtedit:330" FS "S"
#define MNE_SAVE_AS          "dtedit:331" FS "A"
#define MNE_SELECTALL        "dtedit:332" FS "S"
#define MNE_SET_DEFAULT      "dtedit:333" FS "S"
#define MNE_SPLIT            "dtedit:334" FS "S"
#define MNE_TOC_HELP         "dtedit:335" FS "T"
#define MNE_UNDO             "dtedit:336" FS "U"
#define MNE_UNSELECTALL      "dtedit:337" FS "E"
#define MNE_VIEW             "dtedit:338" FS "V"
#define MNE_WORDS            "dtedit:339" FS "W"

#define TXT_DEFAULT_STATUS   "dtedit:350" FS " (Unreadable)"
#define TXT_READONLY_STATUS  "dtedit:351" FS " (Read-only)"
#define TXT_READWRITE_STATUS "dtedit:352" FS " "
#define TXT_NEWFILE_STATUS   "dtedit:353" FS " (New File)"

typedef struct _EditorSettings
   {
   Setting     wrap;
   Setting     numb;
   } EditorSettings;

typedef struct _EditWindow
   {
   unsigned int         status;
   struct _EditWindow * next;
   char *               filename;
   struct stat          stat_buffer;
   TextBuffer *         textBuffer;
   BaseWindowGizmo *    baseWindow;
   Widget               text;
   Gizmo                openPrompt;
   Gizmo                savePrompt;
   Gizmo                overwriteNotice;
   Gizmo                saveNotice;
   Gizmo                exitNotice;
   Gizmo                propertiesPrompt;
#ifdef TXT_FIND
   Gizmo                findPrompt;
#endif
   int                  last_message;
   } EditWindow;

typedef enum
   {
   FileNew, FileOpen, FileSave, FileSaveAs, FilePrint, FileExit
   } FileMenuItemIndex;

typedef enum
   {
   EditUndo, EditCut, EditCopy, EditPaste, EditClear, EditSelectAll, EditUnselectAll
   } EditMenuItemIndex;

typedef enum
   {
   ViewSplit, ViewPreferences
   } ViewMenuItemIndex;

typedef enum
   {
   HelpApp, HelpTOC, HelpHelpDesk
   } HelpMenuItemIndex;

extern void           SetMessage(EditWindow * ew, char * message, int error);
extern void           ChangeEditWindowName(EditWindow * ew, char * filename);
extern void           CreateEditWindow(Widget root, char * filename, TextBuffer * textBuffer);
extern void           DestroyEditWindow(EditWindow * ew);
extern EditWindow *   FindToplevel(Widget shell);
extern EditWindow *   FindEditWindow(Widget w);

extern EditWindow *   filelist;
extern Widget         root;
extern EditorSettings EditorSetting;

extern HelpInfo       PropWinHelp;
extern HelpInfo       OpenWinHelp;
extern HelpInfo       SaveWinHelp;
extern HelpInfo       SNoteHelp;
extern HelpInfo       ONoteHelp;
extern HelpInfo       ENoteHelp;

extern HelpInfo       ApplicationHelp;
extern HelpInfo       TOCHelp;
extern HelpInfo       HelpDeskHelp;

#endif
