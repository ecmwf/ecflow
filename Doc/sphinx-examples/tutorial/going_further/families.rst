.. index::
   single: families

.. _families:

Families
========

| Tasks can be logically grouped into :term:`family`'s.
| You can picture a suite as a hierarchical structure very similar to a unix 
| file system, where the families are the directories and the tasks are the files. 
| The suite is a family with some extra attributes (See :ref:`dates-and-clocks`). 
| Like directories, families can themselves contain other families. 
| And like directories, there can be many tasks with the same name, as long as 
| they are in different families.
 
| Unless you tell ecFlow where to find specific files, the default behaviour 
| is to expect the file structure to reflect the structure of the suite. 

Ecf Script
----------

| In suite definition below we will create a family **f1** with two tasks **t1** and **t2**.
| In this case you will have to create a directory :file:`$HOME/course/test/f1`, 
| and move :file:`t1.ecf` and :file:`t2.ecf` into it. 
| Conversely, the ecFlow jobs and the outputs will be created in this directory.

| Because we have moved the scripts to another directory, ecFlow will not find 
| the two included files :ref:`head_h` and :ref:`tail_h` one directory up from 
| the scripts. 

| We could modify the scripts to search the include file two directories up,
| but this would be very cumbersome. 
| The solution is to define a special ecFlow :term:`variable` called **ECF_INCLUDE** 
| that  points to the directory containing the include files. See :term:`pre-processing`

| Whenever angled brackets are used, ecFlow first looks to see if ECF_INCLUDE
| variable is specified. If the variable exists, it checks to see if file 
| %ECF_INCLUDE%/head.h exists, otherwise it looks for %ECF_HOME%/head.h
| This has the added advantage that specific includes files can be placed under 
| ECF_INCLUDE, and includes file common to **many** tasks can placed in
| ECF_HOME. For more details see :term:`directives`.  

| We need to do the following changes to the :term:`ecf script`'s. 

from::

   %include "../head.h"  
   echo "I am part of a suite that lives in %ECF_HOME%"
   %include "../tail.h" 

to::

   %include <head.h> 
   echo "I am part of a suite that lives in %ECF_HOME%"
   %include <tail.h> 

:term:`suite`'s, :term:`family`'s and :term:`task`'s are called :term:`node`'s.
 

Text
----

::

   # Definition of the suite test.
   suite test
      edit ECF_INCLUDE "$HOME/course"  # replace '$HOME' with the path to your home directory
      edit ECF_HOME    "$HOME/course"
      family f1 
         task t1
         task t2
      endfamily 
   endsuite
   
Python
------

If you are using the :ref:`suite_definition_python_api`:

.. literalinclude:: src/families.py
  
The hierarchy is shown as a tree in :term:`ecflowview`.


**What to do:**

#. Update the :term:`suite definition`
#. Create the directories needed, move the :term:`ecf script`'s
#. Edit the script to include :ref:`head_h` and :ref:`tail_h` from the ECF_INCLUDE directory.
#. Replace the :term:`suite`
#. View the suite in :term:`ecflowview`, notice the tree structure. 
   You may have to unfold **test** and **f1** to see the tasks, using the middle mouse button.
