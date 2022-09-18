/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:spider.c	1.1"
/*
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)spider.c	2.1	90/04/25
 *
 */

/*
 * Spider card logic
 */

#include	"defs.h"
#include	"globals.h"
#include	<ctype.h>

static void	fix_coords();

int 	deltamod = 0;

/*
 * build all the cards, stacks, piles and the original deck
 */
card_init()
{
int	i;
Suit	suit;
Rank	rank;
CardPtr	tmp, tmp2;

	for (i = 0; i < NUM_STACKS; i++)	{
		stack[i] = (CardList) malloc(sizeof(CardListStruct));
		stack[i]->place = i + 11;
		stack[i]->cards = CARDNULL;
		stack[i]->card_delta = CARD_DELTA;
		stack[i]->x = STACK_LOC_X(stack[i]->place);
		stack[i]->y = STACK_LOC_Y;
	}

	for (i = 0; i < NUM_PILES; i++)	{
		piles[i] = (CardList) malloc(sizeof(CardListStruct));
		piles[i]->place = i + 1;
		piles[i]->cards = CARDNULL;
		piles[i]->card_delta = 0;
		piles[i]->x = PILE_LOC_X(piles[i]->place);
		piles[i]->y = PILE_LOC_Y;
	}
	deck = (CardList) malloc(sizeof(CardListStruct));
	deck->place = DECK;
	deck->x = DECK_X;
	deck->y = DECK_Y;
	deck->card_delta = 0;
	deck->cards = CARDNULL;
	tmp2 = CARDNULL;
	for (i = 0; i < NUM_DECKS; i++)	{
		for (suit = Spade; suit <= Club; suit++)	{
			for (rank = Ace; rank <= King; rank++)	{
				tmp = (CardPtr) calloc(sizeof(CardStruct), 1);
				add_card(tmp, tmp2, LOC_BEFORE, deck);
				tmp->rank = rank;
				tmp->suit = suit;
				tmp->type = Facedown;
				tmp2 = tmp;
			}
		}
	}
	deck->cards = tmp;
	srandom(getpid());
	shuffle_cards();
}

/*
 * randomizes order of deck list
 */

struct	shuffle {
	CardPtr	card;
	long	value;
} shuffled_cards[NUM_CARDS];

compare(a1, a2)
struct shuffle	*a1, *a2;
{
	return ((a2->value) - (a1->value));
}

/*
 * removes all the cards from the table and stcks them in a cache
 */
remove_all_cards(cache)
CardPtr	cache[NUM_CARDS];
{
CardPtr	tmp;
int	i, j;

	i = 0;
	while (deck->cards)	{
		tmp = deck->cards;
		remove_card(tmp);
		cache[i++] = tmp;
	}

	for (j = 0; j < NUM_PILES; j++)	{
		while (piles[j]->cards)	{
			tmp = piles[j]->cards;
			remove_card(tmp);
			cache[i++] = tmp;
		}
	}

	for (j = 0; j < NUM_STACKS; j++)	{
		while (stack[j]->cards)	{
			tmp = stack[j]->cards;
			remove_card(tmp);
			cache[i++] = tmp;
		}
	}

	assert(i == NUM_CARDS);
}

/*
 * shuffle the cards 
 */
shuffle_cards()
{
int	i;
CardPtr	cache[NUM_CARDS];
extern long	random();

	remove_all_cards(cache);

	for (i = 0; i < NUM_CARDS; i++)	{
		shuffled_cards[i].card = cache[i];
		shuffled_cards[i].value = random();
	}

	qsort((char *) shuffled_cards, NUM_CARDS, sizeof(struct shuffle), 
		compare);

	for (i = 0; i < NUM_CARDS; i++)	{
		shuffled_cards[i].card->type = Facedown;
		add_card(shuffled_cards[i].card, deck->cards, LOC_START, deck);
	}

	/* save the deal in the save cache */
	make_deck_cache();

	/* reset card spacing */
	for (i = 0; i < NUM_STACKS; i++)	{
		stack[i]->card_delta = CARD_DELTA;
	}

	/* force things to get fixed up */
	XClearArea(dpy, table, 0, 0, 0, 0, True);

	deal_number = 0;
	restart = False;

	/* reset move log */
	init_cache();
}

/*
 * does orginal deal
 */
