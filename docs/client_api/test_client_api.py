# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# The scripts generating the command reference: the roles of each command (D6.3), as rendered by build.py and checked
# by validate_help_manifest.py, and the parity of the generated help with `ecflow_client --help=<command>`.

import copy
import os
import pathlib
import subprocess
import sys

import pytest

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import build  # noqa: E402
from validate_help_manifest import check_cross_references, check_schema, load_json  # noqa: E402


@pytest.fixture(scope="module")
def manifest():
    return load_json(HERE / "help.json")


@pytest.fixture(scope="module")
def schema():
    # The schema is read without jsonschema; the tests validating against it skip without it
    return load_json(HERE / "help.schema.json")


def command(manifest, name):
    return next(c for c in manifest["commands"] if c["name"] == name)


def test_render_roles_of_a_single_entry(manifest):
    assert build.render_roles(command(manifest, "suspend")) == (
        "Roles required, when the server uses Permissions attributes:\n"
        "\n"
        "    x at each node addressed\n"
        "\n"
        "In whitelist mode, r requires read access, and the other roles write access."
    )


def test_render_roles_of_variants_combined_roles_and_notes():
    text = build.render_roles(
        {
            "kind": "user",
            "permissions": [
                {"variant": "a", "roles": "wp", "at": "node", "note": "n"},
                {"variant": "b", "roles": "o", "at": "server"},
            ],
        }
    )
    assert "    a: w and p at each node addressed (n)\n" in text
    assert "    b: o at the server\n" in text
    assert text.endswith("In whitelist mode, r requires read access, and the other roles write access.")


def test_render_roles_of_a_task_command(manifest):
    assert build.render_roles(command(manifest, "abort")) == (
        "Roles required, when the server uses Permissions attributes:\n"
        "\n"
        "    none (task requests are never subject to the authorisation of users)"
    )


def test_options_have_no_roles(manifest):
    assert "Roles required" not in build.render_help_output(manifest, "host")


def test_manifest_is_valid(manifest, schema):
    pytest.importorskip("jsonschema")
    assert check_schema(manifest, schema) == []
    assert check_cross_references(manifest) == []


def test_schema_requires_roles(manifest, schema):
    pytest.importorskip("jsonschema")
    broken = copy.deepcopy(manifest)
    del command(broken, "suspend")["permissions"]
    assert check_schema(broken, schema)
    broken = copy.deepcopy(manifest)
    command(broken, "suspend")["permissions"] = [{"roles": "q", "at": "node"}]
    assert check_schema(broken, schema)


@pytest.mark.parametrize(
    "entry", [{"roles": "none", "at": "node"}, {"roles": "r", "at": "none"}], ids=["none-at-node", "role-at-none"]
)
def test_roles_none_and_at_none_go_together(manifest, entry):
    broken = copy.deepcopy(manifest)
    command(broken, "suspend")["permissions"] = [entry]
    assert any("pairs roles" in problem for problem in check_cross_references(broken))


def test_task_commands_require_no_role(manifest):
    broken = copy.deepcopy(manifest)
    command(broken, "abort")["permissions"] = [{"roles": "r", "at": "server"}]
    assert any("task command 'abort'" in problem for problem in check_cross_references(broken))


def test_generated_help_equals_the_client_help(manifest):
    # The command reference reproduces `ecflow_client --help=<command>` byte for byte, up to the environment banner:
    # the banner depends on the features the client is built with (e.g. without SSL, ECF_SSL is not listed)
    banner = "The client considers, for both user and task commands"
    client = os.environ.get("ECFLOW_CLIENT")
    if not client:
        pytest.skip("ECFLOW_CLIENT is not set")
    for entry in manifest["commands"]:
        if entry["visibility"] == "internal":
            continue
        name = entry["name"]
        produced = subprocess.run(
            [client, f"--help={name}"], capture_output=True, text=True, check=False, env={"PATH": os.environ["PATH"]}
        ).stdout
        expected = build.render_help_output(manifest, name)
        assert banner in produced and banner in expected, name
        assert produced.split(banner)[0] == expected.split(banner)[0], name
