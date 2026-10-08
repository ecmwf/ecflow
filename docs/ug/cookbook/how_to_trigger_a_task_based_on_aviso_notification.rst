.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _how_to_trigger_a_task_based_on_aviso_notification:

How to trigger a Task based on Aviso notification?
**************************************************

The following instructions describe the steps to release a :term:`task` whenever a matching notification is
published by an Aviso server. The task is assigned an :term:`aviso` attribute, which holds the task queued
until a notification matching its listener is received; each notification releases the task once. The
attribute is described in detail in :ref:`text_based_def_aviso`.

The deployment of this feature has the following requirements:

 - ecFlow 5.20.0+, built with Aviso support (see :ref:`build_from_source`), to host the Aviso dependent
   task(s);
 - an Aviso v2 server (aviso-server), reachable from the host of the ecFlow server;
 - a credentials file, accessible to the ecFlow server.

.. note::

   ecFlow 5.20.0 supports Aviso v2 only; ecFlow 5.19.x is the last release that supports Aviso v1.

Set up the ecFlow Server
========================

Deploy the credentials file, so that the file is accessible to the user running the ecFlow server; an ECMWF
API credentials file (:code:`$HOME/.ecmwfapirc`) can be used as it is. The formats of the file are
described in :ref:`text_based_def_aviso_credentials`.

Launch the server, as per the :ref:`regular instructions<starting_the_ecflow_server>`. A server built with
Aviso support logs the version of the Aviso client library when it starts.

Define a Suite with an `Aviso` dependent Task
=============================================

In the :term:`suite` definition file, create a :term:`task` and assign it an :term:`aviso` attribute. Follow
the recommended practice of defining the following ecFlow :term:`variables<variable>` at :term:`suite` level,
which provide the default values of the attribute options :code:`--url` and :code:`--auth`:

 - :code:`ECF_AVISO_URL`, the address of the Aviso server;
 - :code:`ECF_AVISO_AUTH`, the path to the credentials file.

**Example**: Suite definition with an aviso attribute

  .. code-block:: shell

    suite s
      edit ECF_AVISO_URL 'https://aviso.ecmwf.int'
      edit ECF_AVISO_AUTH '/path/to/aviso.json'
      family f
        task process
          aviso --name A --listener '{ "event": "mars", "request": { "class": "od", "stream": "enfo", "step": [0, 6, 12] } }'
      endfamily
    endsuite

The task script accesses the notification that released the task through the generated variables described
in :ref:`text_based_def_aviso_variables`:

  .. code-block:: shell

    payload='%ECF_AVISO_EVENT_DATA_PAYLOAD%'
    echo "Processing notification %ECF_AVISO_EVENT_SEQUENCE%: $payload"

Load and begin the Suite
========================

Load the suite definition containing the :term:`aviso` attribute, and begin the suite.

Whenever a task assigned an :term:`aviso` attribute is (re)queued, the attribute starts watching the Aviso
server. The task is held from execution until a notification matching the listener is received. When the
task completes, the watch stops; when the task is requeued, the watch resumes after the last notification
that released the task, so that the notifications published in the meantime release the task in turn.

When the attribute cannot watch the Aviso server (for example, because the credentials are not accepted),
the task remains queued with the flag :code:`remote_error`, and the reason is shown by the attribute; see
:ref:`text_based_def_aviso_errors`.

Use case: a local Aviso server
==============================

The following script runs the complete use case on a single machine: an aviso-server, running in a
container, an ecflow_server, and a suite whose task prints the notification that released it. The script
publishes three notifications, of which two match the listener, and shows the two releases of the task.

The requirements are an ecFlow build with Aviso support, Docker (or a compatible container engine providing
the command :code:`docker`), access to the aviso-server container image, and :code:`curl`.

.. code-block:: shell

   ./aviso_use_case.sh /path/to/ecflow/bin

The script uses the following aviso-server configuration (an in-memory backend, no authentication, and a
single event type):

.. literalinclude:: src/aviso_server_config.yaml
   :language: yaml
   :lines: 4-

The script itself:

.. literalinclude:: src/aviso_use_case.sh
   :language: bash
   :lines: 4-
