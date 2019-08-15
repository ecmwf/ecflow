#!/bin/sh

ECF_OUT=ecflow_1524.txt
server_dir=/usr/local/apps/ecflow/4.12.0/bin
client_dir=/usr/local/apps/ecflow/4.12.0/bin
#server_dir=../Server/bin/gcc-6.3.0/release
#client_dir=../Client/bin/gcc-6.3.0/release/ 

loop=1
for (( ; ; )) ; do
   echo "attempt number $loop `date`"
   $server_dir/ecflow_server --port 4040 > $ECF_OUT 2>&1 < /dev/null &
   
   # wait for server to start
   set +e # ignore error 
   count=0
   while [ 1 ] ; do   
     $client_dir/ecflow_client --port 4040 --ping  > /dev/null 2>&1
     if [[ $? == 0 ]] ; then
        #echo "  server up and running"
        break;
     fi
     sleep 1
     count=$((count + 1))
     #echo $count
     if [ "$count" -gt "60" ] ; then
        echo "  Timed out after 60 seconds"
        exit 1
     fi
   done
   set -e  # re-enable error 
   
   
   $client_dir/ecflow_client --port 4040 --restart
   $client_dir/ecflow_client --port 4040 --ping  > /dev/null 2>&1 
   $client_dir/ecflow_client --port 4040 --terminate=yes
   
   set +e # ignore error 
   grep "Year is out of valid range" polonius.4040.ecf.log
   if [[ $? == 0 ]] ; then
      echo "Found error in log file"
      exit 1
   fi
   rm -rf polonius.4040.ecf.log
   rm -rf polonius.4040.ecf.check
   rm -rf polonius.4040.ecf.check.b
   rm -rf $ECF_OUT
   loop=$((loop + 1))
done



