/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:MotifIDec.c	1.28"
#endif

#include <stdio.h>

#include <X11/keysym.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/ShellP.h>
#include <X11/Shell.h>
#include <X11/Xatom.h>

#include <Xol/OpenLookP.h>
#include <Xol/DynamicP.h>
#include <Xol/Stub.h>
#include <Xol/Olg.h>
#include <Xol/WSMcomm.h>
#include <Xol/Flat.h>

#include <wm.h>
#include <Xol/OlCursors.h>
#include <Xol/VendorI.h>
#include <WMStepP.h>

/* Default icon */
#include <deficon.xpm>

#include <Extern.h>
#include <limits.h>

typedef struct _AIW {
	WMStepWidget wm;
	Widget motifAIW;
	int mapped;
} AIW;

static AIW motAIW;
/* Defines for drawing icons */

/* Make it 2 pixels for the icon border shadow (around the edge),
 * 2 pixels for the iconImage shadow (around the icon image,
 * inside the icon, and make the SPACING BETWEEN THESE 2 shadows
 * 2 pixels - that's a total of 5 pixels on EACH SIDE of the pixmap
 */
#define ICON_BORDER_SHADOW_WIDTH	2
/* Inside the icon */
/* Try makeing this 2 instead of 2 */
#define ICON_IMAGE_SHADOW_WIDTH	2

/* Padding around the icon pixmap (to the image shadow) */
#define PIXMAP_TOP_PAD		2
#define PIXMAP_BOTTOM_PAD	2
#define PIXMAP_LEFT_PAD		2
#define PIXMAP_RIGHT_PAD	2

#define PIXELS_BETWEEN_SHADOWS	2

/* Note: Image area itself - the width, height - is dependent on the
 * resources iconImageMax.
 * For the label area - some defines needed.  Note that the label part
 * here is not really related to when the icon has focus, because
 * when it has focus, it will be overlayered by a window.
 */
#define LABEL_TOP_PAD		2
#define LABEL_BOTTOM_PAD	2
#define LABEL_TEXT_PAD		2

#define ACTIVELABEL_WIDTH	100

#define MAXLABEL_CHARS		512

extern void InitIconDims OL_ARGS((Widget));
extern void DisplayMotifIcon OL_ARGS((WMStepWidget));

extern void MotifAIWButtonPress OL_ARGS((Widget, XtPointer, XEvent *,
					Boolean *));
extern void MotifAIWExpose OL_ARGS((Widget, XtPointer, XEvent *,
					Boolean *));
extern void MotifAIWLeave OL_ARGS((Widget, XtPointer, XEvent *,
					Boolean *));
static void MotifFillIconBackground OL_ARGS((WMStepWidget, Widget, Boolean));
extern void UnmapMotifAIW OL_NO_ARGS();


/*
 *************************************************************************
 *
 * Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 * InitIconDims.  Dimensions for motif style icons - screen wide.
 ****************************procedure*header*****************************
 */

void
InitIconDims OLARGLIST((w))
OLGRA(Widget, w)
{
Arg args[10];
int k = 0;
EventMask	event_mask;

	Display *display = XtDisplay(w);
	int	iWidth = 0,
		iHeight = 0,
		iLabelHeight = 0;
	unsigned int	requested_decor =
				(motwmrcs->iconDecoration &
				(ICON_IMAGE | ICON_LABEL));
	MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
						&(motCompRes[ICON_COMP]);
	MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *)
							&(mcri->compai);
	int fh = OlFontHeight( _OlGetDefaultFont(w,
	"-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1"),
		ol_font_list);

	iWidth = motwmrcs->iconImageMaximum.width +  /* image */
			PIXMAP_LEFT_PAD +
			PIXMAP_RIGHT_PAD +
			2 * ICON_BORDER_SHADOW_WIDTH + /* icon border */
			2 * ICON_IMAGE_SHADOW_WIDTH +
			2 * PIXELS_BETWEEN_SHADOWS;

	/* Watch here: the image area and label area may differ
	 * depending on the decorations requested.  Try to regard them
	 * as distinct pieces that may appear together in an icon
	 * containing both an image and a label.
	 */
	switch(requested_decor){
		case ICON_LABEL:
			/* Label only: keep the width the same. */
			iHeight = 0;

		iLabelHeight = 2 * ICON_BORDER_SHADOW_WIDTH +
				LABEL_TOP_PAD + fh + LABEL_BOTTOM_PAD;
			break;

		case ICON_IMAGE:
			/* Icon has no label, only pixmap */
			iHeight = 2 * ICON_BORDER_SHADOW_WIDTH +
				PIXMAP_TOP_PAD + 
				PIXMAP_BOTTOM_PAD + 
				2 * PIXELS_BETWEEN_SHADOWS +
				2 * ICON_IMAGE_SHADOW_WIDTH +
				motwmrcs->iconImageMaximum.height;
			break;
		/*  most common case, I hope! */
		case (ICON_IMAGE | ICON_LABEL):
			iHeight = 2 * ICON_BORDER_SHADOW_WIDTH +
				PIXMAP_TOP_PAD + 
				PIXMAP_BOTTOM_PAD + 
				2 * PIXELS_BETWEEN_SHADOWS +
				2 * ICON_IMAGE_SHADOW_WIDTH +
				motwmrcs->iconImageMaximum.height;

			iLabelHeight = 2 * ICON_IMAGE_SHADOW_WIDTH +
				LABEL_TOP_PAD + fh + LABEL_BOTTOM_PAD;

			break;
		default:
			iLabelHeight = iHeight = 0;
			break;
	} /* switch */
	mot_icon_iwidth = iWidth;
	mot_icon_iheight = iHeight;
	mot_icon_label_height = iLabelHeight;

	if (motwmrcs->iconDecoration & ICON_ACTIVELABEL){
		int alabelht = mot_icon_label_height == 0 ?
		   (LABEL_TOP_PAD + fh + 
		   LABEL_BOTTOM_PAD + 2 * ICON_BORDER_SHADOW_WIDTH) :
			mot_icon_label_height;
				
	XtSetArg(args[k], XtNx, (XtArgVal)-(ACTIVELABEL_WIDTH+20) );	k++;
	XtSetArg(args[k], XtNy, (XtArgVal) 0);		k++;
	XtSetArg(args[k], XtNwidth, (XtArgVal) ACTIVELABEL_WIDTH); k++;
	XtSetArg(args[k], XtNheight, (XtArgVal)alabelht); k++;
	XtSetArg(args[k], XtNborderWidth, (XtArgVal) 0);	k++;
	XtSetArg(args[k], XtNoverrideRedirect, (XtArgVal) True);	k++;
	XtSetArg(args[k], XtNmappedWhenManaged, (XtArgVal) False);	k++;
	XtSetArg(args[k], XtNbackground,
			(XtArgVal) mcai->activeBackground);  k++;
	/* motifAIW is external (global) to olwm */
	motifAIW = motAIW.motifAIW = XtCreateApplicationShell("label",
		shellWidgetClass,
		args, k);
	XtRealizeWidget(motAIW.motifAIW);
	XtMapWidget(motAIW.motifAIW);
	
	XDefineCursor(XtDisplay(w), XtWindow(motAIW.motifAIW),
					motifWindowCursor);

	event_mask = ButtonPressMask | ButtonReleaseMask;
	XtAddEventHandler(motAIW.motifAIW, event_mask,
			False, MotifAIWButtonPress, (XtPointer) NULL);
	XtAddEventHandler(motAIW.motifAIW, ExposureMask, False,
		 MotifAIWExpose, (XtPointer)NULL);
	XtAddEventHandler(motAIW.motifAIW, LeaveWindowMask, False,
		 MotifAIWLeave, (XtPointer)NULL);
	} /* if activelabel */

} /* InitIconDims */

