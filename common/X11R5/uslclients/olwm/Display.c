/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)olwm:Display.c	1.52"
#endif

/*
 *************************************************************************
 *
 * Description:
 *	This file contains the code for refreshing the window manager's
 *	decorations.
 *
 ******************************file*header********************************
 */

#include <stdio.h>
#include <X11/IntrinsicP.h>
#include <Xol/OpenLookP.h>
#include <Xol/OlgP.h>
#include <Xol/VendorI.h>
#include <wm.h>
#include <WMStepP.h>
#include <Extern.h>

/* High(er) resolution default icon pixmap (VGA) */
/* Now, one icon for all screens, 48 pixels */
#include <deficon.xpm>
#include <deficon.mask>


/*
 *************************************************************************
 *
 * Forward Procedure declarations
 *
 **************************forward*declarations***************************
 */

static void	CompileMetrics OL_ARGS((Screen *, int));

extern void	CreateDefaultIconMask OL_NO_ARGS();
#if defined(CHISLED_BORDER)
static void	DrawEdges OL_ARGS((WMStepWidget));
#endif

extern void	CreateGC OL_ARGS((WMStepWidget, WMStepPart*, int));
extern void	DisplayWM OL_ARGS((WMStepWidget, XRectangle *));
extern void	DrawBN OL_ARGS((WMStepWidget));
static void	DrawHeaderGlyph OL_ARGS((WMStepWidget,Display *, Window,
					GC, WMStepPart *, unsigned long));
extern void	EraseBN OL_ARGS((WMStepWidget));
extern void	FlipPushpin OL_ARGS((WMStepWidget));
extern void	FlipMenuMark OL_ARGS((WMStepWidget));
extern WMMetrics *	GetMetrics OL_ARGS((WMStepWidget));
static void	CalcRects OL_ARGS((WMStepWidget, WMMetrics *, int,
				XRectangle *, int *, XRectangle *, int *));
extern void	DrawStreaks OL_ARGS((WMStepWidget, int, int, int, int, int,
					int, int, int));
extern XRectangle HeaderRect OL_ARGS((WMStepWidget));

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

/* These values are in pixels */

#define	MotifNROuterShadowThickness 2	/* outer border shadow if no Resize
					 * corners
					 */
#define	MotifROuterShadowThickness 1	/* outer border shadow (w/resize
					 * corners)
					 */
#define MotifBorderInnerShadowThickness 1  /* the inner shadow that closes
					    * off the outer border, making it
					    * look like a real border!
					    */

static WMMetrics builtin_wmmetrics[] =
   {
  /*                          ALL MEASUREMENTS IN TENTHS OF POINTS
   *                                              ====== == ======
   * scale
   * resolution
   * CornerX
   * CornerY
   * Cornerx
   * Cornery
   * LineWidX
   * LineWidY
   * linewidx
   * Linewidy
   * Gapx
   * Gapy
   * Bannerht
   * hgap1
   * linewid
   * hgap2
   * offset (pushpin/menu mark)  - menu mark for motif only
   * baseline
   * markwid
   * selectwidth (horizontal only)
   * min width between menu button and title
   * compiled? (a char, telling us if the set was compiled)
   * As a start, put in for Motif mode:
   *      motifButtonWidth (tens of pts) - for all 3 buttons, AND the title.
   * 		For the title, it is the minimum width.
   *      motifButtonHeight (tens of pts) - for all 3 buttons
   *      motifHResizeBorderWidth (tens) - horizontal border thickness if has
   *		resize corners
   *      motifVResizeBorderWidth (tens) - vertical border thickness if has
   *		resize corners
   *      motifHNResizeBorderWidth (tens) - Horizontal border thickness if
   *		 NO resize corners
   *      motifVNResizeBorderWidth (tens) - Vertical border thickness if
   *		 NO resize corners
   *
   *      
   *  - with no changes yet to the border...
   * In sets of:
   *
   * base window 
   * icon window
   * nohd window
   *
   */
/* mlp - for no-header windows, make gapx = gapy = 0 (the 12th item) */

   { 10, 75, 100, 100, 40, 40, 20, 20, 10, 10, 30, 30, 200, 10, 10, 10, 116,  70, 320, 8, 80, 'N', 185, 185, 110, 110,65, 65},
   { 10, 75,  00,  00, 00, 00, 20, 20, 00, 00, 30, 30, 200, 10, 10, 10, 100,  70, 320, 10, 80, 'N' , 185, 185, 110, 110, 65, 65},
   { 10, 75,  00,  00, 00, 00, 20, 20, 00, 00, 00, 00,  00, 10, 10, 10, 100,  70, 320, 8, 80, 'N' , 185, 185, 110, 110, 65, 65},

   { 12, 75, 110, 110, 40, 40, 20, 20, 10, 10, 30, 30, 220, 10, 10, 10, 140,  70, 320, 10, 90, 'N' , 185, 185, 110,110,65, 65},
   { 12, 75,  00,  00, 00, 00, 20, 20, 00, 00, 30, 30, 220, 10, 10, 10, 120,  70, 320, 10, 90, 'N' , 185, 185, 110, 110, 65, 65},
   { 12, 75,  00,  00, 00, 00, 20, 20, 00, 00, 00, 00,  00, 10, 10, 10, 120,  70, 320, 10, 90, 'N' , 185, 185, 110, 110, 65, 65},

   { 14, 75, 120, 120, 40, 40, 20, 20, 10, 10, 30, 30, 260, 10, 10, 10, 164,  80, 320, 12, 100, 'N' , 185, 185, 110,110,65, 65},
   { 14, 75,  00,  00, 00, 00, 20, 20, 00, 00, 30, 30, 260, 10, 10, 10, 140,  80, 320, 10, 100, 'N' , 185, 185, 110, 110, 65, 65},
   { 14, 75,  00,  00, 00, 00, 20, 20, 00, 00, 00, 00,  00, 10, 10, 10, 140,  80, 320, 12, 100, 'N' , 185, 185, 110, 110, 65, 65},

   { 19, 75, 140, 140, 40, 40, 30, 30, 20, 20, 30, 30, 300, 10, 20, 10, 222, 110, 320, 16, 140, 'N' , 185, 185, 100,100,65, 65},
   { 19, 75,  00,  00, 00, 00, 30, 30, 00, 00, 30, 30, 300, 10, 20, 10, 190, 110, 320, 10, 140, 'N' , 185, 185, 110, 110, 65, 65},
   { 19, 75,  00,  00, 00, 00, 30, 30, 00, 00, 00, 00,  00, 10, 20, 10, 190, 110, 320, 16, 140, 'N' , 185, 185, 110, 110, 65, 65},

   };


WMMetrics * wmmetrics = builtin_wmmetrics;
int         num_wmmetrics = XtNumber(builtin_wmmetrics);


static GC	BlackGC;
static GC	WhiteGC;
static GC	backgroundGC;


/* Macro used by CalcRects() - NWrect is a local variable for an array of
 * Rectangles, and num_NW is the cardinality of the array */
#define NWRECT(X,Y,W,H)	\
   NWrect[num_NW].x = X;	\
   NWrect[num_NW].y = Y;	\
   NWrect[num_NW].width = W;	\
   NWrect[num_NW].height = H;	\
   num_NW++

/* Same as the previous macro..*/
#define SERECT(X,Y,W,H)	\
   SErect[num_SE].x = X;	\
   SErect[num_SE].y = Y;	\
   SErect[num_SE].width = W;	\
   SErect[num_SE].height = H;	\
   num_SE++

#if leave_as_originally_defined
#define WMScreenPointToPixel(d,v,s) \
   (MAX(OlScreenPointToPixel(OL_HORIZONTAL,v,s),\
        OlScreenPointToPixel(OL_VERTICAL,v,s)))

#define ConvertX(s,v)       v = WMScreenPointToPixel(OL_HORIZONTAL, v, s) / 10
#define ConvertY(s,v)       v = WMScreenPointToPixel(OL_VERTICAL, v, s) / 10
#else
			/* Never return a value of zero	*/
#define ConvertX(s,v)  (v = (v == 0 ? v :\
			 (v = (OlScreenPointToPixel(OL_HORIZONTAL, v, s)/10))\
				> 0 ? v : (v=1,v)) )
#define ConvertY(s,v)  (v = (v == 0 ? v : \
			  ( (v = (OlScreenPointToPixel(OL_VERTICAL, v, s)/10)) \
				> 0) ? v : (v=1,v)) )

#endif /* if 0 */

/*
 *************************************************************************
 *
 * Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * CompileMetrics
 *
 ****************************procedure*header*****************************
 */

