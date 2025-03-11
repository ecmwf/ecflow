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

.. note::

   The `listener` parameter is expected to be a valid single line JSON string, enclosed in single quotes.

The listener must define two fields (as per the `Aviso Listerner <https://pyaviso.readthedocs.io/en/latest/guide/define_my_listener.html>`_ definition):

 - :code:`event`, specifies the type of Aviso event
 - :code:`request`, specifies a dictionary with the parameters used to check for matches of Aviso notifications

The following are some examples:

.. code-block:: shell

    '{ "event": "dissemination", "request": { "destination": "abc" } }'

    '{ "event": "mars", "request": { "class": "od", "expver": "0001", "domain": "g", "stream": "abcd", "step": 0 } }'

The Authentication credentials, provided via option :code:`--auth`, are
provided in a `JSON` file with the content following the
`ECMWF Web API <https://www.ecmwf.int/en/computing/software/ecmwf-web-api>`_
(this is conventionally stored in a file located at `$HOME/.ecmwfapirc`):

.. code-block:: json

    {
      "url" : "https://api.ecmwf.int/v1",
      "key" : "<your-api-key>",
      "email" : "<your-email>"
    }
