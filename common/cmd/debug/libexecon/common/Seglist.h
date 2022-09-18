/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Seglist_h
#define Seglist_h
#ident	"@(#)debugger:libexecon/common/Seglist.h	1.5"

#include "Iaddr.h"
#include "Symbol.h"

class NameEntry;
class Object;
class Procctl;
class Proccore;
class Proclive;
class Process;
class Segment;
class Symnode;
class Symtab;

// Maintain list of segments associated with a given process
// and provide access to the segments.

struct Rtl_data;	// opaque to clients

class Seglist {
	char		static_loaded;
	char		uses_rtld;
	short		_has_stsl;
	Segment		*mru_segment;  // most recently used
	Symbol		current_file;
	Symnode		*symnode_file;
	Symnode		*symnode_global;
	Rtl_data	*rtl_data;
	Iaddr           stack_hi, stack_lo;
	Iaddr		start;
	Symnode		*symlist;
	Symnode		*lastsym;
	Process		*proc;
	Segment		*first_segment;
	Segment		*last_segment;
	int		add( int fd, Procctl *, const char *path, 
				int text_only, Iaddr base, Segment * );
	int		add_static_shlib( Object *, Procctl *, int text_only );
	int		add_dynamic_text( Procctl *, const char *exec_name );

	int		add_symnode( Object *, const char * path, Iaddr base );
	int		build_dynamic( Proclive * );
	int		build_static( Proclive *, const char *exec_name );
	int		get_brtbl( const char *objname );
public:
			Seglist( Process *);
			~Seglist();
	int		setup( int fd, int &rtl_used, const char *exec_name);
	int		buildable( Proclive * );
	int		build( Proclive *, const char *exec_name );
	Iaddr		rtl_addr( Proclive * );
	int		readproto( Procctl *txt, Proccore *,
				const char *exec_name);
	Symtab		*find_symtab( Iaddr );
	Symtab		*find_symtab( const char * objname);
	const char	*object_name( Iaddr );
	Segment		*find_segment( Iaddr );
	int		find_source( const char * , Symbol & );
	NameEntry	*first_global();
	Symbol		next_global();
	Symbol		prev_global();
	Symbol		first_file();
	Symbol		next_file();
	Symbol		find_global( const char * );
	int		print_map(Procctl *);
	int		has_stsl() { return _has_stsl; }
	int		in_stack( Iaddr );
	int		in_text( Iaddr );
	void		update_stack( Proclive * );
	Iaddr		start_addr() { return start; };
};

#endif

// end of Seglist.h
