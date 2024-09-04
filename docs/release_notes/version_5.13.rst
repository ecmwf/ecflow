.. _version_5.13:

Version 5.13 updates
////////////////////

.. role:: jiraissue
   :class: hidden

Version 5.13.4
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-09-04

General
-------

- **Fix** eliminate memory leaks detected while using ecFlow UI :jiraissue:`ECFLOW-1969`
- **Fix** modernise use of Boost.ASIO to build with Boost 1.86 :jiraissue:`ECFLOW-1973`

Version 5.13.3
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-07-22

General
-------

- **Fix** enable fallback option values for Mirror related variables :jiraissue:`ECFLOW-1966`

Python
------

- **Fix** enable pythonic use of Node.get_generated_variables :jiraissue:`ECFLOW-1968`

Version 5.13.2
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-07-01

General
-------

- **Fix** correct compilation issue when using Qt5 :jiraissue:`ECFLOW-1964`

Version 5.13.1
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-06-28

General
-------

- **New Feature** enabled synchronisation of Node variables :jiraissue:`ECFLOW-1961`
- **New Feature** enabled synchronisation of Node attributes (Meters, Labels and Events) :jiraissue:`ECFLOW-1963`
- **Improvement** improved error messages handling unresolved variables :jiraissue:`ECFLOW-1962`

Version 5.13.0
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-06-19

General
-------

- **New Feature** enabled support for Aviso notifications :jiraissue:`ECFLOW-1931`
- **New Feature** enabled synchronisation of Node status between ecFlow servers :jiraissue:`ECFLOW-1931`
- **Improvement** improved structure/naming of ecFlow internal sources :jiraissue:`ECFLOW-1943`
