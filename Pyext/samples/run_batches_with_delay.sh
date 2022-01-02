#!/bin/sh
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
# Name        :
# Author      : Avi
# Revision    : $Revision: #10 $
#
# Copyright 2009- ECMWF.
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8

# Example of running a shell command from the GUI.
# Use the following in ecflowUI:
# sh $WK/Pyext/samples/run_batches_with_delay.sh -h %ECF_HOST% -p %ECF_PORT% -b 10 -s 2 -n '<full_name>'
#    - sh at the front is required
#    - the tick quotes on <full_name> are required
#

echo "test.sh $*"
#exit 0

function usage() {
   echo "Usage: -h <host> -p <port> -b <batch_size> -s <sleep_between_batch> -n <node paths>"
   echo "       -h        ECF_HOST"
   echo "       -p        ECF_PORT"
   echo "       -b        batch size. the number of job to be submitted in parallel"
   echo "       -s        number of seconds to sleep between each batch"
   echo "       -n        list of node paths."
}

host=
port=
batch_size=
sleep_between_batch=

while getopts ":h:p:b:s:n:" opt
   do
     case "${opt}" in
        h ) host=$OPTARG;;
        p ) port=$OPTARG;;
        b ) batch_size=$OPTARG;;
	    s ) sleep_between_batch=$OPTARG;;
        n ) node_paths+=("$OPTARG");;
     esac
done

echo "host:$host"
echo "port:$port"
echo "batch_size=$batch_size"
echo "sleep_between_batch=$sleep_between_batch"
echo "paths='${node_paths[@]}'"
 
for path in $node_paths; do
   echo $path
done   

python3 /var/tmp/ma0/workspace/ecflow/Pyext/samples/run_batches_with_delay.py --host $host --port $port --batch_size $batch_size --sleep_between_batch $sleep_between_batch --paths "$node_paths"