static void
CompileMetrics OLARGLIST((screen,i))
OLARG(Screen *, screen)
OLGRA(int, i)
{
/*
 * can read in wmmetrics here
 */
	int temp;

/* point wmmetrics to static array of values; this is initialized to values for
 * 4 scales (small, med., large, and Xlarge), three sets of values per scale
 * (for base window, icon window, and windows without headers (?).
 * Note, all values in the array are given in tenths of points (e.g., actual value
 * of entry in points = value * 10).
 */

/* for each entry per scale, convert to its value in Pixels.
 * ConvertX does this with respect to OL_HORIZONTAL (X and width values);
 * ConvertY does this with respect to OL_VERTIVAL (Y and height values).
 * The value returned by Convert[XY](), after calling WMScreenPointToPixel()
 * (which calls OlScreenPointToPixel() ) is divided by 10.
 * For example, at medium scale, the offset in the header for the pushpin (or
 * window menu button) has an initial value of 120 in the static array
 * wmmetrics; this will be converted to pixels and divided by 10.
 */

if ((i == 3) || (i == 4) || i == 5) {
#if	defined(DISCUSS_METRICS)
fprintf(stderr,"CompileMetrics: BEFORE conversion for medium resolution\n");
if(i == 3)
    fprintf(stderr," -- For normal base windows --\n");
else if (i == 4)
    fprintf(stderr," -- for icon windows -- \n");
else if (i == 5)
    fprintf(stderr," -- for no header windows -- \n");
fprintf(stderr,"cornerX %d, cornerx %d, linewidX %d linewidx %d\n",
    wmmetrics[i].cornerX , wmmetrics[i].cornerx,wmmetrics[i].linewidX,
      wmmetrics[i].linewidx);
fprintf(stderr,"gapx %d, offset %d, markwid %d \n",
    wmmetrics[i].gapx , wmmetrics[i].offset , wmmetrics[i].markwid);
fprintf(stderr,"cornery %d, cornerY %d, linewidY %d linewidy %d \n",
   wmmetrics[i].cornerY , wmmetrics[i].cornery , wmmetrics[i].linewidY,
	 wmmetrics[i].linewidy);
fprintf(stderr,"gapy %d, bannerht %d, hgap1 %d hgap2 %d \n",
        wmmetrics[i].gapy , wmmetrics[i].bannerht , wmmetrics[i].hgap1,
	 wmmetrics[i].hgap2);
fprintf(stderr,"linewid %d, baseline %d \n", wmmetrics[i].linewid,
    wmmetrics[i].baseline);
#endif
   }
   ConvertX(screen, wmmetrics[i].cornerX);
   ConvertX(screen, wmmetrics[i].cornerx);
   ConvertX(screen, wmmetrics[i].linewidX);
   ConvertX(screen, wmmetrics[i].linewidx);
   if ( (temp = wmmetrics[i].linewidx) && (wmmetrics[i].linewidX <= temp))
	wmmetrics[i].linewidX = temp + 1;

   ConvertX(screen, wmmetrics[i].gapx);
   ConvertX(screen, wmmetrics[i].offset);
   ConvertX(screen, wmmetrics[i].markwid);

   ConvertY(screen, wmmetrics[i].cornerY);
   ConvertY(screen, wmmetrics[i].cornery);
   ConvertY(screen, wmmetrics[i].linewidY);
   ConvertY(screen, wmmetrics[i].linewidy);
   if ( (temp = wmmetrics[i].linewidy) && (wmmetrics[i].linewidY <= temp))
	wmmetrics[i].linewidY = temp + 1;

   ConvertY(screen, wmmetrics[i].gapy);
   ConvertY(screen, wmmetrics[i].bannerht);
   ConvertX(screen, wmmetrics[i].hgap1);
   ConvertX(screen, wmmetrics[i].hgap2);
   ConvertX(screen, wmmetrics[i].linewid);
   ConvertX(screen, wmmetrics[i].baseline);
   ConvertX(screen, wmmetrics[i].selectwidth);
   ConvertX(screen, wmmetrics[i].mark_title_gap);
   if (currentGUI == OL_MOTIF_GUI) {
	ConvertX(screen, wmmetrics[i].motifButtonWidth);
	ConvertY(screen, wmmetrics[i].motifButtonHeight);
/*
	ConvertX(screen, wmmetrics[i].motifHResizeBorderWidth);
	ConvertY(screen, wmmetrics[i].motifVResizeBorderWidth);
	ConvertX(screen, wmmetrics[i].motifHNResizeBorderWidth);
	ConvertY(screen, wmmetrics[i].motifVNResizeBorderWidth);
 */
	wmmetrics[i].motifHResizeBorderWidth =
	wmmetrics[i].motifVResizeBorderWidth =
		motwmrcs->resizeBorderWidth > 100 ||
			(motwmrcs->resizeBorderWidth < 5  &&
			 motwmrcs->resizeBorderWidth != 10) ? 10 :
		motwmrcs->resizeBorderWidth;
	wmmetrics[i].motifHNResizeBorderWidth =
	wmmetrics[i].motifVNResizeBorderWidth =
		(motwmrcs->frameBorderWidth != 5 &&
			motwmrcs->frameBorderWidth < 3) ||
		motwmrcs->frameBorderWidth > 100 ? 5 :
		motwmrcs->frameBorderWidth;
	
   }
   wmmetrics[i].compiled = 'Y';

if ((i == 3) || (i == 4) || i == 5) {
#if	defined(DISCUSS_METRICS)
fprintf(stderr,"CompileMetrics: AFTER conversion for medium resolution\n");
if(i == 3)
    fprintf(stderr," -- For normal base windows --\n");
else if (i == 4)
    fprintf(stderr," -- for icon windows -- \n");
else if ( i == 5)
    fprintf(stderr," -- for no header windows -- \n");
fprintf(stderr,"cornerX %d, cornerx %d, linewidX %d linewidx %d\n",
    wmmetrics[i].cornerX , wmmetrics[i].cornerx,wmmetrics[i].linewidX,
      wmmetrics[i].linewidx);
fprintf(stderr,"gapx %d, offset %d, markwid %d \n",
    wmmetrics[i].gapx , wmmetrics[i].offset , wmmetrics[i].markwid);
fprintf(stderr,"cornery %d, cornerY %d, linewidY %d linewidy %d \n",
   wmmetrics[i].cornerY , wmmetrics[i].cornery , wmmetrics[i].linewidY,
	 wmmetrics[i].linewidy);
fprintf(stderr,"gapy %d, bannerht %d, hgap1 %d hgap2 %d \n",
        wmmetrics[i].gapy , wmmetrics[i].bannerht , wmmetrics[i].hgap1,
	 wmmetrics[i].hgap2);
fprintf(stderr,"linewid %d, baseline %d \n", wmmetrics[i].linewid,
    wmmetrics[i].baseline);
#endif
   }

} /* end of CompileMetrics */

/*
 *************************************************************************
 * CreateGC
 * - Called from Initialize() (and possibly others)
 * Get GC for wmstep part of wm widget (if non-null, destroy old one
 * first);  get dimensions of pushpin and window menu mark (ppWidth, ppHeight,
 * mmWidth, mmHeight).
 * - ARG destroy appears to be unused.
 ****************************procedure*header*****************************
 */

extern void
CreateGC OLARGLIST((wm, wmstep, destroy))
OLARG(WMStepWidget, wm)
OLARG(WMStepPart *, wmstep)
OLGRA(int, destroy)
{
XGCValues       values;
static int	first_create = 0;

int		doMotGCs = first_create;
values.font               = wmstep-> font-> fid;
values.foreground         = wmstep-> foreground_pixel;
values.background         = wm-> core.background_pixel;
values.subwindow_mode     = IncludeInferiors;

wm->wmstep.saveBackgroundPixel = wm->core.background_pixel;

if (!first_create) {
	first_create = 1;
	backgroundGC = XCreateGC(XtDisplay(wm),RootWindowOfScreen(XtScreen(wm)),
		GCForeground|GCSubwindowMode|GCBackground,&values);
			
}
if (wmstep-> gc) {
   XtReleaseGC((Widget)wm,wmstep-> gc);
   wmstep->gc = NULL;
}

wmstep-> gc = XtGetGC((Widget)wm, 
   (unsigned) GCFont | GCForeground | GCBackground | GCSubwindowMode, &values);

if (destroy) {
	if (wmstep->mdm != NULL) {
		OlgDestroyAttrs(wmstep->mdm);
		wmstep->mdm = NULL;
	}
	if (wmstep->label_mdm != NULL) {
		OlgDestroyAttrs(wmstep->label_mdm);
		wmstep->label_mdm = NULL;
	}
}
#if defined(DISCUSS_COLORS)
	fprintf(stderr,"CreateGC: wmrcs, f.g. = %d b.g. = %d\n",wmrcs->foregroundColor,wmrcs->backgroundColor);
	fprintf(stderr,"\t...And wmstep->f.g.pixel = %d wm->core.b.g. = %d\n",wmstep->foreground_pixel,wm->core.background_pixel);
#endif
wmstep->mdm = OlgCreateAttrs(XtScreen((Widget)wm), wmstep->foreground_pixel,
	(OlgBG *)&(wm->core.background_pixel),
	(Boolean)FALSE, (Dimension)12);
wmstep->label_mdm = OlgCreateAttrs(XtScreen((Widget)wm),
			wmstep->foreground_pixel,
			(OlgBG *)&(wm->core.background_pixel),
			(Boolean)FALSE, (Dimension)12);

OlgSizeAbbrevMenuB(XtScreen(wm), wmstep->mdm, &mmWidth, &mmHeight);
OlgSizePushPin(XtScreen(wm), wmstep->mdm, &ppWidth, &ppHeight);

/* For the moment, these GCs are global, so the initialization will
 * be done only once by the first widget to do this...  We're assuming
 * for now that all mdms are created equal.
 */
if (currentGUI == OL_MOTIF_GUI && doMotGCs == 0) {
	InitMotifDecorGCs((Widget)wm);
} /* Motif */

} /* end of CreateGC */


#define NOCENTER 0
#define FULLCENTER (1 << 0)
#define PARTIALCENTER (1 << 1)

/*
 *************************************************************************
 * DisplayWM
 *
 * - Called from the expose procedure, Redisplay();
 * Also called to redisplay all of decoration set (rect == NULL).
 ****************************procedure*header*****************************
 */

