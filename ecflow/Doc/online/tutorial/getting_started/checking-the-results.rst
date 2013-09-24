.. index::
   single: Checking the results 
   
.. _checking-the-results:

Checking the results
====================

Let us now see how our suite ran, type the following::

   > ecflow_client --get_state

| This command will retrieve the :term:`suite definition` from the server, 
| and show the :term:`status` for each :term:`node`.
   
| Look at the :term:`task` **t1** if it is :term:`complete` and the :term:`suite` is *complete* 
| then the run was successful.

| If this is not the case, and you might see :term:`aborted`.
| Please check the location (directory) of your :term:`ecf script` before referring 
| to your instructor.

| The :term:`job file` created by the server is located in the same directory as the 
| :term:`ecf script`, and is named *t1.job1*. 
| Compare the files :file:`t1.ecf`, :file:`head.h`, :file:`tail.h` and :file:`t1.job1`.

| The **output** of the job is located in the same directory as the :term:`ecf script`,
| and is named :file:`t1.1`.

Retrieving the suite definition
-------------------------------

*  To retrieve the :term:`suite definition` in a form that is parse-able, type::

      > ecflow_client --get
   
   This can be done in a python script::

      #!/usr/bin/env python2.7
      import ecflow    
      try:
         ci = ecflow.Client()                 
         ci.sync_local()                                   # get server definition, by sync with client defs
         ecflow.PrintStyle.set_style( ecflow.Style.DEFS )  # set printing to show structure
         print ci.get_defs()                               # print the returned suite definition      
      except RuntimeError, e:
         print "Failed: " + str(e) 
      
      
*  To retrieve the :term:`suite definition` and show state:: 

      > ecflow_client --get_state
   
   In python this would be::

      #!/usr/bin/env python2.7
      import ecflow    
      try:
         ci = ecflow.Client()                 
         ci.sync_local()                                   # retrieve server definition, by sync with client defs
         ecflow.PrintStyle.set_style( ecflow.Style.STATE ) # set printing to show structure and state
         print ci.get_defs()                               # print the returned suite definition      
      except RuntimeError, e:
         print "Failed: " + str(e) 
         
* To list just the node paths and states in python please see: :ref:`print-all-states`


**What to do:**

1. Locate the :term:`job file` and the output file
2. Check results by retrieving the :term:`suite definition` from the server
