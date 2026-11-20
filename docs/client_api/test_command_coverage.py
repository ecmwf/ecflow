# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

# Every user command class registered by CtsCmdRegistry is documented and tested alongside its peers: it has a section
# in command_internals.rst, an entry in the option-to-class map of build.py, and an entry in the serialisation round
# trip of TestRequest.cpp.

import os
import pathlib
import re
import sys

import pytest

HERE = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import build  # noqa: E402

# The sources are taken from the environment, so that the test can be pointed at another tree; by default, the tree
# holding this script
SOURCE_DIR = pathlib.Path(os.environ.get("ECFLOW_SOURCE_DIR", HERE.parent.parent))
REGISTRY = SOURCE_DIR / "libs/base/src/ecflow/base/cts/CtsCmdRegistry.cpp"
INTERNALS = SOURCE_DIR / "docs/client_api/command_internals.rst"
BUILD = SOURCE_DIR / "docs/client_api/build.py"
TEST_REQUEST = SOURCE_DIR / "libs/base/test/TestRequest.cpp"


def registered_user_commands():
    """Returns the user command classes registered by CtsCmdRegistry: those of its includes under cts/user/."""
    text = REGISTRY.read_text()
    user = set(re.findall(r'#include "ecflow/base/cts/user/(\w+)\.hpp"', text))
    registered = set(re.findall(r"make_shared<(\w+)>", text))
    return sorted(user & registered)


def mapped_classes():
    """Returns the command classes named by the option-to-class map of build.py, as found in the file given."""
    if BUILD.resolve() == (HERE / "build.py").resolve():
        return set(build.PROTOCOL_ANCHORS.values())
    return set(re.findall(r'"\w[\w-]*": "(\w+)"', BUILD.read_text()))


def test_the_registry_holds_user_commands():
    commands = registered_user_commands()
    assert "AlterCmd" in commands
    assert "PathsCmd" in commands


@pytest.mark.parametrize("cls", registered_user_commands())
def test_user_command_has_a_section_in_command_internals(cls):
    assert re.search(rf"^\.\. _proto_{cls}:$", INTERNALS.read_text(), re.MULTILINE), (
        f"{cls} has no section (.. _proto_{cls}:) in {INTERNALS.name}"
    )


@pytest.mark.parametrize("cls", registered_user_commands())
def test_user_command_is_mapped_by_build(cls):
    assert cls in mapped_classes(), f"{cls} is not mapped to any option in {BUILD.name} (PROTOCOL_ANCHORS)"


@pytest.mark.parametrize("cls", registered_user_commands())
def test_user_command_is_round_tripped_by_test_request(cls):
    assert re.search(rf"\bnew {cls}\(|make_shared<{cls}>\(", TEST_REQUEST.read_text()), (
        f"{cls} is not in the serialisation round trip of {TEST_REQUEST.name}"
    )
