#ident	"@(#)debugger:libmachine/i386/Instr.C	1.11.1.2"

#include <stdio.h>
#include <string.h>
#include "LWP.h"
#include "Itype.h"
#include "Instr.h"
#include "dis.h"
#include "Reg1.h"
#include "Interface.h"
#include "Symbol.h"
#include "Source.h"
#include "Tag.h"


// static data - could have been defined in the class,
// but it's not preserved across calls to any of the public
// functions

static unsigned char	byte[NLINE];	// store instruction
static int		byte_cnt;	// bytes of instr processed
static char		operand[3][OPLEN];	
static char		symarr[3][OPLEN];
static char    		*overreg;	// save the segment override 
					// register if any 
static int 		data16;		// 16- or 32-bit data 
static int		addr16;		// 16- or 32-bit addressing 

// format strings used to print operands
static	char	*fmt1[] =
{
	"%s%s,%s",
	"%s%s,%s,%s",
};

static	char	*fmt2[] =
{
	"%s [ %s, %s ]",
	"%s [ %s, %s, %s ]",
};

static inline unsigned char
get_byte()
{
	return byte[byte_cnt++];
}

// Get the byte following the op code and separate it into the
// mode, register, and r/m fields.
// Scale-Index-Bytes have a similar format.

void
Instr::get_modrm_byte(unsigned *mode, unsigned *reg, unsigned *r_m)
{
	unsigned char curbyte = get_byte();

	*r_m = curbyte & 0x7; 
	*reg = curbyte >> 3 & 0x7;
	*mode = curbyte >> 6 & 0x3;
}

//  Check to see if there is a segment override prefix pending.
// If so, print it in the current 'operand' location and set
// the override flag back to false.

void
Instr::check_override(int opindex)
{
	if (overreg)
	{
		(void)strcpy(operand[opindex], overreg);
		overreg = (char *) 0;
	}
}

// Get and print in the 'operand' array a one, two or four
// byte displacement from a register.

void
Instr::displacement(int no_bytes, int opindex, long  *value)
{
	char	temp[sizeof("0x12345678")];

	getbytes(no_bytes, temp, value);
	check_override(opindex);
	(void)strcat(operand[opindex], temp);
}

void
Instr::get_operand(unsigned mode, unsigned r_m, int wbit, int opindex,
	int symbolic)
{
	int		dispsize;  // size of displacement in bytes
	long		dispvalue; // value of the displacement
	char		*format;   // output format of result
	const char	*resultreg;// representation of index(es)
	int		s_i_b;     // flag presence of scale-index-byte 
	char		indexbuffer[16]; // char representation of index(es)
	Symbol		sym;
	char		*symstring;
	unsigned	ss;    // scale-factor from opcode
	unsigned 	index; // index register number
	unsigned 	base;  // base register number

	// if symbolic representation, skip override prefix, if any
	check_override(opindex);
	if (symbolic)
	{
		symarr[opindex][0] = 0;
		symstring = symarr[opindex];
	}

	// check for the presence of the s-i-b byte 
	if (r_m==REG_ESP && mode!=REG_ONLY && !addr16) 
	{
		s_i_b = TRUE;
		get_modrm_byte(&ss, &index, &base);
	}
	else
		s_i_b = FALSE;

	if (addr16)
		dispsize = dispsize16[r_m][mode];
	else
		dispsize = dispsize32[r_m][mode];

	if (s_i_b && mode==0 && base==REG_EBP) dispsize = 4;

	if (dispsize != 0)
		displacement(dispsize, opindex, &dispvalue);

	if (s_i_b) 
	{
		register const char *basereg = regname32[mode][base];
		if (*indexname[index])
			(void) sprintf(indexbuffer, fmt1[0], basereg,
				indexname[index], scale_factor[ss]);
		else
			strcpy(indexbuffer, basereg);
		resultreg = indexbuffer;
		format = "%s(%s)";
		if (symbolic)
		{
			if ((base == REG_EBP && mode == 0) ||
				(base != REG_EBP && dispsize))
						
			{
				sym = lwp->find_symbol(dispvalue);
				if (!sym.isnull() && sym.name())
				{
					char	*n = lwp->symbol_name(sym);
					symstring += sprintf(symstring,
						"%s", n);
					if (base != REG_EBP)
					{
						symstring += sprintf(symstring,
						"+%s", basereg);
					}
					if (index != REG_ESP)//no index
						symstring += sprintf(symstring,
						"+%s", indexname[index]);
					if (ss)
						sprintf(symstring,
						"*%s", scale_factor[ss]);

				}
			}
		}
	}
	else 
	{ 
		// no s-i-b 
		if (mode == REG_ONLY) 
		{
			format = "%s%s";
			if (data16)
				resultreg = REG16[r_m][wbit] ;
			else
				resultreg = REG32[r_m][wbit] ;
		}
		else 
		{ 
			// Modes 00, 01, or 10
			if (addr16)
				resultreg = regname16[mode][r_m];
			else
				resultreg = regname32[mode][r_m];
			if (r_m ==REG_EBP && mode == 0) 
			{ 
				// displacement only
				format = "%s";
				if (symbolic)
				{
					sym = lwp->find_symbol(dispvalue);
					if (!sym.isnull() && sym.name())
					{
						char	*n = lwp->symbol_name(sym);
						sprintf(symstring,
							"%s", n);
					}
				}
			}
			else 
			{ 
				// Modes 00, 01, or 10, not displacement
				// only, and no s-i-b 
				format = "%s(%s)";
				if (symbolic && (dispsize != 0) &&
						(r_m != REG_EBP))
				{
					sym = lwp->find_symbol(dispvalue);
					if (!sym.isnull() && sym.name())
					{
						char	*n = lwp->symbol_name(sym);
						symstring += sprintf(symstring,
							"%s", n);
						sprintf(symstring, "+%s",
							resultreg);
					}
				}
			}
		}
	}
	(void) sprintf(operand[opindex],format,operand[opindex], resultreg);
}

// getbytes() reads no_bytes from a file and converts them into destbuf.
// A sign-extended value is placed into destvalue if it is non-null.

void
Instr::getbytes(int no_bytes, char *destbuf,long *destvalue)
{
	unsigned char	curbyte;
	int 		j;
	char		*format;
	unsigned long	shiftbuf = 0;
	long		value;

	for (j=0; j < no_bytes; j++) 
	{
		curbyte = get_byte();
		shiftbuf |= (unsigned long) curbyte << (8*j);
	}

	switch(no_bytes) 
	{
	case 1:
		if (shiftbuf & 0x80)
		{
			format = "0x%.4lx";
			value = shiftbuf | ~0xffL;
		}
		else
		{
			format = "0x%.2lx";
			value = shiftbuf & 0xffL;
		}
		break;
	case 2:
		format = "0x%.4lx";
		value = (short) shiftbuf;
		break;
	case 4:
	default:
		format = "0x%.8lx";
		value = shiftbuf;
		break;
	}
	if (destvalue)
		*destvalue = value;
	sprintf(destbuf, format, value);
}

// Determine if 1, 2 or 4 bytes of immediate data are needed, then
// get and print them.

void
Instr::imm_data(int no_bytes, int opindex)
{
	int	len = strlen(operand[opindex]);

	operand[opindex][len] = '$';
	getbytes(no_bytes, &operand[opindex][len+1], 0);
}


// get_opcode (high, low)
// Get the next byte and separate the op code into the high and
//  low nibbles.

