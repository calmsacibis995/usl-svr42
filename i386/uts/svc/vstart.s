/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)uts-x86:svc/vstart.s	1.10"
	.ident	"$Header: $"
	.file	"svc/vstart.s"

include(../util/symbols.m4)

	.set	NBPW, 4		/ no of bytes in a word.

	.set    CR0_ET, 0x10            / extension type (1->387,0->???)
	.set    CR0_TS, 0x08            / task switched
	.set    CR0_EM, 0x04            / use math emulation
	.set    CR0_MP, 0x02            / math coprocessor present
	.set    CR0_PE, 0x01            / protection enable
	.set	PS_T, 0x100		/ trace flag

/
/ This code is the init process; it is copied to user text space
/ and it then does exec( "/sbin/init", "/sbin/init", 0);
/
	.data
	.align	4

	.globl	icode
	.globl	szicode

	.set	_exec, 11
	.set	_open, 5
	.set	_write, 4
	.set	_close, 6

icode:
	movl	$UVTEXT, %eax		/ start of user text
	leal	argv_off(%eax), %ebx
	pushl	%ebx
	leal	sbin_off(%eax), %ebx
	pushl	%ebx
	pushl	$0		/ dummy <ret> so stack looks right in systrap
	movl	$_exec, %eax
	lcall	$USER_SCALL, $0	/ execl("/sbin/init", "/sbin/init", 0);

	/ exec failed - write an error message to /dev/sysmsg

	movl	$UVTEXT, %eax
	pushl	$2
	leal	sysmsg_off(%eax), %ebx
	pushl	%ebx
	pushl	$0		/ dummy <ret> so stack looks right in systrap
	movl	$_open, %eax
	lcall	$USER_SCALL, $0	/ fd = open("/dev/sysmsg", 2);
	movl	%eax, %esi

	movl	$UVTEXT, %eax
	pushl	$ierr_len
	leal	ierr_off(%eax), %ebx
	pushl	%ebx
	pushl	%esi
	pushl	$0		/ dummy <ret> so stack looks right in systrap
	movl	$_write, %eax
	lcall	$USER_SCALL, $0	/ write(fd, IERR_MSG, strlen(IERR_MSG));

	movl	$UVTEXT, %eax
	pushl	%esi
	pushl	$0		/ dummy <ret> so stack looks right in systrap
	movl	$_close, %eax
	lcall	$USER_SCALL, $0	/ close(fd);

	jmp	.

	.align	4
argv:
	.set	argv_off, argv - icode
	.long	UVTEXT+sbin_off
	.long	0

sbin_init:
	.set	sbin_off, sbin_init - icode
	.string	"/sbin/init"

sysmsg:
	.set	sysmsg_off, sysmsg - icode
	.string	"/dev/sysmsg"

ierr_msg:
	.set	ierr_off, ierr_msg - icode
	.string	"** Cant exec /sbin/init\n\n"
	.set	ierr_len, . - ierr_msg - 1

icode_end:

	.align	4
szicode:
	.long	icode_end - icode

/ Put 386/20 into protected mode and call mlsetup/main to do rest of
/ initialization. On return from main, we want to do an inter-level
/ return to the init process. Push enough on stack to make it look like
/		oldss
/		oldesp
/		oldefl
/		oldcs
/		oldeip
/ and initialize ds/es to point to user ds
/ Now a reti will take us to the user process.
/		
	.text

	.set	PROTPORT, 0xE0
	.set	PROTVAL, 1

	.globl	u
	.globl	userstack
	.globl	v
	.globl	mlsetup
	.globl	cmn_err
	.globl	main

	.align	8
	.type	vstart,@function
	.globl	vstart
vstart:
	call	mlsetup

/ Until p0u has run (in mlsetup), we cannot use the kernel
/ stack in the u structure. The intial ktss is set up so we
/ use the slop space between the end of pmons data space and
/ the start of kernel text. p0u resets it to the correct value so
/ proc 0s tss is valid. Well switch to u_stack now.
	movl	$u+KSTKSZ, %esp

/
/ Identify the cpu we are running on
/
	.set	EFL_AC, 0x40000		/ alignment check (1->check)
	.set	EFL_ID,0x200000		/ cpuid opcode (1->supported)

	.data
	.align	4
	.globl	cpu_family
	.globl	cpu_model
	.globl	cpu_stepping
cpu_family:
	.long	0
cpu_model:
	.long	0
