/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpLkup.c	1.3"
/* $XConsortium: XimpLkup.c,v 1.5 91/10/07 17:49:03 rws Exp $ */
/******************************************************************

              Copyright 1991, by Sony Corporation
              Copyright 1991, by FUJITSU LIMITED
              Copyright 1991, by Fuji Xerox Co.,Ltd.

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
that both that copyright notice and this permission notice appear
FUJITSU LIMITED and Fuji Xerox Co.,Ltd. not be used in advertising
Sony Corporation, FUJITSU LIMITED and Fuji Xerox Co.,Ltd. make no

SONY CORPORATION, FUJITSU LIMITED AND FUJI XEROX CO.,LTD. DISCLAIM
SONY CORPORATION, FUJITSU LIMITED, FUJI XEROX CO.,LTD. BE LIABLE FOR
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES

  Author: Masaki Takeuchi,     Sony Corporation
          Takashi Fujiwara,    FUJITSU LIMITED 
          Kazunori Nishihara,  Fuji Xerox Co.,Ltd.

******************************************************************/

#define NEED_EVENTS
#include <X11/keysym.h>
#include "Xlibint.h"
#include "Xutil.h"
#include "Xlcint.h"
#include "Xlibnet.h"
#include <X11/Xatom.h>

#include "Ximplc.h"

#ifdef XIMP_SIGNAL
#include <sys/signal.h>
#endif /* XIMP_SIGNAL */

#ifdef USL
/*
 * macros like htonl are defined in different files for SVR4 and SVR3.2
 * SVR4:    sys/byteorder.h
 * SVR3.2 : defined in libtcp.a or libnet.a; but since we cannot depend
 *	    on these libraries, we define our own.
 */
/*
 *	unsigned long ntohl( nl )
 *	unsigned long nl;
 *	reverses the byte order of 'ulong nl'
 */

asm unsigned long ntohl( nl )
{
%mem	nl;
	movl	nl, %eax
	xchgb	%ah, %al
	rorl	$16, %eax
	xchgb	%ah, %al
	clc
}
#endif

static int	 _time_flag = 0;
#ifdef XIMP_SIGNAL
static int
_time_out()
	{
	_time_flag = 1;
	}
#endif /* XIMP_SIGNAL */

extern Ximp_XIC		_Ximp_LookupXIC();
extern Atom		_Ximp_Protocol_id();
extern Bool 		_Ximp_XimFilter_Client();
extern Bool 		_Ximp_XimFilter_Destroy();
static Bool 		_Ximp_StartXIMP();
extern Bool 		_Ximp_SetOpenXIMP();
extern void 		_Ximp_IM_SendMessage();
extern int		_Ximp_SetupFree();

extern void		_Ximp_A_CreateExtension();
extern void		_Ximp_SetupFreeExtension();
extern void		_Ximp_ProcExtension();

static void		_Ximp_CallCallback();
static void		_Ximp_ProcError();

typedef struct {
	Atom type;
	ICID icid;
	Window owner;
} XimpResetPredArgRec, *XimpResetPredArg;

static Bool
_Ximp_ResetPredicate(d, ev, arg)
Display *d;
XEvent *ev;
XimpResetPredArg arg;
{
	if (ev->type == ClientMessage) {
		if (ev->xclient.message_type == arg->type) {
			if ((ev->xclient.format == 32) &&
				(ev->xclient.data.l[1] == arg->icid)) {
				switch (ev->xclient.data.l[0]) {
				case XIMP_RESET_RETURN:
				case XIMP_ERROR:
				case XIMP_PREEDITDONE:
				case XIMP_PREEDITDRAW:
				case XIMP_PREEDITDRAW_CM:
				case XIMP_STATUSDONE:
				case XIMP_STATUSDRAW:
				case XIMP_STATUSDRAW_CM:
					return(True);
				}
			}
		}
	} else if (ev->type == DestroyNotify) {
		if (ev->xdestroywindow.window == arg->owner) {
			return(True);
		}
	}
	return(False);
}

