#ident	"@(#)iconv:codesets/Deutsche.p	1.1.1.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/iconv/codesets/Deutsche.p,v 1.1 91/02/28 17:33:35 ccs Exp $"
#
# Sample simple mapping for u-umlaut, y->z, z->y, ss and "".
# This is a sample only.
#
map (Deutsche) {
	define(umlaut \042)
	umlaut(u '\374')
	umlaut(\042 \042)
	string("#s" '\315')
	keylist("yzYZ" "zyZY")
}
