/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:Parse.c	1.9"
#endif

#include <X11/IntrinsicP.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <ctype.h>
#include <X11/CoreP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

#include <Xol/OpenLookP.h>
#include <Xol/DynamicP.h>
#include <Xol/Olg.h>
#include <Xol/FButtons.h>
#include <Xol/MenuShell.h>
#include <Xol/WSMcomm.h>

#include <wm.h>
#include <Xol/VendorI.h>
#include <WMStepP.h>
#include <Extern.h>
#include <unistd.h>
#include <limits.h>


#ifdef   I18N
#include <stdlib.h>
#endif

#define MAXMWMLINES	100

#define GROUPTYPE_DEFAULT	0

/*
 *************************************************************************
 *
 * Forward Procedure declarations
 *
 **************************forward*declarations***************************
 */
extern void	AdvanceToken OL_ARGS((String *));
static WMMenuButtonData *BuildMenuButtonDataList OL_ARGS((WMStepWidget, 
		String, char **, int ));
static Boolean	ConsumeAccel OL_ARGS((String *, WMMenuButtonData *));
extern void	ConsumeAlf OL_ARGS((String *));
static void	ConsumeKDetail OL_ARGS((String *));
static Boolean  ConsumeKeySpec OL_ARGS((String *));
static Boolean	ConsumeLabel OL_ARGS((WMMenuButtonData *, String ));
static void	ConsumeMnemonic OL_ARGS((String *, WMMenuButtonData *));
static Boolean	DecodeGroupArg OL_ARGS((String *, unsigned long *));
extern void	FreeMBD OL_ARGS((WMMenuButtonData *));
static Boolean	GetOptionalArg OL_ARGS((String *, WMMenuButtonData *, int));
static void	MapAccSpec OL_ARGS((String , String ,
					String ));
extern String	NextToken OL_ARGS((String *));
static void	ReadResourceFile OL_ARGS((String));
extern void	StringToLCString OL_ARGS((String));

#define OLWM_BEEP	0
#define OLWM_CIRCLEDOWN	1	/* Has up to 3 args [window|icon|transient */
#define OLWM_CIRCLEUP	2	/* Has up to 3 args [window|icon|transient */
#define OLWM_EXEC	3	/* Has a STRING argument */
#define OLWM_FOCUSCOLOR	4
#define OLWM_FOCUSKEY	5
#define OLWM_KILL	6
#define	OLWM_LOWER	7	/* Optional 1 argument */
#define	OLWM_MAXIMIZE	8
#define	OLWM_MENU	9	/* Has a STRING argument */
#define	OLWM_MINIMIZE 	10
#define	OLWM_MOVE	11
#define	OLWM_NEXT_CMAP	12
#define	OLWM_NEXT_KEY	13	/* Has up to 3 args [window|icon|transient */
#define	OLWM_NOP 	14
#define	OLWM_NORMALIZE	15
#define	OLWM_NORMALIZE_RAISE	16
#define	OLWM_PACK_ICONS	17
#define	OLWM_PASS_KEY	18
#define	OLWM_POST_WMENU	19
#define	OLWM_PREV_CMAP	20
#define	OLWM_PREV_KEY	21	/* Has up to 3 args [window|icon|transient */
#define	OLWM_QUIT_MWM	22
#define	OLWM_RAISE	23	/* Optional 1 argument */
#define	OLWM_RAISE_LOWER	24 	/* Optional 1 arg, I think-doc. doesn't
					say so */
#define	OLWM_REFRESH	25
#define	OLWM_REFRESH_WIN	26
#define	OLWM_RESIZE	27
#define	OLWM_RESTART	28
#define	OLWM_SEND_MSG	29	/* MANDATORY 1 ARGUMENT, an integer */
#define	OLWM_SEPARATOR	30
#define	OLWM_SET_BEHAVIOR	31
#define	OLWM_TITLE	32

typedef struct {
	char	*fstr;
	void	(*cb)();
} FunctionData;

/* table containing function names (from Motif mwm documentation) and
 * an associated callback (cb).
 */
FunctionData menuFunctionData[] = {
{	"f.beep",	/* No args */
	OlwmBeep},

	/* circle_down - optional arg = [icon|window] - move window or
	 * icon bottom of stack;
 	 *	icon: applies only to icons.
 	 *	window: applies only to windows.
	 */
{	"f.circle_down",
	OlwmCircle_Down},

	/* circle_up: same optional arg as circle_down, but move window/icon
	 * to top of stack.
	 */
{	"f.circle_up",
	OlwmCircle_Up},

	/* Execute shell command.  Can also specify '!' on line.
	 * Check $MWMSHELL, the $SHELL, for shell to use; else
	 * default to /bin/sh.
	 */
{	"f.exec",
	OlwmExec},

	/* Install colormap focus to window.  No-op if colormapFocusPolicy
	 * isn't explicit.
	 */
{	"f.focus_color",	
	OlwmFocus_Color},

	/* f.focus_key - set keyboard focus to client window/icon.  No-op
	 * if keyboardFocusPolicy isn't explicit (that's pointerFocus to me).
	 */
{	"f.focus_key",
	OlwmFocus_Key},

	/* f.kill - terminate client, send protocol messages if asked for.
	 * Like firing the Quit button.
	 */
{	"f.kill",
	OlwmKill},

	/* f.lower.  Optional arg [-client] (including the -).  Move win to
	 * botton of stack; arg = client name or class - applies to all
	 * clients on screen with this criteria, a la motif.
	 */
{	"f.lower",
	OlwmLower},

	/* f.maximize.  Full size */
{	"f.maximize",
	OlwmMaximize},
/* Unused for our version, really for a named menu in a resource file.
 */
	/* f.menu menu_name. Post menu with menu_name.
	 */
{	"f.menu",		
	OlwmMenu},

/* Iconify. Note in motif: minimized windows are placed on bottom of
 * window stack...
 */
{	"f.minimize",
	OlwmMinimize}, /* may need an argument */

	/* f.move - move operation */
{	"f.move",
	OlwmMove},

/* For window with input focus, install next colormap (WM_COLORMAP_WINDOWS)*/
	/* f.next_cmap.  For window with colormap focus, install next cmap
	 * in the list.
	 */
{	"f.next_cmap",	
	OlwmNext_Cmap},

	/* f.next_key [icon|window|transient]. Move focus to next window/icon
	 * in set.  No-op if keyboardFocusPolicy (pointerFocus) != explicit.
	 * icon: applies to icons only.
 	 * window: applies to windows only.
 	 * transient: transient windows traversed.
	 */
{	"f.next_key",	
	OlwmNext_Key},

{	"f.nop",
	OlwmNop},

	/* f.normalize.  Restore client to normal state (from  iconic or full).
	 */
{	"f.normalize",
	OlwmNormalize},

	/* f.normalize_and_raise.  Restore client to normal size (and it's
	 * transients) and raise in stack.
	 */
{	"f.normalize_and_raise",
	OlwmNormalize_And_Raise},

	/* f.pack_icons.   Pack them in. */
{	"f.pack_icons",	
	OlwmPack_Icons},

	/* f.pass_keys.  Toggle enabling of key bindings for olwm function.
	 *	Disabled: pass key events on to window that has focus.
	 *	Enable: normal processing of window menu ops.
	 */
{	"f.pass_keys",	
	OlwmPass_Keys},

	/* f.post_wmenu.  Post the window menu */
{	"f.post_wmenu",	
	OlwmPost_Wmenu},

	/* f.prev_cmap.  Install previous colormap (from WM_COLORMAP_WINDOWS)
	 * for window with colormap focus.
 	 */
{	"f.prev_cmap",	
	OlwmPrev_Cmap},

	/* f.prev_key. Possible args: [icon|window|transient].
	 *	No args: set focus to previous window/icon.
	 *	icon: icons only.
	 *	window: applies to windows.
	 *	transient: transient windows.
	 *	No-op if keybdFocusPolicy (pointerFocus) != explicit.
	 */
{	"f.prev_key",	
	OlwmPrev_Key},

	/* f.quit_mwm.  Blow out. */
{	"f.quit_mwm",	
	OlwmQuit_Mwm},

	/* f.raise. Optional arg [-client].  Raise to top of stack. */
{	"f.raise",
	OlwmRaise},

	/* f.raise_lower.  Raise window to top of stack IF its partially
	 * obscured by another window; else lower it.
	 */
{	"f.raise_lower",
	OlwmRaise_Lower},

	/* f.refresh - redraw all windows */
{	"f.refresh",	
	OlwmRefresh},

	/* f.refresh_win - redraw 1 window */
{	"f.refresh_win",
	OlwmRefresh_Win},

	/* f.resize - resize op. */
{	"f.resize",
	OlwmResize},

	/* f.restart: restart me */
{	"f.restart",
	OlwmRestart},

	/* f.send_msg.  "msg_num" mandatory integer arg.
	 * Probably the most interesting function. Send client a  client
	 * message of type _MOTIF_WM_MESSAGES with message_type = the msg #.
	 * However, the msg is sent to the client only if the message number
	 * is included in client MWM_MESSAGES property.  If the message
	 * isn't included in the property, then the menu label on the menu
	 * is grayed out, and the message isn't sent.
	 */
{	"f.send_msg",
	OlwmSend_Msg},
	/* f.separator.  Put separator in spot where it's named. Ignore label.
	 * A no-op for us (for now)
	 */
{	"f.separator",	
	OlwmSeparator},

	/* f.set_behavior.  Restart olwm with default behavior (IF a custom
	 * behavior is configured) or revert to custom behavior.  (Bound
	 * to Shift Ctrl Meta <Key>!.
	 */
{	"f.set_behavior",	
	OlwmSet_Behavior},

	/* f.title.  Insert title in menu pane where located.  A no-op
	 * for use (for now).
	 */
{	"f.title",	
	OlwmTitle},
};

