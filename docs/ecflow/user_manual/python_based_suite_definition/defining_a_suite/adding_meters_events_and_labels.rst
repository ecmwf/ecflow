.. _adding_meters_events_and_labels:

Adding Meters, Events and Labels
////////////////////////////////

.. code-block:: python

   from ecflow import Defs,Suite,Task,Meter,Event,Label
   
   defs = Defs(
      Suite('s1',
         Task("t1",
               Event(2),                         # event reference with 2
               Event("wow"),                     # event reference with name "wow"
               Event(10,"Eventname2" ),          # event referenced with name "Eventname2"
               Meter("metername3",0,100),        # name, min, max
               Label("label_name4", "value"))))  # name, value
  
The following show alternative styles, which produce the same definition.

.. code-block:: python

   defs = Defs().add(
            Suite("s1").add(
            Task("t1").add(
               Event(2),
               Event("wow"),
               Event(10,"Eventname2" ),
               Meter("metername3",0,100),
               Label("label_name4", "value"))))   

.. code-block:: python

   defs = Defs() + Suite("s1")
   # Using '+' to add node attributes will only work
   # if we start with a Node i.e. Task('t1') in this case
   defs.s1 += Task("t1") + Event(2) + Event("wow") + \
            Event(10,"Eventname2" ) + \
            Meter("metername3",0,100) + \
            Label("label_name4", "value")

.. code-block:: python

   with Defs() as defs:
      with defs.add_suite("s1") as suite:
         with suite.add_task("t1") as t1:
               t1 += [ Event(2),
                     Event("wow"),
                     Event(10,"Eventname2" ),
                     Meter("metername3",0,100),
                     Label("label_name4", "value") ]
