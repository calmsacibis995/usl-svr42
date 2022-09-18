/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olps:prop.c	1.23"
#endif

/*
 * prop.c - all routines relating to property sheet:
 *	    creation,
 *	    apply, set defaults, reset, reset to factory,
 *          validation for saved values
 *          reading/writing from/to .Xdefaults via olwsm
 */

#include "main.h"
#include "externs.h"
#include "ctype.h"

#include <Xol/OpenLookP.h>
#include "error.h"
#include <Xol/FButtons.h>

#define PROP_PORTRAIT   "portrait"
#define PROP_LANDSCAPE  "landscape"
#define PROP_ZFORMAT	"zpixmap"
#define PROP_XYFORMAT	"xypixmap"
#define PROP_OFF        "off"
#define PROP_ON         "on"

#define FACTORY_FILENAME	""
#define FACTORY_OUTPUT		"postscript"
#define FACTORY_PRINT		" | lp -c "
#define FACTORY_HEADER		""
#define FACTORY_FOOTER		""
#define FACTORY_PAGES		""
#define FACTORY_SCALE		""
#define FACTORY_LEFT		""
#define FACTORY_TOP		""
#define FACTORY_WIDTH		""
#define FACTORY_HEIGHT		""
#define FACTORY_ORIENTATION	"portrait"
#define FACTORY_PIXFORMAT	"zpixmap"
#define FACTORY_REVERSE_VIDEO	"off"

extern char *	getenv();	/* see getenv 3(C) */

extern void 	Apply(), Reset(), SetDefaults(), ResetFactory(),
		Verify(),
		SetProperties(), CopyProps();
extern int  	GetAndValidate(), ValidateInteger(),
		ValidateDecimalNumber(); 
extern char pixmapformat[];
int	atoi();
double	atof();
int	strcmp();

/* want it to popdown unless some things occur */
static 	int     f_popdown = True;
static	Widget	pop_footer_text;
static	Widget  p_filename_textf, p_output_textf, p_print_textf,
                  p_header_textf, p_footer_textf, p_pages_textf,
                  p_scale_textf, p_left_textf, p_top_textf,
                  p_width_textf, p_height_textf;

/* flat_orient - exclusives FlatButtons widget (portrait, landscape);
 * flat_revvideo - exclusives FaltButtons widget (off, on)
 * flat_pixformat - exclusives FaltButtons widget (zpixmap, xypixmap)
 */ 
static Widget	flat_orient, flat_revvideo, flat_pixformat;

struct                    properties {
                                char    filename	[TEXTF_WIDTH_1];
                                char    output  	[TEXTF_WIDTH_2];
                                char    print   	[TEXTF_WIDTH_1];
                                char	orientation	[TEXTF_WIDTH_2];
				char	pixformat	[TEXTF_WIDTH_2];
                                char	reverse_video	[TEXTF_WIDTH_3];
                                char    header  	[TEXTF_WIDTH_2];
                                char    footer  	[TEXTF_WIDTH_2];
                                char    pages   	[TEXTF_WIDTH_3];
                                char    scale   	[TEXTF_WIDTH_3];
                                char    left    	[TEXTF_WIDTH_3];
                                char    top     	[TEXTF_WIDTH_3];
                                char    width   	[TEXTF_WIDTH_3];
                                char    height  	[TEXTF_WIDTH_3];
                         };

/* factory missing first fields, to be set below */
struct  properties      factory = {
                          FACTORY_FILENAME, FACTORY_OUTPUT, FACTORY_PRINT,
                          FACTORY_ORIENTATION, FACTORY_PIXFORMAT,    
			  FACTORY_REVERSE_VIDEO,
                          FACTORY_HEADER,   FACTORY_FOOTER, FACTORY_PAGES,
                          FACTORY_SCALE,    FACTORY_LEFT,   FACTORY_TOP,
                          FACTORY_WIDTH,    FACTORY_HEIGHT,
                        };
struct  properties      temp, saved;

static	char            tempcmd[PRINTCMD_WIDTH];

static	char 	*r_filename, 	*r_output, 	*r_print,
		*r_orientation,	*r_pixformat,   *r_reverse_video,
		*r_header,	*r_footer,	*r_pages,
        	*r_scale,	*r_left,	*r_top,
		*r_width,	*r_height;

static XtResource               resources[] =
{
        { "ps_filename", "Ps_filename", XtRString, sizeof(String),
                (Cardinal) &r_filename,
                XtRString, (XtPointer) FACTORY_FILENAME},
        { "ps_output", "Ps_output", XtRString, sizeof(String),
                (Cardinal) &r_output,
                XtRString, (XtPointer) FACTORY_OUTPUT},
        { "ps_print", "Ps_print", XtRString, sizeof(String),
                (Cardinal) &r_print,
                XtRString, (XtPointer) FACTORY_PRINT},
        { "ps_orientation", "Ps_orientation", XtRString, sizeof(String),
                (Cardinal) &r_orientation,
                XtRString, (XtPointer) FACTORY_ORIENTATION},
	{ "ps_pixformat", "Ps_pixformat", XtRString, sizeof(String),
		(Cardinal) &r_pixformat,
		XtRString, (XtPointer) FACTORY_PIXFORMAT},
        { "ps_reverse_video", "Ps_reverse_video", XtRString, sizeof(String),
                (Cardinal) &r_reverse_video,
                XtRString, (XtPointer) FACTORY_REVERSE_VIDEO},
        { "ps_header", "Ps_header", XtRString, sizeof(String),
                (Cardinal) &r_header,
                XtRString, (XtPointer) FACTORY_HEADER},
        { "ps_footer", "Ps_footer", XtRString, sizeof(String),
                (Cardinal) &r_footer,
                XtRString, (XtPointer) FACTORY_FOOTER},
        { "ps_pages", "Ps_pages", XtRString, sizeof(String),
                (Cardinal) &r_pages,
                XtRString, (XtPointer) FACTORY_PAGES},
        { "ps_scale", "Ps_scale", XtRString, sizeof(String),
                (Cardinal) &r_scale,
                XtRString, (XtPointer) FACTORY_SCALE},
        { "ps_left", "Ps_left", XtRString, sizeof(String),
                (Cardinal) &r_left,
                XtRString, (XtPointer) FACTORY_LEFT},
        { "ps_top", "Ps_top", XtRString, sizeof(String),
                (Cardinal) &r_top,
                XtRString, (XtPointer) FACTORY_TOP},
        { "ps_width", "Ps_width", XtRString, sizeof(String),
                (Cardinal) &r_width,
                XtRString, (XtPointer) FACTORY_WIDTH},
        { "ps_height", "Ps_height", XtRString, sizeof(String),
                (Cardinal) &r_height,
                XtRString, (XtPointer) FACTORY_HEIGHT},
};

