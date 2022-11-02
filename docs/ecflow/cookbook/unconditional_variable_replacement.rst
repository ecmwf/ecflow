.. _unconditional_variable_replacement:

Unconditional variable replacement
//////////////////////////////////////////////////////////////

The utility ~emos/bin/ls.py provides a way to dump server content below a given node. It can be used to replace a variable throughout all suites on a server:

.. code-block:: shell

    LS=~emos/bin/ls.py
    string=/vol/$USER/output
    new=/vol/${USER}_output
    
    PORT=$(($(id -n)+ 1500))
    HOST=localhost
    
    client="ecflow_client --port=$PORT --host=$HOST"
    # list all suites
    suites="$($client --suites)"
    
    # list all variables occurrence with value to change
    for s in $suites; do ECF_HOST=$HOST ECF_PORT=$PORT $LS -NRVv /$s ; done | grep $string | grep -E -v "(ECF_SCRIPT|ECF_JOB)" > list.tmp
    
    # replace
    while read -r path val;  do
        p=$(echo $path | cut -d: -f1);
        name=$(echo $path | cut -d: -f2);
        $client --alter=change variable $name $new $p;
    done < list.tmp
