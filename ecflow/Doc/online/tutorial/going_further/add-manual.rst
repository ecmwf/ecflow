.. index::
   single: manual

.. _manual:

Add Manual
==========
  
A :term:`manual page` allows documentation **in** a :term:`ecf script` to be viewable in :term:`ecflowview`.

The manual page is the concatenation of all the text within the %manual and %end :term:`directives`.

Modify :file:`t2.ecf` to have the following: 

::

   %manual
      Manual for task t2
      Operations: if this task fails, set it to complete and report next working day
      Analyst:    Check something ?
   %end

   %include "../head.h" 
   echo "I am part of a suite that lives in %ECF_HOME%" 
   %include "../tail.h" 

   %manual

      There can be multiple manual pages in the same file.
      When viewed they are simply concatenated.
   %end


**What to do:**

#. Modify :file:`$HOME/course/test/t2.ecf` script
#. View the :term:`manual page` for :term:`task` t2 in :term:`ecflowview`