static unsigned char *
_Ximp_Reset(ic)
	Ximp_XIC	 ic;
{
	XEvent		 Message;
	XEvent		 event;
	XimpResetPredArgRec Arg;

	if(ic->ximp_icpart->icid) {
		/* ClientMessage Send */
		_Ximp_IM_SendMessage(ic, XIMP_RESET, NULL, NULL, NULL);

#ifdef XIMP_SIGNAL
		signal(SIGALRM, _time_out);
		alarm(XIMP_TIME_OUT);
#endif /* XIMP_SIGNAL */
		Arg.type = ((Ximp_XIM)ic->core.im)->ximp_impart->improtocol_id;
		Arg.icid = ic->ximp_icpart->icid;
		Arg.owner = ((Ximp_XIM)ic->core.im)->ximp_impart->fe_window;
		while(_time_flag != 1) {
			if( (XCheckIfEvent(ic->core.im->core.display, &event, _Ximp_ResetPredicate, &Arg)) == False) {
#ifdef XIMP_SIGNAL
				sleep(1);
#endif /* XIMP_SIGNAL */
				continue;
				}
			if (event.type == ClientMessage) {
				switch (event.xclient.data.l[0]) {
				case XIMP_RESET_RETURN:
					{
						int rval;
						Atom actual_type_return;
						int actual_format_return;
						unsigned long nitems_return, bytes_after_return;
						unsigned char *p = NULL;
#ifdef XIMP_SIGNAL
						alarm(0);
#endif /* XIMP_SIGNAL */
						ic->ximp_icpart->icid = (ICID)event.xclient.data.l[1];
						rval = XGetWindowProperty(ic->core.im->core.display,
									((Ximp_XIM)ic->core.im)->ximp_impart->fe_window,
									(Atom)event.xclient.data.l[2], 0, 1024, True,
									AnyPropertyType, &actual_type_return,
									&actual_format_return, &nitems_return,
									&bytes_after_return, &p);
						return(p);
						}
				case XIMP_ERROR:
					_Ximp_ProcError (ic->core.im->core.display,
							 NULL, &event);
					_time_flag = 0;
					return(NULL);
				case XIMP_PREEDITDONE:
				case XIMP_PREEDITDRAW:
				case XIMP_PREEDITDRAW_CM:
				case XIMP_PREEDITDRAW_TINY:
				case XIMP_STATUSDONE:
				case XIMP_STATUSDRAW:
				case XIMP_STATUSDRAW_CM:
					_Ximp_CallCallback (ic->core.im->core.display, event.xclient.window, &event);
					break;
					}
				}
			else {
#ifdef XIMP_SIGNAL
				alarm(0);
#endif /* XIMP_SIGNAL */
				_Ximp_ProcError (ic->core.im->core.display,
							 NULL, &event);
				_time_flag = 0;
				return(NULL);
				}
			}
		_time_flag = 0;
		}
	return((unsigned char *)NULL);
}

#define XIMP_MAXBUF 1024

char *
_Ximp_MbReset(ic)
	Ximp_XIC	 ic;
{
	char *mb;
	int length = XIMP_MAXBUF +1;
	unsigned char *ct = _Ximp_Reset(ic);

	if (!ct) return(NULL);
	mb = Xmalloc(length);
	_Ximp_cttombs(ic->core.im->core.lcd, ct, strlen(ct), mb, &length, NULL);
	mb[length] = '\0';
	return(mb);
}

wchar_t *
_Ximp_WcReset(ic)
	Ximp_XIC	 ic;
{
	wchar_t *wc;
	int length = XIMP_MAXBUF +1;
	unsigned char *ct = _Ximp_Reset(ic);

	if (!ct) return(NULL);
	wc = (wchar_t *)Xmalloc(length * sizeof(wchar_t));
	_Ximp_cttowcs(ic->core.im->core.lcd, ct, strlen(ct), wc, &length, NULL);
	wc[length] = (wchar_t)0;
	return(wc);
}

#define	LookupKeypress	1
#define LookupProperty	2
#define LookupMessage	3

static int		_xim_lookup_sign;
static unsigned int	_xim_backup_keycode;
static unsigned int	_xim_backup_state;
static unsigned char	*_xim_prop_return;
static unsigned long	_xim_string_length;
static int		_xim_message_len;
static unsigned char	_xim_message_buf[24];