/*
 *************************************************************************
 * DisplayMotifIcon.  Draw Motif Mode icon.
 ****************************procedure*header*****************************
 */
void
DisplayMotifIcon OLARGLIST((wm))
OLGRA(WMStepWidget,wm)
{
	XRectangle	lightrect[1], darkrect[1];
	WMStepPart	*wmstep = (WMStepPart *)&(wm->wmstep);
	Display		*display = XtDisplay((Widget)wm);
	Screen		*screen = XtScreen((Widget)wm);
	Window		window = (wmstep->icon_widget)?
				XtWindow(wmstep->icon_widget) : NULL;
	int		iwidth,
			iheight;
	int		iWidth, /* pixmap (only) width */
			iHeight; /* pixmap height */
	GC		pixgc = (GC)NULL;
	XGCValues	gcvals;
	int		x, y;
	unsigned int	width, height, bw, depth;
	Window		root;
	unsigned int	requested_decor =
				(motwmrcs->iconDecoration &
				(ICON_IMAGE | ICON_LABEL ));
	unsigned int	gcmask;
	/* Variable for drawing the icon label */
	char		*text = wmstep->icon_name;
	char		textprint[MAXLABEL_CHARS];
	char		*utext = (char *)&(textprint[0]);
	int		textlen = strlen(text);
	int		textarea, text_y;
	OlgTextLbl	labeldata;
	Dimension	labelwidth, labelheight;
	Boolean		active = wmstep->is_current;
	MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
						&(motCompRes[ICON_COMP]);
	MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *)
							&(mcri->compai);
	OlgAttrs		*label_mdm = (OlgAttrs *)NULL;	/* local */

	if (!window)
		return;
	/* Set up for drawing the icon label, if necessary */
	switch(requested_decor) {
		default:
			break;
		case ICON_ACTIVELABEL:
		case ICON_LABEL:
		case (ICON_IMAGE | ICON_LABEL):
			/* Set up fonts */
			if (mcai->fontList)
				labeldata.font_list = mcai->fontList;
			else
				labeldata.font_list = ol_font_list;
			labeldata.font =  _OlGetDefaultFont((Widget)wm,
"-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1");

			labeldata.accelerator = NULL;
			labeldata.mnemonic = NULL;
			labeldata.flags = (unsigned char) NULL;
			labeldata.label = text;
			labeldata.justification = TL_CENTER_JUSTIFY;
			label_mdm = OlgCreateAttrs(screen,
				active ? mcai->activeForeground :
					 mcai->foreground,
				active ? (OlgBG *)&(mcai->activeForeground) :
					 (OlgBG *)&(mcai->foreground),
				(Boolean)FALSE, (Dimension)12);
			labeldata.normalGC = DefaultDepthOfScreen(screen)
					!= 1 ? OlgGetFgGC(label_mdm) :
						OlgGetBg1GC(label_mdm);
			/* I use the above statement for the normalGC
			 * because I found out that things work very
			 * differently in monochrome.
			 */
	
			if (labeldata.font_list == NULL)
				XSetFont(display, labeldata.normalGC,
					labeldata.font->fid);

			/* Unused, but set to prevent problems */
			labeldata.inverseGC = labeldata.normalGC;

			OlgSizeTextLabel(screen, label_mdm,
				 &labeldata, &labelwidth, &labelheight);
			textarea = mot_icon_iwidth -
				2 * ICON_BORDER_SHADOW_WIDTH -
				2 * LABEL_TEXT_PAD;

              if ( (!(motwmrcs->iconDecoration & ICON_ACTIVELABEL)) &&
					((int)labelwidth > (int)(textarea)
                                        ) ) {
                                int maxwidth = textarea;
                                int tlen;
				int pass = 1;
                                char *p;
                                strncpy((char *)textprint, text, MAXLABEL_CHARS);
                                tlen = strlen((char *)textprint);
                                p = &(textprint[tlen-1]);

				labeldata.label =utext;

                                while ((int)labelwidth >= maxwidth) {
                                        *p = '\0'; p--;
                        		OlgSizeTextLabel(screen, label_mdm,
                                 	  &labeldata, &labelwidth, &labelheight);
                                } /* while */

				/* Remove trailing white space */
				if (isspace(*p)) {
					pass = 0;
                                	while (isspace(*p)) {
                                        	*p = '\0'; p--;
					}
				}
				if (!pass)
                        		OlgSizeTextLabel(screen, label_mdm,
                                 &labeldata, &labelwidth, &labelheight);
                        } /* if */


			/* fall through if necessary, but skip above if
			 * image only.
			 */
		case ICON_IMAGE:
			/* IF iconbgGC is NULL, then we can just do
			 * a setvalues on the icon window background
			 * and save the headaches; but if the iconbgGC is not
			 * null, then it probably has a stupid background
			 * pixmap, and we have to fill rectangles just to
			 * fill in the background.  Either way, we can do the
			 * setvalues with the correct background color.
			 */

			if (wmstep->icon_widget) {
				XRectangle	rect[1];
				int x, y;
				/* Make icon correct background color */

				MotifFillIconBackground(wm,wmstep->icon_widget,
						active);

			/* Fill in image part only with resource color */


			} /* if (wmstep->icon_widget) */
				break;
	} /* switch */

 	if (wmstep->icon_widget) {
		iwidth = wmstep->icon_widget->core.width;
		iheight = wmstep->icon_widget->core.height;
	}

