/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/lvlintersect.c	1.3"

#ifdef __STDC__
	#pragma weak lvlintersect = _lvlintersect
#endif

#include "synonyms.h"
#include <sys/types.h>
#include <sys/param.h>
#include <mac.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

/*
 * The following routines are static to this file, and used by the library
 * routine lvlintersect.
 */

static int	lvl_lid_to_struct();
static void	lvl_intersect();
static void	adjust_lvl();
static int	lvl_struct_to_lid();


/*
 * lvlintersect	- perform intersection operation for the two given LIDS
 *
 * Notes:
 *	1. LIDs must be valid active/inactive for any success.
 *	2. resulting LID must be valid active/inactive for any success.
 *	3. use external errno cautiously; it may be set by other calls.
 *	4. set user's LID only when successful.
 *
 * Return:
 *	0	- success
 *	1	- success, but lid is inactive
 *	-1	- EINVAL: LID is invalid
 *		- EEXIST: more that one entry found and none are valid active
 *		- EACCES: cannot open LTDB
 */
int
lvlintersect(level1p, level2p, level3p)
	const level_t		*level1p;
	const level_t		*level2p;
	level_t		*level3p;
{
	struct mac_level buf1;			/* level structure buffer */
	register struct mac_level *lvl1p = &buf1;	/* level structure ptr */
	struct mac_level buf2;			/* level structure buffer */
	register struct mac_level *lvl2p = &buf2;	/* level structure ptr */
	struct mac_level buf3;			/* level structure buffer */
	register struct mac_level *lvl3p = &buf3;	/* level structure ptr */
	int		err = 0;		/* temporary err */
	int i;

	/* validate given LIDs by getting the mac_level structure for each*/
	err = lvl_lid_to_struct(*level1p, lvl1p); 
	if (err) {
	   errno = err;
	   return(-1);
	}

	err = lvl_lid_to_struct(*level2p, lvl2p); 
	if (err) {
	   errno = err;
	   return(-1);
	}

	(void)memset((void *)lvl3p, 0, (size_t)sizeof(struct mac_level));
	lvl_intersect(lvl1p, lvl2p, lvl3p);

	err = lvl_struct_to_lid(lvl3p, level3p);
	if (err) {
		if (err > 0) {
	   		errno = err;
	   		return(-1);
		}
		else
			return(1);
	}
	return (0);

}

/*
 * lvl_lid_to_struct	- convert LID to level structure
 *
 * Notes:
 *	1. open lid.internal, seek to entry for LID, and read entry.
 *	2. level must be valid; may be active or inactive.
 *
 * Return:
 *	0	- success
 *	EINVAL	- LID is 0, reserved
 *		- cannot seek through lid.internal
 *		- LID is not valid
 *	EACCES	- cannot open LTDB
 */
static int
lvl_lid_to_struct(lid, lvlp)
	level_t	lid;			/* lid to convert */
	struct mac_level *lvlp;		/* structure to use */
{
	int	rfd;			/* read file descriptor */

	/* 0 reserved */
	if (lid == (level_t)0)
		return(EINVAL);

	if ((rfd = open(LTF_LID, O_RDONLY, 0)) == -1)
		return(EACCES);
	
	if (lseek(rfd, lid*sizeof(struct mac_level), 0) == -1) {
		(void)close(rfd);
		return(EINVAL);
	}

	/* level must be valid; may be active or inactive */
	if ((read(rfd, lvlp, sizeof(struct mac_level))
		!= sizeof(struct mac_level))
	||  (lvlp->lvl_valid == LVL_INVALID)) {
		(void)close(rfd);
		return(EINVAL);
	}

	(void)close(rfd);
	return(0);
}

/*
 * lvl_intersect - perform intersection of lvl1p and lvl2p and put sum in lvl3p
 *
 */
static void 
lvl_intersect(lvl1p, lvl2p, lvl3p)
	struct mac_level *lvl1p;
	struct mac_level *lvl2p;
	struct mac_level *lvl3p;
{
	register int i;