int
_Ximp_MbLookupString(ic, ev, buffer, bytes, keysym, status)
	Ximp_XIC	 ic;
	XKeyEvent	*ev;
	char		*buffer;
	int		 bytes;
	KeySym		*keysym;
	Status		*status;
{
	XComposeStatus	 comp_status;
	int		 ret = 0, len;

	if(ev->type == KeyPress && ev->keycode == 0) { /* Filter function */
		if (_xim_lookup_sign == LookupKeypress) {
			ev->state   = _xim_backup_state;
			ev->keycode = _xim_backup_keycode;
			ret = _Ximp_LookupMBText(ic, ev, buffer, bytes, keysym, &comp_status);
			ev->send_event = False ;
			if(ret > 0) {
				if(keysym && *keysym != NoSymbol) {
					if(status) *status = XLookupBoth;
					 }
				else {
					if(status) *status = XLookupChars;
					}
				}
			else {
				if(keysym && *keysym != NoSymbol) {
					if(status) *status = XLookupKeySym;
					}
				else {
					if(status) *status = XLookupNone;
					}
				}
			return(ret);
			}
		else if(_xim_lookup_sign == LookupProperty) {
			if (_Ximp_cttombs(ic->core.im->core.lcd,
					  _xim_prop_return,
					  _xim_string_length,
					  buffer, &bytes, NULL) < 0) 
			    ret = 0;
			else
			    ret = bytes;
			XFree((XPointer)_xim_prop_return);
			}
		else if (_xim_lookup_sign == LookupMessage) {
			if (_Ximp_cttombs(ic->core.im->core.lcd,
					  _xim_message_buf,
					  _xim_message_len,
					  buffer, &bytes, NULL) < 0) 
			    ret = 0;
			else
			    ret = bytes;
			}
		if(status)*status = (ret > 0) ? XLookupChars : XLookupNone;
		return(ret);
		}
	else if(ev->type == KeyPress) {
		if(ic->core.client_window == (Window)NULL) {
			if(status) *status = XLookupNone;
			return(0);
			}
		if (ic->ximp_icpart->input_mode) {/* ON : input_mode */
			_Ximp_IM_SendMessage(ic, XIMP_KEYPRESS,
						 (long)ev->keycode,
						 (long)ev->state, NULL);
			if(status) *status = XLookupNone;
			return(0);
			}
		ret = _Ximp_LookupMBText(ic, ev, buffer, bytes, keysym, &comp_status);
		if(ret >= 0) {
			if(_Ximp_StartXIMP(ic, ev, keysym ? *keysym : 0)) {
				if(status) *status = XLookupNone;
					return(0);
				}
			}
		if(ret > 0) {
			if(keysym && *keysym != NoSymbol) {
				if(status) *status = XLookupBoth;
				}
			 else {
				if(status) *status = XLookupChars;
				}
			}
		else {
			if(keysym && *keysym != NoSymbol) {
				if(status) *status = XLookupKeySym;
				}
			 else {
				if(status) *status = XLookupNone;
				}
			}
 		}
	else {
		if (status)
			*status = XLookupNone;
    		}
	return(ret);
}

