.. _using_backup_servers:

Using backup servers
********************


The :code:`ecflow_client` can be configured to contact alternate backup servers in case the primary server is not available -- this typically most useful in Operational environments.

The use of backup servers is, by default, enabled only for :term:`Task commands <child command>`. This behaviour can be customized by setting the environment variable
:code:`ECF_HOSTFILE_POLICY`. This variable can take the following values:

- :code:`task` (default): backup servers are used only for Task commands.
- :code:`all`: backup servers are used for all commands, including Task and User commands.

The list of backup servers can be specified by defining the environment variable :code:`ECF_HOSTFILE`, indicating the location of a file, by convention located at :code:`$HOME/.ecf_hostfile`, with the following format:

.. code-block:: shell

   # This is a comment
   host1             # port 3141 is used by default, when not specified
   host2:port2
   host3:port3

To enable the :code:`ecflow_client` to read the file and use the listed backup servers, the environment variable :code:`ECF_HOSTFILE` must be set before running the :code:`ecflow_client` command:

.. code-block:: shell

  export ECF_HOSTFILE=$HOME/.ecf_hostfile

.. important::

    The maximum retry period is defined by :code:`ECF_TIMEOUT`, which by default is set to 24 hours.
    This means that the :code:`ecflow_client` will continue to loop over the list and retry primary host followed by alternate hosts for up to :code:`ECF_TIMEOUT`, before giving up and reporting a failure.

.. warning::

    When executing a command, the :code:`ecflow_client` will always first try to connect to the primary host, as defined by command line options or :code:`ECF_HOST`::code:`ECF_PORT`.
    If the first attemp to contact the primary host fails, the client will automatically retry contacting the primary server after waiting for a retry period of 10 seconds.
    Only after this second attempt has failed, will the :code:`ecflow_client` then immediatelly try to connect to the backup servers listed in the :code:`ECF_HOSTFILE`.

    This implies that the :code:`ecflow_client` will not try to connect to the backup servers immediately, and thus contacting the backup server incurs in a minimum 10 seconds delay.
