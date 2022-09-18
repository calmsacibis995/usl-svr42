#ident	"@(#)debugger:libcmd/common/systable.C	1.2"

#include "Parser.h"
#include "str.h"
#include "TSClist.h"
#include "Machine.h"
#include "Interface.h"

// look up the name that corresponds to the system call entry number
// the table has both gaps and duplicated entry numbers, but the
// entries are in increasing numeric order.  For entries with the
// same number, the first entry has the name for the whole group

const char *
sysname(int sysno)
{
	register syscalls *p;

	if (sysno < 1 || sysno > (int)lastone)
		return 0;

	// walk forwards until there is a match
	// it won't walk off the end of the table since the last
	// entry is 0
	for (p = systable; p->entry; ++p)
		if (p->entry == sysno)
			return p->name;

	return 0;
}

IntList *
getsys( register const char *s )
{
	static int init_done = 0;
	register syscalls *p;

	if ( !init_done ) {
		for ( p = systable ; p->name ; p++ )
			p->name = str(p->name);
		init_done = 1;
	}

	s = strlook(s);		// so can compare ptrs

	for ( p = systable ; p->name; p++ )
		if ( s == p->name )
			break;

	if (p->entry)	// zero if at the end of the table
		return getsys_mach(p->entry);
	else
		return 0;
}

void
dump_syscalls()
{
	register syscalls *p;
	int n = 0;

	for (p = systable; p->name; ++p)
	{
		printm(MSG_sys_name, p->entry, p->name);
		if (!(++n%3))
			printm(MSG_newline);
	}
	if (n%3)
		printm(MSG_newline);
}
