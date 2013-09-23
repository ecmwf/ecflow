#!/bin/ksh

if [ %ECF_PORT:0% -gt 0 ] ; then
  for task in $missing; do
    ecflow_client --force complete recursive /%SUITE%/%FAMILY%/tc$task
  done
else

cdp << EOF
define ERROR {
  if(rc==0) then exit 1; endif
}

set SMS_PROG %SMS_PROG%
login %SMSNODE% %USER% 1 ; ERROR
suites -s %SUITE%
loop task ( $missing ) do
  force -r complete /%SUITE%/%FAMILY%/tc\$task ; ERROR
endloop
exit
EOF
fi
