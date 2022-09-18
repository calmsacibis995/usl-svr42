	.ident	"@(#)uts-x86:util/mod/stub.m4	1.2"
	.ident	"$Header: $"
/
/ This file contains macros for the stubs mechanism of
/ the Dynamic Loadable Modules.
/

/
/ MODULE(module)
/
define(`MODULE',`
	.data
$1_modinfo_name:
	.string	"$1"
	.align	4
	.globl	$1_modinfo
$1_modinfo:
	.long	$1_modinfo_name
')
	
/
/ END_MODULE(module)
/
define(`END_MODULE',`
	.long	0
')

/
/ STUB_COMMON(module, fcnname, install_fcn, retfcn)
/
define(`STUB_COMMON',`
	.text
	.globl	$2
	.align	8
$2_install:
	pushl	$$2_info
	call	mod_stub_load
	addl	$ 4, %esp
	orl	%eax, %eax
	je	$2
	jmp	*[$2_info + 12]
	.align	8
$2:
	jmp	*$2_info

	.data
$2_info:
	.long	$3
	.long	$1_modinfo
	.long	$2
	.long	$4
')

/
/ STUB(module, fcnname, retfcn)
/
define(`STUB', `
	STUB_COMMON($1, $2, $2_install, $3)')

/
/ weak stub, will not cause autoloading of the module
/
/ WSTUB(module, fcnname, retfcn)
/
define(`WSTUB', `
	STUB_COMMON($1, $2, $3, $3)')

