/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _IO_WS_TCL_H	/* wrapper symbol for kernel use */
#define _IO_WS_TCL_H	/* subject to change without notice */

#ident	"@(#)uts-x86at:io/ws/tcl.h	1.5"
#ident	"$Header: $"


/* Terminal control language defines for use in the
 * Integrated Workstation Environment
 */


#define CH_TCL ( ('T' << 16) | ('C' << 8) | 'L')

#define	TCL_BELL			0
#define	TCL_BACK_SPCE			1
#define	TCL_NEWLINE			2
#define	TCL_CRRGE_RETN			3
#define	TCL_H_TAB			4
#define	TCL_V_TAB			5
#define	TCL_BACK_H_TAB			6

#define	TCL_KEYCLK_ON			10
#define	TCL_KEYCLK_OFF			11
#define	TCL_CLR_TAB			12
#define	TCL_CLR_TABS			13
#define	TCL_SET_TAB			14
#define	TCL_SHFT_FT_OU			15
#define	TCL_SHFT_FT_IN			16
#define	TCL_ADD_STR			17
	
#define	TCL_SCRL_UP			20
#define	TCL_SCRL_DWN			21
#define	TCL_SEND_SCR			22
#define	TCL_LCK_KB			23
#define	TCL_UNLCK_KB			24
#define	TCL_SET_ATTR			25
#define	TCL_POS_CURS			26
#define	TCL_DISP_CLR			27
#define	TCL_DISP_RST			28
#define	TCL_CURS_TYP			29

#define	TCL_ERASCR_CUR2END		30
#define	TCL_ERASCR_BEG2CUR		31
#define	TCL_ERASCR_BEG2END		32
#define	TCL_ERALIN_CUR2END		33
#define	TCL_ERALIN_BEG2CUR		34
#define	TCL_ERALIN_BEG2END		35

#define	TCL_INSRT_LIN			40
#define	TCL_DELET_LIN			41
#define	TCL_INSRT_CHR			42
#define	TCL_DELET_CHR			43

#define	TCL_FLOWCTL	50

#define	TCL_POSABS	0
#define	TCL_POSREL	1

#define	TCL_FLOWOFF	0
#define	TCL_FLOWON	1

/* added to satisfy ANSI and SCO */

#define TCL_SWITCH_VT			60
#define TCL_SAVE_CURSOR			61
#define TCL_RESTORE_CURSOR		62
#define TCL_AUTO_MARGIN_ON		63
#define TCL_AUTO_MARGIN_OFF		64
#define TCL_SET_FONT_PROPERTIES		65
#define TCL_PRINT_FONTCHAR		66
#define TCL_SET_OVERSCAN_COLOR		67
#define TCL_SET_BELL_PARAMS		68
#define TCL_SET_CURSOR_PARAMS		69
#define TCL_NOBACKBRITE			70
#define TCL_BLINK_BOLD			71
#define TCL_FORGRND_COLOR		72
#define TCL_BCKGRND_COLOR		73
#define TCL_RFORGRND_COLOR		74
#define TCL_RBCKGRND_COLOR		75
#define TCL_GFORGRND_COLOR		76
#define TCL_GBCKGRND_COLOR		77

#define AUTO_MARGIN_ON          1
#define AUTO_MARGIN_OFF         0


union tcl_data {
	struct tcl_mv_cur {
		short delta_x;
		short delta_y;
		unchar x_type;
		unchar y_type;
	} mv_curs;

	struct tcl_add_str {
		short len;
		short keynum;
	} add_str;

} tcl_un;

typedef union tcl_data tcl_data_t;

#endif /* _IO_WS_TCL_H */
