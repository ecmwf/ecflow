.. _version_5.19:

Version 5.19 updates
********************

.. role:: jiraissue
   :class: hidden

.. role:: githubissue
   :class: hidden

.. role:: commitid
   :class: hidden

Version 5.19.0
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2026-09-07

General
-------

- **Feature** enable :code:`ecflow_client --help=definition` and :code:`--help=defs/<item>` to describe definition file items :commitid:`9d50e2dae`
- **Feature** report the protocol in use by the server, in the server information :commitid:`ad7f4af79`
- **Feature** allow a limit to be reset to the nodes currently consuming it :jiraissue:`ECFLOW-2017` :commitid:`32f3c5b59`

- **Improvement** unify the :code:`ecflow_client` help and documentation in a single manifest :commitid:`5dad55f55`
- **Improvement** list all accepted states when a :code:`defstatus` value is rejected :commitid:`cc3530a5b`
- **Improvement** diagnose a mismatch between the client and server protocols, and report it in place of a generic connection error :jiraissue:`ECFLOW-2114` :commitid:`1472cf334`
- **Improvement** allow Linux clients to use the ASIO select reactor for proxychains compatibility :jiraissue:`ECFLOW-1965` :commitid:`2531077f2`
- **Improvement** reduce the use of Boost in the attribute, base, core and node libraries :jiraissue:`ECFLOW-1922` :commitid:`bfff52da6`

- **Fix** correct the identifier parsing so that identifiers attached directly to keyword tokens are rejected :jiraissue:`ECFLOW-2112` :commitid:`2420c7a17`
- **Fix** correct the null character embedded when joining with a literal separator :commitid:`cc3530a5b`
- **Fix** enable builds with OpenSSL 4.0 :commitid:`6619ea49e`
- **Fix** correct compilation issues with Clang 22 :jiraissue:`ECFLOW-2111` :commitid:`4845d3fd6`
- **Fix** declare the correct Qt dependencies in Debian packages :commitid:`cd0e3235f`

REST
----

- **Fix** correct the REST API port selection, and report a failure to bind the port :commitid:`5591fb623`
- **Fix** report unusable REST API certificates separately from port binding failures :commitid:`212abe2c0`

Python API
----------

- **Improvement** convert the Python tests to pytest :jiraissue:`ECFLOW-2110` :commitid:`b88d6151d`

ecFlowUI
--------

- **Feature** add a :code:`Suspended` option to the Defstatus menu :commitid:`cc3530a5b`
- **Feature** associate each server in the tree with a badge showing the configure protocol :jiraissue:`ECFLOW-2114` :commitid:`1472cf334`
- **Feature** store window, tab and pane layout changes automatically after the change :jiraissue:`ECFLOW-2000` :commitid:`142579a10`

- **Improvement** distinguish a communication protocol mismatch from a server that cannot be reached :jiraissue:`ECFLOW-2114` :commitid:`1472cf334`

- **Fix** correct an abort on deeply nested trigger expressions :jiraissue:`ECFLOW-2116` :commitid:`ccacf4605`
- **Fix** prevent a crash when opening an ecFlowUI info panel :commitid:`a4a57b994`
- **Fix** prevent a crash when quitting ecFlowUI with notification widgets active :commitid:`bf683ba92`

Documentation
-------------

- **Improvement** document the ecFlowUI Timeline panel :jiraissue:`ECFLOW-2109` :commitid:`99887f77c`
- **Improvement** document how ecFlowUI reports connection states and server protocols :jiraissue:`ECFLOW-2114` :commitid:`1472cf334`
- **Improvement** improve the structure and content of the documentation :commitid:`0f7368e71`
- **Improvement** improve the Teleport usage instructions :jiraissue:`ECFLOW-1965` :commitid:`df9e62a72`
- **Improvement** clarify :code:`defstatus` and suspended in the documentation :commitid:`cc3530a5b`
- **Improvement** publish generated command, option and client-server internals documentation :commitid:`98fdaa296`
- **Improvement** clarify what Execute, Rerun and Requeue reset :jiraissue:`ECFLOW-2087` :commitid:`4dc364360`
