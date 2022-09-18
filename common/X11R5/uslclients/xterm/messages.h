/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:messages.h	1.10"
#endif

#define OleMperror_reason 	"      Reason"
#define OleMioctl_winSz 	"TIOCSWINSZ failed in TekSetGCFont"

#define OleMuname_badUname 	"uname() failed"
#define OleMtitle_untitled 	"Untitled"
#define OleMname_notLocal 	 "TIOCVTNAME Failed: Not local...."
#define OleMspace_buffer 	"Out of buffer space"
#define OleMspace_truncate 	"Truncating to %d"
#define OleMread_badConsole 	"read from console log driver failed"

#define OleMtty_badTty 		"%s:  bad tty modes \"%s\"\n"

#define OleMtitle_xterm 	"xterm"
#define OleMtitle_window 	"login(" /* extra right parenthesis needed */
#define OleMdup_badDup2 	"Failed to dup2 (%d %d)\r\n"

/* Usage strings */
#define OleMusage_msg1 		"Usage: xterm [-b inner_border_width] [-bd border_color] \\\n"
#define OleMusage_msg1a 	"Usage: xterm [-132] [-b inner_border_width] [-bd border_color] \\\n"
#define OleMusage_msg2 	" [-bg backgrnd_color] [-bw border_width] [-C] [-cr cursor_color] \\\n"
#define OleMusage_msg2a 	" [-bg backgrnd_color] [-bw border_width] [-cr cursor_color] \\\n"
#define OleMusage_msg3	" [-C] [-display display] [-fb bold_font] [-fg foregrnd_color] [-fn norm_font] \\\n"
#define OleMusage_msg3a	" [-display display] [-fb bold_font] [-fg foregrnd_color] [-fn norm_font] \\\n"
#define OleMusage_msg4 	" [-i] [-j] [+j] [-l] [+l] [-lf logfile] [-ls] [-mb] [+mb] [-ml] [+ml]\\\n"
#define OleMusage_msg5 	" [-ms mouse_color] [-n icon name] [-name application name] \\\n"
#define OleMusage_msg6 	" [-nb bell_margin] [-rv|-r] [-rs] [+rs] [-rw] [+rw] \\\n"

#define OleMusage_msg7 	" [-sb] [+sb] [-si] [-sk] [-sl save_lines] [-t] [-T title] \\\n"
#define OleMusage_msg7a 	" [-sb] [+sb] [-si] [-sk] [-sl save_lines] [-T|t title] \\\n"
#define OleMusage_msg7b 	"[-sb] [+sb] [-sl save_lines] [-t] [-T title] \\\n"
#define OleMusage_msg7c 	" [-sb] [+sb] [-sl save_lines] [-T|t title] \\\n"
#define OleMusage_msg7d 	 " [-s] [-sb] [-si] [-sk] [-sl save_lines] [-sn] [-st] \\\n"
#define OleMusage_msg7e 	 " [-T title] [-t title]  [-tb] \\\n"
#define OleMusage_msg7f 	" [-s] [-sb] [-si] [-sk] [-sl save_lines] [-sn] [-st] [-T|t title] [-tb] \\\n"

#define OleMusage_msg8 	 " [-vb] [+vb] [-geometry [columns][xlines][[+-]xoff[[+-]yoff]]] \\\n"
#define OleMusage_msg8a 	 " [-vb] [+vb] [=[width]x[height][[+-]xoff[[+-]yoff]]] \\\n"
#define OleMusage_msg9 	" [%[width]x[height][[+-]xoff[[+-]yoff]]] [#[+-]xoff[[+-]yoff]] \\\n"
#define OleMusage_msg10 	" [-w border_width] [-e|E command_to_exec] [-xrm resource string]\n\n"
#define OleMusage_msg11 	"Fonts must be of fixed width and of same size;\n"
#define OleMusage_msg12 	"If only one font is specified, it will be used for normal and bold text\n"
#define OleMusage_msg13 	"The -132 option allows 80 <-> 132 column escape sequences\n"
#define OleMusage_msg14 	"The -C option forces output to /dev/console to appear in this window\n"

#define OleMusage_msg15 	"The -i  option enables iconic startup\n"
#define OleMusage_msg16 	"The -j  option enables jump scroll\n"
#define OleMusage_msg17 	"The -l  option enables logging\n"

#define OleMusage_msg18 	"The -ls option makes the shell a login shell\n"

