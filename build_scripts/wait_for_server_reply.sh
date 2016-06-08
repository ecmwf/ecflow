#!/bin/sh

# wait for server to start
set +e # ignore error 
count=0
while [ 1 ] ; do   
    sleep 3
    ecflow_client --ping 2> /dev/null
    if [[ $? == 0 ]] ; then
        echo "server up and running"
        break;
    fi
    count=$((count + 1))
    #echo $count
    if [ "$count" -gt "2" ] ; then
        echo "Timed out after 60 seconds"
        exit 1
    fi
done
set -e  # re-enable error 

 