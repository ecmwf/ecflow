.. index::
   single: Object Oriented Suite Design


Object Oriented Suites
----------------------

Python's object oriented design features allows considerable flexibility 
in how we design and structure our :term:`suite definition`.

Each suite will have a different set of forces which determine how it should
be designed. 

Lets consider how we would design the tutorial examples in a more object oriented manner. 
We start with some design criteria we must meet.

* The default variables (ECF_HOME,etc) must be configurable and independent of the suites
* New suites must enable automatic job creation checking 
* We need to write out definition as a separate file
* New suites should be able to re-use the "boiler plate" code defined by the above requirements

Here is possible design, that uses inheritance and the template design pattern:

.. literalinclude:: src/object-oriented-suites.py
