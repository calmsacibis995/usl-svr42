/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r4spider:events.c	1.1"
/*
 *	Spider
 *
 *	(c) Copyright 1989, Donald R. Woods and Sun Microsystems, Inc.
 *	(c) Copyright 1990, David Lemke and Network Computing Devices Inc.
 *
 *	See copyright.h for the terms of the copyright.
 *
 *	@(#)events.c	2.1	90/04/25
 *
 */

/*
 * Spider event handlers
 */

#include	"defs.h"
#include	"globals.h"

static	CardPtr		current_card = CARDNULL;
static	CardList	current_list = CARDLISTNULL;

static CardList		coords_to_list();
static CardPtr		coords_to_card();
static Bool		hit_deck();
Bool			write_confirmer();
Bool			newgame_confirmer();
#ifdef XVIEW
extern void		show_play();
#else
extern Bool		show_play_events();
#endif
#ifdef	KITLESS
static void		handle_table_event();
static void		redraw_table();
static void		button_press();
static void		button_release();
static void		key_press();
static void		resize_event();
static void		do_expand();
static void		handle_message_event();
#endif /* KITLESS */

Bool	usebell = False;

#ifdef KITLESS
/*
 * main event handling loop
 */
event_loop()
{
XEvent	xev;

	while (1)	{
		XNextEvent(dpy, &xev);
		if (xev.xany.window == table)	{
			handle_table_event(&xev);
		} else if (xev.xany.window == message_win)	{
			handle_message_event(&xev);
		}
		if (restart)	{
			shuffle_cards();
		}
	}
}

/*
 * event on table
 */
static void
handle_table_event(xev)
XEvent	*xev;
{
	if (xev->xany.type == Expose)	{
		redraw_table((XExposeEvent *)xev);
	} else if (xev->xany.type == ButtonPress)	{
		button_press((XButtonPressedEvent *)xev);
	} else if (xev->xany.type == ButtonRelease)	{
		button_release((XButtonReleasedEvent *)xev);
	} else if (xev->xany.type == KeyPress)	{
		key_press((XKeyPressedEvent *)xev);
	} else if (xev->xany.type == ConfigureNotify)	{
		resize_event((XConfigureEvent *)xev);
	}
}

static void
handle_message_event(xev)
XEvent	*xev;
{
	if (xev->xany.type == Expose)	{
		show_message(NULL);
	}
}

/* KITLESS doesn't bother to check -- just clobbers existing file */
Bool
write_confirmer()
{
	return True;
}

Bool
newgame_confirmer()
{
	return True;
}
#endif /* KITLESS */


#ifndef KITLESS
void
#else /* KITLESS */
static void
#endif /* KITLESS */
redraw_table(xev)
XExposeEvent	*xev;
{
static Bool	last_was_zero = True;	/* so at init time it paints */

	/*
	 * this mess is to optimize the painting.   a complex exposure
	 * area could cause a card to have several damage areas.  since
	 * the entire card is painted when damaged, the following keeps
	 * track of whether its been painted in the latest flurry of
	 * exposures, and prevents it being painted multiple times.
	 */
	if (last_was_zero)	{
		draw_count++;
		last_was_zero = False;
	}
	if (xev->count == 0)
		last_was_zero = True;

	redraw_deck(xev->x, xev->y, xev->width, xev->height);
	redraw_card_piles(xev->x, xev->y, xev->width, xev->height);
	redraw_card_stacks(xev->x, xev->y, xev->width, xev->height);
}

/* 
 * ignore y when getting list
 */
static CardList
coords_to_list(x, y)
int	x, y;
{
int	i;

	if (y < STACK_LOC_Y)	{
		for (i = 0; i < NUM_PILES; i++)	{
			if ((x >= PILE_LOC_X(piles[i]->place)) &&
			    (x <= (PILE_LOC_X(piles[i]->place) + CARD_WIDTH)))
				return (piles[i]);
		}
		return (CARDLISTNULL);
	} else	{
		for (i = 0; i < NUM_STACKS; i++)	{
			if ((x >= STACK_LOC_X(stack[i]->place)) &&
			    (x <= (STACK_LOC_X(stack[i]->place) + CARD_WIDTH)))
				return (stack[i]);
		}
		return (CARDLISTNULL);
	}
}

static CardPtr
coords_to_card(x, y)
int	x, y;
{
CardList	list;
CardPtr	tmp;

	list = coords_to_list(x, y);
	if (list == CARDLISTNULL || IS_PILE(list))
		return (CARDNULL);
	tmp = list->cards;
	if (tmp == CARDNULL)
		return CARDNULL;
	while (tmp)	{
		if (tmp->next)	{
			if ((y <= (tmp->y + list->card_delta)) &&
			    (y >= tmp->y))	{
				return (tmp);
			}
		} else	{
			if ((y <= (tmp->y + CARD_HEIGHT)) &&
			    (y >= tmp->y))	{
				return (tmp);
			}
		}
		tmp = tmp->next;
	}
	return CARDNULL;
}

