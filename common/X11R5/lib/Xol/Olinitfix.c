/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)shlib:Olinitfix.c	1.8"
#endif


/*
 **************************************************************
 * 
 * 	This file contains Olinitfix() function. It is used to
 * fix up all the external references that were supposed to be
 * resolve at compile time but couldn't because of the extra
 * level of indirection in static shared library.
 *
 * 	For example, in each of the widget class structure,
 * sometimes you initialize a superclass field with a widget
 * class structure outside the static shared library
 * (like &widgetClass). Sometimes you initialize a function
 * pointer to be XtInherit. All these supposed to be resolved
 * at compile time. However, when building a shared library,
 * the actual address of these symbols cannot be known at
 * compile time. Moreover, "&widgetClass" expands to be
 * "&(*_libXol_widgetClass)", because of the level of indirection.
 * The above string causes syntax error.
 * 
 **************************************************************
 */

#ifdef SHARELIB
#define	USE_EXT_VARS
#include "libXoli.h"
#define	__OlStrings_h_
#include "IntrinsicP.h"
#include "Xol/OpenLookP.h"
#include "Xol/PrimitiveP.h"
#include "Xol/ManagerP.h"
#include "Xol/FlatP.h"
#include "Xol/FNonexcluP.h"
#include "Xol/FExclusivP.h"
#include "Xol/FCheckBoxP.h"
#include "Xol/EventObjP.h"
#include "Xol/AbbrevMenP.h"
#include "Xol/AbbrevStaP.h"
#include "Xol/ArrowP.h"
#include "Xol/BaseWindoP.h"
#include "Xol/BulletinBP.h"
/*#include "Xol/ButtonP.h"*/
#include "Xol/MenuButtoP.h"
#include "Xol/ButtonStaP.h"
#include "Xol/CaptionP.h"
#include "Xol/CheckBoxP.h"
/*#include "Xol/ControlArP.h"*/
#include "Xol/ExclusiveP.h"
#include "Xol/FooterPanP.h"
#include "Xol/FormP.h"
#include "Xol/HelpP.h"
#include "Xol/ListPaneP.h"
#include "Xol/MagP.h"
#include "Xol/MenuP.h"
#include "Xol/NonexclusP.h"
#include "Xol/NoticeP.h"
#include "Xol/OblongButP.h"
#include "Xol/PopupWindP.h"
#include "Xol/PushpinP.h"
#include "Xol/RectButtoP.h"
#include "Xol/ScrollbarP.h"
#include "Xol/ScrolledWP.h"
/*#include "Xol/ScrollingP.h"*/
#include "Xol/SliderP.h"
#include "Xol/StaticTexP.h"
#include "Xol/StubP.h"
#include "Xol/TextEditP.h"
#include "Xol/TextFieldP.h"
#include "Xol/TextPaneP.h"
#include "Xol/TextP.h"

extern AbbrevStackClassRec		abbrevStackClassRec;
extern AbbrevMenuButtonClassRec		abbrevMenuButtonClassRec;
extern ArrowClassRec			arrowClassRec;
extern BaseWindowShellClassRec		baseWindowShellClassRec;
extern BulletinBoardClassRec		bulletinBoardClassRec;
extern ButtonClassRec			buttonClassRec;
extern MenuButtonClassRec		menuButtonClassRec;
extern ButtonStackClassRec		buttonStackClassRec;
extern CaptionClassRec			captionClassRec;
extern CheckBoxClassRec			checkBoxClassRec;
extern ControlClassRec			controlClassRec;
extern ExclusivesClassRec		exclusivesClassRec;
extern FooterPanelClassRec		footerPanelClassRec;
extern FormClassRec			formClassRec;
extern HelpClassRec			helpClassRec;
extern ListClassRec			listClassRec;
extern ListPaneClassRec			listPaneClassRec;
extern MagClassRec			magClassRec;
extern MenuShellClassRec		menuShellClassRec;
extern NonexclusivesClassRec		nonexclusivesClassRec;
extern NoticeShellClassRec		noticeShellClassRec;
extern OblongButtonClassRec		oblongButtonClassRec;
extern PopupWindowShellClassRec		popupWindowShellClassRec;
extern PushpinClassRec			pushpinClassRec;
extern RectButtonClassRec		rectButtonClassRec;
extern ScrollbarClassRec		scrollbarClassRec;
extern ScrolledWindowClassRec		scrolledWindowClassRec;
extern SliderClassRec			sliderClassRec;
extern StaticTextClassRec		statictextClassRec;
extern StubClassRec			stubClassRec;
extern TextClassRec			textClassRec;
extern TextFieldClassRec		textFieldClassRec;
extern TextPaneClassRec			textPaneClassRec;
extern EventObjClassRec			eventObjClassRec;
extern FlatCheckBoxClassRec		flatCheckBoxClassRec;
extern FlatExclusivesClassRec		flatExclusivesClassRec;
extern FlatNonexclusivesClassRec	flatNonexclusivesClassRec;
extern FlatClassRec			flatClassRec;
extern ManagerClassRec			managerClassRec;
extern PrimitiveClassRec		primitiveClassRec;
extern TextEditClassRec			textEditClassRec;

