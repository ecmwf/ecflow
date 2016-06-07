#!/bin/sh

set +e # ignore error 
count=0
while [ 1 ] ; do   
    ecflow_client --ping 2> /dev/null
    if [[ $? == 0 ]] ; then
        echo "server up and running"
        break;
    else
        sleep 3
    fi
    count=$((count + 1))
    #echo $count
    if [ "$count" -gt "2" ] ; then
        echo "Timed out after 60 seconds"
        exit 1
    fi
done

 