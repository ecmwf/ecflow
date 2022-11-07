ecflow.UrlCmd
/////////////


.. py:class:: UrlCmd
   :module: ecflow

   Bases: :py:class:`~Boost.Python.instance`

Executes a command ECF_URL_CMD to display a url.

It needs the definition structure and the path to node.

Constructor::

   UrlCmd(defs, node_path)
      defs_ptr  defs   : pointer to a definition structure
      string node_path : The node path.


Exceptions

- raises RuntimeError if the definition is empty
- raises RuntimeError if the node path is empty
- raises RuntimeError if the node path cannot be found in the definition
- raises RuntimeError if ECF_URL_CMD not defined or if variable substitution fails

Usage:
Lets assume that the server has the following definition::

   suite s
      edit ECF_URL_CMD  "${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%"
      edit ECF_URL_BASE "http://www.ecmwf.int"
      family f
         task t1
            edit ECF_URL "publications/manuals/ecflow"
         task t2
            edit ECF_URL index.html

::

   try:
      ci = Client()
      ci.sync_local()
      url = UrlCmd(ci.get_defs(),'/suite/family/task')
      print(url.execute())
   except RuntimeError, e:
       print(str(e))


.. py:method:: UrlCmd.execute( (UrlCmd)arg1) -> None :
   :module: ecflow

Displays url in the chosen browser

