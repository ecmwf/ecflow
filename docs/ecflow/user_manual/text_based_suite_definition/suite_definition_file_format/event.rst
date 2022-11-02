.. _event:

event
/////

The event keyword assigns an event to the task currently being
defined. Only tasks can have events and they can be considered as an
attribute of a task. There can be many events and they are displayed
as nodes.

An event has a number and possibly a name. If it is only defined as a
number, its name is the text representation of the number without
leading zeroes. For example, event 007 can be accessed as event 7 or
as event 007. Event numbers must be positive and their name can
contain only letters and digits. The use of letters is optional; the
event name can consist simply of digits.

If a name is given, you can only refer to the event by its name, not
by number. As such there is no point in using a number and a name
unless they are the same:

.. code-block:: shell

   task x
    event 1 # Can only be referred to as x:1
    event 2 prepok # Can only be referred to as x:prepok
    event 3 99 # This is asking for trouble!


When a job sends an event, it is saying that part of its task has
been carried out and that any task waiting for that part can now
start, unless it also needs other conditions to be met. If the job
then aborts and the task is resubmitted, the restarted job should be
able to carry on from where it previously left off. Otherwise, there
is the possibility of destroying the information needed by the task
triggered by the event.

In order to use events you have to first define the event in the
suite definition file, e.g.

.. code-block:: shell

   suite x
      family f
         task t
            event foo


Where 'foo' is the name of the event. The default value for the event
is "clear" or false (the value is shown when the suite begins). After
(command begin) it looks like:

.. image:: /_static/event/image1.png
   :width: 2.08333in
   :height: 0.33333in

Then you can modify your task to change this event while the job is running, e.g.

.. code-block:: shell

   ecflow_client --init=$$
   ecflow_client --event=foo
   ecflow_client --complete  

After the job has modified the event it looks like:

.. image:: /_static/event/image2.png
   :width: 2.08333in
   :height: 0.33333in

Now the value of the event is "set" or true.

Using Events in Triggers
============================

The purpose of an event is to signal partial completion of a task
and to be able to trigger another job that is waiting for this
partial completion. Task "t1" creates a file, sets an event, and
saves the file (which might take a long time.) Another task, "t2"
only needs the on-line copy of the file so it can start as soon as
the file is made, e.g.

.. code-block:: shell

   suite x
     family f
       task t1
         event foo
       task t2
         trigger t1:foo == set


The "== set" part is optional since the value of the event is Boolean
anyway.

The event keyword assigns an event to the task currently being
defined. Only tasks can have events and they can be considered as an
attribute of a task. There can be many events and they are displayed
as nodes.

An event has a number and possibly a name. If it is only defined as a
number, its name is the text representation of the number without
leading zeroes. For example, event 007 can be accessed as event 7 or
as event 007. Event numbers must be positive and their name can
contain only letters and digits. The use of letters is optional; the
event name can consist simply of digits.

If a name is given, you can only refer to the event by its name, not
by number. As such there is no point in using a number and a name
unless they are the same:

.. code-block:: shell

   task x
     event 1 # Can only be referred to as x:1
     event 2 prepok # Can only be referred to as x:prepok
     event 3 99 # This is asking for trouble!

When a job sends an event, it is saying that part of its task has
been carried out and that any task waiting for that part can now
start, unless it also needs other conditions to be met. If the job
then aborts and the task is resubmitted, the restarted job should be
able to carry on from where it previously left off. Otherwise, there
is the possibility of destroying the information needed by the task
triggered by the event.

In order to use events you have to first define the event in the
suite definition file, e.g.

.. code-block:: shell

   suite x
      family f
         task t
            event foo


Where 'foo' is the name of the event. The default value for the event
is "clear" or false (the value is shown when the suite begins). After
(command begin) it looks like:

.. image:: /_static/event/image1.png
   :width: 2.08333in
   :height: 0.33333in

Then you can modify your task to change this event while the job is
running, e.g.

.. code-block:: shell

   ecflow_client --init=$$
   ecflow_client --event=foo
   ecflow_client --complete

After the job has modified the event it looks like:

.. image:: /_static/event/image2.png
   :width: 2.08333in
   :height: 0.33333in

Now the value of the event is "set" or true.

Using Events in Triggers
=============================

The purpose of an event is to signal partial completion of a task
and to be able to trigger another job that is waiting for this
partial completion. Task "t1" creates a file, sets an event, and
saves the file (which might take a long time.) Another task, "t2"
only needs the on-line copy of the file so it can start as soon as
the file is made, e.g.

.. code-block:: shell

   suite x
      family f
         task t1
            event foo
         task t2
            trigger t1:foo == set

The "== set" part is optional since the value of the event is Boolean
anyway.
