/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*copyright	"%c%*/
#ident	"@(#)r4spider:movelog.c	1.2"
/*	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)movelog.c	2.1	90/04/25
 *
 */

/*
 * move logging code & save/restore
 */

#include	"defs.h"
#include	"globals.h"
#include	<string.h>
/*#include	<sys/file.h>*/
#include	<ctype.h>
#include	<X11/Xos.h>

#define	CACHE_SIZE	50
static int	move_index = 0;
static int	*move_cache = NULL;
static int	cache_size = 0;
int	deck_cache[NUM_CARDS];
int	deck_index = 0;

static int restore_game(), card_to_int();

make_deck_cache()
{
CardPtr	tmp;

	deck_index = 0;
	tmp = deck->cards;
	while(tmp)	{
		deck_cache[deck_index++] = card_to_int(tmp);
		tmp = tmp->next;
	}
}

init_cache()
{
	if (cache_size)
		free((char *)move_cache);
	move_cache = (int *) calloc(CACHE_SIZE, sizeof(int));
	cache_size = CACHE_SIZE;
	move_index = 0;
}

grow_cache()
{
int	*new;

	cache_size += CACHE_SIZE;
	new = (int *) realloc((char *)move_cache, 
				(unsigned)(cache_size * sizeof(int)));
	if (new == (int *) NULL)	{
		(void)fprintf(stderr,"realloc failed\n");
		exit(-1);
	}
	move_cache = new;
}

record(from, dest, num_cards, exposed)
int	from, dest, num_cards;
Bool	exposed;
{
int	val;

	val = 11 * ( 11 * ((exposed ? 1 : 0) * 14 + num_cards) + dest) + from;
	move_cache[move_index++] = val;
	if (move_index >= cache_size)
		grow_cache();
}

unencode(val, from, dest, num_cards, exposed)
int	val;
int	*from, *dest, *num_cards;
Bool	*exposed;
{
	*from = val % 11;
	val /= 11;
	*dest = val % 11;
	val /= 11;
	*num_cards = val % 14;
	*exposed = (val >= 14);
}

undo()
{
int	val;
int	from, dest, num_cards;
Bool	exposed;

	if (move_index == 0)	{
		show_message("No moves available to back up over.");
		spider_bell(dpy, 0);
		return;
	}
	val = move_cache[--move_index];
	if (val > 11 * 11 * 14)	{
		show_message("Cheater!");
		spider_bell(dpy, 0);
	}
	unencode(val, &from, &dest, &num_cards, &exposed);

	if (from == 0)	{	/* undo deal */
		undo_deal();
	} else	{
		/*
		 * note that cols are from 0-9, but saved as 1-10,
		 * so we decrement here before doing the work
		 */
		if (dest == 0)	{
			undo_suit(from - 1, exposed);
		} else	{
			undo_normal(from - 1, dest - 1, num_cards, exposed);
		}
	}
}

/*
 * undoes latest deal
 */
undo_deal()
{
int	i;
CardPtr	tmp;

	for (i = (NUM_STACKS - 1); i >= 0; i--)	{
		tmp = last_card(stack[i]);
		remove_card(tmp);
		add_card(tmp, deck->cards, LOC_BEFORE, deck);
		flip_card(tmp, Facedown);

		/* fix up stack */
		if (stack[i]->card_delta != CARD_DELTA)	{
			recompute_list_deltas(stack[i]);
			show_list(stack[i], stack[i]->cards);
		} else	{
			show_list(stack[i], last_card(stack[i]));
		}
	}
	deal_number--;
	deck_index += 10;
}

/*
 * pulls suit back down from pile
 */
undo_suit(from, exposed)
int	from;
Bool	exposed;
{
int	i;

	if (exposed)
		flip_card(last_card(stack[from]), Facedown);

	for (i = (NUM_PILES - 1); i >= 0; i--)	{
		if (piles[i]->cards)	{
			move_to_list(piles[i]->cards, stack[from], False);
			break;
		}
	}
}

