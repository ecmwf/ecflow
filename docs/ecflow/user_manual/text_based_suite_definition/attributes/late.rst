.. _late:

late
////

Define a tag for a node to be **late** . Suites cannot be late, but
you can define a late tag for submitted in a suite, to be inherited
by the families and tasks. When a node is classified as being late,
the only action ecFlow takes is to set a flag. ecflow_ui will display
these alongside the node name as an icon (and optionally pop up a
window). A separate list is also kept.

.. list-table:: Ways a node can be late
   :header-rows: 1

   * - Status
     - Reason
   * - Submitted
     - The time node can stay submitted (format [+]hh:mm). Submitted is always relative, so + is simply ignored, if present. If node stays submitted longer than the time specified, the late flag is set.
   * - Active
     - The time of day the node must have become active (format hh:mm) If the node is still queued or submitted by the time, late flag is set.
   * - Complete
     - The time node must become complete (format [+]hh:mm). If relative, time is taken from the time node became active, otherwise node must be complete by the time given.


The submitted late time is inherited from parent if not present in 
the node itself. That is defining **late -s** on a suite all tasks 
will have that value:

.. code-block:: shell

   task t1
      late -s +00:15 -a 20:00 -c +02:00

This is interpreted as: the node can stay submitted for a maximum of
15 minutes, and it must become active by 20:00 and the runtime must
not exceed 2 hours.
