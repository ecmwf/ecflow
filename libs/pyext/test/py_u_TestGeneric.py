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


def can_create_generic_with_basic_parameters():
    generic = ecf.Generic("name", [])

    assert generic.name() == "name"
    assert generic.values == []
    assert generic.empty() == False


def can_identify_an_empty_generic():
    task = ecf.Task("t")
    generic = task.find_generic("nonexisting")

    assert generic.empty() == True


if __name__ == "__main__":
    Test.print_test_start(os.path.basename(__file__))

    can_create_generic_with_basic_parameters()
    can_identify_an_empty_generic()

    print("All tests pass")
