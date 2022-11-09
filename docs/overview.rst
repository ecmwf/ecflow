.. _overview:

Overview
//////////////

.. image:: /_static/overview/image2.png
   :width: 4.16667in
   :height: 2.60417in


**ecFlow** is a client/server workflow package that enables users to run
a large number of programs (with dependencies on each other and on time)
in a controlled environment. It provides reasonable tolerance for
hardware and software failures, combined with restart capabilities. It
is used at ECMWF to run all our operational suites across a range of
platforms.

**ecFlow** submits **tasks** (jobs) and receives acknowledgments from
the tasks when they change **status** and when they send **events**,
using **child** commands embedded in your scripts. ecFlow stores the
relationships between tasks, and is able to submit tasks dependant on
triggers, such as when a given task changes its status, for example when
it finishes. 

**ecFlow** functionality is provided by the following executables and
shared libraries

-  :term:`ecflow_client`:
   This executable is a command-line program; it is used for all
   communication with the server. This executable implements the :ref:`ecflow_cli`. The bulk of this functionality is also provided by the :ref:`python_api`

-  :ref:`ecflow_ui`:
   This is a specialized GUI client that monitors and visualizes the
   node tree hierarchy. Based on QT.

-  :term:`ecflow_server`:
   This executable is the server. It is responsible for scheduling the
   jobs and responding to ecflow_client requests.

-  :ref:`ecflow.so <python_api>`: Python interface. This shared library provide the python API for creating the suite definition and communication with the server.

**ecFlow** runs as a server receiving requests from clients. CLI, GUI,
and suite jobs are clients. Communication is based on TCP/IP. Note that
ecFlow is a scheduler sand is not a queuing system such as NQS, SGE, Load
leveler, or PBS. However, it can submit to queuing systems.

History
=======

For almost three decades ECMWF used SMS (Supervisor Monitoring
Scheduler) package to control the workflow for the Centre's operational
models and systems. Written at ECMWF, it allows the design, submission,
and monitoring of jobs both in the Research and Operations Departments,
and provides common tools for scientists, analysts, and operators to
cooperate. A large number of organizations also use SMS (from both
Member and non-Member States). Development and support of SMS has now
stopped.

The replacement, ecFlow 4, has superseded SMS. It has been used for over
a decade. It is a complete rewrite using an object-oriented methodology
and modern standardized components; it acts as a comprehensive
replacement for SMS. The rewrite helps improve maintainability, allows
easier modification, and introduces object orientated features. The
proprietary script language used by SMS, CDP, has been replaced by
Python.

The development of ecFlow 4 has now stopped. But support is still
provided. It has been replaced by ecFlow 5.

You can find an introduction to ecFlow in the `ECMWF Newsletter
article <http://www.ecmwf.int/sites/default/files/elibrary/2011/14594-newsletter-no129-autumn-2011.pdf>`__ 
(Autumn 2011), starting on page 30.



