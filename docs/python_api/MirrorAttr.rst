ecflow.MirrorAttr
/////////////////


.. py:class:: MirrorAttr
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

A :term:`mirror` attribute, assigned to a :term:`node`, enables establishing an external link and locally replicate the state of a node executing on a remote ecFlow server.

Although :term:`mirror` attributes can be set at any level (Suite, Family, Task), it only makes sense to assign mirror attributes to Tasks, and only one mirror attribute per node is allowed.


Constructor::

   MirrorAttr(name, remote_path, ...)
      string name: The Mirror attribute name
      string remote_path: The path on the remote ecFlow server to the node being replicated
      string remote_host: The host of the remote ecFlow server
      string remote_port: The port of the remote ecFlow server
      string polling: The polling interval used to contact the remote ecFlow server
      Bool ssl: `true`, when using SSL to contact the remote ecFlow server; `false`, otherwise
      string auth: The path to the Mirror Authentication credentials


Usage:

.. code-block:: python

   t1 = Task('t1',
             MirrorAttr('name', '/remote/task', 'remote-ecflow', '3141', '60', True, '/path/to/auth'))

   t2 = Task('t2')
   t2.add_aviso('name', '/remote/task', 'remote-ecflow', '3141', '60', True, '/path/to/auth')

The parameters `remote_host`, `remote_port`, `polling`, `ssl`, and `auth` are optional


.. py:method:: MirrorAttr.auth( (MirrorAttr)arg1) -> str :
   :module: ecflow

Returns the path to Authentication credentials used to contact the remote ecFlow server


.. py:method:: MirrorAttr.name( (MirrorAttr)arg1) -> str :
   :module: ecflow

Returns the name of the Mirror attribute


.. py:method:: MirrorAttr.polling( (MirrorAttr)arg1) -> str :
   :module: ecflow

Returns the polling interval used to contact the remove ecFlow server


.. py:method:: MirrorAttr.remote_host( (MirrorAttr)arg1) -> str :
   :module: ecflow

Returns the host of the remote ecFlow server


.. py:method:: MirrorAttr.remote_path( (MirrorAttr)arg1) -> str :
   :module: ecflow

Returns the path on the remote ecFlow server


.. py:method:: MirrorAttr.remote_port( (MirrorAttr)arg1) -> str :
   :module: ecflow

Returns the port of the remote ecFlow server


.. py:method:: MirrorAttr.ssl( (MirrorAttr)arg1) -> bool :
   :module: ecflow

Returns a boolean, where true means that SSL is enabled

