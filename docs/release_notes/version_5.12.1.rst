.. _version_5.12.1:

Version 5.12.1 updates
//////////////////////

.. role:: jiraissue
   :class: hidden

Version 5.12.1
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-03-08

ecFlow Client
-------------

- **Bug** corrected handling of :code:`--label` when value starts with :code:`-` or :code:`--` :jiraissue:`ECFLOW-1945`


ecFlow Python
-------------

- **Improvement** enabled returning statistics as string when calling :code:`ecflow.Client.stats()` :jiraissue:`ECFLOW-1944`
