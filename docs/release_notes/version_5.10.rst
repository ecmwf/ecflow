.. _version_5.10:

Version 5.10 updates
////////////////////


Version 5.10.0
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2023-02-09

REST API
--------

- **New feature**: a new HTTP server provides access to a running ecFlow server via a REST API. This is still experimental and subject to change, but if you'd like to have a look, the documentation is here: :ref:`rest_api`

ecFlowUI
--------

- **Improvement**: a repeat date list is now shown as numbers instead of strings in the suite info panel


General
-------

- **New feature**: added an option `as_bytes` to the Python API call :py:meth:`ecflow.Client.get_file` to return non-ASCII output

- **Improvement**: added a timestamp to the warning generated when the server fails to write its log file

- **Fix**: fixed issue where the server stats showed the wrong number of suites and did not display anything for the request frequency. Available via the client :ref:`stats<stats_cli>` command or the 'Info' tab in ecFlowUI.

- **Fix**: fixed issue where `--password` was not used by the command-line interface even if supplied
