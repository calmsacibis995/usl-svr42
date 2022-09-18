/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/bitvector.c	1.13"
#ifdef BV_DEBUG

#define Arena int
extern char * malloc();
#define Arena_alloc(arena,cnt,type) ((type *)malloc(cnt * sizeof(type)))
#define myVOID char *
#include "bitvector.h"
#ifndef NODBG
#define NDEBUG
#include <assert.h>
#endif

#else
#include "amigo.h"
#endif

#include <stdio.h>
#include <memory.h>
#define NULL 0

/* MACHINE/IMPLEMENTATION DEPENDENT #defines: */

#define BITSPERBYTE 8
#define BYTESPERWORD 4
#define BITSPERWORD 32
#define LOGBITSPERWORD 5
#define BV_SIZE(bv) (*(bv))
/* size in bits: change to static var if we don't store in vector */
#define BV_WORDS(bv) (BV_SIZE(bv) ? (((BV_SIZE(bv))-1)>>LOGBITSPERWORD)+1 : 0 )
/* number of words excluding the size word */
#define BV_LO_WORD 1
/* change to 0 if we don't store max_elts */
#define BV_FIRST_WORD(bv) ((bv) + BV_LO_WORD)
#define BV_LAST_WORD(bv) ( BV_FIRST_WORD(bv) + BV_WORDS(bv) -1 )

	Bit
first_elem(elptr,bv)
Element *elptr;
Bit_vector bv;
{
	register Word *wptr;
	assert(bv != NULL);
	for(wptr = BV_FIRST_WORD(bv); wptr <= BV_LAST_WORD(bv); ++wptr) {
		if( *wptr ) { /* found a word with a bit set */
			register short s=16, d=0;
			for(s = BITSPERWORD/2; s; s >>= 1) 
				/* find index of low bit in word */
				if((*wptr) << d + s) d += s;
			elptr->wrdptr = wptr;
			elptr->offset = d;
			return
#ifndef NODBG
			elptr->bit =
#endif
			(wptr - BV_FIRST_WORD(bv))*BITSPERWORD+BITSPERWORD - d;
		}
	}
	return 
#ifndef NODBG
		elptr->bit =
#endif
		SENTINEL_BIT;
}

	Bit
last_elem(elptr,bv)
Element *elptr;
Bit_vector bv;
{
	register Word *wptr;
	assert(bv != NULL);
	for(wptr = BV_LAST_WORD(bv); wptr >= BV_FIRST_WORD(bv); --wptr) {
		if( *wptr ) { /* found a word with a bit set */
			register short s=16, d=0;
			for(s = BITSPERWORD/2; s; s >>= 1) 
				/* find maximum number of right shifts
				** which will not clear word using a binary
				** search tactic.
				*/
				if(((*wptr) >> d + s))
					d += s;
			elptr->wrdptr = wptr;
			elptr->offset = BITSPERWORD - d - 1;
			return
#ifndef NODBG
			elptr->bit =
#endif
				(wptr - BV_FIRST_WORD(bv)) * BITSPERWORD + d + 1; 
		}
	}
	return 
#ifndef NODBG
		elptr->bit =
#endif
			SENTINEL_BIT;
}

	Bit
next_elem(elptr,bv)
Element *elptr;
Bit_vector bv;
{
	register Word *wptr, word;
	register short s;
	assert(elptr->bit > SENTINEL_BIT && elptr->bit <= BV_SIZE(bv));
	assert(elptr->wrdptr != NULL);
	assert(bv != NULL);
	wptr = elptr->wrdptr;
	/* mask off this bit */
	s = BITSPERWORD - elptr->offset;
	if(s == BITSPERWORD) {
		wptr++; /* no more bits in this word */
		word = *wptr;
	}
	else word = *wptr & (~0 << s);
	for(; wptr <= BV_LAST_WORD(bv); word = *++wptr) {
		if( word ) { /* found a word with a bit set */
			register short d=0;
			for(s = BITSPERWORD/2; s; s >>= 1)
				/* find index of low bit in word */
				if(word << d + s) d += s;
			elptr->wrdptr = wptr;
			elptr->offset = d;
			return
#ifndef NODBG
			elptr->bit =
#endif
				(wptr - BV_FIRST_WORD(bv)) * BITSPERWORD + BITSPERWORD - d;
		}
	}
	return 
#ifndef NODBG
		elptr->bit = 
#endif
			SENTINEL_BIT;
}
	Bit
