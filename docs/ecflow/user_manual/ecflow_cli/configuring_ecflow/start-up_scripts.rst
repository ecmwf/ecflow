.. _start-up_scripts:

Start-up scripts
////////////////

Start-up scripts are another useful way of starting and configuring ECF.
There is an example included in the default installation of ECF;
ecflow_start.sh.

Scripts like these can be used to reconfigure some of the default ecFlow
variables and check that ecFlow is not already running (using
ecflow_client --ping), set the environment (or do it in .profile or
.cshrc), backup log files and checkpoint files, start ecflow_server in
the background and alert operators if there is a problem starting.
