/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	LOCATION_H
#define	LOCATION_H
#ident	"@(#)debugger:inc/common/Location.h	1.3"

#include	<string.h>

// Location description
// The location class manages any expansion of debug variables
// in location descriptions.

class Frame;
class LWP;

enum LDiscrim	{	lk_none,
			lk_addr,
			lk_stmt,
			lk_fcn,
};

// values for kind
#define	L_UNKNOWN	0x0
#define L_LINE		0x1
#define L_ADDR		0x2
#define L_FUNC		0x4
#define L_VAR		0x80	// value is symbolic

// values for flags
#define L_CHECK_LWP	0x1	// could have file or lwp - must check
#define L_PLUS_OFF	0x2	// symbolic offset - add
#define L_MINUS_OFF	0x4	// symbolic offset - subtract
#define L_STRING	0x8	// file name is pure string - ignore $/%

// components that can be either a symbolic value (e.g, $foo,
// or an integer value, are stored as a character string
class Location {
	short		kind;
	short		flags;
	char		*lwp;
	char 		*file_name;
	union		{
				char		*l_name;
				unsigned long	l_val;
			} locn;
	union		{
				char	*o_name;
				long	o_val;
			} off;
	int		get_int_val(LWP *, Frame *, const char *, 
				unsigned long &);
	int		get_str_val(LWP *, Frame *, const char *, char *&);
public:
			Location() 
				{ memset( (void *)this, 0,
				sizeof(Location) ); }
			Location( Location &l) 
			{ memcpy( (void *)this, 
				(void *)&l, sizeof(Location) ); }
			~Location() {}
	// set values
	void		set_flag(int f) { flags |= f; }
	void		clear_flag(int f) { flags &= ~f; }
	void		set_file(char *f) { file_name = f; }
	void		set_lwp(char *l) { lwp = l; }
	void		set_line(unsigned long l) { locn.l_val = l;
				kind = L_LINE; }
	void		set_line(char *l) { locn.l_name = l;
				kind = L_VAR|L_LINE; }
	void		set_addr(unsigned long a) { locn.l_val = a;
				kind = L_ADDR; }
	void		set_addr(char *a) { locn.l_name = a;
				kind = L_VAR|L_ADDR; }
	void		set_func(char *f)  { locn.l_name = f;
				if (*f  == '%') kind = L_VAR|L_FUNC;
				else kind = L_FUNC; }
	void		set_var(char *v) { locn.l_name = v;
				kind = L_VAR; }
	void		set_offset(unsigned long o) { off.o_val = o; }
	void		set_offset(char *o) { off.o_name = o; }
	// get values
	LDiscrim	get_type();
	int		get_file(LWP *, Frame *, char *&);
	int		get_lwp(LWP *&);
	int 		get_line(LWP *, Frame *, unsigned long &);
	int 		get_addr(LWP *, Frame *, unsigned long &);
	int 		get_func(LWP *, Frame *, char *&);
	int 		get_offset(long &);
	char		*print();
	char		*print(LWP *, Frame *);
};

#endif	// end of Location.h
