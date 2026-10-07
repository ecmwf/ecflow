.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

ecflow.AvisoAttr
////////////////


.. py:class:: AvisoAttr
   :module: ecflow

   Bases: :py:class:`~pybind11_builtins.pybind11_object`

An :term:`aviso` attribute, assigned to a :term:`task`, represents an external trigger holding the task queued until an Aviso notification matching the attribute configuration is detected.

:term:`aviso` attributes are only allowed on tasks (or aliases of tasks), and only one aviso attribute per task is allowed; adding one to a suite or a family raises a RuntimeError.


Constructors::

   AvisoAttr(name, listener) (1)
   AvisoAttr(name, listener, url)
   AvisoAttr(name, listener, url, auth=auth)
   AvisoAttr(name, listener, url, auth=auth, collapse=True)
    with:
      string name: The Aviso attribute name
      string listener: The Aviso listener configuration (in JSON format)
      string url: The URL used to contact the Aviso server
      string auth: The path to the Aviso credentials file (keyword only)
      bool collapse: Whether a release consumes all the notifications received, instead of exactly
                     one (keyword only, default False)

Note: Default values, based on %ECF_AVISO_...% variables, are used when
the parameters url and auth are not provided

Specify the :code:`%ECF_AVISO_***%` variables once (at suite level), and then create the
Aviso attributes passing just the name and the listener definition as per call `(1)`.

.. note::   The `listener` parameter is expected to be a valid single line JSON string, enclosed in single quotes.
   As a convenience, missing surrounding single quotes are detected and will automatically be added.

Details regarding the format of `listener` are in the section describing the :term:`aviso` attribute.


Usage:

.. code-block:: python

   t1 = Task('t1', AvisoAttr('name', "'{...}'"))

   t2 = Task('t2')
   t2.add_aviso(AvisoAttr('name', "'{...}'", 'http://aviso.com', auth='/path/to/auth'))

The parameters `url` and `auth` are optional


.. py:method:: AvisoAttr.auth(self: ecflow.AvisoAttr) -> str
   :module: ecflow

Returns the path to Authentication credentials used to contact the Aviso server


.. py:method:: AvisoAttr.collapse(self: ecflow.AvisoAttr) -> bool
   :module: ecflow

Returns whether a release consumes all the notifications received, instead of exactly one


.. py:method:: AvisoAttr.listener(self: ecflow.AvisoAttr) -> str
   :module: ecflow

Returns the Aviso listener configuration


.. py:method:: AvisoAttr.name(self: ecflow.AvisoAttr) -> str
   :module: ecflow

Returns the name of the Aviso attribute


.. py:method:: AvisoAttr.url(self: ecflow.AvisoAttr) -> str
   :module: ecflow

Returns the URL used to contact the Aviso server