static Bool
hit_deck(x, y)
int	x, y;
{
	return ((x <= (deck->x + CARD_WIDTH)) && (x >= deck->x) &&
	    (y <= (deck->y + CARD_HEIGHT)) && (y >= deck->y));
}

#ifndef KITLESS
void
#else	/* KITLESS */
static void
#endif	/* KITLESS */
button_press(xev)
XButtonPressedEvent	*xev;
{

	if (hit_deck(xev->x, xev->y))	{
		current_list = deck;
		return;
	}
	if (xev->button == Button2)	{
		current_card = coords_to_card(xev->x, xev->y);
		if (current_card == CARDNULL)
			return;
#ifdef DEBUG
		if (xev->state & ShiftMask)	{
			current_list = coords_to_list(xev->x, xev->y);
			return;
		}
#endif	/* DEBUG */
		if (!can_move(current_card))	{
			card_message("Can't move", current_card);
			spider_bell(dpy, 0);
			current_card = CARDNULL;
		}
	} else	{
		current_card = CARDNULL;
	}
	current_list = coords_to_list(xev->x, xev->y);
	if (IS_PILE(current_list))	{
		if (current_list->cards)	{
			show_message("Can't move removed cards.");
			spider_bell(dpy, 0);
		} else	{
			show_full_suits();
		}
		current_list = CARDLISTNULL;
	} else if (current_list && current_list->cards == CARDNULL)	{
		show_message("No cards to move.");
		current_list = CARDLISTNULL;
	}
}

#ifndef KITLESS
void
#else	/* KITLESS */
static void
#endif 	/* KITLESS */
button_release(xev)
XButtonReleasedEvent	*xev;
{
CardList	list_hit;
CardPtr		tmp;

	if (current_list == CARDLISTNULL)
		return;

	if (hit_deck(xev->x, xev->y))	{
		if (current_list == deck)	{
			if (deal_number == 0)	{
				deal_cards();
			} else	{
				deal_next_hand(True);
			}
		} else	{	/* no dropping on deck */
			show_message("Can't move cards to deck");
			spider_bell(dpy, 0);
		}
		return;
	}

	list_hit = coords_to_list(xev->x, xev->y);
	if (list_hit == CARDLISTNULL)
		return;

	if (current_card)	{
#ifdef DEBUG
		if (xev->state & ShiftMask)	{
			move_to_list(current_card, list_hit, True);
			current_card = CARDNULL;
			return;
		}
#endif
		if ((IS_PILE(list_hit)))	{
			if((current_card->rank == King) && 
			    (can_move(current_card)) &&
			    (last_card(current_card->list)->rank == Ace)) {
				move_to_pile(current_card);
			} else	{
				card_message("Can't remove", current_card);
				spider_bell(dpy, 0);
			}
			current_card = CARDNULL;
			return;
		}
		if (can_move_to(current_card, list_hit))	{
			move_to_list(current_card, list_hit, True);
		} else	{
			card2_message("Can't move", current_card, "to",
				last_card(list_hit));
			spider_bell(dpy, 0);
		}
		current_card = CARDNULL;
	/* try best move if the mouse wasn't moved */
	} else if (list_hit == current_list)	{
		best_list_move(list_hit);
	} else	{
		if (IS_PILE(list_hit))	{
			tmp = last_card(current_list);
			if (tmp->rank == Ace)	{
				while (tmp && can_move(tmp))	{
					if (tmp->rank == King)	{
						move_to_pile(tmp);
						return;
					}
					tmp = tmp->prev;
				}
			}
			card_message("Can't remove", tmp);
			spider_bell(dpy, 0);
			return;
		}
		tmp = current_list->cards;
		while (tmp)	{
			if (can_move(tmp))	{
				if (can_move_to(tmp, list_hit))	{
					move_to_list(tmp, list_hit, True);
					return;
				}
				/* only try to move the first possible move */
				break;
			}
			tmp = tmp->next;
		}
		if (tmp)
			card_message("Can't move", tmp);
		else
			show_message("No movable cards");
		spider_bell(dpy, 0);
	}
}