extern ButtonGadgetClassRec		buttonGadgetClassRec;
extern MenuButtonGadgetClassRec		menuButtonGadgetClassRec;
extern ButtonStackGadgetClassRec	buttonStackGadgetClassRec;
extern OblongButtonGadgetClassRec	oblongButtonGadgetClassRec;

static void _OlFixup();

void
_OlSharedLibFixup()
{
	/* gadgets stuff */
	_OlFixup(&eventObjClassRec);
	_OlFixup(&buttonGadgetClassRec);
	_OlFixup(&oblongButtonGadgetClassRec);
	_OlFixup(&menuButtonGadgetClassRec);
	_OlFixup(&buttonStackGadgetClassRec);

	_OlFixup(&managerClassRec);
	_OlFixup(&primitiveClassRec);

	_OlFixup(&abbrevMenuButtonClassRec);
	_OlFixup(&abbrevStackClassRec);
	_OlFixup(&arrowClassRec);
	_OlFixup(&baseWindowShellClassRec);
	_OlFixup(&bulletinBoardClassRec);
	_OlFixup(&buttonClassRec);
	_OlFixup(&menuButtonClassRec);
	_OlFixup(&buttonStackClassRec);
	_OlFixup(&captionClassRec);
	_OlFixup(&checkBoxClassRec);
	_OlFixup(&controlClassRec);
	_OlFixup(&exclusivesClassRec);
	_OlFixup(&footerPanelClassRec);
	_OlFixup(&formClassRec);
	_OlFixup(&helpClassRec);
	_OlFixup(&listClassRec);
	_OlFixup(&listPaneClassRec);
	_OlFixup(&magClassRec);
	_OlFixup(&menuShellClassRec);
	_OlFixup(&nonexclusivesClassRec);
	_OlFixup(&noticeShellClassRec);
	_OlFixup(&oblongButtonClassRec);
	_OlFixup(&popupWindowShellClassRec);
	_OlFixup(&pushpinClassRec);
	_OlFixup(&rectButtonClassRec);
	_OlFixup(&scrollbarClassRec);
	_OlFixup(&scrolledWindowClassRec);
	_OlFixup(&sliderClassRec);
	_OlFixup(&statictextClassRec);
	_OlFixup(&stubClassRec);
	_OlFixup(&textClassRec);
	_OlFixup(&textEditClassRec);
	_OlFixup(&textFieldClassRec);
	_OlFixup(&textPaneClassRec);

	_OlFixup(&flatClassRec);
	_OlFixup(&flatCheckBoxClassRec);
	_OlFixup(&flatExclusivesClassRec);
	_OlFixup(&flatNonexclusivesClassRec);

}