deal_cards()
{
int	i, j;
int	num;
	
	/*
	 * this is the way the original does deals -- weird, but
	 * thats compatibility
	 */
	for (i = 0; i < NUM_STACKS; i++)	{
		CardPtr	tmp[6];

		/* stacks 1, 4, 7, and 10 have 1 extra card */
		if (((i+1) % 3) == 1)	{
			num = 5;
		} else	{
			num = 4;
		}

		/* faceup first */
		tmp[num] = deck->cards;
		remove_card(tmp[num]);
		tmp[num]->type = Faceup;

		for (j = (num - 1); j >= 0; j--)	{
			tmp[j] = deck->cards;
			remove_card(tmp[j]);
			tmp[j]->type = Facedown;
		}
		for (j = 0; j <= num; j++)	{
			add_card(tmp[j], stack[i]->cards, LOC_END, stack[i]);
		}
	}
	deck_index -= 54;
	deal_number = 1;

	/*
	 * show the deal
	 */
	for (i = 0; i < NUM_STACKS; i++)	{
		show_list(stack[i], stack[i]->cards);
	}
}

/*
 * deal hand of 10
 */
deal_next_hand(log)
Bool	log;
{
int	i;
CardPtr	tmp;
char	buf[128];
int	old_delta;

	/* be sure all the spaces are filled */
	for (i = 0; i < NUM_STACKS; i++)	{
		if (stack[i]->cards == CARDNULL)	{
			show_message("Can't deal until all spaces are filled");
			spider_bell(dpy, 0);
			return;
		}
	}
	if (deck->cards == CARDNULL)	{
		/* dealt them all */
		show_message("No more cards in deck");
		spider_bell(dpy, 0);
		return;
	}
	/* deal face up cards */
	for (i = 0; i < NUM_STACKS; i++)	{
		old_delta = stack[i]->card_delta;
		tmp = deck->cards;
		remove_card(tmp);
		add_card(tmp, stack[i]->cards, LOC_END, stack[i]);
		if (old_delta != stack[i]->card_delta)	{
			tmp->type = Faceup;
			show_list(stack[i], stack[i]->cards);
		} else	{
			flip_card(tmp, Faceup);
		}
	}

	deck_index -= 10;

	/* force deck to repaint itself if its empty */
	if (deck->cards == CARDNULL)
		redraw_deck(0, 0, table_width, table_height);

	assert (deal_number >= 1);

	if (log)
		record (0, 0, 0, True);
	(void)sprintf(buf, "Dealt hand %d of 5", deal_number);
	show_message(buf);

	assert((deal_number < 5) || (deck_index == 0));

	deal_number++;
}

/*
 * change the state of a card
 */
flip_card(card, state)
CardPtr	card;
Type	state;
{
	card->type = state;
	show_card(card);
}

/*
 * place a card on a list
 *
 * expects 'new' to have been removed from any list
 */
add_card(new, old, location, list)
CardPtr	new, old;
int	location;
CardList	list;
{

	assert(new->prev == CARDNULL);
	assert(new->next == CARDNULL);
	assert((location == LOC_BEFORE) || (location == LOC_AFTER)
		|| (location == LOC_END) || (location == LOC_START));
	assert ((old == NULL) || (old->list == list));


	if (location == LOC_END)	{
		old = last_card(list);
		location = LOC_AFTER;
		/* let the later code do the work */
	} else if (location == LOC_START)	{
		old = list->cards;
		location = LOC_BEFORE;
		/* let the later code do the work */
	}

	assert((location == LOC_BEFORE) || (location == LOC_AFTER));

	/* fix the list */
	if (list->cards == CARDNULL)
		list->cards = new;
	new->list = list;

	if (location == LOC_BEFORE)	{
		if (old)	{
			if (list->cards == old)
				list->cards = new;
			if (old->prev)
				old->prev->next = new;
			new->prev = old->prev;
			new->next = old;
			old->prev = new;
		} else	{
			new->next = old;
		}
	} else 	{		/* LOC_AFTER */
		if (old)	{
			if (old->next)
				old->next->prev = new;
			new->prev = old;
			new->next = old->next;
			old->next = new;
		} else	{
			new->prev = old;
		}
	}

	fix_coords(new, list, False);
}

