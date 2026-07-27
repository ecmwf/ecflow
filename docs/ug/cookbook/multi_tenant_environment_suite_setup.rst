.. _multi_tenant_environment_suite_setup:

Suite Setup for the Multi-Tenant Environment
********************************************

.. warning::

   This recipe is **experimental** and a **work in progress**.

   It describes an internal, development-oriented reference deployment of the multi-tenant ecFlow
   server, which is not yet ready for production use. The stack, the commands, and the configuration
   shown here may change at any time. Treat it as a starting point for experimentation rather than a
   supported deployment procedure.

.. note::

   This note is an early **draft**. Several details are still under investigation and are marked
   ``TBC`` (to be confirmed) below. The structure and the concrete values are expected to change as
   the multi-tenant environment matures.

This note describes the changes a suite requires, relative to a regular suite, in order to run
against the deployment documented in :ref:`multi_tenant_reference_environment`. It also describes
the adjustments required by the ecFlow clients (``ecflow_client``, ``ecflow_ui``) to reach the server
through the authenticating reverse proxy, which is required for both user and task commands.

The multi-tenant deployment places the ecFlow server behind an authenticating reverse proxy: a suite
running there is expected to differ from a regular suite in two respects:

- the job-management commands submit, monitor, and kill jobs using a user-specific configuration;

- the child commands issued by the task scripts must reach the server through the authenticated HTTPS
  path.

Both aspects are controlled through ecFlow variables and a small number of adjustments to the task scripts.

The remainder of this note is organised as follows:

- `ecFlow variable customisation`_ enumerates the variables to override and the values they take in
  this environment;

- `Task script changes`_ describes how the task scripts use those variables.

- `User commands`_ describes how the ecFlow clients (``ecflow_client``, ``ecflow_ui``) must be configured
  to reach the server through the authenticating reverse proxy.

- `Open questions`_ lists the points that remain to be confirmed before this note can be considered
  complete.

ecFlow variable customisation
=============================

The variables below are defined at the suite level (or on any node whose subtree requires them), so
that every task inherits them. Unless noted otherwise, they override values that a regular suite
would either inherit from the server or omit entirely.

Job management commands
-----------------------

In this environment, jobs are submitted, monitored, and killed through `Troika
<https://github.com/ecmwf/troika>`_. The three job-management commands are redefined accordingly.

.. note::

   The exact Troika invocation, including the host argument, the configuration file, and the output
   handling, is **TBC**. The commands below illustrate the intended shape and are expected to be
   refined against a working Troika configuration.

Because the ecFlow server is running inside a container, the Troika client must be invoked
through the ``troikaw`` wrapper script, which automatically loads the Python virtual environment
and the Troika configuration. The wrapper is located in the same directory as the suite home
(``%ECF_HOME%``). The commands are therefore defined as follows:

.. code-block:: shell

   # Submit a job through Troika
   edit ECF_JOB_CMD    '%ECF_HOME%/troikaw -c %ECF_HOME%/troika.yml submit -o %ECF_JOBOUT% %TROIKA_TARGET% %ECF_JOB%'

   # Kill a running job through Troika
   edit ECF_KILL_CMD   '%ECF_HOME%/troikaw -c %ECF_HOME%/troika.yml kill %TROIKA_TARGET% %ECF_JOB%'

   # Query the status of a job through Troika
   edit ECF_STATUS_CMD '%ECF_HOME%/troikaw -c %ECF_HOME%/troika.yml monitor %TROIKA_TARGET% %ECF_JOB%'

In the above commands, ``%TROIKA_TARGET%`` denotes the Troika-configured destination host.
The command definitions follow the conventions described in :ref:`the suite definition variables reference
<ecflow_suite_definition_variables>`: :code:`ECF_KILL_CMD` and :code:`ECF_STATUS_CMD` are expected
to redirect their output to :code:`%ECF_JOB%.kill` and :code:`%ECF_JOB%.stat`, respectively.