static XtCallbackRec applycalls[] = {
                { Apply, NULL },
                { NULL, NULL },
        };

static XtCallbackRec resetcalls[] = {
                { Reset, NULL },
                { NULL, NULL },
        };

static XtCallbackRec defaultcalls[] = {
                { SetDefaults, NULL },
                { NULL, NULL },
        };

static XtCallbackRec factorycalls[] = {
                { ResetFactory, NULL },
                { NULL, NULL },
        };

static XtCallbackRec verifycalls[] = {
                { Verify, NULL },
                { NULL, NULL },
        };

typedef struct {
	XtArgVal e_label;
	XtArgVal is_set;
} Exclbutton;

char *excl_fields[] =  {
	{ XtNlabel },
	{ XtNset }
};

/*
Exclbutton excl_orientation[] = {
	{ "portrait"},
	{ "landscape" }
};

Exclbutton excl_revvideo[] = {
	{ "off"},
	{ "on" }
};
*/
static Exclbutton excl_orientation[2] = {
	{(XtArgVal)NULL, (XtArgVal)False},
	{(XtArgVal)NULL, (XtArgVal)False}
};

static Exclbutton excl_pixformat[2] = {
	{(XtArgVal)NULL, (XtArgVal)False},
	{(XtArgVal)NULL, (XtArgVal)False}
};

Exclbutton excl_revvideo[] = {
	{ (XtArgVal)NULL, (XtArgVal)False},
	{ (XtArgVal)NULL, (XtArgVal)False }
};


/* This will come in handy for the caption titles and mnemonics */
#define GET_CAPTION_LABEL(T, M)  OlGetMessage( display, NULL, 0, OleNcaption, \
						T, OleCOlClientOlpsMsgs, \
						M, (XrmDatabase)NULL)
#define GET_CAPTION_MNEM(T, M)  *(OlGetMessage( display, NULL, 0, OleNmnemonic, \
						T, OleCOlClientOlpsMsgs, \
						M, (XrmDatabase)NULL))

