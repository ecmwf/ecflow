.. index::
   single: Variable inheritance (tutorial)

.. _tutorial-variable-inheritance:   
   
Variable inheritance
=====================

The previous section describes how to define a :term:`variable` for a particular :term:`task`.
When all the tasks in a given :term:`family` share the same variable value,
the value can be defined at the family level. This shared value is then inherited by all the
child nodes of the family -- this is :term:`variable inheritance`.

Variables are inherited from the closest parent node. This means that, going from top to bottom,
if a variable is redefined lower in the tree, it is said to override the previously defined variable.

Defining the variables is an important part of :term:`suite` design, considering that any kind
of variable can be overridden, including generated variables. Opting to override a variable,
in particular generated variables, might result in undesired effects (e.g. overridding :code:`ECF_HOST`
or :code:`ECF_PORT` could prevent the tasks from contacting back the :term:`ecflow_server`).

Update Suite Definition
-----------------------

Consider the following :term:`suite definition`, where the variable :code:`SLEEP` is defined at the level of the family :code:`f1`.

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

        .. note::

            The :term:`variable` :code:`SLEEP`, in the example above, could have been defined at the
            level of the :term:`suite`, achieving the same results.

    .. tab:: Python

        .. literalinclude:: src/variable-inheritance.py
           :language: python
           :caption: $HOME/course/test.py

Quiz
----

Considering the following suite, which includes several levels of families and tasks,
determine the value of the variable :code:`SLEEP` for each task.

.. code-block:: shell

   suite test
      edit SLEEP 100
      family f1
         edit SLEEP 80
         task t1
         task t2
            edit SLEEP 9
         family g1
             edit SLEEP 89
             task x1
                 edit SLEEP 10
             task x2
         endfamily
      endfamily
      family f2
        task t1
        task t2
            edit SLEEP 77
        family g2
             task x1
                 edit SLEEP 12
             task x2
         endfamily
      endfamily
   endsuite

Compare the value of the :code:`SLEEP` used by each of the tasks in the following table:

   ==============  ======
   :term:`node`    SLEEP
   ==============  ======
   /test/f1/t1        80
   /test/f1/t2         9
   /test/f1/g1/x1     10
   /test/f1/g1/x2     89
   /test/f2/t1       100
   /test/f2/t2        77
   /test/f2/g2/x1     12 
   /test/f2/g2/x2    100
   ==============  ======

**What to do**

#. Modify the suite definition to include the variable :code:`SLEEP`, as shown above.
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

#. Observe the task execution in :term:`ecflow_ui`.
