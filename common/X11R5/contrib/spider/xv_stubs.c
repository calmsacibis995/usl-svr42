/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:xv_stubs.c	1.2"
/*
 *      Spider
 *
 *      (c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *      (c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *      (c) Copyright 1990, Heather Rose and Sun Microsystems, Inc.
 *
 *      See copyright.h for the terms of the copyright.
 *
 *      @(#)xv_stubs.c	2.2	90/04/27
 */
#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/file.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/xv_xrect.h>
#include <xview/seln.h>
#include <xview/defaults.h>
#include <X11/Xlib.h>
#include "xv_ui.h"

static char	helpfiles[HELP_MAX+1][32];
char	*instanceName;
char	*resourceFile;
char	*helpDir;

int		INSTANCE, HELPKEY;
spider_window1_objects	*spider_window1;

Bool write_confirmer(), newgame_confirmer(), resource_confirmer();

#define FIRST_BUFFER            0
#define NOT_FIRST_BUFFER        !FIRST_BUFFER
static Seln_rank seln_type = SELN_PRIMARY;
static char *seln_bufs[3];

static struct itimerval timer;
static Notify_value animate();
static int move_index, move_count;
static int replayActive = FALSE, replayStopped = FALSE;

static Window table; 
static Display *dpy;

void
main(argc, argv)
	int		argc;
	char		**argv;
{
	char *save_file = NULL;
	int i;
	static void spider_usage();
	static void spider_init();

	xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, 
		XV_USAGE_PROC,	spider_usage,
		0);

	instanceName = (char *)NULL;
	resourceFile = (char *)NULL;
	helpDir = (char *)NULL;
	for (i = 1; i < argc; i++)      {
                if (strcmp(argv[i], "-s")==0 || 
		    strcmp(argv[i], "-save_file")==0) {
                        if (argv[i+1])  {
                                save_file = argv[++i];
                        } else  {
                                spider_usage(argv[0]);
                        }
		} else if (strcmp(argv[i],"-n")==0 ||
                    strcmp(argv[i],"-name")==0) {
                        if (argv[i+1])  {
                                instanceName = argv[++i];
                        } else  {
                                spider_usage(argv[0]);
                        }        
                } else if (strcmp(argv[i],"-r")==0 ||
                    strcmp(argv[i],"-resource_file")==0) {
                        if (argv[i+1])  {
                                resourceFile = argv[++i];
                        } else  {
                                spider_usage(argv[0]);
                        }        
                } else {
                        spider_usage(argv[0]);
                }
        }
	/*
	 * Initialize user interface components.
	 */
	spider_init();
	INSTANCE = xv_unique_key();
	HELPKEY = xv_unique_key();
	spider_window1 = spider_window1_objects_initialize(NULL, NULL);
	/*
	 * initialize the game components
	 */
	table = xv_get(canvas_paint_window(spider_window1->canvas1), XV_XID);
	dpy = (Display *) xv_get(spider_window1->canvas1, XV_DISPLAY);
	screen = DefaultScreen(dpy);
	gfx_init(dpy, screen);
	table_init(table);
	card_init();
	if (save_file) {
		read_file(save_file);
	}
	xv_main_loop(spider_window1->window1);
	exit(0);
}

static void
spider_usage(name)
	char	*name;
{
	extern void xv_usage();

	(void) fprintf(stderr,
		"usage of %s specific arguments:\n", name);
	(void) fprintf(stderr,
		"FLAG\t(LONG FLAG)\t\tARGS\t\tNOTES\n");
        (void) fprintf(stderr,
		"-s\t(-save_file)\t\t\"string\"\tsaved game file\n");
        (void) fprintf(stderr,
		"-n\t(-name)\t\t\t\"string\"\tname for the game\n");
        (void) fprintf(stderr,
		"-r\t(-resource_file)\t\"string\"\twhich resource file\n");
	xv_usage(name);
	exit(-1);
}