void
CreatePropertyWindow( parent, pop)
Widget parent,
       *pop;
{
Widget pop_upper, pop_lower, pop_footer,
       pop_upper1, pop_upper2,
       caption, exclusives;
int	temp;
Display *display = XtDisplay(parent);

/*
 * (1) Get property fields from .Xdefaults,
 *     (if not found set to factory defaults)
 * (2) strcpy (saved, fields)
 * (3) validated subset of saved fields
 * (4) put fields into printcmd
 * (5) setting Command Popups' textfield filenames
 * (6) create property sheet using the fields
 */

/* initializing stuff that was not initialized at top of this file */
strcpy(factory.filename,getenv("HOME"));

/*
 * (1) Get property fields from .Xdefaults,
 *     (if not found set to factory defaults)
 */
XtGetApplicationResources(toplevel, NULL, resources,
                          XtNumber(resources), NULL, 0);

/* (2) strcpy (saved, fields) */
   /* r_filename is NULL, a dummy */
   strcpy(saved.filename,          r_filename);
   strcpy(saved.output,            r_output);
   strcpy(saved.print,             r_print);
   strcpy(saved.orientation,       r_orientation);
   strcpy(saved.pixformat,         r_pixformat);
   strcpy(saved.reverse_video,     r_reverse_video);
   strcpy(saved.header,            r_header);
   strcpy(saved.footer,            r_footer);
   strcpy(saved.pages,             r_pages);
   strcpy(saved.scale,             r_scale);
   strcpy(saved.left,              r_left);
   strcpy(saved.top,               r_top);
   strcpy(saved.width,             r_width);
   strcpy(saved.height,            r_height);

/* (3) validate all 13 saved fields */
/* These are for cases where there was entry in .Xdefaults,
   but they are not valid, so we have to re-set them to factory settings */

/* check to see if property filename is creatable? */

   /* Because FACTORY_FILENAME cannot be set at init time,
      check here that if .Xdefaults' filename was blank, set to factory */
   if (saved.filename[0] == '\0')
	strcpy(saved.filename, factory.filename);
   if (saved.output[0] == '\0')
	strcpy(saved.output, factory.output);
   if (saved.print[0] == '\0')
	strcpy(saved.print, factory.print);
   if ((strcmp(saved.orientation,PROP_PORTRAIT)!=0)
	&& (strcmp(saved.orientation,PROP_LANDSCAPE)!=0))
		strcpy(saved.orientation, FACTORY_ORIENTATION);
   if ((strcmp(saved.pixformat,PROP_ZFORMAT)!=0)
	&& (strcmp(saved.pixformat,PROP_XYFORMAT)!=0))
		strcpy(saved.pixformat, FACTORY_PIXFORMAT);
   if ((strcmp(saved.reverse_video,PROP_OFF)!=0)
	&& (strcmp(saved.reverse_video,PROP_ON)!=0))
		strcpy(saved.reverse_video, FACTORY_REVERSE_VIDEO);
   /* following fields can be blank */
   /* set to NULL if string is "blank" */
   if (strcmp(saved.header,"blank")==0)
	strcpy(saved.header, "");
   if (strcmp(saved.footer,"blank")==0)
	strcpy(saved.footer, "");
   /* if not number, set to factory, which happens to be blank */
   if (ValidateInteger(saved.pages, NULL, True) != 0) 
	strcpy(saved.pages, FACTORY_PAGES);
   if (ValidateInteger(saved.scale, NULL, True) != 0) 
	strcpy(saved.scale, FACTORY_SCALE);
   temp = ValidateDecimalNumber(saved.left, NULL, False);
   if (temp != 0)		/* okay if 0 */
	strcpy(saved.left, FACTORY_LEFT);
   temp = ValidateDecimalNumber(saved.top, NULL, False);
   if (temp != 0)		/* okay if 0 */
	strcpy(saved.top, FACTORY_TOP);
   if (ValidateDecimalNumber(saved.width, NULL, True) != 0) 
	strcpy(saved.width, FACTORY_WIDTH);
   if (ValidateDecimalNumber(saved.height, NULL, True) != 0) 
	strcpy(saved.height, FACTORY_HEIGHT);

/* (4) put 12 of 13 fields into printcmd */
/* NOT NEEDED: filename field */

   printcmd[0]='\0';
   strcat(printcmd, " xpr");

   /* Output format */
   strcat(printcmd, " -d\"");
   strcat(printcmd, saved.output);
   strcat(printcmd, "\"");

   /* Orientation */
   if (strcmp(saved.orientation,PROP_LANDSCAPE)==0)
	strcat(printcmd, " -l");
   else
	strcat(printcmd, " -p");

   /* Reverse video */
   if (strcmp(saved.reverse_video,PROP_ON)==0)
	strcat(printcmd, " -r");

   /* Header */
   if (saved.header[0] != '\0') {
	strcat(printcmd, " -h'");
	strcat(printcmd, saved.header);
	strcat(printcmd, "'");
   }

   /* Footer */
   if (saved.footer[0] != '\0') {
	strcat(printcmd, " -t'");
	strcat(printcmd, saved.footer);
	strcat(printcmd, "'");
   }

   /* Pages */
   if (saved.pages[0] != '\0') {
   	strcat(printcmd, " -s");
   	strcat(printcmd, saved.pages);
   }

   /* Scale */
   if (saved.scale[0] != '\0') {
   	strcat(printcmd, " -S");
   	strcat(printcmd, saved.scale);
   }

   /* Left offset */
   if (saved.left[0] != '\0') {
   	strcat(printcmd, " -L");
   	strcat(printcmd, saved.left);
   }

   /* Top offset */
   if (saved.top[0] != '\0') {
   	strcat(printcmd, " -T");
   	strcat(printcmd, saved.top);
   }

   /* Max width */
   if (saved.width[0] != '\0') {
   	strcat(printcmd, " -W");
   	strcat(printcmd, saved.width);
   }

   /* Max height */
   if (saved.height[0] != '\0') {
   	strcat(printcmd, " -H");
   	strcat(printcmd, saved.height);
   }

   strcat(printcmd, " ");
   strcat(printcmd, saved.print);

/* (5) setting Command Popups' textfield filenames */
   XtSetArg(args[0], XtNstring, saved.filename);
   XtSetValues(open_pop_textf,    args, 1);
   XtSetValues(save_pop_textf,    args, 1);
   XtSetValues(printf_pop_textf1, args, 1);

/*
 *  Set global variable to be used in dump.c to determine pixmap format
 */
    strcpy(pixmapformat, saved.pixformat);

/* (6) create property sheet using the fields */
        *pop = XtVaCreatePopupShell("PopupShell", popupWindowShellWidgetClass,
			parent,
        		XtNapply, (XtArgVal) applycalls,
        		XtNreset, (XtArgVal) resetcalls,
			XtNsetDefaults, (XtArgVal) defaultcalls,
        		XtNresetFactory, (XtArgVal) factorycalls,
        		XtNverify, (XtArgVal) verifycalls,
			XtNtitle, (XtArgVal) OlGetMessage( display, NULL, 0,
					OleNtitle, OleTlabelProperties,
					OleCOlClientOlpsMsgs,
					OleMtitle_labelProperties,
					(XrmDatabase)NULL),
			(char *)0);
        cnt = 0;
        XtSetArg(args[cnt], XtNupperControlArea, &pop_upper); cnt++;
        XtSetArg(args[cnt], XtNlowerControlArea, &pop_lower); cnt++;
        XtSetArg(args[cnt], XtNfooterPanel, &pop_footer);            cnt++;
        XtGetValues(*pop, args, cnt);

        cnt = 0;
        XtSetArg(args[cnt], XtNtraversalManager, True); cnt++;
	XtSetValues(pop_upper, args, cnt);

        cnt = 0;
        XtSetArg(args[cnt], XtNlayoutType, OL_FIXEDCOLS);         cnt++;
        XtSetArg(args[cnt], XtNmeasure, 1);                       cnt++;
	XtSetArg(args[cnt], XtNalignCaptions, True);              cnt++;
	XtSetArg(args[cnt], XtNborderWidth, 0); cnt++;
        pop_upper1 = XtCreateManagedWidget(  "control",
                                                controlAreaWidgetClass,
                                                pop_upper, args, cnt);
        cnt = 0;
        XtSetArg(args[cnt], XtNlayoutType, OL_FIXEDCOLS);         cnt++;
        XtSetArg(args[cnt], XtNmeasure, 2);                       cnt++;
	XtSetArg(args[cnt], XtNalignCaptions, True);              cnt++;
	XtSetArg(args[cnt], XtNborderWidth, 0); cnt++;
        pop_upper2 = XtCreateManagedWidget(  "control",
                                                controlAreaWidgetClass,
                                                pop_upper, args, cnt);
	caption= XtVaCreateManagedWidget("Default Pathname", captionWidgetClass,
		pop_upper1,
		XtNborderWidth, (XtArgVal)0,
		XtNlabel, GET_CAPTION_LABEL(OleTlabelDefaultPath,
					OleMcaption_labelDefaultPath),
		XtNmnemonic, GET_CAPTION_MNEM(OleTlabelDefaultPath,
					OleMmnemonic_labelDefaultPath),
		(char *) 0);
					
		
				
        cnt = 0;
        XtSetArg(args[cnt], XtNtraversalOn, True);		cnt++;
        XtSetArg(args[cnt], XtNstring, saved.filename);		cnt++;
        XtSetArg(args[cnt], XtNwidth, 
		OlMMToPixel(OL_HORIZONTAL, MM1));              	cnt++;
        p_filename_textf = XtCreateManagedWidget(	"textfield",
                                        	textFieldWidgetClass,
                                        	caption, args, cnt);

        caption = XtVaCreateManagedWidget("Output Format", captionWidgetClass,
			pop_upper2,
			XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelOutputFormat,
					OleMcaption_labelOutputFormat),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelOutputFormat,
					OleMmnemonic_labelOutputFormat),
			(char *)0);

			
        cnt = 0;
        XtSetArg(args[cnt], XtNtraversalOn, True);		cnt++;
        XtSetArg(args[cnt], XtNstring, saved.output);		cnt++;
        XtSetArg(args[cnt], XtNwidth,                                          
                OlMMToPixel(OL_HORIZONTAL, MM2));              	cnt++;
        p_output_textf = XtCreateManagedWidget(	"textfield",
                                        	textFieldWidgetClass,
                                        	caption, args, cnt);
        caption = XtVaCreateManagedWidget("Print Command", captionWidgetClass,
			pop_upper2,
			XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelPrintCmd,
					OleMcaption_labelPrintCmd),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelPrintCmd,
					OleMmnemonic_labelPrintCmd),
			(char *)0);
        cnt = 0;
        XtSetArg(args[cnt], XtNtraversalOn, True);		cnt++;
        XtSetArg(args[cnt], XtNstring, saved.print);		cnt++;
        XtSetArg(args[cnt], XtNwidth,
                OlMMToPixel(OL_HORIZONTAL, MM2));              	cnt++;
        p_print_textf = XtCreateManagedWidget(	"textfield",
                                        	textFieldWidgetClass,
                                        	caption, args, cnt);
        caption = XtVaCreateManagedWidget("Orientation", captionWidgetClass,
			pop_upper2,
			XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelOrientation,
					OleMcaption_labelOrientation),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelOrientation,
					OleMmnemonic_labelOrientation),
			(char *)0);

	/* Adjust exclusives settings */
	/* Set the exclusives */
   	if (strcmp(saved.orientation,PROP_PORTRAIT)==0)
		/* highlight (press) portrait */
		excl_orientation[0].is_set = (XtArgVal)True;
	else
		/* highlight (press) landscape */
		excl_orientation[1].is_set = (XtArgVal)True;

   	if (strcmp(saved.pixformat,PROP_ZFORMAT)==0)
		/* highlight (press) zpixmap */
		excl_pixformat[0].is_set = (XtArgVal)True;
	else
		/* highlight (press) xypixmap */
		excl_pixformat[1].is_set = (XtArgVal)True;

	if (strcmp(saved.reverse_video,PROP_OFF)==0) {
		/* Turn "on" the Off button */
		excl_revvideo[0].is_set = (XtArgVal)True;
	}
	else
		/* Turn "on" the On button */
		excl_revvideo[1].is_set = (XtArgVal)True;

	excl_orientation[0].e_label = (XtArgVal)OlGetMessage(display, NULL, 0,
					OleNbutton, OleTlabelPortrait,
					OleCOlClientOlpsMsgs,
					OleMbutton_labelPortrait,
					(XrmDatabase)NULL);
	excl_orientation[1].e_label = (XtArgVal)OlGetMessage(display, NULL, 0,
					OleNbutton, OleTlabelLandscape,
					OleCOlClientOlpsMsgs,
					OleMbutton_labelLandscape,
					(XrmDatabase)NULL);
	flat_orient = XtVaCreateManagedWidget("orientation",
				flatButtonsWidgetClass,caption,
				XtNlayoutType, OL_FIXEDROWS,
				XtNmeasure, 1,
				XtNbuttonType, OL_RECT_BTN,
				XtNexclusives, True,
				XtNitems, excl_orientation,
				XtNnumItems, XtNumber(excl_orientation),
				XtNitemFields, excl_fields,
				XtNnumItemFields, XtNumber(excl_fields),
				(char *)0);

	caption = XtVaCreateManagedWidget("PixFormat", captionWidgetClass,
                        pop_upper2,
                        XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelPixformat,
                                        OleMcaption_labelPixformat),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelPixformat,
                                        OleMmnemonic_labelPixformat),
			(char *)0);

				
	excl_pixformat[0].e_label = (XtArgVal)OlGetMessage(display, NULL, 0,
					OleNbutton, OleTlabelZpixmap,
					OleCOlClientOlpsMsgs,
					OleMbutton_labelZpixmap,
					(XrmDatabase)NULL);
	excl_pixformat[1].e_label = (XtArgVal)OlGetMessage(display, NULL, 0,
					OleNbutton, OleTlabelXYpixmap,
					OleCOlClientOlpsMsgs,
					OleMbutton_labelXYpixmap,
					(XrmDatabase)NULL);
	flat_pixformat = XtVaCreateManagedWidget("pixformat",
				flatButtonsWidgetClass,caption,
				XtNlayoutType, OL_FIXEDROWS,
				XtNmeasure, 1,
				XtNbuttonType, OL_RECT_BTN,
				XtNexclusives, True,
				XtNitems, excl_pixformat,
				XtNnumItems, XtNumber(excl_pixformat),
				XtNitemFields, excl_fields,
				XtNnumItemFields, XtNumber(excl_fields),
				(char *)0);
				
        caption = XtVaCreateManagedWidget("Reverse Video", captionWidgetClass,
			pop_upper2,
			XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelReverseVideo,
					OleMcaption_labelReverseVideo),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelReverseVideo,
					OleMmnemonic_labelReverseVideo),
			(char *)0);
	excl_revvideo[0].e_label = (XtArgVal)OlGetMessage(display, NULL, 0,
					OleNbutton, OleTlabelOff,
					OleCOlClientOlpsMsgs,
					OleMbutton_labelOff,
					(XrmDatabase)NULL);
	excl_revvideo[1].e_label = (XtArgVal)OlGetMessage(display, NULL, 0,
					OleNbutton, OleTlabelOn,
					OleCOlClientOlpsMsgs,
					OleMbutton_labelOn,
					(XrmDatabase)NULL);
	flat_revvideo = XtVaCreateManagedWidget("revvideo",
				flatButtonsWidgetClass,caption,
				XtNlayoutType, OL_FIXEDROWS,
				XtNmeasure, 1,
				XtNbuttonType, OL_RECT_BTN,
				XtNexclusives, True,
				XtNitems, excl_revvideo,
				XtNnumItems, XtNumber(excl_revvideo),
				XtNitemFields, excl_fields,
				XtNnumItemFields, XtNumber(excl_fields),
				(char *)0);

        caption = XtVaCreateManagedWidget("Header", captionWidgetClass,
			pop_upper2,
			XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelHeader,
					OleMcaption_labelHeader),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelHeader,
					OleMmnemonic_labelHeader),
			(char *)0);
        cnt = 0;
        XtSetArg(args[cnt], XtNtraversalOn, True);		cnt++;
        XtSetArg(args[cnt], XtNstring, saved.header);		cnt++;
        XtSetArg(args[cnt], XtNwidth,
                OlMMToPixel(OL_HORIZONTAL, MM2));              	cnt++;
        p_header_textf = XtCreateManagedWidget(	"textfield",
                                        	textFieldWidgetClass,
                                        	caption, args, cnt);
        caption = XtVaCreateManagedWidget("Footer", captionWidgetClass,
			pop_upper2,
			XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelFooter,
					OleMcaption_labelFooter),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelFooter,
					OleMmnemonic_labelFooter),
			(char *)0);
        cnt = 0;
        XtSetArg(args[cnt], XtNtraversalOn, True);		cnt++;
        XtSetArg(args[cnt], XtNstring, saved.footer);		cnt++;
        XtSetArg(args[cnt], XtNwidth,
                OlMMToPixel(OL_HORIZONTAL, MM2));              	cnt++;
        p_footer_textf = XtCreateManagedWidget(	"textfield",
                                        	textFieldWidgetClass,
                                        	caption, args, cnt);

        caption = XtVaCreateManagedWidget("Pages", captionWidgetClass,
			pop_upper2,
			XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelPages,
					OleMcaption_labelPages),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelPages,
					OleMmnemonic_labelPages),
			(char *)0);
        cnt = 0;
        XtSetArg(args[cnt], XtNtraversalOn, True);		cnt++;
        XtSetArg(args[cnt], XtNstring, saved.pages);		cnt++;
        XtSetArg(args[cnt], XtNwidth,
                OlMMToPixel(OL_HORIZONTAL, MM3));              	cnt++;
        XtSetArg(args[cnt], XtNmaximumSize, 6);			cnt++;
        p_pages_textf = XtCreateManagedWidget(	"textfield",
                                        	textFieldWidgetClass,
                                        	caption, args, cnt);

        caption = XtVaCreateManagedWidget("Scale", captionWidgetClass,
			pop_upper2,
			XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelScale,
					OleMcaption_labelScale),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelScale,
					OleMmnemonic_labelScale),
			(char *)0);
        cnt = 0;
        XtSetArg(args[cnt], XtNtraversalOn, True);		cnt++;
        XtSetArg(args[cnt], XtNstring, saved.scale);		cnt++;
        XtSetArg(args[cnt], XtNwidth,
                OlMMToPixel(OL_HORIZONTAL, MM3));              	cnt++;
        XtSetArg(args[cnt], XtNmaximumSize, 6);			cnt++;
        p_scale_textf = XtCreateManagedWidget(	"textfield",
                                        	textFieldWidgetClass,
                                        	caption, args, cnt);

        caption = XtVaCreateManagedWidget("Left Offset", captionWidgetClass,
			pop_upper2,
			XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelLeftOffset,
					OleMcaption_labelLeftOffset),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelLeftOffset,
					OleMmnemonic_labelLeftOffset),
			(char *)0);
        cnt = 0;
        XtSetArg(args[cnt], XtNtraversalOn, True);		cnt++;
        XtSetArg(args[cnt], XtNstring, saved.left);		cnt++;
        XtSetArg(args[cnt], XtNwidth,
                OlMMToPixel(OL_HORIZONTAL, MM3));              	cnt++;
        XtSetArg(args[cnt], XtNmaximumSize, 6);			cnt++;
        p_left_textf = XtCreateManagedWidget(	"textfield",
                                        	textFieldWidgetClass,
                                        	caption, args, cnt);

        caption = XtVaCreateManagedWidget("Top Offset", captionWidgetClass,
			pop_upper2,
			XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelTopOffset,
					OleMcaption_labelTopOffset),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelTopOffset,
					OleMmnemonic_labelTopOffset),
			(char *)0);
        cnt = 0;
        XtSetArg(args[cnt], XtNtraversalOn, True);		cnt++;
        XtSetArg(args[cnt], XtNstring, saved.top);		cnt++;
        XtSetArg(args[cnt], XtNwidth,
                OlMMToPixel(OL_HORIZONTAL, MM3));              	cnt++;
        XtSetArg(args[cnt], XtNmaximumSize, 6);			cnt++;
        p_top_textf = XtCreateManagedWidget(	"textfield",
                                        	textFieldWidgetClass,
                                        	caption, args, cnt);

        caption = XtVaCreateManagedWidget("Max width", captionWidgetClass,
			pop_upper2,
			XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelMaxWidth,
					OleMcaption_labelMaxWidth),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelMaxWidth,
					OleMmnemonic_labelMaxWidth),
			(char *)0);
        cnt = 0;
        XtSetArg(args[cnt], XtNtraversalOn, True);		cnt++;
        XtSetArg(args[cnt], XtNstring, saved.width);		cnt++;
        XtSetArg(args[cnt], XtNwidth,
                OlMMToPixel(OL_HORIZONTAL, MM3));              	cnt++;
        XtSetArg(args[cnt], XtNmaximumSize, 6);			cnt++;
        p_width_textf = XtCreateManagedWidget(	"textfield",
                                        	textFieldWidgetClass,
                                        	caption, args, cnt);

        caption = XtVaCreateManagedWidget("Max height", captionWidgetClass,
			pop_upper2,
			XtNborderWidth, (XtArgVal)0,
			XtNlabel, GET_CAPTION_LABEL(OleTlabelMaxHt,
					OleMcaption_labelMaxHt),
			XtNmnemonic, GET_CAPTION_MNEM(OleTlabelMaxHt,
					OleMmnemonic_labelMaxHt),
			(char *)0);
        cnt = 0;
        XtSetArg(args[cnt], XtNtraversalOn, True);		cnt++;
        XtSetArg(args[cnt], XtNstring, saved.height);		cnt++;
        XtSetArg(args[cnt], XtNwidth,
                OlMMToPixel(OL_HORIZONTAL, MM3));              	cnt++;
        XtSetArg(args[cnt], XtNmaximumSize, 6);			cnt++;
        p_height_textf = XtCreateManagedWidget(	"textfield",
                                        	textFieldWidgetClass,
                                        	caption, args, cnt);
        cnt = 0;
        XtSetArg(args[cnt], XtNborderWidth, 0); cnt++;
        XtSetArg(args[cnt], XtNgravity, WestGravity); cnt++;
        pop_footer_text = XtCreateManagedWidget(  "footer", 
				staticTextWidgetClass,
                                pop_footer, args, cnt);

}


