/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpICG.c	1.2"
/* $XConsortium: XimpICG.c,v 1.5 91/10/10 20:08:25 rws Exp $ */
/******************************************************************

    Copyright 1991, by FUJITSU LIMITED.
    Copyright 1991, by Sun Microsystems, Inc.

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,

FUJITSU LIMITED AND SUN MICROSYSTEMS, INC. DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MICROSYSTEMS, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR

Author: Takashi Fujiwara     FUJITSU LIMITED
		       fujiwara@a80.tech.yk.fujitsu.co.jp
        Hideki Hiura (hhiura@Sun.COM)
				     Sun Microsystems, Inc.

******************************************************************/

#define NEED_EVENTS
#include <X11/keysym.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include <X11/Xatom.h>

#include "Ximplc.h"

#ifdef XIMP_SIGNAL
#include <sys/signal.h>
#endif /* XIMP_SIGNAL */

extern char 		*_Ximp_GetICValues();
extern Bool		 _Ximp_GetICExtension();

static Bool 		 _Ximp_PreGetAttributes();
static Bool 		 _Ximp_StatusGetAttributes();
static XPointer		 _Ximp_GetRequestIM();

char *
_Ximp_GetICValues(ic, values)
	Ximp_XIC	 ic;
	XIMArg		*values;
{
	XIMArg		*p;
	char		*p_char;
	long		*p_long;
	XIMCallback 	*p_callback;
	char		*return_name = NULL;
	int		 len;

	for(p = values; p->name != NULL; p++) {
		if(strcmp(p->name, XNInputStyle) == 0) {
			if(ic->ximp_icpart->value_mask & XIMP_INPUT_STYLE) {
			    (*((long *)(p->value))) = (long)ic->core.input_style;
			} else {			    
			    return_name = p->name;
			    break;
			}
		} else if(strcmp(p->name, XNClientWindow)==0) {
			if(ic->ximp_icpart->value_mask & XIMP_CLIENT_WIN) {
			    (*((long *)(p->value))) = (long)ic->core.client_window;
			} else {
			    return_name = p->name;
			    break;
			}
		} else if(strcmp(p->name, XNFocusWindow)==0) {
			if(ic->ximp_icpart->proto_mask & XIMP_FOCUS_WIN_MASK) {
			    (*((long *)(p->value))) = (long)ic->core.focus_window;
			} else if(((Ximp_XIM)ic->core.im)->ximp_impart->inputserver){
			    return_name = p->name;
			    break;
			} else {
			    XPointer tmp = _Ximp_GetRequestIM(ic,
						 XIMP_FOCUS_WIN_MASK,
						((Ximp_XIM)ic->core.im)->ximp_impart->focus_win_id,
						XA_WINDOW);
			    (*((long *)(p->value))) = *(long*)tmp ;
			    free(tmp) ;
			}
		} else if(strcmp(p->name, XNResourceName)==0) {
			if(ic->core.im->core.res_name != (char *)NULL) {
			    len = strlen(ic->core.im->core.res_name);
			    if((p_char = Xmalloc(sizeof(len+1))) == NULL) {
				return_name = p->name;
				break;
			    }
			    strcpy(p_char, ic->core.im->core.res_name);
			    (*((long *)(p->value))) = (long)p_char;
			} else {
			    return_name = p->name;
			    break;
			}
		} else if(strcmp(p->name, XNResourceClass)==0) {
			if(ic->core.im->core.res_class != (char *)NULL) {
			    (*((long *)(p->value))) = (long)ic->core.im->core.res_class;
			} else {
			    return_name = p->name;
			    break;
			}
		} else if(strcmp(p->name, XNGeometryCallback)==0) {
			if(ic->ximp_icpart->value_mask & XIMP_GEOMETRY_CB) {
			    (*((long *)(p->value))) = (long)ic->core.geometry_callback.callback;
			} else {
			    return_name = p->name;
			    break;
			}
		} else if(strcmp(p->name, XNFilterEvents)==0) {
		    (*((long *)(p->value))) = (long)ic->core.filter_events;
		} else if(strcmp(p->name, XNPreeditAttributes)==0) {
			if( _Ximp_PreGetAttributes(ic, p->value,
						   return_name) == False)
				break;
		} else if(strcmp(p->name, XNStatusAttributes)==0) {
			if( _Ximp_StatusGetAttributes(ic, p->value,
						   return_name) == False)
				break;
		} else {
			if( _Ximp_GetICExtension(ic, p->name, p->value) == False) {
				return_name = p->name;
				break;
			}
		}
	}
	return(return_name);
}

