/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)olwm:WMStepP.h	1.18"
#endif

#ifndef _WMStepP_h
#define _WMStepP_h

#include <X11/CoreP.h>		/* include superclass's private header	*/
#include <WMStep.h>		/* include this class's public header	*/
#include <Olg.h>
#include <Xol/DynamicP.h>

/*
 * standard widget typedefs
 */

typedef struct {int foo;} WMStepClassPart;

typedef struct _WMStepClassRec 
   {
   CoreClassPart        core_class;
   WMStepClassPart      wmstep_class;
   } WMStepClassRec;

typedef struct 
   {
   XWindowAttributes   window_attributes;
   XWindowAttributes * windowAttributes;
   Window              window;
   Widget              child;
   int			dragX;
   int			dragY;
   XFontStruct *       font;
   GC                  gc;
   WMSize              size;
   WMMetrics *         metrics;
   WMGeometry          prev;
   WMGeometry *        icon;
   Boolean             saveSet;
   Boolean             destroy_child;
   int                 menu_default;
   Cardinal	       menu_pane_default; /* which menu pane is the default 
					   * index in menu_default for?
					   * Always 0 if this step child has
					   * 1 menu pane.
					   */
   WMMenuButtonData	*mbd_menu_default; /* Only for windows that have
					    * special menus, this points to
					    * the default.
					    */
   void		       (*default_cb)();
   int                 cascade_default;
   char *              window_name;
   char *              icon_name;
   Pixel               foreground_pixel;
   Pixel               saveBackgroundPixel;
   unsigned long       protocols;
   unsigned long       hints;
   unsigned long       decorations;
   unsigned long	menu_functions;
   unsigned long       focus;
   unsigned long	olwincolors;
   Window              transient_parent;
   XWMHints *          xwmhints;
   XClassHint          classhint;
   XSizeHints          xnormalsize;
   OLWinAttr           olwa;
   WMDecorationHints   wmdh;
   int                 window_bw;
   Boolean             is_current;
   Boolean             is_grabbed;
   SubWindowInfo       *subwindow_data;
   Window	       *colormap_windows;
   int		       num_cmap_windows;
   OlgAttrs		*mdm, *label_mdm;
   PropMwmHints		mwmh;
   Widget		icon_widget;
   WMMenuButtonData	*menu_ext; /* For MWM_MENU prop ("menu extension") */
   short		private_buttons_avail;	/* For MWM_MENU */
   short		private_buttons_used;	/* For MWM_MENU */
   short		private_keys_used;	/* For MWM_MENU */
   WMMenuDef		*private_buttons; /* For MWM_MENU */
   OlVirtualEventTable	private_db; /* For MWM_MENU */
   OlKeyOrBtnRec	*private_keys; /* For MWM_MENU */
   long			*mwm_msgs; /* for MWM_MESSAGES property */
   int			num_mwm_msgs; /* for MWM_MESSAGES */
   Pixmap		icon_pixmap;
   int			icon_map_pos_row;
   int			icon_map_pos_col;
   MotCSInfo		*csinfo; /* For Motif client specific resources */
   GC			iconimagetopGC; /* Motif - top shadows */
   GC			iconimagebotGC; /* Motif - bottom shadows */
   GC			iconimagefgGC; /* Motif - foreground GCs */
   GC			iconimagebgGC; /* Motif - background GCs */
   } WMStepPart;

typedef struct _WMStepRec 
   {
   CorePart   core;
   WMStepPart wmstep;
   } WMStepRec;

extern WMStepClassRec wmstepClassRec;

	/*
	 * Define some macros
	 */

#define HasPushpin(w)		(w-> wmstep.decorations & WMPushpin) 
#define HasFullMenu(w)		(w-> wmstep.decorations & WMHasFullMenu)
#define HasLimitedMenu(w)	(w-> wmstep.decorations & WMHasLimitedMenu)
#define HasMenu(w)		(w-> wmstep.decorations & WMHasFullMenu ||\
				w-> wmstep.decorations & WMHasLimitedMenu)
#define IsIconic(w)		(w-> wmstep.size == WMICONICNORMAL ||\
				w-> wmstep.size == WMICONICFULL)
#define Resizable(w)		(w-> wmstep.decorations & WMResizable)
#define Header(w)		(w-> wmstep.decorations & WMHeader)
#define CoreW(w)		(w-> core.width)
#define CoreH(w)		(w-> core.height)
#define CornerX(w)		(w-> wmstep.metrics-> cornerX)
#define CornerY(w)		(w-> wmstep.metrics-> cornerY)
#define Cornerx(w)		(w-> wmstep.metrics-> cornerx)
#define Cornery(w)		(w-> wmstep.metrics-> cornery)
#define LineWidX(w)		(w-> wmstep.metrics-> linewidX)
#define LineWidY(w)		(w-> wmstep.metrics-> linewidY)
#define LineWidx(w)		(w-> wmstep.metrics-> linewidx)
#define LineWidy(w)		(w-> wmstep.metrics-> linewidy)
#define GapX(w)			(w-> wmstep.metrics-> gapx)
#define GapY(w)			(w-> wmstep.metrics-> gapy)
#define Offset(w)		(w-> wmstep.metrics-> offset)
#define MarkWid(w)		(w-> wmstep.metrics-> markwid)
#define BorderX(w)		(LineWidX(w) + GapX(w))
#define BorderY(w)		(LineWidY(w) + GapY(w))
#define BannerHt(w)		(w-> wmstep.metrics-> bannerht)
#define BannerWid(w)		(CoreW(w) - BorderX(w) * 2)
#define LineHt(w)		(w-> wmstep.metrics-> linewid + \
				 w-> wmstep.metrics-> hgap1 + \
				 w-> wmstep.metrics-> hgap2)
#define OriginX(w)		(BorderX(w))
#define OriginY(w)	( (w->wmstep.decorations & WMHeader) ?  \
			  (BorderY(w) + BannerHt(w) + LineHt(w)) : \
			  LineWidY(w))

#define ChildBW(w)		0
#define ParentWidth(w,W)	(W + (ChildBW(w) + LineWidX(w) + GapX(w)) * 2)
#define ParentHeight(w,H)	(H + (ChildBW(w) + LineWidY(w) + GapY(w)) * 2 +\
				 BannerHt(w) + LineHt(w))
#define ChildWidth(w)	(CoreW(w) - (ChildBW(w) + LineWidX(w) + GapX(w)) * 2)
#define ChildHeight(w)	(CoreH(w) - (ChildBW(w) + LineWidY(w) + GapY(w)) \
				* 2 - BannerHt(w) - LineHt(w))
				/* What's with the extra "- 2" at the end??
				* 2 - BannerHt(w) - LineHt(w) - 2)
				 */

#define NewChildWidth(w, parentwidth)	(parentwidth - (ChildBW(w) + \
					 LineWidX(w) + GapX(w)) * 2)

#define NewChildHeight(w, parentheight)	(parentheight - (ChildBW(w) + \
					 LineWidY(w) + GapY(w)) * 2 - \
					 BannerHt(w) - LineHt(w))

#define Baseline(w)	(BorderY(w) + BannerHt(w) - w-> wmstep.metrics->\
				baseline)
#endif /*  _WMStepP_h */