int
_Ximp_WcLookupString(ic, ev, buffer, wlen, keysym, status)
	Ximp_XIC		 ic;
	XKeyEvent	*ev;
	wchar_t		*buffer;
	int		 wlen;
	KeySym		*keysym;
	Status		*status;
{
	XComposeStatus	 comp_status;
	int		 ret, len;
	char		 look[128];

	if(ev->type == KeyPress && ev->keycode == 0) { /* Filter function */
		if (_xim_lookup_sign == LookupKeypress) {
			ev->state   = _xim_backup_state;
			ev->keycode = _xim_backup_keycode;
			ret = _Ximp_LookupWCText(ic, ev, buffer, wlen, keysym, &comp_status);
			ev->send_event = False ;
			if(ret > 0) {
				if(keysym && *keysym != NoSymbol) {
					if(status) *status = XLookupBoth;
					 }
				else {
					if(status) *status = XLookupChars;
					}
				}
			else {
				if(keysym && *keysym != NoSymbol) {
					if(status) *status = XLookupKeySym;
					}
				else {
					if(status) *status = XLookupNone;
					}
				}
			return(ret);
			}
		else if(_xim_lookup_sign == LookupProperty) {
			len = wlen;
			if (_Ximp_cttowcs(ic->core.im->core.lcd,
							  _xim_prop_return,
							  _xim_string_length,
							  buffer, &len, NULL) < 0)
				ret = 0;
			else
				ret = len;
			XFree((XPointer)_xim_prop_return);
			}
		else if (_xim_lookup_sign == LookupMessage) {
			len = wlen;
			if (_Ximp_cttowcs(ic->core.im->core.lcd,
							  _xim_message_buf,
							  _xim_message_len,
							  buffer, &len, NULL) < 0)
				ret = 0;
			else
				ret = len;
			}
		if(status)*status = (ret > 0) ? XLookupChars : XLookupNone;
		return(ret);
		}
	else {
		if(ic->core.client_window == (Window)NULL) {
			if(status) *status = XLookupNone;
			return(0);
			}
		if (ic->ximp_icpart->input_mode) {/* ON : input_mode */
			_Ximp_IM_SendMessage(ic, XIMP_KEYPRESS,
						 (long)ev->keycode,
						 (long)ev->state, NULL);
			if(status) *status = XLookupNone;
			return(0);
			}
		ret = _Ximp_LookupWCText(ic, ev, buffer, wlen, keysym, &comp_status);
		if(ret >= 0) {
			if(_Ximp_StartXIMP(ic, ev, keysym ? *keysym : 0)) {
				if(status) *status = XLookupNone;
					return(0);
				}
			if(keysym && *keysym != NoSymbol) {
				if(status) *status = XLookupBoth;
				}
			 else {
				if(status) *status = XLookupChars;
				}
			}
		else {
			if(keysym && *keysym != NoSymbol) {
				if(status) *status = XLookupKeySym;
				}
			 else {
				if(status) *status = XLookupNone;
				}
			}
 		}
	return(ret);
}

static Bool
_Ximp_FocusInput (window, mask)
	Window		window;
	unsigned long	*mask;
{
	int		i;
	Ximp_XIM	pim;
	Ximp_XIC	pic;
	extern int	Ximp_Xim_count;
	extern Ximp_XIM	*Ximp_Xim_List;

	for(i = 0; i < Ximp_Xim_count; i++) {
		pim = Ximp_Xim_List[i];
		for (pic = (Ximp_XIC)pim->core.ic_chain;
			pic; pic = (Ximp_XIC)pic->core.next) {
			if(pic->core.focus_window == window &&
				pic->ximp_icpart->input_mode) {
				*mask = pic->ximp_icpart->back_mask;
				return(True);
			}
		}
	}
	return (False);
}

static Bool
_Ximp_StartXIMP(ic, ev, keysym)
	Ximp_XIC		 ic;
	XKeyEvent		*ev;
	KeySym			 keysym;
{
	Ximp_KeyList		*list;
	int			 i, isEventPassedToIMS;
	XWindowAttributes	 ret_attributes;
	unsigned long		 dummy_mask;
	XEvent			 Message;
	extern Bool		_Ximp_Setup ();

	if(ic->ximp_icpart->input_mode) /* ON : input_mode */
		return(False);

	if(!(((Ximp_XIM)ic->core.im)->ximp_impart->connectserver)) {
		if(keysym && keysym ==
		  (((Ximp_XIM)ic->core.im)->ximp_impart->def_startkeysym)) {
			if(_Ximp_Setup (ic->core.im) == False)
				isEventPassedToIMS = 1;
			else
				isEventPassedToIMS = 0;
		}
		else
			isEventPassedToIMS = 1;
	}
	else {	
		list = ((Ximp_XIM)ic->core.im)->ximp_impart->im_keyslist;
		for(i = 0, isEventPassedToIMS = 1; i < (int)list->count_keys; i++) {
			if(   (keysym && keysym == list->keys_list[i].keysym)
			   && ((ev->state & list->keys_list[i].modifier_mask)
			       == list->keys_list[i].modifier ) ) {
				isEventPassedToIMS = 0;
				break;
				}
			}
	}
	if(isEventPassedToIMS) return(False);

	if(ic->ximp_icpart->icid == NULL)
		if(!(_Ximp_SetOpenXIMP(ic))) return(False);
	_XRegisterFilterByType(ic->core.im->core.display,
			       ic->core.client_window,
			       ClientMessage, ClientMessage,
			       _Ximp_XimFilter_Client, NULL);
	_XRegisterFilterByType(ic->core.im->core.display,
			       ((Ximp_XIM)ic->core.im)->ximp_impart->fe_window,
			       DestroyNotify, DestroyNotify,
			       _Ximp_XimFilter_Destroy, NULL);
	XSelectInput (ic->core.im->core.display,
			((Ximp_XIM)ic->core.im)->ximp_impart->fe_window,
			StructureNotifyMask);

	if (_Ximp_FocusInput (ic->core.focus_window, &dummy_mask))
		ic->ximp_icpart->back_mask = dummy_mask;
	else {
		Display *d = ic->core.im->core.display;

		XGetWindowAttributes(d, ic->core.focus_window, &ret_attributes);
		dummy_mask = ret_attributes.your_event_mask;
		ic->ximp_icpart->back_mask = dummy_mask;
		if(ic->ximp_icpart->is_bep_mode == XIMP_FRONTEND)
			dummy_mask &= ~(KeyPressMask | KeyReleaseMask);
		else
			dummy_mask &= ~(KeyReleaseMask);
		XSelectInput(d, ic->core.focus_window, dummy_mask);
	}
	ic->ximp_icpart->input_mode = 1;
	_Ximp_IM_SendMessage(ic, XIMP_BEGIN, NULL, NULL, NULL);
	XFlush(ic->core.im->core.display);
	return(True);
}

