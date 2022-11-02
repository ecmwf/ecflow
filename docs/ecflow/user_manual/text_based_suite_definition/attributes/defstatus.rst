.. _defstatus:

defstatus
/////////

Defines the default status for a task/family to be assigned to the node
when the **begin(CLI)** command is issued. By default, all nodes get a
**queued** status when you use **begin(CLI)** on a suite.

**defstatus** is useful in preventing suites from running automatically
once begun or in setting tasks **complete** so they can be run
selectively.

For suites anything other than **suspended** is void. The status of a
family reflects the status of its children.

For a family defstatus can be either " **suspended"** , " **queued"** or
" **complete"** . A family with **defstatus complete** means that all
tasks and families are marked complete without running them.

.. code-block:: shell

    family f
        task t1
        task t2
            defstatus complete # by default will not be run
        task t3
            defstatus suspended # needs 2 B resumed