/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:defs.h	1.1"
/*
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)defs.h	2.1	90/04/25
 *
 */

/*
 * std includes and types
 */
#include	<X11/Xlib.h>
#include	<X11/Xutil.h>
#include	<stdio.h>
#include	"assert.h"
#include	"copyright.h"

#ifdef DEBUG
/*
 * so i don't have to keep looking up my constants
 */
typedef enum	{Spade, Heart, Diamond, Club}	Suit;

typedef enum	{Ace, Deuce, Three, Four, Five, Six, Seven,
		 Eight, Nine, Ten, Jack, Queen, King}	Rank;

typedef enum	{Faceup, Facedown, Joker}	Type;

#else /* DEBUG*/

typedef	char	Suit;
typedef	char	Rank;
typedef	char	Type;

#define	Spade	0
#define	Heart	1
#define	Diamond	2
#define	Club	3

#define	Ace	0
#define	Deuce	1
#define	Three	2
#define	Four	3
#define	Five	4
#define	Six	5
#define	Seven	6
#define	Eight	7
#define	Nine	8
#define	Ten	9
#define	Jack	10
#define	Queen	11
#define	King	12

#define	Faceup		0
#define	Facedown	1
#define	Joker		2

#endif /*DEBUG*/

#define	NUM_DECKS	2
#define	NUM_PILES	8
#define	NUM_STACKS	10
#define	NUM_RANKS	13
#define	NUM_SUITS	4
#define	CARDS_PER_DECK	(NUM_RANKS * NUM_SUITS)
#define	NUM_CARDS	(NUM_DECKS * CARDS_PER_DECK)

/* diff locations for a cardlist */
#define	DECK	0

#define	PILE_1	1
#define	PILE_2	2
#define	PILE_3	3
#define	PILE_4	4
#define	PILE_5	5
#define	PILE_6	6
#define	PILE_7	7
#define	PILE_8	8

/* convert a pile value to an array index */
#define	PILE_INDEX(i)	((i) - 1)

#define	STACK_1		11
#define	STACK_2		12
#define	STACK_3		13
#define	STACK_4		14
#define	STACK_5		15
#define	STACK_6		16
#define	STACK_7		17
#define	STACK_8		18
#define	STACK_9		19
#define	STACK_10	20

/* convert a stack value to an array index */
#define	STACK_INDEX(i)	((i) - 11)

#define	LOC_BEFORE	1
#define	LOC_AFTER	2
#define	LOC_END		3
#define	LOC_START	4

typedef	struct	_CardStruct	{
	struct _CardStruct	*prev;
	struct _CardStruct	*next;
	struct	_CardList	*list;
	int	x,y;		/* location */
	Suit	suit;
	Rank	rank;
	Type	type;
	int	draw_count;
}	CardStruct, *CardPtr;

#define	CARDNULL	((CardPtr) 0)

typedef struct _CardList	{
	CardPtr	cards;
	int	place;
	int	card_delta;	/* pixels between cards in stack */
	int	x, y;
}	CardListStruct,	*CardList;

#define	CARDLISTNULL	((CardList) 0)

#ifndef SMALL_CARDS
#define	CARD_DELTA	30	
#else
#define	CARD_DELTA	20
#endif	/* !SMALL_CARDS */


#define	IS_PILE(list)	(((list) != CARDLISTNULL) && (list)->place < STACK_1)

/* gfx defs */

/* card info*/
#ifndef SMALL_CARDS
#define	CARD_HEIGHT	123
#define	CARD_WIDTH	79

#define	FACECARD_WIDTH	47
#define	FACECARD_HEIGHT	92

#define	RANK_WIDTH	9
#define	RANK_HEIGHT	14

#define	RANK_LOC_X	4
#define	RANK_LOC_Y	7

#define	SMALL_LOC_X	4
#define	SMALL_LOC_Y	(RANK_HEIGHT + RANK_LOC_Y + 3)

#define	MID_CARD_X	(CARD_WIDTH/2)
#define	MID_CARD_Y	(CARD_HEIGHT/2)

