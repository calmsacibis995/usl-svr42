/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Copyright (c) 1991, 1992 UNIX System Laboratories, Inc. */
/*	All Rights Reserved     */

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*	UNIX System Laboratories, Inc.			        */
/*	The copyright notice above does not evidence any        */
/*	actual or intended publication of such source code.     */

#ident	"@(#)wksh:wextra.h	1.2"

#define WK_EXTRA_FUNCS \
	int do_widload(); \
	int do_cmdload(); \
	int do_widlist(); \
	int do_XBell() ; \
	int do_XFlush() ; \
	int do_XtIsSensitive(); \
	int do_XtIsRealized(); \
	int do_XtIsManaged(); \
	int do_XtAppInitialize();\
	int do_XtSetSensitive();\
	int do_XtRemoveAllCallbacks();\
	int do_XtCallCallbacks();\
	int do_XtAppCreateShell(); \
	int do_XtAddInput(); \
	int do_XtCreateManagedWidget();\
	int do_XtCreateWidget();\
	int do_XtDestroyWidget();\
	int do_XDraw(); \
	int do_XtRealizeWidget(); \
	int do_XtUnrealizeWidget(); \
	int do_XtSetValues(); \
	int do_XtGetValues(); \
	int do_XtAddCallback(); \
	int do_XtCreatePopupShell(); \
	int do_XtPopup(); \
	int do_XtPopdown(); \
	int do_XtMapWidget(); \
	int do_XtUnmapWidget(); \
	int do_XtAddTimeOut(); \
	int do_XtManageChildren(); \
	int do_XtUnmanageChildren(); \
	int do_XtMainLoop();

#define WK_EXTRA_MSG 

#define WK_EXTRA_TABLE \
	{ "widload", VALPTR(do_widload), N_BLTIN|BLT_FSUB }, \
	{ "cmdload", VALPTR(do_cmdload), N_BLTIN|BLT_FSUB }, \
	{ "widlist", VALPTR(do_widlist), N_BLTIN|BLT_FSUB }, \
	{ "XBell", VALPTR(do_XBell), N_BLTIN|BLT_FSUB }, \
	{ "XFlush", VALPTR(do_XFlush), N_BLTIN|BLT_FSUB }, \
	{ "XtAppInitialize", VALPTR(do_XtAppInitialize), N_BLTIN|BLT_FSUB }, \
	{ "XtIsSensitive", VALPTR(do_XtIsSensitive), N_BLTIN|BLT_FSUB }, \
	{ "XtIsRealized", VALPTR(do_XtIsRealized), N_BLTIN|BLT_FSUB }, \
	{ "XtIsManaged", VALPTR(do_XtIsManaged), N_BLTIN|BLT_FSUB }, \
	{ "XtCreateManagedWidget", VALPTR(do_XtCreateManagedWidget), N_BLTIN|BLT_FSUB }, \
	{ "XtAppCreateShell", VALPTR(do_XtAppCreateShell), N_BLTIN|BLT_FSUB }, \
	{ "XtCreateWidget", VALPTR(do_XtCreateWidget), N_BLTIN|BLT_FSUB }, \
	{ "XtDestroyWidget", VALPTR(do_XtDestroyWidget), N_BLTIN|BLT_FSUB }, \
	{ "XtSetValues", VALPTR(do_XtSetValues), N_BLTIN|BLT_FSUB }, \
	{ "XtSetSensitive", VALPTR(do_XtSetSensitive), N_BLTIN|BLT_FSUB }, \
	{ "XtAddCallback", VALPTR(do_XtAddCallback), N_BLTIN|BLT_FSUB }, \
	{ "XtRemoveAllCallbacks", VALPTR(do_XtRemoveAllCallbacks), N_BLTIN|BLT_FSUB }, \
	{ "XtCallCallbacks", VALPTR(do_XtCallCallbacks), N_BLTIN|BLT_FSUB }, \
	{ "XtGetValues", VALPTR(do_XtGetValues), N_BLTIN|BLT_FSUB }, \
	{ "XtCreatePopupShell", VALPTR(do_XtCreatePopupShell), N_BLTIN|BLT_FSUB }, \
	{ "XtPopup", VALPTR(do_XtPopup), N_BLTIN|BLT_FSUB }, \
	{ "XtPopdown", VALPTR(do_XtPopdown), N_BLTIN|BLT_FSUB }, \
	{ "XtMapWidget", VALPTR(do_XtMapWidget), N_BLTIN|BLT_FSUB }, \
	{ "XtUnmapWidget", VALPTR(do_XtUnmapWidget), N_BLTIN|BLT_FSUB }, \
	{ "XtManageChildren", VALPTR(do_XtManageChildren), N_BLTIN|BLT_FSUB }, \
	{ "XtUnmanageChildren", VALPTR(do_XtUnmanageChildren), N_BLTIN|BLT_FSUB }, \
	{ "XtAddTimeOut", VALPTR(do_XtAddTimeOut), N_BLTIN|BLT_FSUB }, \
	{ "XtAddInput", VALPTR(do_XtAddInput), N_BLTIN|BLT_FSUB }, \
	{ "XtRealizeWidget", VALPTR(do_XtRealizeWidget), N_BLTIN|BLT_FSUB }, \
	{ "XtUnrealizeWidget", VALPTR(do_XtUnrealizeWidget), N_BLTIN|BLT_FSUB }, \
	{ "XDraw", VALPTR(do_XDraw), N_BLTIN|BLT_FSUB }, \
	{ "XtMainLoop", VALPTR(do_XtMainLoop), N_BLTIN|BLT_FSUB }, \


#define WK_EXTRA_VAR


#define WK_EXTRA_ALIAS \
	"wver",	"echo \"WKSH RELEASE 1.0 b\"",	N_FREE, \
	"bell",	"XBell",			N_FREE, \
	"mc",	"XtManageChildren",		N_FREE, \
	"umc",	"XtUnmanageChildren",		N_FREE, \
	"ato",	"XtAddTimeOut",			N_FREE, \
	"cmw",	"XtCreateManagedWidget",	N_FREE, \
	"cw",	"XtCreateWidget",		N_FREE, \
	"dw",	"XtDestroyWidget",		N_FREE, \
	"mw",	"XtMapWidget",			N_FREE,	\
	"umw",	"XtUnmapWidget",		N_FREE,	\
	"cps",	"XtCreatePopupShell",		N_FREE, \
	"acs",	"XtAppCreateShell",		N_FREE, \
	"pu",	"XtPopup",			N_FREE,	\
	"pd",	"XtPopdown",			N_FREE,	\
	"acb",	"XtAddCallback",		N_FREE,	\
	"racb",	"XtRemoveAllCallbacks",		N_FREE,	\
	"ccb",	"XtCallCallbacks",		N_FREE,	\
	"ainp",	"XtAddInput",			N_FREE,	\
	"gv",	"XtGetValues",			N_FREE,	\
	"sv",	"XtSetValues",			N_FREE,	\
	"ml",	"XtMainLoop",			N_FREE, \
	"rw",	"XtRealizeWidget",		N_FREE, \
	"urw",	"XtUnrealizeWidget",		N_FREE, \
	"ai",	"XtAppInitialize",		N_FREE, \
	"resources",	"widlist -r",		N_FREE, \
	"constraints",	"widlist -R",		N_FREE, \
	"classes",	"widlist -c",		N_FREE, \
	"handles",	"widlist -h",		N_FREE,
