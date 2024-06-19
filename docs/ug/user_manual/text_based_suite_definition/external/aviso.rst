.. _text_based_def_aviso:

aviso
/////

This defines an :term:`aviso` attribute, and thus a node dependency on an Aviso
notification. The options defining the attribute can be provided in any order.

.. code-block:: shell

    task t1
      aviso --name A --listener '{ ... }' --url http://aviso.ecmwf.int/ --schema /path/to/schema.json --polling 300 --auth /path/to/auth.json

    task t2
      aviso --name B --listener '{ ... }'
        # when not provided, the following default options are used
        #     --url %ECF_AVISO_URL%
        #     --schema %ECF_AVISO_SCHEMA%
        #     --auth %ECF_AVISO_AUTH%
        #     --polling %ECF_AVISO_POLLING%

Notice that the :code:`--listener` option must be surrounded by single quotes,
and is composed as a single line `JSON`. The `JSON` must define two fields:

 - :code:`event`, specifies the type of Aviso event
 - :code:`request`, specifies a dictionary with the parameters used to check for matches of Aviso notifications

The following are some examples:

.. code-block:: shell

    '{ "event": "dissemination", "request": { "destination": "abc" } }'

    '{ "event": "mars", "request": { "class": "od", "expver": "0001", "domain": "g", "stream": "abcd", "step": 0 } }'

The Authentication credentials, provided via option :code:`--auth`, are
provided in a `JSON` file with the following content:

.. code-block:: json

    {
      "url": "http://host.int:1234",
      "key": "*******************************************************************",
      "email": "user@host.int"
    }
