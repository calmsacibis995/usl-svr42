#ident	"@(#)debugger:libexp/common/Const.C	1.3"


#include <stdio.h>
#include <errno.h>
#include "Interface.h"
#include "Const.h"
#include "fpemu.h"

Const&
Const::init(ConstKind kind, char* s)
{
    const_kind = kind;

    switch (const_kind) {
    case CK_CHAR:
	c = s[0];
	break;

    case CK_INT:
	if (sscanf(s, "%i", &l) != 1) {
	    printe(ERR_int_overflow, E_ERROR, s);
	}
	break;

    case CK_UINT:
	if (sscanf(s, "%ui", &ul) != 1) {
	    printe(ERR_int_overflow, E_ERROR, s);
	}
	break;

    case CK_FLOAT:
	// this code handles float constants like 1.0F as doubles
	if (sscanf(s, "%lg", &d) != 1) {
	    printe(ERR_int_overflow, E_ERROR, s);
	}
	break;
    case CK_XFLOAT:
	errno = 0;
	x = fp_atox(s);
	if (errno)
	    printe(ERR_int_overflow, E_ERROR, s);
	break;
    default:
	printe(ERR_internal, E_ERROR, "Const::init", __LINE__);
    }
    return *this;
}