Bool
_Ximp_SetOpenXIMP(ic)
	Ximp_XIC	ic;
{
	unsigned long	 mask;
	XEvent		 event;

	if(ic->core.client_window == (Window)NULL)
		return(False);

	if(!(ic->ximp_icpart->proto_mask & XIMP_FOCUS_WIN_MASK)) {
		ic->ximp_icpart->proto_mask |= XIMP_FOCUS_WIN_MASK;
		ic->core.focus_window = ic->core.client_window;
		}
	
	/* Property Data Set */
	XChangeProperty(ic->core.im->core.display, ic->core.client_window,
			((Ximp_XIM)ic->core.im)->ximp_impart->version_id,
			XA_STRING, 8, PropModeReplace,
			XIMP_PROTOCOL_VERSION, strlen(XIMP_PROTOCOL_VERSION));
	XFlush(ic->core.im->core.display);

	mask = ic->ximp_icpart->proto_mask;
	if(mask & XIMP_PROP_FOCUS)
		_Ximp_SetFocusWindow(ic);
	if(!(   (ic->core.input_style & XIMPreeditCallbacks)
	     || (ic->core.input_style & XIMPreeditNone) ) ) { 
		if(mask & XIMP_PROP_PREEDIT)
			_Ximp_SetPreeditAtr(ic);
		if(mask & XIMP_PROP_PREFONT)
			_Ximp_SetPreeditFont(ic);
		}
	else {
		mask &= ~(XIMP_PROP_PREEDIT | XIMP_PROP_PREFONT);
		}
	if(!(   (ic->core.input_style & XIMStatusCallbacks)
	     || (ic->core.input_style & XIMStatusNone) ) ) { 
		if(mask & XIMP_PROP_STATUS)
			_Ximp_SetStatusAtr(ic);
		if(mask & XIMP_PROP_STSFONT)
			_Ximp_SetStatusFont(ic);
		}
	else {
		mask &= ~(XIMP_PROP_STATUS | XIMP_PROP_STSFONT);
		}

	/* ClientMessage Send */
	_Ximp_IM_SendMessage(ic, XIMP_CREATE, ic->core.input_style, mask, NULL);

#ifdef XIMP_SIGNAL
	signal(SIGALRM, _time_out);
	alarm(XIMP_TIME_OUT);
#endif /* XIMP_SIGNAL */
	while(_time_flag != 1) {
		if( (XCheckTypedEvent(ic->core.im->core.display, ClientMessage, &event)) == False) {
#ifdef XIMP_SIGNAL
			sleep(1);
#endif /* XIMP_SIGNAL */
			continue;
			}
		if(event.xclient.message_type != ((Ximp_XIM)ic->core.im)->ximp_impart->improtocol_id) {
			XPutBackEvent(ic->core.im->core.display, &event);
			continue;
			}
		else if(event.xclient.data.l[0] != XIMP_CREATE_RETURN) {
#ifdef XIMP_SIGNAL
			alarm(0);
#endif /* XIMP_SIGNAL */
			_Ximp_ProcError (ic->core.im->core.display, NULL, &event);
			_time_flag = 0;
			/* return(False); */
			continue;
			}
		else { /* XIMP_CRETAE_RETURN   Event */
#ifdef XIMP_SIGNAL
			alarm(0);
#endif /* XIMP_SIGNAL */
			ic->ximp_icpart->icid = (ICID)event.xclient.data.l[1];
			_Ximp_A_CreateExtension(ic);
			_time_flag = 0;
			return(True);
			}
		}
	_time_flag = 0;
	return(False);
}

