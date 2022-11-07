.. _checking_the_suite_definition:

Checking the suite definition
///////////////////////////////

The following python code shows how to check expression and limits.

Checking existing definition file that has been saved as a file:

.. code-block:: python
   
    def = ecflow.Defs("/my/path.def")  # will load file '/my/path.def'  
    print(def.check())  # check trigger expressions and limits 

Here is another example where we create the suite definition on the fly.
In fact, using the python API allows for a correct by construction
paradigm.

.. code-block:: python

    defs = ecflow.Defs()  # create a empy defs
    suite = defs.add_suite("s1")  # create a suite 's1' and add to defs
    task = suite.add_task("t1")  # create a task 't1' and add to suite
    task.add_trigger("t2 == active)")  # mismatched brackets
    result = defs.check()  # check trigger expressions and limits
    print("Message: '" + result + "'")
    assert len(result) != 0, "Expected Error: mis-matched brackets in expression."

