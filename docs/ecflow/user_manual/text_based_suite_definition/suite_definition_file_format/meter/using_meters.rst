.. _using_meters:

Using meters
//////////////


In order to use meters you have to first define the meter in the suite
definition file, e.g.

.. code-block:: shell

   suite x
      family f
         task t
            meter foo 0 100 100
         

foo is the "name" of the meter and the three numbers are minimum,
maximum, and threshold values for the meter. The default value is the
minimum value (the value shown when the suite begins). After the command
"begin" it looks like:

.. image:: /_static/using_meters/image1.png
   :width: 2.08333in
   :height: 0.33333in

In the ecFlow job file you can then modify your task to change the meter
while the job is running, e.g. like:

.. code-block:: shell

   ecflow_client --init $$
   for i in 10 20 30 40 ... ; do
      ecflow_client --meter=foo $i
      sleep 1
   done
   ecflow_client --complete


After the job has modified the meter a few times it looks like:

.. image:: /_static/using_meters/image2.png
   :width: 2.08333in
   :height: 0.33333in

And in the end, the meter looks like:

.. image:: /_static/using_meters/image3.png
   :width: 2.08333in
   :height: 0.33333in
