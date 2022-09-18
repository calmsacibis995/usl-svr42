#pragma ident	"@(#)train:scores.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include	<stdio.h>
#include	<X11/Xlib.h>
#include	<X11/Xutil.h>
#include	<X11/Xatom.h>
#include	<X11/cursorfont.h>
#include	<Xos.h>
#include	<X11/keysym.h>
#include	<setjmp.h>
#include	"train.h"

static int	passenger_area_x;
static int	passenger_area_y;
static int	passenger_area_width;
static int	scored_passenger_start_x;
static int	scored_passenger_x;
static int	score;
static int	score_x;
static int	score_y;
static int	score_width;
static int	time_x;
static int	time_y;
static int	time_width;
static XFontStruct *score_font;

extern int	train_len[];
extern int	level;
extern int	num_active_trains;
extern Window	train_windows [NUM_TRAINS] [MAX_TRAIN_LEN];
extern jmp_buf	env;
extern Window	window2, score_window;
extern GC	pas_GC;
extern Display	*dpy;
extern int	the_screen;
extern XColor	my_red, my_black;
extern int	t_pause;
extern Pixmap	pas1_pixmap;
extern int	time_count;
extern int	passenger_train;
extern int	passenger_src;
extern int	demo_mode, monochrome_mode;

InitializeFont()
{
	score_font = XLoadQueryFont(dpy, SCORE_FONT);

	if (!score_font)
	{
		fprintf(stderr, "Can't load font %s, using 'fixed'\n",
								SCORE_FONT);
		score_font = XLoadQueryFont(dpy, "fixed");
	}
}
/*
**	Check for high score
*/

CheckHighScore(score, level)
int score;
int level;
{
	XEvent	ev;
	KeySym	sym;
	Scores	high_score[5];
	FILE	*fp;
	int	i,
		j,
		k,
		top_font_x,
		top_font_y,
		middle_font_x,
		middle_font_y,
		bottom_font_x,
		bottom_font_y;
	char	buf[50],
		a_char;

	XClearWindow(dpy, window2);
	XSetFont(dpy, pas_GC, score_font->fid);
	XSetForeground(dpy, pas_GC, WhitePixel(dpy, the_screen));
	XSetFillStyle(dpy, pas_GC, FillSolid);

	if ((fp = fopen(SCORE_FILE, "r")) == NULL)
	{
		fp = fopen(SCORE_FILE, "w");
		if (fp == NULL) {
			printf ("Cannot open score file %s\n",SCORE_FILE);
			return;
		}
		for (i = 0; i < 5; i++)
			fprintf(fp, "%.3s %d %d\n", "---", 0, 0);
		fclose(fp);
		fopen(SCORE_FILE, "r");
	}

	for (i=0; i<5; i++)
	{
		fscanf(fp, "%s %d %d", high_score[i].name,
					&(high_score[i].score),
					&(high_score[i].level));
	}
	fclose(fp);


	middle_font_x = (TILE_SIZE * GRID_SIZE - 
			XTextWidth(score_font, "Enter 3 Initials:", 17))/ 2;
	middle_font_y = (TILE_SIZE * GRID_SIZE + 
			score_font->max_bounds.ascent) / 2;
	top_font_x = (TILE_SIZE * GRID_SIZE - 
			XTextWidth(score_font, "HIGH SCORE!", 11))/ 2;
	top_font_y = middle_font_y -
			(score_font->max_bounds.ascent +
			score_font->max_bounds.descent) - 5;
	bottom_font_x = (TILE_SIZE * GRID_SIZE - 
			XTextWidth(score_font, "WWW", 3))/ 2;
	bottom_font_y = middle_font_y +
			score_font->max_bounds.ascent +
			score_font->max_bounds.descent +
			TRAIN_HEIGHT + 5;

	if (score > high_score[4].score)
	{
		XSelectInput(dpy, window2, KeyPressMask);
		XSetInputFocus(dpy, window2, RevertToParent, CurrentTime);

		strcpy(buf, "HIGH SCORE!");
		XDrawString(dpy, window2, pas_GC, top_font_x, top_font_y, buf,
								strlen(buf));
		strcpy(buf, "Enter 3 Initials:");
		XDrawString(dpy, window2, pas_GC, middle_font_x, middle_font_y,
							buf, strlen(buf));
		for (i = 0; i < 3; i++)
		{
			XMaskEvent(dpy, KeyPressMask, &ev);
			XLookupString((XKeyPressedEvent *) (&ev), &a_char, 1,
								&sym, NULL);
			if (!IsModifierKey(sym))
			{
				high_score[4].name[i] = a_char;
				XDrawString(dpy, window2, pas_GC, bottom_font_x,
						bottom_font_y,
						&(high_score[4].name[i]), 1);
				bottom_font_x += score_font->max_bounds.width;
			}
			else
				--i;
		}

		high_score[4].score = score;
		high_score[4].level = level;
		bsort(high_score, 5);

		fp = fopen(SCORE_FILE, "w");
		for (i = 0; i < 5; i++) {
			fprintf(fp, "%.3s %d %d\n", high_score[i].name,
							high_score[i].score,
							high_score[i].level);
		}
		fclose(fp);
	}

	XClearWindow(dpy, window2);
	XSetFont(dpy, pas_GC, score_font->fid);
	XSetForeground(dpy, pas_GC, WhitePixel(dpy, the_screen));
	XSetFillStyle(dpy, pas_GC, FillSolid);

	for (i = 0; i < 5; ++i)
	{
		sprintf(buf, "%s %05d %d", high_score[i].name,
						high_score[i].score,
						high_score[i].level);
		XDrawString(dpy, window2, pas_GC, top_font_x,
					top_font_y, buf, strlen(buf));
		top_font_y += score_font->max_bounds.ascent + 5;
	}

	XSync(dpy, True);

	Spin(5000);

	XDrawString(	dpy, window2, pas_GC, 
			(TILE_SIZE * GRID_SIZE - 
				XTextWidth(score_font, "Click mouse to start", 20))/ 2,
			(TILE_SIZE * GRID_SIZE - (2 *
				(score_font->max_bounds.ascent +
				score_font->max_bounds.descent))),
			"Click mouse to start", 20);

	XSync(dpy, True);

	if (demo_mode == True) {
		Spin(4000);
		return;
	}

	else {

		XMaskEvent(	dpy,
				ButtonPressMask,
				&ev);
	}

	XSetFillStyle(dpy, pas_GC, FillStippled);
	XClearWindow(dpy, window2);
}


