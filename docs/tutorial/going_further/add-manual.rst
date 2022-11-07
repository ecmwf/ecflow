.. index::
   single: manual (tutorial)

.. _tutorial-manual:

Add Manual
==========
  
A :term:`manual page` allows documentation **in** a :term:`ecf script` to be viewable in :term:`ecflow_ui`.

The manual page is the concatenation of all the text within the %manual and %end :term:`directives`.

Modify :file:`t2.ecf` to have the following: 

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

A manual page can also be added to a family and suite node. Create a file called :file:`f1.man`` in **$HOME/course/test** as the FAMILY  man page.

.. code-block:: shell
   :caption: $HOME/course/test/f1.man ( FAMILY MAN PAGE )

   This manual is for family %FAMILY%
   It can have any text and will also have variable substitution
   Notice that this does not have manual..end since the whole file is a manual page.

Now create the file :file:`test.man`` in  **$HOME/course**  as the SUITE man page:

.. code-block:: shell
   :caption: $HOME/course/test.man ( SUITE MAN PAGE )

   %manual
   This is the manual page for the %SUITE% suite.
   It lives in %ECF_HOME%
   %end
   This text is not visible in the man page since it is out side of the %manual..%end

Notice that for family and suite nodes that the addition %manual..%end is not strictly necessary. However by adding %manual..%end you can decide what parts are made visible to the GUI.


**What to do**

#. Modify :file:`$HOME/course/test/t2.ecf` script
#. Add the file :file:`$HOME/course/test/f1.man`
#. Add the file :file:`$HOME/course/test.man`
#. View the :term:`manual page` for :term:`task` **t2** in :term:`ecflow_ui`
#. View the :term:`manual page` for :term:`family` **test/f1** in :term:`ecflow_ui`
#. View the :term:`manual page` for :term:`suite` **test** in :term:`ecflow_ui`