#ifndef KITLESS
void
#else	/* KITLESS */
static void
#endif	/* KITLESS */
key_press(xev)
XKeyPressedEvent	*xev;
{
char	str[32];
char	buf[512];
char	*fname;
int	num;
#ifdef XVIEW
extern char *get_selection();
#endif /* XVIEW */

	num = XLookupString(xev, str, 32, NULL, NULL);
	if (num == 0)
		return;
	switch (str[0])	{
	case	'f':		/* find card */
	case	'F':
		if ((fname = get_selection()) == NULL)	{
			show_message("Selection is unusable or unobtainable.");
		} else	{
			locate(fname);
		}
		break;
	case	'l':
	case	'L':
		if ((fname = get_selection()) == NULL)	{
			show_message("Selection is unusable or unobtainable.");
		} else	{
			read_file_or_selection(fname);
		}
		/* force everything to redraw */
		force_redraw();
		break;
	case	'w':
	case	'W':
		/* write to selection */
		if ((fname = get_selection()) == NULL)	{
			show_message("Selection is unusable or unobtainable.");
		} else	{
			write_file(fname, write_confirmer);
		}
		break;
	case	's':
	case	'S':
		/* score */
		(void)sprintf(buf, "Current position scores %d out of 1000.", 
			compute_score());
		show_message(buf);
		break;
	case	'a':
	case	'A':
		/* play again */
		(void)replay();
		init_cache();	/* reset move cache */
		break;
	case	'r':
	case	'R':
		/* show move log */
#ifdef XVIEW
		show_play();
#else
		show_play(0, 0, show_play_events, delay);
#endif
		break;
	case	'n':
	case	'N':
		/* start over */
		if (newgame_confirmer())	{
			restart = True;
			clear_message();
		}
		break;
	case	'e':
	case	'E':
		do_expand();
		break;
	case	'd':
	case	'D':
		if (deal_number == 0)	{
			deal_cards();
		} else	{
			deal_next_hand(True);
		}
		/* deal next hand */
		break;
	case	'u':
	case	'U':
		undo();
		break;
	case	'v':
	case	'V':
		print_version();
		break;
	case	'Q':
		/* quit */
		exit(0);
	case	'#':
		if (deal_number == 0)	{
			show_message("Haven't dealt yet.");
		} else if (deal_number == 1)	{
			show_message("Initial deal.");
		} else	{
			sprintf(buf, "Deal number %d of 5", deal_number - 1);
			show_message(buf);
		}
		break;
	case	'?':
		if (deal_number == 0)	{
			show_message("Haven't dealt yet.");
		} else	{
			advise_best_move();
		}
		break;
	default:
		str[num] = '\0';	/* NULL terminate it */
		(void) sprintf(buf, "Unknown command: '%s'", str);
		show_message(buf);
		break;
	}
}

#ifdef KITLESS
static void
resize_event(xev)
XConfigureEvent	*xev;
{
int	i;

	table_height = xev->height;
	table_width = xev->width;

	/* adjust message window */
	XMoveResizeWindow(dpy, message_win, 0, (table_height - 2 * TABLE_BW - 
		(message_font->ascent + message_font->descent)),
		(table_width - 2 * TABLE_BW),
		(message_font->ascent + message_font->descent));

	/* fix stacks */
	for (i = 0; i < NUM_STACKS; i++)	{
		if (stack[i])
			recompute_list_deltas(stack[i]);
	}
	/* exposure will repaint them */
}
#endif	/* KITLESS */

void
print_version()
{
char	buf[256];

	(void)sprintf(buf, "Spider version %s, last built: %s", version,
							build_date);
	show_message(buf);
}


static CardList
get_list()
{
XEvent	event;
CardList	list = CARDLISTNULL;

	if (XGrabPointer(dpy, table, False, 
			ButtonPressMask | ButtonReleaseMask,
			GrabModeAsync, GrabModeAsync, table, None,
			CurrentTime) != GrabSuccess)	{
		show_message("Unable to grab pointer.");
		return CARDLISTNULL;
	}
	while (1)	{
		XNextEvent(dpy, &event);

		switch (event.type)	{
		case	ButtonRelease:
			if (event.xbutton.window == table)	{
				list = coords_to_list(event.xbutton.x,
					event.xbutton.y);
			}
			XUngrabPointer(dpy, CurrentTime);
			return list;
		default:
			break;
		}
	}
}

#ifdef KITLESS
static void
#else	/* KITLESS */
void
#endif	/* KITLESS */
do_expand()
{
CardList	list;

	show_message("Click over the column whose contents you want to see.");
#ifdef	XAW
	flush_message();
#endif	/* XAW */
	list = get_list();
	if (list && !IS_PILE(list))	{
		expand(list);
	} else	{
		show_message("That wasn't over a column!");
	}
}

#ifdef KITLESS
Bool
show_play_events()
{
XEvent	xev;

	while (XPending(dpy))	{
		XNextEvent(dpy, &xev);

		/* any key or button will stop it */
		switch(xev.type)	{
			default:
				if (xev.xany.window == table)   {
					handle_table_event(&xev);
				} else if (xev.xany.window == message_win) {
					handle_message_event(&xev);
				}
				break;

			case	KeyPress:
			case	KeyRelease:
			case	ButtonPress:
			case	ButtonRelease:
				return False;
		}
	}
	return True;
}
#endif	/* KITLESS */

spider_bell(d, level)
Display	*d;
int	level;
{
	if (usebell)
		XBell(d, level);
}
