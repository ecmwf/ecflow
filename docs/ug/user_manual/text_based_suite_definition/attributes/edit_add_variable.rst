.. _edit_add_variable:

Edit-add variable
///////////////////

This defines a variable for ecFlow job substitution in a node,
equivalent to an external variable. There can be any number of
variables. The variables are names inside a pair of \`%'(ECF_MICRO)
characters in an ecFlow script. However, remember that ecFlow is case
sensitive.

The content of a variable replaces the variable name in the ecFlow
script at submission time. Special characters in a definition must be
placed inside single quotes if misinterpretation is to be avoided or
inside double quotes variable substitution is to be carried out. Quotes
are needed if defining as a list.

.. code-block:: shell

    edit ECF_JOB_CMD "/bin/sh %ECF_JOB% &"
    edit ECF_JOB_CMD "/usr/local/bin/qsub %ECF_JOB%"
    edit ECF_JOB_CMD "rsh %ECF_HOST% sh <%ECF_JOB% 1>%ECF_JOBOUT% 2>&1"
    edit KEEPLOGS no  


Submission is done via a **system(3)** call which executes /bin/sh.
