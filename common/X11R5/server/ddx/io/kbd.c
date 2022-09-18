/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)siserver:ddx/io/kbd.c	1.1"

/************************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved
********************************************************/
/* $Header: lk201.c,v 1.29 88/02/09 13:53:44 rws Exp $ */

#include "X.h"
#define NEED_EVENTS
#include "Xproto.h"
#include "keynames.h"
#include "keysym.h"

/* This file is device dependent, but is common to several devices */

#include <sys/types.h>
#include "inputstr.h"

/* Handle keyboard input from I386 */

/* Whatever key is associated with the Lock modifier is treated as a toggle key.
 * The code assumes that such keys are always in up/down mode.
 */

void
ProcessI386Input (e, dev)
	register xEvent *e;
	DevicePtr dev;
{
#ifdef INGNORE_UNMAPPED_KEYS
	/* The following codes are not mapped */
	if (
	    (e->u.u.detail >= 84 &&
		e->u.u.detail <= 86) ||		/* Alt-SysRq, ??, ?? */
	    e->u.u.detail == 70			/* ScrollLock */
	   ) return;
#endif

	/*
	 * Have to offset our table to avoid KEYCODE values lower than 8;
	 * see the protocol specification, SECTION 6: KEYBOARDS (p. 16)
	 */
	e->u.u.detail += TABLE_OFFSET;

	/* DIX will take care of checking against the minimum & the maximum */

	(*dev->processInputProc)(e, dev, 1);	/* rjk */
}

Bool
LegalModifier(key)
    register BYTE key;
{
    key -= TABLE_OFFSET;	/* shift local copy of keycode */
    if ((key == K_Caps_Lock)
     || (key == K_Num_Lock)
     || (key == K_Shift_L)
     || (key == K_Shift_R)
     || (key == K_Alt_L)
     || (key == K_Alt_R)
     || (key == K_Control_L)
     || (key == K_Control_R))
        return TRUE;
    return FALSE;
}

