#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

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
