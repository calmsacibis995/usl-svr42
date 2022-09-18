#ident	"@(#)debugger:libexecon/i386/Frame.C	1.6.1.3"

// Frame.C -- stack frames and register access, i386 version

#include "Reg.h"
#include "Frame.h"
#include "RegAccess.h"
#include "LWP.h"
#include "Interface.h"
#include "Symtab.h"
#include "Attribute.h"
#include "Procctl.h"
#include "Instr.h"
#include <string.h>

/*
 *  This code makes some assumptions about what a "standard"
 *  stack frame looks like.  This is compiler-dependent,
 *  but follows the conventions set down in the 386 ABI.
 *  Registers %esi, %edi and %ebx are callee saved,
 *  and are preserved in the stack frame.
 *  The standard stack looks like this:
 *-----------------------------------------------------*
 *  4n+8(%ebp)	arg word n	|	High Addresses *
 *  		...		|		       *
 *     8(%ebp)	arg word 0	|	Previous frame *
 *-----------------------------------------------------*
 *     4(%ebp)	return addr	|		       *
 *     0(%ebp)	previous %ebp	|	Current frame  *
 *    -4(%ebp)	x words local	|		       *
 *   		...		|		       *
 *   -4x(%ebp)	0th local	|		       *
 *     8(%esp)  caller's %edi	|	(if necessary) *
 *     4(%esp)  caller's %esi	|	  "  "         *
 *     0(%esp)  caller's %ebx	|	Low Addresses  *
 *-----------------------------------------------------*
 *
 * While there is no "standard" function prolog, for heuristic
 * purposes we assume a prolog has the following sequence of
 * instructions:
 * 1)
 *   pushl	%ebp		/ save old frame pointer
 * 2)
 *   movl	%esp,%ebp	/ set new frame pointer
 * 3)
 *   
 * At position 1), %esp points to the return address
 * At position 2), 4(%esp) points to the return address
 * At position 3), 4(%esp) or 4(%ebp) point to the return address
 *
 * For functions returning structures, an extra "argument"
 * is pushed on the stack, the address where the callee should
 * put the structure being returned.
 * In this case there are 2 extra instructions before the standard
 * prolog:
 * 1)
 *  popl	%eax		/ save the return address in eax
 * 2)
 *  xchgl	0(%esp), %eax   / put return address at stack top
 *				/ and save structure address
 *
 * In either case, the prolog may come at the first instruction
 * of the function, or may be the target of a jump from the
 * first instruction
 */

//
// offsets to saved regisers in signal handler stack frame
// very much implementation dependent
//
#define SIG_EIPOFF	0x44
#define SIG_EBPOFF	0x24
#define SIG_ESPOFF     	0x50

// The access vector (accv) and saved register vector (saved_regs)
// work together to locate registers for a particular stack frame.
// accv[i] contains the stack address at which register i is saved,
// or 0 if the register is not saved on the stack.
// %esp is a special case: accv[REG_ESP] holds the actual register
// contents, not the address at which it is saved.
//
// saved_regs is filled in the by the Instr::fcn_prolog().
// saved_regs[0] contains the offset from the current frame
// pointer where user registers are saved.  saved_regs[1-3]
// contain the index of the register saved at word 1,2,3, 
// respectively of that offset.  So if saved_regs[1] == 
// REG_EDX, then %edx is saved at current %ebp - saved_regs[0] -
// 1*sizeof(int).
// For stack frames that do not set a frame pointer, then the offset
// is off %esp, rather than %ebp.
//
// prevebp and ebp are used to save the frame pointer when it
// is calculated outside of the normal means; i.e. when we have
// no symbol information or no real frame

struct framedata {
	Iaddr		accv[NGREG];	// "access vector"
	RegRef		saved_regs[NGREG];	// saved registers
	int		nargwds;
	Iaddr		prevpc;
	Iaddr		ebp;
	Iaddr		prevebp;
	short		noprolog;
	short		prolog_size;
			framedata();
			~framedata() {}
};

framedata::framedata()
{
	for ( register int i = 0; i < NGREG ; i++ ) 
	{
		accv[i] = 0;
		saved_regs[i] = 0;
	}
	noprolog = -1;
	ebp = (Iaddr)-1;
	prevebp = (Iaddr)-1;
	prolog_size = 0;
	prevpc = (Iaddr)-1;
	nargwds = -1;
}