void
_Ximp_MakeKeypress (d, w, ev)
	Display			*d;
	Window			w;
	XKeyEvent		*ev;
{
	ev->type = KeyPress;
	ev->keycode = 0;
}

void
_Ximp_ProcKeypress (d, w, ev)
	Display			*d;
	Window			w;
	XClientMessageEvent	*ev;
{
	ICID	icid;

	icid = ev->data.l[1];
	_xim_backup_keycode = ev->data.l[2];
	_xim_backup_state = ev->data.l[3];
	_xim_lookup_sign = LookupKeypress;
}

static void
_Ximp_ProcCreateReturn (d, w, ev)
	Display			*d;
	Window			w;
	XClientMessageEvent	*ev;
{
	ICID	icid;

	icid = ev->data.l[1];
}

static void
_Ximp_ProcConversionBegin (d, w, ev)
	Display			*d;
	Window			w;
	XClientMessageEvent	*ev;
{
	ICID			icid;
	Ximp_XIC		ic;
	XWindowAttributes	ret_attributes;
	unsigned long		dummy_mask;

	icid = ev->data.l[1];
	ic   = _Ximp_LookupXIC(icid);

	if(ic->ximp_icpart->input_mode) /* ON : input_mode */
		return;

	if (_Ximp_FocusInput (ic->core.focus_window, &dummy_mask))
		ic->ximp_icpart->back_mask = dummy_mask;
	else {
		XGetWindowAttributes(d, ic->core.focus_window, &ret_attributes);
		dummy_mask = ret_attributes.your_event_mask;
		ic->ximp_icpart->back_mask = dummy_mask;
		if(ic->ximp_icpart->is_bep_mode == XIMP_FRONTEND)
			dummy_mask &= ~(KeyPressMask | KeyReleaseMask);
		else
			dummy_mask &= ~(KeyReleaseMask);
		XSelectInput(d, ic->core.focus_window, dummy_mask);
		XFlush(d);
	}
	ic->ximp_icpart->input_mode = 1;
	return;
}

static void
_Ximp_ProcConversionEnd (d, w, ev)
	Display			*d;
	Window			w;
	XClientMessageEvent	*ev;
{
	ICID			icid;
	Ximp_XIC		ic;

	icid = ev->data.l[1];
	ic   = _Ximp_LookupXIC(icid);
	XSelectInput(d, ic->core.focus_window, ic->ximp_icpart->back_mask );
	XFlush(d);
	ic->ximp_icpart->input_mode = 0;
	return;
}

static void
_Ximp_ProcReadProperty (d, w, ev)
	Display			*d;
	Window			w;
	XClientMessageEvent	*ev;
{
	Ximp_XIC		ic;
	ICID			icid;
	Atom			read_prop;
	int			rval;
	Atom			actual_type_return;
	int			actual_format_return;
	unsigned long		nitems_return;

	icid      = ev->data.l[1];
	read_prop = ev->data.l[2];
	ic   = _Ximp_LookupXIC(icid);
	rval = XGetWindowProperty( d,
				((Ximp_XIM)ic->core.im)->ximp_impart->fe_window,
				read_prop, 0, 1024, True,
				AnyPropertyType, &actual_type_return,
				&actual_format_return, &_xim_string_length,
				&nitems_return, &_xim_prop_return );
	/*
	 * Note:
	 *	After getting the result from _xim_prop_return,
	 *	do not forget to do XFree it.
	 */
	_xim_lookup_sign = LookupProperty;
}

