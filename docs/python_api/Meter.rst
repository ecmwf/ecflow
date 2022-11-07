ecflow.Meter
////////////


.. py:class:: Meter
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

:term:`meter` s can be used to indicate proportional completion of :term:`task`

They are able to :term:`trigger` another job, which is waiting on this proportion.
Can also be used to indicate progress of a job. Meters can be used in
:term:`trigger` and :term:`complete expression`.

Constructor::

   Meter(name,min,max,<optional>color_change)
      string name                : The meter name
      int min                    : The minimum and initial meter value
      int max                    : The maximum meter value. Must be greater than min value.
      int color_change<optional> : default = max, Must be between min-max, used in the GUI

Exceptions:

- raises IndexError when an invalid Meter is specified

Usage:

Using a meter requires:

- Defining a meter on a :term:`task`::

     meter = Meter('progress',0,100,100)
     task.add_meter(meter)

- Updating the corresponding :term:`ecf script` file with the meter :term:`child command`::

     ecflow_client --init=$$
     for i in 10 20 30 40 50 60 80 100; do
         ecflow_client --meter=progress $i
         sleep 2 # or do some work
     done
     ecflow_client --complete

- Optionally addition in a :term:`trigger` or :term:`complete expression` for job control::

     trigger task:progress ge 60

  trigger and complete expression should *avoid* using equality i.e::

     trigger task:progress == 60

  Due to network issues the meter event's may **not** arrive in sequential order
  hence the :term:`ecflow_server` will ignore meter value's, which are less than the current value
  as a result triggers's which use meter equality may never evaluate


.. py:method:: Meter.color_change( (Meter)arg1) -> int :
   :module: ecflow

returns the color change


.. py:method:: Meter.empty( (Meter)arg1) -> bool :
   :module: ecflow

Return true if the Meter is empty. Used when returning a NULL Meter, from a find


.. py:method:: Meter.max( (Meter)arg1) -> int :
   :module: ecflow

Return the Meters maximum value


.. py:method:: Meter.min( (Meter)arg1) -> int :
   :module: ecflow

Return the Meters minimum value


.. py:method:: Meter.name( (Meter)arg1) -> str :
   :module: ecflow

Return the Meters name as string


.. py:method:: Meter.value( (Meter)arg1) -> int :
   :module: ecflow

Return meters current value

