.. _ecf_micro:

ECF_MICRO
/////////

The variable **ECF_MICRO** can be set to change the ecFlow
micro-character. This affects both the commands and the interpretation
of the ecFlow files. The default value is "%".

An example showing changing the micro-character follows:

.. code-block:: shell
    :caption: Define ECF_MICRO in definition file

    suite x
        edit ECF_MICRO "&"
        family f
            task t

.. code-block:: shell
    :caption: t.ecf

    &include <head.h>
    echo job here
    &include <end.h>

Another way of changing the micro-character is to set it up in the
ecFlow script. It only affects the script interpretation, not the
commands ECF_JOB_CMD or ECF_KILL_CMD.

.. code-block:: shell
    :caption: Define ecfmicro directly in the script

    #!/bin/bash
    %ecfmicro ^
    ^include <script.pl>
    echo "^VAR:default^"
    ^ecfmicro %
    echo "reverted %VAR:default%"