static void
_Ximp_ProcError (d, w, ev)
	Display			*d;
	Window			w;
	XClientMessageEvent	*ev;
{
	ICID			icid;
	Ximp_XIC		ic;
	unsigned long		data[3];

	/*
	 * ToDo:
	 *	If you want to process the error from IM server,
	 *	you should modify this routine.
	 */

	icid = ev->data.l[1];
	ic   = _Ximp_LookupXIC(icid);
	if (ic->ximp_icpart->error.callback) {
		data[0] = ev->data.l[2];
		data[2] = ev->data.l[4];
		if(ev->data.l[0] != XIMP_ERROR)
			data[1] = XIMP_BadProtocol;
		else
			data[1]  = ev->data.l[3];

		(*ic->ximp_icpart->error.callback)(ic,
					ic->ximp_icpart->error.client_data,
					data);
		}
}

static void
_Ximp_ProcReadMessage (d, w, ev)
	Display			*d;
	Window			w;
	XClientMessageEvent	*ev;
{
	ICID	icid;
	long	nicid;

	nicid = *(unsigned long *)(&(ev->data.b[0]));
	icid = (ICID)ntohl(nicid);
	_xim_message_len = ev->data.b[4];
	strncpy ((char *)_xim_message_buf, &(ev->data.b[5]), _xim_message_len);
	_xim_message_buf[_xim_message_len] = 0;
	_xim_message_buf[_xim_message_len + 1] = 0;
	_xim_message_buf[_xim_message_len + 2] = 0;
	_xim_message_buf[_xim_message_len + 3] = 0;
	_xim_lookup_sign = LookupMessage;
}

static void
_Ximp_CallCallback (d, w, ev)
	Display			*d;
	Window			w;
	XClientMessageEvent	*ev;
{
	ICID		icid;
	Ximp_XIC	ic;

	icid = ev->data.l[1];
	ic   = _Ximp_LookupXIC(icid);
	switch (ev->data.l[0]) {
		case XIMP_GEOMETRY:
			_Ximp_CallGeometryCallback (ic, ev);
			break;
		case XIMP_PREEDITSTART:
			_Ximp_CallPreeditStartCallback (ic, ev);
			break;
		case XIMP_PREEDITDONE:
			_Ximp_CallPreeditDoneCallback (ic, ev);
			break;
		case XIMP_PREEDITDRAW:
			_Ximp_CallPreeditDrawCallback (ic, ev);
			break;
		case XIMP_PREEDITDRAW_CM:
			_Ximp_CallPreeditDrawCallback2 (ic, ev);
			break;
		case XIMP_PREEDITDRAW_TINY:
			_Ximp_CallPreeditDrawCallback3 (ic, ev);
			break;
		case XIMP_PREEDITCARET:
			_Ximp_CallPreeditCaretCallback (ic, ev);
			break;
		case XIMP_STATUSSTART:
			_Ximp_CallStatusStartCallback (ic, ev);
			break;
		case XIMP_STATUSDONE:
			_Ximp_CallStatusDoneCallback (ic, ev);
			break;
		case XIMP_STATUSDRAW:
			_Ximp_CallStatusDrawCallback (ic, ev);
			break;
		case XIMP_STATUSDRAW_CM:
			_Ximp_CallStatusDrawCallback2 (ic, ev);
			break;
		default:
			break;
		}
}

