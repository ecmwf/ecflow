.. _end_suite_family_task:

end[suite, family, task]
//////////////////////////

endsuite
===========

This terminates a suite definition. **endsuite** is also an implicit **endfamily/endtask** for all families/tasks currently being defined. 

.. code-block::

    suite obs
        task t1
    endsuite

endfamily
============

This terminates a family definition. You must use **endfamily** to
terminate the current family in order to start a new family definition
at the same level.

Typically **endfamily** is followed by a **task** , a **family** or an
**endsuite** command.

endtask
=========

This terminates a task definition. This is not strictly speaking needed,
unless you want to add properties to the family containing the task or
if you are using automatic generation to setup a suite. The example below highlights the
use of **endtask**

.. code-block:: shell

    family f
     task t1
     task t2
        edit ECFHOST c90
     endtask
     edit ECFHOST ymp8 # This variable is for family f!
    endfamily


The following achieves the same effect and is clearer:

.. code-block:: shell

    family f
     edit ECFHOST ymp8
     task t1
     task t2
        edit ECFHOST c90
    endfamily