#define OleMusage_msg19 	"The -mb option turns the margin bell on\n"
#define OleMusage_msg19a 	"The -ml option turns mouseless mode on\n"
#define OleMusage_msg20 	"The +rs option disables window resizing in curses mode\n"
#define OleMusage_msg21 	"The -rs option allows window resizing in curses mode\n"
#define OleMusage_msg22 	"The -rv option turns reverse video on\n"
#define OleMusage_msg23 	"The -rw option turns reverse wraparound on\n"

#define OleMusage_msg24 	"The -s  option enables asynchronous scrolling\n"
#define OleMusage_msg25 	"The -sb option enables the scrollbar\n"
#define OleMusage_msg26 	"The -si option disables re-positioning the scrollbar at the bottom on input\n"
#define OleMusage_msg27 	"The -sk option causes the scrollbar to position at the bottom on a key\n"
#ifdef TEK
#define OleMusage_msg28 	"The -t  option starts Tektronix mode\n"
#endif
#define OleMusage_msg29 	"The -vb option enables visual bell\n"

#define OleMpty_noAvail 	"%s: Not enough available pty's\n"
#define OleMsignal_sighup 	"sighup\n"
#define OleMpty_open 	"Open of pseudo-tty failed\n"
#define OleMdup2_badDup2Msg2 	"dup2(screen->respond, Xsocket + 1) failed\n"

#define OleMexec_badExecvp 	"%s: Can't execvp %s\n"
#define OleMexec_badExeclp 	"%s: Could not exec %s!\n"

#define OleMstrindex_co 	"%s: Can't find co# in termcap string %s\n"
#define OleMstrindex_li 	"%s: Can't find li# in termcap string %s\n"

/* the "%s" is needed for an argument - this is for OlGetMessage() */
#define OleMopen_badConsole 	"open(%s) failed\n"

#define OleMlabel_hideTek 	"Hide Tek window"
#define OleMlabel_showTek 	"Show Tek window"

#define OleMtitle_secureKbd 	"%s: Secure Keyboard"

/* These are from the flat checkbox */
#define OleMnames_visualBell 	"Visual Bell"
#define OleMnames_logging  	"Logging"
#define OleMnames_jumpScroll 	"Jump Scroll"
#define OleMnames_reverseVideo 	"Reverse Video"
#define OleMnames_autoWrap  	"Auto Wraparound"
#define OleMnames_reverseWrap  	"Reverse Wraparound"
#define OleMnames_autoLf  	"Auto Linefeed"
#define OleMnames_appCursor  	"Application Cursor"
#define OleMnames_appPad  	"Application Pad"
#define OleMnames_scrollbar  	"Scroll Bar"
#define OleMnames_marginBell  	"Margin Bell"
#define OleMnames_secureKbd	"Secure Keyboard"
#define OleMnames_cursesResize  "Curses Resize"
#ifdef XTERM_COMPAT
#define OleMnames_autoRepeat   	"Auto Repeat"
#define OleMnames_scrollonKey  	"Scroll on key"
#define OleMnames_scrollonInput "Scroll on input"
#endif


#define OleMlabel_hideVt 	"Hide VT window"
#define OleMlabel_showVt 	"Show VT window"

#define OleMexcl_largeChar 	"Large Characters"
#define OleMexcl_mediumChar 	"Medium Characters"
#define OleMexcl_smallChar 	"Small Characters"
#define OleMexcl_tinyChar 	"Tiny Characters"

/* misc.c */
#define OleMexec_OleTbadExecl 	"%s: Can't exec '%s'\n"
#define OleMaccess_loginFile 	"\nCannot access login file %s, ERRNO = %d\n"
#define OleMopen_loginFile 	"\nCannot open login file %s\n"

#define OleMprintf_errmsg1 	"%s: Error %d, errno %d:\n"
#define OleMprintf_errmsg2 	"%s: Error %d\n"
#define OleMprintf_errmsg3	"Request code %d, minor code %d, serial #%ld, resource id %ld\n"

#define OleMopen_openpty1 	"spipe: open 0 failed, errno=%d\n"
#define OleMopen_openpty2 	"spipe: open 1 failed, errno=%d\n"
#define OleMioctl_ptem 	"ptem failed"
#define OleMioctl_consem 	"consem failed"
#define OleMioctl_ldterm 	"ldterm failed"
#define OleMioctl_ttcompat 	"ttcompat failed"

#define OleMioctl_tcseta 	"ioctl TCSETA failed in openpty()"

