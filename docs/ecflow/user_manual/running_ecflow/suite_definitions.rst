.. _suite_definitions:

Suite definitions
/////////////////

Once you have the server running you can **define** a suite to run on it. The suite is described by a **suite definition file**. This is covered more completely in later sections. A suite definition in text format will have a structure similar to the following:

.. code-block:: shell

    # Definition of the suite test                                                                                                      
    suite test
      edit ECF_HOME /tmp/COURSEDIR
      task t1
    endsuite                                                     

1. The first line is a comment line. Any characters between the # and
   the end of the line are ignored.

2. Defines a new suite by the name of test. 

3. Defines the ecFlow variable **ECF_HOME**. This variable defines the
   directory where all the UNIX files that will be used by the suite
   test will reside.

4. Defines a task named **t1**.

5. The last line finishes the definition of the suite test

Defining suites using the `Python API <https://confluence.ecmwf.int/display/ECFLOW/ecFlow+Python+Api>`__ is discussed later in this manual.
