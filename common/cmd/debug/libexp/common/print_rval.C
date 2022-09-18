/*
 * $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.	All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.	This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 *
 */

#ident	"@(#)debugger:libexp/common/print_rval.C	1.13"

#include "Iaddr.h"
#include "TYPE.h"
#include "Fund_type.h"
#include "Interface.h"
#include "Symbol.h"
#include "Tag.h"
#include "Attribute.h"
#include "Locdesc.h"
#include "Place.h"
#include "Buffer.h"
#include "LWP.h"
#include "Rvalue.h" // for extended floats
#include "Value.h" // for bit fields
#include "cvt_util.h" 
#include "Machine.h" 

#include <string.h>
#include <setjmp.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h> 


enum Chunk_kind {a_long, a_double, a_pointer};

static char lbuf[PRINT_SIZE+400];  // local buffer for sprintfs - we assume
				// cannot request a field width greater
				// than PRINT_SIZE
				// we leave room for %f style formats with
				// large mantissa and exponent

class Print_chunk
{
private:
	LWP		*lwp;
	void		*value;
	int		size;
	char		format;
	char		*format_str;
	Chunk_kind	chunk_kind;
	int		special_type;
	Buffer	*buf;
	
public:

		Print_chunk(LWP * lwp, void * value, int size, TYPE type, 
			char format, char *format_str, Buffer *);

		void Print_next();
		int  Valid() { return(size > 0); }
};

static void print_enum(long value, Symbol type_symbol, int brief, Buffer *buf);
static void print_composite(
	LWP*lwp, Symbol type_symbol, void *value, int brief, Buffer *);
static void print_array(
	LWP*lwp, Symbol type_symbol, void *value, int size, int brief, 
		Buffer *);

static inline int is_string(TYPE & element_type)
{
	Fund_type element_fund_type;
	return element_type.fund_type(element_fund_type)
		&& (element_fund_type == ft_char
		|| element_fund_type == ft_schar
		|| element_fund_type == ft_uchar);
}

static void print_string(char * the_string, int max_length, Buffer *);
static const char * escape_sequence(char c);
static char *get_program_string(LWP *lwp, void **address, int limit = 0);
static void check_for_fp_error(int sig);

