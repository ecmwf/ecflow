.. _handling_output:

Handling Output
///////////////

ecFlow stdout and stderr
========================

Normally you run the ecFlow server in the **background** and the
**stderr** and **stdout** are redirected to **/dev/null** by executing
the following command in a start-up script:

.. code-block:: shell

    ecflow_server > /dev/null 1>&2                                 

When learning how to use ECF, you may open a window and run ecFlow in
that window interactively. Notice that server still writes the log file
(into
`ECF_LOG <https://confluence.ecmwf.int/display/ECFLOW/ECFLOW+Server+environment+variables>`__.)

ecFlow log server
=================

The default behaviour of the GUI client is to access the output file
directly (case 1 in :numref:`fig_network`).

When this is not possible, e.g. when the GUI host cannot see the
relevant file system, the ecFlow server is asked to request the output
(case 2 in :numref:`fig_network`). If the output file is large the ecFlow server
will supply the last 10000 lines of the output. You can use the
following command to get the relevant file associated with a given
node:

.. code-block:: shell

  ecflow_client --file=node_path [ script, | job | jobout | manual | stat ]


This will output the file to standard output. This capability uses
ecFlow server to get the file. The original file can be located in a
directory that is visible to the server, but not to the client.

To view output from a server where the ecFlow server does not have
access to the file systems we can use a log server (case 3 in :numref:`fig_network`).

Using the GUI, you click on the manual, script, output, or job
buttons. If you configured your GUI to retrieve files locally
(Edit/preferences) it first looks if the required file is directly
accessible. If not, it looks into the suite definition for the
variables ECF_LOGHOST and ECF_LOGPORT to retrieve the file from the
ecFlow log server. If the log server does not respond it contacts the
ecFlow server and retrieves the file from it. This consumes available
resources for the ecFlow server, so the log server is useful in
reducing the ecFlow server load when many users try to get large
output files.

.. figure:: /_static/ug/handling_output/image1.png
    :name: fig_network
    :width: 5.26316in
    :height: 4.16667in

    Accessing job output, using **the GUI.**

The log server consists in a Perl script 'logsvr.pl' launched by a
shell script. These are available in the ecFlow distribution. The log
server uses three variables set by the shell script:

-  ECF_LOGHOST: the name of the log server host

-  ECF_LOGPORT : the port used by the log server (typically we use 9316)

-  ECF_LOGPATH : main path where the scripts may be found below, on the
   target host e.g. export ECF_LOGPATH=path1:path2:path3

-  ECF_LOGMAP : indicates the log server how to transform the file name
   to retrieve the file locally, e.g. export
   ECFLOGMAP=path1:path1:path1:local1:path2:path2:path2:local2 This
   indicates that a mapping between local1 by path1 from the file name
   when name fits, or local2 by path2 ..., path1 is local to the target
   host, and local1 is local to the ecFlow server. The log server is
   launched on the target host (ECF_LOGHOST).

You can contact the log server manually using:

.. code-block:: shell

    telnet <host><port> get_file_path                                  

The actual log server is a Perl program: ecflow_logsvr.pl, which is run
on the remote host.

It needs:

-  LOGPORT Typically 9316.

-  LOGPATH  This is a list of all the ECF_OUT specified in the suite.
   (these are directories on the remote host) separated by ':'

-  LOGMAP  This is the mapping: ECF_OUT:ECF_OUT, and then
   ECF_HOME:ECF_OUT lists,

