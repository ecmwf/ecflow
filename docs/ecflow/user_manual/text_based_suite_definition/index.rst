.. _text_based_suite_definition:

Text based suite definition
////////////////////////////

ecFlow manages suites. A suite is a collection of families, and a
family is a collection of tasks and possibly other families. Tasks
may have events, meters, labels, etc. When it does not matter which
one of the terms suite, family, or task is in question, the generic
term **node** refers to any of them.

By default, suites are independent of each other. If necessary,
suites can have **cross-suite dependencies** . These are best avoided
since ideally, a suite is a self-contained unit.

There is an analogy between a suite definition and the UNIX file
system hierarchy (see :numref:`Table %s<suite_def_analogy_table>`).

.. list-table::  Suite definition analogy with UNIX file system
   :header-rows: 1
   :widths: 20 80
   :name: suite_def_analogy_table

   * - Suite
     - File system
   * - Family
     - Directory
   * - Task
     - File (executable)
   * - Event
     - Signal (not part of the file system)
   * - Meter
     - Numerical signal (not part of the file system)
   * - Label
     - Text signal (not part of the file system)
   * - Dependency
     - Soft link (can span file systems)


Normally a suite is defined using a file, or via the python API.
Suite definitions are best stored in a suite per file.

A suite definition is normally placed in a definition file. Typically
the name for the file is *suitename.def* , but any name may be used.

It is good practice to list all the attributes for families and
suites before any tasks are defined. This makes the reading of the
suite-definition file easier. There are two ways of defining a suite.

- Using the ASCII suite definition file format
- Using the :ref:`python_api`

.. toctree::
    :maxdepth: 1

    suite_definition_file_format/index.rst
    dependencies/index.rst
    attributes/index.rst