/* We have two different icon drawings, though both are the same size: one
 * for the non-selected (not current), and one for the current (selected).
 * Start with the non-selected icons; the outer shadow is out, the inner
 * one is in;  calculate the width/height the same way each time.
 * The main difference between motif and open look is that there are
 * three possibilities with motif, and therefore three possible ways
 * to size the icons on a global basis.
 */
	if (1) {
		switch(requested_decor){
			case (ICON_LABEL):
				break;
			case (ICON_IMAGE | ICON_LABEL):
			case (ICON_IMAGE | ICON_ACTIVELABEL):
			case ICON_IMAGE:
				/* Draw inner shadow around pixmap - this is
				 * the iconImage[Top|Bottom] GCs
				 */
				_OlgDrawBorderShadow(XtScreen(wm), window, 
					wm->wmstep.mdm, OL_SHADOW_ETCHED_IN, 
					ICON_IMAGE_SHADOW_WIDTH,
			ICON_BORDER_SHADOW_WIDTH + PIXELS_BETWEEN_SHADOWS,
			ICON_BORDER_SHADOW_WIDTH + PIXELS_BETWEEN_SHADOWS,
					motwmrcs->iconImageMaximum.width +
				PIXMAP_LEFT_PAD + PIXMAP_RIGHT_PAD +
						2 * ICON_IMAGE_SHADOW_WIDTH,
					motwmrcs->iconImageMaximum.height +
				PIXMAP_TOP_PAD + PIXMAP_BOTTOM_PAD +
						2 * ICON_IMAGE_SHADOW_WIDTH,
					active ? mcri->activetopGC :
					 wmstep->iconimagetopGC,
					active ? mcri->activebotGC :
					 wmstep->iconimagebotGC);
				/* Draw dividing line between pixmap and
				 * label.  Dark over light.
				 */
				lightrect[0].height = darkrect[0].height =
					darkrect[0].x = lightrect[0].x = 1;
				darkrect[0].width = lightrect[0].width =
							 iwidth - 2;
				lightrect[0].y =
				  (darkrect[0].y = mot_icon_iheight - 1) + 1;
				XFillRectangles(display, window,
					active ? mcri->activebotGC :
						mcri->botGC, darkrect, 1);
				XFillRectangles(display, window, 
					active ? mcri->activetopGC :
						mcri->topGC, lightrect, 1);
				/* Now all I have to do is fill in the
				 * pixmap, and the label, too.  Then break.
				 * Is a pixmap or bitmap supplied, or is
				 * a window supplied, or nothing. (Forget
				 * about colors for now).
				 */
				if (!(wmstep->icon_pixmap)) {
				if (wmstep->xwmhints->flags&IconWindowHint&&
				    wmstep->xwmhints->icon_window)
					/* Do nothing - this should already
					 * be reparented to the correct spot
					 * in the window - it may just need
					 * the extra internal shadow.
					 */
					break;
				else
				  if (!(wmstep->csinfo->useClientIcon) &&
					(XGetGeometry(display,
					 wmstep->csinfo->iconImage,
					  &root, &x, &y, &width,
					  &height, &bw, &depth)) ) {

						/* Use csinfo.iconImage */
					  int iWidth, iHeight;
					  Pixmap pix;
					  XGCValues gcvals;

					iWidth =
				motwmrcs->iconImageMaximum.width +
				PIXMAP_LEFT_PAD + PIXMAP_RIGHT_PAD;
					iHeight =
				motwmrcs->iconImageMaximum.height +
				PIXMAP_TOP_PAD + PIXMAP_BOTTOM_PAD;
					pix = XCreatePixmap(display,
				RootWindowOfScreen(XtScreen(wm)),
				iWidth, iHeight, DefaultDepthOfScreen(
							XtScreen((Widget)wm)) );

				/* Set up background, foreground */
				gcvals.foreground =
					wmstep->csinfo->iconImageBackground;
				pixgc = XtGetGC((Widget)wm, GCForeground,
			 					&gcvals);

				/* This makes the whole pixmap background
				 * the iconImageBackground color (pixmap).
				 */
				XFillRectangle(display, pix, pixgc, 0, 0,
					iWidth, iHeight);
				XtReleaseGC((Widget)wm, pixgc);
				pixgc = NULL;
				/* I can try and center the pixmap - hope
				 * it looks O.K. if the size is bad!
				 */
				/* width, height were returned by XGetGeometry
				 */
				width = MIN(width, motwmrcs->
						iconImageMaximum.width);
				height = MIN(height, motwmrcs->
						iconImageMaximum.height);
				/* This is the pixmap we want with the
				 * colors we want - copy the bitmap to
				 * the pixmap with XCopyPlane - if it
				 * was a pixmap, then use XCopyArea.
				 * Center it in any extra space there is.
				 */
				if (depth != 1 && depth ==
				   DefaultDepthOfScreen(XtScreen(wm))){
                                gcmask = GCForeground|GCBackground;
                                gcvals.foreground =
                                          wmstep->csinfo->iconImageForeground;
                                        gcvals.background =
                                          wmstep->csinfo->iconImageBackground;

/* Don't use the icon mask supplied by the client - may not look good..
                        if ((wmstep-> xwmhints-> flags & IconMaskHint) &&
                                        wmstep-> xwmhints-> icon_mask) {
                                gcvals.clip_mask = wmstep->xwmhints->icon_mask;
                                gcmask |= GCClipMask;
                                gcvals.clip_x_origin =
					(iWidth - width) / 2;
				gcvals.clip_y_origin = (iHeight - height)/2; 
                                gcmask |= GCClipXOrigin | GCClipYOrigin;
                        }
 */
                                        pixgc = XtGetGC((Widget)wm,
                                                gcmask, &gcvals);

               				XCopyArea(display,
						wmstep->csinfo->iconImage,
						pix, pixgc, 0, 0, width,
						height,  (iWidth - width) / 2,
						(iHeight - height) / 2);
				}
				else {
					gcvals.foreground = 
					  wmstep->csinfo->iconImageForeground;
					gcvals.background = 
					  wmstep->csinfo->iconImageBackground;
                                       gcmask = GCForeground|GCBackground;
/* Once again, don't use this mask...
                        if ((wmstep-> xwmhints-> flags & IconMaskHint) &&
                                        wmstep-> xwmhints-> icon_mask) {
                                gcvals.clip_mask = wmstep->xwmhints->icon_mask;
                                gcmask |= GCClipMask;
                                gcvals.clip_x_origin = (iWidth - width)/2;
				gcvals.clip_y_origin= (iHeight - height)/2;
                                gcmask |= GCClipXOrigin | GCClipYOrigin;
                        }
 */
				pixgc = XtGetGC((Widget)wm,
					gcmask, &gcvals);

					XCopyPlane(display,
			wmstep->csinfo->iconImage, pix, pixgc, 0, 0,
			width, height, (iWidth - width)/2,
			(iHeight - height) / 2, 1L);
				} /* else it's a bitmap */
				XtReleaseGC((Widget)wm, pixgc);
				pixgc = NULL;
				wmstep->icon_pixmap = pix;
				  } /* useClientIcon */
					else
				 if((wmstep->xwmhints->flags&IconPixmapHint)
					&&(wmstep->xwmhints->icon_pixmap) &&
					(XGetGeometry(display,
						wmstep->xwmhints->icon_pixmap,
						&root, &x, &y, &width,
						&height, &bw, &depth)) ) {
					  int iWidth, iHeight;
					  Pixmap pix;
					  XGCValues gcvals;
					/* bitmap/pixmap supplied.  To adhere
					 * to size and resource color specs,
					 * create a pixmap to
					 * hold the image part.  Watch
					 */
					iWidth =
				motwmrcs->iconImageMaximum.width +
				PIXMAP_LEFT_PAD + PIXMAP_RIGHT_PAD;
					iHeight =
				motwmrcs->iconImageMaximum.height +
				PIXMAP_TOP_PAD + PIXMAP_BOTTOM_PAD;
					pix = XCreatePixmap(display,
				RootWindowOfScreen(XtScreen(wm)),
				iWidth, iHeight, DefaultDepthOfScreen(
							XtScreen(wm)) );

				/* Set up background, foreground */
				gcvals.foreground =
					wmstep->csinfo->iconImageBackground;
				pixgc = XtGetGC((Widget)wm, GCForeground,
			 					&gcvals);

				/* This makes the whole pixmap background
				 * the iconImageBackground color (pixmap).
				 */
				XFillRectangle(display, pix, pixgc, 0, 0,
					iWidth, iHeight);
				XtReleaseGC((Widget)wm, pixgc);
				pixgc = NULL;
				/* I can try and center the pixmap - hope
				 * it looks O.K. if the size is bad!
				 */
				/* width, height were returned by XGetGeometry
				 */
				width = MIN(width, motwmrcs->
						iconImageMaximum.width);
				height = MIN(height, motwmrcs->
						iconImageMaximum.height);
				/* This is the pixmap we want with the
				 * colors we want - copy the bitmap to
				 * the pixmap with XCopyPlane - if it
				 * was a pixmap, then use XCopyArea.
				 * Center it in any extra space there is.
				 */
				if (depth != 1 && depth ==
					   DefaultDepthOfScreen(XtScreen(wm))){
                                gcmask = GCForeground|GCBackground;
                                gcvals.foreground =
                                          wmstep->csinfo->iconImageForeground;
                                        gcvals.background =
                                          wmstep->csinfo->iconImageBackground;

                        if ((wmstep-> xwmhints-> flags & IconMaskHint) &&
                                        wmstep-> xwmhints-> icon_mask) {
                                gcvals.clip_mask = wmstep->xwmhints->icon_mask;
                                gcmask |= GCClipMask;
                                gcvals.clip_x_origin =
					(iWidth - width) / 2;
				gcvals.clip_y_origin = (iHeight - height)/2; 
                                gcmask |= GCClipXOrigin | GCClipYOrigin;
                        }
                                        pixgc = XtGetGC((Widget)wm,
                                                gcmask, &gcvals);

               				XCopyArea(display,
						wmstep->xwmhints->icon_pixmap,
						pix, pixgc, 0, 0, width,
						height,  (iWidth - width) / 2,
						(iHeight - height) / 2);
				}
				else {
					gcvals.foreground = 
					  wmstep->csinfo->iconImageForeground;
					gcvals.background = 
					  wmstep->csinfo->iconImageBackground;
                                       gcmask = GCForeground|GCBackground;
                        if ((wmstep-> xwmhints-> flags & IconMaskHint) &&
                                        wmstep-> xwmhints-> icon_mask) {
                                gcvals.clip_mask = wmstep->xwmhints->icon_mask;
                                gcmask |= GCClipMask;
                                gcvals.clip_x_origin = (iWidth - width)/2;
				gcvals.clip_y_origin= (iHeight - height)/2;
                                gcmask |= GCClipXOrigin | GCClipYOrigin;
                        }
                                        pixgc = XtGetGC((Widget)wm,
                        gcmask, &gcvals);

					XCopyPlane(display,
			wmstep->xwmhints->icon_pixmap, pix, pixgc, 0, 0,
			width, height, (iWidth - width)/2,
			(iHeight - height) / 2, 1L);
				} /* else it's a bitmap */
				XtReleaseGC((Widget)wm, pixgc);
				pixgc = NULL;
				wmstep->icon_pixmap = pix;
				  } /*  supplied bitmap */
				else { /* nothing supplied, use default */
					  int iWidth, iHeight;
					  Pixmap pix;
					  XGCValues gcvals;
				if (default_icon == (Pixmap)NULL) {
					/* create default icon pixmap - all
					 * I have is a motif style bitmap;
					 * create a pixmap, copy.
					 */
/*
					default_icon = XCreateBitmapFromData(
				display, RootWindowOfScreen(XtScreen(wm)), 
						(char *)def_motif_icon_bits,
						def_motif_icon_width,
						def_motif_icon_height);
 - Try that new icon...
 */
                        default_icon = XCreatePixmapFromData( display,
                                RootWindowOfScreen(XtScreen(wm)),
				DefaultColormapOfScreen(screen),
                                deficon_width, deficon_height,
                                DefaultDepthOfScreen(screen),
                                deficon_ncolors, deficon_chars_per_pixel,
                                deficon_colors, deficon_pixels);
                        default_icon_width = deficon_width;
                        default_icon_height = deficon_height;
			/* Initialize default_icon_mask */
			CreateDefaultIconMask();

				} /* default_icon == NULL */

				/* Create pixmap for this icon with clients
				 * specific colors - such a waste (management)
				 * if all are same color.
				 */

					iWidth =
				motwmrcs->iconImageMaximum.width +
				PIXMAP_LEFT_PAD + PIXMAP_RIGHT_PAD;

					iHeight =
				motwmrcs->iconImageMaximum.height +
				PIXMAP_TOP_PAD + PIXMAP_BOTTOM_PAD;

					pix = XCreatePixmap(display,
				RootWindowOfScreen(XtScreen(wm)),
				iWidth, iHeight, DefaultDepthOfScreen(
							XtScreen(wm)));

				/* Set up background, foreground */
				gcvals.foreground =
					wmstep->csinfo->iconImageBackground;
				pixgc = XtGetGC((Widget)wm, GCForeground,
							&gcvals);
				XFillRectangle(display, pix, pixgc, 0, 0,
					iWidth, iHeight);
				XtReleaseGC((Widget)wm, pixgc);
				pixgc = NULL;
				/* I can try and center the pixmap - hope
				 * it looks O.K. if the size is bad!
				 */
/*
				width = MIN(def_motif_icon_width, motwmrcs->
			iconImageMaximum.width);
				height = MIN(def_motif_icon_height, motwmrcs->
			iconImageMaximum.height);
 */
				width = MIN(deficon_width, motwmrcs->
			iconImageMaximum.width);
				height = MIN(deficon_height, motwmrcs->
			iconImageMaximum.height);
				/* This is the pixmap we want with the
				 * colors we want - copy the bitmap to
				 * the pixmap with XCopyPlane - if it
				 * was a pixmap, then use XCopyArea.
				 * Center it in any extra space there is.
				 */
				/* First set up a different GC, necessary
				 * for a successful XCopyPlane with the
				 * desired colors.
				 */
		{
			unsigned long pixmask = GCForeground|GCBackground;
				gcvals.foreground =
					wmstep->csinfo->iconImageForeground;
				gcvals.background =
					wmstep->csinfo->iconImageBackground;
				if (default_mask) {
				gcvals.clip_mask = default_mask;
			pixmask |= GCClipMask;
			gcvals.clip_x_origin = (iWidth - deficon_width)/2;
			gcvals.clip_y_origin = (iHeight - deficon_height)/2;
			pixmask |= GCClipXOrigin | GCClipYOrigin;
				}
				pixgc = XtGetGC((Widget)wm,
						pixmask,
						&gcvals);
		}
/*
				XCopyPlane(display, default_icon, pix,
					pixgc, 0, 0, def_motif_icon_width,
					def_motif_icon_height,
					(iWidth - def_motif_icon_width)/2,
					(iHeight - def_motif_icon_height) / 2,
					1L);
 * Again (mlp) - try the new icon * 
 */
	{ /* start block */
	int retx, rety;
	unsigned int retwidth, retheight, retbw, depth;
	Window root;
        XGetGeometry(display, default_icon, &root,
            &retx, &rety, &retwidth, &retheight, &retbw, &depth);

		if (depth == 1 || depth != wm->core.depth) {
               		 XCopyPlane(display, default_icon, pix, pixgc,
                       		 0, 0, deficon_width, deficon_height,
					(iWidth - deficon_width)/2,
					(iHeight - deficon_height)/2, 1L);
        	}
        	else
			XCopyArea(display, default_icon, pix, pixgc,
				0, 0, deficon_width, deficon_height,
				(iWidth - deficon_width)/2,
				(iHeight - deficon_height)/2);
	}	 /* end block */

				XtReleaseGC((Widget)wm, pixgc);
				pixgc = NULL;
				wmstep->icon_pixmap = pix;
				} /* else (use default bitmap) */
			   } /* icon_pixmap == NULL */

			/* Copy the pixmap to window (icon widget) */
			x = y =  ICON_BORDER_SHADOW_WIDTH +
				PIXELS_BETWEEN_SHADOWS +
				ICON_IMAGE_SHADOW_WIDTH;
			iWidth = motwmrcs->iconImageMaximum.width +
			  PIXMAP_LEFT_PAD + PIXMAP_RIGHT_PAD;
			iHeight = motwmrcs->iconImageMaximum.height +
			  PIXMAP_TOP_PAD + PIXMAP_BOTTOM_PAD;

			gcvals.function = GXcopy;
			gcmask = GCFunction;
			
			pixgc = XtGetGC((Widget)wm, gcmask, &gcvals);

		if (DefaultDepthOfScreen(screen) == 1)
               		 XCopyPlane(display, wmstep->icon_pixmap, window, pixgc,
                       		 0, 0, iWidth, iHeight, x, y, 1L);
        	else
			XCopyArea(display, wmstep->icon_pixmap, window,
					    pixgc, 0, 0, iWidth, iHeight, x, y);
		XtReleaseGC((Widget)wm, pixgc);
		pixgc = NULL;
				    break;
			    default:
				    break;
		    } /* switch */

		     if (motwmrcs->iconDecoration & ICON_ACTIVELABEL){
				    MoveAIW(wm);
		    }
			    if ( (motwmrcs->iconDecoration & ICON_ACTIVELABEL &&
				     motwmrcs->iconDecoration & ICON_LABEL &&
					    !(wmstep->is_current) ) ||
			    (!(motwmrcs->iconDecoration&ICON_ACTIVELABEL) &&
				    (motwmrcs->iconDecoration&ICON_LABEL))) {

				    /* Draw the icon label.  It is
				     * irrelevant whether or not the icon
				 * is current, because the background
				 * is taken care of.
				 */

				text_y = wmstep->icon_widget->core.height -
					mot_icon_label_height;

				/* First, clear out the area where you
				 * are drawing the label - if there is
				 * a pixmap.
				 */
			{ /* start block */
				int temp, mint, usew, usex;
		mint = ICON_BORDER_SHADOW_WIDTH + LABEL_TEXT_PAD;
					temp = (icon_width -
						labelwidth) / 2;
					if (temp > mint)
						usex = temp;
					else usex =  mint;
					usew = icon_width - 2 * mint;
					if ((int)labelwidth < usew)
						usew = labelwidth;

				if (mcai->backgroundPixmap)
				   XClearArea(display, window,
					usex,
					text_y, usew,
					labelheight, False);

				OlgDrawTextLabel(XtScreen(wm), window,
					label_mdm, 
		ICON_BORDER_SHADOW_WIDTH + LABEL_TEXT_PAD, 
					text_y, textarea,
					mot_icon_label_height -
					LABEL_TOP_PAD - LABEL_BOTTOM_PAD -
					ICON_BORDER_SHADOW_WIDTH, &labeldata);
			} /* end block */
			} /* else if icon label */
	} /* if 1 */
	if (label_mdm)
		OlgDestroyAttrs(label_mdm);
} /* DisplayMotifIcon */


