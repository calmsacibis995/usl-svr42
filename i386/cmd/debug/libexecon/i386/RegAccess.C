#ident	"@(#)debugger:libexecon/i386/RegAccess.C	1.10"

#include "RegAccess.h"
#include "Reg.h"
#include "i_87fp.h"
#include "fpemu.h"
#include "Interface.h"
#include "Procctl.h"
#include "Proctypes.h"
#include "Frame.h"
#include "Fund_type.h"
#include <string.h>
#include <sys/types.h>
#include <sys/procfs.h>
#include <sys/regset.h>
#include <sys/fp.h>

extern RegAttrs regs[]; // to overcome overloaded name problem
extern void extended2double(void *, double *);

static int emulate_only = -1;
extern int _fp_hw;

RegAccess::RegAccess()
{
	pctl = 0;
	core = 0;
	fpcurrent = 0;
}

int
RegAccess::setup_core( Proccore *coreptr )
{
	if (emulate_only == -1)
	{
		// Note: we should actually read the value of _fp_hw from
		// the core file, but it might not be available
		emulate_only =  (_fp_hw < FP_HW);
	}
	core = coreptr;
	if (!core->read_greg(&gpreg))
		return 0;
	fpcurrent = core->read_fpreg(&fpreg);
	return 1;
}


int
RegAccess::setup_live( Proclive *p)
{
	if (emulate_only == -1)
	{
		// Note: we should actually read the value of _fp_hw from
		// the core file, but it might not be available
		emulate_only =  (_fp_hw < FP_HW);
	}
	pctl = p;
	return 1;
}

int
RegAccess::update()
{
	if (pctl)
	{
		fpcurrent = 0;
		// force float state into u-block
		asm("fnop");
		return(pctl->read_greg(&gpreg));
	}
	return 1;
}

int
RegAccess::readlive( RegRef regref, long * word )
{
	int		i;
	struct		fpstate_t dfpstate;
	RegAttrs	*rattrs = regattrs(regref);

	if ( !pctl || !rattrs)
	{
		return 0;
	}
	if (rattrs->flags != FPREG)
	{
		word[0] = gpreg.gregs[regs[regref].offset];
	} 
	else if (!fpcurrent && (pctl->read_fpreg(&fpreg) == 0)) 
	{
		return 0;
	} 
	else 
	{
		fpcurrent = 1;
		memcpy( (char*) &dfpstate,
			(char *)&fpreg.fpregs.fp_reg_set.fpchip_state.state,
			sizeof(dfpstate));
		
		switch(regref)
		{
		case REG_FPSW:
			word[0] = dfpstate.fp_status;
			break;
		case REG_FPCW:
			word[0] = dfpstate.fp_control;
			break;
		case REG_FPIP:
			word[0] = dfpstate.fp_ip;
			break;
		case REG_FPDP:
			word[0] = dfpstate.fp_data_addr;
			break;
		default:
			i = regref - REG_XR0;
			word[2] = 0;
			memcpy((void *)word, &(dfpstate.fp_stack[i].ary[0]),
				EXTENDED_SIZE);
		}
	}
	return 1;
}

int
RegAccess::writelive( RegRef regref, long * word )
{
	int		i;
	struct		fpstate_t dfpstate;
	RegAttrs	*rattrs = regattrs(regref);

	if ( !pctl || !rattrs)
	{
		return 0;
	}

	if (rattrs->flags != FPREG)
	{
		gpreg.gregs[regs[regref].offset] = (int)word[0];
		return(pctl->write_greg(&gpreg));
	} 
	else 
	{
		fpcurrent = 0;
		memcpy( (char*) &dfpstate,
			(char *)&fpreg.fpregs.fp_reg_set.fpchip_state.state,
			sizeof(dfpstate));
		switch(regref)
		{
		case REG_FPSW:
			dfpstate.fp_status = (unsigned int)word[0];
			break;
		case REG_FPCW:
			dfpstate.fp_control = (unsigned int)word[0];
			break;
		case REG_FPIP:
			dfpstate.fp_ip = (unsigned int)word[0];
			break;
		case REG_FPDP:
			dfpstate.fp_data_addr = word[0];
			break;
		default:
			i = regref - REG_XR0;
			memcpy((void *) &(dfpstate.fp_stack[i].ary[0]),
				(void *)word, EXTENDED_SIZE);
			break;
		}
		memcpy( (char*) &fpreg.fpregs.fp_reg_set.fpchip_state.state,
			(char *)&dfpstate, sizeof(dfpstate));
		return(pctl->write_fpreg(&fpreg));
	}
}

