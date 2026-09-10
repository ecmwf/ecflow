.. SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
.. SPDX-License-Identifier: Apache-2.0

.. index::
   single: cookbook
   
.. _monitor-task:
   
How can I monitor my suite independent of the GUI?
**************************************************

In this example we want to monitor a particular task. If this :term:`task` is :term:`aborted` for any reason, we ask the server for the job output. This could be mailed to the user.

.. literalinclude:: src/monitor-task.py
   :language: python