/* ARGSUSED */
static void
Apply(w,client_data,call_data)
Widget w;
XtPointer client_data, call_data;
{
	SetFooterText(pop_footer_text, "");
	f_popdown = True;
	if (GetAndValidate(&temp) != 0)
		return;
	CopyProps( &saved, &temp);
	strcpy(printcmd, tempcmd);
	strcpy(pixmapformat, saved.pixformat);
}

extern void InitializeResourceBuffer();
extern int AppendToResourceBuffer();
extern void SendResourceBuffer();
	
/* ARGSUSED */
static void
SetDefaults(w,client_data,call_data)
Widget w;
XtPointer client_data, call_data;
{
	SetFooterText(pop_footer_text, "");
	f_popdown = True;
	if (GetAndValidate(&temp) != 0)
		return;
	CopyProps( &saved, &temp);
	strcpy(printcmd, tempcmd);

	InitializeResourceBuffer();
	AppendToResourceBuffer(appname,".ps_filename",	saved.filename);
	AppendToResourceBuffer(appname,".ps_output",	saved.output);
	AppendToResourceBuffer(appname,".ps_print",	saved.print);
	/*
	 * For header and footer only -
	 * if blank, save with string "blank"
	 */
	if (saved.header[0] == '\0')
		AppendToResourceBuffer(appname,".ps_header",	"blank");
	else
		AppendToResourceBuffer(appname,".ps_header",	saved.header);
	if (saved.footer[0] == '\0')
		AppendToResourceBuffer(appname,".ps_footer",	"blank");
	else
		AppendToResourceBuffer(appname,".ps_footer",	saved.footer);
	if (saved.pages[0] == '\0') 
		AppendToResourceBuffer(appname,".ps_pages",	"blank");
	else
		AppendToResourceBuffer(appname,".ps_pages",	saved.pages);
	if (saved.scale[0] == '\0') 
		AppendToResourceBuffer(appname,".ps_scale",	"blank");
	else
		AppendToResourceBuffer(appname,".ps_scale",	saved.scale);
	if (saved.left[0] == '\0') 
		AppendToResourceBuffer(appname,".ps_left",	"blank");
	else
		AppendToResourceBuffer(appname,".ps_left",	saved.left);
	if (saved.top[0] == '\0') 
		AppendToResourceBuffer(appname,".ps_top",	"blank");
	else
		AppendToResourceBuffer(appname,".ps_top",	saved.top);
	if (saved.width[0] == '\0') 
		AppendToResourceBuffer(appname,".ps_width",	"blank");
	else
		AppendToResourceBuffer(appname,".ps_width",	saved.width);
	if (saved.height[0] == '\0') 
		AppendToResourceBuffer(appname,".ps_height",	"blank");
	else
		AppendToResourceBuffer(appname,".ps_height",	saved.height);
   	if (strcmp(saved.orientation,PROP_PORTRAIT)==0)
		AppendToResourceBuffer(appname,".ps_orientation",PROP_PORTRAIT);
	else
		AppendToResourceBuffer(appname,".ps_orientation",PROP_LANDSCAPE);
 	if (strcmp(saved.pixformat,PROP_ZFORMAT)==0)
		AppendToResourceBuffer(appname,".ps_pixformat",PROP_ZFORMAT);
	else
            	AppendToResourceBuffer(appname,".ps_pixformat",PROP_XYFORMAT);
   	if (strcmp(saved.reverse_video,PROP_OFF)==0)
		AppendToResourceBuffer(appname,".ps_reverse_video",PROP_OFF);
	else
		AppendToResourceBuffer(appname,".ps_reverse_video",PROP_ON);
	SendResourceBuffer(dpy, XtWindow(toplevel), 0, appname);

}

