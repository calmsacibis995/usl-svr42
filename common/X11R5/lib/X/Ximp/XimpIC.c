/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpIC.c	1.2"
/* $XConsortium: XimpIC.c,v 1.4 91/10/07 17:48:30 rws Exp $ */
/******************************************************************

              Copyright 1991, by FUJITSU LIMITED

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
that both that copyright notice and this permission notice appear

IN NO EVENT SHALL FUJITSU LIMITED BE LIABLE FOR ANY SPECIAL, INDIRECT
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE

  Author: Takashi Fujiwara     FUJITSU LIMITED 
                               fujiwara@a80.tech.yk.fujitsu.co.jp

******************************************************************/

#define NEED_EVENTS
#include <X11/Xatom.h>
#include "Xlibint.h"
#include "Xlcint.h"

#include "Ximplc.h"

static void		_Ximp_DestroyIC();
static void		_Ximp_SetFocus();
static void		_Ximp_UnSetFocus();
extern char 		*_Ximp_SetICValues();
extern char 		*_Ximp_GetICValues();
extern char		*_Ximp_MbReset();
extern wchar_t		*_Ximp_WcReset();
extern int		_Ximp_MbLookupString();
extern int		_Ximp_WcLookupString();

extern char 		*_Ximp_SetICValueData();
extern void		_Ximp_SetValue_Resource();
extern Bool		_Ximp_SetOpenXIMP();

extern void		_Ximp_SetFocusWindow();
extern void		_Ximp_SetPreeditAtr();
extern void		_Ximp_SetPreeditFont();
extern void		_Ximp_SetStatusAtr();
extern void		_Ximp_SetStatusFont();
extern Bool		_Ximp_XimFilter_Client();

static void 		_Ximp_AttributesSetL();
extern void 		_Ximp_IM_SendMessage();

static XICMethodsRec Ximp_ic_methods = {
				_Ximp_DestroyIC, 	/* destroy */
				_Ximp_SetFocus,  	/* set_focus */
				_Ximp_UnSetFocus,	/* unset_focus */
				_Ximp_SetICValues,	/* set_values */
				_Ximp_GetICValues,	/* get_values */
				_Ximp_MbReset,		/* mb_reset */
				_Ximp_WcReset,		/* wc_reset */
				_Ximp_MbLookupString,	/* mb_lookup_string */
				_Ximp_WcLookupString,	/* wc_lookup_string */
				};

Bool
_Ximp_XimFilter_Keypress (d, w, ev, client_data)
	Display		*d;
	Window		w;
	XEvent		*ev;
	XPointer	*client_data;
{
	extern Bool	_Ximp_Keypress ();

	return(_Ximp_Keypress (d, w, ev, client_data));
}

static Ximp_XIC		current_xic = 0;

XIC
_Ximp_CreateIC(im, values)
	XIM		 im;
	XIMArg		*values;
{
	Ximp_XIC	 ic;
	long		 dummy;
	XICXimpRec	*ximp_icpart;

	if((ic = (Ximp_XIC)Xmalloc(sizeof(Ximp_XICRec))) == (Ximp_XIC)NULL) {
		return((XIC)NULL);
		} 
	if((ximp_icpart = (XICXimpRec *)Xmalloc(sizeof(XICXimpRec))) == (XICXimpRec *)NULL) {
		Xfree(ic);
		return((XIC)NULL);
		} 
	bzero((char *)ic, sizeof(Ximp_XICRec));
	bzero((char *)ximp_icpart, sizeof(XICXimpRec));

	ic->methods = &Ximp_ic_methods;
	ic->core.im = im;
	/* Filter Event : for Ximp Protocol */
	ic->core.filter_events = KeyPressMask | KeyReleaseMask | StructureNotifyMask;

	ic->ximp_icpart = ximp_icpart;
	_Ximp_SetICValueData(ic, values, XIMP_CREATE_IC, &dummy);

	/* The Value must be set */
	if(!(ximp_icpart->value_mask & XIMP_INPUT_STYLE)) /* Input Style */
		goto Set_Error;
	if(ic->core.input_style & XIMPreeditPosition)
		if(!(ximp_icpart->proto_mask & XIMP_PRE_SPOTL_MASK)) /* SpotLocation */
			goto Set_Error;
	if(   (ic->core.input_style & XIMPreeditPosition)
	   || (ic->core.input_style & XIMPreeditArea)    )
		if(!(ximp_icpart->proto_mask & XIMP_PRE_FONT_MASK)) /* FontSet */
			goto Set_Error;
	if(ic->core.input_style & XIMStatusArea)
		if(!(ximp_icpart->proto_mask & XIMP_STS_FONT_MASK)) /* FontSet */
			goto Set_Error;
	if(!(ximp_icpart->value_mask & XIMP_CLIENT_WIN)) /* Client Window */
		goto Set_Error;
	
	_Ximp_SetValue_Resource(ic, &dummy);

	if(((Ximp_XIM)im)->ximp_impart->inputserver) {
		if(_Ximp_SetOpenXIMP(ic) == False)
			goto Set_Error;
	}
	if (!current_xic) {
		_XRegisterFilterByType (ic->core.im->core.display,
				ic->core.focus_window,
				KeyPress, KeyPress,
				_Ximp_XimFilter_Keypress,
				ic);
		current_xic = ic;
	}
	return((XIC)ic);

   Set_Error :
	Xfree(ic);
	Xfree(ximp_icpart);
	return((XIC)NULL);
}

