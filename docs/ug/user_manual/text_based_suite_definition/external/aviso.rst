.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _text_based_def_aviso:

aviso
/////

This defines an :term:`aviso` attribute, and thus a :term:`task` dependency on notifications published by an
`Aviso <https://github.com/ecmwf/aviso-server>`_ server. The task is held queued until a notification
matching the attribute is received, and each notification releases the task once. The options defining
the attribute can be provided in any order.

.. important::

   ecFlow 5.20.0 supports Aviso v2 only, and requires the following minimum versions:

    - **aviso-client 2.4.2**, the Aviso v2 client library (:code:`libaviso_ffi`) with which the ecFlow server
      is built (see :ref:`build_from_source`);
    - **aviso-server 0.13.0**, the Aviso v2 server contacted by the attribute.

   ecFlow 5.19.x is the last release that supports Aviso v1 (the etcd-based server). The Aviso v1 options
   :code:`--schema` and :code:`--polling` are rejected when a :term:`suite definition` is loaded, and are
   ignored when a :term:`check point` written by an earlier release is loaded.

.. code-block:: shell

    task t1
      aviso --name A --listener '{ "event": "mars", "request": { "class": "od", "step": [0, 6] } }' --url https://aviso.ecmwf.int --auth /path/to/aviso.json

    task t2
      aviso --name B --listener '{ "event": "dissemination", "request": { "destination": "ABC" } }'
        # when not provided, the following default options are used
        #     --url %ECF_AVISO_URL%
        #     --auth %ECF_AVISO_AUTH%

    task t3
      aviso --name C --listener '{ "event": "mars", "request": { "class": "od" } }' --collapse

The options are the following:

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Option
     - Description
   * - :code:`--name`
     - The name of the attribute (mandatory).
   * - :code:`--listener`
     - The selection of notifications, as a single-quoted JSON object (mandatory); see
       :ref:`text_based_def_aviso_listener`.
   * - :code:`--url`
     - The address of the Aviso server (e.g. :code:`https://aviso.ecmwf.int`). Default: :code:`%ECF_AVISO_URL%`.
   * - :code:`--auth`
     - The path to the credentials file; see :ref:`text_based_def_aviso_credentials`.
       Default: :code:`%ECF_AVISO_AUTH%`.
   * - :code:`--collapse`
     - A release consumes all the notifications received, instead of exactly one; see
       :ref:`text_based_def_aviso_release`.

The values of :code:`--listener`, :code:`--url` and :code:`--auth` can be composed of
:term:`variables <variable>`, which are resolved when the node is queued. The variables
:code:`ECF_AVISO_URL` and :code:`ECF_AVISO_AUTH` are not provided by the server, and are typically defined
once at :term:`suite` level.

.. important::

   An :term:`aviso` attribute is only allowed on a :term:`task` (or an alias of a task), and only one per
   task. An :term:`aviso` attribute on a :term:`suite` or a :term:`family` is rejected: a
   :term:`suite definition` or a :term:`check point` holding one fails to load, and the Alter command and the
   Python API refuse to add one.

.. _text_based_def_aviso_listener:

Listener
========

The listener is a JSON object, given on a single line and enclosed in single quotes, with the following
fields:

 - :code:`event`, the mandatory event type of the notifications (e.g. :code:`mars`, :code:`dissemination`),
   as configured on the Aviso server;
 - :code:`request`, an optional object selecting the notifications of that event type. Each entry
   constrains one identifier of the notification: a single value requires an exact match, and an array
   of values requires a match with any of them. An array is only accepted for an identifier whose type,
   in the schema of the Aviso server, supports it (e.g. an integer or an enumeration, but not a free
   string); otherwise, the Aviso server refuses the request.

.. code-block:: shell

    '{ "event": "dissemination", "request": { "destination": "ABC" } }'

    '{ "event": "mars", "request": { "class": "od", "expver": "0001", "domain": "g", "stream": "enfo", "step": [0, 6, 12, 18] } }'

The identifiers and their valid values are defined by the schema configured on the Aviso server, which
validates each request: a request naming an identifier unknown to the server is reported as an error on the
node (see :ref:`text_based_def_aviso_errors`).

.. _text_based_def_aviso_credentials:

Credentials
===========

The credentials file, given by option :code:`--auth`, is a JSON file holding either an email and a key, in
the format of the ECMWF API credentials file (conventionally :code:`$HOME/.ecmwfapirc`), or a user name and
a password. The key is sent as a bearer token; the email must be present, but its value is not used, and
any other field (such as :code:`url`) is ignored:

.. code-block:: json

    {
      "url": "https://api.ecmwf.int/v1",
      "key": "<key>",
      "email": "<email>"
    }

The user name and the password are sent as HTTP Basic authentication:

.. code-block:: json

    {
      "username": "<user>",
      "password": "<password>"
    }

When both are present, the key is used. The file is read by the ecFlow server, whenever the attribute
contacts the Aviso server, and must therefore be accessible to the user running the server.

.. important::

   The credentials file is mandatory.

   A relative path is resolved against the :code:`ECF_HOME` of the ecFlow server, i.e. the directory in which
   the server runs (and not against an :code:`ECF_HOME` variable defined in the suite): :code:`--auth aviso.json`
   refers to the file :code:`$ECF_HOME/aviso.json` of the server. The character :code:`~` is not expanded; to
   use the ECMWF API credentials file of the user running the server, give its absolute path (e.g.
   :code:`/home/user/.ecmwfapirc`).

