.. _version_5.11:

Version 5.11 updates
********************

Version 5.11.4
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2023-10-10


General
-------

- **Fix**: Correct the finding of the logserver script


Version 5.11.3
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2023-07-18


General
-------

- **Fix**: Correct handling of client SSL connection


Version 5.11.2
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2023-07-13


General
-------

- **Fix**: Correct compilation issues on GCC 13
- **Fix**: Correct compilation issues on Conda Forge


Version 5.11.1
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2023-07-03


General
-------

- **Fix**: Correct the use of child commands to handle ecflow UDP requests


Version 5.11.0
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2023-05-25


UDP API
--------

- **New feature**: a new UDP server provides access to a running ecFlow server via UDP. This is still experimental and
  subject to change, but if you'd like to have a look, the documentation is here: :ref:`udp_api`


ecFlowUI
--------

- **Improvement**: enabled the use of user-defined command with quotes


General
-------

- **Fix**: enabled successful compilation using Clang 16

- **Improvement**: various source code improvements: silence compiler warnings, apply clang-format, remove unused
  source code

- **Improvement**: corrected miscellaneous documentation issues
