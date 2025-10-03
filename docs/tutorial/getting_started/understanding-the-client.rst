.. index::
   single: Client (tutorial)
   
.. _tutorial-understanding-the-client:
   
Understanding the client
========================

There are multiple ways to communicate with an :term:`ecflow_server`, including using CLI (Command Line Interface)
:term:`ecflow_client`, programmatically via the :ref:`Python API <python_api>` or using the GUI (Graphical User Interface)
:term:`ecflow_ui`.

For any kink of communication with the server, since there can be multiple servers running in parallel on the same machine,
the client needs to know the machine host name and port to access the server.

This section shows how to contact the server using the CLI client and via the Python API.

To configure the client (CLI and Python API) to contact a server on the given _host_ and _port_ consider the following:

* The default _host_:_port_ is :code:`localhost:3141`.
* These defaults are overridden by setting the environment variables: :code:`ECF_HOST` and :code:`ECF_PORT`.
* The explicitly defined options :code:`--port` and :code:`--host` will always be used whenever provided.

.. tabs::

    .. tab:: CLI

        The :term:`ecflow_client` is a command line tool that allows sending commands to the :term:`ecflow_server`,
        and retrieving information about the current state of the elements that compose the :term:`suite definition`.

        A list of available commands can be found by using the :code:`--help` option:

        .. code-block:: shell

           ecflow_client --help

        To assess the connectivity to a server, explicitly define the :code:`--port` and :code:`--host` options, and use the :code:`--ping` command:

        .. code-block:: shell
           :caption: Ping an ecFlow server using the Python API

           ecflow_client --host machinex --port 4141  --ping

    .. tab:: Python

        The :ref:`Python API <python_api>` provides the same functionality as the CLI :term:`ecflow_client`,
        with the added bonus that allows to leverage on Python for automation.

        In terms of connectivity to the server, the :ref:`Python API <python_api>` uses the same logic as the CLI,
        including the ability to explicitly set :code:`host` and :code:`port` options.

        The class :py:class:`ecflow.Client` provides the interface to the :term:`ecflow_server`, such as assessing the connectivity to a server
        using the :py:meth:`ecflow.Client.ping` method:

        .. literalinclude:: src/understanding-the-client.py
           :language: python
           :caption: Ping an ecFlow server using the Python API
    
**What to do**

#. List the available commands of :term:`ecflow_client` using the :code:`--help` option.

#. Ping the :term:`ecflow_server` using the CLI :term:`ecflow_client`, explicitly defining the :code:`--host` and :code:`--port` options.

#. Ping the :term:`ecflow_server` using the CLI :term:`ecflow_client`, exporting environment variables :code:`ECF_HOST` and :code:`ECF_PORT`.