/* the size of my function array is NUM_FUNCTIONS - if the array
 * size changes, then this number must also.
 */
#define NUM_FUNCTIONS	33

Global_Menu_Info *global_menu_info = (Global_Menu_Info *)NULL;

static Global_Menu_Info	*NextGMISlot();

/*
 *************************************************************************
 * ParseResourceFile
 *
 ****************************procedure*header*****************************
 */
void
ParseResourceFile()
{
	int	file_found = 0;
	char	name[PATH_MAX],
		*namep = (char *)&(name[0]),
		result[PATH_MAX],
		*resultp,
		*tempname,
		*path,
		*xwinhome,
		*home,
		*lang;
	int	step;
	int	homelen;
	int	confdef = -1;
	/* usehomedir tells me that the configFile begins with a ~/ */
	int	usehomedir = 0;
	/* usefullpath tells me to use the configFile as is, because it
	 * begins with a '/'
	 */
	int	usefullpath = 0;

	/* Find the resource file */
	if (motwmrcs->configFile && *(motwmrcs->configFile) == '/') {
		/* Full Path given */
		if (access((const char *)motwmrcs->configFile, R_OK) == 0) {
			file_found++;
			strcpy(result, motwmrcs->configFile);
		}
		else
		   fprintf(stderr,"Olwm: Can't open configFile resource\n");
	}
	home = getenv("HOME");
	homelen = strlen(home);
	lang = getenv("LANG");
	name[0] = '\0';
	if (!file_found && !(strncmp(motwmrcs->configFile,"~/", 2)) &&
			(int)strlen(motwmrcs->configFile) > (int)2 ) {
		/* File not found, but name given starts with home
		 * directory.
		 */
		strcpy(namep, home);
		if (lang) {
			strcat(name,"/");
			strcat(name, lang);
			/* start with [1] - the slash */
			strcat(name, (const char *)&(motwmrcs->configFile[1]));
			if (access((const char *)name, R_OK) == 0) {
				file_found++;
				strcpy(result, name);
			}
		}
		if (!file_found) {
			name[0] = '\0';
			strcpy(name, home);
			strcat(name, (const char *)&(motwmrcs->configFile[1]));
			if (access((const char *)name, R_OK) == 0) {
				file_found++;
				strcpy(result, name);
			}
		}
	}
	if (!file_found) {
		if ((int)strlen(motwmrcs->configFile) > (int)0) {
			/* Try current working directory */
			getcwd(name, PATH_MAX);
			strcat(name,"/");
			strcat(name, motwmrcs->configFile);
			if (access((const char *)name, R_OK) == 0) {
				file_found++;
				strcpy(result, name);
			}
		}
		if (!file_found) {
			/* Give up, look for .mwmrc */
			xwinhome = getenv("XWINHOME");
			lang = getenv("LANG");
			strcpy(name, home);
			strcat(name,"/");
			if (lang) {
				strcat(name, lang);
				strcat(name,"/");
				strcat(name,".mwmrc");
				if (access((const char *)name, R_OK) == 0) {
					file_found++;
					strcpy(result, name);
				}
			}
			if (!file_found) {
				strcpy((char *)&(name[homelen+1]),".mwmrc");
				if (access((const char *)name, R_OK) == 0) {
					file_found++;
					strcpy(result, name);
				}
				else {
					char *p;
					strcpy(name, xwinhome);
					strcat(name,"/lib");
					p = &(name[strlen(name)]);
					if (lang) {
						strcat(name, lang);
						strcat(name, "/");
						strcat(name, "system.mwmrc");
				if (access((const char *)name, R_OK) == 0) {
					file_found++;
					strcpy(result, name);
				}
					} /* if lang */
					if (!file_found) {
						strcat(p, "system.mwmrc");
				if (access((const char *)name, R_OK) == 0) {
						file_found++;
						strcpy(result, name);
				}
					} /* if !file_found */
				} /* else */
			} /* if !file_found */	
		} /* if !file_found */
	} /* if !file_found */

		if (file_found) {
			ReadResourceFile(result);
		}
		else
			fprintf(stderr,"Can't find file system.mwmrc\n");
} /* ParseResourceFile */

/*
 *************************************************************************
 * ReadResourceFile
 * Some clues:
 *	fgets reads a line at a time AS IS from the file - if a line
 *	ends in \n, then that's the way it is read; if it ends in a
 *	\, then the returned line will have its last character being a \.
 *	My proposal is simple: take the returned lines, fill up an array
 *	similar to lineArray in the main function in this file, and
 *	pass the result to that function.  Plain and simple: in some
 *	cases, the function will have this argument, in which case we
 *	know that the data has been read from a resource file;
 *	in other cases, the argument is null, and we are dealing with
 *	an MWM_MENU property;  better yet: forget the argument - add
 *	a Boolean argument that says to read the stuff from the resource
 *	file, call the function from ParseResourceFile, etc. - this would
 *	involve the full parsing, but that can be done in a separate
 *	function, and the lineArray array can be passed as an argument.
 *	A lot of other code will have to change to accommodate the Boolean
 *	variable, but nothing serious in my opinion.
 *	In fact, the argument can be the filename - if the filename is
 *	non-NULL, then we are dealing with a resource file!!
 *
 *	We will blow out the way the mwm parser does if we get a bad line
 *	as far as syntax goes - it'll ignore the rest of the file after
 *	that.
 ****************************procedure*header*****************************
 */

