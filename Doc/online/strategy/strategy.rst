
.. index::
   single: definition creation strategies
   
.. _strategy:
   
==================================
**Definition creation strategies**
==================================
 
In ecFlow there are many different ways of creating the :term:`suite definition`.

This section will outline some of the approaches that could be taken, along
with advantages and disadvantages. Which approach is best depends on the users 
circumstance.


Python
------

The :term:`suite definition` can be built using the :ref:`suite_definition_python_api`. 
This takes a correct by construction approach and hence is less error prone.The definition can
be loaded into the server directly from Python or written to a file, for
later loading via :term:`ecflow_client`.

Can take full advantage of Python's object oriented features, to create small/large suites.
The :term:`suite definition` can be retrieved from the server and all node tree, state data 
is directly accessible in Python. ( see :ref:`print-all-states` as an example)
In addition :ref:`checking-job-creation` and simulation is available from the Python API.

.. _text_only:

Text Only
---------

The :term:`suite definition` is created using the :ref:`grammer` only,
and entered into a ASCII file. This file is then loaded into the
server using :term:`ecflow_client`

This approach is suitable for small suites only, since there are no conditional's, 
looping structures or functions.

The definition can be retrieved from the server and written to standard output. i.e ::

   > ecflow_client --get
   
However extraction of :term:`status` related data for each :term:`node` would 
have to rely on parsing the output returned from: ::

   > ecflow_client --get_state
   
Although feasible this would be very cumbersome.


Shell/Perl, etc
---------------

Here we can take advantage of the language that the user is familiar with. 
This is then used to generate the :term:`suite definition` using the :ref:`grammer`.
This approach allows use of conditionals, looping structures, and functions to create 
larger :term:`suite definition`'s. 

The generated definition can then be loaded into the :term:`ecflow_server`, using 
:term:`ecflow_client`. However like the :ref:`text_only` approach if you need
to extract :term:`status` data from the :term:`suite definition` returned from the server, 
it would require separate parsing.



 
