.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

ecflow.CodedError
/////////////////


.. py:class:: CodedError
   :module: ecflow

   Bases: :py:class:`~builtins.RuntimeError`

Server failure with a raw numeric code. Subclasses RuntimeError; code zero is still a failure. Construct with (formatted_description, raw_code, unformatted_server_message).


.. py:method:: CodedError.code(self: object) -> int
   :module: ecflow

Return the raw code, including zero or an unknown value.


.. py:method:: CodedError.message(self: object) -> str
   :module: ecflow

Return the unformatted server diagnostic.
