.. _version_5.18:

Version 5.18 updates
********************

.. role:: jiraissue
   :class: hidden

.. role:: githubissue
   :class: hidden

Version 5.18.0
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2026-07-07

General
-------

- **Feature** add :code:`ECF_DIRNAME`/:code:`ECF_BASENAME` generated variables :jiraissue:`ECFLOW-2071`
- **Feature** enable state propagation for Mirror attribute, via new :code:`--propagate` option :githubissue:`GH#353`
- **Feature** enable :code:`ecflow_client --query variable /:name` to retrieve variables attached to the server itself :jiraissue:`ECFLOW-2108`

- **Improvement** add username to failed Authentication/Authorisation messages :jiraissue:`ECFLOW-2097`
- **Improvement** suppress noisy Mirror poll debug messages :jiraissue:`ECFLOW-2098`
- **Improvement** add GitHub Action and Dockerfile to build/publish the ecFlow Docker image

- **Fix** correct Resource values (memory/page size) in Server Information panel to match expected units :jiraissue:`ECFLOW-2096`

REST
----

- **Feature** add new :code:`.../info` endpoint to provide node information :jiraissue:`ECFLOW-2105`

Python API
----------

- **Improvement** enable dynamic attributes on ecflow Python objects :jiraissue:`ECFLOW-2100`
- **Improvement** allow creating Variables with any Python integer :jiraissue:`ECFLOW-2100`
- **Improvement** reorganise and improve the Python API documentation :jiraissue:`ECFLOW-2101`

- **Fix** correct comparison/equality operators and string conversion of enums for invalid values :jiraissue:`ECFLOW-2100`

ecFlowUI
--------

- **Feature** automatically reload and scroll to the latest job output content, with new "Output refresh rate" Preference :jiraissue:`ECFLOW-2065`

- **Improvement** replace :code:`boost::property_tree` with new (drop-in) :code:`ecf::PTree` implementation :jiraissue:`ECFLOW-1922`

Documentation
-------------

- **Improvement** add Related Tools section to the User Manual
- **Improvement** clarify the Cron attribute documentation
- **Improvement** improve the description of Suite variables :githubissue:`GH#354`

- **Fix** correct :code:`ECF_JOB_CMD` definition in documentation :githubissue:`GH#360`