/*
 * MoveAIW.
 *  Move the active icon label window someplace else, possibly off
 *  the screen.
 */
void
MoveAIW OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
Display  *display = XtDisplay((Widget)wm);
XWindowChanges xwc;
int xpos, ypos;
int aiwx, aiwy;
int screenwidth = WidthOfScreen(XtScreen((Widget)wm));
int screenheight = HeightOfScreen(XtScreen((Widget)wm));

	/* This first if is to see if a window that had the active
	 * label doesn't want it anymore.
	 */
	if ( motAIW.wm == wm &&
			((!(IsIconic(wm))) || !(wmstep->is_current)) ) {
		if (motAIW.mapped) {
			XtUnmapWidget(motAIW.motifAIW);
			motAIW.wm = NULL;
			motAIW.mapped = 0;
		}
		return;
	}
	if (!(wmstep->is_current))
		/* Possibly motAIW.wm == NULL */
		return;
	aiwx = motAIW.motifAIW->core.x;
	aiwy = motAIW.motifAIW->core.y;

	xpos = wmstep->icon_widget->core.x + mot_icon_iwidth /2 -
		ACTIVELABEL_WIDTH / 2;
	if (xpos < 0)
		xpos = 0;
	else
		if (xpos > screenwidth)
			xpos = screenwidth - ACTIVELABEL_WIDTH;

	ypos = wmstep->icon_widget->core.y + mot_icon_iheight;
	if ((int)(ypos + motAIW.motifAIW->core.height) > (int)screenheight)
		ypos =  screenheight - motAIW.motifAIW->core.height;
	if (xpos != motAIW.motifAIW->core.x ||
			ypos != motAIW.motifAIW->core.y) {
		/* Two cases: if it's the same icon, just move it,
		 * don't unmap it; if it's a different icon, unmap
		 * and remap it.
		 */
		if (motAIW.wm != wm && motAIW.mapped) {
			motAIW.mapped = 0;
			XtUnmapWidget(motAIW.motifAIW);
		}
		XtMoveWidget(motAIW.motifAIW, xpos, ypos);
		xwc.sibling = XtWindow(wmstep->icon_widget);
		xwc.stack_mode = Above;
		XConfigureWindow(display,XtWindow(motAIW.motifAIW),
		 	CWSibling | CWStackMode , &xwc);
		if (!(motAIW.mapped)) {
			XtMapWidget(motAIW.motifAIW);
			motAIW.mapped++;
		}
	}
	else {
		/* AIW positions check out OK, make sure it's mapped
		 * and in proper stacking order.
		 */
		xwc.sibling = XtWindow(wmstep->icon_widget);
		xwc.stack_mode = Above;
		XConfigureWindow(display,XtWindow(motAIW.motifAIW),
		 	CWSibling | CWStackMode , &xwc);
		if (!(motAIW.mapped)) {
			XtMapWidget(motAIW.motifAIW);
			motAIW.mapped++;
		}
	} /* else */
	motAIW.wm = wm;
}

