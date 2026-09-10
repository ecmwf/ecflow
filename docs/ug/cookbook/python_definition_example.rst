.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. index::
   single: cookbook-python_definition_example
   
.. _cookbook-python_definition_example:

Python definition example
*************************

Here is an example of a Python Suite Definition, hosting a task that
might be helpful for simple SMS suite translation to ecFlow:

.. code-block:: shell
  
  cd ~map/course/201303/ecflow; python course.py
 
.. literalinclude:: src/course.py
  :language: python

A simple tar file can be downloaded to get script wrappers and include
files (~map/course/201303/course2013ex.tgz).

.. literalinclude:: src/ecf.py
  :language: python
