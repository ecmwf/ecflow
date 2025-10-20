.. _version_5.15:

Version 5.15 updates
********************

.. role:: jiraissue
   :class: hidden

Version 5.15.1
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2025-10-20

Server
------

- **Bug Fix** correct handling of :code:`state_change` in checkpoint file :jiraissue:`ECFLOW-2042`
- **Improvement** allow use of :code:`ecflow_start.sh` script when mail command is not available :jiraissue:`ECFLOW-2040`

Version 5.15.0
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2025-09-11

General
-------

- **New Feature** enable *HTTPS* authentication for ecFlow :jiraissue:`ECFLOW-2041`

.. important::

   This feature is still experimental and subject to change.
   To learn how to use this feature see the documentation available :ref:`here <how_to_setup_ecflow_with_https_authentication>`.

- **Improvement** enable connection retry for User Commands based on :code:`ECF_HOSTFILE_POLICY` :jiraissue:`ECFLOW-2034`
- **Improvement** allow including ecFlow variables in Aviso attribute configuration :jiraissue:`ECFLOW-2029`
- **Improvement** consolidate Boost dependencies :jiraissue:`ECFLOW-1922`

  - remove use of :code:`Boost.chrono` and :code:`Boost.timers`
  - reduce scope of :code:`Boost.posix_time` and :code:`Boost.date_time`

- **Fix** correct use of :code:`ECF_HOSTFILE` with option :code:`--host`/:code:`--port` :jiraissue:`ECFLOW-2036`


ecFlowUI
--------

- **Fix** correct progress markers (:code:`...`) display for Repeats :jiraissue:`ECFLOW-2038`
- **Fix** correct display of DateTime values :jiraissue:`ECFLOW-2035`

Python
------

- **Fix** correct Defs indentation when printing using Python :jiraissue:`ECFLOW-2037`

Documentation
-------------

- **Improvement** improve :code:`--alter` documentation regarding updating Repeats :jiraissue:`ECFLOW-2039`
