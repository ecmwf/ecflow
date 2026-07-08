#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

"""
Shared pytest configuration for the ecFlow Python binding tests.

This file is intentionally minimal.  Most unit tests only need a working
``import ecflow`` and use bare ``assert`` statements.  Global fixtures or
hooks should only be added here when they are genuinely shared across many
files.
"""

import pathlib
import sys

import pytest


def pytest_configure(config):
    """Ensure ecflow is importable before any test is collected."""
    try:
        import ecflow  # noqa: F401
    except ImportError as exc:
        raise RuntimeError(
            "Could not import ecflow. Make sure the binding is on PYTHONPATH."
        ) from exc