prev_elem(elptr,bv)
Element *elptr;
Bit_vector bv;
{
	register Word *wptr, word;
	register unsigned short s;
	assert(elptr->bit > SENTINEL_BIT && elptr->bit <= BV_SIZE(bv));
	assert(elptr->wrdptr != NULL);
	assert(bv != NULL);
	wptr = elptr->wrdptr;
	/* mask off this bit */
	s = elptr->offset + 1;
	if(s == BITSPERWORD) { /* element #1 */
		wptr--; /* no more bits in this word */
		word = *wptr;
	}
	else 
		word = *wptr & ((unsigned)~0 >> s);

	for(; wptr >= BV_FIRST_WORD(bv); word = *--wptr) {
		if( word ) { /* found a word with a bit set */
			register short d=0;
			for(s = BITSPERWORD/2; s; s >>= 1)
				/* find maximum number of right shifts
				** which will not clear word using a binary
				** search tactic.
				*/
				if(word >> d + s)
					d += s;
			elptr->wrdptr = wptr;
			elptr->offset = BITSPERWORD - d - 1;
			return
#ifndef NODBG
				elptr->bit = 
#endif
				(wptr - BV_FIRST_WORD(bv)) * BITSPERWORD + d + 1; 
		}

	}
	return
#ifndef NODBG
		elptr->bit =
#endif
		SENTINEL_BIT;
}


	Bit_vector
bv_alloc(bit_count,arena)
int bit_count;
Arena arena;
{
	Bit_vector ret;
	assert(bit_count);	
#define WORDS_NEEDED(x) ((x)>0 ? ((x)-1>>LOGBITSPERWORD) + BV_LO_WORD + 1 : 1)
	ret = (Bit_vector)Arena_alloc(arena,WORDS_NEEDED(bit_count),Word);
#undef WORDS_NEEDED
	BV_SIZE(ret) = bit_count;
	return ret;
}

/* fill bv with 1's or 0's */
	void
bv_init(init_value, bv)
	Boolean init_value;
	Bit_vector bv;
{
	assert(bv);
	if (!BV_SIZE(bv))
		return;
	if(init_value) {
		memset((char *)(bv+1), ~0,sizeof(Word) * BV_WORDS(bv));
#define UNUSED (BITSPERWORD - BV_SIZE(bv) % BITSPERWORD) % BITSPERWORD
		*BV_LAST_WORD(bv) &= (~(unsigned)0 >> UNUSED);
#undef UNUSED
	}
	else
		memset((char *)(bv+1), 0,sizeof(Word) * BV_WORDS(bv));
		
}

#define BV_WORD_OFFSET(bit) ((((bit)-1) >> LOGBITSPERWORD) + BV_LO_WORD)
#define BV_BIT_MASK(bit) ((unsigned)1 << (((bit)-1) % BITSPERWORD)) 

	void
bv_set_bit(bit,bv)
Bit bit;
Bit_vector bv;
{
	assert(bit <= BV_SIZE(bv));
	bv[BV_WORD_OFFSET(bit)] |= BV_BIT_MASK(bit);
}

	void
bv_clear_bit(bit,bv)
Bit bit;
Bit_vector bv;
{
	assert(bit <= BV_SIZE(bv));
	assert(bit > 0);
	bv[BV_WORD_OFFSET(bit)] &= ~BV_BIT_MASK(bit);
}

	Boolean
bv_belongs(bit,bv)
Bit bit;
Bit_vector bv;
{
	assert(bit <= BV_SIZE(bv));
	assert(bit > 0);
	return (Boolean) !!(bv[BV_WORD_OFFSET(bit)] & BV_BIT_MASK(bit));
}

	void
bv_print(bv)
Bit_vector bv;
{
	assert(bv);
	fputc('(',stderr);
	BV_FOR(bv,bit)
		fprintf(stderr," %d",bit);
	END_BV_FOR
	fputs(" )\n",stderr);	
	fflush(stderr);
}

	char *
bv_sprint(bv)
Bit_vector bv;
{
	static char str[200]="(";
	register char * p = str + 1;
	assert(bv);
	BV_FOR(bv,bit)
		p += sprintf(p," %d",bit);
	END_BV_FOR
	(void)sprintf(p," )");
	return str;
}

	void
