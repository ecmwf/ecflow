.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

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

Limitations
===================

Label values are stored between double quotes, without escaping. The
default value of a label, that is, the value given in the suite
definition, must therefore not contain a double quote that is followed
by blanks and a hash character, in other words the sequence
``" #``. A default value containing this sequence may be truncated, and
the current value of the label may be lost or read incorrectly, when the
definition is loaded, when the server restarts from its checkpoint file,
or when the suite is synchronised to ecflow_ui or to a client.

To stay clear of the limitation:

- Do not use ``" #`` (a double quote, one or more spaces or tabs, then
  ``#``) anywhere in a default value. In particular, do not end a default
  value with ``" #``, with or without trailing blanks, and do not embed a
  quoted fragment followed by a comment-like ``#``, such as
  ``see "note" # important``.

- Any other content is allowed in a default value: a ``#`` on its own
  (``"#40fd83"``, ``"a # b"``), double quotes on their own
  (``""note""``, ``"say "hi""``), apostrophes, tabs, leading or trailing
  spaces, and newlines written as ``\n``.

- In a hand-written definition file, a ``#`` after the closing quote
  starts a comment (``label file "" # for empty label``). If the value
  itself must contain ``" #``, delimit the value with single quotes
  instead:

  .. code-block:: shell

    label info 'see "note" # important'

  This makes the definition load correctly. The rule above still applies
  once the server writes the value back with double quotes, so this is a
  way of writing comments, not a way of lifting the rule.

- The current value of a label, set from a job with
  ``ecflow_client --label`` or with ``ecflow_client --alter change label``,
  may contain ``#``, double quotes, ``" #``, or any combination of them,
  and is always read back correctly. When it contains the separator
  sequence ``" # "`` itself, the line can no longer be told apart from a
  default value that contains it, so the server logs a warning naming the
  label and the values read each time the checkpoint is restored or the
  suite is synchronised; the values are nevertheless the expected ones.

- A value cannot contain the two characters ``\n`` literally, since they
  are read back as a newline, and only one ``label`` may be written per
  line.
