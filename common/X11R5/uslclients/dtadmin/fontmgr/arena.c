/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtadmin:fontmgr/arena.c	1.1"

/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       arena.c
 */

#if	defined(SVR4)
#include "stdarg.h"
#else
#include "varargs.h"
#endif

#include "malloc.h"
#include "stdio.h"
#include "memory.h"

#include "arena.h"

/*
 * Convenient macros:
 */

	/*
	 * AT() gives the address of the Nth item.
	 * Note: Assumes local variable "data_size" is set.
	 */
#define AT(pVar,N) ((char *)(pVar)->arena + data_size * (N))

	/*
	 * CMP() compares two elements.
	 * Note: Assumes local variables "pVar" and "data_size" are set.
	 */
#define CMP(pA,pB) \
	(pVar->compare?							\
		  ((*pVar->compare)((char *)pA, (char *)pB))		\
		: memcmp((mem_hush)pA, (mem_hush)pB, data_size)		\
	)

	/*
	 * EQU() sees if two elements are equal.
	 * Note: Assumes local variables "pVar" and "data_size" are set.
	 */
#define EQU(pA,pB)	(CMP(pA,pB) == 0)

	/*
	 * This keeps the compiler from complaining about the arguments
	 * we pass to the memory(3C) routines.
	 */
#if	OlNeedFunctionPrototypes
# define mem_hush	const void *
#else
# define mem_hush	char *
#endif

/*
 * Private data:
 */

static Cardinal		__OlArenaDataSize	= 0;

/*
 * Private routines:
 */

static Boolean		___OlArenaFind OL_ARGS((
	_OlArenaGeneric *	pVar,
	int *			pIndx,
	int			hint,
	Cardinal		data_size,
	va_list			ap
));

/**
 ** _OlArenaDefaultCompare()
 **
 ** Default comparison routine for use with "qsort" and similar routines.
 ** Note: Requires static variable "__OlArenaDataSize".
 **/

int
#if	OlNeedFunctionPrototype
_OlArenaDefaultCompare (
	char *			pA,
	char *			pB
)
#else
_OlArenaDefaultCompare (pA, pB)
	char *			pA;
	char *			pB;
#endif
{
	return (memcmp((mem_hush)pA, (mem_hush)pB, __OlArenaDataSize));
} /* _OlArenaDefaultCompare() */

/**
 ** __OlArenaFind()
 **/

int
#if	OlNeedFunctionPrototypes
__OlArenaFind (
	String			file,
	int			line,
	_OlArenaGeneric *	pVar,
	int *			pIndx,
	Cardinal		data_size,
	...
)
#else
__OlArenaFind (file, line, pVar, p_index, data_size, va_alist)
	String			file;
	int			line;
	_OlArenaGeneric *	pVar;
	int *			pIndx;
	Cardinal		data_size;
	va_dcl
#endif
{
	va_list			ap;

	int			indx	= _OL_NULL_ARENA_INDEX;

	Boolean			found	= False;


#if	defined(__STDC__)
	va_start (ap, data_size);
#else
	va_start (ap);
#endif


	if (!pVar)
		OlVaDisplayWarningMsg (
			(Display *)0,
			"emptyArena",
			"illegalOperation",
			"OlToolkitWarning",
			"Null arena pointer: \"%s\", line %d.",
			file, line
		);
	else {
		found = ___OlArenaFind(pVar, &indx, _OL_NULL_ARENA_INDEX, data_size, ap);
		if (pIndx)
			*pIndx = indx;
	}

	return (found? indx : _OL_NULL_ARENA_INDEX);
} /* __OlArenaFind */

/**
 ** ___OlArenaInsert()
 **/

int
#if	OlNeedFunctionPrototypes
___OlArenaInsert (
	String			file,
	int			line,
	_OlArenaGeneric *	pVar,
	int			flag,
	int			indx,
	Cardinal		data_size,
	...
)
#else
____OlArenaInsert (file, line, pVar, flag, indx, data_size, va_alist)
	String			file;
	int			line;
	OlArenaGeneric *	pVar;
	int			flag;
	int			indx;
	Cardinal		data_size;
	va_dcl