unsigned char
Instr::get_opcode(unsigned * high, unsigned  * low)
{
	unsigned char	curbyte = get_byte();;

	*low = curbyte & 0xf; 	 	// ----xxxx low 4 bits 
	*high = curbyte >> 4 & 0xf;  	// xxxx---- bits 7 to 4
	return curbyte;
}

// get text, replace breakpints with original text.

int
Instr::get_text_nobkpt( Iaddr start)
{
	
	char	*oldtext;
	int	cnt, idx; 

	if ( lwp == 0 || start == 0 )
		return 0;

	if ((cnt = lwp->read(start, NLINE, (char *) byte )) <= 0 )
		return 0;
	byte_cnt = 0;
	// if there are any breakpoints in byte[] replace them
	// with the original text
	for (idx = 0; idx < cnt; idx++)
	{
		oldtext = lwp->text_nobkpt(start + idx);
		if ( oldtext  != 0 )  
			byte[idx] = *oldtext;
	}
	return 1;
}
// get_text reads the NLINE no of bytes from the address addr
// NLINE is the max. no of characters in an assembly instruction
int
Instr::get_text(Iaddr start)
{
	if ( lwp == 0 || start == 0 )
		return 0;

	byte_cnt = 0;
	return ( lwp->read(start, NLINE, (char *) byte ) > 0 );
}

// if the current instr is CALL, return the address of
// the following instr

Iaddr
Instr::retaddr(Iaddr addr)
{
	unsigned char	op;
	unsigned	mode, r_m, reg;

	if (get_text_nobkpt(addr) == 0) 
	{
		printe(ERR_get_text, E_ERROR, addr);
		return 0;
	}
		
	op = get_byte();
	switch(op)
	{
	default:
		return 0;
	case CALL:
		return addr + 5;
	case LCALL:
		return addr + 7;
	case ICALL:
		// the ICALL opcode is shared by indirect jumps
		// and indirect calls - must distinguish
		get_modrm_byte(&mode, &reg, &r_m);
		if (reg != 2 && reg != 3)
			return 0;
		operand[0][0] = '\0';
		overreg = (char *) 0;
		data16 = addr16 = 0;
		get_operand(mode, r_m, LONGOPERAND, 0, 0);
		return addr + byte_cnt;
	}
	/*NOTREACHED*/
}

// Is last instruction "CALL" to current function?
// We can only handle direct calls and indirect calls
// that go through memory.
int
Instr::iscall(Iaddr caller, Iaddr callee)
{	
	Iaddr		*addrptr;
	int		i = 0;

	// go back far enough to handle CALL and ICALL
	if (get_text_nobkpt(caller - 7) == 0) 
		return 0;
	if (byte[2] == CALL)
	{
		// direct call
		Iaddr	dest;

		addrptr = (Iaddr*) &byte[3];
		dest = caller + (*addrptr);
		if (dest == callee)
			return 1;
		// not call direct to callee;
		// might be a call through a procedure
		// linkage table entry.  In that case,
		// we have:
		// call callee@PLT
		// PLT:
		//	jmp	*callee@GOT
		// GOT:	
		//	&callee
		// If instruction at destination is a jump, we check
		// whether jump target is our callee
		if (jmp_target(dest) == callee)
			return 1;
	}
	if (byte[0] == ICALL)
		i = 1;
	else if (byte[1] == ICALL)
		i = 2;
	if (i > 0)
	{
		// indirect call or jump
		Itype	itype;

		unsigned char	mod = (byte[i] >> 3) & 0x3;
		if (mod == 0x3)
		{
			// indirect call through memory
			addrptr = (Iaddr*)&byte[i+1];
			if (lwp->read(*addrptr, Saddr, itype) != sizeof(Iaddr))
			{
				return 0;
			}
			return (itype.iaddr == callee);
		}
		else if (mod == 0x2)
			// indirect through register - can't handle
			// but we don't want to continue looking
			return -1;
	}
	else
		return 0;
}
	

// given a frame return address calculate size of previous
// call instruction
int
Instr::call_size(Iaddr pc)
{
	int		i = 0;

	// go back far enough to handle CALL and ICALL
	if (get_text_nobkpt(pc - 7) == 0) 
		return 0;
	if (byte[2] == CALL)
	{
		Iaddr	*addrptr;
		Iaddr	dest;

		addrptr = (Iaddr*) &byte[3];
		dest = pc + (*addrptr);
		if (lwp->in_text(dest))
			return 5;
	}
	if (byte[0] == ICALL)
		i = 1;
	else if (byte[1] == ICALL)
		i = 2;
	if (i > 0)
	{
		// indirect call or jump
		unsigned char	mod = (byte[i] >> 3) & 0x3;
		if (mod == 0x3 || mod == 3)
		{
			// indirect call 
			return(i == 1 ? 7 : 6);
		}
	}
	return 0;
}

//
// is next instruction "return" ?
//
int
Instr::isreturn(Iaddr addr)
{	
	if (get_text_nobkpt(addr) == 0)
		return 0;
	unsigned char op = byte[0];
	return (op == RETURNNEAR
		|| op == RETURNFAR
		|| op == RETURNNEARANDPOP
		|| op == RETURNFARANDPOP);
}

int 
Instr::is_bkpt( Iaddr addr )
{
	unsigned low, high;

	if (get_text(addr) == 0) 
	{
		printe(ERR_get_text, E_ERROR, addr);
		return 0;
	}
	if ( get_opcode(&high, &low) == 0xCC )
		return 1;
	else
		return 0;
}


// compoff() will compute the location to which control is to be 
// transferred. 'lng' is the number indicating the jump amount
// (already in proper form, meaning masked and negated if necessary)
// and temp is a character array which already has the actual
// jump amount. The result computed here will go at the end of 'tmp'

char *
Instr::compoff(Iaddr lng, char *temp, Iaddr addr)
{

	Symbol	sym;
	char	*name;

	lng += addr + byte_cnt;
	sprintf(temp,"%s <%lx>",temp, lng);
	sym = lwp->find_entry(lng) ;
	name = lwp->symbol_name( sym );
	return name;
}

// if at a breakpoint, pc points to the next instruction.
// adjust to point to the breakpoint instr itself

Iaddr
Instr::adjust_pc()
{
	Iaddr	pc, newpc;
	Itype	data;

	pc = lwp->getreg(REG_PC);
	newpc = pc - 1;
	if ( get_text(newpc) && (byte[0] == 0xCC) ) 
	{
		data.iaddr = newpc;
	        lwp->writereg(REG_PC, Saddr, data ); 
		return newpc;
	}
	return pc;
}

// get number of arguments
// addr is return address; number of argument bytes is amount
// added to the stack frame when we return from the current function;
// for example:
// 	pushl	%ebx
//	pushl	%eax
// 	call	f
// addr:addl	$8,%esp  / 8 bytes of arguments

int 
Instr::nargbytes(Iaddr addr)
{

	if ( ! get_text_nobkpt(addr) ) 
	{		
		// read instr into bye[]
		printe(ERR_get_text, E_ERROR, addr);
        	return 0;
	}

	if ( (byte[0] == ADDLimm8) && ( byte[1] == toESP) ) 
	{
		return 	( byte[2] );
	}
	// popl %ecx might be used to adjust the stack after the call
	else if ( byte[0] == POPLecx )
		return 4;
	else
		return 0;
	
}

