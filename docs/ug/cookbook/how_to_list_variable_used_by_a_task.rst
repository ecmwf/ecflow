.. _how_to_list_variable_used_by_a_task:

How to list variable used by a task?
************************************
 
**Example 1**: I need to list all variable accessible by all task's called 'fred' under a given path: /suite/main

  .. code-block:: shell

    ./list_variables.py --host my_host --port 4141 --path /suite/main --task fred

**Example 2**: I need to validate that variable is of a given value, for all tasks of a given name. Here I need to list all tasks where the variable value is NOT 12 for a variable of name PGNODES, for all tasks of name prodgen under suite /emc

  .. code-block:: shell

    ./list_variables.py --host machine1 --port 43333 --path /emc --task prodgen --var_name PGNODES --not_value 12  

.. literalinclude:: src/list_variables.py
   :language: python