struct frameid {
	Iaddr ebp;
};

FrameId::FrameId(Frame *frame)
{
	id = 0;
	if ( frame ) 
	{
		id = new frameid;
		id->ebp = frame->getreg( REG_EBP );
	}
}

FrameId::~FrameId()
{
	delete id;
}

void
FrameId::null()
{
	delete id;
	id = 0;
}

FrameId &
FrameId::operator=( FrameId & other )
{
	if ( other.id == 0 && id == 0 )
		return *this;
	else if ( other.id == 0 )
	{
		delete id;
		id = 0;
		return *this;
	}
	else if ( id == 0 ) id = new frameid;
	*id = *other.id;
	return *this;
}

#ifdef DEBUG
void
FrameId::print( char * s )
{
	if (s ) printf(s);
	if ( id == 0 )
		printf(" is null.\n");
	else
		printf(" ebp is %#x",id->ebp);
}
#endif

int
FrameId::operator==( FrameId& other )
{
	if ( (id == 0) && ( other.id == 0 ) )
		return 1;
	else if ( id == 0 )
		return 0;
	else if ( other.id == 0 )
		return 0;
	else if ( id->ebp != other.id->ebp )
		return 0;
	else
		return 1;

}

int
FrameId::operator!=( FrameId& other )
{
	return ! (*this == other);
}

Frame::Frame( LWP *nlwp )
{
	DPRINT(DBG_EXECON,("new topframe() == %#x\n", this));

	data = new framedata;
	level  = 0;
	lwp = nlwp;
	epoch = lwp->p_epoch();	// epoch never changes

}

Frame::Frame( Frame *prev )
{
	data = new framedata;
	append( prev );
	for ( register int i = 0; i < NGREG ; i++ ) 
	{
		data->accv[i] = prev->data->accv[i];
	}
	data->ebp = prev->data->ebp;
	lwp = prev->lwp;
	level = prev->level + 1;
	epoch = prev->epoch;
	DPRINT(DBG_EXECON,("new next frame(%#x) == %#x\n", prev, this));
}

Frame::~Frame()
{
	DPRINT(DBG_EXECON,("%#x.~Frame()\n", this));
	unlink();
	delete data;
}
int		
Frame::valid()
{
	return(this && data && epoch == lwp->p_epoch()); 
}

FrameId
Frame::id()
{
	FrameId *fmid = new FrameId(this);
	return *fmid;
}

