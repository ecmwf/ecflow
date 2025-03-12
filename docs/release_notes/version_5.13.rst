.. _version_5.13:

Version 5.13 updates
////////////////////

.. role:: jiraissue
   :class: hidden

Version 5.13.8
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2025-03-12

General
-------

- **Fix** correct Aviso notification retrieval after automatic requeueing :jiraissue:`ECFLOW-2010`

Python
------

- **Fix** correct quote handling for ecflow.AvisoAttr listener :jiraissue:`ECFLOW-2011`

Documentation
-------------

- **Improvement** clarify use of schema for ecFlow Aviso attribute :jiraissue:`ECFLOW-2008`
- **Improvement** clarify how to define ecFlow Aviso authentication :jiraissue:`ECFLOW-2008`

Version 5.13.7
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2025-02-06

General
-------

- **Improvement** allow requesting explicit execution of ecflow_start.sh :jiraissue:`ECFLOW-1991`
- **Improvement** enable mirror of non-generated inherited variables :jiraissue:`ECFLOW-1999`
- **Fix** replace use of deprecated functions to build with Boost 1.87 :jiraissue:`ECFLOW-1997`

ecFlow UI
---------

- **Fix** correct handling of Repeat DateTime values :jiraissue:`ECFLOW-1993`
- **Fix** correct selection in suite filter after Suite replacement :jiraissue:`ECFLOW-1994`

Documentation
-------------

- **Improvement** clarify use of attributes date and cron :jiraissue:`ECFLOW-1996`

Version 5.13.6
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-11-29

General
-------

- **Improvement** enable explicit 'reload' of Mirror attribute configuration :jiraissue:`ECFLOW-1986`
- **Improvement** enable automatic builds of rpm/deb packages :jiraissue:`ECFLOW-1967`
- **Fix** correct replacement of nodes with Repeat DateTime attributes :jiraissue:`ECFLOW-1992`
- **Fix** correct handling of :code:`ecflow_client` option :code:`--host` when searching for SSL certificate file :jiraissue:`ECFLOW-1985`
- **Fix** correct build setup used on conda-forge with Python 3.13 :jiraissue:`ECFLOW-1987`

ecFlow UI
---------

- **Improvement** enable Repeat DateTime attribute editor :jiraissue:`ECFLOW-1988`

Version 5.13.5
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-10-31

General
-------

- **Fix** correct the token used for Aviso authentication :jiraissue:`ECFLOW-1982`

ecFlow UI
---------

- **Fix** correct the disabled/enabled status of Session dialog buttons :jiraissue:`ECFLOW-1980`

ecFlow HTTP
-----------

- **Fix** reorganise :code:`inherited_variables` to allow repeated names of Node ancestors :jiraissue:`ECFLOW-1978`
- **Fix** add missing inherited variables to response of endppoint :code:`.../tree` :jiraissue:`ECFLOW-1977`

Python
------

- **Fix** correct :code:`check_job_creation()` crash on suite with Mirror/Aviso attribute :jiraissue:`ECFLOW-1976`
- **Fix** allow default parameters in Aviso/Mirror attribute constructors :jiraissue:`ECFLOW-1981`

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
