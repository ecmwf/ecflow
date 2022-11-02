.. _ecflow_check_point_file_and_failure_tolerance:

ecFlow check point file and failure tolerance
/////////////////////////////////////////////


There are a number of reasons that could cause ecFlow to stop working
(server crashes, the computer ecFlow is running on crashes, etc.). The
ecFlow checkpoint file allows ecFlow to restart at the point of the
last checkpoint before a failure. This gives reasonable tolerance
against failures.

When the server starts, if the checkpoint file exists and is readable
and is complete, the ecFlow server recovers from that file. Once
recovered the status of the server may not exactly reflect the real
status of the suite, it could be up to a few minutes old. Tasks that
were running may have now completed so the task status should be
checked for consistency.

The checkpoint files can be read by any ecFlow running on any
operating system
  
There are two separate checkpoint files::

  ECF_CHECK     ecf.check
  ECF_CHECKOLD  ecf.check.b                                   

When ecFlow needs to write a checkpoint file it first moves (renames)
the previous file ECF_CHECK to ECF_CHECKOLD and then creates a new file
with the name ECF_CHECK. This means that you should always have a file
that is good. If a crash happens while writing ECF_CHECK, you can still
recover from ECF_CHECKOLD (by copying its contents to ECF_CHECK),
although that version is not quite as up to date.

.. tip::

    You can copy the checkpoint files between systems.            

Another ecFlow server can be started with the original server's
checkpoint file and take over from the original ecFlow server host in
case of a catastrophic system failure.
