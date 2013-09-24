family $PROJECT
  def_fam
  families="00 12"

  edit ENSEMBLES 50
  trigger "(./make==complete)"
  for fam in  $families ; do
    family $fam # onsc
    if (( $fam == 00 )); then
	edit DELTA_DAY 1
    else
	edit DELTA_DAY 0
    fi
    edit EMOS_BASE $fam

    call_skull

    family pop;    oncl
    trigger "(./${PROJECT}==complete)"
    call_pop_skull
    endfamily

    family ws;    onws
    trigger "(./${PROJECT}==complete)"
    call_pop_skull
    endfamily

    endfamily #fam
  done