undo_normal(from, dest, num_cards, exposed)
int	from, dest, num_cards;
Bool	exposed;
{
CardPtr	tmp;

	if (exposed)
		flip_card(last_card(stack[from]), Facedown);
	tmp = last_card(stack[dest]);

	/* get start of run to move */
	while (--num_cards)	{
		tmp = tmp->prev;
	}

	move_to_list(tmp, stack[from], False);
}

/* file I/O */

/*
 * this was teken from the NeWS version of spider, which copied
 * it verbatim from the Mesa version, so they'll all work together.
 * but the results of 2 language translations gets a bit messy...
 */

static int	hash_card();
static int	char_index = 0;
static int	prev_hash = 0;

#define	EOS	0xffff		/* end of seq flag */
#define	FUS	0xeeee		/* flag to say rest of seq is faceup */

static int
hash_card(val)
int	val;
{
int	ret;

	char_index += 5;

	ret = ((val ^ prev_hash) ^ (char_index % 4) + (char_index * 4)) & 077;

	return (ret);
}

static	int*
read_sequence(str)
char	*str;
{
int	i, len;
int	*seq, *seqp;
int	val;

	len = strlen(str);
	/* need number of cards plus EOS and FUS */
	seqp = seq = (int *)calloc((unsigned)(len + 2), sizeof(int));
	if (strchr(str, '/') == NULL)	{	/* if no '/', all are faceup */
		*seqp++ = FUS;
	}
	for (i = 0; i < len; i++)	{
		if (str[i] == '/')	{	/* flag '/' */
			*seqp++ = FUS;
			continue;
		}
		if (!(str[i] >= '0' && str[i] <= 'o'))	{
			return NULL;
		}
		val = (int) (str[i] - '0');
		val = hash_card(val);
		prev_hash = val;
		*seqp++ = ((13 - val/4) + ((3 - val % 4) * 13)) % 52;
	}
	*seqp = EOS;
	return (seq);
}

static void
write_sequence(fp, seq)
FILE	*fp;
int	*seq;
{
int	*seqp;
char	c;
int	val;

	seqp = seq;
	while (*seqp != EOS)	{
		if (*seqp == FUS)	{
			(void)fputc('/', fp);
			seqp++;
			continue;
		}
		val = *seqp;
		val = (3 - val/13) + ((13 - val % 13) * 4);
		c = (char) hash_card(val) + '0';
		prev_hash = val;
		assert (c >= '0' && c <= 'o');
		/* depend on file buffering to make this efficient */
		(void)fputc(c, fp);	
		seqp++;
	}
}

Rank	flip_ranks[NUM_RANKS] =	{
	King, Queen, Jack, Ten, Nine, Eight, Seven, 
	Six, Five, Four, Three, Deuce, Ace
};

static void
int_to_card(val, suit, rank)
int	val;
Suit	*suit;
Rank	*rank;
{
	*suit = val / 13;
	*rank = val % 13;
	*rank = flip_ranks[*rank];
	assert (*suit >= Spade && *suit <= Club);
	assert (*rank >= Ace && *suit <= King);
}

static int
card_to_int(card)
CardPtr	card;
{
int	val;

	val = card->suit * 13 + flip_ranks[card->rank];
	return (val);
}

static CardPtr
find_card(cache, suit, rank)
CardPtr	*cache;
Suit	suit;
Rank	rank;
{
int	i;
CardPtr	tmp = CARDNULL;

	for (i = 0; i < NUM_CARDS; i++)	{
		if (cache[i] == CARDNULL)
			continue;
		else if ((cache[i]->suit == suit) && 
		    (cache[i]->rank == rank))	{
			tmp = cache[i];
			cache[i] = CARDNULL;
			break;
		}
	}
	return (tmp);
}

/*
 * get all the cards back in the deck to recover from something
 * evil in the restore process
 */