static void
ReadResourceFile OLARGLIST((filename))
OLGRA(String, filename)
{
FILE *fp;
char *str;
int i = 0;
int needleftbrace = 0;
int needrightbrace = 0;
char bufpuf[2048];
char *bufp = (char *)&(bufpuf[0]);
char *lineArray[MAXMWMLINES];
char *p, *cp;
char *token;
int linecnt;
int len;
char menuname[512];
Boolean inmenu = False,
	inbuttons = False,
	inkeys = False;
WMMenuButtonData *mbd;
Global_Menu_Info *gmi;
char	*reverse_ptr;
char	*end_str;

	if ( (fp = fopen(filename, "r") ) == NULL) {
		fprintf(stderr,"Can't open file = %s\n", filename);
		return;
	}
	while ( (str = fgets(bufpuf, 2048, fp)) != NULL) {
		bufp = (char *)&(bufpuf[0]);
		end_str = strstr(bufp,"\n");
		if (end_str)
			*end_str = '\0';
		end_str = strstr(bufp,"\\n");
		if (end_str)
			*end_str = '\0';
		reverse_ptr = bufp;
		/* Strip off \n from end of line or \ from end of line */
		
		/*fprintf(stderr,"This line: %s\n", str);*/
		i++;
		p = &(bufpuf[0]);
		AdvanceToken(&p);
		if (strlen(p) && *p == '#')
			continue;
		if (needleftbrace) {
			if (*p != '{') {
			 fprintf(stderr,"missing '{' at line %d\n",i);
			 break;
			}
			needleftbrace = 0;
			needrightbrace++;
			continue;
		} /* if needleftbrace */
				
		if (!inmenu && !inkeys && !inbuttons) {
			token = NextToken(&p);
			if (strcmp(token,"Buttons") == 0) {
			/*	checkbuttons++;*/
				inbuttons = True;
				needleftbrace++;
			}
			else if (strcmp(token, "Menu") == 0) {
				/* checkmenu++  */;
				inmenu = True;
				needleftbrace++;
			}
			else if (strcmp(token,"Keys") == 0) {
				/* checkkeys++; */
				inkeys = True;
				needleftbrace++;
			}
			if (inmenu || inkeys || inbuttons ) {
				token = NextToken(&p);
				if ( token && (len = strlen(token)) > 0) {	
					/* assume token is all right */
					if (inmenu) {
						if (len > 511)
							len = 511;
					strncpy(menuname, token, len+1);

					/* Reset linecnt */
					linecnt = 0;

					}
					needleftbrace++;
					continue;
				} /* len > 0 */
				else
					{
					/* Bad syntax, blow out */
					fprintf(stderr,"No name supplied\n");
					break;
					} /* else */
			} /* if inmenu or keys or buttons */
			/* if you get here, then the line is considered
			 * junk.
			 */
			continue;
		} /* if !inmenu and !inkeys and !inbuttons */
			

		if (needrightbrace) {
			if (inkeys || inbuttons) {
			/* Look for the right brace - don't care about these*/
				if (*p == '}') {
					needrightbrace = 0;
					inkeys = inbuttons = 0;
				}
					continue;
			}
			if (inmenu) {
				/* Look for the right brace */

				if (*p == '}') {
				   needrightbrace = inmenu = 0;
				   /* FINISH up this menu pane...
				    * fill in the necessary information
				    * here
				    */
/*
				for (i=0; i < linecnt; i++)
				  fprintf(stderr,"\t%s\n", lineArray[i]);
 */
		mbd = BuildMenuButtonDataList((WMStepWidget)NULL,
						(String)NULL, lineArray,
						linecnt);
				if (mbd) {
					if ( (gmi = NextGMISlot()) != NULL) {
					   gmi->mbd = mbd;
					   gmi->menuname=strdup(menuname);
					}
				}
				for (i=0; i < linecnt; i++) {
					XtFree(lineArray[i]);
					lineArray[i] = NULL;
				}
				/* Now have this array of ptrs - use them
				 * in the argument	
				 */
				   } /* if *p == rt. brace */
				else {
				   /* No right brace, but we have another menu
				    * entry.  Fill in the necessary info here,
				    * and continue.
				    */
/*fprintf(stderr,"We have another menu item, it is %s\n", p);*/
				   if ( (lineArray[linecnt] = (char *)malloc(
			strlen(p) + 1))  == NULL) {
				fprintf(stderr,"Olwm Error: No space\n");
					break;
				   }
				   strcpy(lineArray[linecnt++], p);
				} /* else */
				   continue;
			} /* if inmenu */
		} /* if needsrightbrace */
	} /* while */
	CreateGlobalMenus();
} /* ReadResourceFile */


static Global_Menu_Info *
NextGMISlot()
{
	Global_Menu_Info *gmi_ptr = global_menu_info;

	if (gmi_ptr == NULL) {
		gmi_ptr = (Global_Menu_Info *)
				XtMalloc(sizeof(Global_Menu_Info));
		global_menu_info = gmi_ptr;
	}
	else {
		while (gmi_ptr->next != NULL)
			gmi_ptr = gmi_ptr->next;	
		gmi_ptr->next = (Global_Menu_Info *)
				XtMalloc(sizeof(Global_Menu_Info));
		gmi_ptr = gmi_ptr->next;
	}
	gmi_ptr->menuname = (String)NULL;
	gmi_ptr->mbd = NULL;
	gmi_ptr->MenuShell =  (Widget)NULL;	/* This popup menu shell */
	gmi_ptr->menu_default = 0;
	gmi_ptr->num_menu_items = (int *)0;
	gmi_ptr->wmap = NULL;
	gmi_ptr->num_mbmap = 0;
	gmi_ptr->mbmap = NULL;
	gmi_ptr->next = (Global_Menu_Info *)NULL;
	return(gmi_ptr);

} /* NextGMISlot */	

/*
 *************************************************************************
 *  GetMWMMenu()
 *
 *  Read and process _MWM_MENU property.
 *  Returns list of WMMenuButtonData structures, or NULL.
 ****************************procedure*header*****************************
 */


WMMenuButtonData *
GetMWMMenu OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
char		*prop_ret= NULL;
Atom		ret_type;
int		ret_format;
unsigned long	ret_leftover;
unsigned long	ret_nitems;
WMMenuButtonData *mbd;
Atom		xa_MWM_MENU;

/* Atom name for _MWM_MENU property */
#define _XA_MOTIF_WM_MENU       "_MOTIF_WM_MENU"
#define _XA_MWM_MENU            _XA_MOTIF_WM_MENU

	xa_MWM_MENU = XInternAtom (XtDisplay(wm), _XA_MWM_MENU, False);

	if ( (XGetWindowProperty (XtDisplay(wm), wm->wmstep.window,
		xa_MWM_MENU, 0L,
		(long)1000000, False, AnyPropertyType,
		&ret_type, &ret_format, &ret_nitems, &ret_leftover,
		 (String *)&prop_ret) != Success) ||
			ret_type == None ||
			ret_format != 8) {
		/* Can't read _MWM_MENU prop, or bad type/format */
		mbd = NULL;
	}
	else {
		/* parse the property string */
		mbd = BuildMenuButtonDataList(wm, (String )prop_ret,
			(char **)NULL, 0);
		XFree ((char *)prop_ret);
	}

	return (mbd);

} /* GetMWMMenu */


/*
 *************************************************************************
 *
 * BuildMenuButtonDataList.
 *   -Parse a specification for a menu item.  It has the following
 * syntax:
 *
 *	label	[mnemonic]	[accelerator]	function_name [args ...]
 *	..	..		..		..
 *	..	..		..		..
 *	label	[mnemonic]	[accelerator]	function_name [args ...]
 *							[}]
 *	  If a curly right bracket is found on the line BY ITSELF
 *	(nothing else is on the line with it) then the rest of the
 *	string is disregarded.  I assume that when reading the
 *	property string, each line in the above syntax is separated
 *	by "\n" - therefore, I regard each line as a TOKEN, with
 *	the token separator being "\n".
 *	   The property string, mwm_menu, is initially parsed with
 *	strtok(3C) to break it into tokens as above.  Then each
 *	token (each LINE in the above syntax) is individually
 *	scrutinized to be sure it adheres to the above syntax.  The
 *	number of lines (tokens) separated by "\n" is limited
 *	to MAXMWMLINES (now 100).
 *	   The individual tokens in each line are parsed by brute force.
 *	Two functions that help parse through each line:
 *		-Let char *ptr point somewhere in the line being
 *	parsed;
 *		AdvanceToken(String *ptr) - Move *ptr to next
 *	token in string pointed to by ptr.
 *		NextToken(String *ptr) - advance *ptr to next
 *	token in string, (DON'T: malloc space equal to length of string, copy
 *	token into malloc'd space and return new copy. )
 *	
 *
 ****************************procedure*header*****************************
 */
