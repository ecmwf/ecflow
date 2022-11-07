.. _autocancel:

autocancel
//////////

Autocancel is a way to automatically delete a node that has completed.
The deletion may be delayed by an amount of time in hours and minutes or
expression in days. This will help the maintenance of **living suites**. Notice that, if the suite is **repeated** and part of it is cancelled, that part will obviously not be run.

The node deletion is never immediate. The nodes are checked once a
minute (by default) and expired autocancel-nodes are deleted.

Any node can have autocancel statement like:

.. code-block:: shell

    autocancel +00:10 # Cancel 10 minutes later
    autocancel 0      # Cancel immediately
    autocancel 3      # Cancel three days later

The effect of **autocancel** is the same as if the user would use:

.. code-block:: shell

    ecflow_client –-delete=/path/to/node/with/autocancel               

This means the deleted nodes if used to trigger other nodes, may leave a
node to wait for the (now missing) node. To solve this problem use a
trigger like:

.. code-block:: shell

    task t
    trigger node_name==complete or node_name==unknown   
    
It is best not to use autocancelled nodes in the triggers.

Using autocancel
====================

Sometimes you may want to have a suite in which you incrementally add
things and once these parts have served their purpose you want to
dispose of them.

**autocancel** is a way of automatically removing these families. Nodes
with this property defined will be automatically removed by ecFlow once
they become complete and the time defined has elapsed.

.. code-block:: shell

    suite x
    family fam
        autocancel +05:00
        task t
    endfamily
    endsuite


In this example family **fam** will be removed from the suite once it 
has been complete for more than five hours.                           
                                                                    
This is equivalent to the user issuing the CLI command                

.. code-block:: shell

    ecflow_client –-delete=/x/fam                                      

This means that if there are other tasks dependent on **fam** or its
children their triggers may never allow them to run. To guard against
such situations you can use triggers that allow other nodes to disappear
or that not been defined at all. This is done by using the status value
unknown for undefined nodes.

.. code-block:: shell

    suite x
    family fam
        autocancel +05:00
        task t
    endfamily
    family ff
        trigger fam==complete or fam==unknown