static void
recover(cache)
CardPtr	*cache;
{
int	i;

	for (i = 0; i < NUM_CARDS; i++)	{
		if (cache[i])	{
			cache[i]->type = Facedown;
			add_card(cache[i], deck->cards, LOC_END, deck);
		}
	}
}

static int
read_position(str)
char	*str;
{
char	*tmp;
char	*card_str, *cards_left;
int	*seq, *seqp;
CardPtr	card;
CardPtr	cache[NUM_CARDS];
Suit	suit;
Rank	rank;
Type	type;
int	i, num_dealt, num_undealt;
int	dealt[NUM_CARDS], undealt[NUM_CARDS];

	prev_hash = 0;
	char_index = 0;
	deck_index = 0;

	/* stash all cards temporarily */
	remove_all_cards(cache);

	/* deck is everything up to a ' ' */
	tmp = strchr(str, ' ');
	if (tmp == NULL)	{		/* bad data */
		recover(cache);
		return (-1);
	}
	card_str = calloc((unsigned)(tmp - str + 1), 1);
	(void)strncpy(card_str, str, tmp - str);

	/*
	 * first read in the hand.  first come the undealt cards, then
	 * the dealt cards.  the first undealt is the next one that 
	 * will be dealt, and the first dealt card is the first card that 
	 * was dealt
	 */
	seq = read_sequence(card_str);
	if (seq == NULL)	{
		recover(cache);
		return (-1);
	}
	free(card_str);
	seqp = seq;
	num_undealt = 0;
	while (*seqp != FUS)	{	/* stop at dealt cards */
		undealt[num_undealt++] = *seqp;
		int_to_card(*seqp, &suit, &rank);
		card = find_card(cache, suit, rank);
		if (card == CARDNULL)	{
			recover(cache);
			return (-1);
		}
		card->type = Facedown;
		add_card(card, deck->cards, LOC_END, deck);
		seqp++;
	}
	seqp++;

	num_dealt = 0;
	while (*seqp != EOS)	{
		dealt[num_dealt++] = *seqp;
		seqp++;
	}
	assert (num_dealt + num_undealt == NUM_CARDS);

	/* reverse the dealt cards and stick them in cache */
	while (--num_dealt >= 0)	{
		deck_cache[deck_index++] = dealt[num_dealt];
	}
	for (i = 0; i < num_undealt; i++)	{
		deck_cache[deck_index++] = undealt[i];
	}
	assert(deck_index == NUM_CARDS);
	deck_index = num_undealt;
	/* reset deal_number correctly */
	deal_number = 6 - deck_index/10;

	free((char *)seq);

	/*
	 * now deal with the stacks
	 */
	cards_left = tmp;
	cards_left++;		/* skip space */

	/* deal with each stack */
	for (i = 0; i < 10; i++)	{
		stack[i]->card_delta = CARD_DELTA;	/* restore delta */
		type = Facedown;
		tmp = strchr(cards_left, ' ');

		/* deal with '.' at end of string */
		if (tmp == NULL)	{
			tmp = strchr(cards_left, '.');
			if (tmp == NULL)	{		/* bad data */
				recover(cache);
				return (-1);
			}
		}
		if (tmp == cards_left)	{	/* empty col */
			cards_left++;
			continue;
		}

		/* make local copy of stack */
		card_str = calloc((unsigned)((tmp - cards_left) + 1), 1);
		(void)strncpy(card_str, cards_left, tmp - cards_left);
		cards_left = tmp + 1;

		seqp = seq = read_sequence(card_str);
		if (seq == NULL)	{
			recover(cache);
			return (-1);
		}
		while (*seqp != FUS && *seqp != EOS)	{
			int_to_card(*seqp, &suit, &rank);
			card = find_card(cache, suit, rank);
			if (card == CARDNULL)	{
				recover(cache);
				return (-1);
			}
			card->type = type;
			add_card(card, stack[i]->cards, LOC_END, stack[i]);
			seqp++;
		}

		/* handle faceup cards */
		if (*seqp != EOS)	{
			seqp++;		/* skip FUS */
			type = Faceup;
			while (*seqp != EOS)	{
				int_to_card(*seqp, &suit, &rank);
				card = find_card(cache, suit, rank);
				if (card == CARDNULL)	{
					recover(cache);
					return (-1);
				}
				card->type = type;
				add_card(card, stack[i]->cards, LOC_END, 
					stack[i]);
				seqp++;
			}
		}
		free((char *)seq);
		free(card_str);
	}

	/* deal with remaining stuff -- should all be piles */
	if (*tmp != '.')	{
		assert (*tmp == ' ');
		tmp = strchr(cards_left, '.');
		if (tmp == NULL)	{		/* bad data */
			recover(cache);
			return (-1);
		}
		/* make local copy of piles */
		card_str = calloc((unsigned)((tmp - cards_left) + 1), 1);
		(void)strncpy(card_str, cards_left, tmp - cards_left);
		cards_left = tmp + 1;
		type = Faceup;

		seqp = seq = read_sequence(card_str);
		if (seq == NULL)	{
			recover(cache);
			return (-1);
		}
		i = 0;
		while (*seqp != EOS)	{
			if (*seqp == FUS)	{
				seqp++;
				continue;
			}
			int_to_card(*seqp, &suit, &rank);
			assert(rank == Ace);
			for (rank = Ace; rank <= King; rank++)	{
				card = find_card(cache, suit, rank);
				if (card == CARDNULL)	{
					recover(cache);
					return (-1);
				}
				card->type = type;
				add_card(card, piles[i]->cards, LOC_START, 
					piles[i]);
			}
			seqp++;
			i++;
		}
	}

	return (0);
}

