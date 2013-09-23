.. index::
   single: Variable inheritance

.. _variable-inheritance:   
   
Variable inheritance
=====================

| In the previous chapter, we saw how to define a :term:`variable` for a :term:`task`. 
| When all the tasks of the same :term:`family` share the same variable value, 
| the value could be defined at the family level. 
| This is termed :term:`variable inheritance`

| In the examples below the :term:`variable` could have been defined at the 
| level of the :term:`suite`, achieving the same results. 

| Variables are inherited from the parent node. 
| If a variable is redefined lower in the tree, it is said to be overridden. 
| In this case the new definition is the one being used. 
| It is possible to override generated variables. 
| This is not recommended and you should understand all the consequences 
| if you decide to do so.

Text
----

::

   # Definition of the suite test.
   suite test
      edit ECF_INCLUDE "$HOME/course"   # replace '$HOME' with the path to your home directory
      edit ECF_HOME    "$HOME/course"
      family f1
         edit SLEEP 20 
         task t1
         task t2
      endfamily
   endsuite
      
Python
------

.. literalinclude:: src/variable-inheritance.py


Quiz
----

Let us have a quiz. Consider the following suite::

   suite test
      edit SLEEP 100
      family f1
         edit SLEEP 80
         task t1
         task t2
            edit SLEEP 9
         family g1
             edit SLEEP 89
             task x1
                 edit SLEEP 10
             task x2
         endfamily
      endfamily
      family f2
        task t1
        task t2
            edit SLEEP 77
        family g2
             task x1
                 edit SLEEP 12
             task x2
         endfamily
      endfamily
   endsuite

Here is the value for SLEEP for the above suite. Make sure you understand this.

   ==============  ======
   :term:`node`    SLEEP
   ==============  ======
   /test/f1/t1        80
   /test/f1/t2         9
   /test/f1/g1/x1     10
   /test/f1/g1/x2     89
   /test/f2/t1       100
   /test/f2/t2        77
   /test/f2/g2/x1     12 
   /test/f2/g2/x2    100
   ==============  ======
