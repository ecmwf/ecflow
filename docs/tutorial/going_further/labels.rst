.. index::
   single: labels (tutorial)

.. _tutorial-labels:

Labels
======

Sometime it is very useful to see :term:`task` specific information in :term:`ecflow_ui`. For this we can define a :term:`label`. A :term:`label` is a string that is attached to a :term:`task` and that can be updated using the :term:`child command` :term:`ecflow_client` --label

Ecf Script
----------

We will create new family **f3** with a task **t1**. Create :term:`ecf script` :file:`$HOME/course/test/f3/t1.ecf`

.. code-block:: shell
   :caption: $HOME/course/test/label/t1.ecf

   %include <head.h>
   
   n=1
   while [[ $n -le 5 ]]                  # Loop 5 times
   do
      msg="The date is now $(date)"
      ecflow_client --label=info "$msg"  # Set the label
      sleep 60                           # Wait a one minute
      (( n = $n + 1 ))
   done
   
   ecflow_client --label=info "I have now finished my work."
   
   %include <tail.h>

Text
----

.. code-block:: shell

   # Definition of the suite test. Note Family f1,f2 from previous pages are omitted
   suite test
      edit ECF_INCLUDE "$HOME/course"
      edit ECF_HOME    "$HOME/course"
      family label
         task t1
               label info ""
      endfamily
   endsuite


Python
------

The following section shows how to add a :py:class:`ecflow.Label` in Python.  (Note: **families f1 and f2 are omitted for brevity**).

.. literalinclude:: src/labels.py
   :language: python
   :caption: $HOME/course/test.py


**What to do**

#. Modify the :term:`suite definition` file or python script
#. Create the new :term:`ecf script` file :file:`$HOME/course/test/f3/t1.ecf`
#. Replace the :term:`suite definition`
#. Watch in :term:`ecflow_ui`
#. Change the label colour in :ref:`ecflow_ui`, Tools > Preferences... > Appearance > Node Labels
