.. index::
   single: python
  
.. _using-python-scripting:

Using python scripting
======================

As you have already seen, ecFlow has a :ref:`python_api`:

.. literalinclude:: src/using-python-scripting.py
   :lines: 1-2

| This allows the :term:`suite definition` to be built with python. 
| It also allows communication with the :term:`ecflow_server`.  

| This is a very powerful feature, that helps to define very complex suites in a relatively compact way. 
| Consider the following :term:`suite`

::

   suite test
    family f1
        task a
        task b
        task c
        task d
        task e
    endfamily
    family f2
        task a
        task b
        task c
        task d
        task e
    endfamily
    family f3
        task a
        task b
        task c
        task d
        task e
    endfamily
    family f4
        task a
        task b
        task c
        task d
        task e
    endfamily
    family f5
        task a
        task b
        task c
        task d
        task e
    endfamily
    family f6
        task a
        task b
        task c
        task d
        task e
    endfamily
   endsuite
                
                
This can be written in python as:

.. literalinclude:: src/using-python-scripting.py
   :lines: 4-11

         
| Python variables can be used to generate :term:`trigger` :term:`dependencies`. 
| Imagine that we want to chain the families f1 to f6, so that f2 runs after f1, f3 after f2 and so on. 
| The following will do the trick:

.. literalinclude:: src/using-python-scripting.py
   :lines: 12-21
 
| The following python code shows **examples** of adding the various attributes to a :term:`node` tree.
| For a detailed  explanation please consult the user manual.

.. literalinclude:: src/using-python-scripting-1.py
