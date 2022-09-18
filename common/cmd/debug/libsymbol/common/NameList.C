#ident	"@(#)debugger:libsymbol/common/NameList.C	1.3"
#include	"Attribute.h"
#include	"NameList.h"
#include	<string.h>

#define demangle elf_demangle
#ifdef __cplusplus
extern "C" {
#endif
extern char *demangle(char *);
#ifdef __cplusplus
}
#endif

NameEntry::NameEntry()
{
	namep = 0;
	form = af_none;
	value.word = 0;
}

NameEntry::NameEntry( const NameEntry & name_entry )
{
	namep = name_entry.namep;
	form = name_entry.form;
	value = name_entry.value;
}

// Make a NameEntry instance
Rbnode *
NameEntry::makenode()
{
	char *	s;

	s = new(char[sizeof(NameEntry)]);
	memcpy(s,(char*)this,sizeof(NameEntry));
	return (Rbnode*)s;
}

//used to do lookup
int
NameEntry::cmpName(const char* s)
{
	int rslt;

	if( !(rslt=strcmp(namep, s)) )
	{
		return rslt;
	}

	// demangled function names are prototyped.  If one of the
	// names is prototyped, compare name up to the first '(' (i.e.,
	// up to the parameter list).  Either this->namep or s could
	// be prototyped, but if they are both prototyped  don't do
	// abbreviated comparision.

	char* namepParenPosition;
	int namepLen;
	namepLen = ( (namepParenPosition=strchr( namep, '(')) ? 
			namepParenPosition - (char*)namep: strlen(namep) );

	char* sParenPosition;
	int sLen;
	sLen = ( (sParenPosition=strchr( s, '(')) ?
			sParenPosition - (char*)s: strlen(s) );

	if( (!namepParenPosition || !sParenPosition) && namepLen==sLen )
		rslt = strncmp(namep,s,sLen);

	return rslt;
}

// used to do insert
int
NameEntry::cmp( Rbnode & t )
{
	NameEntry *	name_entry = (NameEntry*)(&t);

	return ( strcmp(namep,name_entry->namep));
}

void
NameEntry::setNodeName(char* name)
{
	char *demangledNm =  demangle(name);
	if( demangledNm == (char *)-1 )
	{
		// really an error in demangle
		namep = name;
	}
	else if( demangledNm == name )
	{
		namep = name;
	}
	else // have a mangled name
	{
		char *permenentDemNm = new char[strlen(demangledNm)+1];
		strcpy(permenentDemNm, demangledNm);
		namep = permenentDemNm;
	}
}

NameEntry *
NameList::add( const char * s, const Attribute * a )
{
	NameEntry	node;

	node.setNodeName((char *)s);
	node.form = af_symbol;
	node.value.symbol = (Attribute *) a;
	return (NameEntry*)tinsert(node);
}

NameEntry *
NameList::add( const char * s, long w, Attr_form form )
{
	NameEntry	node;

	node.setNodeName((char *)s);
	node.form = form;
	node.value.word = w;
	return (NameEntry*)tinsert(node);
}
