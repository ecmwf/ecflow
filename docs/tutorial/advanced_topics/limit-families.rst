.. index::
   single: limit (tutorial)

.. _tutorial-limit-families:

Limit (Families)
================

The previous section has shown how to apply :code:`limit`/:code:`inlimit` to constraint task execution/submission.

This section shows how to apply similar constrains to how many suites/families are allowed to run in parallel.
When a family is limited, the contained tasks are unconstrained. In the following eample, while only **two** families
can run at a time, all the tasks in the family are allowed to start at once.

.. tabs::

    .. tab:: Text

        Modify the :term:`suite definition` file, as follows:

        .. code-block:: shell

           # Definition of the suite test.
           suite test
            edit ECF_INCLUDE "$HOME/course"
            edit ECF_HOME    "$HOME/course"
            edit SLEEP 20
            limit fam 2
            family lf1
                inlimit -n fam
                task t1 ;  task t2 ; task t3 ; task t4; task t5 ; task t6; task t7; task t8 ; task t9
            endfamily
            family lf2
                inlimit -n fam
                task t1 ;  task t2 ; task t3 ; task t4; task t5 ; task t6; task t7; task t8 ; task t9
            endfamily
            family lf3
                inlimit -n fam
                task t1 ;  task t2 ; task t3 ; task t4; task t5 ; task t6; task t7; task t8 ; task t9
            endfamily
           endsuite


    .. tab:: Python

        .. literalinclude:: src/limit-families.py
           :language: python
           :caption: $HOME/course/test.py


**What to do**

#. Apply the changes to :term:`suite definition`.
#. Copy the task script from the previous section example into the new families directories.
#. In :term:`ecflow_ui`, observe the effects on the number of active families/submitted tasks.
#. How could the suive be updated to limit tasks as well as families (i.e. only allow 1 task to run in each family)?
