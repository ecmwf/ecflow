.. index::
   single: embedded triggers (tutorial)

.. _tutorial-embedded-triggers:  
   
Embedded triggers
===================

This is a small follow up exercise showing the use of embedded triggers. These are trigger expressions embedded within the scripts using  **--wait** child command. Whilst the expression is **not** true, the job will hold. Where possible you should give preference to triggers on the definitions, since these are checked on creation, whereas embedded triggers are checked at run time.

Ecf Script
-------------

Add/amend the **t2.ecf** script.

.. code-block:: shell
    :caption: $HOME/course/test/f1/t2.ecf

    %include <head.h>
    ecflow_client --wait="t1 == complete" # wait for expression to become true
    %include <tail.h>

Text
-----

.. code-block:: shell
        
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
---------

.. literalinclude:: src/embedded-triggers.py
   :language: python
   :caption: $HOME/course/test.py


**What to do**


#. Edit the :term:`suite definition` file.
#. Replace the :term:`suite`.

   | Python: ``python3 test.py ; python3 client.py``
   | Text: ``ecflow_client --suspend=/test ;  ecflow_client --replace=/test test.def``

#. Observe the tasks in ecflow_ui .
#. Notice the wait icon on task t2.
#. Introduce an error in the wait expression, the job should abort:

   .. code-block:: shell
      :caption: Introduce error in wait expression

      ecflow_client --wait="txx == complete"  # there is no node with name txx, this should abort the task

#. Introduce an impossible expression, what is the effect?

   .. code-block:: shell
      :caption: expression that will never be satisfied

      ecflow_client --wait="1 == 0"

