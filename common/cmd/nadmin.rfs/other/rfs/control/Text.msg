#ident	"@(#)nadmin.rfs:other/rfs/control/Text.msg	1.4.4.1"
#ident  "$Header: Text.msg 2.0 91/07/12 $"
title="$ARG1"
columns=50
`readfile $INTFBASE/display.msg | message -p`

text="`readfile $ARG2`"
