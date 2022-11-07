.. index::
   single: late attribute (tutorial)

.. _tutorial-late-attribute:

Late attribute
=================

Sometimes tasks don't run as expected, and we want to get a notification when this is the case. For this, we use the late attribute.

A node can only have **one** late attribute. The late attribute **only** applies to a task. You can define it on a Suite/Family in which case it will be inherited. Any late defined lower down the hierarchy will override the aspect(submitted, active, complete) defined higher up.

* -s submitted: The time node can stay submitted (format ``[+]hh:mm``). submitted is always relative, so + is simply ignored, if present. If the node stays submitted longer than the time specified, the late flag is set
* -a Active:      The time of day the node must have become active (format ``hh:mm``). If the node is still queued or submitted, the late flag is set
* -c Complete: The time node must become complete (format ``{+}hh:mm``). If relative, time is taken from the time the node became active, otherwise the node must be complete by the time given.

.. code-block:: shell
   :caption: Late example

   task t1
      late -s +00:15 -a 20:00 -c +02:00

This is interpreted as, the node can stay submitted for a maximum of 15 minutes, and it must become active by 20:00 and the runtime must not exceed 2 hours.

For the purposes of this tutorial, we will add a late attribute for the runtime only.

Ecf Script
-------------

We will add a new :term:`task` /test/f6/t1.
Create new :term:`ecf script` file $HOME/course/test/f6/t1.ecf for which we want to be late.

.. code-block:: bash
   :caption: $HOME/course/test/f6/t1.ecf

   %include <head.h>
   echo "I will now sleep for %SLEEP% seconds"
   sleep %SLEEP%
   %include <tail.h>

Text
-------

Let us modify the :term:`suite definition` file again:

.. code-block:: shell

   # Definition of the suite test.
   suite test
    edit ECF_INCLUDE "$HOME/course"    # replace $HOME with the path to your home directory
    edit ECF_HOME    "$HOME/course"

    family f6
         edit SLEEP 120
         task t1
              late -c +00:01 # set late flag if task take longer than a minute
    endfamily
   endsuite

Python
---------- 

.. literalinclude:: src/late-attribute.py
   :language: python
   :caption: $HOME/course/test.py


**What to do**

#. Type in the changes
#. Replace the :term:`suite definition`
#. Run the suite, you should see the task late flag set in :term:`ecflow_ui`
#. When the job completes, if you re-queue family node f6 or task t1, it will clear the late flag. The late flag can also be cleared manually, select task t1, then with Right Mouse Button > Special > Clear late flag