bsort(hi, num)
Scores hi[];
int num;
{
	int	tmp;
	char	ctmp[3];

	for (num = 4; num > 0; num--)
	{
		if (hi[num].score > hi[num - 1].score)
		{
			tmp = hi[num - 1].score;
			hi[num - 1].score = hi[num].score;
			hi[num].score = tmp;
			tmp = hi[num - 1].level;
			hi[num - 1].level = hi[num].level;
			hi[num].level = tmp;
			strcpy(ctmp, hi[num - 1].name);
			strcpy(hi[num - 1].name, hi[num].name);
			strcpy(hi[num].name, ctmp);
		}
	}
}


/*
**	Initialize time and score
*/
InitializeScore()
{
	char	buf[50];

	score = 0;
	time_count = 0;

	InitializeFont();

	score_x = 5;
	score_y = score_font->max_bounds.ascent + 5;
	score_width = XTextWidth(score_font, "00000", 5);
	time_x = TILE_SIZE * (GRID_SIZE - 1) -
					XTextWidth(score_font, "00", 2);
	time_y = score_font->max_bounds.ascent + 5;
	time_width = XTextWidth(score_font, "00", 2);
	passenger_area_x = score_x + score_width + 5;
	passenger_area_y = (PANEL_HEIGHT - pas1_height) / 2;
	passenger_area_width = time_x - passenger_area_x - 5;
	scored_passenger_start_x = passenger_area_x + ((passenger_area_width -
				(PASSENGERS_FOR_NEXT_LEVEL * pas1_width)) / 2);
	scored_passenger_x = scored_passenger_start_x;

	sprintf(buf, "%05d", score);
	XSetFont(dpy, pas_GC, score_font->fid);
	XSetForeground(dpy, pas_GC, WhitePixel(dpy,the_screen));
	XSetFillStyle(dpy, pas_GC, FillSolid);
	XDrawString(dpy, score_window, pas_GC, score_x, score_y, buf,
								strlen(buf));
	sprintf(buf, "%02d", time_count);
	XDrawString(dpy, score_window, pas_GC, time_x, time_y, buf,
								strlen(buf));
	XSetFillStyle(dpy, pas_GC, FillStippled);
}