Frame *
Frame::caller()
{
	Iaddr	pc, ebp, sp;
	Itype	itype;
	Symbol	sym;
	char	*name = 0;

	DPRINT(DBG_EXECON,("%#x.caller()\n", this));

	Frame *p = (Frame *) next();
	if ( p ) 
		return p;
	// try to construct  a new frame
	DPRINT(DBG_EXECON,("no next, building it\n"));

	pc = getreg(REG_EIP);
	DPRINT(DBG_EXECON,("%#x.caller() pc == %#x\n", this, pc));

	if ( !lwp->in_text(pc) ) 
		return 0;
	
	// retaddr sets noprolog and saved_regs
	if (!retaddr(data->prevpc, sp))
	{
		data->prevpc = (Iaddr)-1;
		return 0;
	}
	DPRINT(DBG_EXECON,("%#x.caller() prevpc = %#x\n", this, data->prevpc));


	if (data->noprolog == 0)
	{
		sp = getreg(REG_ESP);
		ebp = getreg(REG_EBP);
		(void)lwp->read(ebp , Saddr, itype);
		DPRINT(DBG_EXECON,("%#x.caller() ebp == %#x\n", this, ebp));
		DPRINT(DBG_EXECON,("%#x.caller() sp == %#x\n", this, sp));
		if ( !lwp->in_stack(ebp))
			return 0;
	}

	p = new Frame(this);

	//
	// if prevpc  == _sigreturn or _sigacthandler, 
	// the current frame is a 
	// signal handler, and the caller's context was pushed on the
	// stack by the kernel.
	//
	sym = lwp->find_entry(data->prevpc);
	if (!sym.isnull())
	{
		name = lwp->symbol_name(sym);
	}	
	if (name && (strcmp(name, "_sigreturn") == 0))
	{
		// old style signal handler
		// kernel pushes return address, esp and ebp on
		// stack
		p->data->accv[REG_EIP] = ebp + SIG_EIPOFF;
		p->data->accv[REG_EBP] = ebp + SIG_EBPOFF;
		(void)lwp->read(ebp + SIG_ESPOFF, Saddr, itype);
		p->data->accv[REG_ESP] = itype.iaddr;
	}
	else if (name && (strcmp(name, "_sigacthandler") == 0))
	{
		// we have a new style signal handler
		// can't deal with it at the moment
		p->unlink();
		delete p;
		return 0;
	}
	else 
	if (data->noprolog) 
	{
		int i, saved_regs_off;

		saved_regs_off = data->saved_regs[0];
		for (i = 1; data->saved_regs[i] != 0; i++) 
		{
			p->data->accv[data->saved_regs[i]] = 
				sp - saved_regs_off - i*sizeof(int);
		}

		p->data->accv[REG_EIP] = sp;
		p->data->accv[REG_ESP] = sp + 4; 
		if (level == 0)
		{
			p->data->accv[REG_EBP] = (Iaddr)-1;
			p->data->ebp = getreg(REG_EBP);
		}
		else if (data->prevebp != -1)
		{
			p->data->accv[REG_EBP] = (Iaddr)-1;
			p->data->ebp = data->prevebp;
		}
	}
	else 
	{
		int i, saved_regs_off;

		p->data->accv[REG_EIP] = ebp + 4;
		p->data->accv[REG_EBP] = ebp;
		p->data->accv[REG_ESP] = ebp + 8;
		//
		// set saved registers
		//
		saved_regs_off = data->saved_regs[0];
		for (i = 1; data->saved_regs[i] != 0; i++) 
		{
			p->data->accv[data->saved_regs[i]] = 
				ebp - saved_regs_off - i*sizeof(int);
		}

	}
	DPRINT(DBG_EXECON,("%#x.caller() accv[PC] = %#x, [AP] = %#x, [FP] = %#x [ESP]=%#x\n", this,
		p->data->accv[REG_EIP], p->data->accv[REG_EBP],
		p->data->accv[REG_EBP], p->data->accv[REG_ESP]));

	return p;
}

// either read from lwp directly, or from saved locations
// on the stack, using the addresses saved in the access vector
int
Frame::readreg( RegRef which, Stype what, Itype& dest )
{
	if (( level == 0 )  ||
		(which > REG_EIP) ||
		( !data->accv[which] ))
	{
		return lwp->readreg( which, what, dest );
	} 
	else 
	{	// on stack, possibly
		if ( which == REG_ESP )
		{ 
			// special case
			dest.iaddr = (Iaddr)
				data->accv[which];
			return 1;
		}
		else if ( which == REG_EBP && data->accv[REG_EBP] 
			== (Iaddr)-1)
		{
			dest.iaddr = data->ebp;
			return 1;
		}
		return lwp->read(data->accv[which] , what, dest);
	}
}

int
Frame::writereg( RegRef which, Stype what, Itype& dest )
{
	if (( level == 0 )  ||
		(which > REG_EIP) ||
		( !data->accv[which] ))
	{
		return lwp->writereg(which, what, dest);
	}
	else
	{
		if ( which == REG_ESP )
		{ 
			// special case
			data->accv[which] = dest.iaddr;
			return 1;
		}
		else if (which == REG_EBP && data->accv[REG_EBP] == 
			(Iaddr)-1)
		{
			data->ebp = dest.iaddr;
			return 1;
		}
		return lwp->write(data->accv[which], what, dest);
	}
}

Iaddr
Frame::getreg( RegRef which )
{
	Itype itype;
	if ( !readreg( which, Saddr, itype ) ) 
	{
		if (lwp->get_state() != es_dead)
			printe(ERR_read_reg, E_ERROR, 
				lwp->lwp_name());
		return 0;
	}
	return itype.iaddr;
}

