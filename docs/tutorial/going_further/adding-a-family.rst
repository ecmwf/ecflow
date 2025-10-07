.. index::
   single: families (tutorial)

.. _tutorial-families:

Adding a family
===============

:term:`Tasks <task>` can be logically grouped into :term:`families <family>`.
This section will describe how to group the previously designed tasks :code:`t1` and
:code:`t2` in a new :term:`family`.

You can consider that a :term`suite` is an hierarchical structure, very similar to a _Unix_
file system, where :term:`families <family>` are the directories and the :term:`tasks <task>` are regular files.
In this metaphor, the :term:`suite` is a *special* family with extra attributes (see :ref:`tutorial-dates-and-clocks`).
Families can themselves contain other families and, like directories/files, there can be many tasks
with the same name as long as they are in different families.

.. note::

   In ecFlow nomenclature, :term:`suites <suite>`, :term:`families <family>` and :term:`tasks <task>` are called :term:`nodes <node>`.

The default ecFlow behaviour, regarding finding task specific files (e.g. :term:`ecf script` file),
is to expect the file location to be a reflection of the structure of the suite.
For example, if a task is located at :code:`/test/f1/t1` (i.e. task :code:`t1` is inside family
:code:`f1`, inside suite :code:`test`), ecFlow will expect the related :term:`ecf script` file
to be found at :code:`{BASE}/test/f1/t1.ecf` -- where :code:`{BASE}`, by default, is the
:term:`ecflow_server` current working directory. The :term:`job file` and job output will
also be created in this directory.

The following steps are required to create a new family :code:`f1`:

#. Update the task scripts
#. Create a directory structure to reflect the new family
#. Update the :term:`suite definition`


Update Task Scripts
-------------------

Since the task scripts need to be moved to another directory, to match the location in the family,
the paths in the :code:`%include` directories need to be adjusted.

An alternative to adjusting the relative paths to the include files,
is to make use of the special ecFlow :term:`variable` named :code:`ECF_INCLUDE`.
This variable points to a directory used during :term:`pre-processing` to locate include files.

When angle brackets are used (i.e. :code:`%include <>`), ecFlow checks if the :code:`ECF_INCLUDE` variable is
specified and if a matching include file exists in it to be used. If an include file is not found
using the :code:`ECF_INCLUDE` variable, it falls back to using the :code:`ECF_HOME` variable.
This has the added advantage that specific includes files can be placed under :code:`ECF_INCLUDE`,
and include files common to *many* tasks can placed in :code:`ECF_HOME` -- for more details see :term:`directives`.

This means that task scripts using :code:`%include <>` can remain unchanged, as long as the :code:`ECF_INCLUDE`
variable is set to point to the directory where the include files can be located.

Update the task scripts to be as follows (notide the use of :code:`<>`):

.. code-block:: shell

   %include <head.h>
   echo "I am part of a suite that lives in %ECF_HOME%"
   %include <tail.h>


Create Directory Structure
--------------------------

As mentioned before, having a family :code:`f1`, implies creating a directory :file:`{{HOME}}/course/test/f1`,
and moving existing :file:`t1.ecf` and :file:`t2.ecf` files into it.


Update Suite Definition
-----------------------

The :term:`suite definition` needs to be updated to create a family :code:`f1` with two tasks :code:`t1` and :code:`t2`.
The :code:`ECF_INCLUDE` variable also needs to be set to point to the directory where the include files can be found.

.. tabs::

    .. tab:: Text

        Update the :term:`suite definition` to be as follows:

        .. code-block:: shell
           :caption: $HOME/course/test.def

           # Definition of the suite test.
           suite test
              edit ECF_INCLUDE "{{HOME}}/course"
              edit ECF_HOME    "{{HOME}}/course"
              family f1
                 task t1
                 task t2
              endfamily
           endsuite

    .. tab:: Python

        The following script generates updated :term:`suite definition` using the :ref:`python_api`:

        .. literalinclude:: src/families.py
           :language: python
           :caption: $HOME/course/test.py

Once loaded into the :term:`ecflow_server`, the updated :term:`suite definition` hierarchy is shown as a tree in :term:`ecflow_ui`.

**What to do:**

#. Edit the task scripts to use :code:`%include<>`
#. Create the directory struture reflecting the family :code:`f1`, and move the task scripts into it.
#. Update the :term:`suite definition` file, to include family :code:`f1` with two tasks :code:`t1` and :code:`t2`.
#. Replace the :term:`suite` in the :term:`ecflow_server` with the new definition.
#. Inspect and run the suite in :term:`ecflow_ui`, notice the tree structure.
   You may have to expand :code:`test` and :code:`f1` nodes to observe the tasks.