/* ARGSUSED */
static void
Reset(w,client_data,call_data)
Widget w;
XtPointer client_data, call_data;
{
	SetFooterText(pop_footer_text, "");
	SetProperties(&saved);
	f_popdown = False;
}

/* ARGSUSED */
static void
ResetFactory(w,client_data,call_data)
Widget w;
XtPointer client_data, call_data;
{
	SetFooterText(pop_footer_text, "");
	SetProperties(&factory);
	f_popdown = False;
}

/* called by Property Sheet when it sees if I want to popdown or not */
/* ARGSUSED */
static void
Verify(w,client_data,call_data)
Widget w;
XtPointer client_data, call_data;
{
	Boolean *flag = call_data;

	if (f_popdown == False)
		*flag = False;
}

static void
SetProperties(p)
struct properties *p;
{
	int change;
        XtSetArg(args[0], XtNstring, p->filename); 
        XtSetValues(p_filename_textf, args, 1);

        XtSetArg(args[0], XtNstring, p->output);
        XtSetValues(p_output_textf, args, 1);

        XtSetArg(args[0], XtNstring, p->print);
        XtSetValues(p_print_textf, args, 1);

        XtSetArg(args[0], XtNstring, p->header);
        XtSetValues(p_header_textf, args, 1);

        XtSetArg(args[0], XtNstring, p->footer);
        XtSetValues(p_footer_textf, args, 1);