cpu_stepping:
	.long	0

	.text
	pushfl				/ push FLAGS value on stack
	popl	%eax			/ get FLAGS into AX
	pushl	%eax			/ save original FLAGS
	movl	%eax, %ecx		/ save copy of FLAGS

	xorl	$EFL_AC, %eax		/ flip AC bit for new FLAGS
	pushl	%eax			/ push new value on stack
	popfl				/ attempt setting FLAGS.AC
	pushfl				/ push resulting FLAGS on stack
	popl	%eax			/ get that into AX
	cmpl	%eax, %ecx		/ succeeded in flipping AC?
	je	cpu_is_386		/ AX is same as CX for i386

	movl	%ecx, %eax		/ get original FLAGS again
	xorl	$EFL_ID, %eax		/ flip ID bit for new FLAGS
	pushl	%eax			/ push new value on stack
	popfl				/ attempt setting FLAGS.ID
	pushfl				/ push resulting FLAGS on stack
	popl	%eax			/ get that into AX
	cmpl	%eax, %ecx		/ succeeded in flipping ID?
	je	cpu_is_486		/ AX is same as CX for i486

	cpuid				/ get cpu family-model-stepping
					/ (sets %eax, %ebx, %ecx, %edx!)

	movl	%eax, %ebx		/ extract stepping id
	andl	$0x00F, %ebx		/     from bits [3:0]
	movl	%ebx, cpu_stepping

	movl	%eax, %ebx		/ extract model
	andl	$0x0F0, %ebx		/     from bits [7:4]
	shrl	$4, %ebx
	movl	%ebx, cpu_model

	movl	%eax, %ebx		/ extract family
	andl	$0xF00, %ebx		/     from bits [11:8]
	shrl	$8, %ebx
	movl	%ebx, cpu_family

	movl    %cr4,%eax
	orl		$0x44,%eax		/ enable machine check exception, disable rdtsc
	movl	%eax,%cr4

	jmp	cpu_identified

cpu_is_486:
	movl	$4, cpu_family
	jmp	cpu_identified

cpu_is_386:
	movl	$3, cpu_family

cpu_identified:
	popfl				/ restore original FLAGS

/
/ Do 80386 B1 stepping detection, if requested.
/
/ First, we check the tuneable; 2 means do automatic detection
/
	cmpl	$2,do386b1		/ if do386b1 < 2,
	jb	.skip_b1_detect		/    skip B1 detection
/
/ The detection is done by looking for the presence of the Errata 5 bug,
/ which causes a single step of REP MOVS to go through 2 iterations, not 1.
/
	pushl	[idt+8+4]		/ save current debug trap
	pushl	[idt+8]
	movl	$.b1_ss_trap,%eax
	movw	%ax,[idt+8]		/ set debug trap to b1_ss_trap
	shrl	$16,%eax
	movw	%ax,[idt+8+6]
	pushfl				/ save flags register
	movl	$2,%ecx			/ set up for a REP MOVS w/count of 2
	movl	$u,%esi			/   (to and from an arbitrary addr)
	movl	%esi,%edi
	pushl	$PS_T			/ set the single step flag
	popfl
	rep
	movsb
.b1_ss_trap:
	addl	$12,%esp		/ skip the iret
	xorl	$1,%ecx			/ ECX has 1 if no bug, else 0 -
	movl	%ecx,do386b1		/   store 0 or 1, resp., in d0386b1
	popfl				/ restore flags
	popl	[idt+8]			/ restore debug trap
	popl	[idt+8+4]
.skip_b1_detect:
/
/ Set up for floating point.  Check for any chip at all by tring to
/ do a reset.  if that succeeds, differentiate via cr0.
/
	clts                            / clear task switched bit in CR0
	fninit                          / initialize chip
	fstsw	%ax			/ get status
	orb	%al,%al			/ status zero? 0 = chip present
	jnz     .mathemul                / no, use emulator
/
/ at this point we know we have a chip of some sort; 
/ use cr0 to differentiate.
/
	movl    %cr0,%edx               / check for 387 present flag
/	testl	$CR0_ET,%edx            / ...
/	jz      .is287                   / z -> 387 not present
	movb    $FP_387,fp_kind         / we have a 387 chip
	movl	do386b1,%eax
	movl	%eax,do386b1_387	/ set flag for B1 workarounds and 387 chip
/	jmp     .mathchip
/
/ No 387; we must have an 80287.
/
/.is287:
/	fsetpm				/ set the 80287 into protected mode
/	movb    $FP_287,fp_kind         / we have a 287 chip
/
/ We have either a 287 or 387.
/
.mathchip:
	andl    $-1![CR0_TS|CR0_EM],%edx	/ clear emulate math chip bit
	orl     $[CR0_ET|CR0_MP],%edx           / set 387 math chip present bit
	movl    %edx,%cr0               / in machine status word
	movl	do386b1,%eax
	movl	%eax,do386b1_x87	/ set flag for B1 workarounds and math chip
	orl	%eax,%eax
	jnz	.b1_enabled		/ if B1 workarounds disabled
	movl	%eax,do387cr3		/    disable do387cr3
	jmp	.cont
.b1_enabled:
	cmpl	$2,do387cr3		/ if = 2, auto-detect if workaround
	jb	.skip_cr3_detect	/    is possible on this machine
