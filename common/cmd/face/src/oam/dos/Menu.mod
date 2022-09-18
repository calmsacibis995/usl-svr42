#ident	"@(#)face:src/oam/dos/Menu.mod	1.1.4.2"
#ident  "$Header: Menu.mod 1.5 91/10/15 $"

menu=$$uxface:286:"Modify Global MS-DOS Programs"

framemsg=$$uxface:529:"Move the cursor to the item you want and press ENTER to select it."

help=open TEXT $INTFBASE/Text.itemhelp 'menu:F1'

`fmlgrep '^vmsys:' /etc/passwd | fmlcut -f6 -d: | set -e VMSYS;
$VMSYS/bin/listserv -p -m VMSYS`