/*
 * fix up the inter-card spacing for a stack
 */
static void
fix_coords(new, list, display)
CardPtr	new;
CardList	list;
Bool	display;
{
	/* fix the coords */
	new->x = list->x;
	if (new->prev)	{	/* added to bottom */
		if ((new->prev->y + list->card_delta + CARD_HEIGHT) > 
		     table_height)	{
			recompute_list_deltas(list);
			if (display)
				show_list(list, list->cards);
		}
		new->y = new->prev->y + list->card_delta;
	} else 	{
		new->y = list->y;
	}
}

/*
 * compute the inter-card spacing for a stack
 */
void
recompute_list_deltas(list)
CardList	list;
{
CardPtr	tmp;
int	delta, num = 0;
	
	assert (list->place >= STACK_1);

	tmp = list->cards;
	while (tmp)	{
		num++;
		tmp = tmp->next;
	}

	/* don't do anything if 1 or fewer cards */
	if (num <= 1)	{
		delta = CARD_DELTA;
		return;
	}

	/* adjust 'size' of stack to limit the amount of redrawing */
	if (deltamod)
		num = (num + deltamod - 1)/deltamod * deltamod;

	delta = (table_height - (STACK_LOC_Y + 10 + CARD_HEIGHT))/(num - 1);

	if (delta > CARD_DELTA)
		delta = CARD_DELTA;

	if (list->card_delta != delta)	{
		list->card_delta = delta;
		tmp = list->cards;
		while (tmp)	{
			fix_coords(tmp, list, False);
			tmp = tmp->next;
		}
	}
}

/*
 * remove a card from a list
 */
remove_card(card)
CardPtr	card;
{
	/* fix card pointers */
	if (card->prev)
		card->prev->next = card->next;
	if (card->next)
		card->next->prev = card->prev;

	/* fix up card list */
	if (card->prev == CARDNULL)
		card->list->cards = card->next;

	/* clear pointers */
	card->next = CARDNULL;
	card->prev = CARDNULL;
	card->list = CARDLISTNULL;
}

/*
 * move an entire sublist to another list
 */
void
move_to_list(card, list, log)
CardPtr		card;
CardList	list;
Bool		log;
{
CardPtr	tmp;
int	from, dest;
int	count = 0;
Bool	exposed = False;
int	delta;

	/* fix old list */
	if (card->prev)	{
		card->prev->next = CARDNULL;
		if (card->prev->type == Facedown)	{
			exposed = True;
			card->prev->type = Faceup;
		}
	} else	{
		card->list->cards = CARDNULL;
	}

	/* shrink stack if necessary */
	if (card->list->place >= STACK_1 && 
	    card->list->card_delta != CARD_DELTA)	{
		recompute_list_deltas(card->list);
		show_list(card->list, card->list->cards);
	} else	{
		show_list(card->list, card->prev);
	}
	from = STACK_INDEX(card->list->place) + 1;

#ifdef DEBUG
	validate_card_list(card->list);
#endif

	tmp = last_card(list);
	if (tmp)	{
		assert(tmp->next == CARDNULL);
		tmp->next = card;
		card->prev = tmp;
	} else	{
		list->cards = card;
		card->prev = CARDNULL;
	}
	tmp = card;
	while (tmp)	{
		count++;
		tmp->list = list;
		delta = list->card_delta;
		fix_coords(tmp, list, True);
		/* only show card if fix_coords() didn't */
		if (delta == list->card_delta)
			show_card(tmp);
		tmp = tmp->next;
	}

#ifdef DEBUG
	validate_card_list(list);
#endif
	if (log)	{
		dest = (IS_PILE(list)) ? 0 : STACK_INDEX(list->place) + 1;
		record(from, dest, count, exposed);
	}
}

#ifdef DEBUG
print_list(list)
CardList	list;
{
CardPtr	tmp;
	
	tmp = list->cards;

	while (tmp)	{
		(void) fprintf(stderr,"card is %s of %s (%s)\n",
			rank_name(tmp->rank), 
			suit_name(tmp->suit), 
			type_name(tmp->type));
		tmp = tmp->next;
	}
}

