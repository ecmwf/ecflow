.. _python_based_suite_definition:

Python based suite definition
/////////////////////////////////////

ecFlow provides a :ref:`python_api`. This allows: 

- complete specification of the suite definition, including trigger and time dependencies 
- full access to the command level interface(CLI)

Since the full power of python is available to specify the suite definition, there is considerable flexibility. The API is documented using the python _doc_ facility.

.. warning::

   Before the :ref:`python_api` can be used you need to set some variables.
   
   **PYTHONPATH** must be set to include the directory where the file 'ecflow.so' has been installed

.. toctree::
   :maxdepth: 1

   defining_a_suite/index.rst
   adding_dependencies/index.rst
   adding_attributes/index.rst
   control_structures_and_looping
   adding_externs_automatically
   checking_the_suite_definition
   checking_job_generation
   handling_dummy_tasks
   debugging_suite_definition/index
   error_handling
   

