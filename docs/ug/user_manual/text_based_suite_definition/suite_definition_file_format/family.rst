.. _family:

family
//////

The **family** keyword is used to create a new family inside a suite or
inside another family. The only implicit action is to terminate the
previous task definition.

The only parameter this keyword takes is the name of the new family to
be defined.

The definition of a family must be terminated by either **endfamily** or
**endsuite** or by the end of the suite definition file.

A family is used to collect tasks together or to group other families.
Typically you place tasks that are related to each other inside the same
family, analogous to the way you create directories to contain related
files.

Sometimes it is useful to group a set of tasks into a family to get the
trigger dependencies cleaner, e.g. you might have ten tasks that all
need to be complete before the eleventh task can run, as in the
following definition file.

.. code-block:: shell

    family f
        task t0
        family ff
            task t1
            ...
            task t10
        endfamily
        task t11
            trigger ff==complete
    endfamily

