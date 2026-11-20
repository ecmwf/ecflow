.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

ecflow.GroupError
/////////////////


.. py:class:: GroupError
   :module: ecflow

   Bases: :py:class:`~builtins.RuntimeError`

Group failure with ordered child (raw code, unformatted message) entries; no group-level code. Construct with (formatted_description, ordered_failed_children).


.. py:method:: GroupError.entries(self: object) -> object
   :module: ecflow

Return ordered (raw code, message) pairs for failed children.
