.. _variable_inheritance:

Variable inheritance
//////////////////////

When a variable is needed at submission time, it is first sought in
the task itself. If it is not found in the task, it is sought from
the task's parent and so on, up through the node levels until found.
For any node, there are two places to look for variables. The
user-defined variables are looked for first, and then any generated
variables.

At present, an **undefined variable** causes a task to *abort*
immediately, without submitting a job. ecflow_ui will display this
failure in the info-window.

.. code-block:: shell
   :caption: Example of Variable inheritance

   suite x
      edit TOPLEVEL 10
      edit MIDDLE 10
      edit LOWER 10
      family f
         edit MIDDLE 20
         task t
            edit LOWER abc
         task t2
      endfamily
      family f2
         edit TOPLEVEL 40
         task z
      endfamily
   endsuite


This would produce the following results:

.. list-table::
   :header-rows: 1
  
   * - Command
     - task t
     - task t2
     - task z
   * - echo TOPLEVEL %TOPLEVEL%
     - 10
     - 10
     - 40
   * - echo MIDDLE %MIDDLE%
     - 20
     - 20
     - 10
   * - echo LOWER %LOWER%
     - abc
     - 10
     - 10

