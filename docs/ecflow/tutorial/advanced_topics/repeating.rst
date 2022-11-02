.. index::
   single: repeating

.. _repeat:

Repeat
======

| It is sometimes useful to repeat the same :term:`task` or :term:`family` several times, 
| looping on a specific value. You can do that by defining a :term:`repeat` attribute. 
| You can iterate over sequences of:

* strings
* integers
* dates

| A sequence of integers or dates is created by specifying the
| first and last element, with an optional increment (the default is one).
| An ecFlow variable, whose name corresponds to the name of the repeat,
| will be generated. This can be used in scripts or :term:`trigger` expressions.

Ecf Script
----------

| We will add a new :term:`task` /test/f4/f5/t1. 
| Create new :term:`ecf script` file :file:`$HOME/course/test/f4/f5/t1.ecf` to use these variables.

::

   %include <head.h> 
   ecflow_client --label=info "My name is %NAME%" "My value is %VALUE%" "My date is %DATE%" 
   sleep %SLEEP% 
   %include <tail.h> 


Text
----

Let us modify the :term:`suite definition` file again

::

   # Definition of the suite test.
   suite test
    edit ECF_INCLUDE "$HOME/course"
    edit ECF_HOME    "$HOME/course"
 
    family f4 
        edit SLEEP 2 
        repeat string NAME a b c d e f 
        family f5 
            repeat integer VALUE 1 10 
            task t1 
                repeat date DATE 19991230 20000105 
                label info "" 
        endfamily 
    endfamily 
   endsuite
   
Python
------

.. literalinclude:: src/repeat.py


**What to do:**

1. Type in the changes
2. Replace the :term:`suite definition`
3. How many times will **/test/f4/f5/t1** run?
4. In :term:`ecflowview`, try to modify the values of a :term:`repeat`