        XtSetArg(args[0], XtNstring, p->pages);
        XtSetValues(p_pages_textf, args, 1);

        XtSetArg(args[0], XtNstring, p->scale);
        XtSetValues(p_scale_textf, args, 1);

        XtSetArg(args[0], XtNstring, p->left);
        XtSetValues(p_left_textf, args, 1);

        XtSetArg(args[0], XtNstring, p->top);
        XtSetValues(p_top_textf, args, 1);

        XtSetArg(args[0], XtNstring, p->width);
        XtSetValues(p_width_textf, args, 1);

        XtSetArg(args[0], XtNstring, p->height);
        XtSetValues(p_height_textf, args, 1);

	/* see which exclusives to set to highlight */
	change = 0;
   	if (strcmp(p->orientation,PROP_PORTRAIT)==0) {
		/* turn on portrait */
		if (! (Bool)excl_orientation[0].is_set) {
			/* portrait = set, but it isn't now, so change */
			change++;
			excl_orientation[0].is_set = (XtArgVal)True;
			excl_orientation[1].is_set = (XtArgVal)False;
		}
	}
	else { /* turn on landscape */
		if (! (Bool)excl_orientation[1].is_set) {
			excl_orientation[0].is_set = (XtArgVal)False;
			excl_orientation[1].is_set = (XtArgVal)True;
			change++;
		}
	}
	if (change) {
		change = 0;
		OlVaFlatSetValues(flat_orient, 0,
			XtNset, excl_orientation[0].is_set,
			(char *)0);
		OlVaFlatSetValues(flat_orient, 1,
			XtNset, excl_orientation[1].is_set,
			(char *)0);
		/* can't do this???
		XtVaSetValues(flat_orient,
			XtNitems, excl_orientation,
			XtNnumItems, XtNumber(excl_orientation),
			XtNitemFields, excl_fields,
			XtNnumItemFields, XtNumber(excl_fields),
			(char *)0);
		 */
	}
	/* see which exclusives to set to highlight */
	change = 0;
   	if (strcmp(p->pixformat,PROP_ZFORMAT)==0) {
		/* turn on zpixmap */
		if (! (Bool)excl_pixformat[0].is_set) {
			/* zpixformat = set, but it isn't now, so change */
			change++;
			excl_pixformat[0].is_set = (XtArgVal)True;
			excl_pixformat[1].is_set = (XtArgVal)False;
		}
	}
	else { /* turn on landscape */
		if (! (Bool)excl_pixformat[1].is_set) {
			excl_pixformat[0].is_set = (XtArgVal)False;
			excl_pixformat[1].is_set = (XtArgVal)True;
			change++;
		}
	}
	if (change) {
		change = 0;
		OlVaFlatSetValues(flat_pixformat, 0,
			XtNset, excl_pixformat[0].is_set,
			(char *)0);
		OlVaFlatSetValues(flat_pixformat, 1,
			XtNset, excl_pixformat[1].is_set,
			(char *)0);
		/* can't do this???
		XtVaSetValues(flat_orient,
			XtNitems, excl_orientation,
			XtNnumItems, XtNumber(excl_orientation),
			XtNitemFields, excl_fields,
			XtNnumItemFields, XtNumber(excl_fields),
			(char *)0);
		 */
	}
   	if (strcmp(p->reverse_video,PROP_OFF)==0) {
		/* turn on Off button */
		if (! excl_revvideo[0].is_set) {
			/* portrait = set, but it isn't now, so change */
			change++;
			excl_revvideo[0].is_set = (XtArgVal)True;
			excl_revvideo[1].is_set = (XtArgVal)False;
		}
	}
	else { /* turn on On */
		if (! excl_revvideo[1].is_set) {
			excl_revvideo[0].is_set = (XtArgVal)False;
			excl_revvideo[1].is_set = (XtArgVal)True;
			change++;
		}
	}
	if (change) {
		OlVaFlatSetValues(flat_revvideo, 0,
			XtNset, excl_revvideo[0].is_set,
			(char *)0);
		OlVaFlatSetValues(flat_revvideo, 1,
			XtNset, excl_revvideo[1].is_set,
			(char *)0);
		/* Again - can't do this??
		XtVaSetValues(flat_revvideo,
			XtNitems, excl_revvideo,
			XtNnumItems, XtNumber(excl_revvideo),
			XtNitemFields, excl_fields,
			XtNnumItemFields, XtNumber(excl_fields),
			(char *)0);
			*/
	}
}

