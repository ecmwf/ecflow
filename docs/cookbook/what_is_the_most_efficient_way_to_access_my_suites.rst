.. _what_is_the_most_efficient_way_to_access_my_suites:

What is the most efficient way to access my suites?
//////////////////////////////////////////////////////////////////

We will use the python api, as this this gives us the most flexibility.

The standard way to access your suites is to use sync_local() , i.e.

.. code-block:: python
  :caption: sync_local

  import ecflow
  ci = ecflow.Client("my_host",3141) # replace the host and port with your own.
  ci.sync_local()                    # download all the suites on the server
  print(ci.get_defs())  

The code above will suffice for the vast majority of cases. However if
you have several suites, and are only interested in a subset, then the
method above is not the most optimal. This method will be lot quicker.

.. code-block:: python
  :caption: ch_register

  import ecflow
  ci = ecflow.Client("my_host",3141)               # replace the host and port with your own.
  suites_of_interest = [ 's1', 's2' ]              # These are the *only* suites that I am interested in
  ci.ch_register(False,suites_of_interest)         # register interest in the suites
  ci._sync_local()                                 # sync_local() will now ONLY return the suites s1,s2
  print(ci.get_defs())
  ci.ch_drop()      


Here is more full blown example, demonstrating the performance
differences, in retrieving all the suites, and in retrieving suites, via registration.

.. literalinclude:: src/timing_example.py
   :language: python
   :caption: Timing example, sync_local with ch_register    
