.. _meter:

Meter
/////

This is an extension of the event. In some tasks, there may be many
events that are set in order, e.g. in a 10-day weather forecast, an
event might be set every six hours, more than 40 events. These events
are set in increasing order. By creating a meter that can have values,
in this case from 0 to 240, will help to have more valid information on
the display.

In a suite definition file, one would have:

.. code-block:: shell

    task forecast
    meter step 0 240 240

with the following syntax:

.. code-block:: shell

    meter name min max [colour-change]


The meter can be used in triggers in the same way as the events, except
that the meter will have a value, e.g.

.. code-block:: shell

    task plot5days
        trigger fc/model:step eq 120 # 5 days done
    
The numeric value used in the triggering means that there is an
ambiguity if you have a node with the same name, let's say task "120".

.. toctree::
    :maxdepth: 1

   using_meters
   using_meters_in_triggers
