/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)uts-comm:util/malloc.c	1.3.3.3"
#ident	"$Header: $"

#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/map.h>
#include <util/param.h>
#include <util/spl.h>
#include <util/types.h>

/*
 * Allocate 'size' units from the given map.
 * Return the base of the allocated space.
 * In a map, the addresses are increasing and the
 * list is terminated by a 0 size.
 * Algorithm is first-fit.
 *
 * As part of the kernel evolution toward modular naming, the 
 * functions malloc and mfree are being renamed to rmalloc and rmfree.
 * Compatibility will be maintained by having malloc call 
 * rmalloc and mfree call rmfree.
 */

unsigned long
malloc(mp, size)
register struct map *mp;
register size_t size;
{
	return rmalloc(mp, size);
}

unsigned long
rmalloc(mp, size)
register struct map *mp;
register size_t size;
{
	register unsigned int a;
	register struct map *bp;
	register int s;

	if (size == 0)
		return 0;

	ASSERT(size > 0);
	s = splhi();

	for (bp = mapstart(mp); bp->m_size; bp++) {

		if (bp->m_size >= size) {

			a = bp->m_addr;
			bp->m_addr += size;
			bp->m_size -= size;

			/*
			 * If the entry's size is 0,
			 * shift the following entries up by one slot 
			 * since only the last entry has size 0.
			 */
			if (bp->m_size == 0) {
				do {
					bp++;
					(bp-1)->m_addr = bp->m_addr;
				} while (((bp-1)->m_size = bp->m_size) != 0);

				/* keep track of the number of empty slots */
				mapsize(mp)++;
			}

			ASSERT(bp->m_size < (unsigned)0x80000000);

			splx(s);
			return a;
		}
	}

	splx(s);
	return 0;
}

/*
 * Free the previously allocated space 'a'
 * of 'size' units into the specified map.
 * Sort 'a' into map and combine on
 * one or both ends if possible.
 */

void
mfree(mp, size, a)
register struct map *mp;
register size_t size;
register unsigned long a;
{
	rmfree(mp, size, a);
}

void
rmfree(mp, size, a)
register struct map *mp;
register size_t size;
register unsigned long a;
{
	register struct map *bp;
	register unsigned int t;
	register int s;

	if (size == 0)
		return;

	ASSERT(size > 0);
	s = splhi();

	for (bp = mapstart(mp); bp->m_addr <= a && bp->m_size != 0; bp++)
		;

	if (bp > mapstart(mp) && (bp-1)->m_addr+(bp-1)->m_size == a) {
		/* 'a' adjoins bp's previous entry */

		(bp-1)->m_size += size;

		if (bp->m_addr) {	/* m_addr==0 end of map table */

			ASSERT(a+size <= bp->m_addr);

			if (a+size == bp->m_addr) { 
				/* 'a' also adjoins bp's entry */

				(bp-1)->m_size += bp->m_size;

				/* compress adjacent map addr entries */
				while (bp->m_size) {
					bp++;
					(bp-1)->m_addr = bp->m_addr;
					(bp-1)->m_size = bp->m_size;
				}

				/* keep track of the number of empty slots */
				mapsize(mp)++;
			}
		}

	} else {

		if (a+size == bp->m_addr && bp->m_size) {
			/* 'a' adjoins bp's entry */

			bp->m_addr -= size;
			bp->m_size += size;
			ASSERT(bp == mapstart(mp) ||
			  ((bp - 1)->m_addr + (bp - 1)->m_size) < bp->m_addr);

		} else {
			/*
			 * 'a' doesn't adjoin bp's previous entry
			 * nor does it adjoin bp's entry.
			 */

			ASSERT(bp == mapstart(mp) ||
			  ((bp - 1)->m_addr + (bp - 1)->m_size) < a);

			ASSERT(bp->m_size == 0 || (a+size < bp->m_addr));

			if (mapsize(mp) == 0) {
				/* no more empty slots left in map */
				cmn_err(CE_WARN,
					"rmfree map overflow %x.  Lost %d items at %d\n",
					mp, size, a);
				splx(s);
				return;
			}

			/*
			 * Shift map's entries down one slot
			 * to accommodate 'a' and decrement
			 * the number of empty slots.
			 */

			do {
				t = bp->m_addr;
				bp->m_addr = a;
				a = t;
				t = bp->m_size;
				bp->m_size = size;
				bp++;
				size = t;
			} while (size);

			mapsize(mp)--;
		}
	}

	/* 
	 * Check if there are any processes waiting on the map.
	 * If so, wake them up.
	 */
	if (mapwant(mp)) {
		mapwant(mp) = 0;
		wakeprocs((caddr_t)mp, PRMPT);
	}

	splx(s);
}