extern void
DisplayWM OLARGLIST((w, rect))
OLARG(WMStepWidget, w)
OLGRA(XRectangle *, rect)
{
Display *    display   = XtDisplay(w);
Screen		*screen = XtScreen((Widget)w);
Window       window    = XtWindow(w);
WMStepPart * wmstep    = &w-> wmstep;
XRectangle   nw_corners[32];
XRectangle   se_corners[32];
int          nw_i      = 0;
int          se_i      = 0;
int	xstarthere,
	ystarthere,
	xendthere,
	image_area;
XRectangle	temp_rect;
XRectangle	icon_rect;	/* pcc */
Pixel	white = WhitePixelOfScreen(screen);
Pixel	black = BlackPixelOfScreen(screen);
GC           nw_gc     = wmstep-> gc;
GC           se_gc     = wmstep-> gc;
GC           text_gc   = wmstep-> gc;
Boolean      iconic    = (wmstep-> size == WMICONICNORMAL || 
                          wmstep-> size == WMICONICFULL);
char		*text = iconic ? wmstep->icon_name : wmstep->window_name;
int	textlen   = strlen(text);
int	textwid;
int	markwidth = (HasPushpin(w) ? ppWidth : 
		(wmstep->decorations & WMHasFullMenu ? mmWidth : 0));
/* Adjust text area of icon, and text area of header, too; once again,
 * Offset() may be wrong.
 */
int	textarea = 0;
/*
int	textarea  = iconic ? (icon_width - LineWidX(w) * 2 - wmstep->metrics->
							selectwidth) :
				CoreW(w) - BorderX(w) - Offset(w) - markwidth -
				2 * wmstep->metrics->mark_title_gap;
 */

				/* Subtract extra BorderX(w) because CoreW()
				 * includes right border/gap;  Offset() already
				 * includes the left border. 
				 */
/* textarea is the area from the right of the window menu mark to the
 * end of the banner on the right
 */
Boolean		truncated = False;
int		centered = NOCENTER;
int		vwidth, hwidth;

OlgTextLbl	labeldata;
Dimension	labelwidth, labelheight;
OlgAttrs	*motlabel_mdm = (OlgAttrs *)NULL,
		*iconlabel_mdm = (OlgAttrs *)NULL;
Widget		targetWidget = IsIconic(w) ?
				 (Widget)(w->wmstep.icon_widget) : (Widget)w;
MotifCompResourceInfo	*mcri;
MotCompAppearanceInfo	*mcai;

if (!XtIsRealized((Widget)w) || w->wmstep.size == WMWITHDRAWN)
   return;

if (rect != NULL)
   if (rect-> width == 0 || rect-> height == 0)
      {
      FPRINTF((stderr,"nothing to draw\n"));
      return;
      }

if ( (!iconic) &&  currentGUI == OL_MOTIF_GUI) {
	/* Motif text area = core width - the outside border width -
	 * the 3 buttons (menu, minimize, maximize) - 4 (one pixel
	 * for the border around the "text button", and one pixel to  buffer
	 * the text from this border)
	 */
		if (Resizable(w)) {
			vwidth = wmstep->metrics->motifVResizeBorderWidth;
			hwidth = wmstep->metrics->motifHResizeBorderWidth;
		}
		else if (wmstep->decorations & WMBorder) {
			vwidth = wmstep->metrics->motifVNResizeBorderWidth;
			hwidth = wmstep->metrics->motifHNResizeBorderWidth;
		}
		else 
			vwidth = hwidth = wmstep->prev.border_width;
	textarea = w->core.width - 2 * hwidth - 
		3 * wmstep->metrics->motifButtonWidth - 4;
	if (!(wmstep->decorations & WMMinimize))
		textarea += wmstep->metrics->motifButtonWidth;
	if (!(wmstep->decorations & WMMaximize))
		textarea += wmstep->metrics->motifButtonWidth;
	if (!(wmstep->decorations & WMMenuButton))
		textarea += wmstep->metrics->motifButtonWidth;
}

if ( (currentGUI == OL_OPENLOOK_GUI) ||
		( (currentGUI == OL_MOTIF_GUI) && !iconic)) {
   if (currentGUI == OL_OPENLOOK_GUI) {

	if (IsIconic(w)) {
		iconlabel_mdm = OlgCreateAttrs(screen, wmrcs->iconForeground,
			(OlgBG *)&(wmrcs->iconForeground), (Boolean)FALSE,
			(Dimension)12);
		labeldata.normalGC = DefaultDepthOfScreen(screen) != 1 ?
					OlgGetFgGC(iconlabel_mdm) :
					OlgGetBg1GC(iconlabel_mdm);
	}
	else
		labeldata.normalGC = wmstep->gc;

	/* This field not used, fill in to stop core dumps */
	labeldata.inverseGC = labeldata.normalGC;

	labeldata.font = (currentGUI == OL_OPENLOOK_GUI && IsIconic(w)) ?
					 wmrcs->iconFont : wmrcs->font;
	labeldata.font_list = ol_font_list;
	if (labeldata.font_list == NULL)
		XSetFont(display, labeldata.normalGC,
				labeldata.font->fid);
   }
   else {
	mcri = (MotifCompResourceInfo *) &(motCompRes[CLIENT_COMP]);
	mcai = (MotCompAppearanceInfo *) &(mcri->compai);
	motlabel_mdm = OlgCreateAttrs(screen,
			wmstep->is_current ? mcai->activeForeground :
				 mcai->foreground,
			wmstep->is_current ?
				(OlgBG *)&(mcai->activeForeground) :
				(OlgBG *)&(mcai->foreground),
			(Boolean)FALSE, (Dimension)12);
	labeldata.normalGC = DefaultDepthOfScreen(screen) != 1 ?
				OlgGetFgGC(motlabel_mdm) :
				OlgGetBg1GC(motlabel_mdm);
	labeldata.inverseGC = labeldata.normalGC;
	labeldata.font = wmrcs->font;
	if (labeldata.font) /* SHOULDN'T BE NON_NULL */
		XSetFont(display, labeldata.normalGC,
				labeldata.font->fid);
	if (mcai->fontList == NULL)
		labeldata.font_list = ol_font_list;
	else
		labeldata.font_list = mcai->fontList;
   }
   labeldata.accelerator = NULL;
   labeldata.mnemonic = NULL;
   labeldata.flags = (unsigned char) NULL;
   labeldata.justification = TL_CENTER_JUSTIFY;

  labeldata.label = text;

  if (currentGUI == OL_OPENLOOK_GUI)
	OlgSizeTextLabel(screen, iconic ? iconlabel_mdm : wmstep->label_mdm,
			&labeldata, &labelwidth, &labelheight);
else
	if (!iconic)
		OlgSizeTextLabel(screen, motlabel_mdm, &labeldata,
					&labelwidth, &labelheight);

   /* Note that font_list is obtained within a primitive widget by
    * the XtNfontGroup resource.
    */
} /* Open Look or (Motif AND not iconic) */

FPRINTF((stderr, "in DisplayWM with window %x name = %s\n", window, text));

WhiteGC = OlgGetBrightGC(wmstep->mdm);
BlackGC = OlgGetBg3GC(wmstep->mdm);

FPRINTF((stderr, "textwid = %d textarea = %d\n", textwid, textarea));

	if (currentGUI == OL_OPENLOOK_GUI)
		textarea  = iconic ? (icon_width - LineWidX(w) * 2 -
			wmstep->metrics-> selectwidth) :
			CoreW(w) - BorderX(w) - Offset(w) - markwidth -
			2 * wmstep->metrics->mark_title_gap;
    if (currentGUI == OL_OPENLOOK_GUI || !iconic) {
	textwid = labelwidth;
	truncated = (textwid > textarea);
	if (truncated) {
		char *p;
		if ( (p = strrchr(text, ':')) != NULL) {
			if ( (int)strlen(p) > 1)
				text = ++p;
			else
				*p = '\0';
			textlen = strlen(text);
			labeldata.label = text;
		if (currentGUI == OL_OPENLOOK_GUI)
			OlgSizeTextLabel(screen, iconic ? iconlabel_mdm :
					 wmstep->label_mdm, &labeldata,
					&labelwidth, &labelheight);
		else
			OlgSizeTextLabel(screen, motlabel_mdm,
					&labeldata, &labelwidth,
					&labelheight);
			truncated = ((int)(labelwidth) > textarea);
			textwid = labelwidth;
		}
	}
    } /* big if */

if (OlgIs3d())
   {
   /* Three dimensional look needs these global GC's */
   nw_gc = WhiteGC;
   se_gc = BlackGC;
   }

if (iconic)
   {
   int hastitle = 0;
   int title_area = 0;

   int continue_processing = 0;
   int icon_border_width = LineWidX(w) + wmstep->metrics->selectwidth;
   int x, y;
   unsigned int width, height, bw, depth;
   Window root;



   window = XtWindow(targetWidget);
   if (currentGUI == OL_OPENLOOK_GUI &&
		wmstep->xwmhints->flags & IconWindowHint &&
		wmstep-> xwmhints->icon_window)
      {
      FPRINTF((stderr, "use icon window\n"));
	/* this window has already been reparented and mapped - I hope.
	 * all I have to do is draw the label in the allocated space.
	 */
	if (wmstep->decorations & WMIconWindowReparented &&
	    wmstep->decorations & WMIconWindowMapped) {
		int	label_area_wid = icon_width - 2 * icon_border_width,
			ystarthere,
			label_area_ht;
	
		if ( (labeldata.label) && (int)(strlen(labeldata.label)) > 0) {
			xstarthere = icon_border_width;
			/* For starting y position of the icon label, center it
			 * just below the ol_icon_image_height
			 */
			ystarthere = icon_border_width + ol_icon_image_height;
			/* NO BUFFER between the image and label -
			 * just center it
			 */

			label_area_ht = icon_height - ystarthere -
						 icon_border_width;

			labeldata.justification = TL_CENTER_JUSTIFY;

			OlgDrawTextLabel(screen, window, iconlabel_mdm,
			 xstarthere, ystarthere, label_area_wid, label_area_ht,
							&labeldata);
		} /* labeldata.label && strlen > 0 */

	} /* IconWindowReparented && IconWindowMapped */
      } /* IconWindowHint && icon_window */
   else
	if (currentGUI == OL_MOTIF_GUI) {
		DisplayMotifIcon(w);
		return;
	}
	else {
	/* Not an icon window or motif mode - must be a pixmap, ours or theirs */

	XGCValues gcvals;
	GC iconGC = (GC)NULL;
	unsigned long gcmask;


      if ( ((wmstep-> xwmhints-> flags & IconPixmapHint) &&
          wmstep-> xwmhints-> icon_pixmap) &&
		XGetGeometry(display, wmstep->xwmhints->icon_pixmap,
			&root, &x, &y, &width, &height,
					&bw, &depth))
         {
	 /* icon pix(bit)map supplied in WM_HINTS property, use it! */

#ifdef TOO_BIG
	if (width > icon_width || height > icon_height) {
		/* Icon pixmap is just too big */
		continue_processing++;
	}
	else
#endif
	{

	/* x = starting x position of the pixmap.  x must be >= 0 because
	 * of the above check
	 */
         x = (icon_width - width) / 2;
	 if (x < 3)
		x = 3;

	/* y = starting y position of the pixmap */
	if (wmstep->icon_name && strlen(wmstep->icon_name) == 0 ||
		(height > ol_icon_image_height) ) {
		/* empty string, "", or big pixmap (no title for these) */
         	if (height > ol_icon_image_height) {
			y = 3; /* for top border */
			hastitle = 1;
		}
		else {
			y = (icon_height - height) / 2;
			if (y < 3)
				y = 3;
			hastitle = 0;
		}
	}
	else {
		y = (ol_icon_image_height - height) / 2;
		y += 3; /* for top border */
		hastitle = 1;

	} /* else  there is an icon name */

		gcmask = GCForeground|GCBackground;
		gcvals.foreground = wmrcs->iconForeground;
		gcvals.background = wmrcs->iconBackground;

		if ((wmstep-> xwmhints-> flags & IconMaskHint) &&
				wmstep-> xwmhints-> icon_mask) {
			gcvals.clip_mask = wmstep->xwmhints->icon_mask;
			gcmask |= GCClipMask;
			gcvals.clip_x_origin = x;
			gcvals.clip_y_origin = y;
			gcmask |= GCClipXOrigin | GCClipYOrigin;
		}
		iconGC = XtGetGC((Widget)w, gcmask, &gcvals);
/* pcc */
		icon_rect.x = x;
		icon_rect.y = y;
		icon_rect.width = width;
		icon_rect.height = height;

	    /* This is a pixmap with depth == depth of screen */
         if (depth != 1 && depth == w-> core.depth)
            {
		int	usewidth,
			useheight;
		if ((x + width) > (icon_width - 3))
			width = icon_width - 6;
		if (height > ol_icon_image_height)
			height = ol_icon_image_height;
	
            XCopyArea(display, wmstep-> xwmhints-> icon_pixmap, 
               window, iconGC, 0, 0, width, height, x, y);
            }
	else {
	    /* Bitmap supplied (hopefully- doesn't check to see if depth == 1 */
               XCopyPlane(display, wmstep-> xwmhints-> icon_pixmap, 
                  window, iconGC, 0, 0, width, height, x, y, 1L);
	}
		if (iconGC) {
			XtReleaseGC((Widget)w, iconGC);
			iconGC = (GC)NULL;
		}
            }
		}
	else 
		continue_processing++;
	if (continue_processing) {
		/* use default icon */

	 /* Neither window or pix(bit)map supplied, use default icon */ 

#define PICTURE_OFFSET 2

	if(default_icon == (Pixmap)0) {
		Window root_window = RootWindowOfScreen(screen);

			default_icon = XCreatePixmapFromData( display,
				root_window, DefaultColormapOfScreen(screen),
				deficon_width, deficon_height,
				DefaultDepthOfScreen(screen),
				deficon_ncolors, deficon_chars_per_pixel,
				deficon_colors, deficon_pixels);
			default_icon_width = deficon_width;
			default_icon_height = deficon_height;
			CreateDefaultIconMask();
	} /* end if (default_icon == 0) */

         XGetGeometry(display, default_icon, &root,
            &x, &y, &width, &height, &bw, &depth);

	xstarthere = (icon_width - width) / 2;

	/* ystarthere = starting y position of the pixmap - Must have a
	 * title for this type with no icon window or pixmap.
	 */
	ystarthere = (ol_icon_image_height - height) / 2 + icon_border_width;
	hastitle = 1;



	/* For these, you must have a title, so figure a 'y' into the
	 * calculations.
	 */
	if (!(labeldata.label) || strlen(labeldata.label) == 0) {
		labeldata.label = wmstep->window_name;
		if (!(labeldata.label) || strlen(labeldata.label) == 0)
			labeldata.label = "untitled";
		OlgSizeTextLabel(screen, iconic ? iconlabel_mdm :
					wmstep->label_mdm, &labeldata,
					&labelwidth, &labelheight);
	} /* fill in title */

		gcmask = GCForeground|GCBackground;
		gcvals.foreground = wmrcs->iconForeground;
		gcvals.background = wmrcs->iconBackground;

		if ((wmstep-> xwmhints-> flags & IconMaskHint) &&
				wmstep-> xwmhints-> icon_mask ||
						default_mask) {
			if (default_mask) {
				gcvals.clip_mask = default_mask;
			}
			else
				gcvals.clip_mask = wmstep->xwmhints->icon_mask;
			gcmask |= GCClipMask;
			gcvals.clip_x_origin = xstarthere;
			gcvals.clip_y_origin = ystarthere;
			gcmask |= GCClipXOrigin | GCClipYOrigin;
		}

		iconGC = XtGetGC((Widget)w, gcmask, &gcvals);


	if (depth == 1 || depth != w->core.depth) {
		XCopyPlane(display, default_icon, window, iconGC,
			x, y, width, height, xstarthere, ystarthere, 1L );
	}
	else
         	XCopyArea(display, default_icon, window, iconGC,
			x, y, width, height, xstarthere, ystarthere);

	} /* else (using default icon) */

	/* Now draw the title, if any, under the icon */

	 if (hastitle) {
		/* Draw title in icon.
	 	 * Center the text in the area below the default icon.
		 * The text is in labeldata.text, the GC is already
		 * set up with the correct font (labeldata.normalGC).
		 */
		int	label_area_wid = icon_width - 2 * icon_border_width,
			ystarthere,
			label_area_ht;
	
		xstarthere = icon_border_width;
		/* For starting y position of the icon label, center it
		 * just below the ol_icon_image_height
		 */
		ystarthere = icon_border_width + ol_icon_image_height;

		/* NO BUFFER between the image and label */

		label_area_ht = icon_height - ystarthere - icon_border_width;

		/* Hopefully, label_area_ht > labelheight; if not, then
		 * you're out of luck.
		 */

		labeldata.justification = TL_CENTER_JUSTIFY;

		OlgDrawTextLabel(screen, window, iconlabel_mdm,
			 xstarthere, ystarthere, label_area_wid, label_area_ht,
							&labeldata);
	 } /* if hastitle */
		if (iconGC) {
			XtReleaseGC((Widget)w, iconGC);
			iconGC = (GC)NULL;
		}
         } /* Open Look mode icon, not icon window */

	if (iconlabel_mdm) {
		OlgDestroyAttrs(iconlabel_mdm);
		iconlabel_mdm = (OlgAttrs *)NULL;
	}
   /* Use icon border if resource = True, or 1 bit per pixel screen */
   if (wmrcs->iconBorder || (DefaultDepthOfScreen(screen) == 1) )
      {
      /* CalcRects() - 
       *  - third arg - BD - the "compass" flag telling function that it's
       * working on  the border. 
       *  - 4th, 6th args - array of XRectangles;
       *  - 5th, 7th - addr. of ints (return values telling us how many
       *    XRectangles were filled in for each array) < on/about 1825 >
       */
      CalcRects(w, wmstep-> metrics, BD, nw_corners, &nw_i, se_corners, &se_i);
      XFillRectangles(display, window, nw_gc, nw_corners, nw_i);
      XFillRectangles(display, window, se_gc, se_corners, se_i);
      } /* iconBorder or monochrome */
   } /* end, if (iconic) */
else
   {
   if (currentGUI == OL_MOTIF_GUI) {
	int client_add = (Header(w) ? wmstep->metrics->motifButtonHeight:
		0);
/*
 *	if (wmstep->protocols & HasFocus || wmstep->is_current)
 *
** In Motif, we want to be sure that only one window is current/has
 * focus at one time, whether it be a base window or icon.
*/
	if (wmstep->is_current)
		MotifFillFocusBackground(w, (Boolean)True, hwidth, vwidth);
	else
		MotifFillFocusBackground(w, (Boolean)False, hwidth, vwidth);

	/* Next draw the shadows that make up the outer border - 2 pixels-
	 * north and west are light, south and east are dark
	 */
	if (wmstep->decorations & WMBorder) {
          _OlgDrawBorderShadow(screen, window, w->wmstep.mdm,
			OL_SHADOW_OUT, 2, 0, 0, w->core.width, w->core.height,
			wmstep->is_current ? mcri->activetopGC : mcri->topGC,
			wmstep->is_current ? mcri->activebotGC : mcri->botGC);

	/*  Now the "inner" border */

	  _OlgDrawBorderShadow(screen, window, w->wmstep.mdm,
			OL_SHADOW_IN, 1, hwidth - 1, vwidth - 1,
			w->core.width - 2 * hwidth + 2,
			w->core.height - 2 * vwidth + 2,
			wmstep->is_current ? mcri->activetopGC : mcri->topGC,
			wmstep->is_current ? mcri->activebotGC : mcri->botGC);

	/* Next draw the shadow surrounding the client */
	_OlgDrawBorderShadow(screen, window, w->wmstep.mdm,
			OL_SHADOW_IN, 1, hwidth, vwidth +
			client_add,
			w->core.width - 2 * hwidth,
			w->core.height - 2 * vwidth - client_add,
			wmstep->is_current ? mcri->activetopGC : mcri->topGC,
			wmstep->is_current ? mcri->activebotGC : mcri->botGC);
	} /* Has border */
	else /* Simple 1 pixel thick shadow in header, if has one */
		if (Header(w))
		  _OlgDrawBorderShadow(screen, window, w->wmstep.mdm,
			OL_SHADOW_OUT, 1, 0, 0, w->core.width,
			wmstep->metrics->motifButtonHeight + 2,
			wmstep->is_current ? mcri->activetopGC : mcri->topGC,
			wmstep->is_current ? mcri->activebotGC : mcri->botGC);
		
			

	/* Now go to work on the buttons, including the title button but
	 * not the title text.
	 */
	if (Header(w))
		DrawHeaderGlyph(w, display, window, text_gc, wmstep,
						wmstep-> decorations);
	/* I'm now going to go to work on the resize corners */
	if (Resizable(w)) {
		/* We've got 16 lines to draw at this point if the
		 * frame has resize corners. Half light, half dark.
		 */
		DrawMotifResizeCorners(w); 
		/* for label text */
		if (Header(w)) {
		  xstarthere = wmstep->metrics->motifHResizeBorderWidth +
			2;
		  ystarthere = wmstep->metrics->motifVResizeBorderWidth +
			2;
		}
	} /* if Resizable */
	else {
		if (Header(w)) {
		  if (wmstep->decorations & WMBorder) {
		    xstarthere = wmstep->metrics->motifHNResizeBorderWidth +
			2;
		    ystarthere = wmstep->metrics->motifVNResizeBorderWidth +
			2;
		  } /* Has border */
		  else {
			xstarthere = 3;
			ystarthere = 2;
			/* ??? Why do we add 2 to ystarthere above?? */
		  }
		} /* Header */
	} /* not resizable */

	if (Header(w)) {
		/* Note - must check to see if WMMenu8utton is present
		 * in the decorations - can't use HasMenu() macro,
		 * because that tells us if there are any menu functions,
		 * which means pop up the menu if a menu key or event
		 * occurs.
		 */
		if (wmstep->decorations & WMMenuButton)
			xstarthere += wmstep->metrics->motifButtonWidth;
		labeldata.justification = TL_CENTER_JUSTIFY;
		if (wmstep->is_current && activeBackgroundPixmap ||
				!(wmstep->is_current) && backgroundPixmap)
			XClearArea(display, window,
				xstarthere + textarea/2 - labelwidth/2,
				ystarthere, labelwidth, labelheight, False);
		OlgDrawTextLabel(screen, window, motlabel_mdm,
			xstarthere, ystarthere,
			textarea, wmstep->metrics->motifButtonHeight-2,
			&labeldata);
	}

   } /* Motif */
else { /* Open Look GUI */

   /* not iconic, draw the decorations */
   if (Resizable(w)) {
      /* Get XRectangles for resize corners... */
      CalcRects(w, wmstep-> metrics, NW, nw_corners, &nw_i, se_corners, &se_i);
      CalcRects(w, wmstep-> metrics, NE, nw_corners, &nw_i, se_corners, &se_i);
      CalcRects(w, wmstep-> metrics, SE, nw_corners, &nw_i, se_corners, &se_i);
      CalcRects(w, wmstep-> metrics, SW, nw_corners, &nw_i, se_corners, &se_i);

   } /* end if (Resizable(w)) */

	/* If this isn't the "current" window, just draw the standard
	 * border.  If it is current, and we are using 3D, we'll draw the
	 * select border later.  This is necessary because a different GC may
	 * be needed. For 2D graphics, it's O.K. to use the same function.
	 */
	if ( (!wmstep->is_current) || (!OlgIs3d()) )
		CalcRects(w, wmstep-> metrics, BD, nw_corners, &nw_i,
							 se_corners, &se_i);

   if (Header(w))
      {
	/*
	 * Use new function to draw chistled dividing line.  Currently, the
	 * line width is fixed regardless of the scale.  The '2' argument
	 * is for "strokes" - it is used when drawing a line for 2-D.
	 */
	if (! (IsIconic(w)))
		OlgDrawLine(screen, XtWindow(w), wmstep->mdm, BorderX(w),
			 BorderY(w) + BannerHt(w) + w->wmstep.metrics->hgap1,
						BannerWid(w), 2, 0);
	/*  the current and focus window must be the same */
	if (wmstep->is_current)
         {
	 /* HasFocus flag would have been set in ClientFocusChange() */
         XRectangle rect;
         if ( wmrcs->pointerFocus) 
         {
            /* erase the title area */
            XSetForeground(display, text_gc, wmrcs->backgroundColor);
            
            /* HeaderRect() fills in rect with header dimensions */
            rect = HeaderRect(w);
            XFillRectangles(display, window, text_gc, &rect, 1);
            
            XSetForeground(display, text_gc, wmrcs->foregroundColor);
         }
         else
            {
	    /* Make header the inputFocusColor for click-to-type - use
	     * the text_gc, then reset it afterward to wmstep->f.g._pixel
	     */
		if (headcase == 0) /* normal case, proceed as usual */
            XSetForeground(display, text_gc, wmrcs->inputFocusColor);
		/* otherwise, fill it in with the normal foreground pixel */
		else
				 /* Headcase 1:
				  * On focus, make back = white,
			          *		fore = black;
				  * Headcase 2:
				  *	 On focus, make back = black,
				  *			fore = white.
				  */
			if (headcase == 1)
				XSetForeground(display, text_gc, white);
			else if (headcase == 2)
				XSetForeground(display, text_gc, black);

	    /* HeaderRect() fills in rect with header dimensions */
            rect = HeaderRect(w);
            XFillRectangles(display, window, text_gc, &rect, 1);
		if (headcase == 0)
            XSetForeground(display, text_gc, wmstep-> foreground_pixel);
            }
         } /* end, if (wmstep->is_current) */

	 temp_rect = HeaderRect(w);

	/* Draw the title with careful placement.
	 * The text in the title (if it has a header)
 	 * can be centered in the entire header if, after calculating the center
 	 * position and the starting point of the title, there is
 	 * wmstep->metrics->mark_title_gap pixels between the window mark
	 * (pin or button) and the title, AND that same amount of space is
	 * available to the right of the title near the right portion of the
	 * header. If not enough space is available, try and center it in
	 * the text area only.
 	 */

	labeldata.justification = TL_CENTER_JUSTIFY;

	if (currentGUI == OL_OPENLOOK_GUI) {
	xstarthere = CoreW(w)/2 - textwid/2;
	xendthere = xstarthere + textwid;
	if (xstarthere >= (Offset(w) + markwidth +
					 wmstep->metrics->mark_title_gap) ) {
		centered = FULLCENTER;
		xstarthere = 0;
	}
	else { /* not enough room for fully centering within title bar.
		* any room to center to right of menu mark ?
		* If not, it will be left justified at mark_title_gap
		* pixels from the menu mark and truncated with an arrow
		* if necessary.
		*/
		xstarthere = Offset(w) + markwidth +
						wmstep->metrics->mark_title_gap;
		centered = PARTIALCENTER;
	}		
	} /* currentGUI == OPENLOOK */
	else /* MOTIF */
		xstarthere = hwidth + wmstep->metrics->motifButtonWidth + 2;

				 /* Headcase 1:
				  * On focus, make back = white,
			          *		fore = black;
				  * Headcase 2:
				  *	 On focus, make back = black,
				  *			fore = white.
				  */
	if (wmstep->is_current && headcase && !wmrcs->pointerFocus) {
		if (headcase == 1)
			XSetForeground(display, labeldata.normalGC, black );
		else
			/* Currently only 2 headcases */
			XSetForeground(display, labeldata.normalGC, white);
	}

	OlgDrawTextLabel(screen, window, wmstep->label_mdm,
			xstarthere,
			temp_rect.y,
			centered & FULLCENTER ? CoreW(w) :
			textarea, temp_rect.height, &labeldata);

	if (wmstep->is_current && headcase)
            XSetForeground(display, labeldata.normalGC, wmstep-> foreground_pixel);

	if (! (IsIconic(w))) {
      /* Draw pushpin or window menu mark - it follows this function */
      DrawHeaderGlyph(w, display, window, text_gc, wmstep, wmstep-> decorations);
	 /* Draw 3d box around title */
	 if (OlgIs3d() && !(wmrcs->pointerFocus)) {
		Boolean IsPressed = wmstep->is_current ? True : False;

		OlgDrawBox(screen, window, wmstep->mdm,
			temp_rect.x, temp_rect.y,
			temp_rect.width, temp_rect.height, IsPressed);
	 } /* if (OlgIs3d()) */
	} /* end if (! IsIconic()) */
      } /* end (if (Header(w) ) */
   XFillRectangles(display, window, nw_gc, nw_corners, nw_i);
   XFillRectangles(display, window, se_gc, se_corners, se_i);
	/* Save for last - if using 3D, real-estate based focus, and it has
	 * focus, and it has a header, then draw the BN lines (extra 2
	 * lines in the header - because it uses different GCs when it's
	 * 3D for these lines than it does for the resize corners and
	 * borders.
	 */
   if ( Header(w) && wmstep->is_current && wmrcs->pointerFocus)
	DrawBN(w);
} /* end else (OPen Look) - I think! (mlp) */
   } /* ends else (for non-iconic windows) */

	/* If this is the "current" window, 
	 * AND 3D visuals are in use, draw the "selected"
	 * If this is the current window, and 2D visuals are
	 * in use, then the border is taken care of in
	 * above call to CalcRects() with arg = BD.
	 */
	if (currentGUI == OL_OPENLOOK_GUI)
	if (wmstep->is_current && OlgIs3d()) {
		DrawSelectBorder(w);
	}

if (currentGUI == OL_OPENLOOK_GUI) {
	if (!(IsIconic(w)) && wmstep-> decorations & WMBusy) {
		XRectangle rect;

		/* HeaderRect() just fills in XRectangle fields with header
		 * (banner) dims 
		 */
		rect = HeaderRect(w);
		XFillRectangles(display, window, wmstep->mdm->pDev->busyGC,
								&rect, 1);
	}
	if ((IsIconic(w)) && wmstep-> decorations & WMBusy)	 /* pcc */
		XFillRectangles(display, window, wmstep->mdm->pDev->busyGC,
							&icon_rect, 1);
}
  if (motlabel_mdm)
	OlgDestroyAttrs(motlabel_mdm);
} /* end of DisplayWM */