char *
print_rvalue(LWP * lwp, void *raw_value, int size, TYPE &type, 
	char format, char *format_str, Buffer *buf)
{
	buf->clear();

	if (format && format != DEBUGGER_FORMAT && 
		format != DEBUGGER_BRIEF_FORMAT)
	{
		Print_chunk chunk(lwp, raw_value, size, type, format, 
			format_str, buf);
		while (chunk.Valid()) 
			chunk.Print_next();
		return (char *)*buf;
	}

	if (type.form() == TF_fund)
	{
		Fund_type fund_type;
		if (!type.fund_type(fund_type))
		{
			printe(ERR_internal, E_ERROR, "print_rvalue",
				__LINE__);
			return 0;
		}
		switch (fund_type)
		{
		case ft_char:
		case ft_schar:
		case ft_uchar:
		{
			int	c;
			char	*p = lbuf;

			if (fund_type == ft_uchar)
				c = *(unsigned char*)raw_value;
			else
				c = *(char*)raw_value;
			p += sprintf(p, "%d", c);
			if (format != DEBUGGER_BRIEF_FORMAT)
			{
				if (isprint(c))
					sprintf(p, " (or '%c')", c);
				else if (c > 6)
				// 0-6 have no real escape sequence
					sprintf(p, " (or '%s')",
						escape_sequence(c));
			}
			buf->add(lbuf);
			break;
	 	}

		case ft_short:
		case ft_sshort:
		case ft_int:
		case ft_sint:
		case ft_long:
		case ft_slong:
		{
			Print_chunk chunk(lwp, raw_value, size, type,
				'd', "%d", buf);
			while (chunk.Valid()) 
				chunk.Print_next();
			break;
		}
		case ft_ushort:
		case ft_uint:
		case ft_ulong:
		{
			Print_chunk chunk(lwp, raw_value, size, type, 
				'u', "%u", buf);
			while (chunk.Valid()) 
				chunk.Print_next();
			break;
		}
		case ft_pointer:
		case ft_void:
		{
			Print_chunk chunk(lwp, raw_value, size, type, 
				'x',"0x%x", buf);
			while (chunk.Valid()) 
				chunk.Print_next();
			break;
		}
		case ft_sfloat:
		case ft_lfloat:
		{
			Print_chunk chunk(lwp, raw_value, size, type, 
				'g', "%g", buf);
			while (chunk.Valid())
				chunk.Print_next();
			break;
		}
		case ft_xfloat:
		{
			Print_chunk chunk(lwp, raw_value, size, type, 
				'g', "%Lg", buf);
			while (chunk.Valid()) 
				chunk.Print_next();
			break;
		}
		case ft_string:
		{
			buf->add('"'); 
			Print_chunk chunk(lwp, raw_value, size, type, 
				's', "%s", buf);
			while (chunk.Valid()) 
				chunk.Print_next();
			buf->add('"');
			break;
		}
		default:
			printe(ERR_internal, E_ERROR, "print_rvalue", 
				__LINE__);
			return 0;
		}
	}
	else if (type.form() == TF_user)
	{
		Symbol type_symbol;
		Tag    type_tag;

		type.user_type(type_symbol);
		if (type_symbol.isnull() ||
			((type_tag = type_symbol.tag()) == t_none))
		{
			printe(ERR_internal, E_ERROR, "print_rvalue", __LINE__);
			return 0;
		}
		switch (type_tag)
		{
		case t_enumlittype:
			type_symbol.null();  // and go on to ..
		case t_enumtype: 
			long enum_value;
			if (size == sizeof(char)) 
				enum_value = *(char*)raw_value;
			else if (size == sizeof(short))
				enum_value = *(short*)raw_value;
			else if (size == sizeof(int))
				enum_value = *(int*)raw_value;
			else if (size == sizeof(long))
				enum_value = *(long*)raw_value;
			else 
			{
				printe(ERR_internal, E_ERROR, 
					"print_rvalue", __LINE__);
				return 0;
			}
			print_enum(enum_value, type_symbol, 
				format==DEBUGGER_BRIEF_FORMAT, buf);
			break;
		case t_pointertype:
		{
			TYPE referenced_type;
			if (type.deref_type(referenced_type)
				&& is_string(referenced_type))
			{
				if (*(void **)raw_value == NULL)
				{
					buf->add("NULL");
					break;
				}
				else
				{
					void *addr = *(void **)raw_value;
					char *str =
						get_program_string(lwp, 
						&addr, 100);
					if (str)
					{
						print_string(str, 
							100, buf);
						break;
					}
				}
			}
			sprintf(lbuf, "0x%x", *(void **)raw_value);
			buf->add(lbuf);
			break;
		}
		case t_structuretype:
		case t_uniontype:
			print_composite(lwp, type_symbol, raw_value,
			format == DEBUGGER_BRIEF_FORMAT, buf);
			break;

		case t_arraytype:
			print_array(lwp, type_symbol, raw_value, size,
			format == DEBUGGER_BRIEF_FORMAT, buf);
			break;

		case t_functiontype:
			if (size != sizeof (void*))
			{
				printe(ERR_internal, E_ERROR, 
					"print_rvalue", __LINE__);
				return 0;
			}
			sprintf(lbuf, "%#x", *(void **)raw_value);
			buf->add(lbuf);
			break;

		default:
			printe(ERR_internal, E_ERROR, "print_rvalue",
				__LINE__);
			return 0;
		}
	}
	else if (type.isnull())
	{
		Print_chunk chunk(lwp, raw_value, size, type, 'X', "%X",
			buf);
		while (chunk.Valid()) 
			chunk.Print_next();
	}
	else
	{
		printe(ERR_internal, E_ERROR, "print_rvalue", __LINE__);
		return 0;
	}
	return((char *)(*buf));
}

