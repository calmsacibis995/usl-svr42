/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xmodmap:wq.h	1.1"
/*
 * xmodmap - program for loading keymap definitions into server
 *
 * $XConsortium: wq.h,v 1.4 88/10/08 15:41:34 jim Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */


/* 
 * Input is parsed and a work queue is built that is executed later.  This
 * allows us to swap keys as well as ensure that we don't mess up the keyboard
 * by doing a partial rebind.
 */

enum opcode { doKeycode, doAddModifier, doRemoveModifier, doClearModifier,
	      doPointer };

struct op_generic {
    enum opcode type;			/* oneof enum opcode */
    union op *next;			/* next element in list or NULL */
};


/*
 * keycode KEYCODE = KEYSYM
 * keysym OLDKEYSYM = NEWKEYSYM
 *
 * want to eval the OLDKEYSYM before executing the work list so that it isn't
 * effected by any assignments.
 */

struct op_keycode {
    enum opcode type;			/* doKeycode */
    union op *next;			/* next element in list or NULL */
    KeyCode target_keycode;		/* key to which we are assigning */
    int count;				/* number of new keysyms */
    KeySym *keysyms;			/* new values to insert */
};


/*
 * add MODIFIER = KEYSYM ...
 */

struct op_addmodifier {
    enum opcode type;			/* doAddModifier */
    union op *next;			/* next element in list or NULL */
    int modifier;			/* index into modifier list */
    int count;				/* number of keysyms */
    KeySym *keysyms;			/* new values to insert */
};


/*
 * remove MODIFIER = OLDKEYSYM ...
 *
 * want to eval the OLDKEYSYM before executing the work list so that it isn't
 * effected by any assignments.
 */

struct op_removemodifier {
    enum opcode type;			/* doRemoveModifier */
    union op *next;			/* next element in list or NULL */
    int modifier;			/* index into modifier list */
    int count;				/* number of keysyms */
    KeyCode *keycodes;			/* old values to remove */
};


/*
 * clear MODIFIER
 */

struct op_clearmodifier {
    enum opcode type;			/* doClearModifier */
    union op *next;			/* next element in list or NULL */
    int modifier;			/* index into modifier list */
};

/*
 * pointer = NUMBER ...
 *
 * set pointer map to the positive numbers given on the right hand side
 */

#define MAXBUTTONCODES 256		/* there are eight bits of buttons */

struct op_pointer {
    enum opcode type;			/* doPointer */
    union op *next;			/* next element in list or NULL */
    int count;				/* number of new button codes */
    unsigned char button_codes[MAXBUTTONCODES];
};


/*
 * all together now
 */
union op {
    struct op_generic generic;
    struct op_keycode keycode;
    struct op_addmodifier addmodifier;
    struct op_removemodifier removemodifier;
    struct op_clearmodifier clearmodifier;
    struct op_pointer pointer;
};

extern struct wq {
    union op *head;
    union op *tail;
} work_queue;


extern struct modtab {
    char *name;
    int length;
    int value;
} modifier_table[];

#define AllocStruct(s) ((s *) malloc (sizeof (s)))

#define MAXKEYSYMNAMESIZE 80		/* absurdly large */
