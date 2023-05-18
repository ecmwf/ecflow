.. _udp_api:

ecFlow UDP
//////////////////////

.. caution:: 
  ecFlow's UDP server is experimental, actively under implementation, and its details are subject to change.
  The documentation reflects the current implementation status.

Compilation
-----------

ecFlow UDP API provides a lightweight (and low level) proxy to interact with ecFlow server.
The build of ecFlow UDP server can be enabled (ON) or disabled (OFF) using cmake option:

.. code::

    -DENABLE_UDP=ON

When the ecFlow UDP server is enabled, the build produces the following additional executables:

- **ecflow_udp**, the actual ecFlow UDP server
- **ecflow_udp_client**, an ecFlow UDP client capable of sending a text payload to the UDP server

When launched, the server will by default listen to port 8080. This can be changed with command line option ``-p``,
``--port``. The option ``--verbose`` can be used to enable logging output.

.. Important::

    The communication between the ecFlow UDP server and the ecFlow server is done in clear text. At the moment,
    no encryption is applied to the exchanged information.

Starting ecFlow UDP server
--------------------------

The ecFlow UDP server is started with the following command:

.. code-block:: shell

  ecflow_udp --verbose [--port <ecflow-udp-port>]

By default the ecFlow UDP server expects ecFlow server to be available at localhost:3141. This can be changed by
customizing the environment variables ``ECF_HOST`` and ``ECF_PORT`` before starting the ecFlow UDP server, or by
using the CLI options ``--ecflow_host`` and ``--ecflow_port`` (n.b. the CLI options override the environment variables).

Command Line Options and Environment Variables
----------------------------------------------

ecFlow UDP options can be controlled either with command line options or with environment variables.
All environment variables with "UDP" are new.


.. list-table::
   :header-rows: 1

   * - Command line option
     - Environment variable
     - Default Value
     - Description
   * - --ecflow_host
     - ECF_HOST
     - localhost
     - ecFlow server hostname
   * - --ecflow_port
     - ECF_PORT
     - 3141
     - ecFlow server port
   * - --port,-p
     - ECF_UDP_PORT
     - 8080
     - ecFlow UDP port
   * - --verbose
     - ECF_UDP_VERBOSE
     - false
     - Enable verbose mode
   * - --version,-v
     -
     - false
     - Display version information

Authentication
--------------

ecFlow UDP forwards basic authentication information, as part of the request, to the ecFlow server
where the authentication takes place considering the available mechanisms (e.g while lists files,
password based authentication).


ecFlow UDP Requests
-------------------

The current implementation of ecFlow UDP allows the following task requests:

- update the value of a meter
- update the value of a label
- set/clear an event

Each of the operations can be triggered be sending a JSON-formatted request to ecFlow UDP server.
The following sections present the description of the request format.

Update meter value
~~~~~~~~~~~~~~~~~~

.. code:: json

    {
        "method": "put",
        "version: "1",
        "header": {
            "task_rid": "<task-remote-identifier>",
            "task_password": "<task-password>"},
            "task_try_no": "<task-try-number>"}
        },
        "payload": {
            "command": "meter",
            "path": "</path/to/task>",
            "name": "<meter-name>",
            "value": "<new-meter-value>"
        }
    }

Update label value
~~~~~~~~~~~~~~~~~~

.. code:: json

    {
        "method": "put",
        "version: "1",
        "header": {
            "task_rid": "<task-remote-identifier>",
            "task_password": "<task-password>"},
            "task_try_no": "<task-try-number>"}
        },
        "payload": {
            "command": "label",
            "path": "</path/to/task>",
            "name": "<label-name>",
            "value": "<new-label-value>"
        }
    }

Set/clear event value
~~~~~~~~~~~~~~~~~~~~~

.. code:: json

    {
        "method": "put",
        "version: "1",
        "header": {
            "task_rid": "<task-remote-identifier>",
            "task_password": "<task-password>"},
            "task_try_no": "<task-try-number>"}
        },
        "payload": {
            "command": "event",
            "path": "</path/to/task>",
            "name": "<event-name>",
            "value": "<new-event-value>" // the value is either 1 to set or 0 to clear the event
        }
    }
