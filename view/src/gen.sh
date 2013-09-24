#!/bin/ksh
#-transparent-color E79D 
CMD="convert -size 16x16 -background #e5e5e5e5e5e5 -fill"
CMD="convert -size 16x16 -background grey90 -fill"
# convert -list color | grep 229

CIRCLE="circle 1,1 5,5;"
$CMD 'orange'   -draw "$CIRCLE text 8,12 'u'" circle_red.svg icon_force_abort.xpm
$CMD 'green' -draw "$CIRCLE text 8,12 'e'" circle_red.svg icon_user_edit.xpm
$CMD 'red' -draw "$CIRCLE   text 8,12 'a'" circle_red.svg icon_task_aborted.xpm
$CMD 'red' -draw "$CIRCLE   text 8,12 'e'" circle_red.svg icon_edit_failed.xpm
$CMD 'red' -draw "$CIRCLE   text 8,12 'c'" circle_red.svg icon_cmd_failed.xpm
$CMD 'red' -draw "$CIRCLE   text 8,12 's'" circle_red.svg icon_no_script.xpm
$CMD 'red' -draw "$CIRCLE   text 8,12 'k'" circle_red.svg icon_killed.xpm
$CMD 'green' -draw "$CIRCLE text 8,12 'r'" circle_red.svg icon_byrule.xpm
$CMD 'green' -draw "$CIRCLE text 8,12 'q'" circle_red.svg icon_queuelimit.xpm

for f in *xpm; do perl xpm2cc $f > $(basename $f xpm)cc; done