static void
read_moves(str)
char	*str;
{
char	*s;
	
	s = str;
	while (*s != '.')	{
		move_cache[move_index++] = (int)(*s - '0') * 64 + 
			(int)(*(s+1) - '0');
		assert (move_cache[move_index - 1] >= 0);
		if (move_index >= cache_size)
			grow_cache();
		s += 2;
	}
}

static void
write_moves(fp)
FILE	*fp;
{
int	i;
int	val;

	for (i = 0; i < move_index; i++)	{
		val = move_cache[i];
		(void)fputc((char)(val/64 + '0'), fp);
		(void)fputc((char)(val%64 + '0'), fp);
	}
}

write_file(fname, confirmer)
char	*fname;
Bool	(*confirmer)();
{
FILE	*fp;
char	buf[512];
int	i, num;
int	seq[1024], *seqp;
CardPtr	tmp;

	fname = remove_newlines(fname);

	if (access(fname, F_OK) == 0)	{
		if ((*confirmer)())	{
			show_message("Overwriting existing file.");
		} else	{
			show_message("Cancelling save.");
			return;
		}
	}

	if ((fp = fopen(fname, "w")) == NULL)	{
		(void)sprintf(buf, "Can't open output file %s.", fname);
		show_message(buf);
		return;
	}

	seqp = seq;

	prev_hash = 0;
	char_index = 0;

	num = NUM_CARDS - deck_index;
	for (i = num; i < NUM_CARDS; i++)	{
		*seqp++ = deck_cache[i];
	}
	*seqp++ = FUS;
	for (i = (num - 1); i >= 0; i--)	{
		*seqp++ = deck_cache[i];
	}

	*seqp = EOS;
	write_sequence(fp, seq);
	seqp = seq;


	for (i = 0; i < NUM_STACKS; i++)	{
		(void)fputc(' ', fp);
		tmp = stack[i]->cards;
		seqp = seq;
		if (tmp)	{
			while (tmp->type == Facedown)	{
				*seqp++ = card_to_int(tmp);
				tmp = tmp->next;
			}
			if (seqp != seq)	{
				*seqp++ = FUS;
			}

			while (tmp)	{
				*seqp++ = card_to_int(tmp);
				tmp = tmp->next;
			}

			if (seqp != seq)	{
				*seqp = EOS;
				write_sequence(fp, seq);
			}
		}
	}

	/* save piles */
	seqp = seq;
	for (i = 0; i < NUM_PILES; i++)	{
		if (piles[i]->cards)	{
			*seqp++ = card_to_int(last_card(piles[i]));
		}
	}
	if (seqp != seq)	{
		*seqp = EOS;
		(void)fputc(' ', fp);
		write_sequence(fp, seq);
	}
	fputs(".\n", fp);

	/* write out the moves */
	write_moves(fp);
	fputs(".\n\n", fp);

	/* write human readable version */
	write_human(fp);
	(void)fclose(fp);
	(void)sprintf(buf, "Position saved to file %s.", fname);
	show_message(buf);
}

