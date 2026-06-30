.. _text_based_def_mirror:

mirror
//////

This defines a :term:`mirror` attribute, which synchronizes the status of a
:term:`node` between a local and a remote ecFlow server. The options defining
the attribute can be provided in any order.

The :term:`mirror` attribute is defined by the coordinates to the remote :term:`node` to be mirrored, including the
remote :term:`node` path, the remote host name, the remote port number; and the polling interval in seconds.
Furthermore, the option :code:`--ssl` enables the use of a Secure connection and the option :code:`--auth` can be used
to provide the authentication credentials for the remote ecFlow server.

.. code-block:: shell

    task t1
      mirror --name A --remote_path /s/f/tA --remote_host host --remote_port 3141 --polling 300 --ssl --auth /path/to/auth.json

    task t2
      aviso --name B --remote_path /s/f/tB
        # when not provided, the following default options are used
        #     --remote_host %ECF_MIRROR_REMOTE_HOST%
        #     --remote_port %ECF_MIRROR_REMOTE_PORT%
        #     --polling %ECF_MIRROR_POLLING%
        #     --auth %ECF_MIRROR_AUTH%


By default, the :term:`mirror` attribute will not propagate the state of the remote :term:`node` to the local node tree,
and only the actual associated local :term:`node` will see its state updated.
The option :code:`--propagate` can be used to enable the propagation of the remote :term:`node` state to the local node
tree.

.. code-block:: shell

    task t1
      mirror --name A --remote_path /s/f/tA --remote_host host --remote_port 3141 --polling 300 --propagate


.. note::
   The ``--polling`` value (in seconds) controls how often the local ecFlow
   server contacts the remote server. Lower values reduce latency but
   have impact on the remote server as the network traffic increases.
   A value of 300 seconds is considered suitable for most cases, and a minimum
   of 60 seconds being imposed to avoid overloading the remote server.

The Authentication credentials, provided via option :code:`--auth`, are
provided in a `JSON` file with the following content:

.. code-block:: json

    {
      "url": "http://host.int:1234",
      "username": "user",
      "password": "************",
      "email": "user@host.int"
    }
