.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _server_environment.cfg:

server_environment.cfg
//////////////////////


The file server_environment.cfg can be found in the ecFlow/Server
folder. Modifying it changes the default environment variables
for ecFlow, including the default commands for submitting and killing
tasks.

.. warning::

    The server looks for the file in the current working directory; move the file accordingly.

A missing, empty or comments-only configuration file uses built-in defaults.
An existing configuration path must name a readable regular file; a directory, an
unreadable file or a read error stops initialisation with the path and cause.
An unknown or obsolete setting is a parsing error, and also stops initialisation.

Explicitly empty required paths and command strings are rejected after environment
overrides, before filename resolution, changing directory or opening the log.
This includes the server home, log, checkpoint and security-file paths, configured
commands, URL settings and preprocessing character. Omitting a setting still selects
its default; a supported non-empty environment override can replace an empty config value.

``ECF_TASK_THRESHOLD`` sets the job-profiling threshold in milliseconds, with a
default of 4000. Environment settings override the configuration-file value.
The effective value must be a positive integer; zero and negative values stop startup
before the value is converted to the profiler's unsigned representation.

``ECF_PRUNE_NODE_LOG`` is a non-negative number of days. Zero preserves all edit
history when recovering a checkpoint. The default is 30 days, and the environment
variable of the same name overrides the configuration-file value.

``ECF_CHECKINTERVAL`` sets the checkpoint interval in seconds, with a default of 120.
The environment variable of the same name overrides the configuration-file value.

The effective interval after overrides must be a positive integer; zero and negative values
stop server initialisation with a diagnostic identifying the setting and its value.

``ECF_CHECKMODE`` accepts the following case-sensitive values:

- ``CHECK_NEVER``: disable automatic checkpointing.
- ``CHECK_ON_TIME``: save checkpoints periodically.
- ``CHECK_ALWAYS``: save a checkpoint after each state change.

When the setting is omitted, the default is ``CHECK_ON_TIME``. An explicitly empty or unsupported
value stops server initialisation; the diagnostic identifies the configuration file, the invalid
value and the supported modes.

The example below shows some of the default variables that can be configured.