write_human(fp)
FILE	*fp;
{
CardPtr	tmps[NUM_STACKS];
int	i;
int	done = 0;
Bool	toprow = True;

	for (i = 0; i < NUM_STACKS; i++)	{
		tmps[i] = stack[i]->cards;
	}

	while (done < NUM_STACKS)	{
		done = 0;
		for (i = 0; i < NUM_STACKS; i++)	{
			if (tmps[i] == CARDNULL)	{
				/* empty stack */
				if (toprow)	{
					fputs("(sp)\t", fp);
				} else	{
					(void)fputc('\t', fp);
				}
				done++;
				continue;
			} else	{
				if (tmps[i]->type == Facedown)	{
					fputs(" --", fp);
				} else	{
					(void)fprintf(fp, "%2s%c", 
					    rnk_name(tmps[i]->rank),
					    tolower(*suit_name(tmps[i]->suit)));
				}
			}
			(void)fputc('\t', fp);
			tmps[i] = tmps[i]->next;
		}
		(void)fputc('\n', fp);
		toprow = False;
	}
}

read_selection(buf)
char	*buf;
{
char	*moves;

	buf = remove_newlines(buf);
	moves = strchr(buf, '.') + 1;	/* find moves, if any */
	if (!strchr(moves, '.'))
		moves = NULL;
	if (restore_game(buf, moves) != 0)	{
		show_message("Bogus data in selection.");
		restart = True;
	} else	{
		show_message("Position loaded from selection.");
	}
	free(buf);
}

read_file_or_selection(fname)
char	*fname;
{
	if (access(fname, R_OK) == -1)	{
		read_selection(fname);
	} else	{
		read_file(fname);
	}
}

read_file(fname)
char	*fname;
{
FILE	*fp;
char	buf[1024], buf2[1024];
char	*dp;

	fname = remove_newlines(fname);
	if ((fp = fopen(fname, "r")) == NULL)	{
		(void)sprintf(buf, "Can't open file %s for loading.", fname);
		show_message(buf);
		return;
	}

	/* read card string */
	dp = buf;
	while ((*dp = (char)fgetc(fp)) != EOF)	{
		if (*dp == '\n')	/* ignore any CR */
			continue;
		if (*dp == '.')
			break;
		dp++;
	}
	*++dp = '\0';

	/* read moves string */
	dp = buf2;
	while ((*dp = (char)fgetc(fp)) != EOF)	{
		if (*dp == '\n')	/* ignore any CR */
			continue;
		if (*dp == '.')
			break;
		dp++;
	}
	*++dp = '\0';

	if (restore_game(buf, buf2) == 0)	{
		(void)sprintf(buf, "Saved game %s loaded.", fname);
	} else	{
		(void)sprintf(buf, "Bogus save file %s.", fname);
		restart = True;
	}

	show_message(buf);
}

static int
restore_game(str, str2)
char	*str, *str2;
{

	if (read_position(str) != 0)	{
		return (-1);
	}

	init_cache();		/* clear out the move cache */

	if (str2)
		read_moves(str2);

	return (0);
}

/*
 * play the same deck again
 */