static Bool
_Ximp_PreGetAttributes(ic, vl, return_name)
	Ximp_XIC	 ic;
	XIMArg		*vl;
	char		*return_name;
{
	XIMArg		*p;
	XRectangle	*p_rect;
	XPoint		*p_point;
	long		*p_long;
	unsigned long	 mask;
	int		 im_preedit_flag = 0;
	int		 im_font_flag    = 0;
	Ximp_PreeditPropRec	*preedit_data;
	XIMCallback 	*p_callback;

	if(((Ximp_XIM)ic->core.im)->ximp_impart->inputserver) {
		for(mask = 0, p = vl; p->name != NULL; p++) {
			if(strcmp(p->name, XNArea)==0)
				mask |= XIMP_PRE_AREA_MASK;
			else if(strcmp(p->name, XNAreaNeeded)==0)
				mask |= XIMP_PRE_AREANEED_MASK;
			else if(strcmp(p->name, XNSpotLocation)==0)
				mask |= XIMP_PRE_SPOTL_MASK;
			else if(strcmp(p->name, XNColormap)==0)
				mask |= XIMP_PRE_COLORMAP_MASK;
			else if(strcmp(p->name, XNStdColormap)==0)
				mask |= XIMP_PRE_COLORMAP_MASK;
			else if(strcmp(p->name, XNBackground)==0)
				mask |= XIMP_PRE_BG_MASK;
			else if(strcmp(p->name, XNForeground)==0)
				mask |= XIMP_PRE_FG_MASK;
			else if(strcmp(p->name, XNBackgroundPixmap)==0)
				mask |= XIMP_PRE_BGPIXMAP_MASK;
			else if(strcmp(p->name, XNLineSpace)==0)
				mask |= XIMP_PRE_LINESP_MASK;
			else if(strcmp(p->name, XNCursor)==0)
				mask |= XIMP_PRE_CURSOR_MASK;
			else if(strcmp(p->name, XNFontSet)==0)
				im_font_flag = 1;
		}
		if(mask) {
			preedit_data = (Ximp_PreeditPropRec *)_Ximp_GetRequestIM(ic, mask,
				((Ximp_XIM)ic->core.im)->ximp_impart->preedit_atr_id,
				((Ximp_XIM)ic->core.im)->ximp_impart->preedit_atr_id);
			if(preedit_data != (Ximp_PreeditPropRec *)NULL)
				im_preedit_flag = 1;
		}
	}
	for(p = vl; p->name != NULL; p++) {
		if(strcmp(p->name, XNArea)==0) {
			if(im_preedit_flag) {
				p_rect = (XRectangle *)(p->value) ;
				p_rect->x       = preedit_data->Area.x;
				p_rect->y       = preedit_data->Area.y;
				p_rect->width   = preedit_data->Area.width;
				p_rect->height  = preedit_data->Area.height;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_PRE_AREA_MASK) {
				        p_rect = (XRectangle *)(p->value) ;
					p_rect->x       = ic->core.preedit_attr.area.x;
					p_rect->y       = ic->core.preedit_attr.area.y;
					p_rect->width   = ic->core.preedit_attr.area.width;
					p_rect->height  = ic->core.preedit_attr.area.height;
				} else {
				        return_name = p->name;
				        return(False);
				}
			}
		} else if(strcmp(p->name, XNAreaNeeded)==0) {
			if(im_preedit_flag) {
				p_rect =  (XRectangle *)(p->value) ;
				p_rect->x  = p_rect->y  = 0;
				p_rect->width   = preedit_data->AreaNeeded.width;
				p_rect->height  = preedit_data->AreaNeeded.height;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_PRE_AREANEED_MASK) {
				        p_rect = (XRectangle *)(p->value) ;
					p_rect->x  = p_rect->y  = 0;
					p_rect->width   = ic->core.preedit_attr.area_needed.width;
					p_rect->height  = ic->core.preedit_attr.area_needed.height;
				} else {
				        return_name = p->name;
				        return(False);
				}
			}
		} else if(strcmp(p->name, XNSpotLocation)==0) {
			if(im_preedit_flag) {
			        p_point = (XPoint *)(p->value);
				p_point->x = preedit_data->SpotLocation.x;
				p_point->y = preedit_data->SpotLocation.y;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_PRE_SPOTL_MASK) {
				        p_point = (XPoint *)(p->value);
					p_point->x = ic->core.preedit_attr.spot_location.x;
					p_point->y = ic->core.preedit_attr.spot_location.y;
				} else {
				        return_name = p->name;
				        return(False);
				}
			}
		} else if(  strcmp(p->name, XNColormap)==0
		        || strcmp(p->name, XNStdColormap)==0) {
			if(im_preedit_flag) {
			         p_long = (long *)(p->value);
				*p_long = preedit_data->Colormap;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_PRE_COLORMAP_MASK) {
				         p_long = (long *)(p->value);
					*p_long = ic->core.preedit_attr.colormap;
				} else {
					return_name = p->name;
					return(False);
				}
			}
		} else if(strcmp(p->name, XNBackground)==0) {
			if(im_preedit_flag) {
			         p_long = (long *)(p->value);
				*p_long = preedit_data->Background;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_PRE_BG_MASK) {
				         p_long = (long *)(p->value);
					*p_long = ic->core.preedit_attr.background;
				} else {
					return_name = p->name;
					return(False);
				}
			}
		} else if(strcmp(p->name, XNForeground)==0) {
			if(im_preedit_flag) {
			         p_long = (long *)(p->value);
				*p_long = preedit_data->Foreground;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_PRE_FG_MASK) {
				         p_long = (long *)(p->value);
					*p_long = ic->core.preedit_attr.foreground;
				} else {
					return_name = p->name;
					return(False);
				}
			}
		} else if(strcmp(p->name, XNBackgroundPixmap)==0) {
			if(im_preedit_flag) {
			         p_long = (long *)(p->value);
				*p_long = preedit_data->Bg_Pixmap;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_PRE_BGPIXMAP_MASK) {
				         p_long = (long *)(p->value);
					*p_long = ic->core.preedit_attr.background_pixmap;
				} else {
					return_name = p->name;
					return(False);
				}
			}
		} else if(strcmp(p->name, XNFontSet)==0) {
			if(ic->ximp_icpart->proto_mask & XIMP_PRE_FONT_MASK) {
			         p_long = (long *)(p->value);
				*p_long = (long)ic->core.preedit_attr.fontset;
			} else {
				return_name = p->name;
				return(False);
			}
		} else if(strcmp(p->name, XNLineSpace)==0) {
			if(im_preedit_flag) {
			         p_long = (long *)(p->value);
				*p_long = preedit_data->LineSpacing;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_PRE_LINESP_MASK) {
				         p_long = (long *)(p->value);
					*p_long = ic->core.preedit_attr.line_space;
				} else {
					return_name = p->name;
					return(False);
				}
			}
		} else if(strcmp(p->name, XNCursor)==0) {
			if(im_preedit_flag) {
			         p_long = (long *)(p->value);
				*p_long = preedit_data->Cursor;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_PRE_CURSOR_MASK) {
				         p_long = (long *)(p->value);
					*p_long = ic->core.preedit_attr.cursor;
				} else {
					return_name = p->name;
					return(False);
				}
			}
		} else if(strcmp(p->name, XNPreeditStartCallback)==0) {
			if((int)ic->core.preedit_attr.callbacks.start.callback) {
			        p_callback = (XIMCallback *)(p->value) ;
				p_callback->client_data =
					ic->core.preedit_attr.callbacks.start.client_data;
				p_callback->callback =
					ic->core.preedit_attr.callbacks.start.callback;
			} else {
				return_name = p->name;
				return(False);
			}
		} else if(strcmp(p->name, XNPreeditDrawCallback)==0) {
			if((int)ic->core.preedit_attr.callbacks.draw.callback) {
			        p_callback = (XIMCallback *)(p->value) ;
				p_callback->client_data =
					ic->core.preedit_attr.callbacks.draw.client_data;
				p_callback->callback =
					ic->core.preedit_attr.callbacks.draw.callback;
			} else {
				return_name = p->name;
				return(False);
			}
		} else if(strcmp(p->name, XNPreeditDoneCallback)==0) {
			if((int)ic->core.preedit_attr.callbacks.done.callback) {
			        p_callback = (XIMCallback *)(p->value) ;
				p_callback->client_data =
					ic->core.preedit_attr.callbacks.done.client_data;
				p_callback->callback =
					ic->core.preedit_attr.callbacks.done.callback;
			} else {
				return_name = p->name;
				return(False);
			}
		} else if(strcmp(p->name, XNPreeditCaretCallback)==0) {
			if((int)ic->core.preedit_attr.callbacks.caret.callback) {
			        p_callback = (XIMCallback *)(p->value) ;
				p_callback->client_data =
					ic->core.preedit_attr.callbacks.caret.client_data;
				p_callback->callback =
					ic->core.preedit_attr.callbacks.caret.callback;
			} else {
				return_name = p->name;
				return(False);
			}
		}
	}
	XFree(preedit_data);
	return(True);
}

