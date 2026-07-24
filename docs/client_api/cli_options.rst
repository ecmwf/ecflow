
Options
=======

.. list-table:: List of common options for `ecflow_client` commands
    :header-rows: 1
    :width: 100%
    :widths: 20 80
    :name: ecflow_client_options

    * - Option
      - Description

    * - :ref:`add_cli`
      - Add variables e.g. name1=value1 name2=value2. Can only be used in combination with --init command.

    * - :ref:`debug_cli`
      - Enables the display of client environment settings and execution details.

    * - :ref:`help_cli`
      - Produce help message

    * - :ref:`host_cli`
      - When specified overrides the environment variable ECF_HOST and default host: 'localhost'

    * - :ref:`http_cli`
      - Enables communication over HTTP between client/server.

    * - :ref:`https_cli`
      - Enables communication over HTTPS between client/server.

    * - :ref:`password_cli`
      - Specifies the password used to contact the server. Must be used in combination with option --user.

    * - :ref:`port_cli`
      - When specified overrides the environment variable ECF_PORT and default port: '3141'

    * - :ref:`remove_cli`
      - remove variables i.e name name2

    * - :ref:`rid_cli`
      - When specified overrides the environment variable ECF_RID. Can only be used for task commands.

    * - :ref:`ssl_cli`
      - Enables the use of SSL when contacting the server.

    * - :ref:`user_cli`
      - Specifies the user name used to contact the server. Must be used in combination with option --password.

    * - :ref:`version_cli`
      - Show ecflow client version number, and version of the boost library used


.. toctree::
    :maxdepth: 1
    :hidden:

    add (option) <api/add.rst>
    debug (option) <api/debug.rst>
    help (option) <api/help.rst>
    host (option) <api/host.rst>
    http (option) <api/http.rst>
    https (option) <api/https.rst>
    password (option) <api/password.rst>
    port (option) <api/port.rst>
    remove (option) <api/remove.rst>
    rid (option) <api/rid.rst>
    ssl (option) <api/ssl.rst>
    user (option) <api/user.rst>
    version (option) <api/version.rst>
