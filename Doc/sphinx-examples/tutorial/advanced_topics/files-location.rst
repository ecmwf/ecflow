.. index::
   single: file location
   single: ECF_INCLUDE
   single: ECF_FILES
   single: ECF_OUT
   single: script management

.. _file-location:

File location
=============

| We have seen so far that :term:`ecflow_server` is looking for the files it needs in specific locations. 
| You can control the location of your files by using the following :term:`variable`'s

* ECF_INCLUDE  Is where :term:`ecflow_server` will look for include files.
* ECF_FILES    Is where :term:`ecflow_server` will look for the :term:`ecf script` s if they are not at their default location.
* ECF_OUT      Is where the job output files will go.

| If two tasks use the same :term:`ecf script`, and are simply using different values of the same :term:`variable`'s, 
| you do not want to maintain several copies of the same file. 
| You can use the same script in multiple places within your :term:`suite` and using the same name 
| by keeping the script into a common directory and pointing to this location using the :term:`variable` ECF_FILES. 
| Many users use just one directory for their scripts and point to this directory with ECF_FILES.

If the tasks have different names, you can use the unix command :command:`ln -s` to create several names for the same file.


**What to do:**

1. Try to imagine how we could use ECF_FILES and **ln** to reduce the number of scripts in our example suite