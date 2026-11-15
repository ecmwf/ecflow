# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Acceptance scenarios of the authorisation by Permissions attributes, as seen by
# the Python client.

import os

import pytest

import ecflow as ecf
import ecflow_test_util as Test

admin = ("admin", "admin-secret")
operator = ("operator", "operator-secret")


def test_a7_3_denied_operation():
    """A7.3: an operator with r at server level and r, but not x, on /sB requeues the
    aborted task /sB/t; the request raises PermissionDenied and the task stays aborted"""
    with Test.PermissionsServer(
        admin,
        [operator],
        [
            {"user": "admin", "permissions": "rwxlops"},
            {"user": "operator", "permissions": "r"},
        ],
    ) as server:
        definition = os.path.join(server.home(), "acceptance.def")
        with open(definition, "w") as f:
            f.write(
                "suite sB\n"
                'permissions [{"user": "admin", "permissions": "rwxlop"},'
                ' {"user": "operator", "permissions": "r"}]\n'
                "  task t\n"
                "endsuite\n"
            )
        administrator = server.admin_client()
        administrator.load(definition)
        administrator.force_state("/sB/t", ecf.State.aborted)

        with pytest.raises(ecf.PermissionDenied) as denied:
            server.client("operator").requeue("/sB/t")
        assert "Permission denied: --requeue" in str(denied.value)

        administrator.sync_local()
        state = administrator.get_defs().find_abs_node("/sB/t").get_state()
        assert state == ecf.State.aborted
