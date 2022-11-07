.. _manual_pages:

Manual pages
///////////////////

**Manual pages** are part of the ecFlow script.(i.e. .ecf or .py, etc).
This is to ensure that the manual page is updated when the script is
updated. The manual page is a very important operational tool allowing you to view a
description of a task, its importance, task dependencies and possibly
describing solutions to common problems. The ecFlow pre-processor can be used to **extract**
the manual page from the script file to be displayed in the GUI. The manual page is the text contained within the %manual and %end tags. They can be seen
using the manual tab in the info panel in :ref:`ecflow_ui`.

Manual pages are a vital source of information for users. The text on 
manual pages is not copied into the job-file when ecFlow sends a task 
into execution. Suites, families, and tasks can have manual pages.    
Manual pages for tasks are placed in the ecFlow script inside a pair  
of **pre-processor lines** as in the following example:               

.. code-block:: shell

    %manual
        OPERATORS: If this task fails, set it complete and report
        next working day
        ANALYST: Check something or do something clever! %end
        ls -l pwd hostname %manual Rest of the manual page is placed here, closer to the code
    %end


There can be multiple manual sections in the same file. When viewed they
are simply concatenated. This helps in maintaining the manual pages. It
is good practice to modify the manual pages when the script is changed.

Viewing manual page from the above ecFlow script would look something
like::

    OPERATORS: If this task fails, set it complete and report the next
    working day
    ANALYST: Check something or do something clever!

Rest of the manual page is placed here, closer to the code. After **%manual** all pre-processor symbols are ignored until **%end**
is found. Thus you cannot use **%comment** - **%end** to **un-comment**
manual pages.

Manual pages may have **include statements** like in the following
extract:

.. code-block:: shell

    %manual
        OPERATORS: If this task fails, set it complete and report
        and next working day
        ANALYST: Check something or do something clever!
    %include <manual/foo.bar>
    %end
        ls -l
        pwd
        hostname
    %manual
        Rest of the manual page is placed here
    %end


For example, standard instructions for operators could be placed in a
single file and then included in every task (like contact phone numbers,
etc.) How the include file is found is explained in the next section.

Suites and families can also have manual pages. However, these are
separate files placed in the suite/family directories e.g. the manual
page for a family **family1** is a file family1.man in the relevant directory.