Print_chunk::Print_chunk( LWP * lwp, void * value, int size, TYPE type,
	char format, char *format_str, Buffer *buf)
{
	this->lwp = lwp;
	this->value = value;
	this->size = size;
	this->format = format;
	this->format_str = format_str;
	this->buf = buf;
	special_type = 0;
	// Set chunk_kind and special_type
	switch (format)
	{
	case 'c':
	case 'd':
	case 'i':
	case 'u':
	case 'o':
	case 'p':
	case 'x':
	case 'X':
		chunk_kind = a_long;
		if (type.form() == TF_fund)
		{
			Fund_type fund_type;
			if (!type.fund_type(fund_type))
			{
				printe(ERR_internal, E_ERROR, 
					"Print_chunk::Print_chunk",
					__LINE__);
				size = 0;
				return;
			}
			switch (fund_type)
			{
			case ft_char:
				if (GENERIC_CHAR_SIGNED)
					special_type = 1;
				break;
			case ft_schar:
			case ft_short:
			case ft_sshort:
			case ft_int:
			case ft_sint:
			case ft_long:
			case ft_slong:
				special_type = 1;
				break;
			default:
				break;
			}
	 	}
	 	else if (type.form() == TF_user)
	 	{
			Symbol type_symbol;
			type.user_type(type_symbol);
			if (!type_symbol.isnull() && 
				type_symbol.isEnumType())
				special_type = 1;
			break;
	 	}
	 	break;
	case 'e':
	case 'E':
	case 'f':
	case 'g':
	case 'G':
		chunk_kind = a_double;
		int len = strlen(format_str);
		if (type.form() == TF_fund)
		{
			Fund_type fund_type;
			if (!type.fund_type(fund_type))
			{
				printe(ERR_internal, E_ERROR, 
					"Print_chunk::Print_chunk",
					__LINE__);
				size = 0;
				return;
			}
			if ((format_str[len-2] == 'L') ||
				(fund_type == ft_xfloat))
				special_type = 1;
		}
		else
		{
			if (format_str[len-2] == 'L')
				special_type = 1;
		}
		break;
	case 's':
		chunk_kind = a_pointer;
		// = 0 if value is deferenced pointer
		// > 0 is value is program pointer
		if (type.form() == TF_fund)
	 	{
			Fund_type fund_type;
			if (!type.fund_type(fund_type))
			{
				printe(ERR_internal, E_ERROR, 
					"Print_chunk::Print_chunk",
					__LINE__);
				size = 0;
				return;
			}
			if (fund_type != ft_string)
				special_type = 1;
	 	}
		else if (type.form() == TF_user)
	 	{
			Symbol type_symbol;
			type.user_type(type_symbol);
			if (type_symbol.isnull())
				break;
			Tag type_tag = type_symbol.tag();
			switch (type_tag)
			{
			case t_structuretype:
			case t_uniontype:
			case t_arraytype:
				break;
			default:
				special_type = 1;
				break;
			}
		}
	 	break;
	default:
		{
		char	f[4];
		f[0] = '%';
		f[1] = '%';
		f[2] = format;
		f[3] = 0;
		printe(ERR_format_spec, E_ERROR, f);
		chunk_kind = a_long;
		size = 0;
		break;
		}
	}
}

static jmp_buf the_jmp_buf;

