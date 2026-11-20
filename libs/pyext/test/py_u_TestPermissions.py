# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# The Permissions attribute in the Python API: the value class, nodes, definitions,
# bulk application and offline validation.

import copy
import os
import tempfile

import pytest

import ecflow as ecf


def test_create_from_command_line_form():
    p = ecf.Permissions("bob:r,alice:rwxs")
    assert p.entries() == ["alice:rwxs", "bob:r"]
    assert p.to_cli() == "alice:rwxs,bob:r"


def test_create_from_json_form():
    p = ecf.Permissions('[{"user": "bob", "permissions": "rx"}]')
    assert p.entries() == ["bob:rx"]
    assert p.to_json() == '[{"permissions":"rx","user":"bob"}]'
    assert ecf.Permissions(p.to_json()) == p


def test_create_from_list_of_entries():
    p = ecf.Permissions(["bob:xr", "alice:p"])
    assert p.entries() == ["alice:p", "bob:rx"]


def test_create_from_a_tuple_or_an_attribute():
    p = ecf.Permissions(("bob:r", "alice:rw"))
    assert p.entries() == ["alice:rw", "bob:r"]
    assert ecf.Permissions(p) == p


@pytest.mark.parametrize("value", [[], ["alice:r", "alice:w"], ["alice:r,bob:r"], ["alice:"]])
def test_invalid_lists_are_refused(value):
    with pytest.raises(RuntimeError) as e:
        ecf.Permissions(value)
    assert "Invalid Permissions" in str(e.value)


@pytest.mark.parametrize("value", [3, None, {"alice": "r"}, ["alice:r", 3]])
def test_other_objects_are_refused(value):
    with pytest.raises(TypeError):
        ecf.Permissions(value)


def test_representation_recreates_the_attribute():
    for value in ["alice:rws,bob:r", "o'neil:r", 'say"hi:r', "back\\slash:r"]:
        p = ecf.Permissions(value)
        assert eval(repr(p), {"Permissions": ecf.Permissions}) == p


def test_roles_of_a_user():
    p = ecf.Permissions("alice:rwxs,bob:r")
    assert p.roles("alice") == "rwx"
    assert p.sticky("alice") is True
    assert p.roles("bob") == "r"
    assert p.sticky("bob") is False
    assert p.roles("carol") is None
    assert p.sticky("carol") is False


@pytest.mark.parametrize(
    "value",
    [
        "",
        "alice",
        "alice:",
        "alice:q",
        "alice:rr",
        "alice:s",
        "alice:r,alice:w",
        "*:r",
        "-alice:r",
        "al ice:r",
        "[]",
        '[{"user": "alice"}]',
        "[{",
    ],
)
def test_invalid_values_are_refused(value):
    with pytest.raises(RuntimeError) as e:
        ecf.Permissions(value)
    assert "Invalid Permissions" in str(e.value)


def test_invalid_entry_in_list_is_refused():
    with pytest.raises(RuntimeError) as e:
        ecf.Permissions(["alice:r", "bob:z"])
    assert "Invalid Permissions" in str(e.value)


def test_equality_and_hashing():
    a = ecf.Permissions("alice:rw,bob:r")
    b = ecf.Permissions(["bob:r", "alice:wr"])
    c = ecf.Permissions("alice:rw")
    assert a == b
    assert a != c
    assert hash(a) == hash(b)
    assert len({a, b, c}) == 2


def test_string_form_is_the_defs_line():
    p = ecf.Permissions("alice:rws")
    assert str(p) == 'permissions [{"permissions":"rws","user":"alice"}]'
    suite = ecf.Suite("s")
    suite.add_permissions(p)
    assert str(p) in str(suite)


def test_copy_of_the_attribute():
    p = ecf.Permissions("alice:r")
    q = copy.copy(p)
    assert q == p
    assert q is not p


def test_add_get_and_delete_on_nodes():
    for node in (ecf.Suite("s"), ecf.Family("f"), ecf.Task("t")):
        assert node.get_permissions() is None
        node.add_permissions(ecf.Permissions("alice:r"))
        assert node.get_permissions() == ecf.Permissions("alice:r")
        node.delete_permissions()
        assert node.get_permissions() is None
        node.delete_permissions()  # deleting a missing attribute does nothing
        assert node.get_permissions() is None


def test_add_accepts_the_textual_forms():
    task = ecf.Task("t")
    task.add_permissions("alice:rx")
    assert task.get_permissions() == ecf.Permissions("alice:rx")
    family = ecf.Family("f")
    family.add_permissions('[{"user": "bob", "permissions": "r"}]')
    assert family.get_permissions() == ecf.Permissions("bob:r")


def test_a_node_carries_at_most_one_attribute():
    task = ecf.Task("t", ecf.Permissions("alice:r"))
    with pytest.raises(RuntimeError):
        task.add_permissions(ecf.Permissions("bob:r"))
    assert task.get_permissions() == ecf.Permissions("alice:r")


def test_composition_into_definitions():
    defs = ecf.Defs(
        ecf.Suite(
            "s",
            ecf.Permissions("alice:rwxp"),
            ecf.Family("f", ecf.Permissions("alice:r"), ecf.Task("t")),
        )
    )
    assert defs.find_abs_node("/s").get_permissions() == ecf.Permissions("alice:rwxp")
    assert defs.find_abs_node("/s/f").get_permissions() == ecf.Permissions("alice:r")
    assert defs.find_abs_node("/s/f/t").get_permissions() is None

    task = ecf.Task("t2") + ecf.Permissions("bob:r")
    assert task.get_permissions() == ecf.Permissions("bob:r")


