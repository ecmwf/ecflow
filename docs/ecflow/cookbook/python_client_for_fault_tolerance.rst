.. _python_client_for_fault_tolerance:

Python Client for fault tolerance
///////////////////////////////////

An ecFlow Python Client can be used to solve a Fault Tolerance request, for example, when three out of five families or tasks are enough to carry on submitting new jobs.

Such client can be used as an ecFlow task wrapper, with few more lines.


.. image:: /_static/python_client_for_fault_tolerance/image1.png
   :width: 5.97778in
   :height: 1.79167in

.. image:: /_static/python_client_for_fault_tolerance/image2.png
   :width: 5.97778in
   :height: 1.79167in


.. literalinclude:: src/ecflow3of5.py
    :language: python