Iaddr
RegAccess::getreg( RegRef regref )
{
	long	word[3];

	if (core) 
	{
		if (!readcore(regref, word))
		{
			return 0;
		}
	}
	else 
	{
		if (!readlive(regref, word))
		{
			return 0;
		}
	}
	return word[0];
}

int
RegAccess::readreg( RegRef regref, Stype stype, Itype & itype )
{
	long	word[3];

	if (core) 
	{
		if (!readcore(regref, word))
		{
			return 0;
		}
	}
	else 
	{
		if (!readlive(regref, word))
		{
			return 0;
		}
	}
	switch (stype)
	{
	case SINVALID:	return 0;
	case Schar:	itype.ichar = (char)word[0];		break;
	case Suchar:	itype.iuchar = (unsigned char)word[0];	break;
	case Sint1:	itype.iint1 = (char)word[0];		break;
	case Suint1:	itype.iuint1 = (unsigned char)word[0];	break;
	case Sint2:	itype.iint2 = (short)word[0];		break;
	case Suint2:	itype.iuint2 = (unsigned short)word[0];	break;
	case Sint4:	itype.iint4 = word[0];		break;
	case Suint4:	itype.iuint4 = word[0];		break;
	case Saddr:	itype.iaddr = word[0];		break;
	case Sbase:	itype.ibase = word[0];		break;
	case Soffset:	itype.ioffset = word[0];	break;
	case Sxfloat:	itype.rawwords[2] = (int)word[2];
	case Sdfloat:	itype.rawwords[1] = (int)word[1];
	case Ssfloat:	itype.rawwords[0] = (int)word[0];	break;
	default:	return 0;
	}
	return 1;
}

int
RegAccess::readcore( RegRef regref, long * word )
{
	
	RegAttrs		*rattrs = regattrs(regref);
	int i;

	if ( core == 0 || !rattrs)
		return 0;

	if (rattrs->flags != FPREG)
		*word   = gpreg.gregs[regs[regref].offset];

	else if (fpcurrent == 0)
		return 0;
	else
	{
		struct	fpstate_t dfpstate;

		memcpy( (char*) &dfpstate, (char*)&fpreg.fpregs, sizeof(dfpstate) );
		
		switch(regref)
		{
		case REG_FPSW:
			word[0] = dfpstate.fp_status;
			break;
		case REG_FPCW:
			word[0] = dfpstate.fp_control;
			break;
		case REG_FPIP:
			word[0] = dfpstate.fp_ip;
			break;
		case REG_FPDP:
			word[0] = dfpstate.fp_data_addr;
			break;
		default:
			i = regref - REG_XR0;
			word[2] = 0;
			memcpy((void *)word, &(dfpstate.fp_stack[i].ary[0]),
				EXTENDED_SIZE);
		}
	}
	return 1;
}

int
RegAccess::writereg( RegRef regref, Stype stype, Itype & itype )
{
	long	word[3];

	switch (stype)
	{
		case SINVALID:	return 0;
		case Schar:	word[0] = itype.ichar;		break;
		case Suchar:	word[0] = itype.iuchar;		break;
		case Sint1:	word[0] = itype.iint1;		break;
		case Suint1:	word[0] = itype.iuint1;		break;
		case Sint2:	word[0] = itype.iint2;		break;
		case Suint2:	word[0] = itype.iuint2;		break;
		case Sint4:	word[0] = itype.iint4;		break;
		case Suint4:	word[0] = itype.iuint4;		break;
		case Saddr:	word[0] = itype.iaddr;		break;
		case Sbase:	word[0] = itype.ibase;		break;
		case Soffset:	word[0] = itype.ioffset;	break;
		case Sxfloat:	word[2] = itype.rawwords[2];
		case Sdfloat:	word[1] = itype.rawwords[1];
		case Ssfloat:	word[0] = itype.rawwords[0];	break;
		default:	return 0;
	}
	if (core) 
	{
		printe(ERR_core_write_regs, E_ERROR);
		return 0;
	}
	else 
	{
		return writelive(regref, word);
	}
}


#define NUM_PER_LINE	3

