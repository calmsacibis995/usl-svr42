/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)widgetTree:WidgetTree.h	1.1"
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <Xol/OpenLook.h>

struct widget_tree  {
        String class;
        String ref;
        OlDefine pos;
        XtCallbackProc callback;
	int motif;
};

typedef struct _item  {
	XtPointer	label;
	XtPointer	select;
}  item;

extern char * item_fields[];
extern int num_item_fields;

void AbbreviatedButtonCB OL_ARGS((Widget, XtPointer, XtPointer));
void AbbreviatedMenuCB OL_ARGS((Widget, XtPointer, XtPointer));
void BulletinBoardCB OL_ARGS((Widget, XtPointer, XtPointer));
void CaptionCB OL_ARGS((Widget, XtPointer, XtPointer));
void CategoryCB OL_ARGS((Widget, XtPointer, XtPointer));
void CheckboxCB OL_ARGS((Widget, XtPointer, XtPointer));
void ControlAreaCB OL_ARGS((Widget, XtPointer, XtPointer));
void DestroyCB OL_ARGS((Widget, XtPointer, XtPointer));
void ExclusivesCB OL_ARGS((Widget, XtPointer, XtPointer));
void FButtonsCB OL_ARGS((Widget, XtPointer, XtPointer));
void FCheckBoxCB OL_ARGS((Widget, XtPointer, XtPointer));
void FExclusivesCB OL_ARGS((Widget, XtPointer, XtPointer));
void FListCB OL_ARGS((Widget, XtPointer, XtPointer));
void FNonexclusivesCB OL_ARGS((Widget, XtPointer, XtPointer));
void FooterCB OL_ARGS((Widget, XtPointer, XtPointer));
void FooterPanelCB OL_ARGS((Widget, XtPointer, XtPointer));
void FormCB OL_ARGS((Widget, XtPointer, XtPointer));
void GaugeCB OL_ARGS((Widget, XtPointer, XtPointer));
void HelpCB OL_ARGS((Widget, XtPointer, XtPointer));
void IntegerFieldCB OL_ARGS((Widget, XtPointer, XtPointer));
void MenuButtonGadgetCB OL_ARGS((Widget, XtPointer, XtPointer));
void MenuButtonWidgetCB OL_ARGS((Widget, XtPointer, XtPointer));
void MenuCB OL_ARGS((Widget, XtPointer, XtPointer));
void ModalCB OL_ARGS((Widget, XtPointer, XtPointer));
void NonexclusivesCB OL_ARGS((Widget, XtPointer, XtPointer));
void NoticeCB OL_ARGS((Widget, XtPointer, XtPointer));
void OblongButtonGadgetCB OL_ARGS((Widget, XtPointer, XtPointer));
void OblongButtonWidgetCB OL_ARGS((Widget, XtPointer, XtPointer));
void PanesCB OL_ARGS((Widget, XtPointer, XtPointer));
void PopdownCB OL_ARGS((Widget, XtPointer, XtPointer));
void PopupMenuCB OL_ARGS((Widget, XtPointer, XtPointer));
void PopupWindowCB OL_ARGS((Widget, XtPointer, XtPointer));
void RectButtonWidgetCB OL_ARGS((Widget, XtPointer, XtPointer));
void RubberTileCB OL_ARGS((Widget, XtPointer, XtPointer));
void ScrollbarCB OL_ARGS((Widget, XtPointer, XtPointer));
void ScrolledWindowCB OL_ARGS((Widget, XtPointer, XtPointer));
void ScrollingListCB OL_ARGS((Widget, XtPointer, XtPointer));
void SliderCB OL_ARGS((Widget, XtPointer, XtPointer));
void StaticTextCB OL_ARGS((Widget, XtPointer, XtPointer));
void StepFieldCB OL_ARGS((Widget, XtPointer, XtPointer));
void StubCB OL_ARGS((Widget, XtPointer, XtPointer));
void TextCB OL_ARGS((Widget, XtPointer, XtPointer));
void TextEditCB OL_ARGS((Widget, XtPointer, XtPointer));
void TextFieldCB OL_ARGS((Widget, XtPointer, XtPointer));
