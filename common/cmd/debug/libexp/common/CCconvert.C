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
#ident	"@(#)debugger:libexp/common/CCconvert.C	1.2"

#include "Machine.h"
#include "TYPE.h"
#include "LWP.h"
#include "Frame.h"
#include "Rvalue.h"
#include "Interface.h"
#include "Value.h"
#include "Symbol.h"
#include "Language.h"
#include "Fund_type.h"
#include "cvt_util.h"
#include "Tag.h"
#include "Debug_var.h"
#include "Proglist.h"


CVT_ACTION ft_cvt_table[ftgMAX][ftgMAX] = {
	
//                                                                old |
//    ftgSINT   ftgUINT  ftgPTR    ftgFP   ftgSTR   ftgOTHER  <-new   v
//--------------------------------------------------------
	cSINT,   cUINT,   cUINT,   cTOFP,   cSTR,   cNEQV,   // ftgSINT
	cSINT,   cUINT,   cUINT,   cTOFP,   cSTR,   cNEQV,   // ftgUINT
	cSINT,   cUINT,   cNULL,   cNEQV,   cSTR,   cNEQV,   // ftgPTR
	cSINT,   cUINT,   cNEQV,   cTOFP,   cNEQV,  cNEQV,   // ftgFP
	cSINT,   cUINT,   cUINT,   cNEQV,   cNULL,  cNEQV,   // ftgSTR
	cNEQV,   cNEQV,   cNEQV,   cNEQV,   cNEQV,  cNEQV,   // ftgOTHER
};

FT_GRP
ft_group(Fund_type ft)
{
	switch (ft) {
	case ft_char:
		return (GENERIC_CHAR_SIGNED ? ftgSINT : ftgUINT);
	case ft_schar:
	case ft_short:
	case ft_sshort:
	case ft_int:
	case ft_sint:
	case ft_long:
	case ft_slong:
		return ftgSINT;
	case ft_uchar:
	case ft_ushort:
	case ft_uint:
	case ft_ulong:
		return ftgUINT;
	case ft_pointer:
		return ftgPTR;
	case ft_sfloat:
	case ft_lfloat:
	case ft_xfloat:
		return ftgFP;
	case ft_string:
		return ftgSTR;
	case ft_none:
	default:
		return ftgOTHER;
	}
}

static int
CCcvt_fund_type(Fund_type oldtype, Fund_type newtype, Rvalue &rval)
{
	if (oldtype == newtype) 
		return 1;

	register FT_GRP oldgrp = ft_group(oldtype);
	register FT_GRP newgrp = ft_group(newtype);

	switch (ft_cvt_table[oldgrp][newgrp])
	{
	case cNULL: return 1;	// done.
	case cNEQV: return 0;	// not convertable.
	case cSINT:
		return( cvt_to_SINT(oldtype, newtype, rval));
	case cUINT:
		return( cvt_to_UINT(oldtype, newtype, rval));
	case cTOFP:
		return( cvt_to_FP(oldtype, newtype, rval) );
	case cSTR:
		return( cvt_to_STR(oldtype, newtype, rval));
	default:
		printe(ERR_internal, E_ERROR, "CCcvt_fund_type", __LINE__);
		return 0;
	}
	// NOTREACHED
}

static int
CCcvt_user_type(TYPE &oldtype, TYPE &newtype, Rvalue &rval)
{
	// get the target type
	Type_form new_form = newtype.form();
	Fund_type new_ft;	// only one of these declarations ...
	Symbol new_ut;		//  ... is used (i.e., new is fund or user type)

	if( newtype.isIntegral(&new_ft, &new_ut) && !new_ut.isnull() )
	{
		// integral user type; can be treated like a fundemental type
		Tag new_tag = new_ut.tag();
		if( new_tag == t_enumtype )
		{
			new_ft = ft_int;
			new_form = TF_fund;
		}
		else if( new_tag==t_bitfield )
		{
                	Attribute *attrPtr;
			if( (attrPtr=new_ut.attribute(an_type)) &&
					attrPtr->form==af_fundamental_type )
                	{
				new_ft = attrPtr->value.fund_type;
				new_form = TF_fund;
			}
		}
		else
		{
			printe(ERR_internal, E_ERROR, "CCcvt_user_type", __LINE__);
			return 0;
		}
	}

	// oldtype is a user type, get the Tag
	Symbol old_ut;
	oldtype.user_type(old_ut);
	Tag old_tag = old_ut.tag();

	switch(old_tag)
	{
	case t_arraytype:
	case t_pointertype:
		if( new_form == TF_fund )
		{
			if(  !CCcvt_fund_type(ft_pointer, new_ft, rval) )
			{
				return 0;
			}
		}
		// other cases here and in the other switch cases are
		// already handled by CCtype_match
		break;
	case t_bitfield:
		if(new_form==TF_fund )
		{
                	Attribute *oldAttrPtr;
			if( !(oldAttrPtr=old_ut.attribute(an_type)) ||
			    oldAttrPtr->form!=af_fundamental_type ||
			    !CCcvt_fund_type(oldAttrPtr->value.fund_type,
								new_ft,rval) )
			{
				return 0;
			}
		}
		break;
	case t_enumtype:
		if( new_form==TF_fund && !CCcvt_fund_type(ft_int,new_ft,rval) )
		{
			return 0;
		}
		break;
	case t_structuretype:
	case t_uniontype:
		// earlier calls (from CCresovle) to CCtype_match insure that 
		// that these  structs of unions are compatiable; no conversion 
		// is necessary
		return 1;
	default:
		return 0;
	}

	return 1;
}

int
CCconvert(Rvalue* rval, TYPE& newtype )
{
	Fund_type ft1, ft2;

	TYPE	oldtype = rval->type();

	if( oldtype.form() == TF_fund )
	{
		oldtype.fund_type(ft1);

		Symbol ut;
		if( newtype.user_type(ut) )
		{	// convert fundemental type to user type
			Tag tag = ut.tag();
			switch( tag )
			{
			case t_arraytype:
			case t_pointertype:
				if(!CCcvt_fund_type(ft1, ft_pointer, *rval))
				{
					printe(ERR_type_convert, E_ERROR);
					return 0;
				}
				break;
			case t_bitfield:
                		Attribute *attrPtr;
				if( !(attrPtr=ut.attribute(an_type)) ||
			     	    attrPtr->form!=af_fundamental_type ||
				    !CCcvt_fund_type(ft1, 
					      attrPtr->value.fund_type, *rval) )
				{
					printe(ERR_type_convert, E_ERROR);
					return 0;
				}
				break;
			case t_enumtype:
				
				if(!CCcvt_fund_type(ft1, ft_int, *rval))
				{
					printe(ERR_type_convert, E_ERROR);
					return 0;
				}
				break;
			default:
				printe(ERR_type_convert, E_ERROR);
				return 0;
			}
		}
		else if( newtype.fund_type(ft2) )
		{	// convert fundemental type to fundemental type
			if( !CCcvt_fund_type(ft1, ft2, *rval) )
			{
				printe(ERR_type_convert, E_ERROR);
				return 0;
			}
		}
		else
		{
			printe(ERR_internal, E_ERROR,  "CCconvert", __LINE__);
			return 0;
		}
	}
	else if( !CCcvt_user_type(rval->type(), newtype, *rval) )
	{
		printe(ERR_type_convert, E_ERROR);
		return 0;
	}

	rval->set_type(newtype);
	return 1;
}
