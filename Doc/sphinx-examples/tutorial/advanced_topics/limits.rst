.. index::
   single: limits
   single: inlimit

.. _limits:

Limits
======

| Limits provide simple load management by limiting the number of tasks
| submitted by a specific :term:`ecflow_server`
      
| We have learnt from experience that suite designers were using :term:`trigger`'s in 
| two different ways: as data dependency triggers and as courtesy triggers. 
| Triggers where designed for the former. The latter are used to prevent too 
| many jobs running at once and are actually an artificial way of queueing jobs. 

| Because ecFlow does not distinguish between the two sorts of triggers, suites can become 
| difficult to maintain after a while. So the concept of :term:`limit` was introduced. 
| Limits are declared with the **limit** keyword 

.. _inlimit:

inlimit
-------

| Limits are used in conjunction with :term:`inlimit` keyword.

| First, a :term:`limit` must be defined using the 'limit NAME N'.
| The limit definition is typically placed at the :term:`suite` scope.

| Next we create a group of tasks to which we want to apply the limit.
| This is done by attaching an 'inlimit NAME' attribute to the nodes.
| Attaching the attribute to a :term:`task` adds the task to the group.
| Attaching it to a :term:`family` adds all tasks from that :term:`family`.

| The effect of a :term:`limit` is that no more than N tasks
| of a group will run at once.

| A :term:`node` can be limited by several limits.

Ecf script
----------

| We will create :term:`family` f5 with nine tasks. 
| Create new :term:`ecf script` s in :file:`$HOME/course/test/f5/` directory, each one containing:

::

   %include <head.h>
   echo "I will now sleep for %SLEEP% seconds"
   sleep %SLEEP%
   %include <tail.h>

Text
----

Let us modify our :term:`suite definition` file::

   # Definition of the suite test.
   suite test
    edit ECF_INCLUDE "$HOME/course"
    edit ECF_HOME    "$HOME/course"
    limit l1 2 
  
    family f5 
        inlimit l1 
        edit SLEEP 20 
        task t1 
        task t2 
        task t3 
        task t4 
        task t5 
        task t6 
        task t7 
        task t8 
        task t9 
    endfamily
   endsuite
   
Python
------

.. literalinclude:: src/limits.py


**What to do:**

1. Edit the changes
2. Replace the :term:`suite definition`
3. In :term:`ecflowview`, observe the triggers of the :term:`limit` **l1**
4. Open the Info panel for **l1**
5. Change the value of the :term:`limit`
6. Open the Why? panel for one of the :term:`queued` tasks of **/test/f5**