/*
 *************************************************************************
 * DrawHeaderGlyph
 * - Called from DisplayWM(), FlipPushpin(), and FlipWindowMark().
 * Complete drawing of window header by drawing either pushpin
 * or window menu mark in the correct state.  
 ****************************procedure*header*****************************
 */

static void
DrawHeaderGlyph OLARGLIST((wm, display, window, text_gc, wmstep, decorations))
OLARG(WMStepWidget,	wm)
OLARG(Display *,	display)
OLARG(Window,		window)
OLARG(GC,		text_gc)
OLARG(WMStepPart *,	wmstep)
OLGRA(unsigned long,	decorations)
{
Screen * screen = XtScreen(wm);
unsigned long menufuncs = wmstep->menu_functions;

if (! (decorations & WMHeader) )
	/* Don't waste my time here */
	return;
if (decorations & WMPushpin) {
	GC pinGC = OlgGetFgGC(wmstep->mdm); /* GC used to draw the 2-D pin */
	unsigned long foreground = pinGC->values.foreground;
	Boolean reverse = (wmstep->is_current && !wmrcs->pointerFocus &&
					headcase && (!OlgIs3d()) );
	/* Double check whether to reverse the foreground color of
	 * the pushpin - the colors may have been changed by the user
	 * for this client.
	 */
	if (reverse && foreground == wmstep->foreground_pixel)
            XSetForeground(display, pinGC, wm->core.background_pixel);
   if (decorations & WMPinIn)
      OlgDrawPushPin(screen, window, wmstep->mdm, 
         Offset(wm), BorderY(wm) + (BannerHt(wm) - (int)ppHeight) / 2,
         ppWidth, ppHeight, PP_IN);
   else
      OlgDrawPushPin(screen, window, wmstep->mdm, 
         Offset(wm), BorderY(wm) + (BannerHt(wm) - (int)ppHeight) / 2,
         ppWidth, ppHeight, PP_OUT);
   if (reverse)
	XSetForeground(display, pinGC, foreground);
}
else
   /* New rule: can't have a menubutton and a limited menu */
   if ( (currentGUI == OL_OPENLOOK_GUI && decorations & WMHasFullMenu) ||
	(currentGUI == OL_MOTIF_GUI && decorations & WMMenuButton) )
	if (currentGUI == OL_OPENLOOK_GUI) {
      if (mmstate &&
	(!NumMenushellsPosted || ( (wmstep->decorations & WMHasFullMenu) &&
		(WMCombinedMenu->w == (Widget)wm) )) ) {

	/* There is only one mmstate variable that tells us if the
	 * window menu button is selected or not; it's possible for it
	 * to be set to True, when we really want to to be False (not
	 * indented).  This can occur with this sequence:
	 *	This window has input focus, thus requiring a highlighted
	 *	header;  The window menu is popped up for another window
	 *	doesn't have input focus, which may require drawing a
	 *	selected window menu button on that window header - as
	 *	a result, leaving the mmstate variable set to True;
	 *	Finally, we are about to draw the window header for this
	 *	window, the one that LOST focus when the menu popped up,
	 *	but the mmstate variable is set to TRUE!!
	 * The solution is to check the values of the mmstate variable
	 * and the NumMenushellsPosted variable before drawing the window
	 * menu button for any header.
	 */
         	OlgDrawAbbrevMenuB(screen, window, wmstep->mdm, 
            		Offset(wm), BorderY(wm) + (BannerHt(wm) -
				(int)mmHeight) / 2, AM_SELECTED);
	}
      else  /* mmstate is off */
         	OlgDrawAbbrevMenuB(screen, window, wmstep->mdm, 
			Offset(wm), BorderY(wm) + (BannerHt(wm)
				- (int)mmHeight) / 2, AM_NORMAL);
       } /* Open Look menu button */
	else /* Motif */
		DrawMotifMenuButton((Widget)wm, 
		  ((menufuncs & WMMenuButtonState ? AM_SELECTED : AM_NORMAL)));
	if (currentGUI == OL_MOTIF_GUI) {
		/* have a minimize button ?? */
		if (wmstep->decorations & WMMinimize)
		   DrawMotifMinimizeButton((Widget)wm, 
		     menufuncs & WMMinButtonState ? AM_SELECTED : AM_NORMAL);
		if (wmstep->decorations & WMMaximize)
		   DrawMotifMaximizeButton((Widget)wm,
		    menufuncs & WMMaxButtonState ? AM_SELECTED : AM_NORMAL);
		/* Draw the box around the title */
		   DrawMotifTitleShadow((Widget)wm,
		     menufuncs & WMTitleButtonState ? AM_SELECTED : AM_NORMAL);
/* *** No longer needed since we added that check for WMMenuButton ...
 *
   if (decorations & WMHasLimitedMenu)
		DrawMotifMenuButton((Widget)wm, 
		  ((menufuncs & WMMenuButtonState ? AM_SELECTED : AM_NORMAL)));
			
 */
	} /* if currentGUI == OL_MOTIF_GUI */
		
} /* end of DrawHeaderGlyph */