static Bool
_Ximp_StatusGetAttributes(ic, vl, return_name)
	Ximp_XIC	 ic;
	XIMArg	 	*vl;
	char		*return_name;
{
	XIMArg		*p;
	XRectangle	*p_rect;
	XPoint		*p_point;
	long		*p_long;
	unsigned long	 mask;
	int		 im_status_flag = 0;
	int		 im_font_flag    = 0;
	Ximp_StatusPropRec	*status_data;
	XIMCallback 	*p_callback;

	if(((Ximp_XIM)ic->core.im)->ximp_impart->inputserver) {
		for(mask = 0, p = vl; p->name != NULL; p++) {
			if(strcmp(p->name, XNArea)==0)
				mask |= XIMP_STS_AREA_MASK;
			else if(strcmp(p->name, XNAreaNeeded)==0)
				mask |= XIMP_STS_AREANEED_MASK;
			else if(strcmp(p->name, XNColormap)==0)
				mask |= XIMP_STS_COLORMAP_MASK;
			else if(strcmp(p->name, XNStdColormap)==0)
				mask |= XIMP_STS_COLORMAP_MASK;
			else if(strcmp(p->name, XNBackground)==0)
				mask |= XIMP_STS_BG_MASK;
			else if(strcmp(p->name, XNForeground)==0)
				mask |= XIMP_STS_FG_MASK;
			else if(strcmp(p->name, XNBackgroundPixmap)==0)
				mask |= XIMP_STS_BGPIXMAP_MASK;
			else if(strcmp(p->name, XNLineSpace)==0)
				mask |= XIMP_STS_LINESP_MASK;
			else if(strcmp(p->name, XNCursor)==0)
				mask |= XIMP_STS_CURSOR_MASK;
			else if(strcmp(p->name, XNFontSet)==0)
				im_font_flag = 1;
		}
		if(mask) {
			status_data = (Ximp_StatusPropRec *)_Ximp_GetRequestIM(ic, mask,
				((Ximp_XIM)ic->core.im)->ximp_impart->status_atr_id,
				((Ximp_XIM)ic->core.im)->ximp_impart->status_atr_id);
			if(status_data != (Ximp_StatusPropRec *)NULL)
				im_status_flag = 1;
		}
	}

	for(p = vl; p->name != NULL; p++) {
		if(strcmp(p->name, XNArea)==0) {
			if(im_status_flag) {
				p_rect = (XRectangle *)(p->value) ;
				p_rect->x       = status_data->Area.x;
				p_rect->y       = status_data->Area.y;
				p_rect->width   = status_data->Area.width;
				p_rect->height  = status_data->Area.height;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_STS_AREA_MASK) {
					p_rect = (XRectangle *)(p->value) ;
					p_rect->x       = ic->core.status_attr.area.x;
					p_rect->y       = ic->core.status_attr.area.y;
					p_rect->width   = ic->core.status_attr.area.width;
					p_rect->height  = ic->core.status_attr.area.height;
				} else {
				        return_name = p->name;
				        return(False);
				}
			}
		} else if(strcmp(p->name, XNAreaNeeded)==0) {
			if(im_status_flag) {
				p_rect =  (XRectangle *)(p->value) ;
				p_rect->x  = p_rect->y  = 0;
				p_rect->width   = status_data->AreaNeeded.width;
				p_rect->height  = status_data->AreaNeeded.height;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_STS_AREANEED_MASK) {
					p_rect =  (XRectangle *)(p->value) ;
					p_rect->x  = p_rect->y  = 0;
					p_rect->width   = ic->core.status_attr.area_needed.width;
					p_rect->height  = ic->core.status_attr.area_needed.height;
				} else {
					return_name = p->name;
					return(False);
				}
			}
		} else if(  strcmp(p->name, XNColormap)==0
		        || strcmp(p->name, XNStdColormap)==0) {
			if(im_status_flag) {
			         p_long = (long *)(p->value);
				*p_long = status_data->Colormap;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_STS_COLORMAP_MASK) {
				         p_long = (long *)(p->value);
					*p_long = ic->core.status_attr.colormap;
				} else {
					return_name = p->name;
					return(False);
				}
			}
		} else if(strcmp(p->name, XNBackground)==0) {
			if(im_status_flag) {
			         p_long = (long *)(p->value);
				*p_long = status_data->Background;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_STS_BG_MASK) {
				         p_long = (long *)(p->value);
					*p_long = ic->core.status_attr.background;
				} else {
					return_name = p->name;
					return(False);
			        }
			}
		} else if(strcmp(p->name, XNForeground)==0) {
			if(im_status_flag) {
			         p_long = (long *)(p->value);
				*p_long = status_data->Foreground;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_STS_FG_MASK) {
				         p_long = (long *)(p->value);
					*p_long = ic->core.status_attr.foreground;
				} else {
					return_name = p->name;
					return(False);
				}
			}
		} else if(strcmp(p->name, XNBackgroundPixmap)==0) {
			if(im_status_flag) {
			         p_long = (long *)(p->value);
				*p_long = status_data->Bg_Pixmap;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_STS_BGPIXMAP_MASK) {
				         p_long = (long *)(p->value);
					*p_long = ic->core.status_attr.background_pixmap;
				} else {
					return_name = p->name;
					return(False);
			        }
			}
		} else if(strcmp(p->name, XNFontSet)==0) {
			if(ic->ximp_icpart->proto_mask & XIMP_STS_FONT_MASK) {
			         p_long = (long *)(p->value);
				*p_long = (long)ic->core.status_attr.fontset;
			} else {
				return_name = p->name;
				return(False);
			}
		} else if(strcmp(p->name, XNLineSpace)==0) {
			if(im_status_flag) {
			         p_long = (long *)(p->value);
				*p_long = status_data->LineSpacing;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_STS_LINESP_MASK) {
				         p_long = (long *)(p->value);
					*p_long = ic->core.status_attr.line_space;
				} else {
					return_name = p->name;
					return(False);
				}

			}
		} else if(strcmp(p->name, XNCursor)==0) {
			if(im_status_flag) {
			         p_long = (long *)(p->value);
				*p_long = status_data->Cursor;
			} else {
				if(ic->ximp_icpart->proto_mask & XIMP_STS_CURSOR_MASK) {
				         p_long = (long *)(p->value);
					*p_long = ic->core.status_attr.cursor;
				} else { 
					return_name = p->name;
					return(False);
			        }
			}
		} else if(strcmp(p->name, XNStatusStartCallback)==0) {
			if((int)ic->core.status_attr.callbacks.start.callback) {
			        p_callback = (XIMCallback *)(p->value) ;
				p_callback->client_data =
					ic->core.status_attr.callbacks.start.client_data;
				p_callback->callback =
					ic->core.status_attr.callbacks.start.callback;
			} else {
				return_name = p->name;
				break;
			}
		} else if(strcmp(p->name, XNStatusDrawCallback)==0) {
			if((int)ic->core.status_attr.callbacks.draw.callback) {
			        p_callback = (XIMCallback *)(p->value) ;
				p_callback->client_data =
					ic->core.status_attr.callbacks.draw.client_data;
				p_callback->callback =
					ic->core.status_attr.callbacks.draw.callback;
			} else {
				return_name = p->name;
				break;
			}
		} else if(strcmp(p->name, XNStatusDoneCallback)==0) {
			if((int)ic->core.status_attr.callbacks.done.callback) {
			        p_callback = (XIMCallback *)(p->value) ;
				p_callback->client_data =
					ic->core.status_attr.callbacks.done.client_data;
				p_callback->callback =
					ic->core.status_attr.callbacks.done.callback;
			} else {
				return_name = p->name;
				break;
			}
		}
	}
	XFree(status_data);
	return(True);
}

