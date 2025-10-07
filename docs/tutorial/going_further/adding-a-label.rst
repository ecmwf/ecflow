.. index::
   single: labels (tutorial)

.. _tutorial-labels:

Adding a label
==============

It is very useful to see :term:`task` specific information in the :term:`ecflow_ui`. This can be achieved by defining
a :term:`label`, i.e. a string that is attached to a :term:`task`, and that can be populates using the
:term:`child command` :term:`ecflow_client` :code:`--label`.

Update Task Script
------------------

To create a new family :code:`label` with a task :code:`t1`, start by creating the :term:`task script <ecf script>` at :file:`$HOME/course/test/label/t1.ecf`

.. code-block:: shell
   :caption: $HOME/course/test/label/t1.ecf

   %include <head.h>
   
   n=1
   while [[ $n -le 5 ]]                      # Loop 5 times
   do
      msg="The date is now $(date)"
      ecflow_client --label=info "$msg"      # Set the label
      sleep 30                               # Wait half a minute
      (( n = $n + 1 ))
   done
   
   ecflow_client --label=info "Job is done!" # Set the final label
   
   %include <tail.h>

Update Suite Definition
-----------------------

Update the suite definition file to add the new family :code:`label` and task :code:`t1`.

.. tabs::

    .. tab:: Text

        .. code-block:: shell

           suite test
             edit ECF_INCLUDE "{{HOME}}/course" # replace '$HOME' appropriately
             edit ECF_HOME    "{{HOME}}/course"

             [... previous families omitted for brevity ..]

             family label
               task t1
                 label info ""
             endfamily
           endsuite

    .. tab:: Python

        The following example shows how to add a :py:class:`ecflow.Label` in Python. (Note: **families f1 and f2 are omitted for brevity**).

        .. literalinclude:: src/labels.py
           :language: python
           :caption: $HOME/course/test.py

**What to do**

#. Create the new :term:`task script <ecf script>` file, as shown above.
#. Modify the suite definition to include the new family and task, as shown above.
#. Replace the :term:`suite`, using:

   .. tabs::

      .. tab:: Text

         .. code-block:: shell

            ecflow_client --suspend /test
            ecflow_client --replace /test test.def

      .. tab:: Python

         .. code-block:: shell

            python3 test.py
            python3 client.py

#. Observer the label change using the :term:`ecflow_ui`.
#. Customise the label colouring in :ref:`ecflow_ui`, at Tools > Preferences... > Appearance > Node Labels