/*
 *************************************************************************
 * FlipPushpin
 * - Called from SelectPushpin() or PollHandlePushpin().
 * reverse ppstate (in->out or out->in).
 ****************************procedure*header*****************************
 */

extern void
FlipPushpin OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
Pixel	white = WhitePixelOfScreen(XtScreen(wm));
Pixel	black = BlackPixelOfScreen(XtScreen(wm));

if (ppstate & WMPinIn) {
   ppstate &= ~WMPinIn;
	wm->wmstep.decorations &=~WMPinIn;
}
else {
   ppstate |= WMPinIn;
	wm->wmstep.decorations |= WMPinIn;
}

/* I believe the HasFocus flag should be eliminated for cases like this,
 * because there are going to be windows that become current/active
 * that don't take focus, and the only ones that do take focus are
 * ones that really take keyboard focus.  So add the
 * || is_current now, and later, take  out the HasFocus
 */
if (wm->wmstep.is_current)
   {
   /* IF this window has input focus, fill in the rectangular area where the
    * pushpin is with the inputFocusColor.  If using pointerFocus or this is
    * headcase, do it with the background color.
    */
	if (wmrcs->pointerFocus) 
   		XSetForeground(XtDisplay(wm), wm-> wmstep.gc,
					 wm->core.background_pixel);
	else
				 /* Headcase 1:
				  * On focus, make back = white,
			          *		fore = black;
				  * Headcase 2:
				  *	 On focus, make back = black,
				  *			fore = white.
				  */
		if (headcase == 1)
   			XSetForeground(XtDisplay(wm), wm-> wmstep.gc,
					 white);
		else
			if (headcase == 2)
   				XSetForeground(XtDisplay(wm), wm-> wmstep.gc,
						 black);
			else
   				XSetForeground(XtDisplay(wm), wm-> wmstep.gc,
						 wmrcs->inputFocusColor);
   	XFillRectangle(XtDisplay(wm), XtWindow(wm), wm-> wmstep.gc, 
      		Offset(wm), BorderY(wm) + (BannerHt(wm) - (int)ppHeight) / 2, 
		ppWidth, ppHeight);
   	XSetForeground(XtDisplay(wm), wm-> wmstep.gc, wm-> wmstep.foreground_pixel);
   }