void 
Print_chunk::Print_next(void)
{
	int	bytes_used;

	switch (chunk_kind)
	{
	case a_long:
	{
		long the_long;
		if (size < sizeof(short))
		{
			if (special_type) 
				the_long = *(signed char*)value;
			else	
				the_long = *(unsigned char*)value;
			bytes_used = sizeof(char);
	 	}
		else if (size < sizeof(int))
	 	{
			if (special_type) 
				the_long = *(short*)value;
			else
				the_long = *(unsigned short*)value;
			bytes_used = sizeof(short);
		}
	 	else if (size < sizeof(long))
	 	{
			if (special_type) 
				the_long = *(int*)value;
			else	
				the_long = *(unsigned int*)value;
			bytes_used = sizeof(int);
	 	}
		else
	 	{
			if (special_type)
				the_long = *(long*)value;
			else			
				the_long = *(unsigned long*)value;
			bytes_used = sizeof(long);
		}
		sprintf(lbuf, format_str, the_long);
		buf->add(lbuf);
		break;
	}
	case a_double:
	{
		double the_double;
		if (setjmp(the_jmp_buf) != 0)
		{
			clear_fp_error_recovery();
			size = 0;
			return;
		}
		if (size < sizeof(float))
		{
			printe(ERR_float_eval, E_ERROR);
			size = 0;
			return;
		}
		init_fp_error_recovery(check_for_fp_error);
		if (size < sizeof(double))
		{
			the_double = *(float*)value;
			bytes_used = sizeof(float);
		}
		else if (!special_type)
		{
			the_double = *(double*)value;
			bytes_used = sizeof(double);
		}
	 	if (special_type)  // extended float
	 	{
			// assumes that size is correct!
#ifdef NO_LONG_DOUBLE
			// target machine printf doesn't support long double
			extended2double((void *)value, &the_double);
	 		sprintf(lbuf, format_str, the_double);
#else
			char	*tmpbuf;
			tmpbuf = print_extended((void*)value, format, 
				format_str, lbuf);
			if (tmpbuf)
			{
				// tmpbuf is used for %f formats with 
				// long oubles; we allow only PRINT_SIZE
				// chars; the long double may actually
				// be longer
				tmpbuf[PRINT_SIZE] = 0;
				if (strlen(tmpbuf) == PRINT_SIZE)
				{
					printe(ERR_short_read,
						E_WARNING);
				}
				buf->add(tmpbuf);
				delete tmpbuf;
			}
			else
#endif
	 			buf->add(lbuf);
			bytes_used = XFLOAT_SIZE;
		}
		else
		{
			sprintf(lbuf, format_str, the_double);
			buf->add(lbuf);
		}
	 	clear_fp_error_recovery();
		break;
	}
	case a_pointer:
	{
		void * the_pointer;
		if (special_type == 0) // value is chars to be printed
		{
			the_pointer = value;
			bytes_used = size;
		}
		else // value is debugger or program pointer
		{
			if (size < sizeof(void*))
			{
				if (size < sizeof(short))
					the_pointer = 
						(void*)(*(char*)value);
				else if (size < sizeof(int))
					the_pointer =
						(void*)(*(short*)value);
				else if (size < sizeof(long))
					the_pointer = 
						(void*)(*(int*)value);
				else
					the_pointer = *(void **)value;
				bytes_used = size;
			}
			else
			{
				the_pointer = *(void **)value;
				bytes_used = sizeof(void*);
			}
			if (special_type > 0) // program pointer
			{
				void *addr = the_pointer;
				if ((the_pointer =
					get_program_string(lwp, &addr))
					== 0)
				{
					printe(ERR_proc_read, E_ERROR, 
						lwp->lwp_name(), 
						(unsigned long)addr);
					size = 0;
					return;
				}
			}

		}
		if (the_pointer)
		{
			sprintf(lbuf, format_str, the_pointer);
			buf->add(lbuf);
		}
		break;
	}
	}
	value = (char*)value + bytes_used;
	size = size - bytes_used;
	if (size > 0)
		buf->add(" "); // coming back for more...
}

static char *
get_program_string( LWP * lwp, void **address, int limit)
{
	int		actual_limit = limit ? limit : PRINT_SIZE;
	static char	tbuf[PRINT_SIZE];
	char		*bufptr = tbuf;
	void		*addr = *address;
	if (!lwp)
	{
		printe(ERR_no_proc, E_ERROR);
		return 0;
	}
	int current_size = 0;
	while (current_size < actual_limit)
	{
		// this should probably be PRINT_SIZE
		// but we want to avoid hundreds of reads for ptrace
		int char_count = lwp->read((long)addr, 100, bufptr);
		if (char_count <= 0)
		{
			*address = addr;
			return 0;
		}
		else
		{
			bufptr[char_count] = 0; // adds a null character
		}
	
		int temp_len = strlen(bufptr);
		current_size += temp_len;
		if (temp_len != char_count) 
			return tbuf; // read a null
		addr = (char*)addr + char_count;
		bufptr += char_count;
	}
	printe(ERR_short_read, E_WARNING);
	return tbuf;
}

