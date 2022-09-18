/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:error.h	1.2.1.11"
#endif
/*
 error.h (C hdr file)
	Acc: 601052260 Tue Jan 17 09:57:40 1989
	Mod: 601054034 Tue Jan 17 10:27:14 1989
	Sta: 601054034 Tue Jan 17 10:27:14 1989
	Owner: 7007
	Group: 1985
	Permissions: 666
*/
/*
	START USER STAMP AREA
*/
/*
	END USER STAMP AREA
*/

/* Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts. */

#include <X11/copyright.h>

#define	ERROR_KMALLOC	10	/* main: malloc() failed for keyboardtype */
#define	ERROR_FIONBIO	11	/* main: ioctl() failed on FIONBIO */
#define	ERROR_TSLOT	12	/* spawn: tslot() failed and getty */
#define	ERROR_TSLOT2	13	/* spawn: tslot() failed and am_slave */
#define	ERROR_OPDEVTTY	14	/* spawn: open() failed on /dev/tty */
#define	ERROR_TIOCGETP	15	/* spawn: ioctl() failed on TIOCGETP */
#define	ERROR_TIOCGETC	16	/* spawn: ioctl() failed on TIOCGETC */
#define	ERROR_TIOCGETD	17	/* spawn: ioctl() failed on TIOCGETD */
#define	ERROR_TIOCGLTC	18	/* spawn: ioctl() failed on TIOCGLTC */
#define	ERROR_TIOCLGET	19	/* spawn: ioctl() failed on TIOCLGET */
#define	ERROR_TIOCCONS	20	/* spawn: ioctl() failed on TIOCCONS */
#define	ERROR_OPDEVTTY2	21	/* spawn: second open() failed on /dev/tty */
#define	ERROR_NOTTY	22	/* spawn: ioctl() failed on TIOCNOTTY */
#define	ERROR_TIOCSETP	23	/* spawn: ioctl() failed on TIOCSETP */
#define	ERROR_TIOCSETC	24	/* spawn: ioctl() failed on TIOCSETC */
#define	ERROR_TIOCSETD	25	/* spawn: ioctl() failed on TIOCSETD */
#define	ERROR_TIOCSLTC	26	/* spawn: ioctl() failed on TIOCSLTC */
#define	ERROR_TIOCLSET	27	/* spawn: ioctl() failed on TIOCLSET */
#define	ERROR_TSLOT3	28	/* spawn: tslot() failed  */
#define	ERROR_FORK	29	/* spawn: fork() failed */
#define	ERROR_EXEC	30	/* spawn: exec() failed */
#define	ERROR_OPDEVTTY3	31	/* spawn: third open() failed on /dev/tty */
#define	ERROR_PTYS	32	/* get_pty: not enough ptys */
#define	ERROR_NOX	33	/* get_terminal: can't connect to server */
#define	ERROR_NOX2	34	/* get_terminal: can't connect and getty */
#define	ERROR_INIT	36	/* spawn: can't initialize window */
#define	ERROR_NOCO	37	/* resize: no `co' in termcap */
#define	ERROR_NOLI	38	/* resize: no `li' in termcap */
#define	ERROR_BORDER	39	/* get_terminal: can't make border tile */
#define	ERROR_BACK	40	/* get_terminal: can't make background tile */
#define ERROR_NOX3      43      /* get_terminal: bad pty from display server */
/* charproc.c */
#define	ERROR_SELECT	50	/* in_put: select() failed */
#define	ERROR_SMALLOC	53	/* VTInit: malloc failed for segment */
#define	ERROR_VINIT	54	/* VTInit: can't initialize window */
/* Tekproc.c */
#define	ERROR_TSELECT	60	/* Tinput: select() failed */
#define	ERROR_TINIT	64	/* TekInit: can't initialize window */
#define	ERROR_TBACK	65	/* TekBackground: can't make background */
/* button.c */
#define	ERROR_BMALLOC2	71	/* SaltTextAway: malloc() failed */
#define ERROR_BADMENU   72      /* ModeMenu: don't know what menu to use */
/* misc.c */
#define	ERROR_LOGEXEC	80	/* StartLog: exec() failed */
#define	ERROR_XERROR	83	/* xerror: XError event */
#define	ERROR_XIOERROR	84	/* xioerror: X I/O error */
#define ERROR_WINNAME   85      /* get_terminal: malloc failed */
/* screen.c */
#define	ERROR_SCALLOC	90	/* Alloc: calloc() failed on base */
#define	ERROR_SCALLOC2	91	/* Alloc: calloc() failed on rows */
#define	ERROR_SREALLOC	92	/* ScreenResize: realloc() failed on alt base */
#define	ERROR_RESIZE	96	/* ScreenResize: malloc() or realloc() failed */
/* scrollbar.c */
#define	ERROR_SBRALLOC	110	/* ScrollBarOn: realloc() failed on base */
#define	ERROR_SBRALLOC2	111	/* ScrollBarOn: realloc() failed on rows */
/* util.c */
#define	ERROR_UBACK	120	/* ReverseVideo: can't make background */
/* main.c */
#define	ERROR_TCSETA	130	/* TCSETA failed - ehr3 */
#define	ERROR_TCGETA	131	/* TCGETA failed - ehr3 */
/* SS-ioctl */
#define ERROR_IOALLOC	140	/* calloc failed for KBTrans_table */
#define ERROR_IOALLOC1	141	/* calloc failed for FKTrans_table */
#define ERROR_IOOPEN	142	/* couldn't open standart map file */
#define ERROR_IOREAD	143	/* couldn't read standart map file */
#define ERROR_IOREAD1	144	/* couldn't read from consem */
#define ERROR_IOWRITE	145	/* couldn't write to consem */
