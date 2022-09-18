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
#ident	"@(#)debugger:libexp/common/Rvalue.C	1.5"

#include <string.h>
#include "Rvalue.h"
#include "Buffer.h"
#include "Fund_type.h"
#include "Interface.h"
#include "Itype.h"
#include "Language.h"
#include "Machine.h"
#include "cvt_util.h"
#include "LWP.h"
#include "Debug_var.h"
#include "Value.h"
#include "TYPE.h"
#include "Proglist.h"
#include <stdio.h>

extern char *
print_rvalue(LWP * lwp, void * raw_value, int size, TYPE & type, 
	char format, char *format_str, Buffer *);

Rvalue::Rvalue(void *basep, int len, TYPE& t, LWP *lwp) : _type(t)
{
	raw_bytes.add(basep, len);
	_lwp = lwp;
}

Rvalue::Rvalue(Rvalue& v) : raw_bytes(v.raw_bytes), _type(v._type)
{
	_lwp = v._lwp;
}

int
Rvalue::operator==(Rvalue& v)
{
	register int len = raw_bytes.size();

	return v._lwp==_lwp && len==v.raw_bytes.size() &&
	     memcmp((char *)raw_bytes.ptr(), (char *)v.raw_bytes.ptr(), len)==0;
}

Rvalue&
Rvalue::operator=(Rvalue& v)
{
	raw_bytes = v.raw_bytes;
	_type     = v._type;
	_lwp = v._lwp;
	return *this;
}

static Buff	dbuf;

static char *
dumphexbytes( void *data,  register int len )
{
	register unsigned char	*p = (unsigned char *) data;
	char			*b;
	register int		cnt = 0;
	
	dbuf.min_size((len * 3) + 2); // 2 chars + space or newline 
				     // per byte, plus terminating null and newline
	b = dbuf.ptr();

	while ( len-- > 0 ) {
		b += sprintf(b, "%02x", *p++);
		if ( (cnt++ & 7) == 7 )
			*b++ = '\n';
		else
			*b++ = ' ';
	}
	if ( cnt & 7 )
		*b++ = '\n';
	*b = 0;
	return dbuf.ptr();
}

// takes a printf-like format string and the format char;
// we assume a maximum requested field width of PRINT_SIZE
//
// uses 3rd level global buffer; print_type uses 1st and 2nd
// levels; print expr uses 1st level
char *
Rvalue::print( LWP * lwp, char format, char *format_str)
{
	register int   len   = raw_bytes.size();
	register char *basep = (char *)raw_bytes.ptr();
	LWP *localLwp = _lwp? _lwp: lwp;

	if (!format) format = DEBUGGER_FORMAT;

	if (_type.isnull()) 	// dump as "raw bytes".
		return dumphexbytes( basep, len );
	else
		return print_rvalue(localLwp, basep, len, _type, format, format_str, &gbuf3);
}

Stype
Rvalue::get_Itype(Itype& val)
{
	int    byte_size = _type.size();
	Stype  stype     = SINVALID;

	if (_type.get_Stype(stype))
	{
		if (stype == Sdebugaddr)
			val.idebugaddr = (char *) raw_bytes.ptr();
		else
			if (byte_size > 0)
				memcpy((char *)&val, (char *)raw_bytes.ptr(),
								    byte_size);
	}
	return stype;
}

void
Rvalue::reinit(void *p, int n, TYPE& t, LWP *lwp)
{
	raw_bytes.clear().add(p, n);
	_type = t;
	_lwp = lwp;
}

