.. index::
   single: limit (tutorial)

.. _tutorial-limit-families:

Limit-families
================== 

The previous exercises on limit/inlimit all affect how tasks are constrained. However, we may also need to constrain how many suites/families are allowed to run in parallel. In the exercise below we will limit families.

When a family is limited, the child tasks are unconstrained. In this case, only **two** families can run at a time. All the tasks in the family can start at once.

Text
------

Let us modify our :term:`suite` definition file:

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


Python
----------

.. literalinclude:: src/limit-families.py
   :language: python
   :caption: $HOME/course/test.py


**What to do**

#. Edit the changes i.e. cp -r f5 lf1; cp -r f5 lf2; cp -r f5 lf3; 
#. Replace the :term:`suite definition`
#. In :term:`ecflow_ui`, observe the effects
#. How could you change suite to also limit tasks as well as families. i.e. only allow 1 task to run in each family?