.. important::

   A single Troika configuration file is currently shared by all the tenants in this environment.
   This configuration still needs to be refined to support submission with per-user credentials.

   An approach under consideration is to define a separate Troika site for each tenant (``troika.<user>.yml``),
   with the ``user`` and ``host`` fields set to the tenant's credentials. The suite would then select the
   appropriate site by defining the ``ECF_JOB_CMD`` variable using ``-c %ECF_HOME%/troika.%USER%.yml``.

As a reference, the ``troikaw`` wrapper script is shown below. It is a simple shell script that loads the Python
virtual environment provided in the ecFlow server container and invokes the Troika client with the provided arguments

.. code-block:: shell

   #!/usr/bin/env bash

   # A wrapper for troika, that activates the virtual environment before running troika

   set -euo pipefail

   VENV="${TROIKA_VENV:-/home/<user>/<ecf_home>/py3}"

   if [[ ! -f "${VENV}/bin/activate" ]]; then
       echo "troikaw: no venv activate script at ${VENV}/bin/activate" >&2
       exit 1
   fi

   source "${VENV}/bin/activate"

   exec troika "$@"

The current Troika configuration file is shown below. It defines the ``atos`` site, which is used by all
the HPC login and batch nodes in the multi-tenant environment. The configuration is expected to be
refined against a working Troika deployment, and the host names and SSH options are **TBC**.

.. code-block:: yaml

   ---
   sites:
       localhost:
           type: direct
           connection: local

       atos: &atos
           type: slurm
           connection: ssh
           ssh_connect_timeout: 30
           user: <user>
           host: ad-batch
           ssh_options: ["-i", "/home/<user>/path/to/secret/key"]
           copy_script: true
           at_startup: ["check_connection"]
           pre_submit: ["create_output_dir", "copy_orig_script", ]
           preprocess: ["remove_top_blank_lines", "slurm_add_output", "slurm_bubble"]
           at_exit: ["copy_submit_logfile"]
           sbatch_command: "ecsbatch"
           scancel_command: 'ecscancel'
           kill_sequence: [[5, 2], [5, 15]]
           directive_translate:
               tmpdir_size: "--gres=ssdtmp:%s"

       hpc-login:
           host: hpc-login
           <<: *atos
           # preprocess: []

       hpc-batch:
           host: hpc-batch
           <<: *atos
           # preprocess: []

       aa:
           host: aa-login
           <<: *atos

       aa-batch:
           host: aa-batch
           <<: *atos
           # preprocess: []

       ab:
           host: ab-login
           <<: *atos

       ab-batch:
           host: ab-batch
           <<: *atos
           # preprocess: []

       ac:
           host: ac-login
           <<: *atos

       ac-batch:
           host: ac-batch
           <<: *atos
           # preprocess: []

       ad:
           host: ad-login
           <<: *atos

       ad-batch:
           host: ad-batch
           <<: *atos
           # preprocess: []

Access permissions
------------------

Because the reverse proxy authenticates every request and forwards the authenticated identity to the
server (through the ``X-Auth-Username`` header), the server can enforce per-user access controls on
the suite nodes. These controls are expressed through the :code:`ECF_PERMISSIONS` variable.

The value of :code:`ECF_PERMISSIONS` is a comma-separated list of ``<user>:<rights>`` entries. The
rights of each entry are a combination of the following characters (case-insensitive, spaces
ignored):

.. list-table::
   :header-rows: 1
   :widths: 10 15 75

   * - Character
     - Right
     - Meaning
   * - ``r``
     - read
     - Read nodes and their attributes.
   * - ``w``
     - write
     - Modify nodes and their attributes.
   * - ``x``
     - execute
     - Execute commands on nodes (for example, run a task).
   * - ``o``
     - owner
     - Load new suites onto the server.
   * - ``s``
     - sticky
     - Prevent the rights of this entry from being restricted lower in the node hierarchy.

For example, granting ``alice`` full access to a suite, ``bob`` read-only access, and ``charlie``
read/write access:

