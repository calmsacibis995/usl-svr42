#ident	"@(#)nadmin.rfs:other/rfs/net/Text.msg	1.2.5.1"
#ident  "$Header: Text.msg 2.0 91/07/12 $"
###############################################################################
#    Module Name:  Text.display
#    Description:  
#     Lists Netwks supporting RFS  
#	(get netspecs of transport providers supporting RFS - dname -n
#	 get description/comment from /etc/netconfig
#
###############################################################################
title="Sets Network Support for Remote File Sharing"
columns=60

`readfile $INTFBASE/display.msg | message`

text="
`readfile $ARG1`"
