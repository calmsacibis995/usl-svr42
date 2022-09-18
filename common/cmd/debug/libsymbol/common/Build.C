#ident	"@(#)debugger:libsymbol/common/Build.C	1.2"

#include "Syminfo.h"
#include "Build.h"
#include "Protorec.h"
#include <string.h>

#define demangle elf_demangle

#ifdef __cplusplus
extern "C" {
#endif
extern char *demangle(char *);
#ifdef __cplusplus
}
#endif

// COFF, DWARF, ELF common routine to handle names that might
// he mangled C++ names.  This routine will probably be moved 
// to Dwarfbuild when DWARF supports C++.
void
Build::buildNameAttrs(Protorec& protorec, char* name)
{
	char *demangledNm =  demangle(name);
	if( demangledNm == (char *)-1 )
	{
		// actually an internal error
		protorec.add_attr(an_name, af_stringndx, name);
	}
	else if( demangledNm == name )
	{
		protorec.add_attr(an_name, af_stringndx, name);
	}
	else // have a mangled name
	{
		char *permanentDemNm = new char[strlen(demangledNm)+1];
		strcpy(permanentDemNm, demangledNm);
		protorec.add_attr(an_name, af_stringndx, permanentDemNm);
		protorec.add_attr(an_mangledname, af_stringndx, name);
	}
}
 
// These are "pure" virtuals; they will never be called.

int
Build::get_syminfo( long, Syminfo & )
{
	return 0;
}

Attribute *
Build::make_record( long, int )
{
	return 0;
}

long
Build::globals_offset()
{
	return 0;
}
