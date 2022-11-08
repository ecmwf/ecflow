ecflow.Autoarchive
//////////////////


.. py:class:: Autoarchive
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Provides a way to automatically archive a suite/family which has completed or is *idle*

This is required when dealing with super large suite/families, they can be archived off, and then restored later.
This reduces the time to checkpoint and network band width.
The archived node is written to disk, as ECF_HOME/<host>.<port>.ECF_NAME.check,
where '/' is replaced with ':' in ECF_NAME.
The node can be recovered using 'autorestore', begin,re-queue and manually via ecflow_client --restore.
The archiving is never immediate. The nodes are checked one a minute, and expired autoarchive nodes are archived.
If the node is suspended or any of its parent are suspended,or previously restored then archive does not happen.

Constructor::

   Autoarchive(TimeSlot,relative,idle)
      TimeSlot     : A time
      bool relative: Relative to node state, False means archive the node at the real time specified.

      bool idle:     True includes queued,aborted and complete, False means archive after completion only

   Autoarchive(hour,minute,relative)
      int hour:      hour in 24 hrs
      int minute:    minute <= 59
      bool relative: Relative to node state. False means archive the node at the real time specified.

      bool idle:     True includes queued,aborted and complete, False means archive after completion only

   AutoArchive(days,idle)
      int days:      archive the node after 'days' elapsed 
      bool idle:     True archive if node queued,aborted and complete, False means archive after completion only


Usage:

.. code-block:: python

   attr = Autoarchive( 1,30, true )              # archive node 1 hour and 30 minutes after completion
   attr = Autoarchive( TimeSlot(0,10), true )    # archive node 10 minutes after completion
   attr = Autoarchive( TimeSlot(10,10), false )  # archive node at 10:10 after completion
   attr = Autoarchive( 3  )                      # archive node 3 days after completion
   attr = Autoarchive( 3,true  )                 # archive node 3 days after complete,queued or aborted, i.e node is idle


.. py:method:: Autoarchive.days( (Autoarchive)arg1) -> bool :
   :module: ecflow

Returns a boolean true if time was specified in days


.. py:method:: Autoarchive.idle( (Autoarchive)arg1) -> bool :
   :module: ecflow

Returns a boolean true if archiving when idle, i.e queued,aborted,complete and time elapsed


.. py:method:: Autoarchive.relative( (Autoarchive)arg1) -> bool :
   :module: ecflow

Returns a boolean where true means the time is relative


.. py:method:: Autoarchive.time( (Autoarchive)arg1) -> TimeSlot :
   :module: ecflow

returns archive time as a TimeSlot

