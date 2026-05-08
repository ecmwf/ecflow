.. _edit_add_variable:

variable
////////

This defines a variable for ecFlow job substitution in a node, equivalent to an external variable.
There can be any number of variables defined at any node level: suite, family or task.

The textual definition of a variable is as follows, where :code:`<name>` is the name of the variable and
:code:`<value>` is the value of the variable. Variable names are case sensitive.

After the variable name and value, an optional comment can be provided. The comment must be preceded by
a :code:`#` character, after the variable value. The comment is ignored, and serves only as a note for
users reading the suite definition.

.. code-block:: shell

    edit <name> <value>
    edit <name> <value> # optional comment
    edit <name> '<value with spaces>'
    edit <name> "<value with spaces>" # optional comment

.. important::

    **Use single quotes to delimit the value of a variable.**

    Although not mandatory, it is a highly recommended practice to *always* use single quotes to enclose
    the value of a variable, even if it does not contain spaces. This avoids any misinterpretation of special
    characters, including whitespace, in the value and ensures that the variable value is treated as a single entity.

.. hint::

     Define each variable on a separate line, and avoid defining multiple variables on the same line.
     This enhances readability and maintainability of the suite definition.

To use the variable in an ecFlow task script refer to it by its name, inside a pair of :code:`%` (:code:`ECF_MICRO`)
characters e.g. :code:`VARIABLE`, taking care to exactly match the case of the variable name.
The replacement of the variable name, surrounded by :code:`ECF_MICRO`, by the value of the variable is performed
during the job script rendering at submission time.

.. code-block:: shell

    edit ECF_JOB_CMD '/bin/sh %ECF_JOB% &'
    edit ECF_JOB_CMD '/usr/local/bin/qsub %ECF_JOB%'
    edit ECF_JOB_CMD 'rsh %ECF_HOST% sh <%ECF_JOB% 1>%ECF_JOBOUT% 2>&1'
    edit KEEPLOGS no  

When defining a variable value, care is needed to ensure that the value is correctly interpreted.

* An unquoted variable value is determined by the first whitespace character after the variable name,
  and the comment character :code:`#` is expected to appear after the value when an optional
  comment is provided. If more content appears after the unquoted value before the presence of a
  comment character :code:`#` the variable definition is considered invalid.

* If a quote is used, determined by the presence of either :code:`'` or :code:`"` after the variable name,
  the variable value includes all the content between the first and last occurrence of the quotation marker
  in the line, potentially making the comment character :code:`#` part of the variable value if it appears
  within the quotes.

The following examples present invalid variable definitions:

.. code-block:: shell

    edit SCRIPT /bin/sh script.sh &

Since the value above is unquoted, the variable value is interpreted as simply `/bin/sh`
and the expected comment marked `#` is missing after the value.

.. code-block:: shell

    edit SCRIPT '/bin/sh script.sh &' # it's a comment

Since the value above is single quoted, determined by the opening single quote of the value,
the value is interpreted as `/bin/sh script.sh &' # it` (delimited by the last `'` that
appears in the entire line) and the expected comment marked `#` is missing after the value.

Server variables
----------------

Variables defined at the top level can either be considered user or server variables.
The definition of server variables is reserved for ecFlow administrators, and are
relevant only in the context of checkpoint files.

.. warning::

    Server variables should not be used by users defining their own suites.

In checkpoint files, Server variables are identified by the presence of the comment
:code:`# server` after the variable value, as shown in the example below.

.. code-block:: shell

    edit ECF_HOME '/home/ecflow' # server
    edit ECF_PORT '3141' # server
    edit ECF_HOST 'ecflow-hostname' # server
