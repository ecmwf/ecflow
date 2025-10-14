.. index::
   single: embedded triggers (tutorial)

.. _tutorial-embedded-triggers:  
   
Using embedded triggers
=======================

This section provides a exercise showing the use of embedded triggers.
These are trigger expressions embedded within the task scrips using the :code:`--wait` command.
Whilst the expression is **not** true, the job will hold i.e. continue to wait.

Whenever possible, giving preference to triggers on the :term:`suite definition` is considered
a better approach, as this allows extra validation upon creation, whereas embedded triggers are
checked at run time.

Update Task Script
------------------

Consider the :term:`task` :code:`t2.ecf` script as follows:

.. code-block:: shell
    :caption: $HOME/course/test/f1/t2.ecf

    %include <head.h>
    ecflow_client --wait="t1 == complete" # wait for expression to become true
    %include <tail.h>

Update Suite Definition
-----------------------

Consider the following :term:`suite definition`, with no :term:`trigger` on task **t2**.

.. tabs::

    .. tab:: Text

        .. code-block:: shell

            # Definition of the suite test.
            suite test
                edit ECF_INCLUDE "{{HOME}}/course" # replace '{{HOME}}' appropriately
                edit ECF_HOME    "{{HOME}}/course"
                family f1
                    edit SLEEP 20
                    task t1
                    task t2
                endfamily
            endsuite

    .. tab:: Python

        .. literalinclude:: src/embedded-triggers.py
           :language: python
           :caption: $HOME/course/test.py

**What to do**

#. Modify the task script and the suite definition as shown above.
#. Replace the :term:`suite`, using:

   .. tabs::

      .. tab:: Text

         .. code-block:: shell

            ecflow_client --suspend /test
            ecflow_client --replace /test test.def

      .. tab:: Python

         .. code-block:: shell

            python3 test.py
            python3 client.py

#. Observe the tasks in :term:`ecflow_ui`.
#. Notice the wait icon on task t2.
#. Introduce an error in the wait expression and requeue the suite. Observe how the job now aborts:

   .. code-block:: shell
      :caption: Introduce error in wait expression

      ecflow_client --wait="t == complete"  # Error: no node with name `t`

#. Introduce an impossible expression in the wait expression and requeue the suite. What is the effect?

   .. code-block:: shell
      :caption: expression that will never be satisfied

      ecflow_client --wait="1 == 0"