validate_card_list(list)
CardList	list;
{
CardPtr	tmp;
	
	tmp = list->cards;
	if (tmp == CARDNULL)
		return;
	if (tmp->prev != CARDNULL)	{
		(void) fprintf(stderr,
			"validate list:  first card has non-null prev\n");
	}
	while (tmp->next)	{
		if (tmp->next->prev != tmp)
			(void) fprintf(stderr,"validate list: bad link\n");
		if (tmp->list != list)
			(void) fprintf(stderr,
				"validate list: card/list mismatch\n");
		tmp = tmp->next;
	}
}
#endif

/*
 * rank & suit value->string roputines
 */


static	char	*rnk_names[] =	{
	"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
};

/*
 * shortened version for save files and info
 */
char	*
rnk_name(rank)
Rank	rank;
{
	assert(rank >= Ace && rank <= King);

	return (rnk_names[rank]);
}

static char	*rank_names[] =	{
	"Ace", "Deuce", "Three", "Four", "Five", "Six", "Seven",
	"Eight", "Nine", "Ten", "Jack", "Queen", "King"
};

char	*
rank_name(rank)
Rank	rank;
{
	assert(rank >= Ace && rank <= King);

	return (rank_names[rank]);
}

static char	*suit_names[] = {
	"Spades", "Hearts", "Diamonds", "Clubs"
}; 

char	*
suit_name(suit)
Suit	suit;
{
	assert (suit >= Spade && suit <= Club);

	return (suit_names[suit]);
}

#ifdef DEBUG
static char	*type_names[] =	{
	"Faceup", "Facedown", "Joker"
};

char	*
type_name(type)
Type	type;
{
	assert (type >= Faceup && type <= Joker);

	return (type_names[type]);
}
#endif /*DEBUG*/


/*
 * return the bottom-most card of a list
 */
CardPtr
last_card(list)
CardList	list;
{
CardPtr	tmp = CARDNULL;

	if ((list == CARDLISTNULL) || (list->cards == CARDNULL))
		return (CARDNULL);

	tmp = list->cards;
	while (tmp->next)	{
		tmp = tmp->next;
	}
	return (tmp);
}

/*
 * can only move card if there's a run of the same suit underneath it
 */
Bool
can_move(card)
CardPtr	card;
{
CardPtr	tmp;
Rank	last_rank;

	if (card->type != Faceup)
		return (False);
	last_rank = card->rank;
	tmp = card->next;
	while (tmp)	{
		if ((tmp->rank != (last_rank - 1)) || (tmp->suit != card->suit))
			return (False);
		last_rank = tmp->rank;
		tmp = tmp->next;
	}
	return (True);
}

/*
 * can 'card' go on to 'dest' ?
 */
Bool
can_move_to(card, list)
CardPtr	card;
CardList	list;
{
CardPtr	tmp;

	assert (can_move(card));
	tmp = last_card(list);
	if (tmp == CARDNULL)
		return (True);
	return (tmp->rank == (card->rank + 1));
}

/*
 * finds the best move for a specific card
 *
 * use the first 'next best' move so we choose them from right to left
 */
CardList
best_card_move(card)
CardPtr	card;
{
CardList	next_best = CARDLISTNULL;
CardList	space = CARDLISTNULL;
CardPtr	tmp;
int	i;

	/* iterate through the stacks */
	for (i = 0; i < NUM_STACKS; i++)	{
		/* don't look at our own stack */
		if (stack[i] == card->list)
			continue;
		tmp = last_card(stack[i]);
		if (tmp == CARDNULL)	{	/* spaces are ok */
			if (next_best == CARDLISTNULL)
				space = stack[i];
			continue;
		}
		/* rank & suit is optimal */
		if (tmp->rank == (card->rank + 1))	{
			if (tmp->suit == card->suit)
				return (tmp->list);
			/* just rank is the next best */
			if (next_best == CARDLISTNULL)
				next_best = tmp->list;
		}
	}
	if (next_best == CARDLISTNULL)
		next_best = space;

	return (next_best);
}

/*
 * performs the best move for an entire sub-list
 */
