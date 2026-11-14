# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Self-tests of the harness running a server under several identities, for the tests
# of Permissions attributes.

import os

import pytest

import ecflow as ecf
import ecflow_test_util as Test

admin = ("admin", "admin-secret")
alice = ("alice", "alice-secret")


def test_permissions_harness_identities():
    """The listed identities are authenticated, others are refused, the user running
    the test included, and the server attribute is configured"""
    with Test.PermissionsServer(
        admin,
        [alice],
        [
            {"user": "admin", "permissions": "rwxlops"},
            {"user": "alice", "permissions": "r"},
        ],
    ) as server:
        server.client("alice").sync_local()
        server.admin_client().ping()

        with pytest.raises(RuntimeError):
            server.client("mallory").sync_local()

        plain = ecf.Client("localhost", server.the_port)
        plain.set_connection_attempts(1)
        with pytest.raises(RuntimeError):
            plain.sync_local()

        with open(os.path.join(server.home(), "server.log")) as log:
            assert "  Users = 2" in log.read()

        port = server.the_port
        assert server.stop()
        assert Test.EcfPortLock.is_port_free(port)


def test_permissions_harness_restores_environment(monkeypatch):
    """The variables cleared for the server are restored afterwards, as they were"""
    monkeypatch.setenv("ECF_PORT", "1234")
    original_passwords = os.environ.get("ECF_CUSTOM_PASSWD")
    with Test.PermissionsServer(admin) as server:
        assert server.the_port != "1234"
        assert "ECF_PORT" not in os.environ
        assert os.environ["ECF_CUSTOM_PASSWD"].endswith("client.custom_passwd")
    assert os.environ["ECF_PORT"] == "1234"
    assert os.environ.get("ECF_CUSTOM_PASSWD") == original_passwords


def test_permissions_harness_start_failure_cleans_up(monkeypatch):
    """A server refused at start-up leaves no lock, no process and the environment
    as it was"""
    monkeypatch.setenv("ECF_PORT", "1234")
    original_passwords = os.environ.get("ECF_CUSTOM_PASSWD")
    harness = Test.PermissionsServer(admin, permissions="not a list")
    with pytest.raises(RuntimeError):
        harness.__enter__()
    assert Test.EcfPortLock.is_port_free(harness.the_port)
    assert not os.path.exists(Test.EcfPortLock()._lock_file(harness.the_port))
    assert os.environ["ECF_PORT"] == "1234"
    assert os.environ.get("ECF_CUSTOM_PASSWD") == original_passwords


def test_permissions_harness_kills_when_termination_is_refused():
    """A server refusing to terminate is killed, and its port released"""
    with Test.PermissionsServer(admin) as server:
        port = server.the_port
        # Termination is requested as a user the server does not know
        server.administrator = ("mallory", "mallory-secret")
    assert Test.EcfPortLock.is_port_free(port)
    assert not os.path.exists(Test.EcfPortLock()._lock_file(port))


def test_permissions_harness_ownership_mismatch_spares_foreign_server():
    """When another server answers on the port, the harness fails without stopping
    it"""
    with Test.PermissionsServer(admin) as foreign:
        with pytest.raises(RuntimeError):
            with Test.PermissionsServer(admin, fixed_port=foreign.the_port):
                pass
        foreign.admin_client().ping()
