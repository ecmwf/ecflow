# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

import ecflow as ecf


def test_create_generic_with_basic_parameters():
    generic = ecf.Generic("name", [])

    assert generic.name() == "name"
    assert generic.values == []
    assert generic.empty() is False


def test_identify_an_empty_generic():
    task = ecf.Task("t")
    generic = task.find_generic("nonexisting")

    assert generic.empty() is True