void
best_list_move(list)
CardList	list;
{
CardPtr	tmp, tmp2;
CardList	best = CARDLISTNULL;

	tmp = list->cards;
	if (tmp == CARDNULL)	{
		show_message("Empty list");
		spider_bell(dpy, 0);
		return;
	}

	/*
	 * iterate through stack.  for each card that can move,
	 * try to find one.  return as soon as we find one
	 */
	while (tmp)	{
		if (can_move(tmp))	{
			/*
			 * special case full suits
			 */
			if (tmp->rank == King)	{
				tmp2 = last_card(list);
				if (tmp2->rank == Ace)	{
					move_to_pile(tmp);
					return;
				}
			}
			best = best_card_move(tmp);
			if (best)	{
				move_to_list(tmp, best, True);
			} else	{
				card_message("Nowhere to move the", tmp);
				spider_bell(dpy, 0);
			}
			return;
		}
		tmp = tmp->next;
	}
}

void
move_to_pile(card)
CardPtr	card;
{
int	i;

	for (i = 0; i < NUM_PILES; i++)	{
		if (piles[i]->cards == CARDNULL)
			break;
	}
	assert(i < NUM_PILES);

	move_to_list(card, piles[i], True);
}


/*
 * is card thru lastcard King - Ace?
 */
static Bool
is_sequence(card)
CardPtr	card;
{
CardPtr	tmp;

	if (card->rank != King)
		return (False);
	tmp = card;
	while (tmp->next)	{
		if (!(tmp->next && (tmp->suit == tmp->next->suit) &&
		    (tmp->rank == (tmp->next->rank + 1))))
			return (False);
		tmp = tmp->next;
	}
	if (tmp->rank == Ace)
		return (True);
	else
		return (False);
}

/*
 * Compute a somewhat arbitrary evaluation function for the position:
 *    2 point per card sitting atop next higher card in same suit
 *   10 per card turned face up
 *   15 extra for each column where all cards have been revealed
 *   50 per completed suit removed (note this costs 12*2 for cards in seq)
 * If all columns are either empty or contain completed suits, then those
 * suits also count 50 (including the 24 for the 12 cards that are atop
 * higher cards), plus an extra 2 for each suit after the first three.
 * Thus the only way to get 1000 points is to win with all eight suits
 * still in the tableau.
 */

int
compute_score()
{
int	score = 0;
int	i;
CardPtr	tmp;
int	num_piles = 0;

	if (deal_number == 0)
		return (0);

	score = 44 * 10;		/* score if all cards flipped */
	for (i = 0; i < NUM_PILES; i++)	{
		if (piles[i]->cards)
			score += 50;
	}

	for (i = 0; i < NUM_STACKS; i++)	{
		if (stack[i]->cards)	{
			if (stack[i]->cards->type == Faceup)	{
				score += 15;
				if (is_sequence(stack[i]->cards))	{
					score += 50;
					num_piles++;
					if (num_piles > 3)	{
						score += 2;
					}
					continue;
				}
			}
		} else	{
			score += 15;
		}

		tmp = stack[i]->cards;
		while (tmp)	{
			if (tmp->type == Faceup)	{
				if (tmp->prev)	{
					if ((tmp->prev->type == Faceup) &&
					    (tmp->rank == (tmp->prev->rank - 1))
					    && (tmp->suit == tmp->prev->suit))
						score += 2;
				}
			} else	{
				score -= 10;	/* still Facedown */
			}
			tmp = tmp->next;
		}
	}

	return (score);
}

/*
 * display which suits have all their cards visible
 */
show_full_suits()
{
char	showing[NUM_RANKS][NUM_SUITS];
Bool	all[NUM_SUITS];
int	num = 0;
int	i, j;
CardPtr	tmp;
char	buf[128];

	for (i = 0; i < NUM_RANKS; i++)
		for (j = 0; j < NUM_SUITS; j++)
			showing[i][j] = 0;

	for (i = 0; i < NUM_STACKS; i++)	{
		tmp = stack[i]->cards;
		while (tmp)	{
			if (tmp->type == Faceup)	{
				showing[tmp->rank][tmp->suit]++;
			}
			tmp = tmp->next;
		}
	}
	for (j = 0; j < NUM_SUITS; j++)	{
		all[j] = True;
		for (i = 0; i < NUM_RANKS; i++)	{
			if (showing[i][j] == 0)	{
				all[j] = False;
				break;
			}
		}
		if (all[j])
			num++;
	}
	if (num == 0)	{
		show_message("No suit has all 13 cards showing.");
	} else	{
		(void) strcpy(buf, 
			"Sufficient cards visible to form complete set of ");
		for (j = 0; j < NUM_SUITS; j++)	{
			if (all[j])	{
				(void)strcat(buf, suit_name((Suit) j));
				if (--num)	{
					(void)strcat(buf, ", ");
				} else	{
					(void)strcat(buf, ".");
				}
			}
		}
		show_message(buf);
	}
}

