# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

"""
Shared pytest configuration for the ecFlow Python binding tests.

This file is intentionally minimal.  Most unit tests only need a working
``import ecflow`` and use bare ``assert`` statements.  Global fixtures or
hooks should only be added here when they are genuinely shared across many
files.
"""

import pytest # to ensure pytest is available

def pytest_configure(config):
    """Ensure ecflow is importable before any test is collected."""
    try:
        import ecflow  # noqa: F401
    except ImportError as exc:
        raise RuntimeError(
            "Could not import ecflow. Make sure the binding is on PYTHONPATH."
        ) from exc