#define OleMioctl_winSz2 	"TIOCSWINSZ failed in main.c\n"

#define OleMopen_badConsole2 	"could not open console"

#define OleMioctl_badConsole 	"Cannot register to receive console log messages"

#define OleMexec_badExec1 	"%s: Can't exec `%s'\n"
#define OleMtitle_tek 	"(Tek)"

#define OleMfont_badFont 	"%s: Could not get font %s; using server default\n"
#define OleMspace_tekMode 	"%s: Not enough core for Tek mode\n"
#define OleMcreate_badWindow1 	"%s: Can't create Tek window\n"

#define OleMputenv_badPutenv 	"\nputenv failed\n"

/* from VTinit.c */
#define OleMolopen_badOlopenIM 	"xterm failed to open input method %s\n"
#define OleMolcreateIc_badOlcreateIc 	"xterm: Failed to create IC for input method: %s\n"
#define OleMolsetIcValues_badOlsetIcValues 	"Could not set Ic values\n"
#define OleMolsetIcValues_badOlsetIcValuesStatus 	"OlSetIcValues failed to reset status area geometry\n"

#ifndef DTM_HELP
#define OleMhelp_helpString 	"Press or click MENU on the Xterm pane to bring up the Xterm menu.  Select the Properties button in the Xterm menu to bring up the Xterm property window."
#endif

#define OleMolGetnextstrsegment_badOlgetnextstrsegment 	"OlGetNextStrSegment() failed\n"
#define OleMpixel_badPixel 	"No index for Pixel 0x%x\n"

#define OleMpanic1	"VTparse:mmenu_resize: XSetNormalHints failed to restore to out-of-curses state\n"
#define OleMpanic2	"VTparse: XGetNormalHints failed to obtain orig_SizeHints\n"
#define OleMpanic3	"VTparse: XGetWindowAttributes failed to obtain orig_win_attrs\n"
#define OleMpanic4	"VTparse: XSetNormalHints failed to change to in-curses state\n"
#define OleMpanic5	"VTparse: XSetNormalHints failed to restore to out-of-curses state\n"
#define OleMpanic6	"VTparse: XGetNormalHints failed to obtain orig_SizeHints\n"
#define OleMpanic7	"input: read returned unexpected error (%d)\n"
#define OleMpanic8	"input: read returned zero\n"
#define OleMpanic9	"unparseputc: error writing character\n"
#define OleMpanic10	"Error: Input() Write failed!\n"
#define OleMpanic11	"Tinput:read returned unexpected error (%d)\n"
#define OleMpanic12	"Tinput: read returned zero\n"
#define OleMpanic13	"Tinput: malloc error (%d)\n"

/***********************************************************/
/* Button labels */


/* Edit menu */
#define OleMlabel_send	"Send"
#define OleMlabel_paste	"Paste"
#define OleMlabel_copy	"Copy"
#define OleMlabel_cut	"Cut"

#define OleMmnemonic_send	"S"
#define OleMmnemonic_paste	"P"
#define OleMmnemonic_copy	"C"
#define OleMmnemonic_cut	"X"

/* Xterm Menu labels and mnemonics */

#define OleMlabel_edit		"Edit"
#define OleMlabel_redraw	"Redraw"
#define OleMlabel_softReset	"Soft Reset"
#define OleMlabel_fullReset	"Full Reset"
#define OleMlabel_properties	"Properties..."
/* #define OleMlabel_showTek	"Show Tek Window" */
#define OleMlabel_interrupt	"Interrupt"
#define OleMlabel_hangup	"Hangup"
#define OleMlabel_terminate	"Terminate"
#define OleMlabel_kill		"Kill"

#define OleMlabel_page		"PAGE"
#define OleMlabel_reset		"RESET"
#define OleMlabel_copy2		"COPY"

#define OleMmnemonic_edit	"E"
#define OleMmnemonic_redraw	"R"
#define OleMmnemonic_softReset	"S"
#define OleMmnemonic_fullReset	"F"
#define OleMmnemonic_properties	"P"
#define OleMmnemonic_showTek	"o"
#define OleMmnemonic_interrupt	"I"
#define OleMmnemonic_hangup	"H"
#define OleMmnemonic_terminate	"T"
#define OleMmnemonic_kill	"K"

#define OleMmnemonic_page	"A"
#define OleMmnemonic_reset	"E"
#define OleMmnemonic_copy2	"C"