#ifdef __cplusplus
extern void add_error_handler(void(*)(int));
#else
extern void add_error_handler(SIG_HANDLER);
#endif

static void check_for_fp_error(int sig)
{
	if (sig == SIGFPE)
	{
		printe(ERR_float_eval, E_ERROR);
		clear_fp_error();
		sigrelse(sig);
		longjmp(the_jmp_buf, 1);
	}
}

static void
print_enum(long value, Symbol type_symbol, int brief, Buffer *buf)
{
	// Output format:
	// 	literal	brief	output
	//	yes	yes	<literal>
	//	yes	no	<literal> (or <value>)
	//	no	yes	<value>
	//	no	no	<value>
	//
	Attribute *literal = 0;
	if (!type_symbol.isnull())
	{
		Symbol literal_list = type_symbol.child();
		while (!literal_list.isnull())
		{
			literal = literal_list.attribute(an_litvalue);
			if (!literal || literal->form != af_int)
			{
				printe(ERR_internal, E_ERROR,
					"print_enum", __LINE__);
				return;
			}
			if (literal->value.word == value) 
				break;
			literal_list = literal_list.sibling();
		}
		if (literal) 
		{
			buf->add(literal_list.name());
			if (!brief)
				buf->add(" (or ");
		}
	}
	// Print value
	if (!brief || !literal)
	{
		sprintf(lbuf, "%d", value);
		buf->add(lbuf);
		if (literal)
			buf->add(')');
	}
}

static const int indent_increment = 4;
static int indent_count	= 0;

static void
newline(Buffer *buf)
{
	sprintf(lbuf, "\n%*s", indent_count, "");
	buf->add(lbuf);
}

static inline void
indent(Buffer *buf)
{
	indent_count += indent_increment;
	newline(buf);
}

static inline void
outdent(Buffer *buf)
{
	indent_count -= indent_increment;
	newline(buf);
}


static void
print_composite( LWP * lwp, Symbol type_symbol, void * value, 
	int brief, Buffer *buf)
{
	Buffer	tbuf;	// for recursive call to print_rvalue
	buf->add('{');
	if (!brief) 
		indent(buf);
	Symbol member_symbol = type_symbol.child();
	while (!member_symbol.isnull())
	{
		TYPE member_type;
		if (!member_symbol.isMember())
		{
			member_symbol = member_symbol.sibling();
			continue;
		}
		if (!member_symbol.type(member_type)) 
		{
			printe(ERR_internal, E_ERROR, "print_composite"
				, __LINE__);
			break;
		}
		Attribute *member_location = member_symbol.attribute(an_location);
		if (!member_location || member_location->form != af_locdesc)
		{
			printe(ERR_internal, E_ERROR, "print_composite"
				, __LINE__);
			break;
		}
		if (!brief)
		{
			buf->add(member_symbol.name());
			buf->add('=');
		}

		Locdesc member_locdesc;
		member_locdesc = member_location->value.loc;
		Place member_place = member_locdesc.place(0, 0,
			Iaddr(value));
		if (member_symbol.tag() == t_bitfield)
		{
			Buff buff;
			buff.min_size(member_type.size());
			Obj_info bit_field_obj(lwp, 0, 
			member_type, member_symbol, member_place);
			memcpy(buff.ptr(), (void*)member_place.addr,
				member_type.size());
			bit_field_obj.extractBitFldValue(buff,
				member_type.size());
			print_rvalue(lwp, (void*)buff.ptr(),
				member_type.size(), member_type,
				brief ?DEBUGGER_BRIEF_FORMAT :
				(int)DEBUGGER_FORMAT, 
				0, &tbuf);
			buf->add((char *)tbuf);
		}
		else
		{
			Symbol type_symbol;
			Tag    tag;
			if (brief && member_type.user_type(type_symbol)
				&& (((tag = type_symbol.tag())
					== t_structuretype)
				|| tag == t_uniontype
				|| tag == t_arraytype))
				buf->add("{*}");
			else
			{
				print_rvalue(lwp, (void*)member_place.addr, 
				member_type.size(), member_type, 
				brief ?DEBUGGER_BRIEF_FORMAT :
				(int)DEBUGGER_FORMAT, 
				0, &tbuf);
				buf->add((char *)tbuf);
			}
		}

		member_symbol = member_symbol.sibling();
		if (!member_symbol.isnull()) 
			if (brief) 	
				buf->add(", ");
			else
				newline(buf);
	}
	if (!brief)
		outdent(buf);
	buf->add('}');

}

