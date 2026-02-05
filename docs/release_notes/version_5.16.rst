.. _version_5.16:

Version 5.16 updates
********************

.. role:: jiraissue
   :class: hidden

Version 5.16.0
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2026-02-06

General
-------

- **Improvement** restrict Mirror attribute polling period to a minimum of 60 seconds :jiraissue:`ECFLOW-1995`
- **Improvement** allow ``extern`` to point into local suite :jiraissue:`ECFLOW-2033`
- **Improvement** add limit usage information to log file :jiraissue:`ECFLOW-2043`
- **Improvement** enable the use of #include <> in whitelist file :jiraissue:`ECFLOW-2049`
- **Improvement** align password file names used by ecflow_server and ecflow_client :jiraissue:`ECFLOW-2056`
- **Improvement** enable to add an inlimit with -s (submission) or -n (group) flags :jiraissue:`ECFLOW-2064`
- **Improvement** include process/machine resources in server `--stats` :jiraissue:`ECFLOW-2066`
- **Improvement** enforce the use of TLS v1.3 by the server #infosec :jiraissue:`ECFLOW-2001`

- **Fix** correct reload of whitelist file when created after server start :jiraissue:`ECFLOW-2057`
- **Fix** correct removal of Mirror attribute when remote server is unavailable :jiraissue:`ECFLOW-2058`
- **Fix** correct log server filtering of null characters :jiraissue:`ECFLOW-2061`
- **Fix** replace hardcoded artifacts by dynamically generated artifacts in tests #infosec :jiraissue:`ECFLOW-2070`

ecFlowUI
--------

- **Improvement** add delete node command to context menu :jiraissue:`ECFLOW-1842` :jiraissue:`ECFLOW-2046`
- **Improvement** add button to easily copy log file path :jiraissue:`ECFLOW-2045`

- **Fix** correct cancel of adding table view :jiraissue:`ECFLOW-2044`
- **Fix** correct display of Trigger AST :jiraissue:`ECFLOW-2048`
