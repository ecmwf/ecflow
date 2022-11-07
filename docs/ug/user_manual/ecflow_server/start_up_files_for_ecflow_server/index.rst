.. _start_up_files_for_ecflow_server:

Start-up files for ecFlow server
////////////////////////////////

When ecFlow is started up it uses a number of files for configuration
and reporting.

These files can be configured through `environment variables <https://confluence.ecmwf.int/display/ECFLOW/ecFlow+Server+environment+variables>`__
or in an ecFlow configuration file (e.g. the file
**server_environment.config** in the ecFlow source directories\ **)**.

The first thing ecFlow does is to change the **current working
directory** (or CWD) into `ECF_HOME <https://confluence.ecmwf.int/display/ECFLOW/ecFlow+Server+environment+variables>`__,
so all the other files listed there are read from that directory (unless
full path names are used).

This directory must be accessible and writable by the user starting
ecFlow otherwise ecFlow cannot start.

.. toctree::
    :maxdepth: 1
   

    ecflow_log_file
    ecflow_check_point_file_and_failure_tolerance