/*
 * MotifAIWExpose.
 * Expose event processing for the active label window
 */
void
MotifAIWExpose OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget,		w)
OLARG(XtPointer,	client_data)
OLARG(XEvent *,		event)
OLGRA(Boolean *,	cont_to_disp)
{
int		textarea;
int		text_y = LABEL_TOP_PAD;
OlgAttrs	*label_mdm = (OlgAttrs *)NULL;	/* local */
OlgTextLbl	labeldata;
Dimension	labelwidth, labelheight;
WMStepWidget	wm = motAIW.wm;
WMStepPart	*wmstep;
Screen		*screen = XtScreen(w);
char		*text;
MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
					&(motCompRes[ICON_COMP]);
MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *)
						&(mcri->compai);
XGCValues	gcvals;

	if (!wm)
		return;
	if (event && event->xexpose.count != 0)
		return;

        wmstep = (WMStepPart *)&(wm->wmstep);
        text = wmstep->icon_name;

	MotifFillIconBackground(wm, motAIW.motifAIW, (Boolean)True);

	labeldata.font =  _OlGetDefaultFont(w,
"-misc-fixed-medium-r-semicondensed--13-120-75-75-c-60-iso8859-1");

	if (mcai->fontList)
		labeldata.font_list = mcai->fontList;
	else
		labeldata.font_list = ol_font_list;
	labeldata.accelerator = NULL;
	labeldata.mnemonic = NULL;
	labeldata.flags = (unsigned char) NULL;
	labeldata.label = text;
	labeldata.justification = TL_CENTER_JUSTIFY;
	label_mdm = OlgCreateAttrs(screen,
				mcai->activeForeground,
				(OlgBG *)&(mcai->activeForeground),
				(Boolean)FALSE, (Dimension)12);
