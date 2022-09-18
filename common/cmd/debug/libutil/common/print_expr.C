/*
 * $Copyright: $
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

#ident	"@(#)debugger:libutil/common/print_expr.C	1.3.1.1"

#include "Proglist.h"
#include "Parser.h"
#include "LWP.h"
#include "Expr.h"
#include "Rvalue.h"
#include "Interface.h"
#include "Buffer.h"
#include "global.h"
#include <signal.h>
#include <stdio.h>

static
class Format_string
//
// Scans and prints printf style string.
// The format string may consist of any string with embedded
// printf-style formats; each format has the following form:
// %[0|-|+|#|space][width][.[precision]][l|h|L][specifier]
// width and precision are decimal numbers < PRINT_SIZE
// specifier may be one of:d,i,o,u,x,X,f,e,E,g,G,c,s,p,%
// the specifier ends each format
//
{
private:
#ifdef __cplusplus
	static char	*default_format;
#endif
	char	*format;
	char	*formbuf;	// used to do sprintfs
	char	*next_char_to_print;
	char	*next_specifier;
	char	*next_format;
	char	*first_specifier;
	char	*first_format;

	int	Find_format();
	void	get_flags();
	int	get_decimal();
	void	get_conversion();
	int	get_specifier();
public:

		Format_string(char *format);

	int	First_format();
	char	Next_specifier(void) { return *next_specifier; }
	char	*Next_format(void);
	int	Print_to_next_format(void);
	void	Complete(void);
};

#ifdef __cplusplus
char	*Format_string::default_format = "%? ";
#else
static char	*default_format = "%? ";
#endif

// uses 1st level buffer; Rvalue::print uses 3rd level
int
print_expr(Proclist *procl, char *fmt, Exp *exp)
//
// foreach proc do
//    first format, first expr
//    while format and expr do
//       eval expr (handle error)
//       print expr with format
//       next expr, next format
//    end
//    if format or expr then
//       error
//    end
// end
//
{
	plist	*list;
	LWP  	*lwp;
	int	ret = 1;
	
	if (procl)
	{
		list = proglist.proc_list(procl);
		lwp = list++->p_lwp;
	}
	else
	{
		list = 0;
		lwp = proglist.current_lwp();
	}

	int multiple = list && list->p_lwp;

	sigrelse(SIGINT);
	do // execute once even with no lwp at all
	{
		gbuf1.clear();
		if (multiple)
			printm(MSG_print_header, lwp->lwp_name());
		Format_string format(fmt);
		if (!format.First_format())
		{
			ret = 0;
			break;
		}
		char          specifier = format.Next_specifier();
		char	      *format_str = format.Next_format();
		int           exp_index = 0;
		while (specifier && (*exp)[exp_index])
		{
			Expr	expr((*exp)[exp_index], lwp);
			Rvalue	rvalue;
	    		// Evaluating the expressions may change 
			// current process. "print %proc="p2", %program
	    		if (!list) 
				lwp = proglist.current_lwp();
			if (expr.eval(lwp) && expr.rvalue(rvalue))
			{
				// Print here because of messages from eval 
				// (especially function call)
				char	*val;
				if (!format.Print_to_next_format())
				{
					ret = 0;
					goto out;
				}
				if (rvalue.isnull())
					printe(ERR_internal, E_ERROR, 
						"print_expr", __LINE__);
				else
				{
					val = rvalue.print(list?lwp:
						proglist.current_lwp(), 
						specifier, format_str);
					gbuf1.add(val);
				}
			}
			else
			{
				format.Print_to_next_format();
				ret = 0;
				goto loop;
			}

			exp_index++;
			specifier = format.Next_specifier();
			format_str = format.Next_format();
		}
loop:
		format.Complete();
		if (gbuf1.size() != 0)
			printm(MSG_print_val, (char *)gbuf1);
		if (list)
			lwp = list++->p_lwp;
		else
			lwp = 0;
	} while (lwp && !(interrupt & sigbit(SIGINT))); 
out:
	sighold(SIGINT);
	return ret;
}

Format_string::Format_string(char * format)
{
	if (format && *format) 
	{
		this->format = format;
	}
	else
	{
		default_format[1] = DEBUGGER_FORMAT;
		this->format = default_format;
	}
	formbuf = new char[strlen(this->format) + 1];
	next_char_to_print = this->format;
}

// invariants:
// next_specifier != 0
// first_specifier != 0
// next_format != 0
// first_format != 0
// next_char_to_print != 0
// *first_format == 0 or '%'
// *next_format == 0 or '%'
// next_format >= first_format
// next_specifier >= first_specifier
int
Format_string::First_format()
{
	if (!Find_format())
	{
		printe(ERR_print_format, E_ERROR, format);
		return 0;
	}
	first_specifier = next_specifier;
	first_format = next_format;
	return 1;
}

// find beginning of next format and the format specifier itself
int 
Format_string::Find_format()
{
	next_format = next_char_to_print;
	while(*next_format &&
		(*next_format != '%' || next_format[1] == '%'))
	{
		if (*next_format == '%')
			next_format += 2; // "%%"
		else
			next_format++;
	}
	if (!*next_format)
	{
		next_specifier = next_format;
		return 1;
	}
	next_specifier = next_format + 1;
	// parse format string
	if (!*next_specifier)
		return 0;
	get_flags();
	if (!*next_specifier)
		return 0;
	if (!get_decimal()) // width
		return 0;
	if (!*next_specifier)
		return 0;
	if (*next_specifier == '.')
		next_specifier++;
	if (!*next_specifier)
		return 0;
	if (!get_decimal())  // precision
		return 0;
	if (!*next_specifier)
		return 0;
	get_conversion();
	if (!*next_specifier)
		return 0;
	return(get_specifier());
}

void
Format_string::get_flags()
{
	switch(*next_specifier)
	{
	case '0':
	case '-':
	case '+':
	case '#':
	case ' ':
		next_specifier++;
		break;
	default:
		break;
	}
	return;
}

void
Format_string::get_conversion()
{
	switch(*next_specifier)
	{
	case 'l':
	case 'h':
	case 'L':
		next_specifier++;
		break;
	default:
		break;
	}
	return;
}

int 
Format_string::get_specifier()
{
	switch(*next_specifier)
	{
	case 'E':
	case 'G':
	case 'X':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
	case 'g':
	case 'i':
	case 'o':
	case 'p':
	case 's':
	case 'u':
	case 'x':
	case 'z':
		return 1;
	default:
		return 0;
	}
}

int
Format_string::get_decimal()
{
	int	val = 0;
	int	place = 1;

	if (*next_specifier == '0')
	{
		next_specifier++;
		return 1;
	}
	while((*next_specifier >= '0') && (*next_specifier <= '9'))
	{
		val += place * (*next_specifier - '0');
		place *= 10;
		next_specifier++;
	}
	if (val > PRINT_SIZE)
	{
		printe(ERR_print_width, E_ERROR, val);
		return 0;
	}
	return 1;
}

char *
Format_string::Next_format()
{
	int		len;
	static char	fbuf[14]; // sizeof("%-1024.1024ld")
	len = next_specifier - next_format + 1;
	if (len > 13)
	{
		printe(ERR_internal, E_ERROR, "Format_string::Next_format", __LINE__);
		len = 13;
	}
	strncpy(fbuf, next_format, len);
	fbuf[len] = 0;
	return fbuf;
}

// Print format string up to next beginning of next complete format
// and advance to following format.
// Assumes next_specifier is not null.
int 
Format_string::Print_to_next_format(void)
{
	if (next_char_to_print > next_format)
	{
		// Beyond last format
		// Print end of string before repeating beginning.
		if (*next_char_to_print)
		{
			// make sure we expand %% to %
			sprintf(formbuf, next_char_to_print);
			gbuf1.add(formbuf);
		}
		next_char_to_print = format;
	}

	if (next_char_to_print != next_format)
	{
		*next_format = 0;
		// make sure we expand %% to %
		sprintf(formbuf, next_char_to_print);
		gbuf1.add(formbuf);
		*next_format = '%';
	}
   	// Set up for next print.
	next_char_to_print = next_specifier+1; // specifier ends format

	// Set up for next format and specifier.
	if (!Find_format())
	{
		delete formbuf;
		formbuf = 0;
		printe(ERR_print_format, E_ERROR, format);
		return 0;
	}

	// Repeat format string if it has any specifier
	if (!*next_format)
	{
		next_format = first_format;
		next_specifier = first_specifier;
	}
	return 1;
}

void
Format_string::Complete(void)
{
	if (format == default_format)
		gbuf1.add('\n');
	else if (next_char_to_print != next_format && 
		*next_char_to_print)
	// Print characters to next format or end
	{
		if (*next_format)
		{
			*next_format = 0;
			// make sure we expand %% to %
			sprintf(formbuf, next_char_to_print);
			gbuf1.add(formbuf);
			*next_format = '%';
		}
		else
		{
			// make sure we expand %% to %
			sprintf(formbuf, next_char_to_print);
			gbuf1.add(formbuf);
		}
	}
	delete formbuf;
	formbuf = 0;
}
