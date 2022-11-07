.. index::
   single: limit (tutorial)

.. _tutorial-limit-submission:

Limit-submission
================== 

In the previous exercise, we showed how ecflow provides simple load management.

The limits can still allow several hundred jobs to be submitted at once. This can cause problems:

- Excessive disk/io in job generation
- Server busy in job generation, and slow to respond to the GUI.
- Overload queuing systems like PBS/SLURM

Hence we need load management that can limit the number of submissions. When the Job becomes active the limit token is released.

This exercise shows the use of '-s' option. Which helps to control the number of tasks in the submitted state. We can have **more than** 2 active jobs since we **only** control the number in the submitted state. If we **removed** the -s then we can only have two active jobs running at one time. The use of -s  allows the configuration of the suite, depending on the load the disk/io and queuing system can sustain.

Here is the simple illustration that modifies the previous example:


Text
-------------

Let us modify our :term:`suite definition` file:

.. code-block:: shell

   # Definition of the suite test.
   suite test
    edit ECF_INCLUDE "$HOME/course"
    edit ECF_HOME    "$HOME/course"
    limit l1 2
    family f5
        inlimit -s l1 # by default consume 1 token from the limit l1
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

.. literalinclude:: src/limit-submission.py
   :language: python
   :caption: $HOME/course/test.py


**What to do**

#. Edit the changes
#. Replace the :term:`suite definition`
#. In :term:`ecflow_ui`, observe effects
#. Change the value of the :term:`limit` and inlimit, observe the effect.


