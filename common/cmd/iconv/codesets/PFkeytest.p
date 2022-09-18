#ident	"@(#)iconv:codesets/PFkeytest.p	1.1.1.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/iconv/codesets/PFkeytest.p,v 1.1 91/02/28 17:33:39 ccs Exp $"
#
# A sample test for timed input of ANSI-style arrow keys.
#
map (funkey) {
	timed
	define(fun "\033[")
	fun(A "funkeyA")
	fun(B "funkeyB")
	fun(C "funkeyC")
	fun(D "funkeyD")
}