static void
_Ximp_DestroyIC(ic)
	Ximp_XIC	 ic;
{
	if(ic->ximp_icpart->icid) {
 		_Ximp_IM_SendMessage(ic, XIMP_DESTROY, NULL, NULL, NULL);
		if (current_xic) {
			_XUnregisterFilter (ic->core.im->core.display,
					ic->core.focus_window,
					_Ximp_XimFilter_Keypress,
					current_xic);
			current_xic = 0;
		}
	}
	Xfree(ic->ximp_icpart);
	return;
}

static void
_Ximp_SetFocus(ic)
	Ximp_XIC	ic;
{
	if (ic->ximp_icpart->icid) {
 		_Ximp_IM_SendMessage(ic, XIMP_SETFOCUS, NULL, NULL, NULL);
		if (current_xic)
			_XUnregisterFilter (ic->core.im->core.display,
						ic->core.focus_window,
						_Ximp_XimFilter_Keypress,
						current_xic);
		_XRegisterFilterByType (ic->core.im->core.display,
					ic->core.focus_window,
					KeyPress, KeyPress,
					_Ximp_XimFilter_Keypress,
					ic);
		current_xic = ic;
	}
	return;
}

static void
_Ximp_UnSetFocus(ic)
	Ximp_XIC	ic;
{
	if(ic->ximp_icpart->icid) {
		_Ximp_IM_SendMessage(ic, XIMP_UNSETFOCUS, NULL, NULL, NULL);
		if (current_xic) {
			_XUnregisterFilter (ic->core.im->core.display,
					ic->core.focus_window,
					_Ximp_XimFilter_Keypress,
					current_xic);
			current_xic = 0;
		}
	}
	return;
}

void
_Ximp_SetFocusWindow(ic)
	Ximp_XIC	 ic;
{
	Atom		 actual_type;
	int		 actual_format;
	unsigned long	 nitems, bytes_after;
	int		*prop_int;

	while(1) {
		XGetWindowProperty(ic->core.im->core.display,
			ic->core.client_window,
			((Ximp_XIM)ic->core.im)->ximp_impart->focus_win_id,
			0L, 1000000L, False, XA_WINDOW,
			&actual_type, &actual_format, &nitems, &bytes_after,
			(unsigned char **)&prop_int);
		if(nitems == 0)
			break;
		XFree((XPointer)prop_int);
		}
	XChangeProperty(ic->core.im->core.display, ic->core.client_window,
			((Ximp_XIM)ic->core.im)->ximp_impart->focus_win_id,
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *)&ic->core.focus_window, 1);
	_XUnregisterFilter(ic->core.im->core.display,
			   ic->core.focus_window,
			   _Ximp_XimFilter_Client, (XPointer)NULL);
	_XRegisterFilterByType(ic->core.im->core.display,
			       ic->core.focus_window,
			       ClientMessage, ClientMessage,
			       _Ximp_XimFilter_Client, NULL);
	return;
}

