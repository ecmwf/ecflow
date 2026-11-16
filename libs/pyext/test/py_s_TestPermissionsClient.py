# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# The operations of the command --permissions in the Python client.

import pytest

import ecflow as ecf
import ecflow_test_util as Test

admin = ("admin", "admin-secret")
alice = ("alice", "alice-secret")
bob = ("bob", "bob-secret")

server_attribute = [
    {"user": "admin", "permissions": "rwxlop"},
    {"user": "alice", "permissions": "rwxp"},
    {"user": "bob", "permissions": "r"},
]


def _load(server):
    defs = ecf.Defs()
    defs.add_suite("s").add_family("f").add_task("t")
    defs.add_suite("u").add_task("t")
    server.admin_client().load(defs)


def test_each_operation():
    with Test.PermissionsServer(admin, [alice, bob], server_attribute) as server:
        _load(server)
        ci = server.client("alice")

        # read
        assert ci.get_permissions("/") == ecf.Permissions("admin:rwxlop,alice:rwxp,bob:r")
        with pytest.raises(RuntimeError) as e:
            ci.get_permissions("/s")
        assert "/s has no Permissions attribute" in str(e.value)

        # create, from the value class and from the textual forms
        ci.create_permissions("/s", ecf.Permissions("alice:rwxp,bob:r"))
        assert ci.get_permissions("/s") == ecf.Permissions("alice:rwxp,bob:r")
        ci.create_permissions("/s/f", "alice:rwxp")
        assert ci.get_permissions("/s/f") == ecf.Permissions("alice:rwxp")
        ci.create_permissions("/u", '[{"user": "alice", "permissions": "rwxp"}]')
        assert ci.get_permissions("/u") == ecf.Permissions("alice:rwxp")
        with pytest.raises(RuntimeError) as e:
            ci.create_permissions("/s", "alice:rwxp")
        assert "/s already has a Permissions attribute" in str(e.value)
        with pytest.raises(RuntimeError) as e:
            ci.create_permissions("/s/f/t", "alice:rp,carol:r")
        assert "carol" in str(e.value)

        # update
        ci.update_permissions("/s", "alice:rwxp,bob:rw")
        assert ci.get_permissions("/s") == ecf.Permissions("alice:rwxp,bob:rw")
        with pytest.raises(RuntimeError) as e:
            ci.update_permissions("/s", "alice:rwx,bob:rw")
        assert "nobody able to change" in str(e.value)
        assert ci.get_permissions("/s") == ecf.Permissions("alice:rwxp,bob:rw")

        # effective
        assert ci.effective_permissions("/s") == "rwxp"
        assert ci.effective_permissions("/s", "bob") == "rw"
        assert ci.effective_permissions("/s", "carol") == ""
        for name in ("*", "al ice", "a:b"):
            with pytest.raises(RuntimeError) as e:
                ci.effective_permissions("/s", name)
            assert "invalid user name" in str(e.value)

        # delete
        ci.delete_permissions("/s/f")
        with pytest.raises(RuntimeError) as e:
            ci.get_permissions("/s/f")
        assert "/s/f has no Permissions attribute" in str(e.value)

        # The changes reach the client copy of the definitions
        ci.sync_local()
        defs = ci.get_defs()
        assert defs.find_abs_node("/s").get_permissions() == ecf.Permissions(
            "alice:rwxp,bob:rw"
        )
        assert defs.find_abs_node("/s/f").get_permissions() is None

        # Definitions received from the server carry its server attribute, which a load refuses
        before = str(defs)
        errors = defs.check_permissions(ecf.Permissions("admin:rwxlops,alice:rwxp,bob:r"))
        assert "carry a server Permissions attribute" in errors
        assert str(defs) == before


def test_invalid_input_is_refused_by_the_client():
    with Test.PermissionsServer(admin, [alice, bob], server_attribute) as server:
        _load(server)
        ci = server.client("alice")
        with pytest.raises(RuntimeError) as e:
            ci.create_permissions("/s", "alice:")
        assert "Invalid Permissions" in str(e.value)
        with pytest.raises(RuntimeError) as e:
            ci.get_permissions("s")
        assert "the path must be absolute" in str(e.value)
        with pytest.raises(RuntimeError):
            ci.get_permissions("/s")


def test_denials_raise_permission_denied():
    with Test.PermissionsServer(admin, [alice, bob], server_attribute) as server:
        _load(server)
        server.client("alice").create_permissions("/s", "alice:rwxp,bob:rwx")
        server.client("alice").create_permissions("/u", "alice:rwxp")
        ci = server.client("bob")
        assert ci.get_permissions("/s") == ecf.Permissions("alice:rwxp,bob:rwx")
        for operation in (
            lambda: ci.update_permissions("/s", "alice:rwxp,bob:rwxp"),
            lambda: ci.delete_permissions("/s"),
            lambda: ci.create_permissions("/s/f", "bob:r"),
            lambda: ci.update_permissions("/", "admin:rwxlop"),
            lambda: ci.get_permissions("/u"),
            lambda: ci.effective_permissions("/u", "alice"),
        ):
            with pytest.raises(ecf.PermissionDenied):
                operation()
        assert server.client("alice").get_permissions("/s") == ecf.Permissions(
            "alice:rwxp,bob:rwx"
        )