/*
 * print cards in list
 */
expand(list)
CardList	list;
{
CardPtr	tmp, last;
char	buf[512], buf2[10];
Bool	sequence = False;


	tmp = list->cards;
	if (tmp == CARDNULL)	{
		show_message("Empty column.");
		return;
	}
	(void)strcpy(buf, "Column contains:");
	last = CARDNULL;
	while (tmp)	{
		if (tmp->type != Faceup)	{
			tmp = tmp->next;
			continue;
		}
		if (last && last->suit == tmp->suit && 
			(last->rank == tmp->rank + 1))		{
			if (!sequence)	{
				sequence = True;
			}
		} else	{
			if (sequence)	{
				(void)sprintf(buf2, "-%s%c %s%c", 
					rnk_name(last->rank),
					tolower(*suit_name(last->suit)),
					rnk_name(tmp->rank),
					tolower(*suit_name(tmp->suit)));
				sequence = False;
			} else	{
				(void)sprintf(buf2, " %s%c",rnk_name(tmp->rank),
					tolower(*suit_name(tmp->suit)));
			}
			(void)strcat(buf, buf2);
		}
		last = tmp;
		tmp = tmp->next;
	}
	/* handle dangling sequences */
	if (sequence)	{
		(void)sprintf(buf2, "-%s%c", rnk_name(last->rank),
			tolower(*suit_name(last->suit)));
		(void)strcat(buf, buf2);
	}
	show_message(buf);
}

static int
col_locate(list, suit, rank, checksuit)
CardList	list;
Suit	suit;
Rank	rank;
Bool	checksuit;
{
CardPtr	tmp;
int	count = 0;

	tmp = list->cards;
	for (tmp = list->cards; tmp; tmp = tmp->next)	{
		if (tmp->type != Faceup)
			continue;
		if ((!checksuit || tmp->suit == suit) && 
		    tmp->rank == rank)
			count++;
	}
	return	count;
}

void
locate(str)
char	*str;
{
int	i, num;
Suit	suit;
Rank	rank;
char	buf[512], buf2[256], times[32];
Bool	found = False, checksuit = False;

	if (!str)
		return;
	/*
	 * assume that the string is well formed (probably stupid
	 * assumption) and treat accordingly
	 */
	for (i = 0; i < strlen(str); i++)	{
		switch(str[i])	{
		case	'D':
		case	'd':
			suit = Diamond;
			checksuit = True;
			break;
		case	'S':
		case	's':
			suit = Spade;
			checksuit = True;
			break;
		case	'H':
		case	'h':
			suit = Heart;
			checksuit = True;
			break;
		case	'C':
		case	'c':
			suit = Club;
			checksuit = True;
			break;
		case	'A':
		case	'a':
			rank = Ace;
			break;
		case	'T':
		case	't':
			rank = Ten;
			break;
		case	'J':
		case	'j':
			rank = Jack;
			break;
		case	'Q':
		case	'q':
			rank = Queen;
			break;
		case	'K':
		case	'k':
			rank = King;
			break;
		default:
			rank = atoi(str) - 1;
			if (rank < Deuce || rank > Ten)	{
				(void)sprintf(buf, 
					"Invalid card specification %s", str);
				show_message(buf);
				return;
			}
			break;
		}
	}

	(void)sprintf(buf, "%s ", str);
	for (i = 0; i < NUM_STACKS; i++)	{
		if (num = col_locate(stack[i], suit, rank, checksuit))	{
			if (found)	{
				(void)strcat(buf, ", ");
			}
			found = True;
			if (num == 1)	{
				(void)strcpy(times, "once");
			} else if (num == 2)	{
				(void)strcpy(times, "twice");
			} else	{
				(void)sprintf(times, "%d times", num);
			}

			(void)sprintf(buf2, "occurs in column %d %s ", i + 1,
						times);
			(void)strcat(buf, buf2);
		}
	}

	if (!found)
		(void)strcat(buf, "is not visible");
	show_message(buf);
}

