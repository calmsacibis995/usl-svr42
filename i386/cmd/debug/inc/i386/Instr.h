/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	Instr_h
#define	Instr_h
#ident	"@(#)debugger:inc/i386/Instr.h	1.4.1.1"

#include	"Iaddr.h"
#include	"Reg.h"

class LWP;

class Instr {
	LWP		*lwp;
	unsigned char	get_opcode(unsigned *, unsigned *);
	char		*compoff(Iaddr jmp, char *, Iaddr addr);
	void		getbytes(int no_bytes, char *, long *);
	void 		imm_data(int no_bytes, int opindex);
	void		displacement(int no_bytes, int opindex, long *);
	void		get_modrm_byte(unsigned *, unsigned *, unsigned *);
	void		check_override(int opindex);
	void		get_operand(unsigned mode, unsigned r_m, int wbit,
				int opindex, int symbolic);
	int		get_text( Iaddr ); // get text from process, as is
	int		get_text_nobkpt( Iaddr );
			 // get text from process, with no breakpoints
public:

			Instr( LWP *l) { lwp = l; }
			~Instr() {};
	Iaddr		retaddr( Iaddr ); // return address if call instr
	int		is_bkpt( Iaddr ); // breakpoint instruction?
	char		*deasm( Iaddr, int &inst_size, int symbolic,
				char *name, Iaddr offset);
				// assembly language instructions
	Iaddr		adjust_pc();	// adjust pc after breakpoint
	int		nargbytes( Iaddr );// number of argument bytes
	Iaddr		fcn_prolog( Iaddr, int skip, int &prosize, 
					RegRef* ); 
				// function prolog with saved registers
	Iaddr           brtbl2fcn( Iaddr ); // branch table to funciton
	Iaddr           fcn2brtbl( Iaddr, int offset ); 
				// function to branch table
	int		iscall( Iaddr caller, Iaddr callee );	
				// is previous instruction CALL to current 
				// func?
	int		isreturn( Iaddr );	// is next instruction return
	Iaddr		jmp_target( Iaddr );	// target addr if JMP
	int		find_return(Iaddr pc, Iaddr &esp, Iaddr &ebp);
	int		call_size(Iaddr);
};
#endif
