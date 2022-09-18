#ident	"@(#)iconv:codesets/Case.p	1.1.1.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/iconv/codesets/Case.p,v 1.1 91/02/28 17:33:31 ccs Exp $"
#
# Sample ASCII Upper-to-lower, Lower-to-upper, and "case-swap" maps.
#
map (uclc) {
	keylist("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz")
}

map (lcuc) {
	keylist("abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ")

}

map (uclc_lcuc) {
	keylist("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz")
	keylist("abcdefghijklmnopqrstuvwxyz"
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ")
}
