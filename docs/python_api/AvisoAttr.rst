ecflow.AvisoAttr
////////////////


.. py:class:: AvisoAttr
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

An :term:`aviso` attribute, assigned to a :term:`node`, represents an external trigger holding the node queued untilan Aviso notification matching the attribute configuration is detected.

Although :term:`aviso` attributes can be set at any level (Suite, Family, Task), it only makes sense to assign aviso attributes to tasks, and only one aviso attribute per node is allowed.


Constructors::

   AvisoAttr(name, listener) (1)
   AvisoAttr(name, listener, url)
   AvisoAttr(name, listener, url, schema)
   AvisoAttr(name, listener, url, schema, polling)
   AvisoAttr(name, listener, url, schema, polling, auth)
    with:
      string name: The Aviso attribute name
      string listener: The Aviso listener configuration (in JSON format)
      string url: The URL used to contact the Aviso server
      string schema: The path to the Aviso schema
      string polling: The polling interval used to contact the Aviso server
      string auth: The path to the Aviso Authentication credentials

Note: Default values, based on %ECF_AVISO_...% variables, will be used for the calls where
the parameters url, schema, polling, and auth are not provided

We suggest to specify :code:`%ECF_AVISO_***%` variables once (at suite level), and then create the
Aviso attributes passing just the name and the listener definition as per call `(1)`.

.. note::   The `listener` parameter is expected to be a valid single line JSON string, enclosed in single quotes.
   As a convenience, missing surrounding single quotes are detected and will automatically be added.

Details regarding the format of `listener` are in the section describing the :term:`aviso` attribute.


Usage:

.. code-block:: python

   t1 = Task('t1', AvisoAttr('name', "'{...}'"))

   t2 = Task('t2')
   t2.add_aviso('name', "'{...}'", 'http://aviso.com', '60', '/path/to/auth')

The parameters `url`, `schema`, `polling`, and `auth` are optional


.. py:method:: AvisoAttr.auth( (AvisoAttr)arg1) -> str :
   :module: ecflow

Returns the path to Authentication credentials used to contact the Aviso server


.. py:method:: AvisoAttr.listener( (AvisoAttr)arg1) -> str :
   :module: ecflow

Returns the Aviso listener configuration


.. py:method:: AvisoAttr.name( (AvisoAttr)arg1) -> str :
   :module: ecflow

Returns the name of the Aviso attribute


.. py:method:: AvisoAttr.polling( (AvisoAttr)arg1) -> str :
   :module: ecflow

Returns polling interval used to contact the Aviso server


.. py:method:: AvisoAttr.schema( (AvisoAttr)arg1) -> str :
   :module: ecflow

Returns the path to the schema used to contact the Aviso server


.. py:method:: AvisoAttr.url( (AvisoAttr)arg1) -> str :
   :module: ecflow

Returns the URL used to contact the Aviso server