// look for function prolog.
// If skipflag is set, return the address of the first instruction past the
// prolog  if there is one , or pc unchanged if there isn't.
// If skipflag is not set, returns 1 if there is a prolog, else 0
// and sets prosize to the size of the prolog in bytes (size does not include
// a jump to/from prolog).
//
// For register variables, if there are more than 3 register variables
// then compiler puts the first three variables in the registers
// and rest of the variables on the stack. save_reg here is used for
// storing the information about register variables. save_reg[1-3]
// is used for storing the registers(edi, ebx, esi) saved on stack.
// save_reg[0] contains the offset from esp corresponding to the
// remaining no. of register variables. 
//
Iaddr
Instr::fcn_prolog(Iaddr pc, int skipflag, int &prosize, RegRef *save_reg)
{
	Iaddr	*addrptr;
	Iaddr	initpc = pc;
	Iaddr	retval = pc;
	int	offset = 0;
	int	idx = 0;
	int	save_idx = 0;
	int	i = 0;
	int	jmp_to_prolog = 0;
	int	b = 0;

	// Note: must be careful here not to run over end of
	// byte array

	if ( ! get_text_nobkpt(pc) ) 
	{ 		
		// read instr into byte[]
		printe(ERR_get_text, E_ERROR, pc);
		return 0;
	}
	if (byte[0] == JMPrel8) 
	{
		jmp_to_prolog = 1;
		retval += 2;
		pc = retval + byte[1];
		get_text_nobkpt(pc);
	}
	else if (byte[0] == JMPrel32) 
	{
		jmp_to_prolog = 1;
		addrptr = (Iaddr*) &byte[1];
		retval += 5;
		pc = retval + (*addrptr);
		get_text_nobkpt(pc);
	}
	// There are 2 forms of function prologs, one
	// used for functions returning structs, the
	// other for all other functions.
	// Must also make sure optimizer is not aligning frame pointer
	if ( (byte[0] == POPLeax) &&
	     (byte[1] == XCHG) )
	{
		int	pos = 0;
		if ((byte[2] == 0x44) &&
			(byte[3] == 0x24) &&
			(byte[4] == 0)) // xchgl 0(%esp), %eax
			pos = 5;
		else if ((byte[2] == 0x04) && (byte[3] == 0x24))
			pos = 4; 	// xchgl (%esp), %eax

		if (pos && (byte[pos] == PUSHLebp) && 
			(byte[pos+1] == MOVLrr)   &&
			(byte[pos+2] == ESPEBP) &&
			((byte[pos+3] != ANDLimm8) || 
			(byte[pos+4] != ANDLebp)) )
		{
			// prolog used for structs returns
			offset = 4;
		        b = pos+3;
		}
	}
	if ( !b && (byte[0] == PUSHLebp) &&
	     (byte[1] == MOVLrr)   &&
	     (byte[2] == ESPEBP) &&
	     ((byte[3] != ANDLimm8) || (byte[4] != ANDLebp)) )
		b = 3;
	if (!b)
	{
		if (skipflag)
			return initpc;
		else if (!save_reg)
			return 0;
	}

	idx = b;
	if (  byte[b]  == PUSHLeax ) 
	{
		 offset += 4;
		 idx++; 
		 save_idx = idx;
	}
	else if ( byte[b] == SUBLimm8 ) 
	{
		 offset += byte[b+2];
		 idx += 3;
		 save_idx = idx;
	}
	else if ( byte[b] == SUBLimm32 ) 
	{
		 addrptr = (Iaddr *) &byte[b+2];
		 offset  += (int)*addrptr;
		 // refill byte array
		 get_text_nobkpt(pc + b + 6);
		 save_idx = idx + 6;
		 idx = 0;
	}

	if (save_reg)
	{
		save_reg[0] = offset;
		// get saved registers. 
		// can be edi, esi and ebx
		i = 0;
		while(i < 4)
		{
			if ( byte[ idx+i ] == PUSHLedi ) 
				save_reg[++i] = REG_EDI;
			else if ( byte[ idx+i ] == PUSHLesi ) 
				save_reg[++i] = REG_ESI;
			else if ( byte[ idx+i ] == PUSHLebx ) 
				save_reg[++i] = REG_EBX;
			else if ( byte[ idx+i ] == PUSHLebp ) 
				save_reg[++i] = REG_EBP;
			else 
				break;
		}
	}
	prosize = b ? b : save_idx+i;
	if (!skipflag)
	{
		return(b != 0);
	}
	else if (jmp_to_prolog == 0)
		return (retval + b);
	//
	// skip NOPs
	//
	int nopcnt = 0;

	if (!get_text_nobkpt(retval)) 
	{
		printe(ERR_get_text, E_ERROR, retval);
		return 0;
	}
	while ( byte[nopcnt] == NOP ) 
		nopcnt++;
	return (retval + nopcnt);
}


static	char  	mneu[1028];	
// array to store mnemonic code for output

