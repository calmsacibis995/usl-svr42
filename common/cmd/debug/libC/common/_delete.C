#ident	"@(#)debugger:libC/common/_delete.C	1.1"
extern free(char*);

extern void operator delete(void* p)
{
	if (p) free( (char*)p );
}