/* Returns 0 if all fields appear ok, else -1 */
static int
GetAndValidate(prop)
struct properties *prop;
{
char	*pstring;
Boolean	bool;

	/* validation and setting of fields */
	tempcmd[0]='\0';
	strcat(tempcmd, " xpr");

	/* Default filename */
	XtSetArg(args[0], XtNstring, &pstring);
	XtGetValues (p_filename_textf, args, 1);
	if (pstring[0] == '\0') {
	   SetFooterText(	pop_footer_text, 
				OlGetMessage( XtDisplay(toplevel),
						NULL,
						0,
						OleNbadFilename,
						OleTblankDefault,
						OleCOlClientOlpsMsgs,
						OleMbadFilename_blankDefault,
						(XrmDatabase)NULL));
	   _OlBeepDisplay(toplevel, 1);
	   f_popdown = False;
	   return (-1);
	}
	strcpy(prop->filename, pstring);
	Free1(pstring, "p_filename_textf");

	/* Output format */
	XtSetArg(args[0], XtNstring, &pstring);
	XtGetValues (p_output_textf, args, 1);
	if (pstring[0] == '\0') {
	   SetFooterText(pop_footer_text, OlGetMessage( XtDisplay(toplevel),
							NULL,
							0,
							OleNbadFormat,
							OleTblankValue,
							OleCOlClientOlpsMsgs,
							OleMbadFormat_blankValue,
							(XrmDatabase)NULL));
	   _OlBeepDisplay(toplevel, 1);
	   f_popdown = False;
	   return (-1);
	}
	strcpy(prop->output, pstring);
	strcat(tempcmd, " -d\"");
	strcat(tempcmd, prop->output);
	strcat(tempcmd, "\"");
	Free1(pstring, "p_output_textf");

	OlVaFlatGetValues(flat_orient, 0, XtNset, &bool,
					(char *)0);
	if (bool == True) {
	   strcpy(prop->orientation, PROP_PORTRAIT);
	   strcat(tempcmd, " -p");
	} else {
	   strcpy(prop->orientation, PROP_LANDSCAPE);
	   strcat(tempcmd, " -l");
	}
	/* Save values */
	excl_orientation[1].is_set =
		((excl_orientation[0].is_set = (XtArgVal)bool) == True) ? False : True;

	OlVaFlatGetValues(flat_revvideo, 0, XtNset, &bool,
					(char *)0);
	if (bool == True) {
	   strcpy(prop->reverse_video, PROP_OFF); /* xpr default */
	} else {
	   strcpy(prop->reverse_video, PROP_ON);
	   strcat(tempcmd, " -r");
	}
	/* Save values */
	excl_revvideo[1].is_set = ( (excl_revvideo[0].is_set = bool) == True) ?
					False : True;

	/* Header */
	XtSetArg(args[0], XtNstring, &pstring);
	XtGetValues (p_header_textf, args, 1);
	strcpy(prop->header, pstring);
	if (pstring[0] != '\0') {
	   strcat(tempcmd, " -h'");
	   strcat(tempcmd, pstring);
	   strcat(tempcmd, "'");
	}
	Free1(pstring, "p_header_textf");

	/* Footer */
	XtSetArg(args[0], XtNstring, &pstring);
	XtGetValues (p_footer_textf, args, 1);
	strcpy(prop->footer, pstring);
	if (pstring[0] != '\0') {
	   strcat(tempcmd, " -t'");
	   strcat(tempcmd, pstring);
	   strcat(tempcmd, "'");
	}
	Free1(pstring, "p_footer_textf");

	/* Pages */
	XtSetArg(args[0], XtNstring, &pstring);
	XtGetValues (p_pages_textf, args, 1);
	strcpy(prop->pages, pstring);
	switch (ValidateInteger(pstring, 
				OlGetMessage( XtDisplay(toplevel),
						NULL,
						0,
						OleNbadPages,
						OleTnotPositive,
						OleCOlClientOlpsMsgs,
						OleMbadPages_notPositive,
						(XrmDatabase)NULL),True)) {
  		case 0:
  			strcat(tempcmd, " -s");
  			strcat(tempcmd, pstring);
  			break;
  	  	case 1:
  			return (-1);
  			break;
	 	case 2:	/* do nothing, since blank */
  			break;
	  	case 3:	/* zero not allowed */
  			return (-1);
  			break;
  	}
	Free1(pstring, "p_pages_textf");

	/* Scale */
	XtSetArg(args[0], XtNstring, &pstring);
	XtGetValues (p_scale_textf, args, 1);
	strcpy(prop->scale, pstring);
	switch (ValidateInteger(pstring, 
				OlGetMessage( XtDisplay(toplevel),
						NULL,
						0,
						OleNbadScale,
						OleTnotPositive,
						OleCOlClientOlpsMsgs,
						OleMbadScale_notPositive,
						(XrmDatabase)NULL),True)) {
	          case 0:
			strcat(tempcmd, " -S");
			strcat(tempcmd, pstring);
	                break;
	          case 1:
	                return (-1);
	                break;
		  case 2:	/* do nothing, since blank */
			break;
		  case 3:	/* zero not allowed */
			return (-1);
			break;
        }
	Free1(pstring, "p_scale_textf");

	/* Left offset */
	XtSetArg(args[0], XtNstring, &pstring);
	XtGetValues (p_left_textf, args, 1);
	strcpy(prop->left, pstring);
	switch (ValidateDecimalNumber(pstring, 
				OlGetMessage( XtDisplay(toplevel),
						NULL,
						0,
						OleNbadLeftOffset,
						OleTnotPositive,
						OleCOlClientOlpsMsgs,
						OleMbadLeftOffset_notPositive,
						(XrmDatabase)NULL),False)) {
	          case 0:
		  		/* zero allowed */
			strcat(tempcmd, " -L");
			strcat(tempcmd, pstring);
	                break;
	          case 1:
	                return (-1);
	                break;
		  case 2:	/* do nothing, since blank */
			break;
        }
	Free1(pstring, "p_left_textf");

	/* Top offset */
	XtSetArg(args[0], XtNstring, &pstring);
	XtGetValues (p_top_textf, args, 1);
	strcpy(prop->top, pstring);
	switch (ValidateDecimalNumber(pstring, 
				OlGetMessage( XtDisplay(toplevel),
						NULL,
						0,
						OleNbadTopOffset,
						OleTnotPositive,
						OleCOlClientOlpsMsgs,
						OleMbadTopOffset_notPositive,
						(XrmDatabase)NULL),False)) {
	          case 0:
			strcat(tempcmd, " -T");
			strcat(tempcmd, pstring);
	                break;
	          case 1:
	                return (-1);
	                break;
		  case 2:	/* do nothing, since blank */
			break;
        }
	Free1(pstring, "p_top_textf");

