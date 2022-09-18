/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

#ident	"@(#)debugger:libexp/i386/Val_BitF.C	1.3"

#include <string.h>
#include <limits.h>
#include "Interface.h"
#include "Value.h"
#include "Itype.h"
#include "Fund_type.h"
#include "TYPE.h"

int
Obj_info::getBitFldDesc( int *bitFldSz, int *bitFldOffset )
{
	Attribute *attrPtr;

	// get number of bits in bit field
	if( !(attrPtr=entry.attribute(an_bitsize)) || attrPtr->form!=af_int )
	{
		printe(ERR_internal, E_ERROR, "getBitFldDesc", __LINE__);
		return 0;
	}
	*bitFldSz = (int)attrPtr->value.word;
	
	// get bit field offset
	if( !(attrPtr=entry.attribute(an_bitoffs)) || attrPtr->form!=af_int )
	{
		printe(ERR_internal, E_ERROR, "getBitFldDesc", __LINE__);
		return 0;
	}
	*bitFldOffset = (int)attrPtr->value.word;

	return 1;
}

static unsigned int bitmask[33] = {// mask off all but low "n" bits (0<=n<=32)
	0x00000000,
	0x00000001, 0x00000003, 0x00000007, 0x0000000f,	//  1,  2,  3,  4
	0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,	//  5,  6,  7,  8
	0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,	//  9, 10, 11, 12
	0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,	// 13, 14, 15, 16
	0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,	// 17, 18, 19, 20
	0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,	// 21, 22, 23, 24
	0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,	// 25, 26, 27, 28
	0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff,	// 29, 30, 31, 32
};

int
Obj_info::extractBitFldValue(Buff &buff, int size)
{
	int bitFldSz,
	    bitFldOffset; 
	if( !getBitFldDesc(&bitFldSz, &bitFldOffset) )
	{
		return 0;
	}

	// get bit field base type
	Attribute *attrPtr = entry.attribute(an_type);
	if( !attrPtr || attrPtr->form!=af_fundamental_type )
	{
		printe(ERR_internal, E_ERROR, "Obj_info::fixBitFldValue", __LINE__);
		return 0;
	}
	TYPE bitFldTYPE = attrPtr->value.fund_type;

	int nbrBits = size * CHAR_BIT; // nbr of bits in bit field's Fund_type

	unsigned long tmp = 0;
	memcpy(&tmp, buff.ptr(), size);
	tmp = tmp >> (nbrBits - (bitFldSz+bitFldOffset));
	tmp &= bitmask[bitFldSz];

	if( bitFldTYPE.isBitFieldSigned() &&  (tmp & (1<<(bitFldSz-1))) )
	{
		// propogate sign bit
		tmp |= ~bitmask[bitFldSz];
	}
	
	memcpy(buff.ptr(), &tmp, size);
	return 1;
}

int
Obj_info::insertBitFldValue(Rvalue &newRval)
{
	int bitFldSz,
	    bitFldOffset; 
	if( !getBitFldDesc(&bitFldSz, &bitFldOffset) )
	{
		return 0;
	}

	// get new value into an Itype union
	Stype newStype;
	Itype newIval;
	newStype = newRval.get_Itype(newIval);

	// convert new value to unsigned long
	unsigned long	new_ul;

	switch(newStype)
	{
	case Schar: new_ul = newIval.ichar; break;
	case Sint1: new_ul = newIval.iint1; break;
	case Sint2: new_ul = newIval.iint2; break;
	case Sint4: new_ul = newIval.iint4; break;
	case Suchar: new_ul = newIval.iuchar; break;
	case Suint1: new_ul = newIval.iuint1; break;
	case Suint2: new_ul = newIval.iuint2; break;
	case Suint4: new_ul = newIval.iuint4; break;
	}

	// get old raw rvalue; get as bitfield's base type to
	// avoid extractBitFld
	Obj_info oldObj = *this;
	oldObj.entry.null(); // this way get_rvalue won't know its a bitfield

	Rvalue oldRval;
	oldObj.get_rvalue(oldRval);

	// get old value into an Itype union
	Stype oldStype;
	Itype oldIval;
	oldStype = oldRval.get_Itype(oldIval);

	// convert old value to unsigned long
	unsigned long	old_ul;
	int		sz; // size of bit field's type

	switch(oldStype)
	{
	case Schar:
		old_ul = oldIval.ichar; 
		sz = sizeof(char);
		break;
	case Sint1: 
		old_ul = oldIval.iint1; 
		sz = sizeof(char);
		break;
	case Sint2: 
		old_ul = oldIval.iint2; 
		sz = sizeof(short);
		break;
	case Sint4: 	
		old_ul = oldIval.iint4; 
		sz = sizeof(int);
		break;
	case Suchar: 
		old_ul = oldIval.iuchar; 
		sz = sizeof(unsigned char);
		break;
	case Suint1: 
		old_ul = oldIval.iuint1; 
		sz = sizeof(unsigned char);
		break;
	case Suint2: 	
		old_ul = oldIval.iuint2; 
		sz = sizeof(unsigned short);
		break;
	case Suint4: 	
		old_ul = oldIval.iuint4; 
		sz = sizeof(unsigned int);
		break;
	}

	sz *= CHAR_BIT;
	int shift = sz - (bitFldOffset + bitFldSz);

	// clear old bit field value
	old_ul &= ~(bitmask[bitFldSz] << shift);

	// fill in new value;
	new_ul &= bitmask[bitFldSz];
	new_ul <<= shift;
	old_ul |= new_ul;

	newRval.reinit((void *)&old_ul, type.size(), type);
	return 1;
}

int
Obj_info::truncateBitFldValue(Rvalue &newRval)
{
	int bitFldSz,
	    bitFldOffset; 
	if( !getBitFldDesc(&bitFldSz, &bitFldOffset) )
	{
		return 0;
	}

	// get new value into an Itype union
	Stype newStype;
	Itype newIval;
	newStype = newRval.get_Itype(newIval);

	// convert new value to unsigned long
	unsigned long	new_ul;
	switch(newStype)
	{
	case Schar: new_ul = newIval.ichar; break;
	case Sint1: new_ul = newIval.iint1; break;
	case Sint2: new_ul = newIval.iint2; break;
	case Sint4: new_ul = newIval.iint4; break;
	case Suchar: new_ul = newIval.iuchar; break;
	case Suint1: new_ul = newIval.iuint1; break;
	case Suint2: new_ul = newIval.iuint2; break;
	case Suint4: new_ul = newIval.iuint4; break;
	}

	//  mask off extra bits (i.e., truncate value)
	new_ul &= bitmask[bitFldSz];

	newRval.reinit((void *)&new_ul, type.size(), type);
	return 1;
}