int
RegAccess::display_regs(Frame *frame)
{
	RegAttrs *p;
	Itype	  x;
	int	  i, k, tag;
	struct fpstate_t	dfpstate;
#ifdef NO_LONG_DOUBLE
	int	fpregvals[16];
#endif
	static char *tagname[] = {"VALID","ZERO ","INVAL","EMPTY" };

	i = 1;
	for( p = regs;  !(p->flags & FPREG);  p++ ) 
	{
		if (frame)
			frame->readreg( p->ref, Suint4, x );
		else
			readreg( p->ref, Suint4, x );
		if ( i >= NUM_PER_LINE )
		{
			printm(MSG_int_reg_newline, p->name, x.iuint4);
			i = 1;
		}
		else
		{
			i++;
			printm(MSG_int_reg, p->name, x.iuint4);
		}
	}
	if (core != 0) 
	{
		if (fpcurrent == 0)
		{
			printm(MSG_newline);
			return 0;
		}

		memcpy( (char*) &dfpstate, (char*)&fpreg.fpregs, sizeof(dfpstate) );
	} 
	else if (!fpcurrent && (pctl->read_fpreg(&fpreg) == 0)) 
	{
		printm(MSG_newline);
		return 0;
	} 
	else 
	{
		fpcurrent = 1;
		memcpy( (char*) &dfpstate,
			(char *)&fpreg.fpregs.fp_reg_set.fpchip_state.state,
			sizeof(dfpstate));
	}
#ifdef NO_LONG_DOUBLE
// target system printf doesn't support long double
// convert to double before printing
	for (i = 0; i < 8; i++ )
		extended2double((void *)&dfpstate.fp_stack[i],
			(double *)&fpregvals[i*2]);
#endif

	unsigned int fpsp = dfpstate.fp_status >> 11 & 0x7;
	printm(MSG_int_reg, "%fpsw", dfpstate.fp_status);
	printm(MSG_int_reg_newline, "%fpcw", dfpstate.fp_control);
	printm(MSG_int_reg, "%fpip", dfpstate.fp_ip);
	printm(MSG_int_reg_newline, "%fpdp", dfpstate.fp_data_addr);
	for (i = 0; i < 8 ; i++ )
	{
		char	fpname[15];
		char	fpval[64];
		// registers are ordered differently in emulator from
		// the way the 387 orders them
		k = emulate_only ? i : (fpsp + i) % 8;
		tag = dfpstate.fp_tag >> (k * 2) & 0x3 ;
		sprintf(fpname, "%.5s [ %s ]",regs[FP_INDEX+i].name,
					  tagname[tag]);
#ifdef NO_LONG_DOUBLE
		sprintf(fpval, "0x%.4x %.4x %.4x %.4x %.4x ==\t%.14g",
			*(unsigned short *)&(dfpstate.fp_stack[i].ary[8]),
			*(unsigned short *)&(dfpstate.fp_stack[i].ary[6]),
			*(unsigned short *)&(dfpstate.fp_stack[i].ary[4]),
			*(unsigned short *)&(dfpstate.fp_stack[i].ary[2]),
			*(unsigned short *)&(dfpstate.fp_stack[i].ary[0]),
			*(double *)&fpregvals[i*2]);
#else
// Cfront 1.2 doesn't support long double
// pass the long double as 3 longs
		sprintf(fpval, "0x%.4x %.4x %.4x %.4x %.4x ==\t%.18Lg",
			*(unsigned short *)&(dfpstate.fp_stack[i].ary[8]),
			*(unsigned short *)&(dfpstate.fp_stack[i].ary[6]),
			*(unsigned short *)&(dfpstate.fp_stack[i].ary[4]),
			*(unsigned short *)&(dfpstate.fp_stack[i].ary[2]),
			*(unsigned short *)&(dfpstate.fp_stack[i].ary[0]),
			*(unsigned long *)&(dfpstate.fp_stack[i].ary[0]),
			*(unsigned long *)&(dfpstate.fp_stack[i].ary[4]),
			(unsigned long)*(unsigned short *)&(dfpstate.fp_stack[i].ary[8]));
#endif
		printm(MSG_flt_reg, fpname, fpval);
	}
	return 1;
}

Fund_type
regtype(RegRef ref)
{
    RegAttrs *regattr = regattrs(ref);

    switch(regattr->stype)
    {
	case Suint4:	return ft_pointer;
	case Sxfloat:	return ft_xfloat;
	default:
			return ft_none;
    }
}

int
RegAccess::set_pc(Iaddr addr)
{
	Itype	itype;

	itype.iaddr = addr;
	return(writereg(REG_EIP, Saddr, itype));
}
