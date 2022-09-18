/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)uts-x86:util/misc.s	1.12"
	.ident	"$Header: $"
	.file	"util/misc.s"

/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft 
/	Corporation and should be treated as Confidential.

/	misc.s - miscellaneous assembler utility routines.
/	This is the home of last resort for assembler routines
/	for which no more appropriate place can be found.
/	Try to keep this stuff to a minimum.

include(../util/symbols.m4)

	.set	NBPW, 4		/ no of bytes in a word.

	.set    CR0_ET, 0x10            / extension type (1->387,0->???)
	.set    CR0_TS, 0x08            / task switched
	.set    CR0_EM, 0x04            / use math emulation
	.set    CR0_MP, 0x02            / math coprocessor present
	.set    CR0_PE, 0x01            / protection enable
	.set	PS_T, 0x100		/ trace flag

ifdef(`WEITEK',`
	.set	WEITEK_LDCTX,	0xffc0c000	/ load context register
	.set	WEITEK_STCTX,	0xffc0c400	/ store context register
	.globl	weitek_kind
	.globl	weitek_map
	.globl	weitek_unmap
')

	.text

	.align	4
	.type	flushtlb,@function
	.globl	flushtlb
flushtlb:
	movl	%cr3, %eax
	movl	%eax, %cr3
	ret
	.size	flushtlb,.-flushtlb

	.align	4
	.type	_cr2,@function
	.globl	_cr2
_cr2:
	movl	%cr2, %eax
	ret
	.size	_cr2,.-_cr2

	.align	4
	.type	_cr3,@function
	.globl	_cr3
_cr3:
	movl	%cr3, %eax
	andl	$0x7FFFFFFF,%eax
	ret
	.size	_cr3,.-_cr3

	.align	4
	.type	_cr4,@function
	.globl	_cr4
_cr4:
	movl	%cr4, %eax
	ret
	.size	_cr4,.-_cr4

	.align	4
	.type	_wdr0,@function
	.globl  _wdr0
_wdr0:
	movl    4(%esp), %eax
	movl    %eax, %db0
	ret
	.size	_wdr0,.-_wdr0

	.align	4
	.type	_wdr1,@function
	.globl  _wdr1
_wdr1:
	movl    4(%esp), %eax
	movl    %eax, %db1
	ret
	.size	_wdr1,.-_wdr1

	.align	4
	.type	_wdr2,@function
	.globl  _wdr2
_wdr2:
	movl    4(%esp), %eax
	movl    %eax, %db2
	ret
	.size	_wdr2,.-_wdr2

	.align	4
	.type	_wdr3,@function
	.globl  _wdr3
_wdr3:
	movl    4(%esp), %eax
	movl    %eax, %db3
	ret
	.size	_wdr3,.-_wdr3

	.align	4
	.type	_wdr6,@function
	.globl  _wdr6
_wdr6:
	movl    4(%esp), %eax
	movl    %eax, %db6
	ret
	.size	_wdr6,.-_wdr6

	.align	4
	.type	_wdr7,@function
	.globl  _wdr7
_wdr7:
	movl    4(%esp), %eax
	movl    %eax, %db7
	ret
	.size	_wdr7,.-_wdr7

	.align	4
	.type	_dr0,@function
	.globl  _dr0
_dr0:
	movl    %db0, %eax
	ret
	.size	_dr0,.-_dr0

	.align	4
	.type	_dr1,@function
	.globl  _dr1
_dr1:
	movl    %db1, %eax
	ret
	.size	_dr1,.-_dr1

	.align	4
	.type	_dr2,@function
	.globl  _dr2
_dr2:
	movl    %db2, %eax
	ret
	.size	_dr2,.-_dr2

	.align	4
	.type	_dr3,@function
	.globl  _dr3
_dr3:
	movl    %db3, %eax
	ret
	.size	_dr3,.-_dr3

	.align	4
	.type	_dr6,@function
	.globl  _dr6
_dr6:
	movl    %db6, %eax
	ret
	.size	_dr6,.-_dr6

	.align	4
	.type	_dr7,@function
	.globl  _dr7
_dr7:
	movl    %db7, %eax
	ret
	.size	_dr7,.-_dr7

	.align	4
	.type	_rdmsr,@function
	.globl  _rdmsr
_rdmsr:
	pushl	%edx
	pushl	%ecx
	movl	12(%esp), %ecx		/ which register to access
	rdmsr
	movl    16(%esp), %ecx
	movl	%eax, (%ecx)
	movl	%edx, 4(%ecx)
	popl	%ecx
	popl	%edx
	ret
	.size	_rdmsr,.-_rdmsr

	.align	4
	.type	_wrmsr,@function
	.globl  _wrmsr
_wrmsr:
	pushl	%edx
	pushl	%ecx
	movl    16(%esp), %ecx
	movl	(%ecx), %eax
	movl	4(%ecx), %edx
	movl	12(%esp), %ecx		/ which register to access
	wrmsr
	popl	%ecx
	popl	%edx
	ret
	.size	_wrmsr,.-_wrmsr

/ caddr_t _ebp()
/ Returns the frame pointer in the callers environment.
/
	.align  4
	.type   _ebp,@function
	.globl  _ebp
_ebp:
	movl	%ebp, %eax
	ret
	.size   _ebp,.-_ebp

/ caddr_t _esp()
/ Returns the stack pointer in the callers environment.  Note we add
/       4 to %esp to account for the return address on the stack
/
	.align  4
	.type   _esp,@function
	.globl  _esp
_esp:
	leal    4(%esp), %eax
	ret
	.size   _esp,.-_esp

/ unsigned long _ebx()
/ Returns the value of %ebx register
	.align  4
	.type   _ebx,@function
	.globl  _ebx
_ebx:
	movl	%ebx, %eax
	ret
	.size   _ebx,.-_ebx

/ unsigned long _edi()
/ Returns the value of %edi register
	.align  4
	.type   _edi,@function
	.globl  _edi
_edi:
	movl	%edi, %eax
	ret
	.size   _edi,.-_edi

/ unsigned long _esi()
/ Returns the value of %esi register
	.align  4
	.type   _esi,@function
	.globl  _esi
_esi:
	movl	%esi, %eax
	ret
	.size   _esi,.-_esi

/       load task register
	.align	4
	.type	get_tr,@function
	.globl	get_tr
get_tr:
	xorl	%eax, %eax
	str	%ax
	ret
	.size	get_tr,.-get_tr

	.align	4
	.type	loadtr,@function
	.globl  loadtr
loadtr:
	movw	4(%esp), %ax
	ltr	%ax
	ret
	.size	loadtr,.-loadtr

	.align	4
	.type	loadldt,@function
	.globl  loadldt
loadldt:
	movw	4(%esp), %ax
	lldt	%ax
	ret
	.size	loadldt,.-loadldt

/
/	min() and max() routines
/
	.set	ARG1, 8
	.set	ARG2, 12

	.align	4
	.type	min,@function
	.globl	min
min:
	pushl	%ebp				/ save old %ebp
	movl	%esp, %ebp
	movl	ARG1(%ebp), %eax		/ %eax <- arg1
	cmpl	ARG2(%ebp), %eax		/ compare args
	jbe	.minxit				/ ? %eax <= ARG2. unsigned
	movl	ARG2(%ebp), %eax		/ %eax = arg2
	.align	4
.minxit:
	popl	%ebp
	ret
	.size	min,.-min


	.align	4
	.type	max,@function
	.globl	max
max:
	pushl	%ebp
	movl	%esp, %ebp			/ set stack frame
	movl	ARG1(%ebp), %eax		/ %eax<-arg1
	cmpl	ARG2(%ebp), %eax		/ compare args
	jae	.maxit				/ ?%eax>=ARG2. unsigned
	movl	ARG2(%ebp), %eax		/ %eax = arg2
.maxit:
	popl	%ebp
	ret
	.size	max,.-max


/
/	upc_scale(pc - u.u_prof.pr_offset, u.u_prof.pr_scale)
/
/	Scales the PC value for user program profiling.
/	Returns slot number in u.u_prof.pr_base array.
/
	.set	REL_PC, 4	/ note: offsets relative to %esp
	.set	SCALE, 8

	.align	4
	.type	upc_scale,@function
	.globl	upc_scale
upc_scale:
	movl	REL_PC(%esp), %eax	/ pc relative to start of region
	mull	SCALE(%esp)		/ Multiply by fractional result
	shrdl	$17,%edx,%eax		/ Scale down to a useful range (>> 17)
	ret
	.size	upc_scale,.-upc_scale


	.globl	oldproc
	.globl	segu_release
	.globl	curproc
ifdef(`WEITEK',`
	.globl  weitek_save
')
	.align	4
ifdef(`KPERF',`
	.type	KPswtch,@function
	.globl	KPswtch
KPswtch:
',`
	.type	swtch,@function
	.globl	swtch
swtch:
')
	call	pswtch
	/ At this point we have already done all the mapping work for
	/ the new process. The tss of the new process is mapped by
	/ JTSSSEL.  LDTSEL descr contains linear addr of the new ldt
	/ (as it will appear in the address mapping of the new process).
	movl	u+u_procp, %eax		/ Is the old process same as
	cmpl	%eax, curproc		/ the new one?
	je	.noswtch			/ If so skip the context switch

ifdef(`WEITEK',`
	/ "call	weitek_save" equivalent
	/ however we still call weitek_save to do the WEITEK_HW testing!
	movl	weitek_proc, %ecx
	jcxz	.donothing
	cmpl	%ecx, u+u_procp
	jne	.donothing
	pushl	$u+u_weitek_reg
	call	weitek_save
	popl	%ecx
	movl	u+u_procp, %eax		/ reload %eax with u.u_procp
.donothing:
')

	/ clear 286 system call gate if used by this process
	movl    u+u_callgatep, %ecx     / addr of call gate in gdt
	jcxz	.swnocallgate		/ if null, dont bother
	subl    %edx, %edx              / zero
	movl    %edx, 0(%ecx)
	movl    %edx, 4(%ecx)
.swnocallgate:

	/ copy reference and modify bits from usertable[] to p_ubptbl[]
	pushl	%esi
	pushl	%edi
	movl	u+u_tss, %esi		/ The ljmp below will save the current
	movl	$u+KSTKSZ, t_esp0(%esi)	/ state into the outgoing tss. We must
                                        / make sure that page gets marked as
                                        / modified. This wont happen auto-
                                        / matically, since the old task segment
                                        / is addressed via the usertable ptes,
                                        / not the p_ubptbl ptes. Instead, we
					/ do it here by modifying the ESP0
					/ field in the tss before we copy the
					/ modify bits.
	movzwl	p_usize(%eax), %ecx
	movl	usertable, %esi
	movl	p_ubptbl(%eax), %edi
.rmcopy_loop:
	slodl
	andl	$[PG_REF+PG_M], %eax
	orl	(%edi), %eax
	sstol
	loop	.rmcopy_loop
	popl	%edi
	popl	%esi

.jmptotss:
	cli				/ disable interrupts
	str	%dx			/ Remember which TSS were in

	/ We want no nop's after the ljmp because
	/ if the processor were to take a fault
	/ at that time, there would not be a valid stack,
	/ therefore we force the desired alignment

	.align	4
	nop

	ljmp	$JTSSSEL, $0		/ Old process will resume at
					/ the next instruction
ifdef(`KPERF',`
	.size	KPswtch,.-KPswtch
',`
	.size	swtch,.-swtch
')

	/ newproc returns here for the child. Remember
	/ to preserve the register variables (ebx, esi,
	/ edi) and eax.
	/
	/ Watch out for ebx, which holds a pointer to
	/ the tss.

	.align	4
	.type	resume,@function
	.globl	resume
resume:
	movl	$nmi_stack+KSTKSZ, %esp	/ usable stack in case of nmi

	movl	curproc, %eax		/ eax = curproc
	movzwl	p_usize(%eax), %ecx
	movl	p_ubptbl(%eax), %esi
	movl	usertable, %edi
	rep				/ copy p_usize page table entries
	smovl				/ from p_ubptbl[] to usertable[]

	/
	/ First ublock page must be user read/write, for two reasons:
	/ 1. To get around a bug in the B1 stepping of the 80386, the
	/    kernel stack must be user readable.
	/ 2. Floating point emulators (executing in user mode) must be
	/    able to read and write the floating-point state.
	/ So, the ublock is set up so the first page contains (only) the
	/ kernel stack and the floating-point state.

	movl	usertable, %eax
	orl	$[PG_US+PG_RW], (%eax)

	call	restorepd		/ set up curprocs page directory

	/
	/ now running on new ublock
	/

	leal	gdt+[JTSSSEL&~0x7], %esi	/ esi = &gdt[JTSSSEL]
	movb	7(%esi), %bh
	movb	4(%esi), %bl
	shl	$16, %ebx
	movw	2(%esi), %bx		/ ebx = &(tss used for context switch)

	movl	t_esp(%ebx), %esp	/ use kernel stack in ublock
	ltr	t_edx(%ebx)

	/* XENIX Support */
	leal	idt+[0xf0\*8], %edi	/ if necessary,
	movl	u+u_fpintgate, %eax	/ customize idt[0xf0 .. 0xff]
	movl	u+u_fpintgate+4, %esi	/ for this process
	cmpl	%eax, (%edi)
	jne	.idtchg
	cmpl	%esi, 4(%edi)
	je	.idtok
.idtchg:
	movl	$[2\*0x10], %ecx	
.idtfill:
	sstol				/ copy u_fpintgate to the idt[]
	xchgl	%eax, %esi
	loop	.idtfill
.idtok:
/* End XENIX Support */
	sti                             / enable interrupts

ifdef(`WEITEK',`
	/ "call	weitek_restore"
	cmpb	$0, u+u_weitek
	je	.skipit
	movl	weitek_proc, %eax
	cmpl	%eax, u+u_procp
	je	.skipit
	call	init_weitek
	pushl	$u+u_weitek_reg
	call	weitek_restore
	popl	%ecx
	movl	u+u_procp, %eax
	movl	%eax, weitek_proc
.skipit:
')

ifdef(`MERGE386',`
	cmpl	$0, merge386enable
	je	.novm86_swtch
	pusha				/ save registers
	call	vm86_swtch		/ call C routine for special processing
	popa				/ of switches to and from vm86 proc
.novm86_swtch:
')

	/ We need to free the ublock for the previous
	/ process if it did an exit.
	movl	oldproc, %ecx
	jcxz	.restorecallgate
	pushl	%ecx
	call	segu_release
	popl	%ecx
	movl	$0, oldproc

	/
	/ restore 286 system call gate if used by this process
	/
.restorecallgate:
	movl    u+u_callgatep, %ecx     / addr of call gate in gdt
	jcxz	.nocallgate
	movl    u+u_callgate, %eax
	movl    %eax, 0(%ecx)
	movl    u+u_callgate+4, %eax
	movl    %eax, 4(%ecx)
.nocallgate:

ifdef(`DEBUG',`
	call	db_resume
')

	mov	t_eax(%ebx), %eax	/ restore modified register variables
	mov	t_edi(%ebx), %edi	/ (ebx, esi, edi) and eax
	mov	t_esi(%ebx), %esi	/ from the tss
	mov	t_ebx(%ebx), %ebx

.noswtch:
	ret
	.size	resume,.-resume


	.text
	.align	4
	.type	monitor,@function
	.globl	monitor
monitor:
	int	$1
	ret
	.size	monitor,.-monitor

/ The following code is used to generate a 10 microsecond delay
/ routine.  It is initialized in ml/pit.c.

	.align	4
	.type	tenmicrosec,@function
	.globl	tenmicrosec
	.globl	microdata
tenmicrosec:
	movl	microdata, %ecx		/ Loop uses ecx.
microloop:
	loop	microloop
	ret
	.size	tenmicrosec,.-tenmicrosec
