.. _version_5.12:

Version 5.12 updates
********************

.. role:: jiraissue
   :class: hidden

Version 5.12.4
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-03-23

ecFlow Client
-------------

- **Bug** corrected ecFlow Python3 module linkage (specific for conda-forge) :jiraissue:`ECFLOW-1950`

ecFlow HTTP
-----------

- **Feature** added :code:`id` field to each Node in the :code:`/<path>/tree` endpoint response :jiraissue:`ECFLOW-1952`
- **Improvement** added *flag* attribute to Nodes in the :code:`/<path>/tree` endpoint response :jiraissue:`ECFLOW-1954`
- **Bug** corrected Alias node structure in the :code:`/<path>/tree` endpoint response :jiraissue:`ECFLOW-1953`

Version 5.12.3
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-03-14

ecFlow Client
-------------

- **Bug** corrected handling of :code:`--meter` when value starts with :code:`-` (e.g. :code:`-1`) :jiraissue:`ECFLOW-1948`


Version 5.12.2
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-03-11

ecFlow Python
-------------

- **Improvement** allow to disable printing output to :code:`stdout` from :code:`ecflow.Client.stats()` :jiraissue:`ECFLOW-1944`
- **Bug** corrected failing tests related to ecFlow Client Python API :jiraissue:`ECFLOW-1944,ECFLOW-1947`


Version 5.12.1
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-03-07

ecFlow Client
-------------

- **Bug** corrected handling of :code:`--label` when value starts with :code:`-` or :code:`--` :jiraissue:`ECFLOW-1945`


ecFlow Python
-------------

- **Improvement** enabled returning statistics as string when calling :code:`ecflow.Client.stats()` :jiraissue:`ECFLOW-1944`


Version 5.12.0
==============

* `Released <https://confluence.ecmwf.int/display/ECFLOW/Releases>`__\  on 2024-02-13

ecFlow Server
-------------

- **New Feature**: enabled hourly iteration (i.e. :code:`Repeat` based on yyyymmddTHHMMSS) :jiraissue:`ECFLOW-1900`
- **Improvement**: enabled the use of other ecFlow variables to define :code:`ECF_INCLUDE` :jiraissue:`ECFLOW-1920`
- **Improvement**: enabled atomic replacement of checkpoint file :jiraissue:`ECFLOW-1925`
- **Improvement**: improved indentation performance for checkpoint/definitions file storage :jiraissue:`ECFLOW-1928`
- **Bug** corrected logging of suite nodes changing state to *queued* :jiraissue:`ECFLOW-1914`

ecFlow Client
-------------

- **Bug** corrected handling of :code:`--debug` option on :code:`ecflow_client` :jiraissue:`ECFLOW-1885`
- **Bug** corrected handling of short form options (:code:`-d`, :code:`-v`, :code:`-h`) on :code:`ecflow_client` :jiraissue:`ECFLOW-1884`
- **Bug** corrected handling of Alter command with attribute value starting with :code:`--` (dashes) :jiraissue:`ECFLOW-1883`
- **Bug** removed options reported as commands in :code:`ecflow_client --help` output :jiraissue:`ECFLOW-1874`

ecFlow UI
---------

- **Improvement** enabled scrollable widget to display errors in timeline view :jiraissue:`ECFLOW-1896`
- **Improvement** corrected handling of commands tokens before executed in client :jiraissue:`ECFLOW-1887`

ecFlow HTTP
-----------

- **Improvement** enabled authentication for Task requests :jiraissue:`ECFLOW-1921`
- **Improvement** enabled compression of response payload :jiraissue:`ECFLOW-1940`
- **Improvement** enabled :code:`/<path>/tree` endpoint to access tree including node status information :jiraissue:`ECFLOW-1939`
- **Bug** updated :code:`/<path>/tree` endpoint response to preserve node order :jiraissue:`ECFLOW-1910`
- **Bug** updated :code:`/<path>/attributes` endpoint response to include generated variables :jiraissue:`ECFLOW-1909`
- **Bug** corrected handling of *queue* child command :jiraissue:`ECFLOW-1937`
- **Bug** corrected failing tests when using AOCC compiler on Atos HPC :jiraissue:`ECFLOW-1882`

General
-------

- **Improvement** reviewed project source code/include structure :jiraissue:`ECFLOW-1932`
- **Improvement** reviewed the unit/integration/system tests structure :jiraissue:`ECFLOW-1934`
- **Improvement** modernized/clean up of CMakeLists.txt :jiraissue:`ECFLOW-1923`
- **Improvement** silenced project configuration warnings when using CMake 3.27+ :jiraissue:`ECFLOW-1936`
- **Improvement** clean up of unused artefacts/documentation :jiraissue:`ECFLOW-1929`
- **Improvement** disabled ecflow_start/stop.sh execution on the Atos HPC :jiraissue:`ECFLOW-1899`
- **Bug** corrected compilation/linkage issue with Intel C++ compiler :jiraissue:`ECFLOW-1913`
- **Bug** corrected CMake configuration without HTTP support when a recent OpenSSL is not found :jiraissue:`ECFLOW-1912`
- **Bug** corrected failing builds of ecFlow on conda-forge :jiraissue:`ECFLOW-1881`
- **Bug** silenced compilation warnings :jiraissue:`ECFLOW-1878`
- **Documentation** clarified minimum SSL key size used by ecflow_server :jiraissue:`ECFLOW-1907,ECFLOW-1904`
- **Documentation** clarified "Build from source" instructions :jiraissue:`ECFLOW-1930`
- **Documentation** clarified Cron dependency description :jiraissue:`ECFLOW-1903`
- **Documentation** updated documentation dependencies at readthedocs :jiraissue:`ECFLOW-1897`
- **Documentation** corrected Python API documentation formatting :jiraissue:`ECFLOW-1886`