/
/ At this point, we have determined that the 387cr3 workaround for
/ B1 stepping Errata 21 is desired.  Determine if the hardware can support
/ the workaround.
/
	movl	$0,do387cr3		/ clear do387cr3 for now
	pushl	kspt0			/ save current pte (for D0000000)
			/ set pte to 2G alias for kpd0
	movl	$0x80000001+[KPTBL_LOC+0x1000],kspt0
	movl	%cr3,%eax		/ flush tlb
	movl	%eax,%cr3
	movl	kpd0,%eax		/ read the first kpd0 entry
	cmpl	%eax,0xD0000000		/ does it match the aliased value?
	jne	.cr3_disable		/ no, we cant use workaround
	incl	0xD0000000		/ change the entry via the alias
	incl	%eax
	cmpl	%eax,kpd0		/ did the kpd0 entry change also?
	jne	.cr3_disable		/ no, we cant use workaround
	decl	kpd0			/ alias was successful, restore kpd0
	incl	do387cr3		/ turn on do387cr3 flag
.cr3_disable:
	popl	kspt0			/ restore pte
	movl	%cr3,%eax		/ flush tlb
	movl	%eax,%cr3
.skip_cr3_detect:
	cmpl	$0,do387cr3
	jz	.cont
	movl	$0x80000000,fp387cr3	/ enable B1 workaround #21
	movl	%cr3, %eax
	orl	fp387cr3,%eax
	movl	%eax, %cr3
	jmp	.cont
/
/ Assume we have an emulator.
/
.mathemul:
	movl    %cr0,%edx
	andl    $-1!CR0_MP,%edx         / clear math chip present
	orl     $CR0_EM,%edx            / set emulate math bit
	movl    %edx,%cr0               / in machine status word
	movb    $FP_SW,fp_kind          / signify that we are emulating
.cont:

ifdef(`WEITEK',`
/
/ test for presence of weitek chip
/ were going to commandeer a page of kernel virtual space to map in 
/ the correct physical addresses.  then were going to play with what
/ we hope to be weitek addresses.  finally, well put things back the
/ way they belong.
/
/ extern unsigned long weitek_paddr;	/* chip physical address */
/
	cmpl	$0, weitek_paddr	/ if (weitek_paddr == 0)
	jz	.weitek_skip		/	goto weitek_skip;
	pushl	%ebx
	pushl	kspt0
	movl	$0xc0000003, kspt0	/ pfn c0000, sup, writeable, present
	movl	%cr3, %eax		/ flush tlb
	movl	%eax, %cr3
	movl	$KVSBASE, %ebx		/ base address for weitek area
	movb	$WEITEK_HW, weitek_kind	/ first assume that there is a chip
	movl	$0x3b3b3b3b, 0x404(%ebx) / store a value into weitek register.
	movl	0xc04(%ebx), %eax	/ and read it back out.
	cmpl	$0x3b3b3b3b, %eax
	jnz	.noweitek		/ no chip
	/ clear weitek exceptions so that floating point exceptions
	/ are reported correctly from here out
	/ initialize the 1167 timers
	movl    $0xc000c003, kspt0      / pfn c000c, sup, writeable, present
	movl	%cr3, %eax		/ flush tlb
	movl	%eax, %cr3
	movl	$0xB8000000, 0x000(%ebx)
	movl	0x400(%ebx), %eax	/ Check for 20 MHz 1163
	andl	$WEITEK_20MHz, %eax
	jnz	.w_init_20MHz
	movl 	$0x16000000, 0x000(%ebx)	/ 16 MHz 1164/1165 flowthrough
						/ timer
	jmp	.w_init_wt1

.w_init_20MHz:
	movl	$0x56000000, 0x000(%ebx)	/ 20 MHz 1164/1165 flowthrough
	movl	$0x98000000, 0x000(%ebx)	/ timer
	
.w_init_wt1:
	movl 	$0x64000000, 0x000(%ebx)	/ 1164 accumulate timer
	movl 	$0xA0000000, 0x000(%ebx)	/ 1165 accumulate timer
	movl 	$0x30000000, 0x000(%ebx)	/ Reserved mode bits (set to 0).
	movl 	weitek_cfg, %eax	/ Rounding modes and Exception
	movl 	%eax, 0x000(%ebx)	/ enables.
	movw	$0xF0, %dx		/ clear the fp error flip-flop
	movb	$0, %al
	outb	(%dx)
	/
	jmp	.weitek_done
.noweitek:
	movb	$WEITEK_NO, weitek_kind		/ no. no weitek

.weitek_done:
	popl	kspt0			/ get the old kpt0[0] back
	movl	%cr3, %eax		/ flush tlb
	movl	%eax, %cr3
	popl	%ebx
.weitek_skip:

')

	/ 
	/ extern int	margc;
	/ extern char	*margv[];
	/
	/ main(margc, margv);
	/
	pushl	$margv
	pushl	margc

	call	main

	addl	$8,%esp			/ pop off margc and margv

	cmpl	$UVTEXT, %eax
	je	.to_user
	call	*%eax			/ Kernel process. Shouldnt return
	jmp	.
.to_user:
	pushl	$USER_DS		/ oldss
	pushl	$userstack		/ old esp
	pushfl				/ old efl
	pushl	$USER_CS		/ old cs
	pushl	$UVTEXT			/ old eip

	movw	$USER_DS, %ax
	movw	%ax, %ds
	movw	%ax, %es
	iret

	/ NEVER REACHED
	jmp	.