static WMMenuButtonData *
BuildMenuButtonDataList OLARGLIST((wm, mwm_menu, args, numargs))
OLARG(WMStepWidget, wm)
OLARG(String , mwm_menu)
OLARG(char **, args)
OLGRA(int, numargs)
{
	String	str;
	String	current_line_ptr;
	WMMenuButtonData	*head = (WMMenuButtonData *)NULL,
				*tail = (WMMenuButtonData *)NULL,
				*mbd;
	int			fndx;
	int			current_line = 0; /* tells me the line in
						   * the mwm_menu string
						   * string that I am currently
						   * parsing. Serves no other
						   * therapeutic purpose.
						   */
	char			*mwm_menu_string = mwm_menu;
	char			*lineArray[MAXMWMLINES];  /* MAX 100 lines per
							   * MWM_MENU
							   */ 
	char			**linePtrs;	/* local char ** ptr */
	int			i = 0;
    
	/*
	 * Scan each line of property - hopefully it will have the form
	 * "label [mnemonic] [accelerator] function"  - until 
	 * "}" or EOF  found.  If } is found on a line by itself,
	 * or it is the first character of a line, then that's it,
	 * ignore remaining lines.  Otherwise  the line separator is '\n'.
	 * If } is found in the middle of a line, then the line is voided.
	 */

	/* Collect lines from the MWM_MENU string (just pointers).
	 * IF wm is non-null, then we are being called from Initialize
	 * with a step widget that has MWM_MENU property on its window;
	 * otherwise, we are being called as a result of parsing a resource
	 * description file.  Adjust variables appropriately.
	 */

	if (wm) {
		for (lineArray[i] = strtok(mwm_menu_string, "\n");
			lineArray[i++] != NULL && i < MAXMWMLINES; 
			lineArray[i] = strtok(NULL,"\n"))
			;
		linePtrs = lineArray;
	}
	else {
		linePtrs = args;
		i = numargs;
	}

	for (; current_line < i && linePtrs[current_line]; current_line++) {
		current_line_ptr = linePtrs[current_line];
		/*    Within line, get next "significant string" - I consider
		 * this a sequence of non-white characters, or a quoted
		 * string (surrounded by DOUBLE quotes).
		 *    If the line starts with an exclamation, go to next line;
		 * if the string currently being looked at has a right
		 * curly bracket, then we're done.
		 */
		if ((*current_line_ptr == '!') || 
			   (str = NextToken(&current_line_ptr)) == NULL) {
			/* the first call to NextToken() MUST return a label */
			continue;
		}

		/* NextToken() didn't malloc a string for us */
		if (*str == '}') {
			/* ignore remainder of string */
			break;
		}

		mbd = (WMMenuButtonData *)XtMalloc (sizeof (WMMenuButtonData));
#ifdef DEBUG
fprintf(stderr,"Malloc (mbd) =%x\n", mbd);
#endif
		mbd->menucb = (Menu_CB)NULL;
 		mbd->accelerator = (char *)NULL;
		mbd->next = (WMMenuButtonData *)NULL;
		mbd->args = (XtPointer)NULL;

		/* Now the brute force parsing: one token at a time.
	 	 * The first token better be a label.  It was read at the
		 * top of the for loop.  If all goes well, place
	 	 * in mbd->label, set (for now) mbd->labelType =
	 	 * PL_TEXT.
	 	 */
		if (!ConsumeLabel(mbd, str)) {
#ifdef DEBUG
		fprintf(stderr,"Free (After ConsumeLabel):mbd=%x\n",mbd);
#endif
			XtFree((char *)mbd); mbd = NULL;
			continue;
		}

		/*
		 * Optional: mnemonic. Must begin with '_'.
		 * Fill in mbd->mnemonic. Does potentially 1 malloc/free.
		 */
#ifdef DEBUG
fprintf(stderr,"Call ConsumeMnemonic, current_line_ptr=%s\n",current_line_ptr);
#endif
		ConsumeMnemonic(&current_line_ptr, mbd);

		/*
		 * Optional: accelerator.
		 *
		 * The format of the Motif accelerator specification is
		 * different than that for Open Look.  That's too bad, because
		 * we have a neat converter that translates our key
		 * specification syntax, which is better in my opinion, to
		 * a key detail and modifier.  No problem: just convert the
		 * string to one that our converter understands,
		 * then pass it along.
		 */

#ifdef DEBUG
		fprintf(stderr,"Call ConsumeAccel, current_line_ptr=%s\n",
				current_line_ptr);
#endif
		if (!ConsumeAccel(&current_line_ptr, mbd)) {
			/* pass on this line - free anything already
			 * malloc'd
			 */
			if (mbd->accelerator) {
#ifdef DEBUG
			fprintf(stderr,"Free: acc=%x\n",mbd->accelerator);
#endif
				XtFree(mbd->accelerator);
				mbd->accelerator = NULL;
			}
#ifdef DEBUG
fprintf(stderr,"Free: mbd=%x\n",mbd);
#endif
			XtFree((char *)mbd->label);
			XtFree((char *)mbd); mbd = NULL;
			continue;
		}

		/*
		 * Mandatory token: function.
		 *    ConsumeFunction() searches my "function table"
		 * for the named function string, returning an index into
		 * the table.  The function names in the table are taken
		 * directly from the OSF/Motif mwm man page.
		 * Bad function names default to f.nop, which is a no-op
		 * operation; therefore all indices returned by ConsumeF()
		 * are valid.
		 */

		fndx = ConsumeFunction(&current_line_ptr, &mbd->menucb);

		/* The function name is supposed to be one token by itself -
		 * some functions allow arguments, so parse these in
		 * DecodeArgument().  Note that mbd->args is an
		 * XtPointer, initialized to NULL.  We pass mbd 
		 * to the function, and let the function malloc any
		 * space needed for args in mbd->args.
	 	 */
		if (!DecodeArgument(fndx, &current_line_ptr, mbd)) {
			FreeMBD(mbd);
			continue;  /* skip this menu item */
		}
		/* Fill in the linked list of MenuButtonData structs */
		if (tail)
			tail->next = mbd;
		else
			head = mbd;
		tail = mbd;
	} /* end the big for */

	/* return the head of the WMMenuButton linked list - to traverse
	 * the list, use the next pointers; no need to know how many
	 * structs are in the list.
	 */
	return(head);

} /* BuildMenuButtonDataList */



/*
 *************************************************************************
 *  NextToken (
 * - return next significant string within the line pointed to by strptr.
 *	Strings are separated by whitespace, but they may have double
 *	quotes and backslashes.  If the FIRST non-white char. is
 *	a double quote, then the string terminates with another double
 *	quote.  Backslashes tell me to quote the next character only.
 *	A pound sign (#) tells me to ignore the rest of the line,
 *	including the '#' (unless, of course, it is quoted).  The pointer
 *	to the string returned must be freed because it is malloc'd space.
 *	The pointer argument (a char **) will be modified to point
 *	beyond the string returned.  This may differ slightly from mwm's
 *	version of a string, at least from a technical point of view-
 *	they may remove trailing whitespace at the end of a quoted
 *	string; but all else remains the same, that's what counts.
 *
 *  My first version malloc'd space for each token, leaving me
 *  responsible for freeing it; forget it.  I'll just make a local
 *  buffer and return pointers to it.  If the string in the buffer is
 *  needed by the calling function, let them make a copy of it.
 *  I will NULL terminate strings.
 ****************************procedure*header*****************************
 */

