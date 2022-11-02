.. _suite:

suite
/////

The **suite** keyword is used to start a new suite definition. The
standard practice is to have one suite defined in a definition file.
However, any number is possible.

The only parameter the **suite** command takes is the name of the new
suite to be defined. After this command, all other commands define
something in the suite.

Currently, there cannot be dependencies at the suite level.

A suite is a collection of families, variables, repeat, and clock
definitions.

A suite is the only component that can be started using **begin(CLI)**

.. code-block:: shell

    suite x
      clock hybrid
      edit ECF_HOME "/some/other/dir"
      family f
      ...
    endsuite