Fund_type
fund_type(Stype stype)
{
	switch( stype )
	{
	case SINVALID:	return ft_none;
	case Schar:	return ft_char;
	case Sint1:	return ft_schar;
	case Sint2:	return ft_sshort;
	case Sint4:	return ft_sint;
	case Suchar:	return ft_uchar;
	case Suint1:	return ft_uchar;
	case Suint2:	return ft_ushort;
	case Suint4:	return ft_uint;
	case Ssfloat:	return ft_sfloat;
	case Sdfloat:	return ft_lfloat;
	case Sxfloat:	return ft_xfloat;
	case Saddr:	return ft_ulong;
	case Sdebugaddr:	return ft_string;
	case Sbase:	return ft_ulong;
	case Soffset:	return ft_ulong;
	}
}

Rvalue::Rvalue( Stype stype, Itype &itype, LWP *lwp)
{
	_type = fund_type( stype );
	_lwp = lwp;
	raw_bytes.add( (void *)&itype, _type.size() );
}

int
Rvalue::convert(TYPE& newtype, Language lang )
{
	switch (lang)
	{
	case C:
	case CPLUS:
		return CCconvert(this, newtype);
	case UnSpec:
	default:
		printe(ERR_internal, E_ERROR, "Rvalue::convet", __LINE__);
		return 0;
	}
}

int
Rvalue::assign(Obj_info& obj, Language lang)
{
	if (! obj.type.isnull()) 
	{  // obj.type.isnull(), assume rhs type.
		switch (lang)
		{
		case C:
		case CPLUS:
			if (! CCconvert(this, obj.type))
			{
				return 0;
			}
			break;
		case UnSpec:
		default:
			printe(ERR_internal, E_ERROR, "Rvalue::assign", __LINE__);
			return 0;
		}
	}

	Vector tmpBitFldVal;
	TYPE tmpBitFldTYPE;
	if( !obj.entry.isnull() && obj.entry.tag()==t_bitfield)
	{
		obj.truncateBitFldValue(*this);
		// the Rvalue object will contain the value of memory
		// surrounding the bitfield so that no other data is
		// changed when the bitfield is written; save the bit field
		// value so it can be restored.
		tmpBitFldVal = raw_bytes;
		tmpBitFldTYPE = _type;
		obj.insertBitFldValue(*this);
	}

	if (obj.lwp == 0 && obj.loc.kind != pDebugvar)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	switch (obj.loc.kind)
	{
		case pAddress:
			int n_bytes   = raw_bytes.size();
			int n_written = obj.lwp->write(obj.loc.addr,
						   raw_bytes.ptr(), n_bytes);
			if (n_written != n_bytes)
			{
				printe(ERR_proc_write, E_ERROR, 
					obj.lwp->lwp_name(), obj.loc.addr);
			}
			break;
		case pRegister:
			Itype ival;
			Stype itype = get_Itype(ival);
	
			if (itype == SINVALID)
			{
				printe(ERR_internal, E_ERROR, "Rvalue::assign", __LINE__);
				return 0;
			}
			if (obj.frame->writereg(obj.loc.reg, itype, ival) == 0)
			{
				printe(ERR_write_reg, E_ERROR,
					obj.lwp->lwp_name());
				return 0;
			}
			break;
		case pDebugvar:
			LWP * current_lwp = proglist.current_lwp();
			if (((Debug_var*)obj.loc.var)->
			   write_value(raw_bytes.ptr(), raw_bytes.size())
			   && current_lwp != proglist.current_lwp())
			// If this changed the current lwp, then the
			// value needs to be updated.
			{
			   ((Debug_var*)obj.loc.var)->
			       set_context(
				   proglist.current_lwp(),
			           proglist.current_lwp()->curframe());
			}
			break;
			// write_value reports the error
		default:
			printe(ERR_internal, E_ERROR, "Rvalue::assign", __LINE__);
			return 0;
		}
	if( !obj.entry.isnull() && obj.entry.tag()==t_bitfield)
	{
		// the Rvalue object contains the value of the bit field 
		// embedded in the memory surrounding it. Restore extracted
		// bit field value.
		reinit(tmpBitFldVal.ptr(), tmpBitFldVal.size(), tmpBitFldTYPE);
	}

	return 1;
}
