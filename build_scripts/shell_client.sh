#!/bin/sh

function meter_payload() {
   # $1 path_to_task
   # $2 password
   # $3 process_id
   # $4 try no
   # $5 meter_name
   # $6 meter value
   
    boost_preamble="22 serialization::archive 10 0 0 0 1 2"
    meter_cmd="8 MeterCmd 1 0 0 0 0 0 0"
    
    path_to_task=$1
    password=$2
    process_id=$3
    try_no=$4
    meter_name=$5
    meter_value=$6
    
    path_to_task_size=$(echo ${#path_to_task}) 
    password_size=$(expr length  $password  )  
    process_id_size=$(expr length  $process_id )  
    meter_name_size=$(expr length  $meter_name )  
    
    data="$boost_preamble $meter_cmd $path_to_task_size $path_to_task $password_size $password $process_id_size $process_id $try_no $meter_name_size $meter_name $meter_value"
    header=$(printf '%8.8s' $(echo "obase=16; ${#data}" | bc) )
     
    payload=$header$data
    return "$payload"
}

path_to_task="/all_trigger_examples/a"
password="F5Q0eiJu"
process_id="24053"
try_no="1"
meter_name=METER
meter_value="10"

payload=$(meter_payload $path_to_task $password $process_id $try_no $meter_name $meter_value)
echo "$payload"
