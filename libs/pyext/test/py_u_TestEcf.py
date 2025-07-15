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

import os
import ecflow

from ecflow import Ecf, Client, debug_build

if __name__ == "__main__":
    print("####################################################################")
    print("Running TestEcf, ecflow version " + Client().version() + " debug build(" + str(debug_build()) + ")")
    print("####################################################################")

    default_debug_level = Ecf.debug_level()
    assert default_debug_level == 0, "Expected default debug level to be 0"
    Ecf.set_debug_level(10)
    assert Ecf.debug_level() == 10, "Expected debug level to be 10"
    Ecf.set_debug_level(0)
    assert Ecf.debug_level() == 0, "Expected debug level to be 0"

    assert Ecf.debug_equality() == False, "Expected  default for Ecf.debug_equality() == False "
    Ecf.set_debug_equality(True)
    assert Ecf.debug_equality() == True, "Expected  debug_equality() == True "
    Ecf.set_debug_equality(False)
    assert Ecf.debug_equality() == False, "Expected  Ecf.debug_equality() == False "

    print("All tests pass")
