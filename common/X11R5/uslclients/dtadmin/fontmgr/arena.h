/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtadmin:fontmgr/arena.h	1.2"

/*
 * Module:     dtadmin:fontmgr   Graphical Administration of Fonts
 * File:       arena.h
 */

#ifndef _Ol_arena_h_
#define _Ol_arena_h_

/*************************************************************************
 *
 * Description:	OpenLook interface to a generic arena
 *
 *	_OlArenaStruct(Type,Name)
 *		Declare an arena named "Name" containing values of
 *		type "Type".
 *
 *	_OlArenaType(Name)
 *		Refer to the arena named "Name", for the purpose
 *		of creating variables of the named arena type.
 *		Needed to create a variable of this arena type,
 *		as in
 *
 *			_OlArenaStruct(double, Dbl);
 *			_OlArenaType(Dbl)	dblArena;
 *
 *		However, the following two might be easier:
 *
 *	_OlArenaDeclare(Name,Var)
 *		Equal to:
 *			_OlArenaType(Name)	Var;
 *
 *	_OlArenaDefine(Name,Var,Initial,Step,Fnc)
 *		Declare and initialize an arena of the named type.
 *		This arena can be accessed with the variable "Var",
 *		and can be referenced with the name "Name".
 *		The arena is initially extended "Initial" elements,
 *		and thereafter extended "Step" elements.
 *		(No space is actually allocated until the first time
 *		an element is added to the arena.)
 *		"Fnc" is the comparison function used to search
 *		for an element and when sorting the arena. It's called
 *		like this:
 *
 *			(*Fnc)(pElement1, pElement2)
 *
 *		and should return -1,0,1 for <,==,>.
 *
 *	void _OlArenaInitialize(pVar,Initial,Step,Fnc)
 *		Like _OlArenaDefine, except it will initialize an
 *		existing arena pointed to by "pVar".
 *
 *	void _OlArenaInit(pVar)
 *		As above, with
 *			Initial == _OlArenaDefaultInitial
 *			   Step == _OlArenaDefaultStep
 *			    Fnc == _OlArenaDefaultCompare
 *
 *		These defaults for "Initial" and "Step" cause
 *		an increasing extension as more space is needed, with
 *		the following progression (total extent):
 *
 *			2, 5, 9, 15, 24, 38, ...
 *
 *		(Each increase is 1/2 the current extent plus 2).
 *
 *		If "Initial" is given explicitly in _OlArenaInitialize()
 *		or _OlArenaDefine() but "Step" is defaulted, the
 *		progression starts with the initial value:
 *
 *			n, n+(n/2)+2, n+(n/2)+2 + (n+(n/2)+2)/2 + 2, ...
 *
 *		If "Initial" is defaulted but "Step" is given
 *		explicitly, the progression is uniform:
 *
 *			Step, 2*Step, 3*Step, 4*Step, ...
 *
 *		The default for "Fnc" is really just "memcmp()" in
 *		disguise. Warning: This default comparison works fine
 *		for elements that don't include pointers in their
 *		substructure (or when pointer comparison is desired).
 *		If the pointed-to values should be compared instead,
 *		you'll have to provide your own comparison routine.
 *
 *	void _OlArenaDelete(pVar,indx)
 *		Deletes the "indx"th element from the arena pointed to
 *		by "pVar".
 *
 *	void _OlArenaFree(pVar)
 *		Frees the arena. Note: You are responsible for freeing
 *		any auxiliary storage pointed to by each element sub-
 *		structure.
 *
 *	void _OlArenaAppend(pVar,Data)
 *		Appends the data element to the arena.
 *
 *	void _OlArenaUniqueAppend(pVar,Data)
 *		Appends the data element to the arena if it doesn't
 *		already exist.
 *
 *	int _OlArenaInsert(pVar,indx,Data)
 *		Inserts the data element after the "indx"th element.
 *		Returns "indx".
 *
 *	int _OlArenaOrderedInsert(pVar,Data)
 *		Inserts the data element, maintaining a sorted
 *		order governed by the comparison function registered
 *		with the arena. As long as only this routine is
 *		used--not _OlArenaInsert()--the arena will stay
 *		sorted. Searches will typically be much faster,
 *		since a binary search can be employed, if the
 *		list is known to be ordered.
 *		Returns the index where the element was inserted.
 *
 *	int _OlArenaOrderedUniqueInsert(pVar,Data)
 *		As above, but only if the element isn't in the list
 *		already.
 *		Returns the index where the element was found or
 *		inserted.
 *
 *	int _OlArenaHintedOrderedInsert(pVar,indx,Data)
 *		Like _OlArenaOrderedInsert, except this one allows
 *		the client to suggest where to start searching for
 *		the correct place to insert the data. Typically
 *		used right after using _OlArenaFindHint() to see
 *		if an element already exists in the arena.
 *
 *	int _OlArenaFind(pVar,Data)
 *		Returns the index into the arena of the given data
 *		element. Returns _OL_NULL_ARENA_INDEX if the data
 *		element is not found.
 *
 *	int _OlArenaFindHint(pVar,pHint,Data)
 *		Like _OlArenaFind, except will also give the
 *		index of where the given data should be inserted
 *		to maintain an ordered arena. The insert index is
 *		assigned to the "int" pointed to by "pHint".
 *
 *	void _OlArenaSort(pVar)
 *		Sorts the arena (uses "qsort").
 *
 *	void _OlArenaSetOrdered(pVar)
 *		Marks the arena as ordered. This is useful when
 *		you have inserted the data ``manually'' (with
 *		_OlArenaInsert, not _OlArenaOrderedInsert) but
 *		have still maintained a sort order.
 *
 *	_OlArenaElement(pVar,indx)
 *		References the "indx"th element in the arena.
 *		The reference is the element itself, not a pointer
 *		to it. It is also an ``lvalue''. Thus, some examples
 *		of its use are:
 *
 *			_OlArenaElement(&Bob,3) = Data;
 *			_OlArenaElement(&Bob,3).field = 23;
 *			&(_OlArenaElement(&Bob,3))->field = 23;
 *
 *		CAUTION: Do not expect the address of an element to
 *		stay constant for the life of that element, even if
 *		it's index does stay constant!
 *
 *	_OlArenaSize(pVar)
 *		Gives the number of elements in the arena (but usually
 *		*not* the allocated extent of the arena!)
 *
 *	CAUTION: The "Data" paramenter must be of the correct type
 *	for these routines to work properly; you'll get no compiler
 *	warning for wrong types (sorry)!
 *
 *******************************file*header******************************/

