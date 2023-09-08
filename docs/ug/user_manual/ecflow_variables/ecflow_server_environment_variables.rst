.. _ecflow_server_environment_variables:

Server environment variables
///////////////////////////////////

ecFlow server environment variables control the execution of ecFlow
and may be set before the start of the server, typically in a
**start-up script**.

ecFlow will start happily without any of these variables being set
since all of them have a default value. These default values can be
overridden by:

-  Setting them in **"server_environment.config".** This file should
   then be placed in the current working directory when invoking the
   server.

-  Explicitly setting environment variables. These will override any
   setting in the **"server_environment.config"** file.

For the full list of environment variables used the server please see
'ecflow_server' in the :ref:`Glossary`.
