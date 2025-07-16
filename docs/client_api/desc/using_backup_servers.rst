.. _using_backup_servers:

Using backup servers
////////////////////


The :code:`ecflow_client`, when running :term:`Task commands <child command>`, can be configured to contact
alternate backup servers in case the primary server is not available -- this typically most useful in Operational environments.

.. important::

    The use of backup servers is only supported for :term:`Task commands <child command>`, and not for User commands such as :code:`ecflow_client --ping` or :code:`ecflow_client --alter`.

A list of backup servers can be defined in a file, by convention located at :code:`$HOME/.ecf_hostfile`, with the following format:

.. code-block:: shell

   # This is a comment
   host1             # port 3141 is used by default, when not specified
   host2:port2
   host3:port3


To enable the :code:`ecflow_client` to read this file, and use the servers listed in it as backup servers, the following environment variable must be set before running the :code:`ecflow_client` command:

.. code-block:: shell

  export ECF_HOSTFILE=$HOME/.ecf_hostfile

.. warning::

    When performing a Task command, the :code:`ecflow_client` it will always first try to connect to the primary server, as defined by command line options or :code:`ECF_HOST`::code:`ECF_PORT`.
    If the first attemp to contact the primary server fails, the client will automatically retry contacting the primary server after waiting for a retry period of 10 seconds.
    Only after this second attempt has failed, will the :code:`ecflow_client` then immediatelly try to connect to the backup servers listed in the :code:`ECF_HOSTFILE`.

    This implies that the :code:`ecflow_client` will not try to connect to the backup servers immediately, and thus contacting the backup server incurs in a minimum 10 seconds delay.