static int	 _time_flag = 0;

#ifdef XIMP_SIGNAL
static int
_time_out()
{
	_time_flag = 1;
}
#endif /* XIMP_SIGNAL */

static XPointer
_Ximp_GetRequestIM(ic, mask, get_atom_id, atom_id)
	Ximp_XIC	 ic;
	unsigned long	 mask;
	Atom		 get_atom_id, atom_id;
{
	XEvent		 event;
	Atom            actual_type_ret;
	int		actual_format_ret;
	unsigned long   nitems_ret;
	unsigned long   bytes_after_ret;
	unsigned char   *data;

	if(ic->ximp_icpart->icid == (ICID)NULL)
		return(NULL);
	if(!(ic->ximp_icpart->value_mask & XIMP_CLIENT_WIN))
		return(NULL);

	_Ximp_IM_SendMessage(ic, XIMP_GETVALUE, mask, NULL, NULL);
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
		if(event.xclient.message_type == ((Ximp_XIM)ic->core.im)->ximp_impart->improtocol_id) {
#ifdef XIMP_SIGNAL
			alarm(0);
#endif /* XIMP_SIGNAL */
			break;
		} else {
			XPutBackEvent(ic->core.im->core.display, &event);
			continue;
		}
	}
	_time_flag = 0;

	if(event.xclient.data.l[0] != XIMP_GETVALUE_RETURN)
		return(NULL);

	XGetWindowProperty(ic->core.im->core.display,
			   ic->core.client_window,
			   get_atom_id, 0L, 1000000L, True, atom_id,
			   &actual_type_ret, &actual_format_ret, &nitems_ret,
			   &bytes_after_ret, &data);

        if(actual_format_ret == 0 || nitems_ret == 0)
		return(NULL);
	return((XPointer)data);
}
