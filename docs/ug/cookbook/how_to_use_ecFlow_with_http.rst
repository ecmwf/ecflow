.. _how_to_use_ecflow_with_http:

How use ecFlow with HTTP/HTTPS?
*******************************

ecFlow uses by default a TCP/IP socket to communicate between the multiple ecFlow components,
but it is possible to use HTTP/HTTPS communication instead. This is useful when
the ecFlow server is running behind a firewall, in an environment that does not allow
TCP/IP socket communication. The following instructions describe the necessary steps to use
HTTP/HTTPS communication between ecFlow server and clients (including CLI and UI).
This feature requires ecFlow 5.14+.

.. warning::
    The support for HTTP/HTTPS is currently experimental, and thus may be subject to change.
    Do not use this feature for operational purposes.

Notice that ecFlow server doesn't support HTTPS communication by default, but it is possible to
configure it to use HTTPS by using a reverse proxy server (e.g. Nginx, Apache) that
can handle SSL termination. The reverse proxy server will listen on port 443 (or any other
port you choose) and forward the requests to the ecFlow server running on a different port (e.g. 3141).

The documentation here first describes how to set up the ecFlow server to use HTTP communication,
and how to configure the multiple ecFlow components to use HTTP.
Then, we proceed to describe how to set up the reverse proxy server and thus enable HTTPS communication.

Setup the ecFlow Server with HTTP
=================================

Deploy the ecFlow server, as per the :ref:`regular instructions<starting_the_ecflow_server>`,
considering that an additional argument `--http` is used to activate the HTTP communication.

For example, to start the ecFlow server with HTTP communication on port 31415:

.. code-block:: shell

  export ECF_PORT=31415
  ecflow_server --http -d

Use other ecFlow components with HTTP
=====================================

The ecFlow clients (CLI and UI) can be easily configured to use HTTP communication.

ecFlow CLI
----------

The ecFlow CLI will use HTTP communication when the additional argument
`--http` is used. For example, ping the ecFlow server started in the previous section with:

.. code-block:: shell

  export ECF_PORT=31415
  ecflow_client --http ping

ecFlowUI
--------

ecFlowUI is enables the use of HTTP communication, by allowing to configure ecFlow servers through the Manage Server dialog.
In the dialog, along the usual server name and port number specify the use of the protocol HTTP.

.. figure:: /_static/cookbook/http_server.png
   :align: center
   :alt: Enable HTTP communication in ecFlowUI

   Enable HTTP communication in ecFlowUI

ecFlow REST & ecFlow UDP
------------------------

ecFlow REST and ecFlow UDP are two components of ecFlow that communicate with the ecFlow server.
Both components can be configured to use HTTP communication via the additional `--http` argument.

For example, to start the ecFlow REST server with HTTP communication on port 31416:

.. code-block:: shell

  ecflow_http --http --ecflow_port 31415 --port 31416

ecFlow UDP can be started with HTTP communication in a similar way on port 31417:

.. code-block:: shell

  ecflow_udp --http --ecflow_port 31415 --port 31417

ecFlow Python API
-----------------

The ecFlow Python API is a useful tool for interacting with the ecFlow server, and can also be configured to use HTTP communication.
The class :code:`ecflow.Client` allows enabling HTTP communication using the :code:`ecflow.Client.enable_http()` method, as shown in the following example:

.. code-block:: python

  import ecflow

  # Create a client instance with HTTP
  client = ecflow.Client()
  client.enable_http()
  client.set_host_port("localhost", str(31415))

  # Ping the server
  client.ping()

Enable HTTPS connection to ecFlow Server
========================================

To enable HTTPS communication, you need to set up a reverse proxy server (e.g. Nginx)
that can handle SSL termination. The reverse proxy server can listen on port 443 (or any
other port you choose) and forward the requests to the ecFlow server running on a
different port (e.g. 31415).

The following example shows how to set up Nginx as a reverse proxy server for the ecFlow server:

.. code-block:: shell

  server {
    listen 80;
    listen [::]:80;
    server_name localhost;
    return 301 https://$host$request_uri;
  }

  server {
    listen 443 ssl;
    listen [::]:443 ssl;
    server_name your_domain_or_ip;

    ssl_certificate /path/to/your/certificate.crt;
    ssl_certificate_key /path/to/your/private.key;

    location /v1/ecflow {
      proxy_read_timeout 500s;
      proxy_connect_timeout 90s;

      proxy_pass http://localhost:31415/v1/ecflow;
      proxy_set_header Host $host;
      proxy_set_header X-Real-IP $remote_addr;
      proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
      proxy_set_header X-Forwarded-Proto $scheme;
    }
  }

Once the Nginx server is configured, you can access the ecFlow server using HTTPS from either:

* ecFlow client, by using the :code:`--https` option
* ecFlowUI, by specifying the HTTPS protocol in the server configuration

Notice that since ecFlow REST & ecFlow UDP are designed to work very closely with the ecFlow server
(typically running on the same platform) they only support HTTP communication.