int
replay()
{
CardPtr	cache[NUM_CARDS];
CardPtr	card;
int	i;
Rank	rank;
Suit	suit;

	remove_all_cards(cache);

	/* reset card spacing */
	for (i = 0; i < NUM_STACKS; i++)	{
		stack[i]->card_delta = CARD_DELTA;
	}

	for (i = 0; i < NUM_CARDS; i++)	{
		int_to_card(deck_cache[i], &suit, &rank);
		card = find_card(cache, suit, rank);
		if (card == CARDNULL)	{
			show_message("Old deck is corrupted -- can't replay.");
			recover(cache);
			return -1;
		}
		card->type = Facedown;
		add_card(card, deck->cards, LOC_END, deck);
	}
	/* wipe the old stuff */
	XClearArea(dpy, table, 0, 0, table_width, table_height, False);

	/* force piles and deck to redraw -- stacks will paint in deal */
	XClearArea(dpy, table, 0, 0, table_width, DECK_Y + CARD_HEIGHT, True);

	deck_index = NUM_CARDS;
	deal_number = 0;
	deal_cards();

	return 0;
}


/*
 * replay removing a suit
 */
static void
show_suit(from)
int	from;
{
CardPtr	tmp;
int	i;

	tmp = last_card(stack[from]);
	assert (tmp->rank == Ace);
	while (tmp && tmp->rank != King)	{
		tmp = tmp->prev;
	}
	assert (tmp->rank == King);

	for (i = 0; i < NUM_PILES; i++)	{
		if (piles[i]->cards == CARDNULL)	{
			break;
		}
	}
	assert (i < NUM_PILES);

	move_to_list(tmp, piles[i], False);
}

/*
 * replay a normal move
 */
static void
show_normal(from, dest, num)
int	from, dest, num;
{
CardPtr	tmp;

	tmp = last_card(stack[from]);

	while (--num)	{
		tmp = tmp->prev;
	}

	move_to_list(tmp, stack[dest], False);
}

#ifdef XVIEW
get_move_index() 
{
        return(move_index);
}
 
Bool
show_n_moves(place, number)
int place, number;
{
int     val;
int     from, dest, num, exposed;
int     i;

        for (i=0; i < number; i++) {
                if ((i+place) > move_index) 
                        return (False);
                val = move_cache[place + i];
                unencode(val, &from, &dest, &num, &exposed);

                if (from == 0)  {
                        deal_next_hand(False);
                } else  {
                        if (dest == 0)  {
                                show_suit(from - 1);
                        } else  {
                                show_normal(from - 1, dest - 1, num);
                        }
                }
        }
        return (True);
}
#else

/*
 * show the moves made so far
 *
 * start is the position into the movelog, num is the number of moves
 * (0 specifies all)
 * event_check is the routine that checks for an abort and handles
 * damage, etc.  delay_func pauses as necessary
 */
void
show_play(start, num_moves, event_check, delay_func)
int	start, num_moves;
Bool	(*event_check)();
void	(*delay_func)();
{
int	i;
int	val;
int	from, dest, num, exposed;
Bool	aborted = False;

	if (replay() == -1)
		return;

	/* handle any events from reseting */
	(void) show_play_events();

	show_message("Showing all moves -- hit any key or button to abort");

	if (num_moves == 0)	/* show all case */
		num_moves = move_index;

	for (i = start; (i < move_index) && (i < (start + num_moves)); i++) {
		val = move_cache[i];
		unencode(val, &from, &dest, &num, &exposed);

		if (from == 0)	{
			deal_next_hand(False);
		} else	{
			if (dest == 0)	{
				show_suit(from - 1);
			} else	{
				show_normal(from - 1, dest - 1, num);
			}
		}
		if ((*event_check)() == False)	{
			aborted = True;
			/* restore to a known state */
			(void) replay();
			break;
		}
		(*delay_func)();
	}
	if (aborted)	{
		show_message("Aborted -- cleaning up");
	} else	{
		show_message("Replay finished");
	}
}
#endif