.. code-block:: shell
    :caption: server_environment.cfg

    # SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
    # SPDX-License-Identifier: Apache-2.0

    #
    # This file is used to define the standard defaults for ECF.
    # Most are *variables* used in the server
    # Some like ECF_TASK_THRESHOLD are used to debug job generation

    # ******************************************************************
    # Warning: Do *NOT* use quotes around the value part.
    #   WRONG: ECF_MICRODEF = "%"
    #   RIGHT: ECF_MICRODEF = %
    # ******************************************************************

    #  *******************************************************************
    #  * ECF_HOME is typically the home/root for all '.ecf' files
    #  * Can be overridden with a environment variable of the same name
    #  *******************************************************************
    ECF_HOME  = .


    #  ******************************************************************
    #  * The name of check point file. i.e. defs file with state
    #  * Can be overridden with a environment variable of the same name
    #  * default: is <host>.<port>.ecf.check
    #  * Note: Any settings will be prepended with <host>.<port>.
    #  ******************************************************************
    ECF_CHECK = ecf.check


    #  ******************************************************************
    #  * The name of the backup checkpoint file
    #  * Can be overridden with a environment variable of the same name
    #  * default: is <host>.<port>.ecf.check.b
    #  * Note: Any settings will be prepended with <host>.<port>.
    #  ******************************************************************
    ECF_CHECKOLD = ecf.check.b


    #  ******************************************************************
    #  * The intervals within the server that the checkpoint file should
    #  * be saved.
    #  * Can be overridden with a environment variable of the same name
    #  ******************************************************************
    ECF_CHECKINTERVAL = 120


    #  ******************************************************************
    #  * Check point configuration:
    #  *
    #  * Mode must be one of:
    #  *   CHECK_NEVER    /* No auto checkpointing                       */
    #  *   CHECK_ON_TIME  /* At intervals specified by ECF_CHECKINTERVAL */
    #  *   CHECK_ALWAYS   /* After any change                            */
    #  * The checkpoint filenames can be configured using environment variables
    #  ******************************************************************
    ECF_CHECKMODE = CHECK_ON_TIME


    #  ******************************************************************
    #  * The port number, this must be consistent between client and server
    #  * If we get "Address in use" then both client/server number should changed.
    #  * Also if two servers are started on the same machine with same port
    #  * then we will also get "Address in use" error and server will bomb out.
    #  * Can be overridden with a environment variable of the same name
    #  ******************************************************************
    ECF_PORT = 3141


    #  ******************************************************************
    #  * The name of log file.
    #  * default log file name is: <host>.<port>.ecf.log, i.e. machine1.3141.ecf.log
    #  * this is required since we can have multiple servers for a single
    #  * machine, where each server will have a separate port number.
    #  * Can be overridden with a environment variable of the same name
    #  *
    #  * Note: Any settings will be prepended with <host>.<port>.
    #  ******************************************************************
    ECF_LOG = ecf.log


    #  ******************************************************************
    #  * The period in second for which we should traverse dependencies
    #  * and submit jobs. This should *RARELY* need changing, as it can affect
    #  * correspondence with real time
    #  ******************************************************************
    ECF_INTERVAL = 60


    #  ******************************************************************
    #  * The standard command use for job submission.
    #  *
    #  * Provides DEFAULT which can be overridden by a user variable
    #  ******************************************************************
    ECF_JOB_CMD = %ECF_JOB% 1> %ECF_JOBOUT% 2>&1


    #  ******************************************************************
    #  * Define the command for killing any jobs.
    #  * The output of the command should be written to %ECF_JOB%.kill
    #  *
    #  * Provides DEFAULT which can be overridden by a user variable
    #  *
    #  * ecmwf: ${ECF_KILL:=/home/ma/emos/bin/ecfkill} %USER% %HOST% %ECF_RID% %ECF_JOB% > %ECF_JOB%.kill 2>&1
    #  ******************************************************************
    ECF_KILL_CMD = kill -15 %ECF_RID%


    #  ******************************************************************
    #  * The command to be executed, by the SERVER, to obtain the
    #  * current status of a Job.
    #  * The output of the command should be written to %ECF_JOB%.stat
    #  *
    #  * Provides DEFAULT which can be overridden by a user variable
    #  *
    #  * ecmwf: ${ECF_STAT:=trimurti} %USER% %HOST% %ECF_RID% %ECF_JOB% status> %ECF_JOB%.stat 2>&1
    #  ******************************************************************
    ECF_STATUS_CMD = ps --pid %ECF_RID% -f > %ECF_JOB%.stat 2>&1

    #  ******************************************************************
    #  * The command to be executed, by the CLIENT (ecFlowUI), to obtain the
    #  * current status of a Job.
    #  *
    #  * DEPRECATED: This command is deprecated and currently not used.
    #  *             Use ECF_STATUS_CMD instead as a SERVER side alternative
    #  *
    #  ******************************************************************
    ECF_CHECK_CMD = ps --pid %ECF_RID% -f


    #  ******************************************************************
    #  * The command to be executed, bye the CLIENT (ecFlowUI) to open a
    #  * Browser with the specified URL.
    #  *
    #  * Provides DEFAULT which can be overridden by a user variables
    #  ******************************************************************
    ECF_URL_CMD    = ${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%
    ECF_URL_BASE   = https://confluence.ecmwf.int
    ECF_URL        = display/ECFLOW/ecflow+home

    #  ******************************************************************
    #  * Defines the character used in ECF_ pre-processing. i.e. identifies includes
    #  * and is also used in variable substitution in '.ecf' scripts
    #  ******************************************************************
    ECF_MICRODEF = %

    #  ******************************************************************
    #  * The ECF_LISTS is used to identify a file, that lists the user
    #  * who can access the server via client commands. Each client command
    #  * (ignoring task based commands, i.e. init, complete, event, meter, label)
    #  * will encode the user name of the process initiating the client request
    #  * This is then compared with list of users in the ecf.lists file.
    #  * If this file is empty, then no authentication is done
    #  * Each server can potionally have a different list.
    #  * default: <host>.<port>.ecf.lists
    #  * Note: Any settings will be prepended with <host>.<port>.
    #  ******************************************************************
    ECF_LISTS = ecf.lists

    #  ******************************************************************
    #  * Password file: Use when every user needs password authentication
    #  * Each server can potionally have a different password file.
    #  * default: <host>.<port>.ecf.passwd
    #  * Note: Any settings will be prepended with <host>.<port>.
    #  ******************************************************************
    ECF_PASSWD = ecf.passwd


    #  ******************************************************************
    #  * Password file:, when only a few users need password, typically
    #  * when a users UID on the remote client, does not match that on the server
    #  * Each server can potionally have a different password file.
    #  * default: <host>.<port>.ecf.custom_passwd
    #  * Note: Any settings will be prepended with <host>.<port>.
    #  ******************************************************************
    ECF_CUSTOM_PASSWD = ecf.custom_passwd

    # ***************************************************************************
    # * ECF_TASK_THRESHOLD:
    # * Report on an task taking longer than the threshold. !!
    # *    export ECF_TASK_THRESHOLD=4000
    # ***************************************************************************
    ECF_TASK_THRESHOLD = 4000