static void
spider_init()
{
	char *home;
	extern char *getenv();

	if (instanceName == (char *)NULL) {
		instanceName = malloc(strlen(SPIDER_NAME) + 1);
		sprintf(instanceName, "%s", SPIDER_NAME);
	}
	if (resourceFile == (char *)NULL) {
		if ((home = getenv("HOME")) != (char *)NULL) {
			resourceFile = malloc(strlen(SPIDER_DEFAULTS) + 
			    strlen(home) + 1);
			sprintf(resourceFile, "%s/%s", home, SPIDER_DEFAULTS);
		} else {
			resourceFile = malloc(strlen(SPIDER_DEFAULTS) + 1);
			sprintf(resourceFile, "%s", SPIDER_DEFAULTS);
		}
        }
	if (helpDir == (char *)NULL) {
		helpDir = malloc(strlen(HELPDIR) +1);
		sprintf(helpDir, "%s", HELPDIR);
	}
	sprintf(helpfiles[HELP_INTRO], "doc.intro");
	sprintf(helpfiles[HELP_RULES], "doc.rules");
	sprintf(helpfiles[HELP_CNTRLS], "doc.controls");
	sprintf(helpfiles[HELP_EXS], "doc.examples");
	sprintf(helpfiles[HELP_MISC], "doc.misc");
	sprintf(helpfiles[HELP_SUM], "doc.summary");
}

void
show_message(str)
char    *str;
{
    xv_set(spider_window1->window1, FRAME_LEFT_FOOTER, str, NULL);
}

/*
 * Menu handler for `BackUpMenu (One Move)'.
 */
Menu_item
undo_onemove_handler(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	if (op == MENU_NOTIFY)
		undo();
	return item;
}

/*
 * Menu handler for `BackUpMenu (Start Over)'.
 */
Menu_item
undo_startover_handler(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	if (op == MENU_NOTIFY) {
		(void)replay();
		init_cache();
	}
	return item;
}

/*
 * Menu handler for `BackUpMenu (Replay)'.
 */
Menu_item
undo_replay_handler(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	if (op == MENU_NOTIFY) {
		show_play();
	}
	return item;
}

/*
 * Menu handler for `FileMenu (Save in File)'.
 */
