.. _using_meters_in_triggers:

Using meters in triggers
////////////////////////

The purpose of a meter is to signal proportional completion of a task
and to be able to trigger another job that is waiting for this
proportional completion. Let us say that the task "model" creates a
hundred files, and there are ten other tasks to process these files.
Task "t0" processes files 0-9, task "t1" files 10-19, and so on. The
python API would look something like:

.. code-block:: shell

    suite = Suite("x")
    f = suite.add_family("f")
    task_model = f.add_task("model")
    task_model.add_meter("file",0,100,100)
    for i in range(0,9):
        file = i*10 + 10
        t = f.add_task("t" + str)
        t.add_trigger("model:file ge " + str(file))
    