#ident	"@(#)filemgmt:common/cmd/oamintf/files/make/Text.make	1.5.2.2"
#ident  "$Header: Text.make 2.0 91/07/12 $"
Title="Create a File System (make)"
help=OPEN TEXT $INTFBASE/Text.itemhelp $LININFO
close=`/bin/rm /tmp/make.out 2>/dev/null`

`readfile $INTFBASE/display.msg | message -p`

begrow=distinct
begcol=distinct
rows=6
columns=50

text="`readfile /tmp/make.out`"