#include <X11/Intrinsic.h>
#include <Xol/OpenLookP.h>	/* for OL_ARGS, OlNeedFunctionPrototypes */

/*
 * Need the following to put in front of arg #4 of qsort,
 * to keep the compiler from warning us.
 */
typedef int (*_OlArenaQsortArg4) OL_ARGS((const void * , const void *));

typedef int (*_OlArenaCompareFncType) OL_ARGS(( char * , char * ));

#define _OlArenaStruct(Type,Name) \
	struct Name {							\
		Type *			arena;				\
		Cardinal		num_elements;			\
		Cardinal		num_slots;			\
		Cardinal		initial;			\
		Cardinal		step;				\
		_OlArenaCompareFncType	compare;			\
		unsigned int		flags;				\
	}
#define _OL_ARENA_IS_ORDERED	0x0001

typedef _OlArenaStruct(char,__OlArenaGeneric)	_OlArenaGeneric;

#define _OlArenaType(Name)			struct Name
#define _OlArenaDeclare(Name,Var)		_OlArenaType(Name) Var

#define _OlArenaDefine(Name,Var,Initial,Step,Fnc) \
	_OlArenaDeclare(Name,Var) = {					\
		0, 0, 0,						\
		Initial, Step,						\
		(_OlArenaCompareFncType)Fnc,				\
		_OL_ARENA_IS_ORDERED					\
	}

#define _OlArenaInit(pVar) \
	_OlArenaInitialize(pVar,_OlArenaDefaultInitial,_OlArenaDefaultStep,0)

#define _OlArenaInitialize(pVar,Initial,Step,Fnc) \
	( (pVar)->arena         = 0,					\
	  (pVar)->num_elements  = 0,					\
	  (pVar)->num_slots     = 0, 					\
	  (pVar)->initial       = (Initial),				\
	  (pVar)->step          = (Step),				\
	  (pVar)->compare       = (_OlArenaCompareFncType)(Fnc),	\
	  (pVar)->flags		= _OL_ARENA_IS_ORDERED			\
	)

#define _OlArenaDelete(pVar,I) \
	if ((pVar)->arena && 0 <= (I) && (I) < (pVar)->num_elements) {	\
		register char * pI  = (char *)&((pVar)->arena[I]);	\
									\
		(pVar)->num_elements--;					\
		OlMemMove (						\
			char,						\
			pI,						\
			(char *)&((pVar)->arena[(I)+1]),		\
			_OlArenaElementSize(pVar) * (pVar)->num_elements\
				 - (pI - (char *)(pVar)->arena)		\
		);							\
	} else