.. code-block:: shell

   edit ECF_PERMISSIONS 'alice:rwx,bob:r,charlie:rw'

The variable is evaluated hierarchically, starting from the server and descending to the node being
accessed:

- the server-level :code:`ECF_PERMISSIONS` establishes the initial set of permissions;

- at a suite (the root node), the permissions *supersede* the active set, so that suites can add
  users that the server level did not mention;

- at a family or task, the permissions *restrict* the active set, so that a user can never gain, on a
  lower node, a right that a higher node does not already grant;

- a right marked *sticky* (``s``) is preserved regardless of the entries defined on lower nodes,
  which is useful for server-level rights that must not be weakened by a suite.

When several users require different rights on the same node, list them as additional
comma-separated entries.

.. implementation::

   This behaviour follows the node access-control model introduced on the ``feature/node_acl``
   branch (see ``libs/node/src/ecflow/node/permissions/``). The relevant details are:

   - the node variable name is ``ECF_PERMISSIONS`` (``ecf::environment::ECF_PERMISSIONS``); an
     invalid value causes the corresponding permissions to be treated as empty;

   - the server selects its authorisation source by precedence: a valid white list file
     (``ECF_LISTS``) takes priority; otherwise, if a server-level ``ECF_PERMISSIONS`` is defined and
     valid, node-based authorisation is used; otherwise, access is *unrestricted* (everything is
     allowed, for backward compatibility);

   - therefore, node-based ``ECF_PERMISSIONS`` on suites and tasks only takes effect when a
     server-level ``ECF_PERMISSIONS`` is set and no white list file is in force.

   Whether the multi-tenant reference environment enables node-based authorisation (rather than a
   white list file) is **TBC**.

Interaction with the white list and password files
----------------------------------------------------

The server also supports two long-standing access-control mechanisms that predate the authenticating
reverse proxy: the password files (:code:`ECF_PASSWD`, :code:`ECF_CUSTOM_PASSWD`) used to
*authenticate* a user, and the white list file (:code:`ECF_LISTS`) used to *authorise* read/write
access.

The currently deployed environment does not include either a password file or a white list file, and
is expected to use:

- Authentication, via the reverse proxy and Auth-o-tron, for all requests;

- Authorisation, via node-based :code:`ECF_PERMISSIONS`.

Regardless, the interaction between the password and white list files and the HTTPS/Auth-o-tron path
is not uniform, and is stated explicitly here to avoid a natural but incorrect assumption that both
are simply superseded by the reverse proxy.

.. important::

   The mechanics of this interaction, i.e. which files are consulted, and when, are established below
   and verified against the code. What remains **TBC**, and needs further discussion, is whether
   relying solely on the reverse proxy for authentication and solely on node-based
   :code:`ECF_PERMISSIONS` for authorisation, without a white list file as a second line of defence, is
   the intended security posture for this environment.

- **The password files are bypassed.**

  When a request carries an identity resolved by the reverse proxy (that is, an identity extracted from
  the ``Authorization`` or ``X-Auth-Username`` headers set  by Auth-o-tron), the server treats the request
  as already authenticated and does not check it against :code:`ECF_PASSWD` or :code:`ECF_CUSTOM_PASSWD`.

- **The white list file is not bypassed.** Authorisation is a separate step from authentication, and
  the white list file (:code:`ECF_LISTS`), if configured, continues to be consulted for read/write
  access regardless of how the identity was established.

