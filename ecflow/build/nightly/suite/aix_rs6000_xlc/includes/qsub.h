#!/bin/ksh
#@ shell           = /usr/bin/ksh

        # Specifies the shell that parses the script. If not
        # specified, your login shell will be used.

#@ class           = large

        # Specifies that your job should be run in the class (queue)
        # express.

#@ job_name        = %TASK%.%ECF_TRYNO%

        # Assigns the specified name to the request

#@ output          = %ECF_JOBOUT%

        # Specifies the name and location of STDOUT. If not given, the
        # default is /dev/null.  The file will be written in the
        # submitting directory, by default.

#@ error           = %ECF_JOBOUT%

        # Specifies the name and location of STDERR. If not given, the
        # default is /dev/null.  The file will be written in the
        # submitting directory, by default.

#@ environment     = COPY_ALL

        # Specifies that all environment variables from your shell
        # should be used. You can also list individual variables which
        # should be separated with semicolons.

#@ notification    = error

        # Specifies that email should be sent in case the job failes.
        # Other options include always, complete, start, and
        # never. The default is notification = complete.

#@ job_cpu_limit   = 5:01:00,5:00:55

        # Specifies the total CPU time which can be used by all
        # processes of a serial job step. In this job the hard limit
        # is set to 1 min and the soft limit to 55 sec. Note: All
        # limits are capped by those specified in the class.

#@ wall_clock_limit= 12:05:00,12:04:50

        # Specifies that your job requires HH:MM:SS of wall clock time.

# =============================================================
# please note the queue is automatically added by smssubmit
# If we add it here, we will have two jobs, where one would be a zombie
# ==============================================================
# The queue statement marks the end of your LoadLeveler
# keyword definitions and places your job in the queue. At least
# one queue statement is mandatory. It must be the last keyword
# specified. Any keywords placed after this in the script are
# ignored by the current job step.

#-------------------------------
# print the current directory and its content
#-------------------------------

echo "
Please note that the initial directory is the current working
directory at the time you submitted the job:"

pwd     # prints the path name of the current directory

echo "
Using the LoadLeveler keyword initialdir you can specify the path name
of the directory to be used as the initial working directory during the
execution of the job. For example, you can add

"
echo "#@ initialdir = %ECF_HOME%

to your loadleveler script (and modify the given path).
"

#-------------------------------
# change to %ECF_HOME%
#-------------------------------

cd %ECF_HOME%

echo "
List of the content of the current directory:
"
ls -l

#-------------------------------
# show your PATH
#-------------------------------

echo "
This is your path:
"
echo $PATH
echo "
All the commands will be searched for in these directories
unless  you give an absolute pathname for the command."