/*
	labeldata.normalGC = (GC)NULL;
	gcvals.foreground = mcai->activeForeground;
	labeldata.normalGC = XtGetGC(motAIW.motifAIW, GCForeground,
				&gcvals);
 */
	labeldata.normalGC = DefaultDepthOfScreen(screen) != 1 ?
				OlgGetFgGC(label_mdm) :
				OlgGetBg1GC(label_mdm);

	if (labeldata.font)
		XSetFont(XtDisplay(w), labeldata.normalGC, labeldata.font->fid);

	/* Unused, but prevents problems */
	labeldata.inverseGC = labeldata.normalGC;

	OlgSizeTextLabel(screen, label_mdm,
				 &labeldata, &labelwidth, &labelheight);

	textarea = ACTIVELABEL_WIDTH - 2 * ICON_BORDER_SHADOW_WIDTH -
			2 * LABEL_TEXT_PAD;
	OlgDrawTextLabel(XtScreen(wm), XtWindow(motAIW.motifAIW),
			label_mdm, ICON_BORDER_SHADOW_WIDTH + LABEL_TEXT_PAD,
			text_y, textarea,
			labelheight, &labeldata);
	OlgDestroyAttrs(label_mdm);
	label_mdm = NULL;

/*
	if (labeldata.normalGC)
		XtReleaseGC(motAIW.motifAIW, labeldata.normalGC);
 */
}

