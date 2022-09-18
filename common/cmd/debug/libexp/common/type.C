#ident	"@(#)debugger:libexp/common/type.C	1.3"

#include "Buffer.h"
#include "Fund_type.h"
#include "Interface.h"
#include "Language.h"
#include "Symbol.h"
#include "str.h"
#include "TYPE.h"
#include "Tag.h"

#include <string.h>

static const char *
CC_fund_type(Fund_type ftype)
{
	switch(ftype)
	{
	case ft_char:		return "char";
	case ft_schar:		return "signed char";
	case ft_uchar:		return "unsigned char";
	case ft_short:		return "short";
	case ft_ushort:		return "unsigned short";
	case ft_sshort:		return "signed short";
	case ft_int:		return "int";
	case ft_sint:		return "signed int";
	case ft_uint:		return "unsigned int";
	case ft_long:		return "long";
	case ft_slong:		return "signed long";
	case ft_ulong:		return "unsigned long";
	case ft_pointer:	return "void *";
	case ft_sfloat:		return "float";
	case ft_lfloat:		return "double";
	case ft_xfloat:		return "long double";
	case ft_void:		return "void";
	default:		return "";	
	}
}

// CC_type recursively calls itself to create a string representation
// of the type of the symbol
// The complete type string at each level is stored in result
// If the the next level has to prepend something to the type string,
// if creates the new string in work, and then swaps work with result
static Buffer	*result = &gbuf1;
static Buffer	*work = &gbuf2;

#define SWAP(b1, b2)	(temp = b1, b1 = b2, b2 = temp)

static void
CC_type(Symbol s, Attr_name atype)
{
	TYPE		t, t2;
	Fund_type	ftype;
	Symbol		tsym, tsym2;
	Attribute	*attr;
	const char	*name;
	Buffer		*temp;
	

	if (!s.type(t, atype))
		return;
	else if (t.form() == TF_user && t.user_type(tsym))
	{
		if (tsym.isnull())
			return;

		Tag tag = tsym.tag();
		name = tsym.name();
		if (name[0] == '.' && strstr(name, "fake"))
			name = 0;

		switch (tag)
		{
		default:
			if (name)
				result->add(name);
			return;

		case t_pointertype:
			// find out if it is a pointer to a function or array, if so,
			// add parens.  If it is a pointer to anything else
			// the parens are not needed
			work->clear();
			if (tsym.type(t2, an_basetype)
				&& (t2.form() == TF_user)
				&& t2.user_type(tsym2)
				&& ((tsym2.tag() == t_functiontype)
				   ||(tsym2.tag() == t_arraytype)))
			{
				work->add("(*");
				if (result->size())
					work->add((char *)(*result));
				work->add(")");
			}
			else
			{
				work->add("*");
				if (result->size())
					work->add((char *)(*result));
			}
			SWAP(result, work);
			CC_type(tsym, an_basetype);
			break;

		case t_arraytype:	
			{
				char dim[10], *pdim;
				if ((attr = tsym.attribute(an_hibound))
					&& attr->form == af_int)
				{
					sprintf(dim, "[%d]", attr->value.word+1);
					pdim = dim;
				}
				else
					pdim = "[]";
				result->add(pdim);
			}
			CC_type(tsym, an_elemtype);
			break;

		case t_enumtype:
		case t_structuretype:
		case t_uniontype:
			work->clear();
			if (tag == t_structuretype)
				work->add("struct");
			else if (tag == t_enumtype)
				work->add("enum");
			else
				work->add("union");
			if (name)
			{
				work->add(" ");
				work->add(name);
			}
			if (result->size())
				work->add((char *)(*result));
			SWAP(result, work);
			break;

		case t_functiontype:
			result->add("()");
			CC_type(tsym, an_resulttype);
			break;
		}
	}
	else if (t.fund_type(ftype))
	{
		work->clear();
		work->add(CC_fund_type(ftype));
		if (result->size())
			work->add((char *)(*result));
		SWAP(result, work);
	}
}

// lower level routines use 1st and 2nd level global buffers
const char *
print_type(Symbol& s, Language lang)
{
	Tag tag = s.tag();

	result->clear();
	switch(lang)
	{
		case C:
		case CPLUS:
			if (IS_ENTRY(tag))
			{
				result->add("()");
				CC_type(s, an_resulttype);
			}
			else if (tag == t_label)
				result->add("label");
			else
				CC_type(s, an_type);
			break;

		default:
			printe(ERR_internal, E_ERROR, "print_type", __LINE__);
			break;
	}

	if (result->size())
		return (char *)(*result);
	else
		return "";
}
