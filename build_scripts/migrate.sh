

prepare_migration() {
   use ecflow
   while read nick host port
   do
     LOG="--host $host --port $port"
     ecflow_client $LOG --ping || continue
     version=$(ecflow_client $LOG --server_version)
     echo $nick $host $port $version
     /usr/local/apps/ecflow/$version/bin/ecflow_client $LOG --migrate > $host.$port.mig
   done < /usr/local/apps/ecflow/current/lib/servers
}

prepare_migration
