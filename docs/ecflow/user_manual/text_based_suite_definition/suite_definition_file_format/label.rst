.. _label:

label
///////

A label has a name and a value and is a way of displaying information
in ecflow_ui. Since the value can be anything (ASCII) it cannot be
used in triggers.

.. code-block:: shell

  task x
    label name string
    label OBS 0
    label file "" # for empty label


The value of the label is set to be the default value given in the
definition file when the suite is begun. This is useful in repeated
suites: a task sets the label to be something, e.g. the number of
observations, and once the suite is complete (and the next day starts)
the number of observations is cleared.

Using labels
===================

In order to use labels you have to first define the label in the suite
definition file, e.g.

.. code-block:: shell

  suite x
    family f
      task t
          label foo ""

foo is the "name" of the label and the empty string is the default
value of the label (the value is shown when the suite begins). After
the command begins it looks like:

In an ecFlow job file, you can then modify your task to change the
label while the job is running, e.g.

.. code-block:: shell

  ecflow_client --init=$$
  ecflow_client --label=foo "some text"
  ecflow_client --complete

After the job has modified the label it looks like:

If you want to send more than one line, use spaces in the text, e.g.

.. code-block:: shell

  ecflow_client --init=$$
  ecflow_client --label=foo multi line label
  ecflow_client --complete

And to have the display lined up better, send the first line as empty:

.. code-block:: shell

  ecflow_client --init=$$
  ecflow_client --label="" multi line label
  ecflow_client --complete
