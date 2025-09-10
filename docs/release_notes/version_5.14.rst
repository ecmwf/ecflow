.. _version_5.14:

Version 5.14 updates
********************

.. role:: jiraissue
   :class: hidden

Version 5.14.1
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2025-07-15

General
-------

- **New Feature** add Docker environment to support HTTP-based deployment of ecFlow :jiraissue:`ECFLOW-2032`
- **Fix** correct the handling of incorrect Aviso authentication credentials file :jiraissue:`ECFLOW-2027`

Python
------

- **Improvement** reduce the footpring of Boost.python in the Python API (internal refactoring) :jiraissue:`ECFLOW-1922`

REST
----

- **Fix** correct memory allocation issues when calling endpoint :code:`.../v1/suites/{path}/definition` :jiraissue:`ECFLOW-2022`

Version 5.14.0
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2025-05-06

General
-------

- **New Feature** enable support for *HTTP* communication between ecFlow components :jiraissue:`ECFLOW-1957`

- **Improvement** enable Alter command to add Events and Meters :jiraissue:`ECFLOW-1915`
- **Improvement** add log entries to register changes to Variables/Triggers/Completed :jiraissue:`ECFLOW-2019`
- **Improvement** add log entries to register *Repeat* value per iteration :jiraissue:`ECFLOW-2018`
- **Improvement** enable support for recent CMake versions (no longer using FindBoost module) :jiraissue:`ECFLOW-1972`

- **Improvements** multiple fixes to source code to allow building with latest GNU/Clang compilers and dependencies
- **Improvements** multiple grammatical errors corrected in error messages

ecFlowUI
--------

- **Improvement** enable beginning a suspended suite :jiraissue:`ECFLOW-2018`
- **Improvement** enable option *Force -> Queued* in Node context menu for operators :jiraissue:`ECFLOW-2015`
- **Improvement** reorder Node context menu to minimize potential errors :jiraissue:`ECFLOW-1990`
- **Fix** correct the rendering of Repeat continuation markers (:code:`...`) :jiraissue:`ECFLOW-1905`

Python
------

- **Fix** correct SSL handling in Python API :jiraissue:`ECFLOW-2013`
- **Improvement** remove Python2 support :jiraissue:`ECFLOW-1974`

Documentation
-------------

- **Improvement** provide description of Trigger functions :code:`cal::date_to_julian`/:code:`cal::julian_to_date` :jiraissue:`ECFLOW-2020`
- **Improvement** provide instructions to avoid "Too many open files" errors :jiraissue:`ECFLOW-2006`
- **Improvement** clarify the use of ECF_CHECK_CMD (now declared deprecated) :jiraissue:`ECFLOW-2005`

- **Improvements** multiple clarifications and corrections to the documentation