/*
**	Decrement time counter
*/
DecrementTime()
{
	char	buf[50];
	int	num, foo;

	--time_count;

	if (time_count < 0)
	{
		UndrawPassenger();
		num = passenger_train;
		if (num == -1) num = 0;
		RemoveCarFromTrain(num);

		for (foo = 0; foo < train_len[num]; foo++) {
			XSetWindowBackground(
					dpy,
					train_windows[num][foo],
					my_black.pixel);
			XClearWindow(
					dpy,
					train_windows[num][foo]);
		}

		passenger_src = PASSENGER_NONE;
		TryToCreateRandomPassenger();
		return;
	}
	else if (time_count == 10 && demo_mode == False)
		XBell(dpy,100);
	else if (time_count < 5 && demo_mode == False)
		XBell(dpy,100);

	XClearArea(dpy, score_window, time_x, 0, time_width, PANEL_HEIGHT,
									False);
	sprintf(buf, "%02d", time_count);
	XSetFont(dpy, pas_GC, score_font->fid);
	XSetForeground(dpy, pas_GC, WhitePixel(dpy,the_screen));
	XSetFillStyle(dpy, pas_GC, FillSolid);
	XDrawString(dpy, score_window, pas_GC, time_x, time_y, buf,
								strlen(buf));
	XSetFillStyle(dpy, pas_GC, FillStippled);
}


/*
**	Update score
*/
UpdateScore(count)
int count;
{
	char		buf[50];
	int		i;

	if (count < 0)
		return;

	score += count;
	XClearArea(dpy, score_window, score_x, 0, score_width, PANEL_HEIGHT,
									False);
	sprintf(buf, "%05d", score);
	XSetFont(dpy, pas_GC, score_font->fid);
	XSetForeground(dpy, pas_GC, WhitePixel(dpy,the_screen));
	XSetFillStyle(dpy, pas_GC, FillSolid);
	XDrawString(dpy, score_window, pas_GC, score_x, score_y, buf,
								strlen(buf));

	if (((score - count) / BONUS_CAR) < (score / BONUS_CAR))
	{
		for (i = 0; i < num_active_trains; i++) {
			AddCarToTrain(i);
		}
	}

	XSetFillStyle(dpy, pas_GC, FillStippled);
}

/*
**	Display a new passenger in score area
*/

AddToPassengerArea()
{
	XSetStipple(dpy, pas_GC, pas1_pixmap);
	XSetTSOrigin(dpy, pas_GC, scored_passenger_x, passenger_area_y);
	XSetForeground(dpy, pas_GC, WhitePixel(dpy, the_screen));
	XFillRectangle(dpy, score_window, pas_GC, scored_passenger_x,
				passenger_area_y, pas1_width, pas1_height);

	scored_passenger_x += pas1_width;
}

/*
**	Display level message
*/

DisplayLevel(level)
int level;
{
	XEvent	ev;
	char	buf[50];

	DisplayBonus(level);

	switch (level) {
	case 10:
	case 20:
	case 30:
	case 40:
		num_active_trains++;
		t_pause = ORIG_PAUSE;
		break;
	}

		

	XClearArea(dpy, score_window, passenger_area_x, passenger_area_y,
				passenger_area_width, pas1_height, False);
	scored_passenger_x = scored_passenger_start_x;

	sprintf(buf, "Level %d", level);

	XSetForeground(dpy, pas_GC, WhitePixel(dpy,the_screen));
	XSetFillStyle(dpy, pas_GC, FillSolid);
	XDrawString(	dpy, window2, pas_GC, 
			(TILE_SIZE * GRID_SIZE - 
				XTextWidth(score_font, buf, strlen(buf)))/ 2,
			(TILE_SIZE * GRID_SIZE + 
				score_font->max_bounds.ascent) / 2,
			buf, strlen(buf));

	XDrawString(	dpy, window2, pas_GC, 
			(TILE_SIZE * GRID_SIZE - 
				XTextWidth(score_font, "Click mouse to start", 20))/ 2,
			(TILE_SIZE * GRID_SIZE - 
				( score_font->max_bounds.ascent +
				score_font->max_bounds.descent) * 2),
			"Click mouse to start", 20);


	XSync(dpy, True);

	if (demo_mode == True) {
		Spin(4000);
	}

	else {

		XMaskEvent(	dpy,
				ButtonPressMask,
				&ev);
		XPutBackEvent (dpy, &ev);
	}

	XClearWindow(dpy,window2);
	XSetFillStyle(dpy, pas_GC, FillStippled);
}