	/* Max width */
	XtSetArg(args[0], XtNstring, &pstring);
	XtGetValues (p_width_textf, args, 1);
	strcpy(prop->width, pstring);
	switch (ValidateDecimalNumber(pstring, 
				OlGetMessage( XtDisplay(toplevel),
						NULL,
						0,
						OleNbadFieldWidth,
						OleTnotPositive,
						OleCOlClientOlpsMsgs,
						OleMbadFieldWidth_notPositive,
						(XrmDatabase)NULL),True)) {
	          case 0:
			strcat(tempcmd, " -W");
			strcat(tempcmd, pstring);
	                break;
	          case 1:
	                return (-1);
	                break;
		  case 2:	/* do nothing, since blank */
			break;
		  case 3:	/* zero not allowed */
			return (-1);
			break;
        }
	Free1(pstring, "p_width_textf");

	/* Max height */
	XtSetArg(args[0], XtNstring, &pstring);
	XtGetValues (p_height_textf, args, 1);
	strcpy(prop->height, pstring);
	switch (ValidateDecimalNumber(pstring, 
				OlGetMessage( XtDisplay(toplevel),
						NULL,
						0,
						OleNbadFieldHeight,
						OleTnotPositive,
						OleCOlClientOlpsMsgs,
						OleMbadFieldHeight_notPositive,
						(XrmDatabase)NULL),True)) {
	          case 0:
			strcat(tempcmd, " -H");
			strcat(tempcmd, pstring);
	                break;
	          case 1:
	                return (-1);
	                break;
		  case 2:	/* do nothing, since blank */
			break;
		  case 3:	/* zero not allowed */
			return (-1);
			break;
        }
	Free1(pstring, "p_height_textf");

	/* Print command */
	XtSetArg(args[0], XtNstring, &pstring);
	XtGetValues (p_print_textf, args, 1);
	strcpy(prop->print, pstring);
	if (pstring[0] == '\0') {
	   SetFooterText(pop_footer_text,
				OlGetMessage( XtDisplay(toplevel),
						NULL,
						0,
						OleNbadCommand,
						OleTblankValue,
						OleCOlClientOlpsMsgs,
						OleMbadCommand_blankValue,
						(XrmDatabase)NULL));
	   _OlBeepDisplay(toplevel, 1);
	   f_popdown = False;
	   return (-1);
	}
	strcat(tempcmd, " ");
	strcat(tempcmd, pstring);
	Free1(pstring, "p_print_textf");

	/* Since everything is fine, set the Command Popups' textfields */
	XtSetArg(args[0], XtNstring, prop->filename);
       	XtSetValues(open_pop_textf,    args, 1);
       	XtSetValues(save_pop_textf,    args, 1);
       	XtSetValues(printf_pop_textf1, args, 1);
	return (0);
}

/* 
 * Returns 0 if string is valid number,
 * 	   1 if string is not valid number
 * 	   2 if string is blank,
 * 	   3 if string is numeral zero,
 */
static int
ValidateInteger(p, field, nozero)
char	*p, *field;
int nozero;
{
int	i,x;
char 	msg[TEXTF_WIDTH_1];

msg[0] = '\0';
i=0;
if (p[i]=='\0'){
	return (2);
}

/* Make sure each character is a digit, before conversion */
while (p[i]!='\0'){
	if (!isdigit(p[i])) {
	   if (field != NULL) {  /* check when called by CreateProperty */
		strcat(msg, field);
	   	SetFooterText(pop_footer_text, msg);
	   	_OlBeepDisplay(toplevel, 1);
	   	f_popdown = False;
 	   }
	   return (1);
	}
	i++;
}
if (nozero == True) {	/* if care about non zeros */
	x = atoi(p); 	/* take out later */
	if (x == 0){
	   if (field != NULL) {  /* check when called by CreateProperty */
		strcat(msg, field);
	   	SetFooterText(pop_footer_text, msg);
	   	_OlBeepDisplay(toplevel, 1);
	   	f_popdown = False;
 	   }
 	   return(3);
	}
}
return(0);
}


/* 
 * Returns 0 if string is valid decimal number,
 * 	   1 if string is not valid number
 * 	   2 if string is blank,
 * 	   3 if string is numeral zero,
 */
static int
ValidateDecimalNumber(p, field, nozero)
char	*p, *field;
int	nozero;
{
int	i, f_once=False;
char 	msg[TEXTF_WIDTH_1];
double	x;

msg[0] = '\0';
i=0;
if (p[i]=='\0'){
	return (2);
}

/* Make sure each character is a digit or decimal, before conversion */
while (p[i]!='\0'){
	if (p[i] == '.') { /* check for decimals */
	   /* see if dot has appeared before */
	   if (f_once == True) {
	   	if (field != NULL) { /* check when called by CreateProperty */
			strcat(msg, field);
	   		SetFooterText(pop_footer_text, msg);
	   		_OlBeepDisplay(toplevel, 1);
	   		f_popdown = False;
		}
	   	return (1);
	   }
	   else {
		i++;
	   	f_once = True;
		continue;
	   }
	}
	if (!isdigit(p[i])) {
	   if (field != NULL) {  /* check when called by CreateProperty */
		strcat(msg, field);
	   	SetFooterText(pop_footer_text, msg);
	   	_OlBeepDisplay(toplevel, 1);
	   	f_popdown = False;
 	   }
	   return (1);
	}
	i++;
}
if (nozero == True) {	/* if care about non zeros */
	x = atof(p); 
	if (x == 0){
	   if (field != NULL) {  /* check when called by CreateProperty */
		strcat(msg, field);
	   	SetFooterText(pop_footer_text, msg);
	   	_OlBeepDisplay(toplevel, 1);
	   	f_popdown = False;
 	   }
 	   return(3);
	}
}
return(0);
}

static void
CopyProps(to, from)
struct properties *to, *from;
{
	strcpy(to->filename, 	from->filename);
	strcpy(to->output, 	from->output);
	strcpy(to->print, 	from->print);
	strcpy(to->orientation, 	from->orientation);
	strcpy(to->pixformat,	from->pixformat);
	strcpy(to->reverse_video, 	from->reverse_video);
	strcpy(to->header, 	from->header);
	strcpy(to->footer, 	from->footer);
	strcpy(to->pages, 	from->pages);
	strcpy(to->scale, 	from->scale);
	strcpy(to->left, 	from->left);
	strcpy(to->top, 	from->top);
	strcpy(to->width, 	from->width);
	strcpy(to->height, 	from->height);

}

