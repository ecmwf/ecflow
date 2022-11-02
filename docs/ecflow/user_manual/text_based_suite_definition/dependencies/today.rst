.. _today:

today
/////

Like :ref:`time`, but " **today"** does not wrap to tomorrow. If suites'
begin time is past the time given for the "today" command the node is
free to run (as far as the time dependency is concern.)

.. code-block:: shell

    today 3:00              # today at 3:00
    today 10:00 20:00 01:00 # every hour from 10am to 8pm

This is useful when you have early morning time-dependency, but you want
repeat the suite in the afternoon. Otherwise the node would wait till
the next morning to run.
