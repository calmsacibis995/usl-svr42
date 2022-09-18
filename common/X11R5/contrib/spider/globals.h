/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:globals.h	1.2"
/*
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)globals.h	2.1	90/04/25
 *
 */

/*
 * spider global variables
 */
Display	*dpy;
int	screen;
Window	table;
#ifdef 	KITLESS
Window	message_win;
XFontStruct	*message_font;
#endif	/* KITLESS */
Pixmap	greenmap;
Pixmap	redmap;
Pixmap	logomap;

unsigned long	blackpixel;
unsigned long	whitepixel;
unsigned long	borderpixel;
unsigned long	greenpixel;

Bool	is_color;

CardList	deck;
CardList	stack[NUM_STACKS];		/* tableau */
CardList	piles[NUM_PILES];		/* full suits */

unsigned int		table_height;
unsigned int		table_width;

int		deck_index;

int		draw_count;

Bool		restart;
int		deal_number;

extern char	*version;
extern char	*build_date;

/* function decls */
char	*rank_name();
char	*rnk_name();
char	*suit_name();
#ifdef	DEBUG
char	*type_name();
#endif	/* DEBUG */
char	*get_selection();
char	*remove_newlines();

Bool	can_move();
Bool	can_move_to();

CardList	best_card_move();
CardPtr	last_card();

int	replay();

void	best_list_move();
void	move_to_list();
void	move_to_pile();
void	recompute_list_deltas();

void	show_message();
void	card_message();
void	card2_message();
void	clear_message();
void	print_version();

void	show_play();
void	locate();

void	advise_best_move();
void	delay();
void	force_redraw();

#ifndef KITLESS
void	key_press();
void	redraw_table();
void	button_press();
void	button_release();
void	do_expand();
#endif /*KITLESS*/

#ifdef XAW
Bool	can_get_help_files();
#endif

#ifndef MEMUTIL
extern char	*malloc();
extern char	*calloc();
extern char	*realloc();
#endif /* MEMUTIL */
