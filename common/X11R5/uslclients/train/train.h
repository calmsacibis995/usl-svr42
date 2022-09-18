#pragma ident	"@(#)train:train.h	1.2"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include	"traxa.bitmap"
#include	"traxb.bitmap"
#include	"traxc.bitmap"
#include	"stn.bitmap"
#include	"pas1.bitmap"
#include	"pas2.bitmap"
#include	"arm.bitmap"
#include	"train.icon"

#define	TILE_SIZE	64
#define	BORDER_WIDTH	1
#define	PANEL_HEIGHT	32
#define	NUM_POSES	4
#define GRID_SIZE	6
#define WAVE_IN		1
#define WAVE_OUT	2
#define PASSENGER_NONE		-1
#define PASSENGER_RIDING	-2
#define PASSENGERS_FOR_NEXT_LEVEL	5
#define TIME_DECREASE			5
#define MAX_TRAIN_LEN	15
#define NUM_TRAINS	4
#define MIN_TIME	25
#define MIN_PAUSE	20
#define ORIG_PAUSE	120
#define NUM_TILES	GRID_SIZE * GRID_SIZE

#define PASSENGER_VALUE	100
#define BONUS		PASSENGER_VALUE
#define BONUS_CAR	5000
#define TRAIN_WIDTH	10
#define TRAIN_HEIGHT	10

#define SCORE_FONT	"met25"

#define SCORE_FILE	"/usr/x/demos/train/scores"

typedef struct Scores {
	char	name[4];
	int	score;
	int	level;
} Scores;

/*
** GRID_SIZE includes the one-bitmap immobile boundry
*/

#define XOFWIN(foo)	((foo % GRID_SIZE) * TILE_SIZE)
#define YOFWIN(foo)	((foo / GRID_SIZE) * TILE_SIZE)

static struct
	{	Window	wid;
		int	type;
	} window_list[GRID_SIZE * GRID_SIZE];

typedef struct foo {
	int		tile;
	int		stnx, stny;
	int		pasx, pasy;
	unsigned long	pixel;
} station_type;

typedef struct timezone	Timezone;
typedef struct timeval	Timeval;


typedef struct bar {
	int position_list[4][NUM_POSES][2];	/* intermediate positions */
	int exits[4];				/* direction you exit in */
} tile_type;