#define	CARD_COL1_X	(3 * CARD_WIDTH/10)
#define	CARD_COL2_X	(CARD_WIDTH/2)
#define	CARD_COL3_X	(7 * CARD_WIDTH/10)

/* 5 diff rows for the two main columns */
/* 1 and 5 are top and bottom, 3 is the middle */
/* 2 & 4 are for the 10 & 9 */
#define	CARD_ROW1_Y	(CARD_HEIGHT/5)
#define	CARD_ROW2_Y	(2 * CARD_HEIGHT/5)
#define	CARD_ROW3_Y	(CARD_HEIGHT/2)
#define	CARD_ROW4_Y	(CARD_HEIGHT - 2 * CARD_HEIGHT/5)
#define	CARD_ROW5_Y	(CARD_HEIGHT - CARD_HEIGHT/5)

/* between 1 & 3, 3 & 5 */
#define	CARD_SEVEN_Y	(7 * CARD_HEIGHT/20)
#define	CARD_EIGHT_Y	(CARD_HEIGHT - 7 * CARD_HEIGHT/20)

/* between rows 1 & 2, 4 & 5 */
#define	CARD_TEN_Y1	(3 * CARD_HEIGHT/10)
#define	CARD_TEN_Y2	(CARD_HEIGHT - 3 * CARD_HEIGHT/10)

/* card positioning */
#define	CARD_INSET_X	10
#define	CARD_INSET_Y	(CARD_HEIGHT/8)

#define	STACK_WIDTH	(CARD_WIDTH + 10)
#define	STACK_LOC_X(i)	((STACK_INDEX(i) * STACK_WIDTH) + CARD_INSET_X)
#define	STACK_LOC_Y	(CARD_HEIGHT + 4 * CARD_INSET_Y)

#define	PILE_WIDTH	STACK_WIDTH
#define	PILE_INSET_X	(STACK_WIDTH + CARD_INSET_X + CARD_WIDTH)
#define	PILE_LOC_X(i)	((PILE_INDEX(i) * PILE_WIDTH) + PILE_INSET_X)
#define	PILE_LOC_Y	(CARD_INSET_Y)

#define	DECK_X		CARD_INSET_X
#define	DECK_Y		CARD_INSET_Y

#define	TABLE_X		10
#define	TABLE_Y		10

#define	TABLE_WIDTH	(STACK_WIDTH * NUM_STACKS + 2 * CARD_INSET_X)
#define	TABLE_HEIGHT	(STACK_LOC_Y + 2 * CARD_HEIGHT)
#define	TABLE_BW	2

/* pip info */
#define	PIP_WIDTH	10
#define	PIP_HEIGHT	10

#else	/* SMALL_CARDS */

#define	CARD_HEIGHT	60
#define	CARD_WIDTH	40

/* card positioning */
#define	CARD_INSET_X	10
#define	CARD_INSET_Y	(CARD_HEIGHT/8)

#define	STACK_WIDTH	(CARD_WIDTH + 10)
#define	STACK_LOC_X(i)	((STACK_INDEX(i) * STACK_WIDTH) + CARD_INSET_X)
#define	STACK_LOC_Y	(CARD_HEIGHT + 4 * CARD_INSET_Y)

#define	PILE_WIDTH	STACK_WIDTH
#define	PILE_INSET_X	(STACK_WIDTH + CARD_INSET_X + CARD_WIDTH)
#define	PILE_LOC_X(i)	((PILE_INDEX(i) * PILE_WIDTH) + PILE_INSET_X)
#define	PILE_LOC_Y	(CARD_INSET_Y)

#define	DECK_X		CARD_INSET_X
#define	DECK_Y		CARD_INSET_Y

#define	TABLE_X		10
#define	TABLE_Y		10

#define	TABLE_WIDTH	(STACK_WIDTH * NUM_STACKS + 2 * CARD_INSET_X)
#define	TABLE_HEIGHT	(STACK_LOC_Y + 2 * CARD_HEIGHT)
#define	TABLE_BW	2

#endif /* !SMALL_CARDS */

#ifdef KITLESS
#define	MESSAGE_FONT	"fixed"
#define	MESSAGE_X	10
#endif /* KITLESS */
