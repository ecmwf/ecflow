#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import pytest

from ecflow import Suite, Family, Task


def test_replace_on_server_errors_for_unattached_nodes():
    # expect error since nodes are not attached to a definition.
    # The error should happen before we connect to the server, hence no need to start server.
    # Avoid suspending node first (since that will require node exist in the server)
    for node in (Suite("s1"), Family("f1"), Task("t1")):
        with pytest.raises(RuntimeError, match="client definition is empty"):
            node.replace_on_server("locahost:3141", suspend_node_first=False)
