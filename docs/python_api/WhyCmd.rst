ecflow.WhyCmd
/////////////


.. py:class:: WhyCmd
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

The why command reports, the reason why a node is not running.

It needs the  definition structure and the path to node

Constructor::

   WhyCmd(defs, node_path)
      defs_ptr  defs   : pointer to a definition structure
      string node_path : The node path


Exceptions:

- raises RuntimeError if the definition is empty
- raises RuntimeError if the node path is empty
- raises RuntimeError if the node path cannot be found in the definition

Usage::

   try:
      ci = Client()
      ci.sync_local()
      ask = WhyCmd(ci.get_defs(),'/suite/family')
      print(ask.why())
   except RuntimeError, e:
       print(str(e))


.. py:method:: WhyCmd.why( (WhyCmd)arg1) -> str :
   :module: ecflow

returns a '/n' separated string, with reasons why node is not running

