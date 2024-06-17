.. _text_based_def_mirror:

mirror
//////

This defines a :term:`mirror` attribute, which synchronizes the status of a
:term:`node` between a local and a remote ecFlow server. The options defining
the attribute can be provided in any order.

.. code-block:: shell

    task t1
      mirror --name A --remote_path /s/f/tA --remote_host host --remote_port 3141 --polling 20 --ssl --auth /path/to/auth.json

    task t2
      aviso --name B --remote_path /s/f/tB
        # when not provided, the following default options are used
        #     --remote_host %ECF_MIRROR_REMOTE_HOST%
        #     --remote_port %ECF_MIRROR_REMOTE_PORT%
        #     --polling %ECF_MIRROR_POLLING%
        #     --auth %ECF_MIRROR_AUTH%

The Authentication credentials, provided via option :code:`--auth`, are
provided in a `JSON` file with the following content:

.. code-block:: json

    {
      "url": "http://host.int:1234",
      "username": "user",
      "password": "************",
      "email": "user@host.int"
    }