String
NextToken  OLARGLIST((strptr))
OLGRA(String *, strptr)
{
String current_line_ptr = *strptr; /* COPY of strptr dereferenced ptr */
String ret_p = (String)NULL;
String temp_p;
static char buffer[512]; /* hope that a token is < 512 chars */
int buff_cnt = 0;
	int            chlen;

 	/* Remove leading white space - advances line pointer.  This call
 	 * advances the COPY of the original pointer.
	 */
	AdvanceToken(&current_line_ptr);

	/*
	 * Return NULL if line is empty, whitespace, or begins with a comment.
	 */
 	/*
 	 * Weed out comments (we allow for '#').  If comment, or NULL,
 	 * then reset dereferenced pointer (*strptr) within it's buffer.
 	 */
 	if (current_line_ptr == NULL || *current_line_ptr == NULL  ||
				(chlen = mblen(current_line_ptr, MB_CUR_MAX)) &&
 						*current_line_ptr == '#') {
		*strptr = current_line_ptr;
		return (NULL);
	}

 	/* Just to make life more complicated, do quoted string;
 	 * if it has a double quote, find opposing one.  If no opposing
 	 * double quote, we'll advance to end of string. For a
 	 * back-slash, quote one char.
 	 * For the sake of argument, the '"' is NOT part of the string.
 	 */

	 /* temp_p points to the returnable string, and will be used
	 * to index into the string to copy in parts of the string
	 * that are useful (e.g., a quoted part of a string).
	 */



	temp_p = buffer;

   if (chlen == 1 && *current_line_ptr == '"') {
	for (current_line_ptr++;
	 current_line_ptr && *current_line_ptr && *current_line_ptr != '"'; ) {
		if ( (chlen = mblen(current_line_ptr, MB_CUR_MAX)) > 0) {
			/* if the char. len == 1, then look for
			 * a backslash, and copy in the ensuing
			 * quoted characters, REGARDLESS of the
			 * char. length (1 or more).  If len > 1,
			 * the it can't be a backslash, and just
			 * copy the whole character in.  Why use
			 * a backslash within a quoted string?
			 * Maybe they want to put a double quote
			 * inside the string.  Enjoy yourself.
			 */
			if (chlen == 1) {
				if (*current_line_ptr == '\\') {
					current_line_ptr++; 
					if ( !(chlen = mblen(current_line_ptr,
								 MB_CUR_MAX)) )
						continue;
				}
			}
			/* We can't get here unless chlen > 0 */
			buff_cnt += chlen;
			if (buff_cnt >= 512)
				return (NULL);
			while (chlen--) {
		    		*temp_p++ = *current_line_ptr++;
			}
		} /* if mblen > 0 */
		else	/* shouldn't happen */
			current_line_ptr++;
	} /* big for */
	if (*current_line_ptr == '"')
		/* Advance one char */
		current_line_ptr++;
   } /* if quoted string */

	/* enough with quoted strings - try unquoted ones; these may be
	 * terminated by a whitespace, or a # to indicate the end of the
	 * substance and the beginning of a comment, similar to the
	 * shell.
	 */
   else
	for (; current_line_ptr && *current_line_ptr ; ) {
		if ( (chlen = mblen(current_line_ptr, MB_CUR_MAX)) > 0) {
			if ( (chlen == 1) ) {
				/* We're out if we get a whitespace or a '#' */
				if (isspace(*current_line_ptr) || 
						*current_line_ptr == '#')
					break;
				/* Here we go again: if we get a backslash 
				 * (maybe they want to put a space inside the
				 * string, for example) then consume the next
				 * char.
			 	 */
				if (*current_line_ptr == '\\') {
					current_line_ptr++;
					if ( !(chlen = 
						mblen(current_line_ptr, MB_CUR_MAX)) )
					   continue;
				}
			} /* chlen == 1 */
			/* Again, can't get here unless chlen > 0 */
			if (buff_cnt >= 512)
				return (NULL);
			while (chlen--) {
		    		*temp_p++ = *current_line_ptr++;
			}
		} /* if chlen > 0 */
		else
			current_line_ptr++;
	} /* end big for */

	/* Hopefully, we have retrieved a string of at least one character;
	 * if we haven't the release the string now and free the space
	 * I asked for; otherwise, reset the pointer passed in to point
	 * beyond the string returned.  It is the responsibility of the
	 * caller of the function to free it - and they better.
	 */
	if (temp_p == buffer) {	/* temp pointer didn't move, so we didn't
				 * copy in anything!
				 */
		return(NULL);
	}
	*temp_p = '\0'; /* NULL terminate */
	if (current_line_ptr != NULL && *current_line_ptr) {
		if (*current_line_ptr == '#') 	/* comment */
			*strptr = NULL;
		else
			*strptr = current_line_ptr;
	}
	else
		*strptr = NULL;
    return((String)buffer);
} /* NextToken */

/*
 *************************************************************************
 *
 *  ConsumeKDetail()
 *   -Called from ConsumeKeySpec() to get Keysym (detail) specified
 *	in string.  Only arg. is a pointer to the line buffer of
 *	the line being parsed; if all goes well, the pointer is
 *	moved ahead in the buffer beyond the detail.
 *
 ****************************procedure*header*****************************
 */
static void 
ConsumeKDetail OLARGLIST((String *strptr))
OLGRA(String *, strptr)
{
	String tmp_ptr = *strptr;
	int            len;
#ifdef I18N
	int            chlen;
#endif

	AdvanceToken(&tmp_ptr);

	/* Advance tmp_ptr beyond the key detail */
#ifdef I18N
	while(*tmp_ptr && ((chlen = mblen(tmp_ptr, MB_CUR_MAX)) > 0) && ((chlen > 1) ||
		     (!isspace (*tmp_ptr) && *tmp_ptr != ',' &&
						 *tmp_ptr != ':')))
		tmp_ptr += chlen;
#else
	while(*tmp_ptr && !isspace(*tmp_ptr) && *tmp_ptr != ',' &&
						 *tmp_ptr != ':' )
		tmp_ptr++;
#endif

	*strptr = tmp_ptr;

} /* ConsumeKDetail */

/*
 *************************************************************************
 *
 *  AdvanceToken()
 *
 * - Given ptr to a string (char **strptr), advance ptr to next non-white
 * char.  The relevant pointer (*strptr) gets advanced if necessary.
 *
 *
 ****************************procedure*header*****************************
 */

void
AdvanceToken OLARGLIST((strptr))
OLGRA(String *, strptr)
{
#if defined(I18N)
	for (; *strptr && mblen(*strptr, MB_CUR_MAX) == 1 && isspace(**strptr);
						 (*strptr)++)
		;
#else
	for (; *strptr && isspace(**strptr); (*strptr)++)
		;
#endif

} /* AdvanceToken */


/*
 *************************************************************************
 *
 *  ConsumeLabel.
 *	- Pass String, MenuButtonData struct as args;
 *	Save the string in the mbd struct.  If time, parse first
 * character in string for the @ sign; if present, then they want a
 * bitmap for the label...
 *	
 ****************************procedure*header*****************************
 */