/*
**	Display bonus message
*/
DisplayBonus(level)
int level;
{
	char	buf[50];
	int	i,j,
		top_font_x,
		top_font_y,
		middle_font_x,
		middle_font_y,
		bottom_font_x,
		bottom_font_y,
		train_x,
		train_y,
		bonus = 0;

	middle_font_x = (TILE_SIZE * GRID_SIZE - 
				XTextWidth(score_font, "x 00", 4))/ 2;
	middle_font_y = (TILE_SIZE * GRID_SIZE + 
				score_font->max_bounds.ascent) / 2;
	top_font_x = (TILE_SIZE * GRID_SIZE - 
				XTextWidth(score_font, "BONUS", 5))/ 2;
	top_font_y = middle_font_y -
				(score_font->max_bounds.ascent +
				score_font->max_bounds.descent) - 5;
	bottom_font_x = (TILE_SIZE * GRID_SIZE - 
				XTextWidth(score_font, "00", 2))/ 2;
	bottom_font_y = middle_font_y +
				score_font->max_bounds.ascent +
				score_font->max_bounds.descent +
				TRAIN_HEIGHT + 5;
	train_x = (TILE_SIZE * GRID_SIZE - (TRAIN_WIDTH + 5) * train_len[0])/2;
	train_y = middle_font_y + score_font->max_bounds.descent;

	sprintf(buf, "x %d", BONUS);

	XSetFont(dpy, pas_GC, score_font->fid);
	XSetForeground(dpy, pas_GC, WhitePixel(dpy,the_screen));
	XSetFillStyle(dpy, pas_GC, FillSolid);

	for (i = 0; i < train_len[0]; i++) {
		XMoveWindow(dpy, train_windows[0][i], train_x, train_y);
		if (!monochrome_mode)
			XSetWindowBackground (dpy, train_windows[0][i], my_red.pixel);
		else 
			XSetWindowBackground (dpy, train_windows[0][i], WhitePixel(dpy,the_screen));
		XMapWindow(dpy, train_windows[0][i]);
		train_x += TRAIN_WIDTH + 5;
	}

	XDrawString(	dpy, window2, pas_GC, 
			top_font_x, top_font_y,
			"BONUS", 5);
	XDrawString(	dpy, window2, pas_GC, 
			middle_font_x, middle_font_y,
			buf, strlen(buf));
	XDrawString(	dpy, window2, pas_GC, 
			bottom_font_x, bottom_font_y,
			"00", 2);

	Spin(4000);
	for (i = 0; i < train_len[0]; i++) {
		Spin(1000);
		XUnmapWindow(dpy, train_windows[0][i]);
		bonus += BONUS;
		XClearArea(dpy, window2, bottom_font_x,
				bottom_font_y - score_font->max_bounds.ascent,
				time_width, PANEL_HEIGHT, False);
		sprintf(buf, "%d", bonus);
		XDrawString(dpy, window2, pas_GC, bottom_font_x, bottom_font_y,
							buf, strlen(buf));
		if (demo_mode == False)
			XBell(dpy, 100);
		XSync(dpy, False);
	}
	Spin(2000);

	UpdateScore(bonus);

	for (i = 0; i < train_len[0]; i++) {
		XMoveWindow(dpy, train_windows[0][i], -1000, -1000);
		XSetWindowBackground (dpy, train_windows[0][i], my_black.pixel);
	}

	XClearWindow(dpy,window2);
	XSetFillStyle(dpy, pas_GC, FillStippled);
}