/*
 * routines to give advice about best move.
 *
 * doing a really good job here may be impossible -- this is just a
 * rough attempt
 */

static void
advise_pile_move(list)
CardList	list;
{
char	buf[128];

	(void) sprintf(buf, "Remove King through Ace in pile %d", 
		STACK_INDEX(list->place) + 1);
	show_message(buf);
}

static void
advise_move(card, from, to)
CardPtr		card;
CardList	from, to;
{
char	buf[128];

	if (to->cards == CARDNULL)	{
		(void) sprintf(buf, "Move %s of %s from stack %d to space in stack %d",
			rank_name(card->rank), suit_name(card->suit),
			STACK_INDEX(from->place) + 1, 
			STACK_INDEX(to->place) + 1);
	} else	{
		(void) sprintf(buf, "Move %s of %s from stack %d to %s of %s on stack %d",
			rank_name(card->rank), suit_name(card->suit),
			STACK_INDEX(from->place) + 1, 
			rank_name(last_card(to)->rank), suit_name(last_card(to)->suit),
			STACK_INDEX(to->place) + 1);
	}
	show_message(buf);
}

/*
 * calculate the relative worth of a move
 *
 * this is by no means an optimal algorithm, since there's no lookahead,
 * but it should be good enough to get a beginner started
 */
/*
 * value is:
 *	head of sublist rank + 100	(best to move the high cards first)
 *    + number of cards			(move as many as possible)
 *    + 200 if show a new card		(dig out cards)
 *    + 400 if show a new space		(dig out cards)
 *    + 800 if same suit		(same suits is preferable)
 *
 * the constants are arbitrary values large enough not to be reached
 * by the rank or count modifiers
 */
#define	RANK_MOVE	100
#define	NEW_CARD_MOVE	200
#define	SPACE_MOVE	400
#define	SAME_SUIT_MOVE	800

/* ARGSUSED */
static int
value_move(card, from, to)
CardPtr		card;
CardList	from, to;
{
int	value;;

	value = card->rank + RANK_MOVE;      /* higher cards are worth more */

	if (!card->prev)	{
		value += SPACE_MOVE;
	} else if (card->prev->type == Facedown)	{
		value += NEW_CARD_MOVE;
	}
	/* avoid moving to a space */
	if (last_card(to) == CARDNULL)	{
		/* don't space hop */
		if (card->prev == CARDNULL)	{
			value = 0;
		} else	{
			value /= 2;
		}
	/* same suit is worth a lot more */
	} else if (card->suit == last_card(to)->suit)	{
		value += SAME_SUIT_MOVE;
	/* avoid jumping back & forth from two equal moves */
	} else if (card->prev && card->prev->type == Faceup &&
		(card->prev->rank == (card->rank + 1)))	{
		value = 0;
	}
	return (value);
}

/*
 * finds the 'best' move and displays it
 */
void
advise_best_move()
{
CardPtr		tmp, tmp2, bestcard;
CardList	move = CARDLISTNULL,
		bestfrom = CARDLISTNULL,
		bestto = CARDLISTNULL;
int		best_value = 0, val;
CardList	list;
int		i;

	for (i = 0; i < NUM_STACKS; i++)	{
		list = stack[i];
		tmp = list->cards;
		if (tmp == CARDNULL)	{
			continue;
		}

		/*
		 * iterate through stack.  for each card that can move,
		 * calculate the move value
		 */
		while (tmp)	{
			if (can_move(tmp))	{
				/*
				 * special case full suits
				 */
				if (tmp->rank == King)	{
					tmp2 = last_card(list);
					if (tmp2->rank == Ace)	{
						advise_pile_move(list);
						return;
					}
				}
				move = best_card_move(tmp);
				if (move)	{
					val = value_move(tmp, list, move);
					if (val > best_value)	{
						bestfrom = list;
						bestto = move;
						bestcard = tmp;
						best_value = val;
					}
				}
				break;	/* finished with this stack */
			}
			tmp = tmp->next;
		}
	}
	if (bestfrom)	{
		advise_move(bestcard, bestfrom, bestto);
	} else	{
		if (deck->cards == CARDNULL)	{
			show_message("Its all over.");
		} else	{
			show_message("Deal the next hand.");
		}
	}
}
