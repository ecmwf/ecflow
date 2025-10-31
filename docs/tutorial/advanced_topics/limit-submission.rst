.. index::
   single: limit (tutorial)

.. _tutorial-limit-submission:

Limit (Task Submission)
=======================

The previous section has shown how ecFlow provides simple load management.

Even when using limits, it is still possible to allow several hundred jobs to be submitted at once, which can cause problems:

- Excessive disk/io in job generation
- Server busy in job generation, and slow to respond to the GUI.
- Overload queuing systems like PBS/SLURM

To avoid these issues, limits can also be used to restrict the number of submissions, and in this case, when the Job becomes active the limit token is released.

The following shows the use of :code:`inlimit -s` option, which helps to control the number of tasks in the submitted state.
There can be **more than** 2 active jobs, since **only** the number in the submitted state is controlled.
The removal of the :code:`-s` option would mean that only two active jobs would be allows to execute in parallel.
The use of the :code:`-s`  allows the configuration of the suite, depending on the load the disk/io and queuing system can sustain.

The following is a simple illustration that modifies the previous example.

.. tabs::

    .. tab:: Text

        Modify the :term:`suite definition` file, as follows:

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

    .. tab:: Python

        .. literalinclude:: src/limit-submission.py
           :language: python
           :caption: $HOME/course/test.py

**What to do**

#. Apply the changes to :term:`suite definition`.
#. In :term:`ecflow_ui`, observe the effects on the number of submitted tasks.
#. Change the value of the :term:`limit` and inlimit, and observe the effects.
