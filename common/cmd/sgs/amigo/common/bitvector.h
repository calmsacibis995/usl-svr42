/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)amigo:common/bitvector.h	1.10"

#ifdef BV_DEBUG
#define Arena int
#endif

#ifndef PROTO
#ifdef __STDC__
#define PROTO(x,y) x y
#else
#define PROTO(x,y) x()
#endif
#endif
 
#define EXTRA(slots) (slots <= 16 ? 32 : slots < 100 ? slots*2 : (slots*3)/2)

#define SENTINEL_BIT -1

#define BV_FOR(bv,bit) { \
	Element _elem; \
	Bit bit; \
	for(bit=first_elem(&_elem,(bv));\
	    bit!=SENTINEL_BIT; bit=next_elem(&_elem,(bv))) {

#define BV_FOR_REVERSE(bv,bit) { \
	Element _elem; \
	Bit bit; \
	for(bit=last_elem(&_elem,(bv));\
	    bit!=SENTINEL_BIT; bit=prev_elem(&_elem,(bv))) {

#define END_BV_FOR } }
	/*
	** Next macro is guaranteed to break from BV_FOR if used
	** at the top level of BV_FOR lexically.
	*/
#define BV_BREAK break

typedef enum { false, true } Boolean;
typedef unsigned Word;
typedef Word *Bit_vector;
typedef int Bit;

typedef struct {
	Word *wrdptr;
	short offset; /* element 1 is at offset 31: one less than
			 the minimum number of left shifts we must
			 do to clear the bit */
#ifndef NODBG
	Bit bit;
#endif
} Element; /* represents a set element being processed */

extern Bit PROTO(first_elem,(Element *,Bit_vector));
extern Bit PROTO(last_elem,(Element *,Bit_vector));
extern Bit PROTO(next_elem,(Element *,Bit_vector));
extern Bit PROTO(prev_elem,(Element *,Bit_vector));
extern Bit_vector PROTO(bv_alloc,(int,Arena));
extern void PROTO(bv_init,(Boolean,Bit_vector));
extern void PROTO(bv_set_bit,(Bit,Bit_vector));
extern void PROTO(bv_clear_bit,(Bit,Bit_vector));
extern Boolean PROTO(bv_belongs,(Bit,Bit_vector));
extern void PROTO(bv_or_eq,(Bit_vector,Bit_vector));
extern void PROTO(bv_and,(Bit_vector,Bit_vector, Bit_vector));
extern void PROTO(bv_and_eq,(Bit_vector,Bit_vector));
extern void PROTO(bv_assign,(Bit_vector,Bit_vector));
extern Bit_vector PROTO(bv_minus,(Bit_vector,Bit_vector));
extern void PROTO(bv_minus_eq,(Bit_vector,Bit_vector));
extern void PROTO(bv_assign,(Bit_vector,Bit_vector));
extern void PROTO(bv_print,(Bit_vector));
extern char * PROTO(bv_sprint,(Bit_vector));
extern Boolean PROTO(bv_equal,(Bit_vector,Bit_vector));
extern void PROTO(bv_set_size, (int size, Bit_vector));
extern PROTO(bv_set_card, (Bit_vector));