void
GetI386Mappings(pKeySyms, pModMap)
    KeySymsPtr pKeySyms;
    CARD8 *pModMap;
{
#define INDEX(in) ((in + TABLE_OFFSET - MIN_I386_KEY) * I386_GLYPHS_PER_KEY)
    int i;
    KeySym *map;

    for (i = 0; i < MAP_LENGTH; i++)
	pModMap[i] = NoSymbol;	/* make sure it is restored */
    pModMap[ K_Caps_Lock+TABLE_OFFSET ]	= LockMask;
    pModMap[ K_Shift_L+TABLE_OFFSET ]	= ShiftMask;
    pModMap[ K_Shift_R+TABLE_OFFSET ]	= ShiftMask;
    pModMap[ K_Control_L+TABLE_OFFSET ]	= ControlMask;
    pModMap[ K_Control_R+TABLE_OFFSET ]	= ControlMask;
    pModMap[ K_Alt_L+TABLE_OFFSET ]	= Mod1Mask;
    pModMap[ K_Alt_R+TABLE_OFFSET ]	= Mod1Mask;
    pModMap[ K_Num_Lock+TABLE_OFFSET ]	= Mod2Mask;

    map = (KeySym *)Xalloc(sizeof(KeySym) * 
				    (MAP_LENGTH * I386_GLYPHS_PER_KEY));
    pKeySyms->minKeyCode = MIN_I386_KEY;
    pKeySyms->maxKeyCode = MAX_I386_KEY;
    pKeySyms->mapWidth = I386_GLYPHS_PER_KEY;
    pKeySyms->map = map;

    for (i = 0; i < (MAP_LENGTH * I386_GLYPHS_PER_KEY); i++)
	map[i] = NoSymbol;	/* make sure it is restored */

	map[INDEX(K_BackSpace)]	= XK_BackSpace;
	map[INDEX(K_Tab)]	= XK_Tab;
	map[INDEX(K_Escape)] 	= XK_Escape;
	map[INDEX(K_Delete)] 	= XK_Delete;
	map[INDEX(K_Delete)+1] 	= XK_KP_Decimal;
	map[INDEX(K_Delete)+2] 	= XK_KP_Decimal;
	map[INDEX(K_Delete)+3] 	= XK_Delete;
	map[INDEX(K_Ex_Delete)]	= XK_Delete;
	map[INDEX(K_Pause)]	= XK_Pause;
	map[INDEX(K_Prior)] 	= XK_Prior; 
	map[INDEX(K_Prior)+1] 	= XK_KP_9;
	map[INDEX(K_Prior)+2] 	= XK_KP_9; 
	map[INDEX(K_Prior)+3] 	= XK_Prior;
	map[INDEX(K_Ex_Prior)] 	= XK_Prior;
	map[INDEX(K_Up)] 	= XK_Up;
	map[INDEX(K_Up)+1] 	= XK_KP_8;
	map[INDEX(K_Up)+2] 	= XK_KP_8;
	map[INDEX(K_Up)+3] 	= XK_Up;
	map[INDEX(K_ExUp)] 	= XK_Up;
	map[INDEX(K_Home)] 	= XK_Home;
	map[INDEX(K_Home)+1] 	= XK_KP_7;
	map[INDEX(K_Home)+2] 	= XK_KP_7;
	map[INDEX(K_Home)+3] 	= XK_Home;
	map[INDEX(K_Ex_Home)] 	= XK_Home;
	map[INDEX(K_Right)] 	= XK_Right;
	map[INDEX(K_Right)+1] 	= XK_KP_6;
	map[INDEX(K_Right)+2] 	= XK_KP_6;
	map[INDEX(K_Right)+3] 	= XK_Right;
	map[INDEX(K_ExRight)] 	= XK_Right;
	map[INDEX(K_Begin)] 	= XK_Begin;
	map[INDEX(K_Begin)+1] 	= XK_KP_5;
	map[INDEX(K_Begin)+2] 	= XK_KP_5;
	map[INDEX(K_Begin)+3] 	= XK_Begin;
	map[INDEX(K_Left)] 	= XK_Left;
	map[INDEX(K_Left)+1] 	= XK_KP_4;
	map[INDEX(K_Left)+2] 	= XK_KP_4;
	map[INDEX(K_Left)+3] 	= XK_Left;
	map[INDEX(K_ExLeft)] 	= XK_Left;
	map[INDEX(K_Next)] 	= XK_Next;
	map[INDEX(K_Next)+1] 	= XK_KP_3;
	map[INDEX(K_Next)+2] 	= XK_KP_3;
	map[INDEX(K_Next)+3] 	= XK_Next;
	map[INDEX(K_Ex_Next)] 	= XK_Next;
	map[INDEX(K_Down)] 	= XK_Down;
	map[INDEX(K_Down)+1] 	= XK_KP_2;
	map[INDEX(K_Down)+2] 	= XK_KP_2;
	map[INDEX(K_Down)+3] 	= XK_Down;
	map[INDEX(K_ExDown)] 	= XK_Down;
	map[INDEX(K_End)] 	= XK_End;
	map[INDEX(K_End)+1] 	= XK_KP_1;
	map[INDEX(K_End)+2] 	= XK_KP_1;
	map[INDEX(K_End)+3] 	= XK_End;
	map[INDEX(K_Ex_End)] 	= XK_End;
	map[INDEX(K_Insert)] 	= XK_Insert;
	map[INDEX(K_Insert)+1] 	= XK_KP_0;
	map[INDEX(K_Insert)+2] 	= XK_KP_0;
	map[INDEX(K_Insert)+3] 	= XK_Insert;
	map[INDEX(K_Ex_Insert)]	= XK_Insert;
	map[INDEX(K_KP_Enter)] 	= XK_Return;
	map[INDEX(K_Enter)] 	= XK_Return;
	map[INDEX(K_KP_Multiply)] = XK_KP_Multiply;
	map[INDEX(K_Ex_Print)]	= XK_Print;
	map[INDEX(K_KP_Add)] 	= XK_KP_Add;
	map[INDEX(K_KP_Subtract)] = XK_KP_Subtract;
	map[INDEX(K_KP_Divide)] = XK_KP_Divide;
	map[INDEX(K_F1)] 	= XK_F1;
	map[INDEX(K_F2)] 	= XK_F2;
	map[INDEX(K_F3)] 	= XK_F3;
	map[INDEX(K_F4)] 	= XK_F4;
	map[INDEX(K_F5)] 	= XK_F5;
	map[INDEX(K_F6)] 	= XK_F6;
	map[INDEX(K_F7)] 	= XK_F7;
	map[INDEX(K_F8)] 	= XK_F8;
	map[INDEX(K_F9)] 	= XK_F9;
	map[INDEX(K_F10)] 	= XK_F10;
	map[INDEX(K_F11)] 	= XK_F11;
	map[INDEX(K_F12)] 	= XK_F12;

	map[INDEX(K_F1)+1] 	= XK_F13;
	map[INDEX(K_F2)+1] 	= XK_F14;
	map[INDEX(K_F3)+1] 	= XK_F15;
	map[INDEX(K_F4)+1] 	= XK_F16;
	map[INDEX(K_F5)+1] 	= XK_F17;
	map[INDEX(K_F6)+1] 	= XK_F18;
	map[INDEX(K_F7)+1] 	= XK_F19;
	map[INDEX(K_F8)+1] 	= XK_F20;
	map[INDEX(K_F9)+1] 	= XK_F21;
	map[INDEX(K_F10)+1] 	= XK_F22;
	map[INDEX(K_F11)+1] 	= XK_F23;
	map[INDEX(K_F12)+1] 	= XK_F24;

	map[INDEX(K_Shift_R)] 	= XK_Shift_R;
	map[INDEX(K_Shift_L)] 	= XK_Shift_L;
	map[INDEX(K_Control_R)] = XK_Control_L;
	map[INDEX(K_Control_L)] = XK_Control_L;
	map[INDEX(K_Alt_R)] 	= XK_Alt_L;
	map[INDEX(K_Alt_L)]	= XK_Alt_L;
	map[INDEX(K_space)] 	= XK_space;
	map[INDEX(K_quoteright)] = XK_quoteright;
	map[INDEX(K_comma)] 	= XK_comma;
	map[INDEX(K_minus)] 	= XK_minus;
	map[INDEX(K_period)] 	= XK_period;
	map[INDEX(K_slash)] 	= XK_slash;
	map[INDEX(K_0)] 	= XK_0;
	map[INDEX(K_1)] 	= XK_1;
	map[INDEX(K_2)] 	= XK_2;
	map[INDEX(K_3)] 	= XK_3;
	map[INDEX(K_4)] 	= XK_4;
	map[INDEX(K_5)] 	= XK_5;
	map[INDEX(K_6)] 	= XK_6;
	map[INDEX(K_7)] 	= XK_7;
	map[INDEX(K_8)] 	= XK_8;
	map[INDEX(K_9)] 	= XK_9;
	map[INDEX(K_semicolon)] = XK_semicolon;
	map[INDEX(K_equal)] 	= XK_equal;
	map[INDEX(K_bracketleft)] = XK_bracketleft;
	map[INDEX(K_bracketleft)+1] = XK_braceleft;
	map[INDEX(K_backslash)] = XK_backslash;
	map[INDEX(K_backslash)+1] = XK_bar;
	map[INDEX(K_bracketright)] = XK_bracketright;
	map[INDEX(K_bracketright)+1] = XK_braceright;
	map[INDEX(K_quoteleft)] = XK_quoteleft;
	map[INDEX(K_a)]	 	= XK_A;
	map[INDEX(K_b)] 	= XK_B;
	map[INDEX(K_c)] 	= XK_C;
	map[INDEX(K_d)] 	= XK_D;
	map[INDEX(K_e)] 	= XK_E;
	map[INDEX(K_f)] 	= XK_F;
	map[INDEX(K_g)] 	= XK_G;
	map[INDEX(K_h)] 	= XK_H;
	map[INDEX(K_i)] 	= XK_I;
	map[INDEX(K_j)] 	= XK_J;
	map[INDEX(K_k)] 	= XK_K;
	map[INDEX(K_l)] 	= XK_L;
	map[INDEX(K_m)] 	= XK_M;
	map[INDEX(K_n)] 	= XK_N;
	map[INDEX(K_o)] 	= XK_O;
	map[INDEX(K_p)] 	= XK_P;
	map[INDEX(K_q)] 	= XK_Q;
	map[INDEX(K_r)] 	= XK_R;
	map[INDEX(K_s)] 	= XK_S;
	map[INDEX(K_t)] 	= XK_T;
	map[INDEX(K_u)] 	= XK_U;
	map[INDEX(K_v)] 	= XK_V;
	map[INDEX(K_w)] 	= XK_W;
	map[INDEX(K_x)] 	= XK_X;
	map[INDEX(K_y)] 	= XK_Y;
	map[INDEX(K_z)] 	= XK_Z;
	/* Num_Lock is used to second group of keysyms */
	map[INDEX(K_Num_Lock)] 	= XK_Mode_switch;
	map[INDEX(K_Caps_Lock)] = XK_Caps_Lock;
	map[INDEX(K_Scroll_Lock)] = XK_Scroll_Lock;

	map[INDEX(K_1)+1] 	= XK_exclam;
	map[INDEX(K_2)+1] = XK_at;
	map[INDEX(K_quoteleft)+1] = XK_asciitilde;
	map[INDEX(K_comma)+1] = XK_less;
	map[INDEX(K_period)+1] = XK_greater;
	map[INDEX(K_3)+1] = XK_numbersign;
	map[INDEX(K_4)+1] = XK_dollar;
	map[INDEX(K_5)+1] = XK_percent;
	map[INDEX(K_6)+1] = XK_asciicircum;
	map[INDEX(K_7)+1] = XK_ampersand;
	map[INDEX(K_8)+1] = XK_asterisk;
	map[INDEX(K_9)+1] = XK_parenleft;
	map[INDEX(K_semicolon)+1] = XK_colon;
	map[INDEX(K_slash)+1] = XK_question;
	map[INDEX(K_equal)+1] = XK_plus;
	map[INDEX(K_quoteright)+1] = XK_quotedbl;
	map[INDEX(K_0)+1] = XK_parenright;
	map[INDEX(K_minus)+1] 	= XK_underscore;
#undef INDEX
}
