.. _how_to_trigger_a_task_based_on_aviso_notification:

How to trigger a Task based on Aviso notification?
--------------------------------------------------

The following instructions describe the necessary steps to use the ability to
trigger a :term:`task` based on an Aviso notification. The intended task must
be assigned an :term:`aviso` attribute, which provides the configuration to
periodically poll the Aviso server, and execution only happens when the
a matching Aviso notification is processed.

The deployment of this feature has the following requirements:
 - ecFlow 5.13+, to host the Aviso dependent task(s)
 - Aviso event listener schema
 - (optional) Authentication credentials, stored as JSON file

Setup the ecFlow Server
^^^^^^^^^^^^^^^^^^^^^^^

Deploy the Authentication credentials and Aviso event listener schema files so
that these files are accessible to the ecFlow server. Launch the server, as per
the :ref:`regular instructions<starting_the_ecflow_server>`.

Define a Suite with an `Aviso` dependent Task
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

On the :term:`suite` definition file, create a :term:`task` and assign it an
:term:`aviso` attribute.

The :term:`aviso` attribute must specify the base URL to contact the Aviso server,
and the remote server connection details. Using the mirrored task, other tasks can use triggers to execute
when the remote task becomes complete.

Follow the recommended practice of defining ecFlow :term:`variables<variable>` at :term:`suite` level:

 - ECF_AVISO_URL
 - ECF_AVISO_SCHEMA
 - ECF_AVISO_POLLING
 - ECF_AVISO_AUTH

**Example 1**: Example suite definition with mirror attributes

  .. code-block:: shell

    suite s
      edit ECF_AVISO_URL 'http://aviso:2379'
      edit ECF_AVISO_SCHEMA '/path/to/event_listener_schema.json'
      edit ECF_AVISO_POLLING '120'
      edit ECF_AVISO_AUTH '/path/to/aviso.auth'
      family f
        task Task
          aviso --name A --listener '{ "event": "mars", "request": { "step": 0 } }' --url %ECF_AVISO_URL% --schema %ECF_AVISO_SCHEMA% --auth %ECF_AVISO_AUTH% --polling %ECF_AVISO_POLLING%
      endfamily
    endsuite

To enable Aviso authentication, set the `--auth` option (directly or via the `ECF_AVISO_AUTH`
variable) to the path of the JSON file containing the authentication credentials, which follows the
`ECMWF Web API <https://www.ecmwf.int/en/computing/software/ecmwf-web-api>`_. The credentials file
is conventionally located at `$HOME/.ecmwfapirc`, with the following content:

.. code-block:: json

   {
     "url" : "https://api.ecmwf.int/v1",
     "key" : "<your-api-key>",
     "email" : "<your-email>"
   }

Define a Suite with an `Aviso` dependent Task
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Load the suite definition containing the :term:`aviso` attribute.

Whenever a task assigned with an :term:`aviso` attribute is (re)queued,
the Aviso will start to be periodically poll. The task is held from execution
until a notification matching the provided listener is found.
Once the task is allowed to execute the periodic polling is stopped, until the
task is requeued again.
