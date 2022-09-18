#ident	"@(#)olmisc:strprep.awk	1.1"
/XtNresize/||/XtCResize/||/XtNeditType/||/XtNfile/||/XtNfont/||/XtNjustify/||/XtNlabel/||/XtNorientation/||/XtNselection/||/XtNselectionArray/||/XtNtextSource/||/XtCEditType/||/XtCFile/||/XtCFont/||/XtCJustify/||/XtCLabel/||/XtCMargin/||/XtCOrientation/||/XtCPosition/||/XtCSelection/||/XtCSelectionArray/||/XtCTextPosition/||/XtCTextSource/{
x=1
}
/#define/ {
stub = substr($2,1,3)
name = substr($2,4)
if (c > 0)
   if (x == 1)
      printf "/* STRING (%s,%s) */\n", stub, name
   else
      printf "STRING (%s,%s)\n", stub, name
c++
x=0
}
