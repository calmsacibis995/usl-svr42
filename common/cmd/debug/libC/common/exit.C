#ident	"@(#)debugger:libC/common/exit.C	1.1"

extern void _exit(int);
extern void _cleanup();
extern void dtors();

extern void exit(int i);

extern void exit(int i)
{
	dtors();
	_cleanup();
	_exit(i);
}
