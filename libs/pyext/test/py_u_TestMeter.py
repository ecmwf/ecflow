# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

import ecflow as ecf


def test_create_meter_from_parameters():
    meter = ecf.Meter("name", 10, 100, 50)
    assert meter.name() == "name"
    assert meter.min() == 10
    assert meter.max() == 100
    assert meter.color_change() == 50


def test_create_meter_from_parameters_with_default_color_change():
    meter = ecf.Meter("name", 10, 100)
    assert meter.name() == "name"
    assert meter.min() == 10
    assert meter.max() == 100
    assert meter.color_change() == 100
