
.. index::
   single: intro
   
.. _intro:
   
================
**Introduction**
================
 
The aim of this tutorial is to go through most of ecFlow functionality by building 
a simple suite. Some extension exercises then follow.

Each page will introduce a new concept and provides a list of things to do. 
Within most pages, there are hypertext links that point to 
relevant information in the online ecFlow documentation.

There are two main methods for describing a suite to the ecflow server.  The first is to write 
a text definition file which is then loaded to the server.  The grammar of this text definition file
is described here :ref:`grammer`.  This grammar does not support conditional statements (such as if,while,for)
nor the ability to define functions. However, the text definition file can be generated using any language
which in itself supports conditional statements.  The language is similar to that offered by SMS/CDP and
as such may be an appropriate migration path for some.

However, ecFlow can also be accessed using a python API which allows more functionality and as such is our preferred way
to define suites to the ecFlow server. See :ref:`python_api`.

.. note::

   The following tutorial will show examples in plain text and python.
   Although plain text is fine for simple suites, it is recommended that you use python.
   Later tutorial examples use conditional statements availble through the python interface like 'if' and looping constructs.
