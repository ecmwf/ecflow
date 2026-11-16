.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

ecflow.PermissionDenied
///////////////////////


.. py:class:: PermissionDenied
   :module: ecflow

   Bases: :py:class:`~builtins.RuntimeError`

Raised by a :py:class:`ecflow.Client` request that the server denies

The server denies a request when the user does not hold the role the request requires (see
:ref:`permissions_attribute_commands`), or, without Permissions attributes, when the white list file
does not grant the access. The message names the reason, the request and the user.

PermissionDenied derives from RuntimeError: the code catching RuntimeError catches it too.

Usage:

.. code-block:: python

   ci = Client()
   try:
       ci.suspend('/s/f')
   except PermissionDenied as e:
       print('denied:', e)  # ... Insufficient permissions (role x required at /s/f) ...
   except RuntimeError as e:
       print('failed:', e)
