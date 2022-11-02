.. _ksh_task_wrappers_and_functions_trapping:

ksh task wrappers and functions trapping
//////////////////////////////////////////

On Linux architecture, be aware of the slightly different behaviour of
ksh functions according to their definition: With the following
syntax, the function gives hand back to the calling script,
so that we define the trapping to simply "exit 1" in case of early
exit, in case of error, or when receiving an external signal ; then
the calling script traps this return code and calls ERROR (ecflow_client --abort)

.. code-block:: shell

  ERROR() {
    trap 0
    ecflow_client --abort=$$
    exit 1
  }
  trap ERROR 0 $SIGNAL_LIST
  # ...
  function call {
    trap '{ echo "Error in function"; exit 1; }' 0 $SIGNAL_LIST
    set -ex
    # ...
    trap 0; return 0 ##### reset trap
  }
  # ...
  call
  trap 0; ecflow_client --complete ; exit 0


In the next example, exit 1 will directly exit the script, so that we
have to call ecflow_client to report the abort to the server.

.. code-block:: shell

  call() {
    trap '{ ecflow_client --abort=$$; exit 1; }' 0 $SIGNAL_LIST
    set -ex
  }

In a similar way, in ``if`` or ``while`` expression, ``[[ ...]]`` or ``((...))``
shall be preferred to ``[...]`` to get the trapping propagated as expected.