#endif
{
	va_list			ap;

#if	defined(__STDC__)
	va_start (ap, data_size);
#else
	va_start (ap);
#endif


	if (!pVar) {
		OlVaDisplayWarningMsg (
			(Display *)0,
			"emptyArena",
			"illegalOperation",
			"OlToolkitWarning",
			"Null arena pointer: \"%s\", line %d.",
			file, line
		);
		return (_OL_NULL_ARENA_INDEX);
	}

	switch (flag) {

	case _OL_ARENA_ORDERED_INSERT:
	case _OL_ARENA_ORDERED_UNIQUE_INSERT:
		if (!(pVar->flags & _OL_ARENA_IS_ORDERED)) {
			OlVaDisplayWarningMsg (
				(Display *)0,
				"orderedInsert",
				"illegalOperation",
				"OlToolkitWarning",
	"Attempted ordered insert into unordered arena: \"%s\", line %d.",
				file, line
			);
			indx = pVar->num_elements;
		} else if (flag == _OL_ARENA_ORDERED_UNIQUE_INSERT) {
			if (___OlArenaFind(pVar, &indx, indx, data_size, ap))
				return (indx);
		} else
			(void)___OlArenaFind(pVar, &indx, indx, data_size, ap);
		break;

	case _OL_ARENA_DIRECT_INSERT:
	default:
		pVar->flags &= ~_OL_ARENA_IS_ORDERED;
		break;
	}

	/*
	 * If the arena is empty and we've been given an explicit
	 * starting size, preallocate to that size. If we haven't been
	 * given an explicit starting size, don't do anything yet...the
	 * step size will be used shortly.
	 */
	if (!pVar->num_slots && pVar->initial != _OlArenaDefaultInitial) {
		pVar->arena = XtMalloc(pVar->initial * data_size);
		pVar->num_slots = pVar->initial;
	}

	/*
	 * If we don't have any more room, allocate some.
	 */
	if (pVar->num_elements == pVar->num_slots) {
		if (pVar->step == _OlArenaDefaultStep)
			pVar->num_slots += (pVar->num_slots / 2) + 2;
		else
			pVar->num_slots += pVar->step;
		pVar->arena = XtRealloc(
			pVar->arena,
			(pVar->num_slots * data_size)
		);
	}

	/*
	 * An out-of-range index causes an append.
	 */
	if ((indx < 0) || (indx > pVar->num_elements))
		indx = pVar->num_elements;

	/*
	 * Move up the data starting at "indx" to make room for the
	 * new data-item. Note: "->num_elements" hasn't been bumped
	 * yet, so "->num_elements - indx" is the number of data-items
	 * from "indx" up.
	 */
	if (pVar->num_elements > indx)
		OlMemMove (
			char,
			AT(pVar, indx+1),
			AT(pVar, indx),
			data_size * (pVar->num_elements - indx)
		);

	/*
	 * Copy the data, bump the count.
	 */
	memcpy (AT(pVar, indx), (char *)ap, data_size);
	pVar->num_elements++;

	return (indx);
} /* ___OlArenaInsert() */

/**
 ** __OlArenaSort()
 **/

void
#if	OlNeedFunctionPrototypes
__OlArenaSort (
	String			file,
	int			line,
	_OlArenaGeneric *	pVar,
	Cardinal		data_size
)
#else
__OlArenaSort (file, line, pVar, data_size)
	String			file;
	int			line;
	OlArenaGeneric *	pVar;
	Cardinal		data_size;
#endif
{
	_OlArenaQsortArg4	cmp;


	if (!pVar) {
		OlVaDisplayWarningMsg (
			(Display *)0,
			"emptyArena",
			"illegalOperation",
			"OlToolkitWarning",
			"Null arena pointer: \"%s\", line %d.",
			file, line
		);
		return;
	}

	__OlArenaDataSize = data_size;	/* just in case */
	if (!(cmp = (_OlArenaQsortArg4)pVar->compare))
		cmp = (_OlArenaQsortArg4)_OlArenaDefaultCompare;

	qsort ((char *)pVar->arena, pVar->num_elements, data_size, cmp);
	pVar->flags |= _OL_ARENA_IS_ORDERED;

	return;
} /* __OlArenaSort */

/**
 ** ___OlArenaFind()
 **/

static Boolean
#if	OlNeedFunctionPrototypes
___OlArenaFind (
	_OlArenaGeneric *	pVar,
	int *			pIndx,
	int			hint,
	Cardinal		data_size,
	va_list			ap
)
#else
___OlArenaFind (pVar, pIndx, hint, data_size, ap)
	_OlArenaGeneric *	pVar;
	int *			pIndx;
	int			hint;
	Cardinal		data_size;
	va_list			ap;
#endif
{
	if (!pVar->num_elements) {
		*pIndx = 0;
		return (False);

	} else if (pVar->flags & _OL_ARENA_IS_ORDERED) {
		register int		low	= 0;
		register int		high	= pVar->num_elements - 1;
		register int		i;
		register int		cmp;
		register int		mid;

		if (low <= hint && hint <= high)
			mid = hint;
		else
			mid = ((high - low) / 2);
		do {
			i = low + mid;

			cmp = CMP(ap, AT(pVar,i));

			if (cmp == 0)
				break;
			else if (cmp < 0)
				high = i - 1;
			else
				low = i + 1;

			mid = ((high - low) / 2);
		} while (high >= low);

		/*
		 * "cmp" holds the result of the last comparison of the
		 * searched-for element and the "i"th element in the
		 * arena. If it shows that the searched-for element was
		 * ``greater'' than the "i"th element, then "i+1" is
		 * where the searched-for element should be inserted.
		 */
		if (cmp > 0)
			i++;
		*pIndx = i;
		return (cmp == 0);

	} else {
		register int		i;
		register int		start;
		register Boolean	found	= False;

		if (hint != _OL_NULL_ARENA_INDEX)
			start = hint;
		else
			start = 0;

		for (i = start; !found && i < pVar->num_elements; i++)
			found = EQU(ap, AT(pVar,i));
		for (i = 0; !found && i < start; i++)
			found = EQU(ap, AT(pVar,i));

		*pIndx = i;
		return (found);
	}
} /* ___OlArenaFind() */
