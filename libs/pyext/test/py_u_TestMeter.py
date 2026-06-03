#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import os
import ecflow as ecf
import itertools as it

import ecflow_test_util as Test


def can_create_meter_from_parameters():
    meter = ecf.Meter("name", 10, 100, 50)
    assert meter.name() == "name"
    assert meter.min() == 10
    assert meter.max() == 100
    assert meter.color_change() == 50


def can_create_meter_from_parameters_with_default_color_change():
    meter = ecf.Meter("name", 10, 100)
    assert meter.name() == "name"
    assert meter.min() == 10
    assert meter.max() == 100
    assert meter.color_change() == 100


if __name__ == "__main__":
    Test.print_test_start(os.path.basename(__file__))

    can_create_meter_from_parameters()

    print("All tests pass")
