ecflow.Autorestore
//////////////////


.. py:class:: Autorestore
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Provides a way to automatically restore a previously archived node

This is required when dealing with super large suite/families, they can be archived off, and then restored later.
This reduces the time to checkpoint the definition and network band width.
The archived node is written to disk, as ECF_HOME/<host>.<port>.ECF_NAME.check,
where '/' is replaced with ':' in ECF_NAME.
The node can be recovered using 'autorestore', begin,re-queue and manually via ecflow_client --restore.
The archiving is never immediate. The nodes are checked one a minute, and expired autoarchive nodes are archived.
If the node is suspended or any of its parent are suspended then then the archive does not happen.

Constructor::

   Autorestore( list of paths )

Usage:

.. code-block:: python

   attr = Autorestore( ['/s1/f1'] )           # restore archived node at /s1/f1
   attr = Autorestore( ['/s1/f1','/s1/f2'] )  # restore archived node at /s1/f1 and /s1/f2


.. py:method:: Autorestore.nodes_to_restore( (Autorestore)arg1) -> object :
   :module: ecflow

returns a list of nodes to be restored

