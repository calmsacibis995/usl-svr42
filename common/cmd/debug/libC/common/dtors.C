#ident	"@(#)debugger:libC/common/dtors.C	1.1"

typedef void (*PFV)();

void dtors()
{
	extern PFV _dtors[];
	static ddone;
	if (ddone == 0) {	// once only
		ddone = 1;
		PFV* pf = _dtors;
		while (*pf) pf++;
		while (_dtors < pf) (**--pf)();
	}
}
