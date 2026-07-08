#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# This test ensures that sorting ecflow objects is correct.

import ecflow as ecf

OBJECT_CTOR = {
    "Meter": lambda name: ecf.Meter(name, 0, 100),
    "Label": lambda name: ecf.Label(name, "value"),
    "Variable": lambda name: ecf.Variable(name, "value"),
    "Limit": lambda name: ecf.Limit(name, 10),
}


def _make_objects(obj, names):
    ctor = OBJECT_CTOR.get(obj, lambda name: getattr(ecf, obj)(name))
    return [ctor(name) for name in names]


def test_sort_objects():
    names = ["zz", "ww", "aa"]
    object_list = [
        "Task",
        "Family",
        "Suite",
        "Event",
        "Meter",
        "Label",
        "Variable",
        "Limit",
    ]
    for obj in object_list:
        assert sorted(_make_objects(obj, names)) == _make_objects(obj, sorted(names)), (
            " sort of " + obj + " failed"
        )


def test_sort_attributes_api():
    nodes = [ecf.Defs(), ecf.Suite("s"), ecf.Family("f"), ecf.Task("t")]
    sort_attr = ["event", "meter", "label", "variable", "all"]
    for attr in sort_attr:
        for node in nodes:
            node.sort_attributes(attr)
            node.sort_attributes(attr, True)
            node.sort_attributes(attr, True, ["/path", "/path2"])

    sort_attr = [
        ecf.AttrType.event,
        ecf.AttrType.meter,
        ecf.AttrType.label,
        ecf.AttrType.variable,
        ecf.AttrType.all,
    ]
    for attr in sort_attr:
        for node in nodes:
            node.sort_attributes(attr)
            node.sort_attributes(attr, True)
            node.sort_attributes(attr, True, ["/path", "/path2"])


def _unsorted_events():
    return [ecf.Event(name) for name in "foo buzz bar google zulu mason".split()]


def _sorted_events():
    return [ecf.Event(name) for name in "bar buzz foo google mason zulu".split()]


def _expected_defs():
    return ecf.Defs(
        ecf.Suite(
            "mysuite",
            ecf.Family("fam1", _unsorted_events()),
            ecf.Family("fam2", _sorted_events(), ecf.Task("task1", _sorted_events())),
            ecf.Task("task2", _sorted_events()),
            _sorted_events(),
        )
    )


def _defs_to_sort():
    return ecf.Defs(
        ecf.Suite(
            "mysuite",
            ecf.Family("fam1", _unsorted_events()),
            ecf.Family(
                "fam2", _unsorted_events(), ecf.Task("task1", _unsorted_events())
            ),
            ecf.Task("task2", _unsorted_events()),
            _unsorted_events(),
        )
    )


def test_no_sort_by_name_with_string_attr_type():
    defs = _defs_to_sort()
    defs.sort_attributes("event", True, ["/mysuite/fam1"])
    assert defs == _expected_defs(), (
        "Expected\n" + str(_expected_defs()) + "but found\n" + str(defs)
    )


def test_no_sort_by_name_with_enum_attr_type():
    defs = _defs_to_sort()
    defs.sort_attributes(ecf.AttrType.event, True, ["/mysuite/fam1"])
    assert defs == _expected_defs(), (
        "Expected\n" + str(_expected_defs()) + "but found\n" + str(defs)
    )


def test_defs_sort_attributes_by_name():
    defs = ecf.Defs()
    defs.add_variable("ZFRED", "/tmp/")
    defs.add_variable(
        "YECF_URL_CMD", "${BROWSER:=firefox} -new-tab %ECF_URL_BASE%/%ECF_URL%"
    )
    defs.add_variable("XECF_URL_BASE", "http://www.ecmwf.int")
    defs.add_variable("AECF_URL", "publications/manuals/sms")
    assert len(list(defs.user_variables)) == 4, "Expected *user* 4 variable"

    expected = ["AECF_URL", "XECF_URL_BASE", "YECF_URL_CMD", "ZFRED"]
    defs.sort_attributes("variable")
    actual = [v.name() for v in defs.user_variables]
    assert actual == expected, (
        "Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)
    )

    expected = ["AECF_URL", "XECF_URL_BASE", "YECF_URL_CMD", "ZFRED", "ZZ"]
    defs.add_variable("ZZ", "x")
    defs.sort_attributes(ecf.AttrType.variable)
    actual = [v.name() for v in defs.user_variables]
    assert actual == expected, (
        "Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)
    )

    expected = ["AA", "AECF_URL", "XECF_URL_BASE", "YECF_URL_CMD", "ZFRED", "ZZ"]
    defs.add_variable("AA", "x")
    defs.sort_attributes("all")
    actual = [v.name() for v in defs.user_variables]
    assert actual == expected, (
        "Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)
    )

    expected = ["AA", "AECF_URL", "BB", "XECF_URL_BASE", "YECF_URL_CMD", "ZFRED", "ZZ"]
    defs.add_variable("BB", "x")
    defs.sort_attributes(ecf.AttrType.all)
    actual = [v.name() for v in defs.user_variables]
    assert actual == expected, (
        "Attributes not sorted, expected:" + str(expected) + " but found:" + str(actual)
    )