Menu_item
file_save_handler(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	spider_window1_objects  *ip;
        char *filename;

	if (op == MENU_NOTIFY) {
		ip = (spider_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
		filename = (char *) xv_get(ip->textfield1, PANEL_VALUE);
		if (filename)
			write_file(filename, write_confirmer);
		else
			show_message("No file name entered.  File not saved.");
	}
	return item;
}

/*
 * Menu handler for `FileMenu (Resume from File)'.
 */
Menu_item
file_resume_handler(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	spider_window1_objects  *ip;
        char *filename;

	if (op == MENU_NOTIFY) {
		ip = (spider_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
		filename = (char *) xv_get(ip->textfield1, PANEL_VALUE);
		if (filename) {
			read_file(filename);
		}
		if (restart) {
			shuffle_cards();
		}
		XClearArea(dpy, table, 0, 0, 0, 0, True);

	}
	return item;
}

/*
 * Menu handler for `FileMenu (Resume from Selection)'.
 */
Menu_item
file_resumefromselection_handler(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	char *game;
	char *xv_get_selection();
	spider_window1_objects  *ip;

	if (op == MENU_NOTIFY) {
		ip = (spider_window1_objects*)xv_get(item,XV_KEY_DATA,INSTANCE);
		game = xv_get_selection(XV_SERVER_FROM_WINDOW(ip->window1));
		read_selection(game);
		if (restart) {
			shuffle_cards();
		}
		XClearArea(dpy, table, 0, 0, 0, 0, True);
	}
	return item;
}

/*
 * Menu handler for `FileMenu (Properties...)'.
 */
Menu_item
file_properties_handler(item, op)
        Menu_item       item;
        Menu_generate   op;
{
        spider_window1_objects  *ip;

        if (op == MENU_NOTIFY) {
                ip = (spider_window1_objects*)xv_get(item,XV_KEY_DATA,INSTANCE);
		if (ip->subwindow2 == (spider_subwindow2_objects *)NULL) {
			ip->subwindow2 = spider_subwindow2_objects_initialize(ip, ip->window1);
		}
		if (!(int)xv_get(ip->subwindow2->window3, XV_SHOW)) {
			xv_set(ip->subwindow2->window3, 
				XV_SHOW, 		TRUE, 
				FRAME_CMD_PUSHPIN_IN,	TRUE,
				NULL);
		} else {
			wmgr_top(ip->subwindow2->window3);
		}
        }
        return item;
}

/*
 * Notify callback function for `button1 (New Game)'.
 */
void
newgame_handler(item, event)
	Panel_item	item;
	Event		*event;
{
	if (newgame_confirmer()) {
		clear_message();
		shuffle_cards();
	} else {
		show_message("No new game started");
	}
}

/*
 * Notify callback function for `button3 (Expand)'.
 */
void
expand_handler(item, event)
        Panel_item      item;
        Event           *event;
{
	extern void do_expand();

	do_expand();
}

/*
 * Notify callback function for `button4 (Locate)'.
 */
void
locate_handler(item, event)
	Panel_item	item;
	Event		*event;
{
	spider_window1_objects  *ip;
	char *fname;

	ip = (spider_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	fname = (char *) xv_get(ip->textfield1, PANEL_VALUE);

	if (strlen(fname)) {
		/* remove leading white space */
		fname = remove_newlines(fname);
		locate(fname);
	} else {
		show_message("Need to enter a card to locate. i.e. 8H will find all eight of hearts, where K will find all kings.");
	}
}

/*
 * Notify callback function for `button5 (Score)'.
 */
void
score_handler(item, event)
	Panel_item	item;
	Event		*event;
{
	char buf[512];
	
	sprintf(buf, "Current position scores %d out of 1000.",
		compute_score());
	show_message(buf);
}

/*
 * Menu handler for `HelpMenu (Intro, Rules, Controls, Examples, Extras, Summary)'.
 */
Menu_item
help_handler(item, op)
	Menu_item	item;
	Menu_generate	op;
{
	int which;
	char buf[256];
	spider_window1_objects  *ip;
	void do_spider_help();

	if (op == MENU_NOTIFY) {
		which = (int) xv_get(item, XV_KEY_DATA, HELPKEY);
		if (which >= HELP_MIN && which < HELP_MAX) {
			ip = (spider_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
			do_spider_help(ip, ip->window1, which);
		} else {
			sprintf(buf,"Error in show help, key = %d", which);
			show_message(buf);
			return item;
		}
	}
	return item;
}

/*
 * Set the help file for the help popup
 */
void
do_spider_help(ip, owner, which)
	spider_window1_objects         *ip;
        Xv_opaque       owner;
	int		which;
{
	char buf[256];

	sprintf(buf,"%s/%s",ip->defaults->helpDir, helpfiles[which]);

	if (ip->subwindow1 == NULL) {
		ip->subwindow1 = spider_subwindow1_objects_initialize(ip, ip->window1);
	}
	xv_set(ip->subwindow1->window2, 
		FRAME_LABEL, buf,
		NULL);
	xv_set(ip->subwindow1->textsw1,
		TEXTSW_READ_ONLY, TRUE,
		TEXTSW_FILE, buf,
		NULL);
	if (!(int)xv_get(ip->subwindow1->window2, XV_SHOW)) {
		xv_set(ip->subwindow1->window2, 
			XV_SHOW,		TRUE, 
			FRAME_CMD_PUSHPIN_IN,	TRUE,
			NULL);
	} else {
		wmgr_top(ip->subwindow1->window2);
	}
	show_message(buf);
}

/*
 * Event callback function for `canvas1 (Game Table)'.
 *
 * TODO:  When dragging a card around, change the cursor to a card image
 *        so people can tell that they are dragging something around.
 */
Notify_value
handle_card_event(win, event, arg, type)
	Xv_Window	win;
	Event		*event;
	Notify_arg	arg;
	Notify_event_type type;
{
	XEvent *xev;

	XAllowEvents(dpy, AsyncBoth, CurrentTime); /* work around bug 1033840 */
	xev = event_xevent(event);
	switch(xev->xany.type) {
	    case Expose:
                redraw_table((XExposeEvent *)xev);
		break;
	    case ButtonPress:
		if (replayActive) {
			replayStopped = TRUE;
		} else 
			button_press((XButtonPressedEvent *)xev);
		break;
	    case ButtonRelease:
		if (replayActive) {
			replayStopped = TRUE;
		} else 
			button_release((XButtonReleasedEvent *)xev);
		break;
	    case KeyPress:
		if (replayActive) {
			replayStopped = TRUE;
		} else 
			key_press((XKeyPressedEvent *)xev);
		break;
	    default:
		return notify_next_event_func(win, event, arg, type);
	}
	if (restart && !replayActive) {
		shuffle_cards();
	}
	return NOTIFY_DONE;
}


/*
 * Resize proc for `canvas1'
 */
void
spider_resize_proc(canvas, width, height)
	Canvas canvas;
	int width, height;
{
	int i;

	table_width = width;
	table_height = height;

	/* fix stacks */
	for (i = 0; i < NUM_STACKS; i++)        {
		if (stack[i]) {
			recompute_list_deltas(stack[i]);
		}
	}
}

/*
 * Notify callback function for `button8 (Intro)', `button9 (Rules)', 
 *	`button10 (Controls)',	`button11 (Examples)', `button12 (Extras)'.
 */
void
subhelp_handler(item, event)
	Panel_item      item;
        Event           *event;
{
	spider_window1_objects *ip;
	char buf[512];
	int which;
	void do_spider_help();

	which = (int) xv_get(item, XV_KEY_DATA, HELPKEY);
	if (which < HELP_MIN || which >= HELP_MAX) {
		sprintf(buf,"Error in show help, key = %d", which);
		show_message(buf);
	} else {
		ip = (spider_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
		do_spider_help(ip, ip->window1, which);
	}
}
		
/*
 * Notify callback function for for `button14 (Done)'
 */
void
help_done_handler(item, event)
	Panel_item      item;
        Event           *event;
{
	spider_window1_objects *ip;
	
	ip = (spider_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
        xv_set(ip->subwindow1->window2,
                XV_SHOW,                FALSE,
                FRAME_CMD_PUSHPIN_IN,   FALSE,
                0);
	xv_set(ip->subwindow1->textsw1,
		TEXTSW_FILE, NULL,
		0);
}

char *
get_selection()
{
	return(xv_get_selection(XV_SERVER_FROM_WINDOW(spider_window1->window1)));
}

/*
 * Return the contents of the primary selection.
 */
char *
xv_get_selection(server)
	Xv_Server	  server;
{
	Seln_holder   holder;
	Seln_result   result;
	Seln_request  *response;
	char		  context = FIRST_BUFFER;
	Seln_result   read_proc();

	holder = selection_inquire(server, seln_type);
 
	result = selection_query(server, &holder, read_proc, &context,
		SELN_REQ_BYTESIZE,			  NULL,
		SELN_REQ_CONTENTS_ASCII,		NULL,
		NULL);
	if ((int)result == SELN_FAILED) {
	show_message("Failed to get the current selection");
		return NULL;
	}
	return seln_bufs[seln_type];
}

/*
 * Called N times to read N buffers of data when the selection is greater
 * than 1 buffer in size.
 */
Seln_result
	read_proc(response)
	Seln_request *response;
{
#ifndef MEMUTIL
	extern char *malloc();
#endif /* MEMUTIL */

	char *reply;
	long seln_len;
	static long seln_have_bytes;

	if (*response->requester.context == FIRST_BUFFER) {
		reply = response->data;
		reply += sizeof(SELN_REQ_BYTESIZE);
		seln_len = *(int *)reply;
		reply += sizeof(long);

		if (seln_bufs[seln_type] != NULL)
			free(seln_bufs[seln_type]);
		if (!(seln_bufs[seln_type] = malloc(seln_len + 1))) {
			show_message("Not enough memory to read the current selection");
			return(SELN_FAILED);
		}
		seln_have_bytes = 0;

		reply += sizeof(SELN_REQ_CONTENTS_ASCII);
		*response->requester.context = NOT_FIRST_BUFFER;
	} else {
		reply = response->data;
	}
	(void) strcpy(&seln_bufs[seln_type][seln_have_bytes], reply);
	seln_have_bytes += strlen(reply);

	return SELN_SUCCESS;
}

/*
 * Notify callback function for `button15 (Apply)'.
 */
void
props_apply_handler(item, event)
        Panel_item      item;
        Event           *event;
{
        spider_window1_objects *ip;
	char buf[256];
	int i;
	char *s;
	Bool resource_confirmer();
	extern Bool usebell, round_cards;
	extern int deltamod;
	extern void force_redraw();

	ip = (spider_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);

	if ((i = (int)xv_get(ip->subwindow2->choice1, PANEL_VALUE)) !=
            ip->defaults->bell) {
		ip->defaults->bell = usebell = i;
		sprintf(buf,"%s.%s", ip->defaults->instanceName, "bell");
		defaults_set_boolean(buf,ip->defaults->bell);
	}
	if ((i = (int)xv_get(ip->subwindow2->numtext1, PANEL_VALUE)) != 
            ip->defaults->replayTime) {
		ip->defaults->replayTime = i;
		sprintf(buf,"%s.%s", ip->defaults->instanceName, "replayTime");
		defaults_set_integer(buf, ip->defaults->replayTime);
	}
#ifdef ROUND_CARDS
	if ((i = (int)xv_get(ip->subwindow2->choice2, PANEL_VALUE)) !=
	    ip->defaults->roundCards) {
		ip->defaults->roundCards = round_cards = i;
		force_redraw();
		sprintf(buf,"%s.%s", ip->defaults->instanceName, "roundCards");
		defaults_set_boolean(buf, ip->defaults->roundCards);
	}
#endif
	if ((i = (int)xv_get(ip->subwindow2->choice3, PANEL_VALUE)) !=
            ip->defaults->confirm) {
		ip->defaults->confirm = i;
		sprintf(buf,"%s.%s", ip->defaults->instanceName, "confirm");
		defaults_set_boolean(buf,ip->defaults->confirm);
	}
	if ((i = (int)xv_get(ip->subwindow2->slider1, PANEL_VALUE)) != 
            ip->defaults->deltaMod) {
		ip->defaults->deltaMod =  deltamod = i;
		sprintf(buf,"%s.%s", ip->defaults->instanceName, "deltaMod");
		defaults_set_integer(buf, ip->defaults->deltaMod);
	}
	if ((i = (int)xv_get(ip->subwindow2->slider2, PANEL_VALUE)) !=  
            ip->defaults->textField) { 
                ip->defaults->textField =  i;
                sprintf(buf,"%s.%s", ip->defaults->instanceName, "textField");
                defaults_set_integer(buf, ip->defaults->textField);
		xv_set(ip->textfield1, PANEL_VALUE_DISPLAY_LENGTH, i, 0);
		xv_set(ip->subwindow2->textfield2, PANEL_VALUE_DISPLAY_LENGTH, i, 0);
        }
	s = (char *)xv_get(ip->subwindow2->textfield2, PANEL_VALUE);
	if (strcmp(ip->defaults->helpDir, s) != 0) {
		if (ip->defaults->helpDir != (char *)NULL) {
			free(ip->defaults->helpDir);
		}
		ip->defaults->helpDir = malloc(strlen(s) + 1);
		sprintf(ip->defaults->helpDir, "%s", s);
		sprintf(buf,"%s.%s", ip->defaults->instanceName, "helpDir");
		defaults_set_string(buf, ip->defaults->helpDir);
		if (helpfiles_exist(ip->defaults->helpDir)) {
			xv_set(ip->button6, PANEL_INACTIVE, FALSE, 0);
		} else {
			xv_set(ip->button6, PANEL_INACTIVE, TRUE, 0);
		}
	}
	/* TODO:  Use xrm directly since XView defaults currently store 
	 *        the entire contents of xrm to the resource file instead of
	 *        a select number of things 
	 */
	if ((access(resourceFile, R_OK) == -1) || 
	    resource_confirmer()) {
		defaults_store_db(resourceFile);
		sprintf(buf,"Defaults saved to %s", resourceFile);
		show_message(buf);
	} else {
		sprintf(buf,"Defaults not saved to %s", resourceFile);
		show_message(buf);
	}
}

/*
 * Notify callback function for `button16 (Reset)'.
 */
void
props_reset_handler(item, event)
        Panel_item      item;
        Event           *event;
{
        spider_window1_objects *ip;

	ip = (spider_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	xv_set(ip->subwindow2->choice1, 
		PANEL_VALUE, ip->defaults->bell, 
		0);
	xv_set(ip->subwindow2->numtext1, 
		PANEL_VALUE, ip->defaults->replayTime, 
		0);
#ifdef ROUND_CARDS
	xv_set(ip->subwindow2->choice2, 
		PANEL_VALUE, ip->defaults->roundCards, 
		0);
#endif
	xv_set(ip->subwindow2->slider1, 
		PANEL_VALUE, ip->defaults->deltaMod, 
		0);
	xv_set(ip->subwindow2->slider2,
                PANEL_VALUE, ip->defaults->textField,
                0);
	xv_set(ip->subwindow2->textfield2, 
		PANEL_VALUE, ip->defaults->helpDir, 
		0);
}

/*
 * Notify callback function for `button17 (Done)'.
 */
void
props_done_handler(item, event)
	Panel_item 	item;
	Event 		*event;
{
	spider_window1_objects *ip;
	Xv_opaque win;

	ip = (spider_window1_objects *) xv_get(item, XV_KEY_DATA, INSTANCE);
	(void) xv_set(ip->subwindow2->window3, 
		XV_SHOW,		FALSE, 
		FRAME_CMD_PUSHPIN_IN,	FALSE,
		0);
}

Bool
write_confirmer()
{
	int result;
	extern int do_notice();

	if (spider_window1->defaults->confirm) {
		return(do_notice("Over-write existing file?")); 
        } else {
                return TRUE;
        }
}

Bool
newgame_confirmer()
{
	int result;
	extern int do_notice();

	if (spider_window1->defaults->confirm) {
		return(do_notice("Really discard current game?"));
        } else {
                return TRUE;
	}
}

Bool
resource_confirmer()
{
	int result;
        extern int do_notice();
	char buf[512];

        if (spider_window1->defaults->confirm) {
		sprintf(buf,"Really over-write file %s?", resourceFile);
                return(do_notice(buf));
        } else {
                return TRUE;
        }
}

/* TODO:  Bring up a dialog box to interactively start/stop/change speed of
 *        of the replay.  May want to add frame busy feedback.
 */
void
show_play()
{
	extern int get_move_index();

	if (replay() == -1)
                return;

	show_message("Showing all moves -- hit any key or button to abort");
	move_count = 0;
	move_index = get_move_index();
	replayActive = TRUE;
	replayStopped = FALSE;
	timer.it_value.tv_usec = (spider_window1->defaults->replayTime );
	timer.it_interval.tv_usec = (spider_window1->defaults->replayTime );
	xv_set(spider_window1->button1, PANEL_INACTIVE, TRUE, 0);
	xv_set(spider_window1->button2, PANEL_INACTIVE, TRUE, 0);
	xv_set(spider_window1->button3, PANEL_INACTIVE, TRUE, 0);
	xv_set(spider_window1->button4, PANEL_INACTIVE, TRUE, 0);
	xv_set(spider_window1->button5, PANEL_INACTIVE, TRUE, 0);
	xv_set(spider_window1->button7, PANEL_INACTIVE, TRUE, 0);
        notify_set_itimer_func(spider_window1->window1, animate,
            ITIMER_REAL, &timer, NULL);

}

static Notify_value
animate()
{
	extern Bool  show_n_moves();
	extern replay();

	if (!replayStopped && (move_count<move_index) && 
	    show_n_moves(move_count, 1)) {
		move_count++;
        } 
	if (replayStopped || move_count >= move_index) {
		notify_set_itimer_func(spider_window1->window1, NOTIFY_FUNC_NULL,
		    ITIMER_REAL, NULL, NULL);
		xv_set(spider_window1->button1, PANEL_INACTIVE, FALSE, 0);
		xv_set(spider_window1->button2, PANEL_INACTIVE, FALSE, 0);
		xv_set(spider_window1->button3, PANEL_INACTIVE, FALSE, 0);
		xv_set(spider_window1->button4, PANEL_INACTIVE, FALSE, 0);
		xv_set(spider_window1->button5, PANEL_INACTIVE, FALSE, 0);
		xv_set(spider_window1->button7, PANEL_INACTIVE, FALSE, 0);
		if (replayStopped) {
			(void)replay();
			show_message("Replay terminated");
		} else {
			show_message("Replay finished");
		}
		replayActive = FALSE;
	}
	return NOTIFY_DONE;
}

helpfiles_exist(helpdir)
	char *helpdir;
{
	char buf[256];
	int i;

	for (i=HELP_MIN; i<HELP_MAX; i++) {
		sprintf(buf,"%s/%s",helpdir, helpfiles[i]);
		if (access(buf, R_OK) == -1)   {
			return FALSE;
		}
	}
	return TRUE;
}