bv_and(bv_return,bv1,bv2)
Bit_vector bv1, bv2,bv_return;
{
	Bit_vector bv;
	assert(bv1);
	assert(bv2);
	assert( BV_SIZE(bv1) == BV_SIZE(bv2));
	assert( BV_SIZE(bv1) == BV_SIZE(bv_return));
	for(bv=BV_FIRST_WORD(bv_return), bv1=BV_FIRST_WORD(bv1), bv2=BV_FIRST_WORD(bv2);
		bv <= BV_LAST_WORD(bv_return);
		*bv++ = *bv1++ & *bv2++);
}

	void
bv_and_eq(bv1,bv2)
Bit_vector bv1, bv2;
{
	Bit_vector bv_to;
	assert(bv1);
	assert(bv2);
	assert( BV_SIZE(bv1) == BV_SIZE(bv2));
	bv_to = BV_LAST_WORD(bv1);
	for(bv1 = BV_FIRST_WORD(bv1), bv2 = BV_FIRST_WORD(bv2);
		bv1 <= bv_to;) {
		*bv1 &= *bv2;
		bv1++;
		bv2++;
	}
}
	void
bv_or_eq(bv1,bv2)
Bit_vector bv1, bv2;
{
	Bit_vector bv_to;
	assert(bv1);
	assert(bv2);
	assert( BV_SIZE(bv1) == BV_SIZE(bv2));
	bv_to = BV_LAST_WORD(bv1);
	for(bv1 = BV_FIRST_WORD(bv1), bv2 = BV_FIRST_WORD(bv2);
		bv1 <= bv_to;) {
		*bv1 |= *bv2;
		bv1++;
		bv2++;
	}
}

	Bit_vector
bv_minus(bv1,bv2)
Bit_vector bv1, bv2;
{
	Bit_vector bv, bv_return;
	assert(bv1);
	assert(bv2);
	assert( BV_SIZE(bv1) == BV_SIZE(bv2));
	bv_return = bv_alloc(BV_SIZE(bv1),0);
	for(bv=BV_FIRST_WORD(bv_return),bv1=BV_FIRST_WORD(bv1),bv2=BV_FIRST_WORD(bv2) ; bv <= BV_LAST_WORD(bv_return);
		*bv++ = *bv1++ & ~*bv2++);
	return bv_return;
}

	void
bv_minus_eq(bv1,bv2)
Bit_vector bv1, bv2;
{
	Bit_vector bv_to;
	assert(bv1);
	assert(bv2);
	assert( BV_SIZE(bv1) == BV_SIZE(bv2));
	bv_to = BV_LAST_WORD(bv1);
	for(bv1 = BV_FIRST_WORD(bv1), bv2 = BV_FIRST_WORD(bv2);
		bv1 <= bv_to;) {
		*bv1 &= ~*bv2;
		bv1++;
		bv2++;
	}
}
	void
bv_assign(bv1,bv2)
Bit_vector bv1, bv2;
{
	Bit_vector bv_to;
	assert(bv1);
	assert(bv2);
	assert( BV_SIZE(bv1) == BV_SIZE(bv2));
	bv_to = BV_LAST_WORD(bv1);
	for(bv1 = BV_FIRST_WORD(bv1), bv2 = BV_FIRST_WORD(bv2);
		bv1 <= bv_to;) {
		*bv1 = *bv2;
		bv1++;
		bv2++;
	}
}

Boolean
bv_equal(bv1,bv2)
Bit_vector bv1, bv2;
{
	return memcmp((myVOID *)bv1, (myVOID *)bv2,(BV_WORDS(bv1)+1)*BYTESPERWORD)
		== 0;
}

	/*
	** Use to reset bitvector size
	*/
void
bv_set_size(size, bv)
int size;
Bit_vector bv;
{
	BV_SIZE(bv) = size;
}

	/*
	** Cardinality of set, i. e., number of elements.
	** This is a quick and dirty version.  It should be
	** rewritten using shifts and masks et al.
	*/
int
bv_set_card(bv)
Bit_vector bv;
{
	int sum;
	if(bv == NULL)
		return 0;
	sum = 0;
	BV_FOR(bv,bit)
		sum += 1;
	END_BV_FOR
	return sum;
}
