#ident	"@(#)sdb:libutil/i386/change_pc.C	1.1"
#include	"utility.h"
#include	"Process.h"
#include	"Interface.h"

#include	"EventTable.h"

int
change_pc( Process * process, Location * location )
{
	Iaddr	addr;
	Itype	itype;

	if ( process == 0 )
	{
		printe("internal error: ");
		printe("process pointer was zero\n");
		return 0;
	}
	else if ( get_addr( process, location, addr ) == 0 )
	{
		return 0;
	}
	else
	{
		itype.iaddr = addr;
		return process->writereg( REG_PC, Saddr, itype );
	}
}