char *
Instr::deasm(Iaddr pcaddr, int &inst_size, int symbolic, char *name,
	Iaddr offset)
{
	const instable	*dp;
	unsigned	mode, reg, r_m;
	int		wbit, vbit;
	long		lngval;
	unsigned char	curbyte;
	Iaddr		addr;

	/* nibbles of the opcode */
	unsigned	opcode1, opcode2, opcode3, opcode4, opcode5;

	const char 	*reg_name;
	char		*sym_name;
	char 		mnemonic[OPLEN];
	int 		got_modrm_byte = 0;
	unsigned short	tmpshort;

	mnemonic[0] = '\0';
	mneu[0] = '\0';
	operand[0][0] = '\0';
	operand[1][0] = '\0';
	operand[2][0] = '\0';
	overreg = (char *) 0;
	data16 = addr16 = 0;
	
	if ( get_text_nobkpt(pcaddr) == 0 ) 
	{
		printe(ERR_get_text, E_ERROR, pcaddr);
		inst_size = 0;
		return 0;
	}

	if ( name != 0 )
	{
		int i = strlen(name);
		// name is max 12 chars; if less than 9, 
		// padd out with spaces
		if (i < 9)
			sprintf(mneu, "(%.12s+%d:)%*s\t", name, offset, 9-i, " ");
		else
			sprintf(mneu, "(%.12s+%d:)\t", name, offset);
	}
	else
		sprintf(mneu, "(..............)\t");

	// As long as there is a prefix, the default segment register,
	// addressing-mode, or data-mode in the instruction will be overridden.
	// This may be more general than the chip actually is.
	for(;;) 
	{
		curbyte = get_opcode(&opcode1, &opcode2);
		dp = &distable[opcode1][opcode2];

		if ( dp->adr_mode == PREFIX ) 
			strcat(mnemonic,dp->name);
		else if ( dp->adr_mode == AM ) 
			addr16 = !addr16;
		else if ( dp->adr_mode == DM ) 
			data16 = !data16;
		else if ( dp->adr_mode == OVERRIDE ) 
			overreg = dp->name;
		else break;
	}

	// Some 386 instructions have 2 bytes of opcode 
	// before the mod_r/m 

	unsigned char	op;
	if (curbyte == 0x0F)
	{
		op = get_opcode(&opcode4, &opcode5);
		if (opcode4 > 12)  
		{
			strcpy(mneu,"***** Error - bad opcode\n");
			inst_size = 0;
			return mneu;
		}
	}
	// Some instructions have opcodes for which several
	// instructions exist.  Part of the mod/rm byte is
	// used to distinguish among them.

	got_modrm_byte = 1;
	get_modrm_byte(&mode, &opcode3, &r_m);

	if (opcode1 == 0xD && opcode2 >= 0x8) 
	{
		// instruction form 5
		if (opcode2 == 0xB && mode == 0x3)
		{
			if (opcode3 == 4)
				dp = &opFP5[r_m];
			else if (opcode3 > 4) 
			{
				strcpy(mneu,"***** Error - bad opcode\n");
				inst_size = 0;
				return mneu;
			}
		}
		// instruction form 4
		else if (opcode2 == 0x9 && mode==0x3 && opcode3 >= 4)
			dp = &opFP4[opcode3-4][r_m];
		// instruction form 3
		else if (mode == 0x3)
			dp = &opFP3[opcode2-8][opcode3];
		// instruction form 1 and 2
		else dp = &opFP1n2[opcode2-8][opcode3];
	}
	else
	{
		switch(curbyte)
		{
		case 0x0F:
			switch(op)
			{
			case 0x0:	
				dp = &op0F00[opcode3];
				break;
			case 0x1:
				dp = &op0F01[opcode3];
				break;
			case 0xBA:
				dp = &op0FBA[opcode3];
				break;
			case 0xC8:
				dp = &op0FC8[opcode3];
				break;
			default:
				if (opcode4 >= 0x8)
					// table is compressed
					// invalid entries 0x40-0x7F
					// have been deleted
					dp = &op0F[opcode4-4][opcode5];
				else if (opcode4 >= 0x4)
					// invalid range
					dp = &op0F[0x2][0xF];
				else
					dp = &op0F[opcode4][opcode5];
				got_modrm_byte = 0;
				byte_cnt--;  // reset 
				break;
			}
			break;
		case 0x80:
			dp = &op80[opcode3];
			break;
		case 0x81:
			dp = &op81[opcode3];
			break;
		case 0x82:
			dp = &op82[opcode3];
			break;
		case 0x83:
			dp = &op83[opcode3];
			break;
		case 0xC0:
			dp = &opC0[opcode3];
			break;
		case 0xC1:
			dp = &opC1[opcode3];
			break;
		case 0xD0:
			dp = &opD0[opcode3];
			break;
		case 0xD1:
			dp = &opD1[opcode3];
			break;
		case 0xD2:
			dp = &opD2[opcode3];
			break;
		case 0xD3:
			dp = &opD3[opcode3];
			break;
		case 0xF6:
			dp = &opF6[opcode3];
			break;
		case 0xF7:
			dp = &opF7[opcode3];
			break;
		case 0xFE:
			dp = &opFE[opcode3];
			break;
		case 0xFF:
			dp = &opFF[opcode3];
			break;
		default:
			// reset to get modrm only if needed
			byte_cnt--;
			got_modrm_byte = 0;
			break;
		}
	}
	// print the mnemonic
	if ( dp->adr_mode != CBW  && dp->adr_mode != CWD ) 
	{
		(void) strcat(mnemonic,dp->name);  
		if (dp->suffix)
			(void) strcat(mnemonic, (data16? "w" : "l") );
		(void) sprintf(mneu, "%s %-7s ", mneu, mnemonic);
	}

	// Each instruction has a particular instruction syntax format
	// stored in the disassembly tables.  The assignment of formats
	// to instructions was made by the author.  Individual formats
	// are explained as they are encountered in the following
	// switch construct.

	switch (dp->adr_mode) 
	{
	case MOVZ:
		// movsbl movsbw (0x0FBE) or movswl (0x0FBF)
		// movzbl movzbw (0x0FB6) or mobzwl (0x0FB7) 
		// wbit lives in 2nd byte, 
		// note that operands are different sized 
		if ( ! got_modrm_byte )
			get_modrm_byte(&mode, &reg, &r_m);
		if ( data16 )
			reg_name = REG16[reg][LONGOPERAND];
		else
			reg_name = REG32[reg][LONGOPERAND];

		wbit = WBIT(opcode5);
		data16 = 1;
		get_operand(mode, r_m, wbit, 0, symbolic);
		sprintf(mneu,fmt1[0],mneu,operand[0],reg_name);
		if (symbolic && symarr[0][0]) 
			sprintf(mneu,fmt2[0],mneu,symarr[0],reg_name);
		inst_size = byte_cnt;
		return mneu;

	case IMUL:
		// imul instruction, with either 8-bit or longer immediate
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 1, symbolic);
		// opcode 0x6B for byte, sign-extended displacement, 0x69 for word(s)
		imm_data( OPSIZE(data16,opcode2 == 0x9), 0);
		if (data16)
			reg_name = REG16[reg][LONGOPERAND];
		else
			reg_name = REG32[reg][LONGOPERAND];
		(void)sprintf(mneu,fmt1[1],mneu,operand[0],operand[1],reg_name);
		if (symbolic && symarr[1][0])
			sprintf(mneu,fmt2[1],mneu, operand[0],
				symarr[1][0] ? symarr[1] : operand[1],
				reg_name);
		inst_size = byte_cnt;
		return mneu;

	case MRw:
		// memory or register operand to register, with 'w' bit
		wbit = WBIT(opcode2);
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, wbit, 0, symbolic);
		if (data16)
			reg_name = REG16[reg][wbit];
		else
			reg_name = REG32[reg][wbit];
		(void) sprintf(mneu,fmt1[0],mneu,operand[0],reg_name);
		if (symbolic && symarr[0][0]) 
			sprintf(mneu,fmt2[0],mneu,symarr[0],reg_name);
		inst_size = byte_cnt;
		return mneu;

	case RMw:
		// register to memory or register operand, with 'w' bit
		// arpl happens to fit here also because it is odd
		wbit = WBIT(opcode2);
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, wbit, 0, symbolic);
		if (data16)
			reg_name = REG16[reg][wbit];
		else
			reg_name = REG32[reg][wbit];
		(void) sprintf(mneu,fmt1[0],mneu,reg_name,operand[0]);
		if (symbolic && symarr[0][0]) 
			sprintf(mneu,fmt2[0],mneu,reg_name,symarr[0]);
		inst_size = byte_cnt;
		return mneu;

	case DSHIFT:
		// Double shift. Has immediate operand specifying
		// the shift.
		get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 1, symbolic);
		if (data16)
			reg_name = REG16[reg][LONGOPERAND];
		else
			reg_name = REG32[reg][LONGOPERAND];
		imm_data(1, 0);
		sprintf(mneu,fmt1[1],mneu,operand[0],reg_name,operand[1]);
		if (symbolic && symarr[1][0])
			sprintf(mneu,fmt2[1],mneu, operand[0], reg_name,
				symarr[1][0] ? symarr[1] : operand[1]);
		inst_size = byte_cnt;
		return mneu;

	case DSHIFTcl:
		// Double shift. With no immediate operand,
		// specifies using %cl.
		get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 0, symbolic);
		if (data16)
			reg_name = REG16[reg][LONGOPERAND];
		else
			reg_name = REG32[reg][LONGOPERAND];
		sprintf(mneu,fmt1[0],mneu,reg_name,operand[0]);
		if (symbolic && symarr[0][0]) 
			sprintf(mneu,fmt2[0],mneu,reg_name,symarr[0]);
		inst_size = byte_cnt;
		return mneu;

	case IMlw:
		// immediate to memory or register operand
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, 1, symbolic);
		/* A long immediate is expected for opcode 0x81, not 0x80 nor 0x83 */
		imm_data(OPSIZE(data16,opcode2 == 1), 0);
		sprintf(mneu,fmt1[0],mneu,operand[0],operand[1]);
		if (symbolic && symarr[1][0])
			sprintf(mneu,fmt2[0],mneu, operand[0],
				symarr[1][0] ? symarr[1] : operand[1]);
		inst_size = byte_cnt;
		return mneu;

	case IMw:
		// immediate to memory or register operand with the
		// 'w' bit present
		wbit = WBIT(opcode2);
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, wbit, 1, symbolic);
		imm_data(OPSIZE(data16,wbit), 0);
		sprintf(mneu,fmt1[0],mneu,operand[0],operand[1]);
		if (symbolic && symarr[1][0])
			sprintf(mneu,fmt2[0],mneu, operand[0],
				symarr[1][0] ? symarr[1] : operand[1]);
		inst_size = byte_cnt;
		return mneu;

	case IR:
		// immediate to register with register in low 3 bits
		// of op code
		wbit = opcode2 >>3 & 0x1; 
		// w-bit here (with regs) is bit 3
		reg = REGNO(opcode2);
		imm_data( OPSIZE(data16,wbit), 0);
		if (data16)
			reg_name = REG16[reg][wbit];
		else
			reg_name = REG32[reg][wbit];
		(void) sprintf(mneu,fmt1[0],mneu,operand[0],reg_name);
		inst_size = byte_cnt;
		return mneu;

	case OA:
		// memory operand to accumulator
		wbit = WBIT(opcode2);
		displacement(OPSIZE(addr16,LONGOPERAND), 0,&lngval);
		reg_name = ( data16 ? REG16 : REG32 )[0][wbit];
		(void) sprintf(mneu,fmt1[0],mneu,operand[0],reg_name);
		if (symbolic)
		{
			Symbol	sym = lwp->find_symbol(lngval);
			if (!sym.isnull() && sym.name())
			{
				char	*n = lwp->symbol_name(sym);
				(void)sprintf(mneu, fmt2[0],
					mneu, n, reg_name);
			}
		}
		inst_size = byte_cnt;
		return mneu;

	case AO:
		// accumulator to memory operand
		wbit = WBIT(opcode2);
		
		displacement(OPSIZE(addr16,LONGOPERAND), 0,&lngval);
		reg_name = ( addr16 ? REG16 : REG32 )[0][wbit];
		(void) sprintf(mneu,fmt1[0],mneu, reg_name, operand[0]);
		if (symbolic)
		{
			Symbol	sym = lwp->find_symbol(lngval);
			if (!sym.isnull() && sym.name())
			{
				char	*n = lwp->symbol_name(sym);
				(void)sprintf(mneu, fmt2[0],
					mneu,reg_name, n);
			}
		}
		inst_size = byte_cnt;
		return mneu;

	case MS:
		// memory or register operand to segment register
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 0, symbolic);
		(void) sprintf(mneu,fmt1[0],mneu,operand[0],SEGREG[reg]);
		if (symbolic && symarr[0][0]) 
			sprintf(mneu,fmt2[0],mneu,symarr[0],SEGREG[reg]);
		inst_size = byte_cnt;
		return mneu;

	case SM:
		// segment register to memory or register operand
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 0, symbolic);
		(void) sprintf(mneu,fmt1[0],mneu,SEGREG[reg],operand[0]);
		if (symbolic && symarr[0][0]) 
			sprintf(mneu,fmt2[0],mneu,SEGREG[reg],symarr[0]);
		inst_size = byte_cnt;
		return mneu;

	case Mv:
		// register to memory or register operand, with 'w' bit
		// arpl happens to fit here also because it is odd
		vbit = VBIT(opcode2);
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, 0, symbolic);
		/* When vbit is set, register is an operand, otherwise just $0x1 */
		reg_name = vbit ? "%cl," : "" ;
		(void) sprintf(mneu,"%s%s%s",mneu, reg_name, operand[0]);
		if (symbolic && symarr[0][0]) 
			sprintf(mneu,"%s\t[ %s%s ]",mneu, reg_name, symarr[0]);
		inst_size = byte_cnt;
		return mneu;

	case MvI:
		// immediate rotate or shift instrutions, which may or
		// may not consult the cl register, depending on the 'v' bit
		vbit = VBIT(opcode2);
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, 0, symbolic);
		imm_data(1,1);
		/* When vbit is set, register is an operand, otherwise just $0x1 */
		reg_name = vbit ? "%cl," : "" ;
		(void) sprintf(mneu,"%s%s,%s%s",mneu,operand[1], reg_name, operand[0]);
		if (symbolic && symarr[0][0])
			sprintf(mneu,"%s [ %s, %s%s ]", operand[1], reg_name,
				symarr[0][0] ? symarr[0] : operand[0]);
		inst_size = byte_cnt;
		return mneu;

	case MIb:
		get_operand(mode, r_m, LONGOPERAND, 0, symbolic);
		imm_data(1,1);
		(void) sprintf(mneu,fmt1[0],mneu,operand[1], operand[0]);
		if (symbolic && symarr[0][0])
			sprintf(mneu,fmt2[0],mneu, operand[1],
				symarr[0][0] ? symarr[0] : operand[0]);
		inst_size = byte_cnt;
		return mneu;

	case Mw:
		// single memory or register operand with 'w' bit present
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, 0, symbolic);
		(void) sprintf(mneu,"%s%s",mneu,operand[0]);
		if (symbolic && symarr[0][0]) 
			sprintf(mneu,"%s [ %s ]",mneu, symarr[0]);
		inst_size = byte_cnt;
		return mneu;

	case M:
		// single memory or register operand
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 0, symbolic);
		(void) sprintf(mneu,"%s%s",mneu,operand[0]);
		if (symbolic && symarr[0][0]) 
			sprintf(mneu,"%s [ %s ]",mneu, symarr[0]);
		inst_size = byte_cnt;
		return mneu;

	case SREG:
		// special register 
		get_modrm_byte(&mode, &reg, &r_m);
		vbit = 0;
		switch (opcode5) 
		{
		case 2:
			vbit = 1;
			// fall thru 
		case 0: 
			reg_name = CONTROLREG[reg];
			break;
		case 3:
			vbit = 1;
			/* fall thru */
		case 1:
			reg_name = DEBUGREG[reg];
			break;
		case 6:
			vbit = 1;
			/* fall thru */
		case 4:
			reg_name = TESTREG[reg];
			break;
		}
		strcpy(operand[0], REG32[r_m][1]);

		if (vbit)
		{
			strcpy(operand[0], reg_name);
			reg_name = REG32[r_m][1];
		}
		
		(void) sprintf(mneu, fmt1[0],mneu, reg_name, operand[0]);
		inst_size = byte_cnt;
		return mneu;

	case R:
		// single register operand with register in the low 3
		// bits of op code
		reg = REGNO(opcode2);
		if (data16)
			reg_name = REG16[reg][LONGOPERAND];
		else
			reg_name = REG32[reg][LONGOPERAND];
		(void) sprintf(mneu,"%s%s",mneu,reg_name);
		inst_size = byte_cnt;
		return mneu;

	case RA: 
	{
		// register to accumulator with register in the low 3
		// bits of op code, xchg instructions
		char *eprefix;
		reg = REGNO(opcode2);
		if (data16) 
		{
			eprefix = "";
			reg_name = REG16[reg][LONGOPERAND];
		}
		else
		{
			eprefix = "e";
			reg_name = REG32[reg][LONGOPERAND];
		}
		(void) sprintf(mneu,"%s%s,%%%sax", mneu,reg_name,eprefix);
		inst_size = byte_cnt;
		return mneu;
	}

	case SEG:
		// single segment register operand, with register in
		// bits 3-4 of op code
		reg = curbyte >> 3 & 0x3; // segment register 
		(void) sprintf(mneu,"%s%s",mneu,SEGREG[reg]);
		inst_size = byte_cnt;
		return mneu;

	case LSEG:
		// single segment register operand, with register in
		// bits 3-5 of op code
		reg = curbyte >> 3 & 0x7; // long seg reg from opcode
		(void) sprintf(mneu,"%s%s",mneu,SEGREG[reg]);
		inst_size = byte_cnt;
		return mneu;

	case MR:
		// memory or register operand to register
		if (!got_modrm_byte)
			get_modrm_byte(&mode, &reg, &r_m);
		get_operand(mode, r_m, LONGOPERAND, 0, symbolic);
		if (data16)
			reg_name = REG16[reg][LONGOPERAND];
		else
			reg_name = REG32[reg][LONGOPERAND];
		(void) sprintf(mneu,fmt1[0],mneu,operand[0],reg_name);
		if (symbolic && symarr[0][0]) 
			sprintf(mneu,fmt2[0],mneu, symarr[0],reg_name);
		inst_size = byte_cnt;
		return mneu;

	case IA: 
	{
		// immediate operand to accumulator
		int no_bytes = OPSIZE(data16,WBIT(opcode2));
		switch(no_bytes) 
		{
			case 1: reg_name = "%al"; break;
			case 2: reg_name = "%ax"; break;
			case 4: reg_name = "%eax"; break;
		}
		imm_data(no_bytes, 0);
		(void) sprintf(mneu,fmt1[0],mneu,operand[0], reg_name) ;
		inst_size = byte_cnt;
		return mneu;
	}
	case MA:
		// memory or register operand to accumulator
		wbit = WBIT(opcode2);
		get_operand(mode, r_m, wbit, 0, symbolic);
		reg_name = ( data16 ? REG16 : REG32) [0][wbit];
		(void) sprintf(mneu,fmt1[0],mneu, operand[0], reg_name );
		if (symbolic && symarr[0][0]) 
			sprintf(mneu,fmt2[0], mneu, symarr[0], reg_name );
		inst_size = byte_cnt;
		return mneu;

	case SD:
		// si register to di register
		check_override(0);
		(void) sprintf(mneu,"%s%s(%%%ssi),(%%%sdi)",mneu,operand[0],
			addr16? "" : "e" , addr16? "" : "e");
		inst_size = byte_cnt;
		return mneu;

	case AD:
		// accumulator to di register
		wbit = WBIT(opcode2);
		check_override(0);
		reg_name = (data16 ? REG16 : REG32) [0][wbit] ;
		(void) sprintf(mneu,"%s%s,%s(%%%sdi)",mneu, reg_name, operand[0],
			addr16? "" : "e");
		inst_size = byte_cnt;
		return mneu;

	case SA:
		// si register to accumulator
		wbit = WBIT(opcode2);
		check_override(0);
		reg_name = (addr16 ? REG16 : REG32) [0][wbit] ;
		(void) sprintf(mneu,"%s%s(%%%ssi),%s",mneu,operand[0],
			addr16? "" : "e", reg_name);
		inst_size = byte_cnt;
		return mneu;

	case D:
		// single operand, a 16/32 bit displacement
		// added to current offset by 'compoff'
		displacement(OPSIZE(data16,LONGOPERAND), 0, &lngval);
		sym_name = compoff(lngval, operand[1], pcaddr);
		(void) sprintf(mneu,"%s+%s%s",mneu,operand[0],
			(lngval == 0) ? "" : operand[1]);
		if (symbolic && sym_name) sprintf(mneu,"%s [ %s ]",mneu, 
			sym_name );
		inst_size = byte_cnt;
		return mneu;

	case INM:
		// indirect to memory or register operand
		get_operand(mode, r_m, LONGOPERAND, 0, symbolic);
		(void) sprintf(mneu,"%s*%s",mneu,operand[0]);
		if (symbolic && symarr[0][0]) 
			sprintf(mneu,"%s\t[ *%s ]",mneu,symarr[0]);
		inst_size = byte_cnt;
		return mneu;

	case SO:
		// for long jumps and long calls -- a new code segment
		// register and an offset in IP -- stored in object
		// code in reverse order
		displacement(OPSIZE(addr16,LONGOPERAND), 1,&lngval);
		// will now get segment operand
		displacement(2, 0,&lngval);
		(void) sprintf(mneu,fmt1[0],mneu,operand[0],operand[1]);
		inst_size = byte_cnt;
		return mneu;

	case BD:
		// jmp/call. single operand, 8 bit displacement.
		// added to current EIP in 'compoff'
		displacement(1, 0, &lngval);
		sym_name = compoff(lngval, operand[1], pcaddr);
		(void) sprintf(mneu,"%s+%s%s",mneu, operand[0],
			(lngval == 0) ? "" : operand[1]);
		if (symbolic && sym_name) sprintf(mneu,"%s [ %s ]",mneu, 
			sym_name );
		inst_size = byte_cnt;
		return mneu;

	case I:
		// single 32/16 bit immediate operand
		imm_data(OPSIZE(data16,LONGOPERAND), 0);
		(void) sprintf(mneu,"%s%s",mneu,operand[0]);
		inst_size = byte_cnt;
		return mneu;

	case Ib:
		// single 8 bit immediate operand
		imm_data(1, 0);
		(void) sprintf(mneu,"%s%s",mneu,operand[0]);
		inst_size = byte_cnt;
		return mneu;

	case ENTER:
		imm_data(2,0);
		imm_data(1,1);
		(void) sprintf(mneu,fmt1[0],mneu,operand[0],operand[1]);
		inst_size = byte_cnt;
		return mneu;

	case RET:
		// 16-bit immediate operand
		imm_data(2,0);
		(void) sprintf(mneu,"%s%s",mneu,operand[0]);
		inst_size = byte_cnt;
		return mneu;

	case P:
		// single 8 bit port operand
		check_override(0);
		imm_data(1, 0);
		(void) sprintf(mneu,"%s%s",mneu,operand[0]);
		inst_size = byte_cnt;
		return mneu;

	case V:
		// single operand, dx register 
		// (variable port instruction)
		check_override(0);
		(void) sprintf(mneu,"%s%s(%%dx)",mneu,operand[0]);
		inst_size = byte_cnt;
		return mneu;

	case INT3:
		// The int instruction, which has two forms: 
		// int 3 (breakpoint) or int n, where n is 
		// indicated in the subsequent byte (format Ib). 
		// The int 3 instruction (opcode 0xCC), where, 
		// although the 3 looks like an operand, it is 
		// implied by the opcode. It must be converted 
		// to the correct base and output.
		(void) strcat(mneu, "$0x3\t[ 0x3 ]");
		inst_size = byte_cnt;
		return mneu;

	case U:
		// an unused byte must be discarded
		(void)get_byte();
		inst_size = byte_cnt;
		return mneu;

	case CBW:
		if (data16)
			(void) strcat(mneu,"cbtw");
		else
			(void) strcat(mneu,"cwtl");
		inst_size = byte_cnt;
		return mneu;

	case CWD:
		if (data16)
			(void) strcat(mneu,"cwtd");
		else
			(void) strcat(mneu,"cltd");
		inst_size = byte_cnt;
		return mneu;

	case GO_ON:
		// no disassembly, the mnemonic was all there was
		// so go on
		inst_size = byte_cnt;
		return mneu;

	case JTAB:
	{
		// Special byte indicating the beginning of a
		// jump table has been seen. The jump table addresses
		// will be skipped until the address 0xffff which
		// indicates the end of the jump table is read.	
		int	cnt = byte_cnt;
		(void) sprintf(mneu,"%s\n", "***JUMP TABLE BEGINNING***");
		addr = pcaddr + (Iaddr) byte_cnt;
		lwp->read(addr, 1, (char *) byte);
		curbyte = byte[0]; 
		cnt++;
		if (curbyte == FILL) 
		{
			addr = addr + (Iaddr) 1;
			lwp->read(addr, 1, (char *) byte);
			curbyte = byte[0]; 
			cnt++;
		}

		tmpshort = curbyte;
		addr = addr + (Iaddr) 1;
		lwp->read(addr, 1, (char *) byte);
		curbyte = byte[0]; 
		cnt++;
		
		while ((curbyte != 0x00ff) || (tmpshort != 0x00ff)) 
		{
			addr = addr + (Iaddr) 1;
			lwp->read(addr, 1, (char *) byte);
			curbyte = byte[0]; 
			tmpshort = curbyte;
			addr = addr + (Iaddr) 1;
			lwp->read(addr, 1, (char *) byte);
			curbyte = byte[0]; 
			cnt += 2;
		}
		(void) strcat(mneu,"***JUMP TABLE END***");
		inst_size = cnt;
		return mneu;
	}
	// float reg
	case F:
		(void) sprintf(mneu,"%s%%st(%1.1d)",mneu,r_m);
		inst_size = byte_cnt;
		return mneu;

	// float reg to float reg, with ret bit present
	case FF:
		if ( opcode2 >> 2 & 0x1 ) 
		{
			/* return result bit for 287 instructions	*/
			/* st -> st(i) */
			(void) sprintf(mneu,"%s%%st,%%st(%1.1d)",mneu,r_m);
		}
		else 
		{
			/* st(i) -> st */
			(void) sprintf(mneu,"%s%%st(%1.1d),%%st",mneu,r_m);
		}
		inst_size = byte_cnt;
		return mneu;

	// an invalid op code 
	case AM:
	case DM:
	case OVERRIDE:
	case PREFIX:
	case UNKNOWN:
	default:
		strcpy(mneu,"***** Error - bad opcode\n");
		inst_size = 0;
		return mneu;

	} // end switch
}