static void
print_array( LWP * lwp, Symbol type_symbol, void * value, int size,
		int brief, Buffer *buf)
{
	Buffer	tbuf;	// for recursive call to print_rvalue
	TYPE element_type;
	if (!type_symbol.type(element_type, an_elemtype))
	{
		printe(ERR_internal, E_ERROR, "print_array", __LINE__);
		return;
	}

	// Determine length of array
	// WARNING: assumes low bound of 0; also assumes
	// high bound is a constant.
	Attribute *high_bound_attr = type_symbol.attribute(an_hibound);
	if (!high_bound_attr || high_bound_attr-> form != af_int) 
	{
		printe(ERR_internal, E_ERROR, "print_array", __LINE__);
		return;
	}

	int high_bound = (int)high_bound_attr->value.word + 1;

	if (is_string(element_type))
	{
		print_string((char*)value, high_bound, buf);
		return;
	}

	int element_size = size/high_bound;

	// Print the elements.
	buf->add('{');
	if (!brief)
		indent(buf);
	for (int element_index = 0; element_index < high_bound;
		element_index++)
	{
		if (!brief) 
		{
			sprintf(lbuf, "[%d]=", element_index);
			buf->add(lbuf);
		}
		print_rvalue(lwp, value, element_size, element_type, 
			brief ? DEBUGGER_BRIEF_FORMAT :
			(int)DEBUGGER_FORMAT, 0, &tbuf);
		buf->add((char *)tbuf);
		value = (char*)value + element_size;
		if (element_index < high_bound - 1) 
			if (brief) 	
				buf->add(", ");
			else	
				newline(buf);
	}
	if (!brief)
		outdent(buf);
	buf->add('}');
}

static void
print_string(char * the_string, int max_length, Buffer *buf)
{
	buf->add('"');
	char * first_char_to_add = the_string;
	while(*the_string && max_length)
	{
		if (!isprint(*the_string))
		{
			char non_printable = *the_string;
			if (the_string != first_char_to_add)
			{
				// Handle characters up to first 
				// character to translate.
				*the_string = 0;
				buf->add(first_char_to_add);
				*the_string = non_printable;
			}
			// Translate character.
			buf->add(escape_sequence(non_printable));
			first_char_to_add = the_string + 1;
		}
		// Next character.
		the_string++, max_length--;
	}
	if (the_string != first_char_to_add)
	{
		char last_char = *the_string;
		*the_string = 0;
		buf->add(first_char_to_add);
		*the_string = last_char;
	}
	if (!max_length) 
		buf->add("...");
	buf->add('"');
}

static const char *
escape_sequence(char c)
{
	switch (c)
	{
	case '\a': return "\\a";
	case '\b': return "\\b";
	case '\f': return "\\f";
	case '\n': return "\\n";
	case '\r': return "\\r";
	case '\t': return "\\t";
	case '\v': return "\\v";
	default:
		{
		static char octal_form[5];
		sprintf(octal_form, "\\%03o", (unsigned char)c);
		return octal_form;
		}
	}
}
