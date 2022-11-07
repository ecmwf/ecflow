.. index::
   single: repeat (tutorial)

.. _tutorial-repeat:

Repeat
======

It is sometimes useful to repeat the same :term:`task` or :term:`family` several times, looping on a specific value. You can do that by defining a :term:`repeat` attribute. You can iterate over sequences of:

* strings
* integers
* dates

A sequence of integers or dates is created by specifying the first and last element, with an optional increment (the default is one). 

We can also loop over an arbitrary list of dates:

.. code-block:: shell
   :caption: datelist

   repeat datelist YMD 20130101 20130102 20130103 20200101 20190101

An ecFlow variable, whose name corresponds to the name of the repeat, will be generated. This can be used in scripts or :term:`trigger` expressions.

.. note::

   If **repeat date**, or **repeat datelist** are used in trigger expressions, they will use date arithmetic.

Repeat with day/date
----------------------

Once the **day monday** is free on family **f1**, it stays free until the automatic re-queue caused by the parent repeat. The net effect being that task t2 will still run, even if we have strayed into Tuesday.


.. code-block:: shell
   :caption: day/date remain free until re-queue

   suite s1
     family f
       repeat integer rep 0 7
       family f1
          day monday
          time 23:00
          task t1
          task t2
               trigger t1 == complete
    ...

Repeat increment
-----------------

A repeat under a family node will only increment when all the child nodes are complete. In the example above, once task **t1**, and **t2** are complete, the repeat integer **rep** will increment to the next value.

Ecf Script
----------

We will add a new :term:`task` /test/f4/f5/t1. Create new :term:`ecf script` file :file:`$HOME/course/test/f4/f5/t1.ecf` to use these variables.

.. code-block:: bash
   :caption:  $HOME/course/test/f4/f5/t1.ecf

   %include <head.h>
   ecflow_client --label=info "My name is '%NAME%' " " My value is '%VALUE%' " " My date is '%DATE%' "
   # Note the use of repeat date generated variables DATE_YYYY, DATE_MM, DATE_DD, DATE_DOW to automatically reference year,month,day of the month,day of the week
   # These can also be used in trigger expression.
   ecflow_client --label=date "year(%DATE_YYYY%) month(%DATE_MM%) day of month(%DATE_DD%) day of week(%DATE_DOW%)"
   sleep %SLEEP%
   %include <tail.h>


Text
----

Let us modify the :term:`suite definition` file again:

.. code-block:: shell

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
                repeat date DATE 20101230 20110105
                label info ""
                label date ""
        endfamily
    endfamily
   endsuite

Python
------

.. literalinclude:: src/repeat.py
   :language: python
   :caption: $HOME/course/test.py


**What to do**

#. Type in the changes
#. Replace the :term:`suite definition`
#. How many times will **/test/f4/f5/t1** run?
#. In :term:`ecflow_ui`, try to modify the values of a :term:`repeat`
#. Since we are using a 2-second delay, remember to use f5(refresh) to see the intermediate values. The default is to refresh every 60 seconds.

