/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:Strings.h	1.16"
#endif

/* Strings (names and types) */

#ifndef _XTERM_STRINGS_H_
#define _XTERM_STRINGS_H_

#define OleCOlClientXtermMsgs "xterm_msgs"

/* Tekproc.c */
#define OleNperror "perror"
#define OleTreason "reason"

#define OleNioctl "ioctl"
#define OleTwinSz "winsize"
#define OleNfont "font"
#define OleTbadFont "badfont"
#define OleNcreate "create"
#define OleTbadWindow1 "badwindow1"

#define OleTtekMode "tekmode"

/* charproc.c */
#define OleNuname "uname"
#define OleTbadUname "baduname"

#define OleNtitle "title"
#define OleTuntitled "untitled"

#define OleNname "name"
#define OleTnotLocal "notlocal"

#define OleNspace "space"
#define OleTbuffer "buffer"

#define OleTtruncate "truncate"

#define OleNread "read"
#define OleTbadConsole "badconsole"

/* main.c */
#define OleTxterm "xterm"
#define OleTwindow "window"

#define OleNtty "tty"
#define OleTbadTty "badtty"

#define OleNdup "dup"
#define OleNdup2 "dup2"
#define OleTbadDup2 "baddup2"

#define OleNusage "usage"
#define OleTmsg1a "msg1a"
#define OleTmsg2a "msg2a"
#define OleTmsg3a "msg3a"
#define OleTmsg7a "msg7a"
#define OleTmsg7b "msg7b"
#define OleTmsg7c "msg7c"
#define OleTmsg7d "msg7d"
#define OleTmsg7e "msg7e"
#define OleTmsg7f "msg7f"
#define OleTmsg8a "msg8a"
#define OleTmsg15 "msg15"
#define OleTmsg16 "msg16"
#define OleTmsg17 "msg17"
#define OleTmsg18 "msg18"
#define OleTmsg19 "msg19"
#define OleTmsg19a "msg19a"
#define OleTmsg20 "msg20"
#define OleTmsg21 "msg21"
#define OleTmsg22 "msg22"
#define OleTmsg23 "msg23"
#define OleTmsg24 "msg24"
#define OleTmsg25 "msg25"
#define OleTmsg26 "msg26"
#define OleTmsg27 "msg27"
#define OleTmsg28 "msg28"
#define OleTmsg29 "msg29"

#define OleNpty "pty"
#define OleTnoAvail "noavail"

#define OleNsignal "signal"
#define OleTsighup "sighup"

#define OleTopen "open"
#define OleTbadDup2Msg2 "baddup2msg2"

#define OleNexec "exec"
#define OleTbadExecvp "badexecvp"

#define OleTbadExeclp "badexeclp"

#define OleNstrindex "strindx"
#define OleTco "co"
#define OleTli "li"

#define OleNopen "open"
#define OleNhelp "help"
#ifndef DTM_HELP
#define OleThelpString "helpstring"
#endif /* DTM_HELP */
/*
 *****************************************
 * Menu.c ...
 */
#define OleNlabel	"label"
#define OleNmnemonic	"mnem"

/* menu labels and mnemonics */
/* #define OleTedit	"ed" */
#define OleTredraw	"rd"
#define OleTsoftReset	"sR"
#define OleTfullReset	"fR"
#define OleTproperties	"prop"
#define OleTshowTekWin	"stw"
#define OleTinterrupt	"intt"
#define OleThangup	"hang"
#define OleTterminate	"term"
#define OleTkill	"kil"

#define OleTpage	"pg"
#define OleTreset	"rst"
#define OleTcopy2	"cp2"


#define OleTsend	"snd"
/* #define OleTpaste	"pst" */
/* #define OleTcopy	"cp" */
/* #define OleTcut		"ct" */

#define OleThideTek	"hidetek"
#define OleTshowTek	"showtek"
#define OleThideVt	"hidevt"
#define OleTshowVt	"showvt"