def test_attribute_returned_is_a_copy():
    task = ecf.Task("t", ecf.Permissions("alice:r"))
    p = task.get_permissions()
    task.delete_permissions()
    assert p == ecf.Permissions("alice:r")


def test_copies_of_nodes_and_definitions_keep_the_attribute():
    defs = ecf.Defs()
    suite = defs.add_suite("s")
    suite.add_permissions("alice:rwxp")
    suite.add_task("t").add_permissions("alice:r")

    copied_defs = copy.copy(defs)
    assert copied_defs == defs
    assert copied_defs.find_abs_node("/s/t").get_permissions() == ecf.Permissions("alice:r")

    copied_suite = copy.copy(suite)
    assert copied_suite.get_permissions() == ecf.Permissions("alice:rwxp")

    copied_defs.find_abs_node("/s/t").delete_permissions()
    assert copied_defs != defs
    assert defs.find_abs_node("/s/t").get_permissions() == ecf.Permissions("alice:r")


def test_definitions_round_trip():
    defs = ecf.Defs()
    suite = defs.add_suite("s")
    suite.add_permissions("alice:rwxps,bob:r")
    suite.add_family("f").add_permissions("bob:r").add_task("t")
    with tempfile.TemporaryDirectory() as directory:
        path = os.path.join(directory, "permissions.def")
        defs.save_as_defs(path)
        restored = ecf.Defs(path)
    assert restored == defs
    assert restored.find_abs_node("/s").get_permissions() == ecf.Permissions(
        "alice:rwxps,bob:r"
    )
    assert restored.find_abs_node("/s/f").get_permissions() == ecf.Permissions("bob:r")


def test_bulk_application_to_a_set_of_nodes():
    defs = ecf.Defs()
    suite = defs.add_suite("s")
    tasks = [suite.add_task("t" + str(i)) for i in range(3)]
    p = ecf.Permissions("alice:rx")
    p.add_to(tasks)
    for task in tasks:
        assert task.get_permissions() == p


def test_bulk_application_is_atomic():
    a = ecf.Task("a")
    b = ecf.Task("b", ecf.Permissions("bob:r"))
    c = ecf.Task("c")
    with pytest.raises(RuntimeError) as e:
        ecf.Permissions("alice:r").add_to([a, b, c])
    assert "b" in str(e.value)
    assert a.get_permissions() is None
    assert b.get_permissions() == ecf.Permissions("bob:r")
    assert c.get_permissions() is None


def test_bulk_application_can_replace():
    a = ecf.Task("a")
    b = ecf.Task("b", ecf.Permissions("bob:r"))
    ecf.Permissions("alice:r").add_to([a, b], replace=True)
    assert a.get_permissions() == ecf.Permissions("alice:r")
    assert b.get_permissions() == ecf.Permissions("alice:r")


def test_bulk_application_refuses_other_objects():
    a = ecf.Task("a")
    with pytest.raises(TypeError):
        ecf.Permissions("alice:r").add_to([a, "b"])
    assert a.get_permissions() is None


def _definitions():
    defs = ecf.Defs()
    suite = defs.add_suite("s")
    suite.add_permissions("alice:rwxp,bob:r")
    suite.add_family("f").add_permissions("bob:r").add_task("t")
    return defs


def test_offline_validation_accepts_valid_definitions():
    server = ecf.Permissions("alice:rwxlops,bob:r")
    assert _definitions().check_permissions(server) == ""


def test_offline_validation_reports_an_unlisted_user():
    server = ecf.Permissions("alice:rwxlops")
    errors = _definitions().check_permissions(server)
    assert "bob" in errors
    assert "/s" in errors


def test_offline_validation_reports_a_user_not_listed_above():
    defs = ecf.Defs()
    suite = defs.add_suite("s")
    suite.add_permissions("alice:rwxp")
    suite.add_task("t").add_permissions("bob:r")
    errors = defs.check_permissions(ecf.Permissions("alice:rwxlops,bob:r"))
    assert "bob" in errors
    assert "/s/t" in errors


def test_offline_validation_reports_a_lockout():
    # Without a sticky entry, nobody holds p at /s, while alice holds it at server level
    defs = ecf.Defs()
    defs.add_suite("s").add_permissions("bob:r")
    errors = defs.check_permissions(ecf.Permissions("alice:rwxlop,bob:r"))
    assert "nobody able to change" in errors
    assert "/s" in errors
    # A sticky entry keeps p at every node
    assert defs.check_permissions(ecf.Permissions("alice:rwxlops,bob:r")) == ""


def test_offline_validation_reports_every_suite_at_fault():
    defs = ecf.Defs()
    defs.add_suite("s1").add_permissions("carol:r")
    defs.add_suite("s2").add_permissions("alice:rwxp")
    defs.add_suite("s3").add_permissions("dave:r")
    errors = defs.check_permissions(ecf.Permissions("alice:rwxlops"))
    assert "carol" in errors
    assert "dave" in errors
    assert "s2" not in errors


def test_offline_validation_without_attributes():
    defs = ecf.Defs()
    defs.add_suite("s").add_task("t")
    assert defs.check_permissions(ecf.Permissions("alice:rwxlops")) == ""


def test_add_to_example_of_the_documentation():
    # The example of Permissions.add_to: the tasks of the definitions
    defs = ecf.Defs()
    suite = defs.add_suite("s")
    suite.add_task("t1")
    suite.add_family("f").add_task("t2")
    ecf.Permissions("alice:rx").add_to(defs.get_all_tasks())
    assert [task.get_permissions().to_cli() for task in defs.get_all_tasks()] == ["alice:rx", "alice:rx"]
