#ident	"@(#)debugger:libexecon/i386/HW_Watch.C	1.2"


#include "Watchlist.h"
#include "Procctl.h"
#include "Proctypes.h"
#include "Event.h"
#include "Iaddr.h"
#include "LWP.h"
#include "Rvalue.h"
#include "TYPE.h"
#include <string.h>
#include <sys/procfs.h>
#include <sys/regset.h>
#include <sys/debugreg.h>

#define MAXSIZE	16	// maximum size of data to watch


// Reads of individual debug registers assume a read_debug has
// been done first. Writes assume they are followed by a write_debug.
class HW_Wdata {
	Proclive	*pctl;
	dbreg_ctl	dbreg;
	Iaddr		db_addr[DR_LASTADDR+1];
	void		*db_expr[DR_LASTADDR+1];
	int		_active;
	int 		read_address(int reg, Iaddr &addr) { 
				addr = dbreg.dbregs.debugreg[reg]; return 1; }
	int		write_address(int reg, Iaddr addr) { 
				dbreg.dbregs.debugreg[reg] = (unsigned int)addr;
				return 1; }
	int		write_control(unsigned int ctl) { 
				dbreg.dbregs.debugreg[DR_CONTROL]=ctl; return 1; }
	int		read_control(unsigned int & ctl) { 
				ctl = dbreg.dbregs.debugreg[DR_CONTROL]; return 1; }
	int		clear_status() { dbreg.dbregs.debugreg[DR_STATUS] = 0;
				return 1; }
	int		get_status(unsigned int &status) { status = 
				dbreg.dbregs.debugreg[DR_STATUS]; return 1; }
	int		read_debug() { return(pctl->read_dbreg(&dbreg)); }
	int		write_debug() { return(pctl->write_dbreg(&dbreg)); }
public:
			HW_Wdata();
			~HW_Wdata();
	int		set_wpt(Iaddr, unsigned long, Proclive *, void *);
	int 		remove_wpt(Iaddr, Proclive *, void *);
	int 		triggered(Proclive *);
};

HW_Wdata::HW_Wdata()
{
	memset((char *)this, 0, sizeof(*this));
}

HW_Wdata::~HW_Wdata()
{
	// turn off all watchpoints
	write_control(0);
	clear_status();
	write_debug();
}

HW_Watch::HW_Watch()
{
	hwdata = new HW_Wdata();
}

HW_Watch::~HW_Watch()
{
	delete hwdata;
}

int
HW_Watch::hw_fired(LWP *p)
{
	if (!hwdata)
		return 0;
	return hwdata->triggered(p->proc_ctl());
}

int
HW_Watch::add(Iaddr pl, Rvalue *rval, LWP *p, void *watchexpr)
{
	unsigned long	sz;
	TYPE		t;

	t = rval->type();	// previous value for watchpoint
	if ((t.isnull()) // can't get type info
		|| ((sz = t.size()) > MAXSIZE)) // can't watch 
						// item this big
		return 0;
	return(hwdata->set_wpt(pl, sz, p->proc_ctl(), watchexpr));
}

int
HW_Watch::remove(Iaddr place, LWP *p, void *watchexpr)
{
	if (!hwdata)
	{
		return 0;
	}
	return(hwdata->remove_wpt(place, p->proc_ctl(), watchexpr));
}

int
HW_Wdata::set_wpt(Iaddr addr, unsigned long size, Proclive *p, void *watchexpr)
{
	unsigned int	ctl;
	int		i;
	int		reg[DR_LASTADDR+1], avail[DR_LASTADDR+1];
	Iaddr		oaddr, naddr;

	oaddr = addr;

	pctl = p;
	if ((!read_debug())
		|| (!read_control(ctl)))
		return 0;

	// find available registers
	for (i = DR_FIRSTADDR; i <= DR_LASTADDR; i++)
	{
		Iaddr	a;
		if (read_address(i, a) == 0)
			return 0;
		reg[i] = 0;
		if (a == 0)
			avail[i] = 1;
		else
			avail[i] = 0;
	}

	i = DR_FIRSTADDR;
	while(size > 0)
	{
		int	len;
		
		if ((size >= sizeof(int)) && !(addr & (sizeof(int)-1)))
		{
			len = DR_LEN_4;
			size -= sizeof(int);
			naddr = addr + sizeof(int);
		}
		else if ((size >= sizeof(short)) && 
			!(addr & (sizeof(short)-1)))
		{
			len = DR_LEN_2;
			size -= sizeof(short);
			naddr = addr + sizeof(short);
		}
		else
		{
			len = DR_LEN_1;
			size -= 1;
			naddr = addr + 1;
		}
		// find first avail
		while(i <= DR_LASTADDR)
		{
			if (avail[i])
				break;
			i++;
		}
		if (i > DR_LASTADDR)
			return 0;
		
		// set up ctl register
		ctl |= (DR_LOCAL_SLOWDOWN | (1<<(i * DR_ENABLE_SIZE)));
		ctl |= (((DR_RW_WRITE | len) << 
			(i * DR_CONTROL_SIZE)) << DR_CONTROL_SHIFT);
		
		reg[i] = (int)addr;
		db_addr[i] = oaddr;
		db_expr[i] = watchexpr;
		addr = naddr;
	}
	if (size > 0)
		return 0;  // not enough available registers
	
	for (i = DR_FIRSTADDR; i <= DR_LASTADDR; i++)
	{
		if (reg[i])
		{
			if (!write_address(i, (Iaddr)reg[i]))
				return 0;
		}
	}
	if ((!write_control(ctl)) || (!write_debug()))
		return 0;
	_active = 1;
	return 1;
}

int
HW_Wdata::remove_wpt(Iaddr addr, Proclive *p, void *watchexpr)
{
	unsigned int	ctl;
	int		i;
	int		found = 0;

	pctl = p;

	if (!read_debug())
		return 0;

	if (!read_control(ctl))
		return 0;

	for (i = DR_FIRSTADDR; i <= DR_LASTADDR; i++)
	{
		if ((db_addr[i] != addr) || (db_expr[i] != watchexpr))
			continue;
		// set up ctl register
		ctl &= ~(1 << (i * DR_ENABLE_SIZE));
		ctl &= ~(((0xF) << (i * DR_CONTROL_SIZE)) << 
			DR_CONTROL_SHIFT);

		if (!write_address(i, 0))
			return 0;
		db_expr[i] = 0;
		db_addr[i] = 0;
		found++;
	}

	if (!ctl)
		_active = 0;
	if (!found)
		return 0;
	if ((!write_control(ctl)) || (!write_debug()))
		return 0;
	return 1;
}

int 
HW_Wdata::triggered(Proclive *p)
{
	unsigned int	status;

	if (!_active)
		return 0;
	pctl = p;
	if (!read_debug())
		return 0;
	if (!get_status(status))
		return 0;
	for(int i = DR_FIRSTADDR; i <= DR_LASTADDR; i++)
	{
		if (status & (1 << i))
		{
			clear_status();
			return 1;
		}
	}
	return 0;
}
