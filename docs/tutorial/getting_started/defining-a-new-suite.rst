.. index::
   single: Defining a new suite (tutorial)
   single: ECF_HOME (tutorial)
    
.. _tutorial-defining-a-suite:

Defining a new suite
====================

Designing a new workflow encompases two closely tied steps:

    1. Define the :term:`suite`, which will eventually be composed of :term:`tasks <task>`, potentially organized in :term:`families <family>`. The outcome of this step is the :term:`suite definition` file.
    2. Create the :term:`task` scripts, that will eventually become :term:`job file` to be executed.

This section describes how to perform first of these steps.

There are several approaches to defining the :term:`suite definition` - the two most common are:

    1. Using a plain text file, with a specific syntax.
    2. Using the ecFlow Python API.

This tutorial will give examples for both the plain text and Python methods.

.. tabs::

   .. tab:: Text

      A test-based :term:`suite definition` is just a plain file, that uses a specific syntax to define the
      components of a suite (or sets of suites).

      To define a simple :term:`suite`, use a text editor to create a file named :code:`test.def`, with the following contents:

      .. code-block:: shell
         :linenos:
         :caption: $HOME/course/test.def

         # A simple example suite

         suite test
           edit ECF_HOME "{{HOME}}/course"
           task t1
         endsuite

      .. warning::

         Please **replace** :code:`{{HOME}}` so that path :code:`{{HOME}}/course` is correct and exists.

      This file contains the :term:`suite definition` of a :term:`suite` named :code:`test`.
      The suite :code:`test` contains a single :term:`task` called :code:`t1`.
      A line by line explanation of the file above is as follows:

      * Line :code:`1` is a comment line. All characters after the symbol :code:`#` ignored.
      * Line :code:`2` is empty.
      * Line :code:`3` defines a :term:`suite` named :code:`test`.
      * Line :code:`4` defines a :term:`variable` called :code:`ECF_HOME`, which defines the directory
        where all the files that will be used by the :term:`suite` test will reside.
      * Line :code:`5` defines a :term:`task` named :code:`t1`
      * Line :code:`6` indicates the end of the :term:`suite definition`.

      .. important::

         This tutorial will consider the :code:`{{HOME}}/course` path throughout, and all other
         file paths will be relative to this base path.

   .. tab:: Python

      Using the ecFlow Python API allows to automate the creation of the :term:`suite definition` file,
      by leveraing the expressiveness of the Python language to create complex :term:`suites <suite>`.

      .. important::

            Even though the plain text method is the most straightforward, it is also limited.
            The use of the Python API is more powerful and flexible, and is the recommended approach for
            defining complex :term:`suites <suite>`.

      To define a simple :term:`suite`, use a text editor to create a Python script named :code:`test.py`, with the following contents:

      .. literalinclude:: src/defining-a-new_suite.py
         :language: python
         :linenos:
         :caption: $HOME/course/test.py

      And then run the Python script:

      .. code-block:: shell

         cd $HOME/course

         # Either run by explicitly invoking python
         python3 ./test.py

         # Or make the script executable, and run it directly
         chmod +x test.py
         ./test.py

      .. warning::

         Ensure ecFlow Python API module is available by including it in the Python module search path.
         This can be done by setting the :code:`PYTHONPATH` environment variable, e.g.:

         .. code-block:: shell

            export PYTHONPATH=$ECFLOW_ROOT_DIR/lib/pyext3:$PYTHONPATH


      Once finished executing, the script will print the :term:`suite` :code:`test` definition on the console
      and also generate the :code:`$HOME/course/test.def`.


**What to do:**

#. Manually create the text-based :term:`suite definition` file, placing the file at :code:`$HOME/course/test.def`.
#. (Optional) Use the Python-based approach to create :term:`suite definition` file.
