#ident	"@(#)debugger:libexp/common/ParsedRep.C	1.2"

#include "TYPE.h"
#include "List.h"
#include "LWP.h"
#include "Value.h"
#include "Resolver.h"
#include "ParsedRep.h"

// null base clase versions of virtual functions; C++ 1.2
// does not allow pure virtual functions

Value *
ParsedRep::eval(Language, LWP *, Frame *, int)
{
	return 0; 
}

ParsedRep *
ParsedRep::clone()	// make deep copy
{
	return 0; 
}

int 
ParsedRep::triggerList(Language, LWP *, Resolver *, List &)
{
	return 0;
}

int 
ParsedRep::exprIsTrue(Language, LWP *, Frame *)
{
	return 0;
}

int 
ParsedRep::getTriggerLvalue(Place&)
{
	return 0;
}

int 
ParsedRep::getTriggerRvalue(Rvalue&)
{
	return 0;
}

ParsedRep* 
ParsedRep::copyEventExpr(List&, List&, LWP*)
{
	return 0;
}
