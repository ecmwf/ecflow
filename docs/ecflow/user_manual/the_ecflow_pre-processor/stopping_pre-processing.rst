.. _stopping_pre-processing:

Stopping pre-processing
///////////////////////

The ecFlow pre-processor allows parts of the ecFlow script to be
included *as is* of without being pre-processed. This was done mainly to
make it easy to use languages such as **perl** which make significant use of the %-sign.

Pre-processing can be stopped in two ways:

* By using a pair of lines: **%nopp** and **%end** which will completely stop the pre-processing between those lines

* By using **%includenopp filename** which will include the file as is without any interpretation.This makes it easy to test the script separately but allows it to be edited by ecflow_ui.

.. code-block:: shell

    %nopp echo "char like % can be safely used here"
        date +%Y.%m.%d
    %end

    echo "otherwise we must write"
 
    date +%%Y.%%m.%%d
