
.. index::
   single: cookbook
   
.. _test_bench:
   
How can I test a definition without writing scripts ?
-----------------------------------------------------

When designing the definition it is often very desirable to
be able to auto generate the :term:`ecf script` s. This allows for a lot
of experimentation with :term:`suite definition` structure without the chore
of continually keeping the scripts in sync.

The following Python code shows that it is possible to test
*any* :term:`suite definition` without having to create any :term:`ecf script` s.

The :term:`ecf script` files are auto generated. Hence if the :term:`suite definition` contains
:term:`event`, :term:`meter` or :term:`label`, the auto generate feature will create scripts
with correct :term:`child command` s to communicate with the server.

The scripts assumes that :term:`ecflow_client` is accessible on $PATH.

To use, copy the contents into a file, say :file:`test_bench.py` ::

   > test_bench.py --help
   
If the server is running locally the typical usage might be ::

   > test_bench.py my_test.def --port 4141

.. literalinclude:: src/test_bench.py