	if (lvl1p->lvl_class < lvl2p->lvl_class)
		lvl3p->lvl_class = lvl1p->lvl_class;
	else
		lvl3p->lvl_class = lvl2p->lvl_class;
	for (i = 0; i < CAT_SIZE; i++) {
		if (lvl1p->lvl_catsig[i] != 0)
			lvl3p->lvl_cat[(lvl1p->lvl_catsig[i] - 1)] |= lvl1p->lvl_cat[i];
		else 
			break;
	}
	for (i = 0; i < CAT_SIZE; i++) {
		if (lvl2p->lvl_catsig[i] != 0)
			lvl3p->lvl_cat[(lvl2p->lvl_catsig[i] - 1)] &= lvl2p->lvl_cat[i];
		else 
			break;
	}
	for (i = 0; i < CAT_SIZE; i++) {
		if (lvl3p->lvl_cat[i]) 
			lvl3p->lvl_catsig[i] = 1;
	}
	adjust_lvl(lvl3p);
}

/*
 * adjust_lvl	- adjust level structure for performance
 *
 * Notes:
 *	1. The catsig and cat arrays are packed so that the significant
 *	   data is kept together.
 */
static void
adjust_lvl(lvlp)
	register struct mac_level *lvlp;	/* level structure */
{
	register int	i;			/* loop counters */
	register int	j;

	/* compact categories */
	for (i = 0, j = 0; i < CAT_SIZE; i++) {
		if (lvlp->lvl_catsig[i]) {
			lvlp->lvl_catsig[j] = i + 1;	/* starts at 1 */
			if (i != j)
				lvlp->lvl_cat[j] = lvlp->lvl_cat[i];
			j++;
		}
	}

	/* zero out the rest */
	while (j < CAT_SIZE) {
		lvlp->lvl_catsig[j] = 0;
		lvlp->lvl_cat[j] = (ulong)0;
		j++;
	}
}

/*
 * lvl_struct_to_lid	- get lid for level structure
 *
 * Notes:
 *	1. Entry must be valid active/inactive.
 *	2. If more than one entry matches; return the
 *	   valid active entry; if all matches are
 *	   inactive error.
 *
 * Return:
 *	0	- success
 *	-1      - success, but lid is inactive
 *	EINVAL	- cannot seek through LTDB internal file
 *		- entry does not exist in LTDB internal file
 *		  (i.e., not valid-active)
 *	EEXIST  - more than one entry found and none
 *		  are valid active.
 *	EACCES	- cannot open LTDB
 */
static int
lvl_struct_to_lid(lvlp, lidp)
	register struct mac_level *lvlp;	/* level structure */
	level_t		*lidp;			/* store at lid ptr */
{
	register int	rfd;			/* read file descriptor */
	register ulong	index;			/* index in file */
	register int	cnt;			/* read count */
	register int	i;			/* loop counter */
	register int	lidcnt=0;		/* count of matching lids in
						   the LTDB */
	level_t		sav_lid=0;		/*  matching lid that is
						   inactive */
	struct mac_level buf;		/* static level buffer */
	register struct mac_level *bufp = &buf;	/* level structure ptr */

	if ((rfd = open(LTF_LID, O_RDONLY, 0)) == -1)
		return(EACCES);
	
	/* 0 is reserved */
	if (lseek(rfd, sizeof(struct mac_level), 0) == -1) {
		(void)close(rfd);
		return(EINVAL);
	}

	for (index = 1, cnt = read(rfd, bufp, sizeof(struct mac_level));
	     cnt == sizeof(struct mac_level);
	     index++, cnt = read(rfd, bufp, sizeof(struct mac_level))) {
		if (bufp->lvl_valid == LVL_INVALID)
			continue;
		if (lvlp->lvl_class != bufp->lvl_class)
			continue;
		for (i = 0; i <= CAT_SIZE; i++) {
			if (lvlp->lvl_catsig[i] != bufp->lvl_catsig[i])
				break;
			if (bufp->lvl_catsig[i] == 0) {
				if (bufp->lvl_valid == LVL_ACTIVE) {
					(void)close(rfd);
					*lidp = (level_t)index;
					return(0);
				}
				sav_lid = (level_t)index;
				lidcnt++;
				break;
			}
			if (lvlp->lvl_cat[i] != bufp->lvl_cat[i])
				break;
		}
	}

	(void)close(rfd);

	if (sav_lid) {
		if (lidcnt == 1) {
			*lidp = sav_lid;
			return(-1);
		}
		else
			return(EEXIST);
	}
	return(EINVAL);
}