.. implementation::

   Verified against ``feature/node_acl``:

   - ``AuthenticationService::is_authentic()``
     (``libs/server/src/ecflow/server/AuthenticationService.cpp``) returns ``true`` unconditionally
     when ``identity.is_secure()``, before any password file is consulted. An identity is "secure"
     only when derived from the ``Authorization: Bearer``, ``Authorization: Basic``, or the paired
     ``X-Auth-Username``/``X-Auth-Password`` headers (see
     ``libs/server/src/ecflow/server/HttpServer.cpp``), i.e. precisely the headers the reverse proxy
     sets after authenticating the caller with Auth-o-tron.

   - ``AuthorisationService::allows()`` (``libs/server/src/ecflow/server/AuthorisationService.cpp``)
     is a separate, later step that does not inspect ``is_secure()`` at all; with ``WhiteListRules`` it
     calls ``WhiteListFile::verify_read_access()``/``verify_write_access()`` using only
     ``identity.username()``. It is therefore fully active over HTTPS.

   The reference environment's ecFlow server is not currently started with an :code:`ECF_LISTS` file
   (see above); should one be introduced later, the :code:`ECF_PERMISSIONS` examples above would stop
   taking effect until that file is removed, or made invalid, on the server.

Client executable location
--------------------------

The child commands in the task scripts (``ecflow_client --init``, ``--complete``, ``--abort``, and
so on) must be issued with the same ecFlow version as the server. When the environment provides a
dedicated client build at a non-default location, the path is advertised through the existing
:code:`ECF_CLIENT_EXE_PATH` variable, which the child commands use in preference to the
``ecflow_client`` found on ``PATH``.

.. code-block:: shell

   edit ECF_CLIENT_EXE_PATH '/opt/ecflow/bin/ecflow_client'

.. note::

   The original design sketch proposed a new ``ECFLOW_CLIENT_EXE`` variable for this purpose. As
   :code:`ECF_CLIENT_EXE_PATH` already exists and serves exactly this role, this note reuses it. This
   choice is **TBC**.

Client connection flags
-----------------------

A regular suite reaches the server over the native TCP protocol using the inherited
:code:`ECF_HOST` and :code:`ECF_PORT` variables. In this environment every command, whether issued
interactively or as a child command from a task script, must instead reach the server over HTTPS,
through the reverse proxy. To keep the task scripts free of hard-coded protocol options, the
connection flags are held in a dedicated variable, referenced by every child command.

.. code-block:: shell

   edit ECFLOW_CLIENT_FLAGS '--https'

.. implementation::

   ``ECFLOW_CLIENT_FLAGS`` is a **proposed** convention local to the suite, not a built-in ecFlow
   variable: it is simply substituted into the child command lines in the task scripts (see `Task
   script changes`_). Its name and its very existence are **TBC**.

   Per :ref:`multi_tenant_reference_environment`, port 8888 (the ecFlow server's internal HTTP
   endpoint, used with the plain ``--http`` flag) is intended to be reachable only by ``revproxy``
   itself, over the ``inner`` Podman network; the only endpoint intended to be reachable from outside
   the stack is port 3141 (``revproxy``, HTTPS). Consequently, in this environment there is no
   internal, unauthenticated path available to a job: every command, task or interactive, is expected
   to go through the reverse proxy over HTTPS, and must therefore provide credentials through an
   ``ecflowapirc`` file, as described in :ref:`how_to_setup_ecflow_with_https_authentication`.

``ECF_AUTHTOKENS`` is an environment variable that points to an ``ecflowapirc`` file containing the user
credentials required to reach the server through the authenticating reverse proxy. The file must be
used by every job, so that every child command issued by a task script can reach the server
over HTTPS. This is achieved by exporting ``ECF_AUTHTOKENS`` in the task scripts before any task command
is issued, so that ``ecflow_client`` can locate the file.

Since each task must authenticate as a specific tenant user, it is suggested that each user is
provisioned with a dedicated ``ecflowapirc`` file, delivered alongside the suite itself. This
file is accessible on the HPC system where the task runs, and is then referenced by the
``ECF_AUTHTOKENS`` variable.

.. note::

   How the file is provisioned to the job (bundled or referenced from a shared location) is **TBC**.

The content of the ``ecflowapirc`` file is a JSON object, as follows:

.. code-block:: json
   :caption: ecflowapirc (example)

   {
     "version": 1,
     "tokens": [
       {
         "type": "basic",
         "server": "https://<host>:<port>",
         "api": {
           "username": "<user>",
           "password": "<secret>"
         }
       }
     ]
   }

