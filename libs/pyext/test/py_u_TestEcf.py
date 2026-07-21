#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# This test ensures that creation of Defs files works as expected.

from ecflow import Ecf


def test_default_debug_level_is_zero():
    assert Ecf.debug_level() == 0, "Expected default debug level to be 0"


def test_debug_level_round_trip():
    Ecf.set_debug_level(10)
    assert Ecf.debug_level() == 10, "Expected debug level to be 10"
    Ecf.set_debug_level(0)
    assert Ecf.debug_level() == 0, "Expected debug level to be 0"


def test_default_debug_equality_is_false():
    assert (
        Ecf.debug_equality() is False
    ), "Expected default for Ecf.debug_equality() == False"


def test_debug_equality_round_trip():
    Ecf.set_debug_equality(True)
    assert Ecf.debug_equality() is True, "Expected debug_equality() == True"
    Ecf.set_debug_equality(False)
    assert Ecf.debug_equality() is False, "Expected Ecf.debug_equality() == False"
