#ident	"@(#)debugger:libC/common/_new.C	1.1"

typedef void (*PFVV)();
extern PFVV _new_handler;
extern char* malloc(unsigned);

extern void* operator new(long size)
{
void* _last_allocation;

	while ( (_last_allocation=malloc(unsigned(size)))==0 ) {
		if(_new_handler)
			(*_new_handler)();
		else
			return 0;
	}
	return _last_allocation;
}
