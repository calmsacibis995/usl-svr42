/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)R5Xlib:Xsi/XIMlibint.h	1.2"

#ifndef	_XIMLIBINT_H_
#define	_XIMLIBINT_H_

#include "XIMproto.h"

#define	XIM_INPUTMETHOD		"_XIM_INPUTMETHOD"

#ifndef	ESC
#define	ESC			0x1b
#endif	/* ESC */
#define	ASCII_DESIGNATE		"\033\050\102"

#define hname_size		128
#define offset_of_portnumber	hname_size
#define portnumber_size		2
#define offset_of_version	(offset_of_portnumber + portnumber_size)
#define version_size		4
#define offset_of_minor_version	(offset_of_version + version_size)

#define ipIMofIC(ic) ((XipIM)ic->core.im)
#ifndef	NO_LOCAL_IM
#define ipLocalIMofIC(ic) ((XipLocalIM)ic->core.im)
#endif

extern short	_XipTypeOfNextICQueue();
extern KeySym	_XipKeySymOfNextICQueue();
extern unsigned int	_XipStateOfNextICQueue();
extern char *	_XipStringOfNextICQueue();
extern void	_XipFreeNextICQueue();
extern int	_XipPutICQueue();
extern void	_XipGetNextICQueue();
extern int	_XipWriteToIM();
extern int	_XipReadFromIM();
extern int	_XipFlushToIM();
extern void	_XipSetCurSock();
extern void	_XipSetCurIM();
extern Bool	_XipConnectIM();
extern void	_XipDisconnectIM();
extern int	_XipCallCallbacks();
extern Bool	_XipBackEndFilter();
extern Status	_XipReceiveICValues();
extern int	_XipSendICValues();
extern Bool	_XipCreateDefIC();
extern char *	_XipICSetAttrValues();
extern char *	_XipICSetValues();
extern char *	_XipICGetValues();
#ifndef	NO_LOCAL_IM
extern Bool	_XipBackEndFilter();
#endif
#ifdef	XML
extern void	_XipChangeLocale();
#endif	/* XML */

#endif	/* _XIMLIBINT_H_ */