// return nth word of arguments
Iint4
Frame::argword(int n)
{
	Itype itype;
	Iaddr ebp;
	Iaddr pc;

	pc = getreg(REG_EIP);

	if (data->noprolog == -1)
	{
		Symbol	entry;
		Iaddr	fn;
		entry = lwp->find_entry( pc );
		if ( !entry.isnull() ) 
			// get address of function start
			fn = entry.pc(an_lopc); 
		if ( fn )
		{
			Iaddr	skipaddr;
			int	prosize;
			if ((skipaddr = lwp->instruct()->fcn_prolog(fn, 
				1, prosize, data->saved_regs)) != fn)
			{
				Iaddr	tmp = 0;
				// prolog might be target of a jump;
				// skipaddr is next instr beyond prolog

				tmp = lwp->instruct()->jmp_target(fn);

				if (pc >= skipaddr && (!tmp || (pc < tmp)))
				{
					data->noprolog = 0;
					data->prolog_size = prosize;
				}
				else
					// haven't set ebp yet
					data->noprolog = 1;
			}
			else
				data->noprolog = 1;
		}
		else
			data->noprolog = 1;
	}
	// if prolog, 
	// arguments start at 8(%ebp)
	if (data->noprolog == 0)
	{
		ebp = getreg(REG_EBP);
		lwp->read(ebp + 8 + (sizeof(int)*n), Sint4, itype );
		return itype.iint4;
	}
	// otherwise, find place arguments start on stack;
	// first argument is just above return address on stack,
	// so we walk back looking for retaddr.

	Iaddr	esp, tmp;

	if (data->prevpc == (Iaddr)-1)
	{
		if (!retaddr(data->prevpc, tmp))
		{
			data->prevpc = (Iaddr)-1;
			return 0;
		}
	}
	esp = getreg(REG_ESP);
	while (lwp->in_stack(esp))
	{
		lwp->read(esp, Sint4, itype);
		if (itype.iaddr == data->prevpc)
			break;
		esp += sizeof(int);
	}
	if (itype.iaddr != data->prevpc)
		return 0;
	lwp->read( esp + 4 + (sizeof(int)*n), Sint4, itype );
	return itype.iint4;
}

int
Frame::nargwds(int &assumed)
{
	Iaddr	tmp;
	// get return address and use instruction there
	// to calculate the number of argument words
	if ( data->nargwds < 0 ) 
	{
		if (data->prevpc == (Iaddr)-1)
		{
			if (!retaddr(data->prevpc, tmp))
			{
				data->prevpc = (Iaddr)-1;
				return 0;
			}
		}

		data->nargwds = 
			lwp->instruct()->nargbytes(data->prevpc) / 
				sizeof(int);
	}
	if (data->nargwds == 0)
	{
		// guessing - we can't tell the difference
		// between really having no arguments and not
		// being able to tell how many we have.
		assumed = 1;
		return 3;
	}
	assumed = 0;
	return data->nargwds;
}

Iaddr
Frame::pc_value()
{
	Iaddr	pc;

	// Return pc value for this frame. pc saved by
	// caller() is the return address of the
	// call to the next function in the stack sequence.
	// We adjust here to return the address of the call itself.
	//
	pc = getreg( REG_EIP );
	if ( level > 0 )
	// we have saved pc on stack
	{
		return(pc - lwp->instruct()->call_size(pc));
			// sizeof call instruction
	}
	else
	// top frame
		return(pc);
}

// get return address for current frame; if frame has a prolog,
// save register vars if necessary
// If frame has a prolog, six possible cases:
// 1) right after call; esp points to retaddr
// 2) in functions returning structs, after pop of return
//      address but before xchg - %eax holds retaddr
// 3) prev ebp pushed on stack; esp + 4 points to retaddr
// 4) current ebp set to point to prev ebp; ebp + 4 is retaddr
// 5) in middle of function; current ebp + 4 is retaddr
// 6) after leave instruction restores old frame pointer
// If no prolog, just look for what looks like a return addr.
// Returns both return addr and stack address at which
// it was found
 
