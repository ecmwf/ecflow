# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Self-tests of the harness running a server under several identities, for the tests
# of Permissions attributes.

import os
import socket

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


def test_permissions_denial_is_a_permission_denied_exception():
    """A request refused by the authorisation of the server raises PermissionDenied,
    a RuntimeError, with a message naming no node"""
    with Test.PermissionsServer(admin, [alice]) as server:
        home = server.home()
        lists = os.path.join(
            home, socket.gethostname() + "." + server.the_port + ".ecf.lists"
        )
        with open(lists, "w") as f:
            f.write("4.5.0\nadmin\n-alice\n")
        server.admin_client().reload_wl_file()
        defs = ecf.Defs()
        defs.add_suite("secret_suite")
        server.admin_client().load(defs)

        with pytest.raises(ecf.PermissionDenied) as denied:
            server.client("alice").suspend("/secret_suite")
        assert isinstance(denied.value, RuntimeError)
        # The part of the message from the server names no node; the client
        # prefixes the request as given
        server_part = str(denied.value).split("Server reply: ")[1]
        assert "Permission denied: --suspend" in server_part
        assert "secret_suite" not in server_part

        # Other errors are not denials
        with pytest.raises(RuntimeError) as failed:
            server.admin_client().suspend("/no/such/node")
        assert not isinstance(failed.value, ecf.PermissionDenied)


def test_permissions_write_without_read():
    """A user holding w but not r on a suite changes it through the single-path forms
    of alter and sort_attributes, and cannot read it; the list forms check the paths
    first, which needs r, and change nothing"""
    with Test.PermissionsServer(
        admin,
        [alice],
        [
            {"user": "admin", "permissions": "rwxlops"},
            {"user": "alice", "permissions": "r"},
        ],
    ) as server:
        definition = os.path.join(server.home(), "write_only.def")
        with open(definition, "w") as f:
            f.write(
                "suite w_only\n"
                'permissions [{"user": "admin", "permissions": "rwxlop"},'
                ' {"user": "alice", "permissions": "w"}]\n'
                "  task t\n"
                "    label l v\n"
                "endsuite\n"
            )
        server.admin_client().load(definition)

        client = server.client("alice")
        client.alter("/w_only/t", "change", "label", "l", "single")
        client.sort_attributes("/w_only/t", "label")
        with pytest.raises(ecf.PermissionDenied) as failed:
            client.alter(["/w_only/t"], "change", "label", "l", "list")
        assert "--check /w_only/t" in str(failed.value)  # refused by the check of the paths
        with pytest.raises(ecf.PermissionDenied) as failed:
            client.sort_attributes(["/w_only/t"], "label")
        assert "--check /w_only/t" in str(failed.value)  # refused by the check of the paths
        with pytest.raises(ecf.PermissionDenied):
            client.query("label", "/w_only/t", "l")

        administrator = server.admin_client()
        administrator.sync_local()
        task = administrator.get_defs().find_abs_node("/w_only/t")
        assert [label.new_value() for label in task.labels] == ["single"]