static void
_OlFixup(wc)
WidgetClass wc;
{
	WidgetClass w;
	int iscomposite	= 0;
	int isprimitive	= 0;
	int ismanager	= 0;
	int isflat	= 0;

	if (wc->core_class.superclass == (WidgetClass)&_libXol_widgetClassRec) {
		wc->core_class.superclass = (WidgetClass)_libXol_widgetClassRec;
	}
	else if	(wc->core_class.superclass == (WidgetClass)&_libXol_shellClassRec) {
		wc->core_class.superclass = (WidgetClass)_libXol_shellClassRec;
	}
	else if	(wc->core_class.superclass == (WidgetClass)&_libXol_topLevelShellClassRec) {
		wc->core_class.superclass = (WidgetClass)_libXol_topLevelShellClassRec;
	}
	else if	(wc->core_class.superclass == (WidgetClass)&_libXol_compositeClassRec) {
		wc->core_class.superclass = (WidgetClass)_libXol_compositeClassRec;
	}
	else if	(wc->core_class.superclass == (WidgetClass)&_libXol_wmShellClassRec) {
		wc->core_class.superclass = (WidgetClass)_libXol_wmShellClassRec;
	}
	else if	(wc->core_class.superclass == (WidgetClass)&_libXol_constraintClassRec)	{
		wc->core_class.superclass = (WidgetClass)_libXol_constraintClassRec;
	}
	else if	(wc->core_class.superclass == (WidgetClass)&_libXol_rectObjClassRec) {
		wc->core_class.superclass = (WidgetClass)_libXol_rectObjClassRec;
	}
	else if	(wc->core_class.superclass == (WidgetClass)&_libXol_applicationShellClassRec) {
		wc->core_class.superclass = (WidgetClass)_libXol_applicationShellClassRec;
	}

	/* fix up function pointers in Core classpart */
	if (wc->core_class.realize == (XtWidgetProc)&_libXol__XtInherit)	{
		wc->core_class.realize = XtInheritRealize;
	}

	if (wc->core_class.resize == (XtWidgetProc)&_libXol__XtInherit)	{
		wc->core_class.resize =	XtInheritResize;
	}

	if (wc->core_class.expose == (XtWidgetProc)&_libXol__XtInherit)	{
		wc->core_class.expose =	XtInheritExpose;
	}

	if (wc->core_class.set_values_almost ==	(XtWidgetProc)&_libXol__XtInherit) {
		wc->core_class.set_values_almost = XtInheritSetValuesAlmost;
	}

	if (wc->core_class.accept_focus	== (XtWidgetProc)&_libXol__XtInherit) {
		wc->core_class.accept_focus = XtInheritAcceptFocus;
	}

	if (wc->core_class.query_geometry == (XtGeometryHandler)&_libXol__XtInherit) {
		wc->core_class.query_geometry =	(XtGeometryHandler)XtInheritQueryGeometry;
	}

	if (wc->core_class.tm_table == (String)&_libXol__XtInheritTranslations)	{
		wc->core_class.tm_table	= XtInheritTranslations;
	}

	/* check if composite */
	for (w=wc; w !=	NULL; w	= w->core_class.superclass) {
		if (w == compositeWidgetClass)
			iscomposite = 1;
		else if	(w == primitiveWidgetClass)
			isprimitive = 1;
		else if	(w == managerWidgetClass)
			ismanager = 1;
		else if	(w == flatWidgetClass)
			isflat = 1;
	}

	if (iscomposite) {
		CompositeWidgetClass cc;

		/* fix up pointers in Composite	classpart */
		cc = (CompositeWidgetClass)wc;
		if(cc->composite_class.geometry_manager	==
			 (XtGeometryHandler)&_libXol__XtInherit)	{
			cc->composite_class.geometry_manager = (XtGeometryHandler)XtInheritGeometryManager;
		}
		if(cc->composite_class.change_managed ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			cc->composite_class.change_managed = (XtWidgetProc)XtInheritChangeManaged;
		}
		if(cc->composite_class.insert_child ==
			 (XtArgsProc)&_libXol__XtInherit) {
			cc->composite_class.insert_child = (XtArgsProc)XtInheritInsertChild;
		}
		if(cc->composite_class.delete_child ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			cc->composite_class.delete_child = (XtWidgetProc)XtInheritDeleteChild;
		}
/*
		if(cc->composite_class.move_focus_to_next ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			cc->composite_class.move_focus_to_next = (XtWidgetProc)XtInheritMoveFocusToNext;
		}
		if(cc->composite_class.move_focus_to_prev ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			cc->composite_class.move_focus_to_prev = (XtWidgetProc)XtInheritMoveFocusToPrev;
		}
*/
	}

	if (ismanager) {
		ManagerWidgetClass mc;

		mc = (ManagerWidgetClass)wc;
		if (mc->manager_class.highlight_handler	==
			 (XtWidgetProc)&_libXol__XtInherit) {
			mc->manager_class.highlight_handler = (XtWidgetProc)XtInheritHighlightHandler;
		}
		if (mc->manager_class.traversal_handler	==
			 (XtWidgetProc)&_libXol__XtInherit) {
			mc->manager_class.traversal_handler = (XtWidgetProc)XtInheritTraversalHandler;
		}
	}

	if (isprimitive) {
		PrimitiveWidgetClass pc;

		pc = (PrimitiveWidgetClass)wc;
		if (pc->primitive_class.highlight_handler ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			pc->primitive_class.highlight_handler =	(XtWidgetProc)XtInheritHighlightHandler;
		}
	}

	if (isflat) {
		FlatWidgetClass	fc;

		fc = (FlatWidgetClass)wc;
/*
		if (fc->flat_class.build_extractor ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			fc->flat_class.build_extractor = (XtWidgetProc)XtInheritFlatBuildExtractor;
		}
*/
/*
		if (fc->flat_class.check_layout_params ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			fc->flat_class.check_layout_params = (XtWidgetProc)XtInheritFlatCheckLayoutParams;
		}
*/
		if (fc->flat_class.draw_item ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			fc->flat_class.draw_item = (XtWidgetProc)XtInheritFlatDrawItem;
		}
/*
		if (fc->flat_class.refresh_item ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			fc->flat_class.refresh_item = (XtWidgetProc)XtInheritFlatRefreshItem;
		}
*/
		if (fc->flat_class.get_draw_info ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			fc->flat_class.get_draw_info = (XtWidgetProc)XtInheritFlatGetDrawInfo;
		}
		if (fc->flat_class.get_index ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			fc->flat_class.get_index = (XtWidgetProc)XtInheritFlatGetIndex;
		}
/*
		if (fc->flat_class.get_overlap ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			fc->flat_class.get_overlap = (XtWidgetProc)XtInheritFlatGetOverlap;
		}
*/
		if (fc->flat_class.item_dimensions ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			fc->flat_class.item_dimensions = (XtWidgetProc)XtInheritFlatItemDimensions;
		}
		if (fc->flat_class.layout ==
			 (XtWidgetProc)&_libXol__XtInherit) {
			fc->flat_class.layout =	(XtWidgetProc)XtInheritFlatLayout;
		}
	}

}

#endif
