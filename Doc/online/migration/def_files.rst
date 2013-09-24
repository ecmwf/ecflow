.. index::
   single: def_files
   
.. _def_files:
   
Definition files migration
--------------------------

This can be the most interesting part, requiring choices for

* suite durability

* common libraries coding

* ensuring maintainability

The CDP client provides an environment where CDP variables, aliases and
functions are global entities. Choosing ksh to design a suite will
lead to similar concepts and practices. Choosing the Python language can
lead to deeper changes.

Milestones are:

* the expanded definition file generation

* the expanded definition file load into ecFlow server

* running the (e)suite

* eventually, expanded definition files played into SMS server, and run
  as (e)suite

Language correspondence:

======================================= ====================================== ==============================
CDP                                     ksh                                    Python
======================================= ====================================== ==============================
if (math) then cmds; else cmds; endif   if ((math)); then cmds; else cmds; fi  if math: cmds; else: cmds
while (math) do cmds; endwhile          while ((math)); do cmds; done          while math: cmds
loop VAR (spaced_list) do cmds; endloop for VAR in spaced_list; do cmds; done  for VAR in list:

case VAR                                case $VAR in                           if VAR in [comma_list]:
in (spaced_list) do cmds; endin         piped_list) cmds;;
in ( ) do cmds; endin                   \*) cmds;;                             else:
endcase                                 esac
--------------------------------------- -------------------------------------- ------------------------------
for VAR math1 math2; do                 var=math1; while ((var < math2)); do   var=math1; while var<math2:
for VAR math1 math2 step math3; do      var=math1; while ((var < math2)); do   var=math1; while var<math2:
                                        var=$((var+math3))                                  var=var+math3
endfor                                  done

$ shell_cmd                             <native>                               popen2.popen2("cmd")
% cshell_cmd                            csh

< file_include                          . file_include                         import library

alias a cmds \; cmds2                   alias a="cmds; cmds2"                  def a: cmds; cmds2

set variable value                      variable=value                         variable="value"
variable=math                           variable=$((math))                     variable=math
variable=`shell_cmd`                    variable=$(shell_cmd)                  <use popen2>          

man                                     man / help                             help()

echo                                    echo / print                           print

# comment                               # comment                              # comment 

set debug on                            set -eux                               python -m trace -t
                                                                               pylint coverage
======================================= ====================================== ==============================
 
