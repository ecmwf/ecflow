.. index::
   single: labels

.. _labels:

Labels
======

| Sometime it is very useful to see :term:`task` specific information in :term:`ecflowview`. 
| For this we can define a :term:`label`. A :term:`label` is a string that is attached to a :term:`task`
| and that can be updated using the :term:`child command` :term:`ecflow_client` --label

Ecf Script
----------

| We will create new family f3 with a task t1. 
| Create :term:`ecf script` :file:`$HOME/course/test/f3/t1.ecf`

::

   %include <head.h> 
   n=1 
   while [[ $n -le 5 ]]                  # Loop 5 times 
   do 
      msg="The date is now $(date)"  
      ecflow_client --label=info "$msg"  # Set the label 
      sleep 60                           # Wait a one minute 
      (( n = $n + 1 )) 
   done 
 
   ecflow_client --label info "I have now finished my work." 
 
   %include <tail.h> 

Text
----

::

   # Definition of the suite test. Note Family f1,f2 from previous pages are omitted
   suite test
    edit ECF_INCLUDE "$HOME/course"
    edit ECF_HOME    "$HOME/course"
    family f3 
        task t1 
            label info "" 
    endfamily 
   endsuite
   
Python
------

The following section shows how to add a :py:class:`ecflow.Label` in python:

.. literalinclude:: src/labels.py


**What to do:**

1. Modify the :term:`suite definition` file or python script
2. Create the new :term:`ecf script` file :file:`$HOME/course/test/f3/t1.ecf`
3. Replace the :term:`suite definition`
4. Watch in :term:`ecflowview`