/*
 * MotifAIWButtonPress.
 * Button press event processing for the active label window.
 */

void
MotifAIWButtonPress OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget,		w)
OLARG(XtPointer,	client_data)
OLARG(XEvent *,		event)
OLGRA(Boolean *,	cont_to_disp)
{
int	src_x, src_y;
int	dest_x, dest_y;
Window	child_win;
Display *display = XtDisplay(w);
Widget	icon_widget;

	/* This could happen if clicking very fast on this label -
	 * remember, unlike the icon, this piece is not destroyed.
	 */
	if (motAIW.wm == (WMStepWidget)NULL) {
		return;
	}

	/* Get the equivalent coordinate within the icon widget,
	 * rather than the motifAIW widget.
	 */
	icon_widget = motAIW.wm->wmstep.icon_widget;
	XTranslateCoordinates(display, XtWindow(w),
		XtWindow(icon_widget),
		event->xbutton.x, event->xbutton.y, 
		&dest_x, &dest_y, &child_win);
	/* Now go to work on the icon widget */
	event->xbutton.x = dest_x;
	event->xbutton.y = dest_y;
	IconButtonPress(w, (XtPointer)(motAIW.wm), event, cont_to_disp);
}

void
UnmapMotifAIW()
{
	XtUnmapWidget(motAIW.motifAIW);
}


void
CheckAIWDown OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
	if (motAIW.wm == wm) {
		XtUnmapWidget(motAIW.motifAIW);
		motAIW.mapped = 0;
		motAIW.wm = NULL;
	}

} /* CheckAIWDown */

extern void
StackAIW OLARGLIST((wm))
OLGRA(WMStepWidget, wm)
{
XWindowChanges xwc;
Display *display = XtDisplay(motAIW.motifAIW);
WMStepWidget motwm = motAIW.wm;
WMStepPart *wmstep;

	if (motwm == wm) {
		if (wm->wmstep.icon_widget) {
			xwc.sibling = XtWindow(wm->wmstep.icon_widget);
			xwc.stack_mode = Above;
			XConfigureWindow(display, XtWindow(motAIW.motifAIW),
		 		CWSibling | CWStackMode , &xwc);
			}
	}
}

/*
 * LowerAIWGroup.
 * Lower the active label to the bottom of the stack, icon window and all
 */
extern void
LowerAIWGroup OLARGLIST((wm, window))
OLARG(WMStepWidget, wm)
OLGRA(Window, window)
{
XWindowChanges  xwc;
Display         *display = XtDisplay(motAIW.motifAIW);
WMStepWidget    motwm = motAIW.wm;
WMStepPart      *wmstep;
Widget		icon_widget = (Widget)NULL;

        if (motAIW.wm == wm) {
		wmstep = (WMStepPart *)&(wm->wmstep);
		if ( (icon_widget = wmstep->icon_widget) != NULL) {
			if (!window)
                        	XLowerWindow(display, XtWindow(icon_widget));
			else {
				xwc.sibling = window;
                        	xwc.stack_mode = Below;
				XConfigureWindow(display, XtWindow(motAIW.motifAIW),
		 			CWSibling | CWStackMode , &xwc);
			}
                        xwc.sibling = XtWindow(icon_widget);
                        xwc.stack_mode = Above;
			XConfigureWindow(display, XtWindow(motAIW.motifAIW),
		 		CWSibling | CWStackMode , &xwc);
		}
	}
} /* LowerAIWGroup */

/*
 * RaiseAIWGroup.
 * Raise the active label to the top of the stack, then
 * configure the icon widget below it.
 */
extern void
RaiseAIWGroup OLARGLIST((wm, window))
OLARG(WMStepWidget, wm)
OLGRA(Window, window)
{
XWindowChanges  xwc;
Display         *display = XtDisplay(motAIW.motifAIW);
WMStepWidget    motwm = motAIW.wm;
WMStepPart      *wmstep;
Widget		icon_widget = (Widget)NULL;

        if (motAIW.wm == wm) {
		wmstep = (WMStepPart *)&(wm->wmstep);
		if ( (icon_widget = wmstep->icon_widget) != NULL) {
			if (!window)
                        	XRaiseWindow(display, XtWindow(motAIW.motifAIW)); 
			else {
                        	xwc.sibling = window;
                        	xwc.stack_mode = Above;
				XConfigureWindow(display, XtWindow(icon_widget),
		 			CWSibling | CWStackMode , &xwc);
			}
                        xwc.sibling = XtWindow(motAIW.motifAIW);
                        xwc.stack_mode = Below;
			XConfigureWindow(display, XtWindow(icon_widget),
		 		CWSibling | CWStackMode , &xwc);
		}
	}
} /* RaiseAIWGroup */

/*
 *************************************************************************
 * MotifFillFocusBackground.
 * Set the color of the window background to either the non-active window
 * color or the active window background color.
 * Boolean flag - tells me if the icon is current.
 *
 ****************************procedure*header*****************************
 */
