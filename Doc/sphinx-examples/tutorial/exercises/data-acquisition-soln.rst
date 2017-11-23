.. index::
   single: data-acquisition-soln

.. rubric: data_acquisition_soln

.. _data_acquisition_soln:

Data acquisition solution
-------------------------

Here is the one possible answer:

.. literalinclude:: src/data-acquisition.py
   :lines: 1-26

It is also possible to automatically generate the :term:`ecf script` using the python api::

   defs.generate_scripts()
   
This enables testing of the :term:`suite definition`, with out worrying about the :term:`ecf script`'s.

.. note::

   When there are no :term:`event`'s, :term:`meter`'s or :term:`label`'s in the :term:`suite definition`, 
   the content of the  generated scripts are identical. Hence this functionality should **only** be used
   as **debug** aid for the definition