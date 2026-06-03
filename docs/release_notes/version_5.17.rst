.. _version_5.17:

Version 5.17 updates
********************

.. role:: jiraissue
   :class: hidden

.. role:: githubissue
   :class: hidden

Version 5.17.0
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2026-06-03

General
-------

- **Feature** enable support for RepeatDateTimeList :jiraissue:`ECFLOW-2080`
- **Feature** enable support for Debian 13 (+ .deb package) :jiraissue:`ECFLOW-2082`
- **Feature** Enable Repeat.current_index()/_value() in Python API :jiraissue:`ECFLOW-2084`

- **Improvement** review LICENSE + NOTICE content :jiraissue:`ECFLOW-2088`
- **Improvement** enable support for compilers GNU GCC 15.2/Intel 2025.3.1 :jiraissue:`ECFLOW-2078/ECFLOW-2079`
- **Improvement** refactor string/node path algorithms (i.e. remove boost::algorithm dependencies) :jiraissue:`ECFLOW-2076,ECFLOW-1922`
- **Improvement** review server log content to ensure no information disclosure :jiraissue:`ECFLOW-2004`
- **Improvement** review SSL server implementation to ensure no use of weak encryption algorithms :jiraissue:`ECFLOW-2003`

- **Fix** correct parsing of Variables with whitespace/special characters in their value :jiraissue:`ECFLOW-2089`
- **Fix** correct handling of '-s'/'-n' flag when adding inlimit using ecflow_client :jiraissue:`ECFLOW-2094`
- **Fix** correct handling of Trigger subtraction expression :code:`:NONEXISTENT - 1` :jiraissue:`ECFLOW-2083`
- **Fix** correct optimisation for parsing of Simple Trigger Expressions :jiraissue:`ECFLOW-2075`
- **Fix** correct handling of :code:`--help` option when ECF_SSL is set :jiraissue:`ECFLOW-2081`
- **Fix** avoid clobbering of :code:`setup.py` and :code:`__init__.py` during multi-preset CMake configuration :jiraissue:`ECFLOW-2077`

Python API
----------

- **Improvement** replace Boost.Python with Pybind11 for Python bindings :jiraissue:`ECFLOW-2090`

ecFlowUI
--------

- **Fix** correct intermittent :code:`ecflow_ui` crash :jiraissue:`ECFLOW-2090`