// translate branch table address to the actual function address
//
Iaddr
Instr::brtbl2fcn( Iaddr addr )
{
	Iaddr *addrptr;

	if ( ! get_text_nobkpt(addr) ) 
	{ 	
		printe(ERR_get_text, E_ERROR, addr);
		return 0;
	}
	if (byte[0] == JMPrel32) 
	{
		addrptr = (Iaddr*) &byte[1];
		return ( addr + (*addrptr) + 5);
	}
	
	return 0;
}

// translate  a function address to the adress of the corresponding
// branch table slot
Iaddr
Instr::fcn2brtbl( Iaddr addr, int offset )
{
	return ((addr  & 0xffff0000 ) + (offset-1) * 5);
}


// If instruction is an unconditional jump, return the
// target address of the jump, else 0.
// This code does not handle all possible unconditional jumps;
// only the ones that the debugger needs to know about (determined by
// experience).

Iaddr
Instr::jmp_target( Iaddr pc)
{
	Iaddr *addrptr;

	if ( ! get_text_nobkpt(pc) ) 	
	{
		printe(ERR_get_text, E_ERROR, pc);
		return 0;
	}
	if (byte[0] == JMPrel8) 
	{
		// 8-bit relative displacement
		// relative to next instruction
		return(pc + 2 + byte[1]);
	}
	else if (byte[0] == JMPrel32) 
	{
		// 32-bit relative displacement
		// relative to next instruction
		addrptr = (Iaddr*) &byte[1];
		return(pc + 5 + (*addrptr));
	}
	else if ((byte[0] == JMPind32) && (byte[1] == 0x25)) 
	{
		// 32-bit indirect - addr in memory
		Itype	itype;
		addrptr = (Iaddr*)&byte[2];
		if (lwp->read(*addrptr, Saddr, itype) != sizeof(Iaddr))
		{
			printe(ERR_proc_read, E_ERROR, lwp->proc_name(),
				*addrptr);
			return 0;
		}
		return(itype.iaddr);
	}
	else
		return 0;
}

