
..
   This is the documentation for ecFlow, a workflow management system developed by ECMWF.
   The documentation is built using Sphinx and, by convention, we use the following heading markup in our reStructuredText files:
 
     ******************************************** (h1)
     ============================================ (h2)
     -------------------------------------------- (h3)
     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ (h4)
     """""""""""""""""""""""""""""""""""""""""""" (h5)
     '''''''''''''''''''''''''''''''''''''''''''' (h6)

Welcome to ecFlow's documentation
*********************************

.. image:: /_static/overview/image1.png
   :width: 300px

*ecFlow* is a client/server workflow package that enables users to run a large number of programs (with dependencies on each other and on time) in a controlled environment. It provides tolerance for hardware and software failures, combined with restart capabilities. It is used at ECMWF to run all our operational suites across a range of platforms.

.. toctree::
   :maxdepth: 1
   :caption: Documentation

   overview
   quickstart.rst
   tutorial/tutorial.rst
   client_api/index.rst
   python_api/python_api.rst
   rest_api.rst
   udp_api.rst
   ug/index.rst
   glossary.rst
   faq

.. toctree::
   :maxdepth: 1
   :caption: Installation

   install/index.rst
   release_notes/index.rst
   support
   contributing
   licence


Indices and tables
******************

* :ref:`genindex`
  
.. * :ref:`modindex`
.. * :ref:`search`