.. _text_based_def_aviso_release:

Release of the node
===================

When the node is queued (e.g. when the :term:`suite` begins, or the node is requeued), the attribute starts
watching the Aviso server for notifications matching its listener. The watch stops when the node completes,
aborts or becomes unknown, and when the server is halted; it resumes when the server is restarted.

Each notification carries a sequence number, increasing within its event type. The attribute keeps the
sequence of the last notification that released the node as its revision, and the next watch resumes after
it: the Aviso server delivers again every matching notification published since, as long as its history
retains them. Before the first release, the revision is 0, and only the notifications published after the
attribute started watching (i.e. since the node was last queued, the server was last restarted, or the
attribute was last reloaded) are considered.

By default, each notification releases the node once, in order: when several notifications are received
while the node is queued, the oldest releases the node, and the others release it again on the following
requeues, one at a time. With the option :code:`--collapse`, a release consumes all the notifications
received, and the node is released once, by the latest one.

The revision, and the notification that released the node, are kept in the :term:`check point` file, so
that a server restarted from a check point continues with the notifications not yet consumed.

.. note::

   The delivery is at least once. As any other state of the node (e.g. its :term:`events <event>` and
   :term:`meters <meter>`), the revision reaches the disk only when the next :term:`check point` is written,
   by default every 120 seconds (see :code:`ECF_CHECKINTERVAL` and :code:`ECF_CHECKMODE`). If the server stops
   abnormally (e.g. a crash, or a forced reboot) after a release and before that check point, or if a backup
   server takes over from an earlier copy of the check point, the server resumes with the earlier revision,
   and the task is released again by the same notification.

   Tasks released by Aviso notifications should therefore tolerate processing the same notification twice;
   for example, a task can record the last :code:`ECF_AVISO_EVENT_SEQUENCE` it processed, and skip a
   notification with the same sequence.

.. _text_based_def_aviso_variables:

Generated variables
===================

A node with an :term:`aviso` attribute defines the following generated variables,
describing the notification that released the node (with :code:`--collapse`, the latest one):

.. list-table::
   :header-rows: 1
   :widths: 40 60

   * - Variable
     - Description
   * - :code:`ECF_AVISO_EVENT_TYPE`
     - The event type of the notification (e.g. :code:`mars`).
   * - :code:`ECF_AVISO_EVENT_SEQUENCE`
     - The sequence number of the notification.
   * - :code:`ECF_AVISO_EVENT_DATA_IDENTIFIER`
     - The identifier of the notification, as a JSON object (e.g. :code:`{"class":"od","step":"6","stream":"enfo"}`).
   * - :code:`ECF_AVISO_EVENT_DATA_PAYLOAD`
     - The payload of the notification, as JSON (e.g. :code:`{"location":"file:///path/to/data"}`).

The variables are always defined: until a notification releases the node, they are empty, and the sequence
is 0. They keep their values until the next release, and survive a server restart.

The identifier and the payload are JSON, and may contain characters that are special to the shell (such as
double quotes); in a task script, enclose them in single quotes:

.. code-block:: shell

    payload='%ECF_AVISO_EVENT_DATA_PAYLOAD%'

.. _text_based_def_aviso_errors:

Errors
======

An error prevents the node from being released, but never stops the server or affects other nodes. The
node remains queued, with the flag :code:`remote_error` set, and the reason is shown by the attribute
(e.g. in ecFlowUI, or with :code:`ecflow_client --get_state`):

 - a configuration error, such as an empty or unresolved URL or credentials variable, an invalid
   listener, or a credentials file that cannot be read;
 - an error reported by the Aviso server, such as a request rejected by its schema, or credentials
   that are not accepted.

An empty or unresolved URL or credentials variable is reported with a plain description, naming the variable
to define. An error raised while creating or running the watch (an invalid listener, a credentials file that
cannot be read, or an error reported by the Aviso server) is reported as
:code:`Aviso error (<kind>[, HTTP <status>]): <message>[ [request <id>]]`, followed by a reminder that
ecFlow 5.19.x is the last release that supports Aviso v1, since an Aviso v1 server, or an Aviso v1
configuration, also causes an error.

The Aviso client library keeps the watch running across routine interruptions: when the Aviso server closes
the stream (e.g. at the end of its maximum connection duration, or when it shuts down), or when the connection
drops, the library reconnects by itself, resuming after the last notification received. A server that cannot
be reached is retried silently by the library, and is not reported on the node.

When the watch ends with an error that the library does not recover from, the watch is created again after a
fixed delay of 60 seconds, resuming after the last notification received; the error is cleared when the watch
is created again.

.. note::

   When the variables providing the configuration are updated, the configuration can be applied without
   requeuing the node, by changing the attribute with the value :code:`reload`:

   .. code-block:: shell

      ecflow_client --alter=change aviso <name> reload /path/to/node

   Changing the attribute with a list of options (e.g. a different listener) replaces it, and the new
   attribute starts from the revision given by :code:`--revision`, or 0 when the option is omitted; in that
   case, only the notifications published after the change are considered. To continue after the
   notification that last released the node, give the current revision of the attribute (as shown in the
   definition, e.g. by :code:`ecflow_client --get`) with :code:`--revision`:

   .. code-block:: shell

      ecflow_client --alter=change aviso <name> "--listener '<json>' --revision <revision>" /path/to/node
