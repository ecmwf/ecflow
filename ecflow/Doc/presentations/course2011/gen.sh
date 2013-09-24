#!/bin/ksh
set -eux
p=pygmentize
p="/home/ma/map/course/s5/Pygments-1.4/pygmentize -O full"
p="/home/ma/map/course/s5/Pygments-1.4/pygmentize -O full,bg=white,python=cool"
p="/home/ma/map/course/s5/Pygments-1.4/pygmentize -O bg=white,python=cool"

convert_k() {
set -eux
kind=$1; shift
format=gif
format=html
for file in $*; do
  $p -f $format -l $kind -o $file.html $file
done
}

convert() {
set -eux
for i in $*; do
case $i in 
expanded*) dim=35
  cat $i | cut -c1-$dim | $p -f html -l raw -o $i.html;;
*.def|*.sms|*.cdp|*.sh) convert_k sh $i;;
*.py)  convert_k python $i;;
esac
done
}

gen_code() {
set -eux
$p -f html -l sh -o skull.def.html skull.def
$p -f html -l sh -o skull.sh.html skull.sh
$p -S emacs -f html > style.css

ins="expanded.sms expanded.ecf"
convert raw $ins
toconvsh="inc_ecf.h compatible.def skull.sh ex.sh ex.cdp ex.cdp ex.sh"
toconvpy="ex.py skull.py python_endt.h python_header.h python.sms"
convert_k sh $toconvsh
convert_k python $toconvpy
convert_k perl perl_endt.h perl_header.h perl.sms
}

export PYTHONPATH=:/home/ma/map/course/s5/docutils:/home/ma/map/course/s5/docutils/extras:./
inst=~map/course/s5/docutils/tools
rst2s5="$inst/rst2s5.py --stylesheet=src/style.css "
rst2h=$inst/rst2html.py
rst2l=$inst/rst2latex.py
args="--cloak-email-addresses --current-slide "
# src=course2011/course2011.rst
from=migration; dest=$from.s5.html
from=example; dest=$from.s5.html
from=s5; dest=$from.s5.html

k=$1
if [[ $# = 0 ]] ; then
  $rst2s5 $args --theme=small_ecmwf $from.rst $dest
elif [[ $k = "clean" ]]; then
  rm m*.dvi m*.log m*.pdf m*out m*toc m*aux
elif [[ $k = code ]]; then 
  if [[ -d src ]]; then cd src; fi
  gen_code
  cd ..; ./gen.sh 

elif [[ $k = "start" ]]; then
  ecflow_server --port 9130 &
  ecf="ecflow_client --port 9130 --host ibis"
  $ecf --load ../src/expanded.ecf
else
  $rst2h $from.rst $from.html
  $rst2l $from.rst $from.tex && pdflatex $from.tex && \
    sed -e 's:height=600bp:width=300bp:' $from.tex | \
    sed -e 's:height=400bp:width=300bp:'  | \
    sed -e 's:\begin{longtable*}:begin{longtable*}[c]{|l|l|l|l|} %:' | \
    > m.tex && mv m.tex $from.tex
fi

exit 0
/usr/local/apps/python/current/bin/

$p -f html -l sh -o skull.html -O bg=white,python=cool skull.def

_mapping.py; python setup.py install

/home/ma/map/course/s5/Pygments-1.4/pygments/lexers/_mapping.py

toks="EOF abort action autocancel   automigrate  autorestore clock complete cron date day defstatus edit endfamily endsuite endtask event extern family inlimit label late limit meter owner repeat suite task text time today triggertokenstring"



