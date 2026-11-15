# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Tests of the user name model, through the Python client.

import os
import socket

import pytest

import ecflow as ecf
import ecflow_test_util as Test

admin = ("admin", "admin-secret")
alice = ("alice", "alice-secret")

INVALID = [
    "al ice",
    "-bob",
    "*",
    "a:b",
    "a;b",
    "a,b",
    "a#b",
    "a\x01b",
    "a\u00a0b",
    "a\u3000b",
]


def test_invalid_user_name_refused_by_client():
    """The client refuses a user name outside the model, before sending a request"""
    with Test.PermissionsServer(admin, [alice]) as server:
        for name in INVALID:
            client = server.client("alice")
            client.set_user_name(name)
            with pytest.raises(RuntimeError) as refused:
                client.ping()
            assert "invalid user name" in str(refused.value), repr(name)
            assert not isinstance(refused.value, ecf.PermissionDenied)
        server.client("alice").ping()


def test_invalid_user_name_after_a_denial():
    """A request refused locally for its user name is not reported as the denial of a
    previous request of the same client"""
    with Test.PermissionsServer(admin, [alice]) as server:
        lists = os.path.join(
            server.home(), socket.gethostname() + "." + server.the_port + ".ecf.lists"
        )
        with open(lists, "w") as f:
            f.write("4.5.0\nadmin\n-alice\n")
        server.admin_client().reload_wl_file()
        client = server.client("alice")
        with pytest.raises(ecf.PermissionDenied):
            client.checkpt()
        client.set_user_name("a:b")
        with pytest.raises(RuntimeError) as refused:
            client.checkpt()
        assert not isinstance(refused.value, ecf.PermissionDenied)
        assert "invalid user name" in str(refused.value)


def test_reload_custom_password_file_with_invalid_user_name():
    """Reloading a password file holding an invalid user name fails, naming the line,
    and the previous content stays in force: the previous password is still accepted"""
    with Test.PermissionsServer(admin, [alice]) as server:
        host = socket.gethostname()
        path = os.path.join(
            server.home(), host + "." + server.the_port + ".ecf.custom_passwd"
        )
        entries = [("admin", "admin-secret"), ("alice", "alice-replacement")]
        lines = ["4.5.0"]
        for user, password in entries:
            for where in (host, "localhost"):
                lines.append(" ".join([user, where, server.the_port, password]))
        lines.append("a\u00a0b localhost " + server.the_port + " x")
        with open(path, "w") as f:
            f.write("\n".join(lines) + "\n")
        with pytest.raises(RuntimeError) as refused:
            server.admin_client().reload_custom_passwd_file()
        assert "invalid user name" in str(refused.value)
        assert "line " + str(len(lines)) in str(refused.value)

        # The client sends the previous password: accepted only if the file on disk,
        # which lists the replacement before the invalid entry, was not applied
        server.client("alice").get_server_defs()


def test_permissions_attribute_with_invalid_user_name(tmp_path):
    """A Permissions attribute naming a user outside the model is refused"""
    for name in ["a\\u00a0b", "a\\u0001b", "a\\u3000b", "a:b", "-bob"]:
        text = (
            "suite s\npermissions "
            '[{"user": "' + name + '", "permissions": "r"}]\nendsuite\n'
        )
        path = tmp_path / "invalid.def"
        path.write_text(text)
        with pytest.raises(RuntimeError) as refused:
            ecf.Defs(str(path))
        assert "Invalid Permissions" in str(refused.value), name