static Boolean
ConsumeLabel OLARGLIST((mbd, string))
OLARG(WMMenuButtonData *, mbd)
OLGRA(String, string)
{

	mbd->label = (String)XtMalloc((unsigned int)(strlen(string) + 1));
#ifdef DEBUG
fprintf(stderr,"Malloc in ConsumeLabel: label=%x string=%s\n", mbd->label,
					string);
#endif

	strcpy(mbd->label, string);
#ifdef DEBUG
fprintf(stderr,"In ConsumeLabel, copied into label=%s\n",mbd->label);
#endif

/* define our own, PL_TEXT, to be used with OlgDrawTextLabel;
 * for later use, we can try to implememt the labels with
 * OlgDrawPixmapLabel(), and set label to PL_IMAGE or
 * PL_BITMAP or PL_PIXMAP.  Skip it for now though.
 * - NOW, we don't use PL_TEXT at all, because we assume it's a text label;
 *   but later we may use a bitmap label instead, so leave it in as a hook.
 */
#define PL_TEXT 68868

	mbd->labelType = PL_TEXT;

#ifdef USE_SPECIAL
	if (*string == '@') {
		/*
		 * Here:  string  = "@<bitmap file>"
		 * Read the label bitmap file, or cache them for later use.
		 * bitmap file.
		 */
		string++;  /* skip "@" */
		if (FindBitmap(((char *)string))) {
			mbd->labelType = PL_PIXMAP;
				...
				...
		}
	}
#endif

	return(True);
} /* ConsumeLabel */

/*
 *************************************************************************
 *
 *  ConsumeMnemonic (strptr, mbd)
 *	- parse mnemonic for menu button.  May not be present (optional)-
 *	if it is, it must begin with '_'.  I allow it to be specified in
 *	double quotes.
 *
 ****************************procedure*header*****************************
 */

static void
ConsumeMnemonic OLARGLIST((strptr, mbd))
OLARG(String *, strptr)
OLGRA(WMMenuButtonData *, mbd)
{
	String tmp_ptr = *strptr;
	String str = (String)NULL;

	AdvanceToken(&tmp_ptr);
	mbd->mnemonic = NULL;

	/* The mnemonic must start with "_", but I'll allow some to
	 * specify a mnemonic as "_X", surrounded by doubles
	 */
	if ( (*tmp_ptr == '"' && *(tmp_ptr+1) == '_') || *tmp_ptr == '_') {
		if (str = NextToken(&tmp_ptr)) {
			if (*str++ != '_' || !str || !(*str))
			/*** - Motif enforces that the mnemonic must be
		 	 * in the label, and the label must be a text label.
		 	 * I'll let it ride for now, because my toolkit
			 * can handle it.
		 	 */
			OlVaDisplayWarningMsg(XtDisplay(Frame),
				OleNmnemonic, OleTbadSpec, OleCOlClientOlwmMsgs,
				OleMmnemonic_badSpec, NULL);
			/* That's a bad mnemonic specification */
			else
				mbd->mnemonic = *str;
		} /* str != NULL */
	} /* if ( (X && Y) || Z ) */
	/* Adjust pointer */	
	*strptr = tmp_ptr;

} /* ConsumeMnemonic */

/*
 *************************************************************************
 *
 *  ConsumeAccel (strptr, mbd)
 * - read (scan) the string pointed to by strptr (pointer to the string
 *	being parsed), convert the accelerator to a form that can be
 *	understood by our Open Look converter.  We will fill in
 *	WMMenuButtonData->accelerator, and advance the pointer to the string
 *	pointed to by strptr to beyond the accelerator.
 *
 ****************************procedure*header*****************************
 */

static Boolean
ConsumeAccel OLARGLIST((strptr, mbd))
OLARG(String *, strptr)
OLGRA(WMMenuButtonData *, mbd)
{
String		tmp_p;
char		*str;
int		success;
XrmValue _from, _to;

#ifdef DEBUG
	fprintf(stderr,"ConsumeAccel, *strptr=%s\n", *strptr);
#endif
	AdvanceToken(strptr);
/*
	if( (tmp_p = *strptr) && (*tmp_p == '!') )
		return(False);
*/
	tmp_p = *strptr;
	/* Return True if "f.XXX....", because it is probably a
	 * function name; if we return False, then it means the
	 * line is no good, but in this case it may be.
	 * Same thing also goes for "! ...".
	 */
	if (strncmp(tmp_p, "f.", 2) == 0 || *tmp_p == '!')
		return(True);
		
	success = (int)True;

	/* ConsumeKeySpec: move tmp_p past the modifiers, event
	 * name (only <Key> allowed) and the key detail named.
	 */
#ifdef DEBUG
fprintf(stderr,"Call ConsumeKeySpec(), tmp_p=%s\n", tmp_p);
#endif
	if (ConsumeKeySpec(&tmp_p)) {
		str = XtMalloc ((unsigned int) (tmp_p - *strptr + 1));
			/* Map the acc. spec to one that my Open Look
			 * converter understands, and pass it along.
			 * We just malloc's "str", also.
			 */
#ifdef DEBUG
fprintf(stderr,"Malloc of keyspec = %x\n", str);
#endif
			MapAccSpec(*strptr, tmp_p, (String)str);

			/* Rememeber to free this string when you free the
			 * mbd structure (mbd->accelerator).
			 */
			mbd->accelerator = str;
			_from.addr = (XtPointer)mbd->accelerator;
			_from.size = strlen(mbd->accelerator);
			_to.addr   = 0;
			if (! (success = XtCallConverter( XtDisplay(Frame),
						_OlStringToOlKeyDef,
						(ArgList)0,
						(Cardinal)0,
						&_from, &_to,
						(XtCacheRef)0)) ) {
				OlVaDisplayWarningMsg(XtDisplay(Frame),
					OleNaccelerator, OleTbadSpec,
					OleCOlClientOlwmMsgs,
					OleMaccelerator_badSpec, NULL);
				if (mbd->accelerator) {
#ifdef DEBUG
fprintf(stderr,"Free(acc) = %x\n", mbd->accelerator);
#endif
					XtFree(mbd->accelerator);
					mbd->accelerator = NULL;
				}
				return(False);
			} /* Converter failed */
			/* Converter passed, accelerator OK */
			mbd->kd = *((OlKeyDef *)(_to.addr));
	} /* if ConsumeKeySpec */
	else {
		OlVaDisplayWarningMsg(XtDisplay(Frame),
			OleNaccelerator, OleTbadSpec,
			OleCOlClientOlwmMsgs,
			OleMaccelerator_badSpec, NULL);
		success = (int)False;
	}

	*strptr = tmp_p;  /* advance pointer BEYOND the specification */
	return((int)success);

} /* ConsumeAccel */


/*
 *************************************************************************
 *
 *  MapAccSpec.
 *	Translate the motif accelerator specification string to a form
 *	understood by Open Look.  Just remove the "<Key>" string and
 *	surround the detail with the less than and greater than sign -
 *		Alt<Key> F12 becomes Alt<F12>
 *	Copy the string starting at str and ending at end_ptr into
 *	a buffer that starts at res_ptr.
 *
 * 
 ****************************procedure*header*****************************
 */

static void
MapAccSpec OLARGLIST((str, end_ptr, res_ptr))
OLARG(String, str)
OLARG(String, end_ptr)
OLGRA(String, res_ptr)
{
#ifdef I18N
	int   chlen;
#endif

	AdvanceToken (&str);

	while (*str != '<') {
		/* One problem: my converter may not understand the ~ -
		 * I have to put it in anyway.
		 */
		if (*str == '~') 
			*res_ptr++ = *str++;

#ifdef I18N
		while (((chlen = mblen(str, MB_CUR_MAX)) > 1) || isalnum (*str)) {
			while (chlen--)
				*res_ptr++ = *str++;
		}
#else
		while (isalnum (*str))
			*res_ptr++ = *str++;
#endif
		/* Put a space after what was hopefully a modifier */
		*res_ptr++ = ' ';
		AdvanceToken(&str);
	}

	str++;	/* move ptr past less than < */
	*res_ptr++ = '<';	/* Surround key detail; for example, <F12> */
	while(*str != '>') {
#ifdef I18N
		str += mblen(str, MB_CUR_MAX);
#else
		str++;
#endif
	}
	str++;  /* Move ptr. past > sign > */

	AdvanceToken (&str);
	while(str != end_ptr)
        	*res_ptr++ = *str++;
	*res_ptr++ = '>';	/* At end of key detail */
	*res_ptr = NULL;

} /* MapAccSpec */

