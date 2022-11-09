.. index::
   single: Defining a new suite (tutorial)
   single: ECF_HOME (tutorial)
    
.. _tutorial-defining-a-suite:

Defining a new suite
====================

| There are several ways of defining the :term:`suite definition`.
| This tutorial will give examples for both the plain text and Python methods.

Text Method
-----------
 
Create a file called :file:`test.def`,using your favourite text editor, with the following contents:

.. code-block:: shell
   :caption: $HOME/course/test.def
 
   suite test
      edit ECF_HOME "$HOME/course"  # replace '$HOME' with the path to your home directory
      task t1
   endsuite

| This file contains the :term:`suite definition` of a :term:`suite` called test. 
| This suite contains a single :term:`task` called **t1**. 
| Let us go through the lines one by one:

1. This line is a comment line. Any characters between the # and the end of line are ignored
2. The second line is empty 
3. This line defines a new :term:`suite` by the name of test.  
4. Here we define a ecFlow :term:`variable` called ECF_HOME.
   This :term:`variable` defines the directory where all the unix files that will be used by the :term:`suite` test will reside.
   For the rest of the course all file names will be given relative to this directory.  
   
   .. warning::

      Be sure to **replace** $HOME with the path to your home directory

5. This defines a :term:`task` named **t1**
6. The :token:`endsuite` finishes the definition of the :term:`suite` test 


Python Method
-------------

Enter the following python code into a file, i.e :file:`test.py` :

.. literalinclude:: src/defining-a-new_suite.py
   :language: python
   :caption: $HOME/course/test.py
   
Then run as a python script:

.. code-block:: shell

   python test.py
   
Alternatively add the following as the first line in test.py:

.. code-block:: python
   :caption:  $HOME/course/test.py

   #!/usr/bin/env python3           
   ...   

.. code-block:: shell

   chmod +x test.py
   ./test.py  # this uses shebang, see below, searches for specified python variant in $PATH   

You should see the text "Creating suite definition" and then your definition as your output.


**What to do:**

#. Initially try both plain text and python examples. Later examples are only in python.
#. Type in the :term:`suite definition` file.  
#. Choose python invocation.  i.e. ``python3 test.py``  |  ``./test.py``.   