else
   /* You don't have input focus, just clear out pin's area with f.g. color */
   XClearArea(XtDisplay(wm), XtWindow(wm), Offset(wm),
	 BorderY(wm) + (BannerHt(wm) - (int)ppHeight) / 2,
         ppWidth, ppHeight, False);

/* DrawHeaderGlyph() draws the pin (or menu mark) in correct state. - ppstate
 * must be an OR-ing of WMPushpin and either WMPinIn or WMPinOut
 */
DrawHeaderGlyph(wm, XtDisplay(wm), XtWindow(wm), wm-> wmstep.gc, &wm-> wmstep,
		wm->wmstep.decorations);
if ( (wm-> wmstep.is_current) && (wmrcs->pointerFocus))
		/* Redraw the extra lines in the header */
		DrawBN(wm);
} /* end of FlipPushpin */

/*
 *************************************************************************
 * FlipMenuMark
 * - similar to FlipPushPin() - except do it for the emnu mark.
 * Flipping it amounts to darken (set) or lighten (unset).
 ****************************procedure*header*****************************
 */

extern void
FlipMenuMark OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);

if (currentGUI == OL_OPENLOOK_GUI)
	mmstate = !mmstate;
else {
	if (wmstep->menu_functions & WMMenuButtonState)
		wmstep->menu_functions &= ~ WMMenuButtonState;
	else
		wmstep->menu_functions |= WMMenuButtonState;
}

/* This is a security check - Don't draw the menu mark if we are
 * in an iconic state.  This could happen if SELECT is pressed on
 * the menu mark to trigger the menu default - FlipMenuMark()
 * is called before AND after the default button's function is
 * executed - if it was the "CLOSE" button, then we would be in
 * iconic state.
 *
 * New: don't draw the menu mark if there is no header.
 */
if (wmstep->size == WMICONICNORMAL || wmstep->size == WMICONICFULL ||
				!(wmstep->decorations & WMHeader) )
	return;

if (currentGUI == OL_OPENLOOK_GUI) {
if (wmstep->is_current) {
	if (wmrcs->pointerFocus || headcase)
   		XSetForeground(XtDisplay(wm), wmstep->gc,
					 wm->core.background_pixel);
	else
		XSetForeground(XtDisplay(wm), wmstep->gc,
						 wmrcs->inputFocusColor);
	XFillRectangle(XtDisplay(wm), XtWindow(wm), wm-> wmstep.gc, 
      		Offset(wm), BorderY(wm) + (BannerHt(wm) - (int)mmHeight) / 2, 
		mmWidth, mmHeight);
	XSetForeground(XtDisplay(wm), wmstep->gc,
						 wmstep->foreground_pixel);
   }
else
   XClearArea(XtDisplay(wm), XtWindow(wm), Offset(wm),
	 BorderY(wm) + (BannerHt(wm) - (int)mmHeight) / 2,
         mmWidth, mmHeight, False);

DrawHeaderGlyph(wm, XtDisplay(wm), XtWindow(wm), wmstep->gc, &wm-> wmstep,
					 wmstep->decorations);

if ( (wm-> wmstep.is_current) && (wmrcs->pointerFocus))
		/* Redraw the extra lines in the header */
		DrawBN(wm);
} /* currentGUI == OPEN LOOK */
else { 	/* Motif */
	if (wmstep->menu_functions & WMMenuButtonState) 
		DrawMotifMenuButton((Widget)wm, AM_SELECTED);
	else
		DrawMotifMenuButton((Widget)wm, AM_NORMAL); 
}

} /* end of FlipMenuMark */

/*
 *************************************************************************
 * GetMetrics
 * - Called from Initialize(), OpenClose(), ClientPropertyChange()
 * Return ptr to the array of hard-coded values among wmmetrics[] array
 * for the scale  used by the application.
 ****************************procedure*header*****************************
 */

extern WMMetrics *
GetMetrics OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
int i;
int s;

/* From Initialize(), first called with size == WMNORNALSIZE;
 * set s = 
 *   0: decoration set includes header, resize corners;
 *   1: header (but no resize corners)
 *   2: no decorations (it may be in iconic state, but who knows)
 */
if (wmstep-> size == WMICONICNORMAL || wmstep-> size == WMICONICFULL)
   s = 2;
else
   if (wmstep-> decorations & WMHeader)
      if (wmstep-> decorations & WMResizable)
         s = 0;  /* has resize and header */
      else
         s = 1;  /* has header */
   else
      s = 2;     /* has nothing */

/* loop until i == num_wmmetrics, the number of "built-in" hard-coded values
 * defined in the wmmetrics static array;  the number was initialized in
 * CompileMetrics() which was called from SetupWindowManager().  It loops
 * by incremementing i by 3 because each scale had 3 sets of numbers.
 * Permscale is a resource (scale).
 */
for (i = s; i < num_wmmetrics; i += 3)
   if (wmmetrics[i].scale == PermScale &&
		wmmetrics[i].resolution == ScreenResolution) {
	if (wmmetrics[i].compiled == 'N')
		CompileMetrics(XtScreen(wm), i);
      return &wmmetrics[i];
   }
if (wmmetrics[s].compiled == 'N')
		CompileMetrics(XtScreen(wm), s);
return &wmmetrics[s];

} /* end of GetMetrics */

/*
 *************************************************************************
 * CalcRects
 *
 * Calling function passes wm widgets and wmstep->metrics, ptrs to two
 * XRectangle arrays, ptrs to two ints (for return values), and "compass_point".
 * The compass point tells CalcRects() the rectangles to fill such that a
 * decoration can be drawn via XFillRectangles().
 *  - BD: window (icon) border;
 *  - LN: dividing line between header and client window pane;
 *  - NW, NE, SW, SE: resize corners;
 *  - BN: looks like one line above the title and one line below it (?)
 *
 *  case 1 -  called from DisplayWM() with compass_point == BD (if icons have
 *	      borders), ptrs to NWrect = 0 and SErect = 0.
 *
 * Important note: NWRECT and SERECT are nothing more than macros that fill
 * in the x, y, width, and height fields of the NWrect or SErect Rectangle
 * with the parameters given.  The macros are defined at the top of this file
 * and use exactly the variables named NWrect and SErect. The macro also
 * increments num_SErect and num_NWrect.
 ****************************procedure*header*****************************
 */

static void
CalcRects OLARGLIST((wm, metrics, compass_point, NWrect, num_NWrect,
					 SErect, num_SErect))
