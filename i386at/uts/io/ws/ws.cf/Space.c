/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-x86at:io/ws/ws.cf/Space.c	1.4"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/immu.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/inline.h>
#include <sys/pic.h>
#include <sys/cmn_err.h>
#include <sys/kmem.h>
#include <sys/vt.h>
#include <sys/at_ansi.h>
#include <sys/kd.h>
#include <sys/ascii.h>
#include <sys/tss.h>
#include <sys/proc.h>
#include <sys/termio.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/termios.h>
#include <sys/strtty.h>
#include <sys/xque.h>
#include <sys/event.h>
#include <sys/ws/ws.h>
#include <sys/ws/tcl.h>
#include <sys/ws/chan.h>
#include <sys/vid.h>
#include <sys/jioctl.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/session.h>
#include <sys/strsubr.h>
#include <sys/cred.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/genvid.h>
#include <config.h>

unsigned char ws_compatflgs[MAXMINOR/8 + 1];
unsigned char ws_compatflgs[MAXMINOR/8 + 1];
unsigned char ws_svr3_compatflgs[MAXMINOR/8 + 1];
unsigned char ws_svr4_compatflgs[MAXMINOR/8 + 1];

struct kdvdc_proc kdvdc_vt[MAXMINOR + 1]; 

