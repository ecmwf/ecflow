.. _ecflow_ui_web_menu:

ecflow_ui web menu
///////////////////

Some might notice that what was known as "Web menu" in ecflowview does
not appear in the latest GUI.

Just click menu **User Defined > Manage Commands > Command**

.. code-block:: shell

    sh %ECF_URL_CMD% # Web                                             

Then **Save Option > sName "Web" > Save As New**

You can then connect a node, Suite, Family, Task, to a web page like a
project front end, html man page, third party monitoring tool ...
ecflowview has also a "second level menus" in order to access man
pages, job file, output, when a suite stores this information as
dedicated (user) variables, and not the default (ecflow) variables
ECF_JOB, ECF_JOBOUT, ...

Here are the commands below, you can define as extra menus in ecflow_ui

.. code-block:: shell

  firefox %ECF_URL_BASE%/%ECF_URL% # Man
  sh xterm -T Script -e vim %ECF_SCRIPT% # Script
  sh xterm -T Job -e vim %ECF_JOB% # Job
  sh xterm -T Output -e vim %ECF_JOBOUT% # Output
  xterm -T Details-%ECF_NAME% -e "grep %ECF_NAME% %ECF_LOG% ; read" # Detail