#define _OlArenaFree(pVar) \
	if ((pVar)->arena) {						\
		XtFree ((char *)(pVar)->arena);				\
		(pVar)->arena         = 0;				\
		(pVar)->num_elements  = 0;				\
		(pVar)->num_slots     = 0;				\
		(pVar)->flags         = _OL_ARENA_IS_ORDERED;		\
	} else

#define	_OlArenaAppend(pVar,Data) \
	(void)_OlArenaInsert((pVar), (pVar)->num_elements, Data)

#define	_OlArenaUniqueAppend(pVar,Data) \
	if (_OlArenaFind(pVar, Data) == _OL_NULL_ARENA_INDEX)		\
		(void)_OlArenaInsert(pVar, (pVar)->num_elements, Data);	\
	else

#define	_OlArenaInsert(pVar,indx,Data) \
	__OlArenaInsert(pVar,_OL_ARENA_DIRECT_INSERT,indx,Data)

#define	_OlArenaOrderedInsert(pVar,Data) \
	__OlArenaInsert(pVar,_OL_ARENA_ORDERED_INSERT,_OL_NULL_ARENA_INDEX,Data)

#define	_OlArenaHintedOrderedInsert(pVar,hint,Data) \
	__OlArenaInsert(pVar,_OL_ARENA_ORDERED_INSERT,hint,Data)

#define	_OlArenaOrderedUniqueInsert(pVar,Data) \
	__OlArenaInsert(pVar,_OL_ARENA_ORDERED_UNIQUE_INSERT,_OL_NULL_ARENA_INDEX,Data)

#define __OlArenaInsert(pVar,flag,indx,Data) \
	___OlArenaInsert(						\
		__FILE__, __LINE__,					\
		(_OlArenaGeneric *)(pVar),				\
		flag,							\
		indx,							\
		_OlArenaElementSize(pVar),				\
		Data							\
	)

#define	_OlArenaFind(pVar,Data) \
	__OlArenaFind(							\
		__FILE__, __LINE__,					\
		(_OlArenaGeneric *)(pVar),				\
		(int *)0,						\
		_OlArenaElementSize(pVar),				\
		Data							\
	)

#define	_OlArenaFindHint(pVar,pHint,Data) \
	__OlArenaFind(							\
		__FILE__, __LINE__,					\
		(_OlArenaGeneric *)(pVar),				\
		pHint,							\
		_OlArenaElementSize(pVar),				\
		Data							\
	)

#define	_OlArenaSort(pVar) \
	if (!((pVar)->flags & _OL_ARENA_IS_ORDERED))			\
		__OlArenaSort(						\
			__FILE__,__LINE__,				\
			(_OlArenaGeneric *)(pVar),			\
			_OlArenaElementSize(pVar)			\
		)

#define _OlArenaSetOrdered(pVar) \
	(pVar)->flags = _OL_ARENA_IS_ORDERED

#define _OlArenaElement(pVar,indx)	((pVar)->arena[indx])
#define _OlArenaSize(pVar)		(pVar)->num_elements
#define _OlArenaElementSize(pVar)	(Cardinal)sizeof(*(pVar)->arena)
#define _OlArenaDefaultInitial		0	/* i.e. 2 */
#define _OlArenaDefaultStep		0	/* n += (n / 2) + 2 */

#define _OL_NULL_ARENA			0
#define _OL_NULL_ARENA_INDEX		-1
#define _OL_ARENA_IS_EMPTY(A)		((A)->num_elements == 0)

#define _OL_ARENA_DIRECT_INSERT		1
#define _OL_ARENA_ORDERED_INSERT	2
#define _OL_ARENA_ORDERED_UNIQUE_INSERT	3

#define _OL_ARENA_INITIAL \
	{ 0, 0, 0, _OlArenaDefaultInitial, _OlArenaDefaultStep,		\
						0, _OL_ARENA_IS_ORDERED }

int			_OlArenaDefaultCompare OL_ARGS((
	char *			pA,
	char *			pB
));
int			___OlArenaInsert OL_ARGS((
	char *			file,
	int			line,
	_OlArenaGeneric *	pVar,
	int			flag,
	int			indx,
	Cardinal		data_size,
	...
));
int			__OlArenaFind OL_ARGS((
	char *			file,
	int			line,
	_OlArenaGeneric *	pVar,
	int *			pIndx,
	Cardinal		data_size,
	...
));
void			__OlArenaSort OL_ARGS((
	char *			file,
	int			line,
	_OlArenaGeneric *	pVar,
	Cardinal		data_size
));

#endif /* _Ol_arena_h_ */
