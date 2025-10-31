.. index::
   single: repeat (tutorial)

.. _tutorial-repeat:

Repeat
======

It is sometimes useful to repeat the same :term:`task` (or :term:`family`) multiple times, iterating on a specific value.
This can be achieved by defining a :term:`repeat` attribute, which enables iterating over:

* a sequence of integers (:code:`repeat integer`)
* a sequence of dates  (:code:`repeat date`)
* a sequence of time instants (:code:`repeat datetime`)
* a list of string  (:code:`repeat string` or :code:`repeat enumerated`)
* a list of dates (:code:`repeat datelist`)

Sequences of integers, dates or time instants are created by specifying the first and last element, with an optional
increment (the default is 1 for integers, 1 day for dates, and 24:00:00 for time instants).
Lists of strings or dates are created by specifying the individual elements.

On the definition file, a :term:`repeat` attribute is specified by adding a line starting with the keyword :code:`repeat`,
followed by the type of repeat, the name of the repeat, and the values to iterate over.

.. code-block:: shell
   :caption: Example of repeat attributes

   repeat integer COUNT 1 10 2
   repeat date DATE 20220101 20220110 2
   repeat datetime INSTANT 20220101T000000 20220103T000000 12:00:00
   repeat string COLOR red green blue yellow
   repeat enumerated FRUIT apple banana cherry
   repeat datelist YMD 20130101 20130102 20130103 20200101 20190101

In order to be used by the task script or :term:`trigger` expressions, each :term:`repeat` attribute generates ecFlow variable(s),
with the name(s) corresponding to the name of the repeat.

.. note::

   If :code:`repeat date`, or :code:`repeat datelist` are used in trigger expressions, they will use date arithmetic.

Repeat with day/date
----------------------

Mixing day/date restrictions with repeat attributes is possible, but may lead to seemingly unexpected results.

Considering the following example, notice that once the :code:`day monday` is free on family :code:`f1`,
it stays free until the automatic re-queue caused by the parent repeat. This means that task t2 will still execute,
even if in the case where the day already changed to Tuesday.

.. code-block:: shell
   :caption: day/date remain free until requeue

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

A :term:`repeat` under a family node will only increment when all the child nodes are complete.
In the previous above, only once tasks :code:`t1` and :code:`t2` are both complete,
the repeat integer :code:`rep` will automatically be incremented to the next value and the :code:`f` family is requeued.

Suite Definition
----------------

.. tabs::

    .. tab:: Text

      Modify the :term:`suite definition` file, to add a new :term:`task` :code:`/test/f4/f5/t1`, as follows:

      .. code-block:: shell

         # Definition of the suite test.
         suite test
          edit ECF_INCLUDE "$HOME/course"
          edit ECF_HOME    "$HOME/course"

          ...

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

          ...

         endsuite

    .. tab:: Python

      .. literalinclude:: src/repeat.py
         :language: python
         :caption: $HOME/course/test.py

Task Script
-----------

Create a new :term:`task script <ecf script>` file :file:`$HOME/course/test/f4/f5/t1.ecf` to use :term:`repeat` variables.

.. code-block:: bash
   :caption:  $HOME/course/test/f4/f5/t1.ecf

   %include <head.h>
   ecflow_client --label=info "My name is '%NAME%' " " My value is '%VALUE%' " " My date is '%DATE%' "
   # Note the use of repeat date generated variables DATE_YYYY, DATE_MM, DATE_DD, DATE_DOW to automatically reference year,month,day of the month,day of the week
   # These can also be used in trigger expression.
   ecflow_client --label=date "year(%DATE_YYYY%) month(%DATE_MM%) day of month(%DATE_DD%) day of week(%DATE_DOW%)"
   sleep %SLEEP%
   %include <tail.h>

**What to do**

#. Apply the changes to :term:`suite definition`.
#. Apply the changes to :term:`task script <ecf script>`.
#. Check how many times the :term:`task` :code:`/test/f4/f5/t1` is executed?
#. In the :term:`ecflow_ui`, modify the values of a :term:`repeat`

   * Consider using :code:`F5` (Refresh) to update and display intermediate values.