void
_Ximp_SetPreeditAtr(ic)
	Ximp_XIC		 ic;
{
	Ximp_PreeditPropRec	*preedit_atr;
	Atom			 actual_type;
	int			 actual_format;
	unsigned long		 nitems, bytes_after;
	int			*prop_int;
	unsigned char		 prop_data[XIMP_PREEDIT_MAX_CHAR];

	preedit_atr = &(ic->ximp_icpart->preedit_attr);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Area.x,             0);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Area.y,             4);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Area.width,         8);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Area.height,       12);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Foreground,        16);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Background,        20);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Colormap,          24);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Bg_Pixmap,         28);
	_Ximp_AttributesSetL(prop_data, preedit_atr->LineSpacing,       32);
	_Ximp_AttributesSetL(prop_data, preedit_atr->Cursor,            36);
	_Ximp_AttributesSetL(prop_data, preedit_atr->AreaNeeded.width,  40);
	_Ximp_AttributesSetL(prop_data, preedit_atr->AreaNeeded.height, 44);
	_Ximp_AttributesSetL(prop_data, preedit_atr->SpotLocation.x,    48);
	_Ximp_AttributesSetL(prop_data, preedit_atr->SpotLocation.y,    52);

	while(1) {
		XGetWindowProperty(ic->core.im->core.display,
			ic->core.client_window,
			((Ximp_XIM)ic->core.im)->ximp_impart->preedit_atr_id,
			0L, 1000000L, False,
			((Ximp_XIM)ic->core.im)->ximp_impart->preedit_atr_id,
			&actual_type, &actual_format, &nitems, &bytes_after,
			(unsigned char **)(&prop_int));
		if(nitems == 0)
			break;
		XFree((XPointer)prop_int);
		}
	XChangeProperty(ic->core.im->core.display, ic->core.client_window,
			((Ximp_XIM)ic->core.im)->ximp_impart->preedit_atr_id,
			((Ximp_XIM)ic->core.im)->ximp_impart->preedit_atr_id,
			32, PropModeReplace, prop_data, XIMP_PREEDIT_MAX_LONG);
	return;
}

void
_Ximp_SetPreeditFont(ic)
	Ximp_XIC		 ic;
{
	Ximp_PreeditPropRec	*preedit_atr;
	Atom			actual_type;
	int			actual_format;
	unsigned long		nitems, bytes_after;
	char			*prop;

	if (ic->core.preedit_attr.fontset != NULL) {
	    while(1) {
		XGetWindowProperty(ic->core.im->core.display,
			ic->core.client_window,
			((Ximp_XIM)ic->core.im)->ximp_impart->preeditfont_id,
			0L, 1000000L, False, XA_STRING,
			&actual_type, &actual_format, &nitems, &bytes_after,
			(unsigned char **)&prop);
		if(nitems == 0)
			break;
		XFree((XPointer)prop);
	        }
	    XChangeProperty(ic->core.im->core.display, ic->core.client_window,
			((Ximp_XIM)ic->core.im)->ximp_impart->preeditfont_id,
			XA_STRING, 8, PropModeReplace,
			(unsigned char *)(ic->ximp_icpart->preedit_font),
			strlen(ic->ximp_icpart->preedit_font));
	    XFree(ic->ximp_icpart->preedit_font);
	    }
	return;
}