#define OleNcheckbox	"checkbox"
#define OleTvisualBell	"visualbell_lab"
#define OleTlogging	"logging_lab"
#define OleTjumpScroll	"jumpscroll_lab"
#define OleTreverseVideo "reversevideo_lab"
#define OleTautoWrap	"autowraparound_lab"
#define OleTreverseWrap	"reversewarparound_lab"
#define OleTautoLf	"autolinefeed_lab"
#define OleTappCursor	"applicationcursor_lab"
#define OleTappPad	"applicationpad_lab"
#define OleTscrollbar	"scrollbar_lab"
#define OleTmarginBell	"marginbell_lab"
#define OleTsecureKbd	"securekeyboard_lab"
#define OleTcursesResize "cursesresize_lab"
#ifdef XTERM_COMPAT
#define OleTautoRepeat 	"autorepeat_lab"
#define OleTscrollonKey	"scrollonkey_lab"
#define OleTscrollonInput	"scrolloninput_lab"
#endif

#define OleNexcl	"excl"
#define OleTlargeChar	"large"
#define OleTmediumChar	"medium"
#define OleTsmallChar	"small"
#define OleTtinyChar	"tiny"

/* misc.c */
#define OleTtek "(Tek)"
#define OleTbadExecl "badexecl"
#define OleNaccess "access"
#define OleTloginFile "loginfile"
#define OleNprintf "printf"
#define OleTerrmsg1 "errmsg1"
#define OleTerrmsg2 "errmsg2"
#define OleTerrmsg3 "errmsg3"

/* openpty.c */
#define OleTopenpty1 "openpty1"
#define OleTopenpty2 "openpty2"
#define OleTptem "ptem"
#define OleTconsem "consem"
#define OleTldterm "ldterm"
#define OleTttcompat "ttcompat"

#define OleTtcseta "tcseta"

#define OleTbadConsole2 "badconsole2"
#define OleTwinSz2 "winsz2"

#define OleNputenv "putenv"
#define OleTbadPutenv "badputenv"

/* resize.c */
#define OleNtimeout "timeout"
#define OleTnoTime "notime"

#define OleNsetsize "setsize"
#define OleTbadSetsize "badsetsize"

#define OleNfopen "fopen"
#define OleTbadFopentty "badfopentty"

#define OleNtgetent "tgetent"
#define OleTbadTgetent "badtgetent"

#define OleNreadstring "readstring"
#define OleTnoRowscols "norowscols"
#define OleTwinSize "winsz"

#define OleTco2 "co2"
#define OleTli2 "li2"

#define OleNgetc "getc"
#define OleTunknownChar "unknownchar"

#define OleTmsg30 "msg30"
#define OleTmsg31 "msg31"

/* VTinit.c */
#define OleNolopenIM "olopenim"
#define OleTbadOlopenIM "badolopenim"

#define OleNolcreateIc "olcreateic"
#define	OleTbadOlcreateIc "badolcreateic"

#define OleNolsetIcValues "olseticvalues"
#define OleTbadOlsetIcValues "badolseticvalues"

#define OleNolsetIcValues "olseticvalues"
#define OleTbadOlsetIcValuesStatus "badolseticvaluesstatus"

/* from screen.c */
#define OleNpixel "pixel"
#define OleTbadPixel "badpixel"

#define OleNolGetnextstrsegment "olgetnextstrsegment"
#define OleTbadOlgetnextstrsegment "badolgetnextstrsegment"

#define OleNpanic	"panic"
#define OleTpanic_msg1	"msg1"
#define OleTpanic_msg2	"msg2"
#define OleTpanic_msg3	"msg3"
#define OleTpanic_msg4	"msg4"
#define OleTpanic_msg5	"msg5"
/* #define OleTpanic_msg6	"msg6" 	/* no longer used: free	*/
#define OleTpanic_msg7	"msg7"
#define OleTpanic_msg8	"msg8"
#define OleTpanic_msg9	"msg9"
#define OleTpanic_msg10	"msg10"
#define OleTpanic_msg11	"msg11"
#define OleTpanic_msg12	"msg12"
#define OleTpanic_msg13	"msg13"
#endif