Task script changes
====================

A regular task script issues child commands by invoking ``ecflow_client`` directly and relying on
the inherited :code:`ECF_HOST`/:code:`ECF_PORT` variables. In this environment the header and
trailer sections are adjusted so that every child command uses the client executable and the
connection flags defined above.

The example below shows the relevant part of a ``head.h`` include, with the multi-tenant adjustments
highlighted relative to the standard header shown in
:ref:`the tutorial <tutorial-defining-a-task>`.

.. code-block:: shell
   :caption: head.h (excerpt)

   export ECF_PORT=%ECF_PORT%    # The server port number
   export ECF_HOST=%ECF_HOST%    # The name of ecf host that issued this task
   export ECF_NAME=%ECF_NAME%    # The name of this current task
   export ECF_PASS=%ECF_PASS%    # A unique password
   export ECF_TRYNO=%ECF_TRYNO%  # Current try number of the task
   export ECF_RID=$$

   export ECF_AUTHTOKENS="%ECF_HOME%/secrets/%USER%/ecflowapirc"  # Path to the ecflowapirc file with credentials

   # Client executable and connection flags supplied by the suite
   ECFLOW_CLIENT="%ECF_CLIENT_EXE_PATH%"
   ECFLOW_CLIENT_FLAGS="%ECFLOW_CLIENT_FLAGS%"

   # Tell ecFlow the task has started
   $ECFLOW_CLIENT $ECFLOW_CLIENT_FLAGS --init=$$

   # Define an error handler
   ERROR() {
      set +e                                       # Clear -e flag, so we do not fail
      wait                                         # Wait for background process to stop
      $ECFLOW_CLIENT $ECFLOW_CLIENT_FLAGS --abort=trap
      trap 0                                       # Remove the trap
      exit 0                                       # End the script
   }

   trap ERROR 0
   trap '{ echo "Killed by a signal"; ERROR ; }' 1 2 3 4 5 6 7 8 10 12 13 15

The corresponding trailer uses the same variables:

.. code-block:: shell
   :caption: tail.h (excerpt)

   wait                                            # Wait for background process to stop
   $ECFLOW_CLIENT $ECFLOW_CLIENT_FLAGS --complete  # Notify ecFlow of a normal end
   trap 0                                          # Remove all traps
   exit 0                                          # End the shell

.. note::

   Since every command reaches the server through the reverse proxy (see `Client connection flags`_),
   each task must be able to locate a ``ecflowapirc`` file carrying valid credentials for the
   authenticated endpoint.

