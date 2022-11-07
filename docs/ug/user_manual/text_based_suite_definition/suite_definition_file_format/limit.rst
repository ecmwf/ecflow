.. _limit:

limit
/////

This command defines a limit into the current node. It is a means of
providing simple load management by say limiting the number of tasks
submitted to a specific server.

Typically you either define limits on suite level or define a separate
suite to hold limits so that they can be used by multiple suites.

.. code-block:: shell

   suite limits
      limit sgi 10
      limit mars 10
   endsuite
   suite obs
      family limits
      limit hpcd 20
      endfamily
      extern /limits:sgi
      task t1
         inlimit sgi
      task t2
         inlimit /obs/limits:hpcd
   endsuite


Using limits
================

In order to use limits, you have to first define a limit in the suite
definition file and then you must also assign families/tasks to use this
limit, e.g.

.. code-block:: shell

   suite x
   limit fast 1
   family f
      inlimit /x:fast
      task t1
      task t2

Where fast is the "name" of the limit and the number after the name
defines the maximum number of tasks that can run simultaneously using
this limit. That's why you do not need a trigger between tasks "t1" and
"t2".

There is no need to change tasks. The jobs are run in the order they
appear in the suite definition. Be aware that the command order may be
used to modify the order after the suite definition has been defined.

The following is a sequence if the jobs run in the normal order

.. image:: /_static/limit/image1.png
   :width: 2.09444in
   :height: 0.66667in

.. image:: /_static/limit/image2.png
   :width: 2.09444in
   :height: 0.64583in

And if you manually now re-run the "t1" (you go over the limit!)

.. image:: /_static/limit/image3.png
   :width: 2.09444in
   :height: 0.64583in

Using integer limits is a bit different. In the following example, we
define limit disk to be 50 (megabytes) and task using 20 (megabytes)
each. This means that only two of them can be running any given time.

.. code-block:: shell

   suite x
   limit disk 50
   family f
      inlimit disk 20
      task t1
      task t2
      task t3
      
Where disk is the "name" of the limit, 50 is the maximum value this limit can be.
