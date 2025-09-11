.. _how_to_mirror_a_remote_task:

How to mirror a remote Task?
****************************

The following instructions describe the necessary steps to use the ability to
synchronise (i.e. :term:`mirror`) the status and attributes, such as variables,
meters, labels and events, of a :term:`task` between a `remote` ecFlow server
and a `local` ecFlow server. The `remote` server is responsible for actually
executing the task, while the `local` server synchronizes the status of a
particular task, in order to trigger the execution of dependent tasks.

The deployment of this feature has the following requirements:
 - `local` ecFlow 5.13+, to host the dependent task(s)
 - `remote` ecFlow 5+, to host the remote task
 - (optional) Authentication credentials, stored as JSON file

Setup the ecFlow Server
=======================

Deploy the `remote` ecFlow server, as per the :ref:`regular instructions<starting_the_ecflow_server>`
and specifying username/password access configuration.

Deploy the Authentication credentials so that the JSON file is accessible
to the `local` ecFlow server. Launch the server.

Consider using SSL encription for the communication between ecFlow server and
client.


Define the Suite with a `mirrored` Task
=======================================

On the :term:`suite` definition file, create a :term:`task` (which will mirror
the remote task status) and assign it a :term:`mirror` attribute. The mirror
attribute must specify the path of the remote node, and the remote server
connection details. Using the mirrored task, other tasks can use triggers to
execute when the remote task becomes complete.

Follow the recommended practice of defining ecFlow :term:`variables<variable>`
at :term:`suite` level:

 - ECF_MIRROR_REMOTE_HOST
 - ECF_MIRROR_REMOTE_PORT
 - ECF_MIRROR_REMOTE_POLLING
 - ECF_MIRROR_REMOTE_AUTH

**Example 1**: Example suite definition with mirror attributes

  .. code-block:: shell

    suite s
      edit ECF_MIRROR_REMOTE_HOST 'remotehost'
      edit ECF_MIRROR_REMOTE_PORT '3456'
      edit ECF_MIRROR_REMOTE_POLLING '120'
      edit ECF_MIRROR_REMOTE_AUTH '/path/to/mirror.auth'
      family f
        task Task
          edit YMD placeholder
          mirror --name A --remote_path /s1/f1/t1 --remote_host %ECF_MIRROR_REMOTE_HOST% --remote_port %ECF_MIRROR_REMOTE_PORT% --polling %ECF_MIRROR_REMOTE_POLLING% --ssl
        task Dependent
          trigger Task == complete and /s/f/Task:VARIABLE >= 20000101
      endfamily
    endsuite

  .. warning::

     Variables that are referred in trigger expressions *must* be defined, as a placeholder
     for variables that eventually get synchronised.

Deploy the Suite with a `mirrored` Task
=======================================

Load the suite definition containing the :term:`mirror` attribute.

Whenever the ecFlow server is in `RUNNING` state, the synchronisation between
`remote` and `local` servers is enabled automatically.
