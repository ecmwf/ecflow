.. index::
   single: late attribute (tutorial)

.. _tutorial-late-attribute:

Late
====

Sometimes tasks do not complete in an expected time constrait, and the user needs to be notificatied.
This notification can be performed by using :term:`late` attribute.

Each node can only have **one** :term:`late` attribute, and :term:`late` attributes **only** apply to tasks.
When a :term:`late` attribute is defined on a :term:`suite` or :term:`family`, it will be inherited;
and, any :term:`late` attribute defined lower down the hierarchy will override the aspect (submitted, active, complete)
defined higher up.

A :term:`late` attribute defines time constraints for a task to be: :term:`submitted`, :term:`active` or :term:`complete`.

* :code:`-s` submitted: The amount of time the node can stay submitted (format :code:`[+]hh:mm`). The time value is always relative, so :code:`+` is simply ignored, if present. The late flag is set if the node stays submitted longer than the specified amount of time.
* :code:`-a` active: The time of day the node must have become active (format :code:`hh:mm`). The late flag is set if the node is still queued or submitted after the specified time.
* :code:`-c` complete: The amount of time node can take to become complete (format :code:`[+]hh:mm`). If relative, the amount of time that the node can take to complete since becoming active; otherwise, the node must be complete by the specified time.

.. code-block:: shell
   :caption: :code:`late` example

   task t1
      late -s +00:15 -a 20:00 -c +02:00

The :code:`late` in the example above is interpreted as:

* the node can stay submitted for a maximum of 15 minutes
* *and* it must become active by 20:00
* *and* the runtime must not exceed 2 hours.

For the purposes of this tutorial, the late attribute will apply a contraint to the runtime only.

Suite Definition
----------------

.. tabs::

    .. tab:: Text

        Modify the :term:`suite definition` file, as follows:

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

    .. tab:: Python

        .. literalinclude:: src/late-attribute.py
           :language: python
           :caption: $HOME/course/test.py

Task Script
-----------

Create new :term:`task script <ecf script>` file :code:`$HOME/course/test/f6/t1.ecf`, related to the new task.

.. code-block:: bash
   :caption: $HOME/course/test/f6/t1.ecf

   %include <head.h>
   echo "I will now sleep for %SLEEP% seconds"
   sleep %SLEEP%
   %include <tail.h>


**What to do**

#. Apply the changes to :term:`suite definition`.
#. Create the new :term:`task script <ecf script>` file as shown above.
#. In the :term:`ecflow_ui`, run the suite and observer the late flag being set
#. When the job completes, requeue the family :code:`f6` or task :code:`t1`, and observe that this clears the late flag.
#. In the :term:`ecflow_ui`, the late flag can also be cleared manually, by selecting task :code:`t1`, and then in the Right Mouse Button context menu, select *Special*, and *Clear late flag*.
