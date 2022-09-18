#ident	"@(#)iconv:codesets/Dvorak.p	1.1.1.1"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/iconv/codesets/Dvorak.p,v 1.1 91/02/28 17:33:37 ccs Exp $"
#
# This is a sample Dvorak keyboard arrangement.  It may not be
# complete and is not guaranteed, but the alphabet is hopefully
# in the right place.
#
map (Dvorak) {
	keylist("qwertyuiop" ";,.pyfgcrl")
	keylist("QWERTYUIOP" ":/?PYFGCRL")
	keylist("asdfghjkl;" "aoeuidhtns")
	keylist("ASDFGHJKL:" "AOEUIDHTNS")
	keylist("zxcvbnm,./" "<qjkxbmwvz")
	keylist("ZXCVBNM<>?" ">QJKXBMWVZ")
}
