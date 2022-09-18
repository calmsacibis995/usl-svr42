/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/io/keynames.h	1.1"

#define TABLE_OFFSET	8	/* Have to shift the table up to avoid lower */
				/* values; see protocol spec, KEYBOARDS sect */

#define MIN_I386_KEY           (  1+TABLE_OFFSET)
#define MAX_I386_KEY           (127+TABLE_OFFSET)
#define I386_GLYPHS_PER_KEY     4

/* These are the values returned from the keyboard driver: */

#define K_Escape	 1
#define K_1		 2
#define K_2		 3
#define K_3		 4
#define K_4		 5
#define K_5		 6
#define K_6		 7
#define K_7		 8
#define K_8		 9
#define K_9		10
#define K_0		11
#define K_minus		12
#define K_equal		13
#define K_BackSpace	14
#define K_Tab		15
#define K_q		16
#define K_w		17
#define K_e		18
#define K_r		19
#define K_t		20
#define K_y		21
#define K_u		22
#define K_i		23
#define K_o		24
#define K_p		25
#define K_bracketleft	26
#define K_bracketright	27
#define K_Enter		28
#define K_Control_L	29
#define K_a		30
#define K_s		31
#define K_d		32
#define K_f		33
#define K_g		34
#define K_h		35
#define K_j		36
#define K_k		37
#define K_l		38
#define K_semicolon	39
#define K_quoteright	40
#define K_quoteleft	41
#define K_Shift_L	42
#define K_backslash	43
#define K_z		44
#define K_x		45
#define K_c		46
#define K_v		47
#define K_b		48
#define K_n		49
#define K_m		50
#define K_comma		51
#define K_period	52
#define K_slash		53
#define K_Shift_R	54
#define K_KP_Multiply	55
#define K_Alt_L		56
#define K_space		57
#define K_Caps_Lock	58
#define K_F1		59
#define K_F2		60
#define K_F3		61
#define K_F4		62
#define K_F5		63
#define K_F6		64
#define K_F7		65
#define K_F8		66
#define K_F9		67
#define K_F10		68
#define K_Num_Lock	69
#define K_Scroll_Lock	70
#define K_Home		71
#define K_Up		72
#define K_Prior		73
#define K_KP_Subtract	74
#define K_Left		75
#define K_Begin		76
#define K_Right		77
#define K_KP_Add	78
#define K_End		79
#define K_Down		80
#define K_Next		81
#define K_Insert	82
#define K_Delete	83
#define K_Ex_Print	84	/* Print Screen */
#define K_ExDown	85
#define K_ExLeft	86
#define K_F11		87
#define K_F12		88
#define K_Ex_Prior     111	/* Page Up */
#define K_Alt_R	       114
#define K_Control_R    115
#define K_KP_Enter     116
#define K_KP_Divide    117

/* The following keycode is used to capture the pause key sequence: */
/* Make - E1 1D 45   Break - E1 9D C5				    */
/* Make - E0 46      Break - E0 C6     with CNTRL		    */
/* KD driver maps these two keys to the same value */

#define K_Pause	       119
#define K_ExUp	       120
#define K_Ex_Delete    121
#define K_Ex_End       122
#define K_Ex_Insert    123
#define K_ExRight      125
#define K_Ex_Next      126	/* Page Down */
#define K_Ex_Home      127