/*
 *************************************************************************
 *
 *  int
 *  ConsumeFunction (strptr, cb)
 *	Called from BuildMenuButtonDataList.  (mlp - we are only concerned
 *	with menu function names, I suppose; but what about button or key
 *	names - how can I use or avoid them??  Based on the former 
 *	"res_spec" argument, I believe that the following don't apply:
 *	f.post_wmenu - a button or key spec;
 *	f.separator - a menu spec;
 *	f.title - a menu spec.
 * 		That's it - the rest are generally ANY.
 *  For bad function names, assume f.nop.
 *
 ****************************procedure*header*****************************
 */
int
ConsumeFunction OLARGLIST((strptr, cb))
OLARG(String *, strptr)
OLGRA(Menu_CB *, cb)
{
String		tmp_pt = *strptr;
String		funcname;
int		min,
		mid,
		max,
		found;

	AdvanceToken (&tmp_pt);

	/* Return if function name starts with exclamation, because
	 * then just exec the rest of the line.
	 */
	if (*tmp_pt == '!') {
		*strptr = ++tmp_pt;
		*cb = OlwmExec;
		return(OLWM_EXEC);
	}

	/*
	 * Identify the function corresponding to the specified name.
	 * Try binary search of the window manager function parse table.
	 * Assume f.nop if the function and resource type cannot be matched.
	 * This handles NULL and comment strings, bad function names, 
	 * and functions  in inappropriate resource sets.
	 */
	if ( (funcname = NextToken(&tmp_pt))) {
		StringToLCString(funcname);
		*strptr = tmp_pt;	/* Move ptr past the function name */

		/* Your basic binary search algorithm, O(log 2 N) */
		min = 0; max = NUM_FUNCTIONS - 1;

		while (min <= max) {
			mid = (min + max)/2;
			if ( (found = strcmp (menuFunctionData[mid].fstr,
							 funcname)) < 0)
				min = mid + 1;
			else
				if (found)
					max = mid - 1;
				else { /* strings match, get out */
					*cb = menuFunctionData[mid].cb;
					return(mid);
				} /* else match */
		} /* while */
	} /* if (funcname) */

	/* Unfortunately, if you get this far, min must be > max, so
	 * no matching function name was found - make it a no-op,
	 * because it looks like motif does that.
	 */
	*cb = OlwmNop;
	return(OLWM_NOP);

} /* ConsumeFunction */

/*
 *************************************************************************
 *
 *  DecodeGroupArg.
 *	- strptr pts to position where args to start;
 *	- grp_ret is assumed to be pointing to an unsigned long, AND
 *	  is assumed to be initialized to 0.
 *
 * Return True if group types specified are OK (window, icon, or
 * transient), false otherwise.
 *
 ****************************procedure*header*****************************
 */

static Boolean
DecodeGroupArg OLARGLIST((String *strptr, unsigned long *grp_ret))
OLARG(String *, strptr)
OLGRA(unsigned long *, grp_ret)
{
String tmp_ptr = *strptr;
String startp;
#define MAXCHARS 50
char   type[MAXCHARS];
int    typelen;


	for (;;) {
		AdvanceToken(&tmp_ptr);
		startp = tmp_ptr;
		ConsumeAlf(&tmp_ptr);
		if (startp == tmp_ptr) {
			/* No group specified, use default, window or icon */
				*grp_ret = GROUPTYPE_DEFAULT;
				break;
		}

		if ( (typelen = (tmp_ptr - startp)) >= MAXCHARS)
			typelen = MAXCHARS - 1;
		(void)strncpy(type, startp, typelen);
		type[typelen] = NULL;
		/* Convert to lower case to make it easy to strcmp */
		StringToLCString(type);

		if (strcmp (type,"icon") == 0) {
			*grp_ret |= ICON;
		}
		else
			if (strcmp(type, "window") == 0) {
				*grp_ret |= WINDOW;
			}
			else
				if (strcmp (type,"transient") == 0) {
					*grp_ret |= TRANSIENT;
				}
				else  {
					OlVaDisplayWarningMsg(XtDisplay(Frame),
						OleNgroup, OleTbadSpec,
						OleCOlClientOlwmMsgs,
						OleMgroup_badSpec, NULL);
					return(False);
				} /* else bad spec */

		AdvanceToken(&tmp_ptr);
		if (tmp_ptr == NULL || *tmp_ptr == NULL) {
			break; 
		}
		else
			if (*tmp_ptr == '|') {
				tmp_ptr++;
			}
	} /* for */

	*strptr = tmp_ptr;
	return(True);

} /* DecodeGroupArg */
#undef MAXCHARS

/*
 *************************************************************************
 *
 * ConsumeAlf.
 * - Given a ptr to a String (char *), advance pointer within the string
 * until a non-alphanumeric character is reached.  The result is the
 * pointer is modified.  Allows for multibyte characters is I18N is
 * defined.
 *
 ****************************procedure*header*****************************
 */

void
ConsumeAlf OLARGLIST((strptr))
OLGRA(String *, strptr)
{
#ifdef I18N
int	chlen;

	if (!strptr || !(*strptr))
		return;
	for (chlen = mblen(*strptr, MB_CUR_MAX); *strptr; (*strptr) += chlen) {
		/* Quit advancing ptr. if mblen ret. 0 for char, or
		 * if char len == 1 and char isn't alphanumheric.
		 * If mblen returns a multibyte character length, we
		 * can't use isalnum to test if it's alphanumeric (continue).
		 */
		if ( !(chlen = mblen(*strptr, MB_CUR_MAX)) ||
				chlen == 1 && !(isalnum(**strptr)) )
			break;
	}
		
#else
	for (; *strptr && isalnum(**strptr); (*strptr)++)
			;
#endif

} /* ConsumeAlf */


/*
 *************************************************************************
 *
 *  ConsumeKeySpec.
 *	- Pass ptr to the line buffer currently being parsed, and extract
 *	the key info.  If the syntax is incorrect, return False.
 *	If all goes well, advance the pointer to the buffer (strptr)
 *	beyond the key specification just parsed.
 *
 * 
 ****************************procedure*header*****************************
 */

static Boolean
ConsumeKeySpec OLARGLIST((strptr))
OLGRA(String *, strptr)
{
String	real_ptr = *strptr;
String	tmp_p; 
char	*pt_leftbracket;
char	*pt_rightbracket;
int	keylen;
char	name[20];
int	i;
 
	/* Advance pointer beyond the modifier until '<' - it must be
	 * followed by <[Kk]ey>
	 */
	AdvanceToken (&real_ptr);
	tmp_p = real_ptr;
	if ( (pt_leftbracket = strchr(tmp_p, '<')) == NULL)
		return(False);
	*strptr = ++pt_leftbracket;
	if ( (pt_rightbracket = strchr(pt_leftbracket, '>')) == NULL)
		return(False);
	*strptr = ++pt_rightbracket;
	if ( (pt_rightbracket - pt_leftbracket - 1) !=
					 (keylen = strlen("key")) )
		return(False);
	strncpy(name, pt_leftbracket, keylen);
	name[keylen] = '\0';
	for (i=0; i < keylen; i++)
		name[i] = tolower(name[i]);
	if (strncmp(name, "key", keylen) != 0)
		return(False);

	/* The modifier MUST be terminated by the '<' sign */
	tmp_p = *strptr;
	ConsumeKDetail(&tmp_p);
        *strptr = tmp_p;
	return(True);

} /* ConsumeKeySpec */


/*
 *************************************************************************
 *
 *  FreeMBD(mbd)
 * 
 ****************************procedure*header*****************************
 */