#define UNDEF 		(Iaddr)-1
#define MAXINST		3000
#define MAXBRANCH	64

// Disassemble forward until we find a return instruction,
// adjusting the stack pointer as we go.
int
Instr::find_return(Iaddr pc, Iaddr &spp, Iaddr &ebpp)
{
	Iaddr	sp, ebp;
	Iaddr	addr, nextaddr;
	char	*p, *endp;
	Iaddr	unk_addr;
	Iaddr	pushed_ebp, ebp_addr;
	Iaddr	branch_addr[MAXBRANCH];
	int	branch_tried[MAXBRANCH];
	int	ninst, nbranch;
	int	br, last_try, start_time;
	int	sz;
	Itype	itype;
	char	*instr;

	ninst = nbranch = 0;
	unk_addr = 0;
restart:
	sp = spp;
	ebp = ebpp;
	ebp_addr = UNDEF;
	start_time = last_try = ninst;

	// Disassemble instructions until we find a return instr.
	for (addr = pc; ; addr = nextaddr) 
	{
		if (++ninst > MAXINST) 
		{
			return 0;
		}
		instr = deasm(addr, sz, 0, 0, 0);
		if (!instr)
			return 0;
		nextaddr = addr + (sz ? sz : 1);

		if (strncmp(instr, "***** Error", 11) == 0) 
		{
			sp = ebp = UNDEF;
			break;
		} 

		// Set instr to point to the mnemonic
		for(; *instr && *instr != ')'; instr++)
			;
		instr++;
		while (*instr && (*instr == ' ' || *instr == '\t'))
			instr++;
		// Set p to point to the first operand, if any.
		for (p = instr; *p && *p != ' '; p++)
			;
		while (*p && *p == ' ')
			p++;
		// Strip trailing blanks; leave endp pointing to end.
		endp = p + strlen(p);

		while (endp != instr && endp[-1] == ' ')
			*--endp = '\0';
		// Check for instructions which affect %esp or %ebp.
		if (strncmp(instr, "ret", 3) == 0) 
		{
			break;
		}
		else if (strncmp(instr, "leave", 5) == 0) 
		{
			if ((sp = ebp) != UNDEF) 
			{
				if (lwp->read(sp, Saddr, itype) !=
					sizeof(Iaddr))
				{
					return 0;
				}
				ebp = itype.iaddr;
				sp += 4;
			}
		} 
		else if (strncmp(instr, "pushl", 5) == 0) 
		{
			if (sp != UNDEF) 
			{
				sp -= 4;
				if (strcmp(p, "%ebp") == 0) 
				{
					pushed_ebp = ebp;
					ebp_addr = sp;
				}
			}
		} 
		else if (strncmp(instr, "pushw", 5) == 0) 
		{
			if (sp != UNDEF)
				sp -= 2;
		} 
		else if (strncmp(instr, "pushal", 6) == 0) 
		{
			if (sp != UNDEF)
				sp -= 32;
		}
		else if (strncmp(instr, "pushaw", 6) == 0) 
		{
			if (sp != UNDEF)
				sp -= 16;
		}
		else if (strncmp(instr, "pushfl", 6) == 0) 
		{
			if (sp != UNDEF)
				sp -= 4;
		}
		else if (strncmp(instr, "pushfw", 6) == 0) 
		{
			if (sp != UNDEF)
				sp -= 2;
		} 
		else if (strncmp(instr, "popl", 4) == 0) 
		{
			if (strcmp(p, "%ebp") == 0) 
			{
				if (sp != UNDEF) 
				{
					if (ebp_addr == sp) 
					{
						ebp = pushed_ebp;
						ebp_addr = UNDEF;
					} 
					else if (lwp->read(sp, Saddr, 
						itype) != sizeof(Iaddr))
					{
						printe(ERR_proc_read, 
							E_ERROR,
							lwp->lwp_name(),
							sp);
						return 0;
					}
					else
						ebp = itype.iaddr;
				}
				else
					ebp = UNDEF;
			}
			if (sp != UNDEF)
				sp += 4;
		} 
		else if (strncmp(instr, "popw", 4) == 0) 
		{
			if (strcmp(p, "%bp") == 0)
				ebp = UNDEF;
			if (sp != UNDEF)
				sp += 2;
		} 
		else if (strncmp(instr, "popal", 5) == 0) 
		{
			if (sp != UNDEF) 
			{
				if (lwp->read(sp, Saddr, itype) !=
					sizeof(Iaddr))
				{
					printe(ERR_proc_read, E_ERROR,
						lwp->lwp_name(), sp);
					return 0;
				}
				ebp = itype.iaddr;
				sp += 32;
			} else
				ebp = UNDEF;
		} 
		else if (strncmp(instr, "popaw", 5) == 0) 
		{
			ebp = UNDEF;
			if (sp != UNDEF)
				sp += 16;
		} 
		else if (strncmp(instr, "popfl", 5) == 0) 
		{
			if (sp != UNDEF)
				sp += 4;
		} 
		else if (strncmp(instr, "popfw", 5) == 0) 
		{
			if (sp != UNDEF)
				sp += 2;
		} 
		else if (strncmp(instr, "call", 4) == 0)
		{
			// we normally ignore calls, since they leave the
			// stack alone; the exception is "call 0x0",
			// which you get in PIC code - this pushes a return addr
			// on the stack, but doesn't clean up
			if ((endp - p == 11) && 
				strncmp(p, "+0x00000000", 11) == 0)
				if (sp != UNDEF)
					sp -= 4;
		}
		else if (endp > instr + 4 &&
			 (strcmp(endp - 4, "%esp") == 0 ||
			  strcmp(endp - 4, "%ebp") == 0 ||
			  strcmp(endp - 3, "%sp") == 0 ||
			  strcmp(endp - 3, "%bp") == 0)) 
		{
			int		reg16 = (endp[-3] != 'e');
			int		immediate, setting_sp;
			unsigned long	imm_val, new_val;

			if ((setting_sp = (strcmp(endp - 2, "sp") == 0)))
			 	new_val = sp;
			else
			 	new_val = ebp;

			if ((immediate = (*p == '$'))) 
			{
				char	*numend;
				++p;
				imm_val = strtoul(p, &numend, 16);
				if (numend == p)
				{
					--p;
					immediate = 0;
				}
			}

			if (strncmp(instr, "inc", 3) == 0) 
			{
				if (reg16)
					new_val = UNDEF;
				else
					new_val++;
			}
			else if (strncmp(instr, "dec", 3) == 0) 
			{
				if (reg16)
					new_val = UNDEF;
				else
					new_val--;
			} 
			else if (strncmp(instr, "mov", 3) == 0) 
			{
				if (reg16)
					new_val = UNDEF;
				else if (immediate)
					new_val = imm_val;
				else if (strncmp(p, "%esp", 4) == 0)
					new_val = sp;
				else if (strncmp(p, "%ebp", 4) == 0)
					new_val = ebp;
				else
					new_val = UNDEF;
			} 
			else if (strncmp(instr, "add", 3) == 0) 
			{
				if (reg16 || !immediate)
					new_val = UNDEF;
				else
					new_val += imm_val;
			} 
			else if (strncmp(instr, "sub", 3) == 0) 
			{
				if (reg16 || !immediate)
					new_val = UNDEF;
				else
					new_val -= imm_val;
			} 
			else if (strncmp(instr, "lea", 3) == 0) 
			{
				char	*numend;
				if (reg16)
					new_val = UNDEF;
				else
				{
					imm_val = strtoul(p, &numend, 16);
					if (numend == p)
						new_val = UNDEF;
					else 
					{
						new_val += imm_val;
						if (setting_sp) 
						{
						    if (strncmp(p, "(%esp)", 6) != 0 &&
						        strncmp(p, "(%esp,1)", 8) != 0)
							new_val = UNDEF;
						} 
						else 
						{
						    if (strncmp(p, "(%ebp)", 6) != 0 &&
						        strncmp(p, "(%ebp,1)", 8) != 0)
							new_val = UNDEF;
						}
					}
				}
			} 
			else 
			if (strncmp(instr, "enter", 5) == 0) 
			{
				new_val = UNDEF; // XXX - for now
			} 
			else if (strncmp(instr, "xchg", 4) == 0 ||
				   strncmp(instr, "lds", 3) == 0 ||
				   strncmp(instr, "les", 3) == 0 ||
				   strncmp(instr, "lfs", 3) == 0 ||
				   strncmp(instr, "lgs", 3) == 0 ||
				   strncmp(instr, "lss", 3) == 0 ||
				   strncmp(instr, "adc", 3) == 0 ||
				   strncmp(instr, "sbb", 3) == 0 ||
				   strncmp(instr, "neg", 3) == 0 ||
				   strncmp(instr, "not", 3) == 0 ||
				   strncmp(instr, "and", 3) == 0 ||
				   strncmp(instr, "or", 2) == 0 ||
				   strncmp(instr, "xor", 3) == 0 ||
				   strncmp(instr, "rol", 3) == 0 ||
				   strncmp(instr, "ror", 3) == 0 ||
				   strncmp(instr, "sal", 3) == 0 ||
				   strncmp(instr, "sar", 3) == 0 ||
				   strncmp(instr, "sal", 3) == 0 ||
				   strncmp(instr, "shr", 3) == 0 ||
				   strncmp(instr, "shl", 3) == 0 ||
				   strncmp(instr, "rcl", 3) == 0 ||
				   strncmp(instr, "rcr", 3) == 0 ||
				   strncmp(instr, "imul", 4) == 0) 
			{
				new_val = UNDEF;
			}
			if (setting_sp)
				sp = new_val;
			else
				ebp = new_val;
		}
		if (sp == UNDEF) 
		{
			if (unk_addr == 0)
				unk_addr = addr;
		} 
		// Check for conditional or unconditional branches
		if (instr[0] != 'j' && strncmp(instr, "loop", 4) != 0)
			continue;
		// Handle branch instructions
		for (br = 0; br < nbranch; br++) 
		{
			if (branch_addr[br] == addr)
				break;
		}
		if (br < nbranch) 
		{
			if (branch_tried[br]) 
			{
				if (branch_tried[br] >= last_try) 
				{
					/* We looped; give up. */
					if (last_try != start_time) 
					{
						goto restart;
					}
					return 0;
				}
				branch_tried[br] = ninst;
				if (strncmp(instr, "jmp", 3) != 0)
					continue;
			} 
			else
				// 2nd time at conditional br;
				// this time take branch
				last_try = branch_tried[br] = ninst;
		} 
		else if (nbranch < MAXBRANCH) 
		{
			last_try = ninst;
			branch_addr[nbranch++] = addr;
			if (strncmp(instr, "jmp", 3) == 0)
				branch_tried[br] = ninst;
			else 
			{
				// 1st time on conditional branch,
				// don't take branch
				branch_tried[br] = 0;
				continue;
			}
		}
		// Follow jump address 
		// Only direct jumps
		while (*p && *p != '<')
			p++;
		if (*p++ == '<')
		{
			char	*numend;
			nextaddr = strtoul(p, &numend, 16);
			if (numend != p)
				continue;
		}
		if (last_try != start_time)
			goto restart;
		return 0;
	}
	if (sp == UNDEF) 
	{
		if (last_try != start_time)
			goto restart;
		return 0;
	}
	spp = sp;
	ebpp = ebp;
	return 1;
}