.. important::

   Task commands *are* authenticated in this environment, just not by ecFlow itself. Every request
   reaching the server, whether an interactive command or a child command such as ``--init``,
   ``--complete``, or ``--abort``, must first pass ``revproxy``'s ``auth_request`` gate: a caller that
   Auth-o-tron cannot authenticate is rejected at the reverse proxy and never reaches the ecFlow
   server at all. This means a task script's :code:`.ecflowapirc` credentials are checked, exactly like
   an interactive user's, before the child command is even delivered to ecFlow.

   What does *not* happen, once a child command reaches the server, is authorisation against
   :code:`ECF_PERMISSIONS` or the white list file: as described in `Access permissions`_, ecFlow's
   authorisation rules are simply not evaluated for child commands, so the rights attached to the
   task's identity are irrelevant to whether the specific command is accepted. Instead, the server
   accepts or rejects the command by matching :code:`ECF_PASS` (the task's password) and
   :code:`ECF_RID` (the process id) against what it recorded when the task started; this mechanism,
   used to detect zombie jobs, is unrelated to the identity Auth-o-tron established at the reverse
   proxy.

   In short: the reverse proxy authenticates every request uniformly, task commands included; ecFlow's
   own :code:`ECF_PERMISSIONS`-based authorisation, by contrast, is specific to interactive commands
   and simply does not apply to child commands.

.. implementation::

   Verified against ``feature/node_acl``: ``Authoriser<COMMAND>::accept()``
   (``libs/base/src/ecflow/base/AuthorisationDetails.hpp``), for any command derived from
   ``TaskCmd``, resolves to ``allows_as_per_read_write_rules()``, which explicitly returns success
   without consulting ``AuthorisationService`` at all ("No actual verification is done for task
   commands"). Likewise ``Authenticator<COMMAND>::accept()``
   (``libs/base/src/ecflow/base/AuthenticationDetails.hpp``) resolves task commands to
   ``verify_task_authentication_rules()``, which always succeeds ("A running task is currently not
   really authenticated!") without ever calling ``AuthenticationService::is_authentic()``; the actual
   protection is ``TaskCmd::check_preconditions()``
   (``libs/base/src/ecflow/base/cts/task/TaskCmd.cpp``), matching the reported pid, password, and try
   number against the recorded task state.

   Both statements describe ecFlow's own, *internal* Authentication and Authorisation steps only. They
   say nothing about ``revproxy``'s ``auth_request`` gate, which sits outside ecFlow and, per
   :ref:`multi_tenant_reference_environment`, applies uniformly to every request reaching
   ``/v1/ecflow``, task commands included (see `Client connection flags`_).

User commands
=============

The user interface commands (``ecflow_client``, ``ecflow_ui``) must be configured to reach the
server through the authenticating reverse proxy. The authentication mechanism is based on
``ecflowapirc`` credentials, provisioned as described in `Client connection flags`_ for the task
side.

The lookup mechanism for this file is already known: the client resolves the file via the ``ECF_AUTHTOKENS``
environment variable, if set, naming the file explicitly; otherwise, it falls back to ``${HOME}/.ecflowapirc`` and,
as a last resort, ``${PWD}/.ecflowapirc``.

As for task commands, user commands must also use the ``--https`` flag to reach the server through
the reverse proxy.

Open questions
==============

The following points remain to be confirmed before this note can be considered complete:

- the exact Troika command lines, the destination host convention, and the per-tenant configuration file naming scheme (see `Job management commands`_);

- the concrete rights each tenant requires for interactive access via node-based ``ECF_PERMISSIONS``
  (``ECF_PERMISSIONS`` does not affect the child commands issued by task scripts, see the note in
  `Task script changes`_), and whether relying solely on the reverse proxy and ``ECF_PERMISSIONS``,
  without a white list file, is the intended security posture (see
  `Interaction with the white list and password files`_);

- the mechanism for provisioning ``ecflowapirc`` to every job, since all commands, task and
  interactive alike, now go through the authenticated HTTPS endpoint (see
  `Client connection flags`_);

- the concrete value of :code:`ECF_CLIENT_EXE_PATH` for this environment, in particular the location
  of the bespoke/latest ``develop`` build of ``ecflow_client`` on HPC (for example, somewhere under
  ``/perm/<user>/...``);

- whether a dedicated ``ECFLOW_CLIENT_FLAGS`` convention is adopted, or the flags are folded into an
  existing mechanism.

Deployment Specifics
====================

Information to be shared with the tenants of the multi-tenant environment, to support testing and debugging
of their suites. This section is **TBC** and will be filled in as the environment matures.

- location of the reverse proxy, including the host name and the port number (for example, ``https://<host>:3141``);

- location of the ecFlow server directories:

    - the server home (default ``%ECF_HOME%``), which contains the ``server_environment.cfg`` file, stores
      the checkpoint files, and is the working directory of the suites;

    - the location of ``ecflowapirc`` files for each tenant, including the username and the password;

- location of the latest ``develop`` build of ``ecflow_client``, for the HPC (for example, somewhere under ``/perm/<user>/...``);

- location of the latest ``develop`` build of ``ecflow_ui``, for the VDI (for example, somewhere under ``/perm/<user>/...``);

- anything else that is relevant to the tenants' testing and debugging of their suites.

Also, the auth-o-tron configuration needs to be adjusted to include credentials for the brave early adopters.
