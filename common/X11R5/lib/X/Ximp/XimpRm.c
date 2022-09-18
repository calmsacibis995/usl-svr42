/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Ximp/XimpRm.c	1.2"
/* $XConsortium: XimpRm.c,v 1.2 91/10/07 17:50:27 rws Exp $ */
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
#include <X11/keysym.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximplc.h"
#include "Xresource.h"

extern void		_Ximp_OpenIMResourceExtension();

void
_Ximp_Get_resource_name(im, res_name, res_class)
	Ximp_XIM	 im;
	char		*res_name;
	char		*res_class;
	{
	if(im->core.res_name == NULL)
		strcpy(res_name, "*");
	else	{
		strcpy(res_name, im->core.res_name);
		strcat(res_name, ".");
		}
	if(im->core.res_class == NULL)
		strcpy(res_class, "*");
	else	{
		strcpy(res_class, im->core.res_class);
		strcat(res_name, ".");
		}
	strcat(res_name, "ximp.");
	strcat(res_class, "Ximp.");
	}

Bool
_Ximp_OpenIM_Resource(im)
	Ximp_XIM	 im;
	{
	char		 res_name[256];
	char		 res_class[256];
	char		*str_type[50];
	XrmValue	 value;
	Bool		 ret = False;
	KeySym		 keysym = NoSymbol;

	if(im->core.rdb == NULL)
		return(ret);

	/* Inputserver */
	_Ximp_Get_resource_name(im, res_name, res_class);
	strcat(res_name, "inputserver");
	strcat(res_class, "Inputserver");
	if(XrmGetResource(im->core.rdb, res_name, res_class,
				str_type, &value) == True) { 
		if(strcmp(value.addr, "off") == 0) {
			/* Keysym */
			_Ximp_Get_resource_name(im, res_name, res_class);
			strcat(res_name, "startkeysym");
			strcat(res_class, "Startkeysym");
			if(XrmGetResource(im->core.rdb, res_name, res_class,
				str_type, &value) == True) { 
				keysym = XStringToKeysym(value.addr);
				}
			if(keysym == NoSymbol)
				ret = False;
			else {
				im->ximp_impart->def_startkeysym = keysym;
				ret = True;
				}
			}
		}
	/* Call Back */
	_Ximp_Get_resource_name(im, res_name, res_class);
	strcat(res_name, "callbackEncoding");
	strcat(res_class, "CallbackEncoding");
	if(XrmGetResource(im->core.rdb, res_name, res_class,
				str_type, &value) == True) { 
		if(strcmp(value.addr, "wchar") == 0) {
			im->ximp_impart->use_wchar = True;
			}
		}
	/* Extension : XOpenIM(, rdb, res_name, res_class) */
	_Ximp_OpenIMResourceExtension(im);
	return(ret);
	}

