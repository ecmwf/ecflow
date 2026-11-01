.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. _ecflow_server_environment_variables:

Server environment variables
///////////////////////////////////

ecFlow server environment variables control the execution of ecFlow
and may be set before the start of the server, typically in a
**start-up script**.

The server starts without any of these variables being set, since each setting has a default
value. A default can be overridden by:

- a setting in :ref:`server_environment.cfg`, read from the directory in which the server starts;
- an environment variable, which overrides the setting in ``server_environment.cfg``.

Not every setting has an environment variable; for example, ``ECF_INTERVAL`` and the commands,
such as ``ECF_JOB_CMD``, can only be set in ``server_environment.cfg``, and ``ECF_SSL`` and
``ECF_DEBUG_SERVER`` only in the environment.

See :ref:`server_configuration` for every setting, its default, the origins that can provide it
and its accepted values.