static void
MotifFillIconBackground OLARGLIST((wm, w, flag))
OLARG(WMStepWidget, wm)
OLARG(Widget, w)
OLGRA(Boolean, flag)
{
WMStepPart *wmstep = (WMStepPart *)&(wm->wmstep);
Display		*display = XtDisplay(w);
Window		window = XtWindow(w);
GC              use_gc, tmp;
XGCValues       val;
XRectangle		rect[1];
MotifCompResourceInfo	*mcri = (MotifCompResourceInfo *)
				&(motCompRes[ICON_COMP]);
MotCompAppearanceInfo	*mcai = (MotCompAppearanceInfo *) &(mcri->compai);

   if (flag && mcai->activeBackgroundPixmap ||
				!flag && mcai->backgroundPixmap) {
	/* backgroundPixmap forces us to use rectangles.  For the background
	 * color, just use SetValues.
	 */
	if (flag) {
		use_gc = mcri->activebgGC;
		XtVaSetValues((Widget)w, XtNbackground, flag?
			mcai->activeBackground:
			/* wmstep->saveBackgroundPixel, (char *)0); */
			mcai->background, (char *)0);
	}
	else {
		use_gc = mcri->backgroundGC;
		XtVaSetValues((Widget)w, XtNbackground, flag?
			mcai->activeBackground:
			/*wmstep->saveBackgroundPixel, (char *)0);*/
			mcai->background, (char *)0);
	}

	/* The top strip */
	rect[0].x = rect[0].y = 0;
	rect[0].width = w->core.width;
	rect[0].height = w->core.height;

        XGetGCValues(display, use_gc, GCBackground, &val);
        val.foreground = val.background;
        tmp = XCreateGC(display, window, GCForeground, &val);
        XFillRectangles(display, window, tmp, rect, 1);
        XFreeGC(display, tmp);
 
	XFillRectangles(display, window, use_gc, rect, 1);

   } /* flag && pixmap */
   else {
	/*  No background pixmaps - this is easy!!!
	 */
	XtVaSetValues((Widget)w, XtNbackground, flag?
		mcai->activeBackground :
		mcai->background, (char *)0);
   } /* else */
   /* Draw the top and bottom (outer) shadows */
	/* Draw outer shadows */
	_OlgDrawBorderShadow(XtScreen(w), window, 
		wm->wmstep.mdm, OL_SHADOW_OUT, 
		ICON_BORDER_SHADOW_WIDTH, 0, 0,
		w->core.width, w->core.height,
		flag ? mcri->activetopGC : mcri->topGC,
		flag ? mcri->activebotGC : mcri->botGC);
} /* MotifFillIconBackground */

/*
 *************************************************************************
 * ReparentMotifIconWindow.
 *
 ****************************procedure*header*****************************
 */
extern void
ReparentMotifIconWindow OLARGLIST((display, wmstep))
OLARG(Display *, display)
OLGRA(WMStepPart *, wmstep)
{
int	x, y;
	/* Reparent window - to where? I'll show you.
	 * - the pixmap image is placed at ICON_BORDER_SHADOW_WIDTH (the
	 * outer shadow of the icon) + PIXELS_BETWEEN_SHADOWS (the number
	 * of pixels between this outer shadow and the inner shadow, also
	 * called the iconImageTopShadow by it's resource name) +
	 * ICON_IMAGE_SHADOW_WIDTH, the actual width in pixels of this
	 * icon image shadow, or inner shadow as I am calling it.
	 */
	x = y = ICON_BORDER_SHADOW_WIDTH +
		PIXELS_BETWEEN_SHADOWS +
		ICON_IMAGE_SHADOW_WIDTH;

	XResizeWindow(display, wmstep->xwmhints->icon_window,
		motwmrcs->iconImageMaximum.width +
				PIXMAP_LEFT_PAD + PIXMAP_RIGHT_PAD,
		motwmrcs->iconImageMaximum.height +
				PIXMAP_TOP_PAD + PIXMAP_BOTTOM_PAD);

	XReparentWindow(display, wmstep->xwmhints->icon_window,
		XtWindow(wmstep->icon_widget), x, y);

} /* ReparentMotifIconWindow */

/*
 *************************************************************************
 * SetMotifIconProperty.
 *
 ****************************procedure*header*****************************
 */
extern void
SetMotifIconProperty OLARGLIST((display, root_win))
OLARG(Display *, display)
OLGRA(Window, root_win)
{
XIconSize	xiconsize;

	xiconsize.max_width = xiconsize.min_width =
		motwmrcs->iconImageMaximum.width +
				PIXMAP_LEFT_PAD + PIXMAP_RIGHT_PAD,
	xiconsize.max_height = xiconsize.min_height =
		motwmrcs->iconImageMaximum.height +
				PIXMAP_TOP_PAD + PIXMAP_BOTTOM_PAD,
	xiconsize.width_inc = xiconsize.height_inc = 0;

	XSetIconSizes(display, root_win, &xiconsize, 1);

} /* SetMotifIconProperty */

void
MotifAIWLeave OLARGLIST((w, client_data, event, cont_to_disp))
OLARG(Widget,		w)
OLARG(XtPointer,	client_data)
OLARG(XEvent *,		event)
OLGRA(Boolean *,	cont_to_disp)
{
WMStepWidget	wm = motAIW.wm;
WMStepPart	*wmstep;

if (!wm)
	return;
wmstep = (WMStepPart *)&(wm->wmstep);
switch (event-> type)
{
   case LeaveNotify:
      if (wmrcs->pointerFocus && !NumMenushellsPosted)
         {
         switch(event-> xcrossing.detail)
            {
            case NotifyAncestor:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyAncestor\n");
#endif

	       if(wm->wmstep.is_current)	{
		  wm->wmstep.is_current = 0;
		  DisplayWM(wm,NULL);
	       }
               SetFocus(Frame, GainFocus, 0);
               break;
            case NotifyVirtual:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyVirtual\n");
#endif

	       if(wm->wmstep.is_current)	{
		  wm->wmstep.is_current = 0;
		  DisplayWM(wm,NULL);
	       }
               SetFocus(Frame, GainFocus, 0);
               break;
            case NotifyInferior:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyInferior\n");
#endif
               break;
            case NotifyNonlinear:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyNonlinear\n");
#endif
               break;
            case NotifyNonlinearVirtual:
#ifdef DEBUG
fprintf(stderr,"\tLeaveNotify had detail = NotifyNonlinearVirtual\n");
#endif
               break;
            }
         }
      break;
   default:
      break;
   }
} 