/*
**	Exit the game
*/

ExitFunc()
{
	XEvent ev;

	XUnmapSubwindows(dpy, window2);
	FreeSubWindows();

	XSetForeground(dpy, pas_GC, WhitePixel(dpy,the_screen));
	XSetFillStyle(dpy, pas_GC, FillSolid);
	XDrawString(	dpy, window2, pas_GC, 
			(TILE_SIZE * GRID_SIZE - 
				XTextWidth(score_font, "Game Over", 9))/ 2,
			(TILE_SIZE * GRID_SIZE + 
				score_font->max_bounds.ascent) / 2,
			"Game Over", 9);

	XSync(dpy, True);
	Spin(5000);

	if (demo_mode != True)
		CheckHighScore(score, level);

	XSetFillStyle(dpy, pas_GC, FillStippled);

	longjmp (env,1);

/*NOTREACHED*/
}

TitleScreen()
{
	XEvent ev;
	int ypos = score_font->max_bounds.ascent * 4;

	XSelectInput(dpy,window2,ExposureMask);
	for(;;)
	{
	ypos = score_font->max_bounds.ascent * 4;
	XClearWindow(dpy,window2);
	XClearWindow(dpy,score_window);
	XSetFont(dpy, pas_GC, score_font->fid);
	XSetForeground(dpy, pas_GC, WhitePixel(dpy,the_screen));
	XSetFillStyle(dpy, pas_GC, FillSolid);
	XDrawString(	dpy, window2, pas_GC, 
			(TILE_SIZE * GRID_SIZE - 
				/* XTextWidth(score_font, "Software Engineer", 17))/ 2, ul92-05857 */
				XTextWidth(score_font, "Train Game", 10))/ 2,
			ypos,
			/* "Software Engineer", 17); ul92-05857 */
			"Train Game", 10);

	ypos += score_font->max_bounds.ascent * 2;
/*
	XDrawString(	dpy, window2, pas_GC, 
			(TILE_SIZE * GRID_SIZE - 
				XTextWidth(score_font, "(c) 1989 AT&T", 13))/ 2,
			ypos,
			"(c) 1989 AT&T", 13);
*/
	ypos += score_font->max_bounds.ascent * 4;
/*
	XDrawString(	dpy, window2, pas_GC, 
			(TILE_SIZE * GRID_SIZE - 
				XTextWidth(score_font, "Programmed by", 13))/2,
			ypos,
			"Programmed by", 13);
*/
	ypos += score_font->max_bounds.ascent * 1.5;
/*
	XDrawString(	dpy, window2, pas_GC, 
			(TILE_SIZE * GRID_SIZE - 
				XTextWidth(score_font, "Andy Oakland", 12))/2,
			ypos,
			"Andy Oakland", 12);
*/
	ypos += score_font->max_bounds.ascent * 1.5;
/*
	XDrawString(	dpy, window2, pas_GC, 
			(TILE_SIZE * GRID_SIZE - 
				XTextWidth(score_font, "Bruce Barnett", 13))/2,
			ypos,
			"Bruce Barnett", 13);
*/
	ypos += score_font->max_bounds.ascent * 3;
	XDrawString(	dpy, window2, pas_GC, 
			(TILE_SIZE * GRID_SIZE - 
				XTextWidth(score_font, "Click mouse to start", 20))/ 2,
			(TILE_SIZE * GRID_SIZE - (2 *
				(score_font->max_bounds.ascent +
				score_font->max_bounds.descent))),
/*
			ypos,
*/
			"Click mouse to start", 20);

	XSync(dpy, True);

	if (demo_mode == True) {
		Spin(4000);
		break;
	}
	else {
                XMaskEvent(     dpy,
                                ExposureMask | ButtonPressMask,
                                &ev);
		if (ev.type == ButtonPress)
			break;
	}
	} /* end of for (;;) */

	XClearWindow(dpy,window2);
	XSetFillStyle(dpy, pas_GC, FillStippled);
}
