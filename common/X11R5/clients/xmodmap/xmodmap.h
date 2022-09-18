/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xmodmap:xmodmap.h	1.1"
/*
 * xmodmap - program for loading keymap definitions into server
 *
 * $XConsortium: xmodmap.h,v 1.7 91/07/17 22:26:31 rws Exp $
 *
 * Copyright 1988 Massachusetts Institute of Technology
 *
 *
 * Author:  Jim Fulton, MIT X Consortium
 */

extern char *ProgramName;
extern Display *dpy;
extern int min_keycode, max_keycode;
extern Bool verbose;
extern Bool dontExecute;
extern char *inputFilename;
extern int lineno;
extern int parse_errors;

extern void initialize_map ();
extern void process_file ();
extern void process_line ();
extern void handle_line ();
extern void print_opcode ();
extern void print_work_queue ();
extern int execute_work_queue ();
extern void print_modifier_map ();
extern void print_key_table ();
extern void print_pointer_map ();
