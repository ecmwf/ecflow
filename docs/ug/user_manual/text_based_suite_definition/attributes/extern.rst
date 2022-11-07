.. _extern:

extern
//////

This allows an external node to be used in a trigger expression. All
nodes in triggers must be known to ecFlow by the end of the **load**
command. No **cross-suite dependencies** are allowed unless the names of tasks outside the
suite are declared as **external**.

An external trigger reference is considered **unknown** if it is not
defined when the trigger is evaluated.

You are strongly advised to **avoid cross-suite dependencies**. Families
and suites that depend on one another should be placed in a single
suite. If you think you need **cross-suite dependencies** , you should consider
merging the suites together and have each as a top-level family in the
merged suite.

To run the task **/b/f/t** when suite ' is not present, use the following trigger, e.g.:

.. code-block:: shell

    extern /a/f/t
    suite b
    family f
        task t
            trigger /a/f/t == complete or /a/f/t == unknown
        