int
Frame::retaddr(Iaddr &addr, Iaddr &stack)
{
	Iaddr	ebp, esp, pc;
	Iaddr	orig_esp, orig_ebp;
	Iaddr	fn = 0;
	Itype	itype;
	Symbol	entry;
	int	prosize = 0;

	data->prevebp = (Iaddr)-1;
	pc = getreg(REG_EIP);
	orig_ebp = ebp = getreg(REG_EBP);
	orig_esp = esp = getreg(REG_ESP);
	entry = lwp->find_entry( pc );
	// get address of function start
	if ( !entry.isnull() ) 
		fn = entry.pc(an_lopc); 
	if ( fn != 0 ) 
	{
		// look for function prolog and save registers
		if (data->noprolog == -1)
		{
			if ((int)lwp->instruct()->fcn_prolog(fn,
				0, prosize, data->saved_regs) == 0)
				data->noprolog = 1;
			else
			{
				data->noprolog = 0;
				data->prolog_size = prosize;
			}
		}
	}
	if (data->noprolog < 0)
		data->noprolog = 1;
	if (data->noprolog == 0)
	{
		int	diff = 0;
		if (pc != fn)
		{
			// might be a jump to prolog
			Iaddr	tmp = lwp->instruct()->jmp_target(fn);
			if (tmp)
			{
				fn = tmp;
				if (pc < tmp)
					// beyond prolog already
					diff = 100; // arbitrary - big
						// enough
				else 
					diff = (int)(pc - fn);
			}
			else
				diff = (int)(pc - fn);
		}
		if (data->prolog_size == 8)
		{
			// function returning struct
			if (diff == 1)
			{
				// after pop of return addr
				stack = esp;
				addr = getreg(REG_EAX);
				data->noprolog = 1;
				return 1;
			}
			else if (diff > 5)
				diff -= 5;
			else
				diff = 0;
		}
		switch (diff)
		{
		case 0:
			// at beginning of prolog
			lwp->read(esp, Saddr, itype);
			stack = esp;
			data->noprolog = 1;
			break;
		case 1:
			// after ebp has been pushed, but not updated
			lwp->read(esp+4, Saddr, itype);
			stack = esp+4;
			data->noprolog = 1;
			break;
		default:
			// after ebp has been updated
			if (lwp->instruct()->isreturn(pc))
			{
				// leave instruction has reset esp and
				// popped ebp
				stack = esp;
				lwp->read(esp, Saddr, itype);
			}
			else
			{
				stack = ebp+4;
				lwp->read(ebp+4, Saddr, itype);
			}
			break;
		}
		addr = itype.iaddr;
		return 1;
	}
	if (!entry.isnull())
	{
		if ((pc - fn) <= data->prolog_size)
		{
			// haven't saved registers yet
			for(int i = 0; i < 5; i++)
				data->saved_regs[i] = 0;
		}
		if (strcmp("_start", lwp->symbol_name(entry)) == 0)
		{
			// handle first frame - doesn't have a prolog
			// if at _start and previous ebp is 0,
			// assume we are at first frame
			if ((lwp->read(ebp, Saddr, itype) <= 0) ||
				(itype.iaddr == 0))
				return 0;
		}
		// no prolog or after old frame pointer reset
		// look for a pc value on the stack
		Iaddr stackword;

		esp = orig_esp;

		DPRINT(DBG_EXECON,("caller() no prolog\n"));
		while ( lwp->in_stack(esp) )
		{
	
			int	i = 0;
			lwp->read(esp, Saddr, itype);
			stackword = itype.iaddr;
			if ( lwp->in_text(stackword) &&
				((i = lwp->instruct()->iscall(stackword, fn))
					== 1))
			{
				addr = stackword;
				stack = esp;
				return 1;
			}
			else if (i == -1)
			// stop looking - we have hit a call
			// but we can't figure out the callee address
				break;
			esp += sizeof(int);
		} 
	}

	// at this point, we probably have a function called
	// via an indirect call through a register; there
	// is nothing we can do but disassemble forward
	// until we find a return

	esp = orig_esp;
	if (!lwp->instruct()->find_return(pc, esp, orig_ebp))
		return 0;
	if (lwp->read(esp, Saddr, itype) != sizeof(Saddr))
		return 0;
	if (!lwp->in_text(itype.iaddr))
		return 0;
	addr = itype.iaddr;
	data->prevebp = orig_ebp;
	stack = esp;
	return 1;
}

int
Frame::incomplete()
{
	return 0;
}