OLARG(WMStepWidget, wm)
OLARG(WMMetrics *,  metrics)
OLARG(int,  compass_point)
OLARG(XRectangle *, NWrect)
OLARG(int *, num_NWrect)
OLARG(XRectangle *,  SErect)
OLGRA(int *, num_SErect)
{
int rx;
int ry;
int XCorner  = CornerX(wm);
int YCorner  = CornerY(wm);
int xCorner  = Cornerx(wm);
int yCorner  = Cornery(wm);
int XLineWid = LineWidX(wm);
int YLineWid = LineWidY(wm);
int xLineWid = LineWidx(wm);
int yLineWid = LineWidy(wm);
int num_NW   = *num_NWrect;
int num_SE   = *num_SErect;
int width    = wm-> core.width;
int height   = wm-> core.height;
int addwidth = 0;

switch (compass_point)
   {
   case BN:
	/*  - BN: Draw one line above the title and one line below it.  Useful
	 *	  for real-estate based input focus.
	 *
	 */
	if (Resizable(wm)) {
      NWRECT(XCorner + xLineWid, BorderY(wm) - YLineWid, 
             width - 2 * (XCorner + xLineWid), YLineWid);
	}
	else {
      NWRECT(BorderX(wm), BorderY(wm) - YLineWid, 
             BannerWid(wm), YLineWid);
	}
      NWRECT(BorderX(wm), BorderY(wm) + BannerHt(wm) - YLineWid, 
             BannerWid(wm), YLineWid);
      break;
   case BD:
	/* First two fill the rectangles needed to draw the top and left
	 * borders respectively, and the next can draw the bottom and right.
	 * When not using 3D, make the border thicker so the current window
	 * appears more distinguishable.
	 */

      if ( wm->wmstep.is_current ) {
			addwidth = metrics->selectwidth;
	}
	if (IsIconic(wm)) {
		width = wm->wmstep.icon_widget->core.width;
		height = wm->wmstep.icon_widget->core.height;
	}
      NWRECT(XCorner, 0, width - 2 * XCorner, YLineWid + addwidth); /* top */
      NWRECT(0, YCorner, XLineWid + addwidth,
					height - 2 * YCorner); /* left */
      SERECT(XCorner, height - (YLineWid + addwidth), width - 2 * XCorner,
					YLineWid + addwidth); /* bottom */
      SERECT(width - (XLineWid + addwidth), YCorner,
					XLineWid + addwidth,
					height - 2 * YCorner); /* right */
      break;
   case LN:
      /* The dividing line between the header and the client window pane */
      NWRECT(BorderX(wm), BorderY(wm) + BannerHt(wm) + metrics-> hgap1, 
             BannerWid(wm), metrics-> linewid);
      SERECT(BorderX(wm), BorderY(wm) + BannerHt(wm) + metrics-> hgap1 + metrics-> linewid, 
             BannerWid(wm), metrics-> linewid);
      break;
   case NW:
      /* upper left resize corner */
      rx = 0;
      ry = 0;
      NWRECT(rx, ry, XCorner, yLineWid);
      NWRECT(rx, ry, xLineWid, YCorner);
      SERECT(rx + xLineWid, ry + YCorner - yLineWid, xCorner, yLineWid);
      SERECT(rx + xCorner, ry + yCorner, xLineWid, YCorner - yCorner);
      SERECT(rx + xCorner, ry + yCorner, XCorner - xCorner, yLineWid);
      SERECT(rx + XCorner - xLineWid, ry + yLineWid, xLineWid, yCorner);
      break;
   case NE:
      /* Upper right resize corner */
      rx = width - XCorner;
      ry = 0;
      NWRECT(rx, ry, XCorner, yLineWid);
      NWRECT(rx, ry + yLineWid, xLineWid, yCorner);
      SERECT(rx + xLineWid, ry + yCorner, 
             XCorner - xCorner - xLineWid, yLineWid);
      NWRECT(rx + XCorner - xCorner - xLineWid, ry + yCorner, 
             xLineWid, YCorner - yCorner);
      SERECT(rx + XCorner - xCorner - xLineWid, ry + YCorner - yLineWid, 
             xCorner + xLineWid, yLineWid);
      SERECT(rx + XCorner - xLineWid, ry, xLineWid, XCorner);
      break;
   case SW:
      /* Lower left resize corner */
      rx = 0;
      ry = height;
      NWRECT(rx + xLineWid, ry - YCorner, xCorner, yLineWid);
      NWRECT(rx, ry - YCorner, xLineWid, YCorner);
      SERECT(rx, ry - yLineWid, XCorner, yLineWid);
      SERECT(rx + XCorner - xLineWid, ry - yLineWid - yCorner, 
             xLineWid, yCorner);
      NWRECT(rx + xCorner, ry - yCorner - yLineWid, 
             XCorner - xCorner - xLineWid, yLineWid);
      SERECT(rx + xCorner, ry - YCorner + yLineWid, 
             xLineWid, YCorner - yCorner - yLineWid);
      break;
   case SE:
      /* Lower right resize corner */
      rx = width;
      ry = height;
      NWRECT(rx - xLineWid - xCorner, ry - YCorner, xCorner, yLineWid);
      NWRECT(rx - xLineWid - xCorner, ry - YCorner + yLineWid, 
              xLineWid, YCorner - yCorner - yLineWid);
      NWRECT(rx - XCorner + xLineWid, ry - yLineWid - yCorner,
              XCorner - xCorner - xLineWid, yLineWid);
      NWRECT(rx - XCorner, ry - yLineWid - yCorner, xLineWid, yCorner);
      SERECT(rx - XCorner, ry - yLineWid, XCorner, yLineWid);
      SERECT(rx - xLineWid, ry - YCorner, xLineWid, YCorner);
      break;
   }

/* Return the number of rectangles used in each array */
*num_SErect = num_SE;
*num_NWrect = num_NW;

} /* end of CalcRects */

/*
 * DrawBN
 * Draw the input focus lines in the header when a client takes focus
 * AND wmrcs->pointerFocus is TRUE.  Use the inputWindowHeader resource
 * color.
 */
extern void
DrawBN OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart * wmstep    = &wm-> wmstep;
XRectangle   nw_corners[4];
XRectangle   se_corners[4];
int          nw_i      = 0;
int          se_i      = 0;
GC           nw_gc     = wmstep-> gc;
GC           se_gc     = wmstep-> gc;
GC           text_gc   = wmstep-> gc;
Window	window = XtWindow(wm);
Display *display = XtDisplay(wm);

   CalcRects(wm, wmstep-> metrics, BN, nw_corners, &nw_i, se_corners, &se_i);

	XSetForeground(XtDisplay(wm), wm-> wmstep.gc,
						 wmrcs->inputFocusColor);
	XFillRectangles(display, window, nw_gc, nw_corners, nw_i);
	XFillRectangles(display, window, se_gc, se_corners, se_i);
	XSetForeground(XtDisplay(wm), wm-> wmstep.gc,
						 wm-> wmstep.foreground_pixel);
} /* DrawBN */

/*
 * EraseBN
 */
extern void
EraseBN OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart	*wmstep = &wm-> wmstep;
XRectangle	nw_corners[4];
XRectangle	se_corners[4];
int		nw_i = 0;
int		se_i = 0;
GC		nw_gc = wmstep-> gc;
GC		se_gc     = wmstep-> gc;
GC		text_gc   = wmstep-> gc;
Window		window = XtWindow(wm);
Display		*display = XtDisplay(wm);

	CalcRects(wm, wmstep-> metrics, BN, nw_corners, &nw_i, se_corners,
								&se_i);
	XSetForeground(XtDisplay(wm), wm->wmstep.gc,
					wm->core.background_pixel);
	XFillRectangles(display, window, nw_gc, nw_corners, nw_i);
	XFillRectangles(display, window, se_gc, se_corners, se_i);
	XSetForeground(XtDisplay(wm), wm-> wmstep.gc,
						 wm-> wmstep.foreground_pixel);
} /* EraseBN */

/*
 *************************************************************************
 * EraseSelectBorder - should probably be adjsuted for icons to draw
 * the icon background if it's in iconic form.
 ****************************procedure*header*****************************
 */
extern void
EraseSelectBorder OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart	*wmstep    = &wm-> wmstep;
XRectangle	nw_corners[4];
XRectangle	se_corners[4];
int		nw_i      = 0;
int		se_i      = 0;
GC		nw_gc     = wmstep-> gc;
GC		se_gc     = wmstep-> gc;
Window	window = IsIconic(wm) ? XtWindow(wmstep->icon_widget) : XtWindow(wm);
Display *display = XtDisplay(wm);

	if (currentGUI == OL_MOTIF_GUI) {
		/* redisplay motif-style icon in non-active mode,
		 * then return.
		 */
		return;
	}
	/* First erase the selected border, then draw the "normal"
	 * border. If we're in the non-3D case, then CalcRects() will
	 * erase the thickeded border for us - no adjustments needed
	 * here. Presumably we enter this chunk of code for a window
	 * that just previously had wmstep->is_current == 1, but NOW
	 * has wmstep->is_current == 0.  Unfortunately, the difference
	 * in the borders for the non-3D case between the selected vs.
	 * non-selected borders is nothing more than the thickness;
	 * therefore, before calling CalcRects() with the non-3D case
	 * we must fool it into thinking it is still doing the selected
	 * case - so first set wmstep->is_current back to 1, then reset
	 * it to 0.
	 * More: make all selected borders the same thickness, regardless
	 * of 2D vs. 3D.
	 */
	/*
	if (!OlgIs3d())
	 */
		wmstep->is_current = 1;
	CalcRects(wm, wmstep-> metrics, BD, nw_corners, &nw_i, se_corners,
								&se_i);

	/* In both cases, it is now safe to reset the is_current field to 0 */
	wmstep->is_current = 0;

	if (IsIconic(wm) && wm->wmstep.icon_widget) {
	    if (wmrcs->iconParentRelative) {
		Window window = XtWindow(wmstep->icon_widget);
		XClearArea(display, window, nw_corners[0].x, nw_corners[0].y,
			nw_corners[0].width, nw_corners[0].height, False);
		XClearArea(display, window, nw_corners[1].x, nw_corners[1].y,
			nw_corners[1].width, nw_corners[1].height, False);
		XClearArea(display, window, se_corners[0].x, se_corners[0].y,
			se_corners[0].width, se_corners[0].height, False);
		XClearArea(display, window, se_corners[1].x, se_corners[1].y,
			se_corners[1].width, se_corners[1].height, False);
		return;
	     }
		XSetForeground(XtDisplay(wm),nw_gc,
				wm->wmstep.icon_widget->core.background_pixel);
		XSetForeground(XtDisplay(wm),se_gc,
				wm->wmstep.icon_widget->core.background_pixel);
	} 
	else { /* not iconic */
		XSetForeground(XtDisplay(wm),nw_gc,
					wm-> core.background_pixel);
		XSetForeground(XtDisplay(wm),se_gc,
				wm-> core.background_pixel);
	}
	XFillRectangles(display, window, nw_gc,
						nw_corners, nw_i);
	XFillRectangles(display, window, se_gc,
						se_corners, se_i);
	XSetForeground(XtDisplay(wm),nw_gc, wmstep->foreground_pixel);
	XSetForeground(XtDisplay(wm),se_gc, wmstep->foreground_pixel);

	/* Now draw the non-selected border - skip it if icons have no
	 * borders.
	 */
	if (IsIconic(wm) && !(wmrcs->iconBorder) && (DefaultDepthOfScreen(
		XtScreen(wm)) > 1) )
		return;
	if (OlgIs3d()) {
		XRectangle header_rect;

   		/* Three dimensional look needs these global GC's */
   		nw_gc = OlgGetBrightGC(wmstep->mdm); /* WhiteGC */
   		se_gc = OlgGetBg3GC(wmstep->mdm); /* BlackGC */

	 	if ( (!(wmrcs->pointerFocus)) && !(IsIconic(wm)) &&
							(Header(wm)) ) {
			header_rect = HeaderRect(wm);
			OlgDrawBox(XtScreen(wm), window, wmstep->mdm,
				header_rect.x, header_rect.y,
				header_rect.width, header_rect.height, False);
		}
   	}
	nw_i = se_i = 0;
	CalcRects(wm, wmstep-> metrics, BD, nw_corners, &nw_i, se_corners, &se_i);
	XFillRectangles(display, window, nw_gc, nw_corners, nw_i);
	XFillRectangles(display, window, se_gc, se_corners, se_i);
} /* EraseSelectBorder */

/*
 *************************************************************************
 * DrawSelectBorder
 * Use the same GC (wmstep->gc) to draw all parts of the border, ignore
 * the 3-D look if it's current.
 ****************************procedure*header*****************************
 */
extern void
DrawSelectBorder OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart * wmstep    = &wm-> wmstep;
XRectangle   nw_corners[4];
XRectangle   se_corners[4];
int          nw_i      = 0;
int          se_i      = 0;
GC           nw_gc     = wmstep-> gc;
GC           se_gc     = wmstep-> gc;
GC           text_gc   = wmstep-> gc;
Window	window = IsIconic(wm) ? XtWindow(wmstep->icon_widget) : XtWindow(wm);

