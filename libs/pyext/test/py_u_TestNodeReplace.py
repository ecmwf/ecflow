# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

import pytest

from ecflow import Suite, Family, Task


def test_replace_on_server_errors_for_unattached_nodes():
    # expect error since nodes are not attached to a definition.
    # The error should happen before we connect to the server, hence no need to start server.
    # Avoid suspending node first (since that will require node exist in the server)
    for node in (Suite("s1"), Family("f1"), Task("t1")):
        with pytest.raises(RuntimeError, match="client definition is empty"):
            node.replace_on_server("locahost:3141", suspend_node_first=False)