void
FreeMBD OLARGLIST((mbd))
OLGRA(WMMenuButtonData *, mbd)
{
	if (mbd->label != NULL) {
		XtFree(mbd->label);
		mbd->label = NULL;
	}

	if (mbd->accelerator != NULL) {
		XtFree(mbd->accelerator);
		mbd->accelerator = NULL;
	}

	/* Be careful - only free the args that you malloc'd! */
	if ((mbd->menucb != NULL) &&
	   ((mbd->menucb == OlwmExec)  || (mbd->menucb == OlwmMenu)  || 
	   (mbd->menucb == OlwmLower) || (mbd->menucb == OlwmRaise) || 
	   (mbd->menucb == OlwmRaise_Lower)))
	{
		XtFree(mbd->args);
		mbd->args = NULL;
	}

	XtFree((char *)mbd);
	mbd = NULL;

} /* FreeMBD */

/*
 *************************************************************************
 *
 *  StringToLCString -
 *	pass a char *, convert all chars in their place to lower case.
 *	Interesting how we spelled the function name with 2 capital letters
 *	for LC...
 *	- May not be necessary, but we check character length for
 *	multibyte characters.
 * 
 ****************************procedure*header*****************************
 */

void
StringToLCString OLARGLIST((str))
OLGRA(String, str)
{
	String	pt = str;	/* local pointer to string */
	int	bpc;			/* bytes per character */
#ifdef I18N
	for (bpc = mblen(pt, MB_CUR_MAX); bpc; pt+= bpc, bpc = 
						mblen(pt, MB_CUR_MAX))
		if (bpc == 1)
			*pt = tolower(*pt);
#else
	for (; pt && *pt; pt++)
		*pt = tolower(*pt);
#endif

} /* StringToLCString */

/*
 * DecodeArgument - Given a symbol, parse the arguments if any.
 * - whether we are looking for arguments depends on the symbol;
 * - I'll be nice and point arg_ret to the argument(s).
 * Return 0 if bad news, 1 otherwise.
 * mlp -**Needs lots of work; still needs the parsing of the number argument **
 */
int
DecodeArgument OLARGLIST((func_symbol, strptr, mbd))
OLARG(int,		func_symbol)
OLARG(String *,	strptr)
OLGRA(WMMenuButtonData *,	mbd)
{
char *str = (char *)NULL;
String tmp_ptr = *strptr; /* local ptr into line buffer to parse */
XtPointer args_ret = NULL;
int len;
	switch(func_symbol) {
		/* First take care of those functions that expect one
		 * argument, interpreted as char *
		 */
		case OLWM_EXEC:
                        { /* Begin block */
                        unsigned int max;
                        char *tempstr;

			AdvanceToken(&tmp_ptr);
			if ((str = tmp_ptr) != (char *)NULL) { 
				if (str[0] != '\0') {	/* Not empty */
                                	tempstr = (char *)XtMalloc(
						 (max = strlen(tmp_ptr)) + 2);
#ifdef DEBUG
fprintf(stderr,"Malloc of tempstr (option) = %x\n",tempstr);
#endif
                                	strcpy(tempstr, tmp_ptr);

                                /* Is this necessary: making sure that the
                                 * string for f.exec ends in &?
                                 */
                                	if (tempstr[max - 1] != '&') {
                                        	tempstr[max++] = '&';
                                        	tempstr[max] = NULL;
                                	}
				}
				else
					tempstr = NULL;
                                args_ret = (XtPointer)tempstr;
                        } /* str != NULL */
                        else { /* return false if that mandatory arg.
                                *  not here
                                */
                                return(False);
                        }
                        } /* end block */
                        break;
		case OLWM_MENU:
			{ /* Begin block */
			unsigned int max;
			char *tempstr;

			if ((str = NextToken(&tmp_ptr)) != (char *)NULL) {
				tempstr = (char *)XtMalloc( (max = strlen(str)) + 2);
#ifdef DEBUG
fprintf(stderr,"Malloc of tempstr (option) = %x\n",tempstr);
#endif
				strcpy(tempstr, str);

				args_ret = (XtPointer)tempstr;
			} /* str != NULL */
    			else { /* return false if that mandatory arg.
				*  not here
				*/
				return(False);
    			}
			} /* end block */
			break;
		/* Next, those that have mandatory int arg */
		case OLWM_SEND_MSG:
			/* We must have an argument that is a number or you
			 * can kill this function good bye - or just gray it
			 * out! (desensitize).  Pass strptr.
			 */
			{ /* send_msg */
			int retval;
			long *savearg;
			char *rest = NULL;

			AdvanceToken(&tmp_ptr);
			if ((retval = (int) strtol(tmp_ptr, &rest, 0)) < 0
				|| (rest && *rest))
				return(-1);
			/* By returning -1, we return a message that will not
			 * be found.
			 */
			args_ret = (XtPointer)retval;
			} /* send_msg */
			break;
		/* Now those that have up to 3 args, [window|icon|transient]-
		 * they can have all three, but the vertical bar must
		 * separate them.
		 */
		case OLWM_CIRCLEDOWN:
		case OLWM_CIRCLEUP:
		case OLWM_NEXT_KEY:
		case OLWM_PREV_KEY:
			{ /* begin block */
			unsigned long func_args = (unsigned long)0;
			String top;
			int status = DecodeGroupArg(&tmp_ptr,&func_args);
			if (status == False)
				return(0);
			*strptr = tmp_ptr;
			args_ret = (XtPointer)func_args;
			} /* end block */
			break;
		/* These two have optional one argument */
		case OLWM_LOWER:
		case OLWM_RAISE_LOWER: /* mlp - I think this, too -check doc.*/
		case OLWM_RAISE:
			AdvanceToken(&tmp_ptr);
			if (tmp_ptr && *tmp_ptr == '-') {
				*strptr = ++tmp_ptr;
				/* May need another look. */
				if (GetOptionalArg(strptr, mbd, func_symbol))
					/* called function sets mbd->args */
					return(True);
				else
					return(False);
			} /* *tmp_ptr == '-' */
			else {
				mbd->args = NULL;
				return(1);
			}
			break;
		default: /* NO ARGUMENTS, just ignore them */
			*strptr = NULL;
			return(1);
		} /* switch */
		/* mbd->args == either a char *, or an int; or NULL */
		mbd->args = (XtPointer) args_ret;
		return(1);
		
} /* DecodeArgument */

/*
 *************************************************************************
 *
 * GetOptionalArg.
 * - Given a string ptr to the current line being parsed, check for
 * any remaining char * within the line.  This will be placed in the
 * args pointer of the menu button data struct for later use.  Remember
 * to faaaaree it when you're all done.
 * - We allow an argument to be quoted, so there can be spaces within,
 * by calling NextToken.  But it shouldn't be.  The argument should
 * be one single token (word, or string).
 *
 ****************************procedure*header*****************************
 */

static Boolean
GetOptionalArg OLARGLIST((strptr, mbd, fndx))
OLARG(String *, strptr)
OLARG(WMMenuButtonData *, mbd)
OLGRA(int, fndx)
{
	String tmp_ptr = *strptr;
	String str = NextToken(&tmp_ptr);
	String arg = NULL;

	mbd->args = NULL;
	if (str) {
		/* extra byte just in case */
		int len = strlen(str);
		int extra_len = len+2;
		char *tmp_str;

		arg = (String)XtMalloc(extra_len + 2);
#ifdef DEBUG
fprintf(stderr,"Malloc of arg (GetOpt) = %x\n", arg);
#endif
		strcpy(arg, str);
	} /* if str */

	/* This must be freed later */
	mbd->args = (XtPointer)arg;
	return(True);
	/* The final pointer within the string isn't reset, but
	 * it doesn't matter because this is the last work to be done
	 * on the string.
	 */
} /* GetOptionalArg */