static Bool
_Ximp_ProtoReceive (d, w, ev, client_data)
	Display			*d;
	Window			w;
	XClientMessageEvent	*ev;
	XPointer		*client_data;
{
	if (ev->message_type != _Ximp_Protocol_id ())
		return (False);
	if (ev->format == 32) {
		switch (ev->data.l[0]) {
		case XIMP_KEYPRESS:
			_Ximp_ProcKeypress (d, w, ev);
			_Ximp_MakeKeypress (d, w, ev);
			ev->send_event = False ;
			XPutBackEvent(d, ev);
			break ;
		case XIMP_CREATE_RETURN:
			_Ximp_ProcCreateReturn (d, w, ev);
			break;
		case XIMP_CONVERSION_BEGIN:
			_Ximp_ProcConversionBegin (d, w, ev);
			break;
		case XIMP_CONVERSION_END:
			_Ximp_ProcConversionEnd (d, w, ev);
			break;
		case XIMP_READPROP:
			_Ximp_ProcReadProperty (d, w, ev);
			_Ximp_MakeKeypress (d, w, ev);
			ev->send_event = False ;
			XPutBackEvent(d, ev);
			break ;
		case XIMP_ERROR:
			_Ximp_ProcError (d, w, ev);
			break;
		case XIMP_GEOMETRY:
		case XIMP_PREEDITSTART:
		case XIMP_PREEDITDONE:
		case XIMP_PREEDITDRAW:
		case XIMP_PREEDITDRAW_CM:
		case XIMP_PREEDITCARET:
		case XIMP_STATUSSTART:
		case XIMP_STATUSDONE:
		case XIMP_STATUSDRAW:
		case XIMP_STATUSDRAW_CM:
			_Ximp_CallCallback (d, w, ev);
			break;
		case XIMP_EXTENSION:
			_Ximp_ProcExtension(d, w, ev);
			break;
		default:
			break;
		}
        } else if (ev->format == 8) {
		_Ximp_ProcReadMessage (d, w, ev);
		_Ximp_MakeKeypress (d, w, ev);
		ev->send_event = False ;
		XPutBackEvent(d, ev);
        }
	return (True);
}

static Bool
_Ximp_ServerDestroy (d, w, ev, client_data)
	Display			*d;
	Window			w;
	XEvent			*ev;
	XPointer		*client_data;
{
	extern Ximp_XIM		*Ximp_Xim_List;
	extern int		Ximp_Xim_count;
	register int		i;
	register XIMXimpRec	*ximp_impart;
	register XIC		 ic;
	long			dummy_mask;

	for(i=0; i < Ximp_Xim_count; i++) {
		if(Ximp_Xim_List[i]->ximp_impart->fe_window == w)
			ximp_impart = Ximp_Xim_List[i]->ximp_impart;
		else
			continue;
		_Ximp_SetupFreeExtension(Ximp_Xim_List[i]);
		_Ximp_SetupFree(ximp_impart->im_proto_vl,
			ximp_impart->im_styles,
			ximp_impart->im_keyslist,
			ximp_impart->im_server_name,
			ximp_impart->im_server_vl,
			ximp_impart->im_vendor_name,
			ximp_impart->im_ext_list);
		ximp_impart->connectserver = 0;
		for(ic = Ximp_Xim_List[i]->core.ic_chain; ic; ic = ic->core.next) {
			((Ximp_XIC)ic)->ximp_icpart->icid = NULL;
			if(((Ximp_XIC)ic)->ximp_icpart->input_mode) {/* ON : input_mode */
				dummy_mask = ((Ximp_XIC)ic)->ximp_icpart->back_mask;
				XSelectInput(ic->core.im->core.display,
				     ic->core.focus_window, dummy_mask);
				((Ximp_XIC)ic)->ximp_icpart->input_mode = 0;
			}
		}
	}
	XFlush (d);
	return (False);
}

Bool
_Ximp_Keypress (d, w, ev, ic)
	Display			*d;
	Window			w;
	XKeyEvent		*ev;
	Ximp_XIC		ic;
{
#define BUFFLIM		32
	KeySym		ks;
	char		buff[BUFFLIM];

	XLookupString (ev, buff, BUFFLIM, &ks, NULL);
	return _Ximp_StartXIMP (ic, ev, ks);
}

/*
 *  _Ximp_XimFilter
 *	Regist _Ximp_XimFilter_Client filter using XRegisterFilterByType
 *	with start_type == end_type  == ClientMessage
 *	Regist _Ximp_XimFilter_Destroy filter using XRegisterFilterByType
 *	with start_type == end_type == DestroyNotify
 */

Bool
_Ximp_XimFilter_Client (d, w, ev, client_data)
	Display		*d;
	Window		w;
	XEvent		*ev;
	XPointer	*client_data;
{
	return(_Ximp_ProtoReceive (d, w, ev, client_data));
}

Bool
_Ximp_XimFilter_Destroy (d, w, ev, client_data)
	Display		*d;
	Window		w;
	XEvent		*ev;
	XPointer	*client_data;
{
	return(_Ximp_ServerDestroy (d, w, ev, client_data));
}