Display *display = XtDisplay(wm);

	if (currentGUI == OL_MOTIF_GUI) {
		/* redisplay icon with motif style active look */
		return;
	}
	
	if (! (IsIconic(wm))) {
		if (OlgIs3d() && !(wmrcs->pointerFocus) && (Header(wm)) ) {
			XRectangle   header_rect;

			header_rect = HeaderRect(wm);
			OlgDrawBox(XtScreen(wm), window, wmstep->mdm,
				header_rect.x, header_rect.y,
				header_rect.width, header_rect.height, True);
		}
	}

   CalcRects(wm, wmstep-> metrics, BD, nw_corners, &nw_i, se_corners, &se_i);
	XFillRectangles(display, window, nw_gc, nw_corners, nw_i);
	XFillRectangles(display, window, se_gc, se_corners, se_i);
} /* DrawSelectBorder */

/*
 *************************************************************************
 * DrawStreaks
 * - Called from OpenClose() - draw the lines on the screen when going
 * from/to an icon from/to a normal sized window.  The whole thing is done
 * with a single call the XDrawSegments() to draw 16 segments.
 * Watch this trick carefully - I think - the first 8 draw the lines draw the
 * lines, the second 8 is will undraw them (in the exact same spot) -BECAUSE
 * the function is GXInvert.
 * Use the WMStepWidgets GC (wmstep->gc), XSetFunction (use GXInvert)
 * prior to drawing, and draw on the root window.  After, reset function
 * to GXCopy with XSetFunction().
 * - Presumably this will work without grabbing the server
 * because the single call to XDrawSegments() is atomic, but if you run on a
 * machine that rocks, how much of a glimpse of these lines will you get?
 ****************************procedure*header*****************************
 */

extern void
DrawStreaks OLARGLIST((wm, from_x, from_y, from_width, from_height, 
                        to_x, to_y, to_width, to_height))
OLARG(WMStepWidget, wm)
OLARG(int, from_x)
OLARG(int, from_y)
OLARG(int,from_width)
OLARG(int,from_height)
OLARG(int, to_x)
OLARG(int, to_y)
OLARG(int, to_width)
OLGRA(int, to_height)
{
XSegment segments[16];
int fromxplusfromwidth = from_x + from_width;
int fromyplusfromheight = from_y + from_height;
int toxplustowidth = to_x + to_width;
int toyplustoheight = to_y + to_height;

segments[0].x1 = 
segments[3].x1 = 
segments[4].x1 = 
segments[7].x1 = 
segments[8].x1 = 
segments[11].x1 =
segments[12].x1 =
segments[15].x1 = from_x;

segments[0].y1 =
segments[1].y1 =
segments[4].y1 =
segments[5].y1 =
segments[8].y1 =
segments[9].y1 =
segments[12].y1 =
segments[13].y1 = from_y;

segments[0].x2 =
segments[3].x2 =
segments[4].x2 =
segments[7].x2 =
segments[8].x2 =
segments[11].x2 =
segments[12].x2 =
segments[15].x2 = to_x;

segments[0].y2 =
segments[1].y2 =
segments[4].y2 =
segments[5].y2 =
segments[8].y2 =
segments[9].y2 =
segments[12].y2 =
segments[13].y2 = to_y;

segments[1].x1 =
segments[2].x1 =
segments[5].x1 =
segments[6].x1 =
segments[9].x1 =
segments[10].x1 =
segments[13].x1 =
segments[14].x1 = fromxplusfromwidth;

segments[1].x2 =
segments[2].x2 =
segments[5].x2 =
segments[6].x2 =
segments[9].x2 =
segments[10].x2 =
segments[13].x2 =
segments[14].x2 = toxplustowidth;

segments[2].y1 =
segments[3].y1 =
segments[6].y1 =
segments[7].y1 =
segments[10].y1 =
segments[11].y1 =
segments[14].y1 =
segments[15].y1 = fromyplusfromheight;

segments[2].y2 =
segments[3].y2 =
segments[6].y2 =
segments[7].y2 =
segments[10].y2 =
segments[11].y2 =
segments[14].y2 =
segments[15].y2 = toyplustoheight;

XSetFunction(XtDisplay(wm), wm-> wmstep.gc, GXinvert);
XDrawSegments(XtDisplay(wm), RootWindowOfScreen(XtScreen(wm)),
   wm-> wmstep.gc, segments, 16);
XSetFunction(XtDisplay(wm), wm-> wmstep.gc, GXcopy);

} /* end of DrawStreaks */

/*
 *************************************************************************
 * HeaderRect
 * - Called by DisplayWM() to fill in the XRectangle needed to draw
 * busy pattern in the header.
 ****************************procedure*header*****************************
 */

extern XRectangle
HeaderRect OLARGLIST((w))
OLGRA(WMStepWidget, w)
{
XRectangle rect;

rect.x      = BorderX(w);
rect.y      = BorderY(w);
rect.width  = BannerWid(w);
rect.height = BannerHt(w);

return (rect);

} /* end of HeaderRect */

/*
 *************************************************************************
 * ShowResizeFeedback.
 * - Open Look mode only - press select on resize corner, fill corner.
 ****************************procedure*header*****************************
 */
void
ShowResizeFeedback OLARGLIST((w, piece, on_off))
OLARG(WMStepWidget, w)
OLARG(WMPiece, piece)
OLGRA(int, on_off)
{
	static XRectangle rect[2];

	WMStepPart *wmstep = (WMStepPart *) &(w->wmstep);
	int	width = w->core.width,
		height = w->core.height;
	GC	nw_gc = (OlgIs3d()) ? OlgGetBrightGC(wmstep->mdm) 
			: OlgGetFgGC(wmstep->mdm), /* WhiteGC */
   		se_gc = (OlgIs3d()) ? OlgGetBg3GC(wmstep->mdm)
			: OlgGetBg1GC(wmstep->mdm); /* BlackGC */

	if (currentGUI != OL_OPENLOOK_GUI)
		return;

	if (on_off) {
		/* turn on feedback - highlight resize corner */
		switch(piece) {
			default: break;
			case WM_NW:
				rect[0].x = rect[0].y = 0;
				rect[0].width = CornerX(w);
				rect[0].height = Cornery(w);
				rect[1].x = rect[1].y = 0;
				rect[1].width = Cornerx(w);
				rect[1].height = CornerY(w);
				break;
			case WM_NE:
				/* This, like the southeast, is a little
				 * bigger than the west ones...
				 */
				rect[0].x = width - CornerX(w);
				rect[0].y = 0;
				rect[0].width = CornerX(w);
				rect[0].height = Cornery(w);
				rect[1].x = width - Cornerx(w) -
					LineWidx(w);
				rect[1].y = 0;
				rect[1].width = Cornerx(w) +
					LineWidx(w);
				rect[1].height = CornerY(w);
				break;
			case WM_SW:
				rect[0].x = 0;
				rect[0].y = height - CornerY(w);
				rect[0].width = Cornerx(w);
				rect[0].height = CornerY(w);
				rect[1].x = 0;
				rect[1].y = height - Cornery(w);
				rect[1].width = CornerX(w);
				rect[1].height = Cornery(w);
				break;
			case WM_SE:
				/* The southeast corner is done
				 * differently - it is a "little"
				 * bigger.
				 */
				rect[0].x = width - Cornerx(w) -
					LineWidx(w);
				rect[0].y = height - CornerY(w);
				rect[0].width = Cornerx(w) + LineWidx(w);
				rect[0].height = CornerY(w);
				rect[1].x = width - CornerX(w);
				rect[1].y = height - Cornery(w) -
					LineWidy(w);
				rect[1].width = CornerX(w);
				rect[1].height = Cornery(w) +
					LineWidy(w);
				break;
			} /* end switch */
		XFillRectangles(XtDisplay(w), XtWindow(w), se_gc, rect, 2);
	}
	else  {
		XRectangle   nw_corners[4];
		XRectangle   se_corners[4];
		int nw_i = 0,
		    se_i = 0,
		    compass_point = NW;
		/* got a button click, or a resize that didn't move... */
		XSetForeground(XtDisplay(w), wmstep->gc, 
					w->core.background_pixel);
		XFillRectangles(XtDisplay(w), XtWindow(w), wmstep->gc, rect, 2);
		XSetForeground(XtDisplay(w), wmstep->gc, wmstep->foreground_pixel);
		switch(piece) {
			case WM_NW:
				compass_point = NW;
				break;
			case WM_NE:
				compass_point = NE;
				break;
			case WM_SW:
				compass_point = SW;
				break;
			case WM_SE:
				compass_point = SE;
				break;
			default:
				break;
		} /* switch */
		CalcRects(w, wmstep-> metrics, compass_point, nw_corners, &nw_i,
							se_corners, &se_i);
		XFillRectangles(XtDisplay(w), XtWindow(w), nw_gc,
							nw_corners, nw_i);
		XFillRectangles(XtDisplay(w), XtWindow(w), se_gc,
							se_corners, se_i);
	} /* else (on_off == 0) */
		
} /* ShowResizeFeedback */

/*
 *************************************************************************
 * SetMotifMins.
 *
 ****************************procedure*header*****************************
 */
extern void
SetMotifMins()
{
int i;
	for (i=0; i < 12; i+= 3){
	  if (wmmetrics[i].scale == PermScale &&
			wmmetrics[i].resolution == ScreenResolution) {
		if (wmmetrics[i].compiled == 'N') {
			CompileMetrics(XtScreen(Frame), i);
			break;
		}
	  }
	}
	/* all else fails, compile the medium resolution */
	if (i >= 12 && wmmetrics[3].compiled == 'N') {
		i = 3;
		CompileMetrics(XtScreen(Frame), 3);
	}
	/* Compute minimums */
	Mresminwidth = 3 * wmmetrics[i].motifButtonWidth +
			2 * wmmetrics[i].motifHResizeBorderWidth;
	Mnoresminwidth = 3 * wmmetrics[i].motifButtonWidth +
			2 * wmmetrics[i].motifHNResizeBorderWidth;
	Mresminheight =  wmmetrics[i].motifButtonHeight +
			2 * wmmetrics[i].motifVResizeBorderWidth + 5;
	Mnoresminheight =  wmmetrics[i].motifButtonHeight +
			2 * wmmetrics[i].motifVNResizeBorderWidth + 5;
} /* SetMotifMins */

extern void
CreateDefaultIconMask()
{
Display *display = XtDisplay(Frame);
Window root_window = RootWindowOfScreen(XtScreen(Frame));
			default_mask  = XCreateBitmapFromData(display,
				root_window,
				(OLconst char *)deficon_mask_bits,
				deficon_mask_width, deficon_mask_height);

}