void
_Ximp_SetStatusAtr(ic)
	Ximp_XIC		 ic;
{
	Ximp_StatusPropRec	*status_atr;
	Atom			 actual_type;
	int			 actual_format;
	unsigned long		 nitems, bytes_after;
	int			*prop_int;
	unsigned char		 prop_data[XIMP_STATUS_MAX_CHAR];

	status_atr = &(ic->ximp_icpart->status_attr);
	_Ximp_AttributesSetL(prop_data, status_atr->Area.x,             0);
	_Ximp_AttributesSetL(prop_data, status_atr->Area.y,             4);
	_Ximp_AttributesSetL(prop_data, status_atr->Area.width,         8);
	_Ximp_AttributesSetL(prop_data, status_atr->Area.height,       12);
	_Ximp_AttributesSetL(prop_data, status_atr->Foreground,        16);
	_Ximp_AttributesSetL(prop_data, status_atr->Background,        20);
	_Ximp_AttributesSetL(prop_data, status_atr->Colormap,          24);
	_Ximp_AttributesSetL(prop_data, status_atr->Bg_Pixmap,         28);
	_Ximp_AttributesSetL(prop_data, status_atr->LineSpacing,       32);
	_Ximp_AttributesSetL(prop_data, status_atr->Cursor,            36);
	_Ximp_AttributesSetL(prop_data, status_atr->AreaNeeded.width,  40);
	_Ximp_AttributesSetL(prop_data, status_atr->AreaNeeded.height, 44);
	_Ximp_AttributesSetL(prop_data, status_atr->window,            48);

	while(1) {
		XGetWindowProperty(ic->core.im->core.display,
			ic->core.client_window,
			((Ximp_XIM)ic->core.im)->ximp_impart->status_atr_id,
			0L, 1000000L, False,
			((Ximp_XIM)ic->core.im)->ximp_impart->status_atr_id,
			&actual_type, &actual_format, &nitems, &bytes_after,
			(unsigned char **)&prop_int);
		if(nitems == 0)
			break;
		XFree((XPointer)prop_int);
		}
	XChangeProperty(ic->core.im->core.display, ic->core.client_window,
			((Ximp_XIM)ic->core.im)->ximp_impart->status_atr_id,
			((Ximp_XIM)ic->core.im)->ximp_impart->status_atr_id,
			32, PropModeReplace, prop_data, XIMP_STATUS_MAX_LONG);
	return;
}

void
_Ximp_SetStatusFont(ic)
	Ximp_XIC		ic;
{
	Atom			actual_type;
	int			actual_format;
	unsigned long		nitems, bytes_after;
	char			*prop;

	if (ic->core.status_attr.fontset != NULL) {
	    while(1) {
		XGetWindowProperty(ic->core.im->core.display,
			ic->core.client_window,
			((Ximp_XIM)ic->core.im)->ximp_impart->statusfont_id,
			0L, 1000000L, False, XA_STRING,
			&actual_type, &actual_format, &nitems, &bytes_after,
			(unsigned char **)&prop);
		if(nitems == 0)
			break;
		XFree((XPointer)prop);
		}
	    XChangeProperty(ic->core.im->core.display, ic->core.client_window,
			((Ximp_XIM)ic->core.im)->ximp_impart->statusfont_id,
			XA_STRING, 8, PropModeReplace,
			(unsigned char *)(ic->ximp_icpart->status_font),
			strlen(ic->ximp_icpart->status_font));
	    XFree(ic->ximp_icpart->status_font);
	    }
	return;
}

static void
_Ximp_AttributesSetL(data, setdata, cnt)
	char	*data;
	long	 setdata;
	int	 cnt;
{
	long	*ptr;

	ptr = (long *)&data[cnt];
	*ptr = setdata;
	return;
}

void
_Ximp_IM_SendMessage(ic, request, data1, data2, data3)
	Ximp_XIC	ic;
	unsigned long	request;
	unsigned long	data1, data2, data3;
{
	XEvent		Message;
	
	/* ClientMessage Send */
	Message.xclient.type         = ClientMessage;
	Message.xclient.display      = ic->core.im->core.display;
	Message.xclient.window       = ((Ximp_XIM)ic->core.im)->ximp_impart->fe_window;
	Message.xclient.message_type = ((Ximp_XIM)ic->core.im)->ximp_impart->improtocol_id;
	Message.xclient.format       = 32;
	Message.xclient.data.l[0]    = request;
	if(request == XIMP_CREATE)
		Message.xclient.data.l[1] = (long)ic->core.client_window;
	else
		Message.xclient.data.l[1] = ic->ximp_icpart->icid;
	Message.xclient.data.l[2]    = data1;
	Message.xclient.data.l[3]    = data2;
	Message.xclient.data.l[4]    = data3;
	XSendEvent(ic->core.im->core.display,
			   ((Ximp_XIM)ic->core.im)->ximp_impart->fe_window,
			   False, NoEventMask, &Message);
	XFlush(ic->core.im->core.display);
	return;
}