void
_Ximp_SetValue_Resource(ic, mask)
	Ximp_XIC	 ic;
	long		*mask;
	{
	Ximp_XIM	 im;
	char		 res_name[256];
	char		 res_class[256];
	char		*str_type[50];
	XrmValue	 value;
	Colormap	 default_colormap;
	XColor		 screen_def, exact_def;
	int		 num;

	im = (Ximp_XIM)XIMOfIC(ic);
	if(im->core.rdb == NULL)
		return;

	if(!(   (ic->core.input_style & XIMPreeditCallbacks)
	     || (ic->core.input_style & XIMPreeditNone) ) ) {
		if(!(ic->ximp_icpart->proto_mask & XIMP_PRE_BG_MASK)) {
			_Ximp_Get_resource_name(im, res_name, res_class);
			strcat(res_name, "preedit.background");
			strcat(res_class, "Preedit.Background");
			if(XrmGetResource(im->core.rdb, res_name, res_class,
					str_type, &value) == True) { 
				default_colormap = DefaultColormap(
						im->core.display,
						DefaultScreen(im->core.display) );
				if( XAllocNamedColor(im->core.display, default_colormap,
					     value.addr,
					     &screen_def, &exact_def) ) {
					ic->core.preedit_attr.background = screen_def.pixel;
					ic->ximp_icpart->preedit_attr.Background = 
						ic->core.preedit_attr.background;
					ic->ximp_icpart->proto_mask |= XIMP_PRE_BG_MASK;
					*mask                       |= XIMP_PRE_BG_MASK;
					}
				}
			}
		if(!(ic->ximp_icpart->proto_mask & XIMP_PRE_FG_MASK)) {
			_Ximp_Get_resource_name(im, res_name, res_class);
			strcat(res_name, "preedit.foreground");
			strcat(res_class, "Preedit.Foreground");
			if(XrmGetResource(im->core.rdb, res_name, res_class,
					str_type, &value) == True) { 
				default_colormap = DefaultColormap(
						im->core.display,
						DefaultScreen(im->core.display) );
				if( XAllocNamedColor(im->core.display, default_colormap,
					     value.addr,
					     &screen_def, &exact_def) ) {
					ic->core.preedit_attr.foreground = screen_def.pixel;
					ic->ximp_icpart->preedit_attr.Foreground = 
						ic->core.preedit_attr.foreground;
					ic->ximp_icpart->proto_mask |= XIMP_PRE_FG_MASK;
					*mask                       |= XIMP_PRE_FG_MASK;
					}
				}
			}
		if(!(ic->ximp_icpart->proto_mask & XIMP_PRE_LINESP_MASK)) {
			_Ximp_Get_resource_name(im, res_name, res_class);
			strcat(res_name, "preedit.linespacing");
			strcat(res_class, "Preedit.Linespacing");
			if(XrmGetResource(im->core.rdb, res_name, res_class,
					str_type, &value) == True) { 
				num = atoi(value.addr);
				ic->core.preedit_attr.line_space = num;
				ic->ximp_icpart->preedit_attr.LineSpacing = 
					ic->core.preedit_attr.line_space;
				ic->ximp_icpart->proto_mask |= XIMP_PRE_LINESP_MASK;
				*mask                       |= XIMP_PRE_LINESP_MASK;
				}
			}
		}
	if(!(   (ic->core.input_style & XIMStatusCallbacks)
	     || (ic->core.input_style & XIMStatusNone) ) ) {
		if(!(ic->ximp_icpart->proto_mask & XIMP_STS_BG_MASK)) {
			_Ximp_Get_resource_name(im, res_name, res_class);
			strcat(res_name, "status.background");
			strcat(res_class, "Status.Background");
			if(XrmGetResource(im->core.rdb, res_name, res_class,
					str_type, &value) == True) { 
				default_colormap = DefaultColormap(
						im->core.display,
						DefaultScreen(im->core.display) );
				if( XAllocNamedColor(im->core.display, default_colormap,
					     value.addr,
					     &screen_def, &exact_def) ) {
					ic->core.status_attr.background = screen_def.pixel;
					ic->ximp_icpart->status_attr.Background = 
						ic->core.status_attr.background;
					ic->ximp_icpart->proto_mask |= XIMP_STS_BG_MASK;
					*mask                       |= XIMP_STS_BG_MASK;
					}
				}

			}
		if(!(ic->ximp_icpart->proto_mask & XIMP_STS_FG_MASK)) {
			_Ximp_Get_resource_name(im, res_name, res_class);
			strcat(res_name, "status.foreground");
			strcat(res_class, "Status.Foreground");
			if(XrmGetResource(im->core.rdb, res_name, res_class,
					str_type, &value) == True) { 
				default_colormap = DefaultColormap(
						im->core.display,
						DefaultScreen(im->core.display) );
				if( XAllocNamedColor(im->core.display, default_colormap,
					     value.addr,
					     &screen_def, &exact_def) ) {
					ic->core.status_attr.foreground = screen_def.pixel;
					ic->ximp_icpart->status_attr.Foreground = 
						ic->core.status_attr.foreground;
					ic->ximp_icpart->proto_mask |= XIMP_STS_FG_MASK;
					*mask                       |= XIMP_STS_FG_MASK;
					}
				}
			}
		if(!(ic->ximp_icpart->proto_mask & XIMP_STS_LINESP_MASK)) {
			_Ximp_Get_resource_name(im, res_name, res_class);
			strcat(res_name, "status.linespacing");
			strcat(res_class, "Status.Linespacing");
			if(XrmGetResource(im->core.rdb, res_name, res_class,
					str_type, &value) == True) { 
				num = atoi(value.addr);
				ic->core.status_attr.line_space = num;
				ic->ximp_icpart->status_attr.LineSpacing = 
					ic->core.status_attr.line_space;
				ic->ximp_icpart->proto_mask |= XIMP_STS_LINESP_MASK;
				*mask                       |= XIMP_STS_LINESP_MASK;
				}
			}
		}
	if(   (ic->ximp_icpart->value_mask & XIMP_RES_NAME)
           || (ic->ximp_icpart->value_mask & XIMP_RES_CLASS) )
		ic->ximp_icpart->value_mask &= ~(XIMP_RES_NAME | XIMP_RES_CLASS);
	return;
	}
