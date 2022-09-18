/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olps:externs.h	1.14"
#endif

#include <DnD/OlDnDVCX.h>

extern 	int 	errno;
#include <stdlib.h>
extern 	char 	* strcpy();
extern 	char 	* strcat();

extern	Arg               args[];
extern	int               cnt;

extern	char		* appname;
extern	Display         * dpy;
extern	int               screen;
extern	GC                gc;
extern	XGCValues         gc_val;
extern	Window            stub_window;
extern	Pixmap            pix;

extern	FILE            * rw_file;
extern	XImage            image_struct, 
			* image_ptr, 
			* image_content, 
			* image_nocontent;
extern	XWDFileHeader     header, 		  header_nc;
extern  int 		  header_size, 	  	  header_size_nc;
extern	char            * win_name, 		* win_name_nc;
extern	int               win_name_size, 	  win_name_size_nc;
extern	XColor          * colors, 		* colors_nc;
extern	int               ncolors, 		  ncolors_nc;
extern	char 		* buffer;
extern	unsigned          buffer_size,	  	  buffer_size_nc;
extern	unsigned long 	  swaptest;
extern  XWindowAttributes win_info;
extern	Window		  rootwindow;
extern	int               rootwidth, rootheight;
extern	Dimension         olps_width_limit, olps_height_limit,
			  form_save_width, form_save_height;

extern	XEvent            event;

extern	int               f_contents,
       	          	  f_contentssaved,
       	          	  f_bypass,
       	          	  f_screen,
       	          	  f_area,
       	          	  f_save_overwrite,
                  	  redraw,
                  	  f_unmapping,
                  	  f_open_pop,
                  	  f_save_pop,
                  	  f_printf_pop,
                  	  f_prop_pop,
                  	  f_fromopenfile,
			  f_toggle,
			  f_failed;

extern	Widget            toplevel,
		          form,
		          upper_control,
		          scrollw,
		          stub,
		          open_pop_textf,
		          save_pop_textf,
		          printf_pop_textf1,
		          footer_text,
		          save_pop_ok,
		          printf_pop_ok,
		          open_pop_ok,
		          save_notice,
		          save_notice_control,
		          save_notice_text,
		          save_notice_overwrite,
		          save_notice_cancel,
			  open_pop,
			  save_pop,
			  printf_pop,
			  prop_pop;

extern	char		  printcmd[];
extern	char		  message[];
extern	char		  *ButtonFields[];

extern	Cursor            wait_cursor,
			  sa_cursor,
			  sw_cursor;

int	CanOpenFile();
void	SetFooterText();
void	Redraw();
void	Save_Resolve();
void	Open_Resolve();
void	Print_Resolve();
void	Open_Cancel();
void	Print_Cancel();
void 	Open_File();
void 	Open_Popup();
void 	Open_Popdown();
void 	Open_Pop();
void 	Save_File();
void 	Save_Overwrite();
void 	Save_Cancel();
void 	Save_Popup();
void 	Save_Popdown();
void 	Save_Pop();
void 	Print_File();
void 	Printf_Popup();
void 	Printf_Popdown();
void 	Printf_Pop();
void 	Dump();
void 	Dump_Screen();
void 	Prop_Popup();
void 	Prop_Popdown();
void 	Properties_Pop();
int	FileToImage();
void	ImageToPane();
int	PaneToFile();
void	Free1();
void	Print_Contents();
void	Capture_Window();
void	WindowToImage();
int	Window_Dump();
void	Print_Window();
void	UnmapTop();
void	MapTop();
void	Capture_Screen();
void	Print_Screen();
void	Capture_Area();
void	Print_Area();

extern void	CreateCommandWindow OL_ARGS((Widget, Widget *, char *,
					char *, char *, Widget *, char *,
					Widget *, ButtonItems *, int));

void 	Fatal_Error();
Window 	Select_Window();
void 	ExecutePrint();
void 	CreatePropertyWindow();

extern Boolean	PSTriggerNotify OL_ARGS((Widget, Window, Position,
					 Position, Atom, Time,
					 OlDnDDropSiteID,
					 OlDnDTriggerOperation, Boolean,
					 Boolean, XtPointer));
