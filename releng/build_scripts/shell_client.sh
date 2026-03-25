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
    path_to_task_size=${#path_to_task}
    password=$2
    password_size=${#password}
    process_id=$3
    process_id_size=${#process_id}
    try_no=$4
    meter_name=$5
    meter_name_size=${#meter_name}
    meter_value=$6

    data="$boost_preamble $meter_cmd $path_to_task_size $path_to_task $password_size $password $process_id_size $process_id $try_no $meter_name_size $meter_name $meter_value"
    header=$(printf '%8.8s' $(echo "obase=16; ${#data}" | bc) )

    payload=$header$data
    return "$payload"
}

path_to_task=${ARG_PATH:-"/example/path/to/task"}
password=${ARG_PASSWORD:-"password"}
process_id=${ARG_PID:-"12345"}
try_no=${ARG_TRYNO:-"1"}
meter_name=${ARG_METER_NAME:-"METER"}
meter_value=${ARG_METER_NAME:-"10"}

payload=$(meter_payload $path_to_task $password $process_id $try_no $meter_name $meter_value)
echo "$payload"
