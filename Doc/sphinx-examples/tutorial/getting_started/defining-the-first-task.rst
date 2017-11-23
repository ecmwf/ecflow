.. index::
   single: script
   single: ECF_HOME
    
.. _defining-a-task:

Defining the first task
=======================

| Next, we need to write the :term:`ecf script` for the task **t1**. 
| By default ecFlow expects files to be in a directory structure below ECF_HOME 
| that reflect the hierarchy of the suites. The :term:`task` **t1** being in the :term:`suite` test, 
| the :term:`ecf script` for the task **t1** must be in a sub-directory test.

* In ECF_HOME, create a directory test::

  > mkdir test

* In test, create a file :file:`t1.ecf` with the following contents::

   %include "../head.h" 
   echo "I am part of a suite that lives in %ECF_HOME%" 
   %include "../tail.h" 


.. _job-creation:

Job creation
------------

| Before submitting the task, the server will transform the :term:`ecf script` to a :term:`job file`
| This process is known as :term:`job creation`. 

| This involves locating the :term:`ecf script` on disk, and then :term:`pre-processing` the
| :term:`directives`. This process includes performing :term:`variable substitution`.

| This will create a file with a '.job' extension.
| This is the script that :term:`ecflow_server` will submit to your system.

In our case:

* %include "../head.h" will be substituted by the content of the file :ref:`head_h`.

  | Note that the file name is given relatively to the file :file:`t1.ecf`, 
  | i.e. in the directory above the one containing :file:`t1.ecf`
   
* %ECF_HOME% will be substituted by the value of the variable ECF_HOME
* %include *"../tail.h"* will be substituted by the content of the file :ref:`tail_h`


**What to do:**

#. Create the :term:`ecf script` :file:`t1.ecf` in $HOME/course/test directory

