.. index::
   single: manual (tutorial)

.. _tutorial-manual:

Adding a manual
===============
  
A :term:`manual page` allows including documentation directly **in** a :term:`ecf script` to be viewed in :term:`ecflow_ui`.

The manual page is the concatenation of all the text within the :code:`%manual` and :code`%end` :term:`directives`.

To include the manual for task :code:`t2`, modify :file:`t2.ecf` to include the following:

.. code-block:: shell
   :caption: $HOME/course/test/f1/t2.ecf

   %manual
      Manual for task t2
      Operations: if this task fails, set it to complete and report next working day
      Analyst:    Check something ?
   %end

   %include "../head.h" 
   echo "I am part of a suite that lives in %ECF_HOME%" 
   %include "../tail.h" 

   %manual

      There can be multiple manual pages in the same file.
      When viewed they are simply concatenated.
   %end

A manual page can also be added to a family and suite node.

To add a manual page for family :code:`f1`, create a file named :file:`f1.man`
with the following content in the directory :file:`{{HOME}}/course/test`:

.. code-block:: shell
   :caption: $HOME/course/test/f1.man (Family Manual Page)

   This manual is for family %FAMILY%
   It can have any text and will also have variable substitution
   Notice that this does not have manual..end since the whole file is a manual page.

To add a manual page for suite :code:`test`, create a file named :file:`test.man``
with the following content in the directory :file:`{{HOME}}/course`:

.. code-block:: shell
   :caption: $HOME/course/test.man (Suite Manual Page)

   %manual
   This is the manual page for the %SUITE% suite.
   It lives in %ECF_HOME%
   %end
   This text is not visible in the man page since it is out side of the %manual..%end

.. important::

   Notice that for family and suite nodes, the enclosure in :code:`%manual ... %end`
   is not strictly necessary. However, adding :code:`%manual ... %end` allows to decide
   what parts are made visible when using :term:`ecflow_ui`.

**What to do**

#. Add the task manual to :file:`t2.ecf` script
#. Add the file :file:`f1.man`
#. Add the file :file:`test.man`
#. View the :term:`manual page` for :term:`task` :code:`t2` in :term:`ecflow_ui`
#. View the :term:`manual page` for :term:`family` :code:`f1` in :term:`ecflow_ui`
#. View the :term:`manual page` for :term:`suite` :code:`test` in :term:`ecflow_ui`
