.. _variables_and_substitution:

Variables and substitution
//////////////////////////

This section describes how to use suite definition **variables** in
ecFlow script files.

Suite definition variables are defined by the `edit <https://confluence.ecmwf.int/display/ECFLOW/Definition+file+Grammar>`__
keyword. The section on `ecFlow
pre-processor <https://confluence.ecmwf.int/display/ECFLOW/The+ecFlow+Pre-processor>`__
gives more information on how variables are used.

In an ecFlow script, variables are written as text enclosed by a pair
of '%' characters (the **micro-character**). As in C-format strings,
if there are two %-characters together they are concatenated to form a
single %-character in the job-file.

For example, if you need to execute UNIX command:

.. code-block:: shell

  date +%d

In a job, you must enter it as following into an ecFlow file:

.. code-block:: shell

  date +%%d  

At present, the default micro-character is %. It can only be defined
when ecFlow is compiled. It can be redefined by setting the variable
**ECF_MICRO**.

A user defines variables in a suite definition file using the **edit**
keyword. User-defined variables can occur at any node level: suite,
family, or task. ecFlow also **generates** variables from the node
name, the host on which ecFlow is running, the time, the date, and so
on.

When a variable is needed at submission time, it is first sought in
the task itself. If it is not found in the task, it is sought from the
task's parent and so on, up through the node levels until found. For
any node, variables are looked for in the following order:

-  The user-defined variables are looked for first

-  Repeat name, in which case current repeat value is used

-  Finally generated variables.

An undefined variable causes a task to abort immediately, without the
job being submitted. A **flag** is set in the task and an entry is
written into ECF-logfile. If this is too severe you can use default
variables in your scripts

.. code-block:: shell

  %VAR:value%

If variable "VAR" is not found, then we use a default value of "value". Clever use of variables can, however, save a lot of work. For example,
you can use the same script in multiple places, but configure it to
behave differently depending on the variable set.

ECF_TRIES and ECF_TRYNO
=======================

If you have set ECFTRIES in your definition file to be greater than one
then your task will automatically rerun on an abort. You can then use
the ecFlow variable ECF_TRYNO to modify the behaviour of your tasks
dependant on the try number, e.g.

.. code-block:: shell

  #QSUB -o %ECFBASE%/log%ECFNAME%.%ECF_TRYNO%
  if [ %ECF_TRYNO% -gt 1 ] ; then
    DEBUG=yes
  else
    DEBUG=no
  fi
