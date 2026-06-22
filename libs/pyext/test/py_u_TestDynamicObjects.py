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

Dynamic-attribute tests for the ecflow Python objects exposed in ExportNodeAttr.cpp.

Goal
----
Prove that every ecflow Python object can be assigned (and have deleted) an
arbitrary Python attribute at runtime.  In pybind11 this capability is enabled
by passing py::dynamic_attr() when the type is bound; the type then carries a
per-instance __dict__.

Each type below gets the same pair of positive assertions, mirroring the
TestVariable.test_supports_dynamic_attribute / test_dynamic_attribute_deletion
tests that previously lived in py_u_TestExportNodeAttr.py:

    * a fresh attribute can be set and read back, and
    * that attribute can subsequently be deleted.

All ecflow types are bound with py::dynamic_attr(), so all tests in this
file are expected to pass.
"""

import ecflow as ecf
import unittest


class TestTrigger(unittest.TestCase):
    """Trigger is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Trigger supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Trigger("a == b")
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Trigger instance."""
        obj = ecf.Trigger("a == b")
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestComplete(unittest.TestCase):
    """Complete is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Complete supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Complete("a == b")
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Complete instance."""
        obj = ecf.Complete("a == b")
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestPartExpression(unittest.TestCase):
    """PartExpression is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """PartExpression supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.PartExpression("a == b")
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a PartExpression instance."""
        obj = ecf.PartExpression("a == b")
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestExpression(unittest.TestCase):
    """Expression is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Expression supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Expression("a == b")
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from an Expression instance."""
        obj = ecf.Expression("a == b")
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestFlagType(unittest.TestCase):
    """FlagType is a py::enum_ bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """FlagType supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.FlagType.force_abort
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a FlagType member."""
        obj = ecf.FlagType.force_abort
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestFlag(unittest.TestCase):
    """Flag is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Flag supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Flag()
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Flag instance."""
        obj = ecf.Flag()
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestFlagTypeVec(unittest.TestCase):
    """FlagTypeVec is a py::bind_vector type WITHOUT py::dynamic_attr().

    Expected to FAIL until the binding adds dynamic-attribute support.
    """

    def test_supports_dynamic_attribute(self):
        """FlagTypeVec should support setting arbitrary Python attributes."""
        obj = ecf.FlagTypeVec()
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute should be deletable from a FlagTypeVec instance."""
        obj = ecf.FlagTypeVec()
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestJobCreationCtrl(unittest.TestCase):
    """JobCreationCtrl is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """JobCreationCtrl supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.JobCreationCtrl()
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a JobCreationCtrl instance."""
        obj = ecf.JobCreationCtrl()
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestZombieType(unittest.TestCase):
    """ZombieType is a py::enum_ WITHOUT py::dynamic_attr().

    Expected to FAIL until the binding adds dynamic-attribute support.
    """

    def test_supports_dynamic_attribute(self):
        """ZombieType should support setting arbitrary Python attributes."""
        obj = ecf.ZombieType.ecf
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute should be deletable from a ZombieType member."""
        obj = ecf.ZombieType.ecf
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestZombieUserActionType(unittest.TestCase):
    """ZombieUserActionType is a py::enum_ WITHOUT py::dynamic_attr().

    Expected to FAIL until the binding adds dynamic-attribute support.
    """

    def test_supports_dynamic_attribute(self):
        """ZombieUserActionType should support setting arbitrary Python attributes."""
        obj = ecf.ZombieUserActionType.fob
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute should be deletable from a ZombieUserActionType member."""
        obj = ecf.ZombieUserActionType.fob
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestChildCmdType(unittest.TestCase):
    """ChildCmdType is a py::enum_ WITHOUT py::dynamic_attr().

    Expected to FAIL until the binding adds dynamic-attribute support.
    """

    def test_supports_dynamic_attribute(self):
        """ChildCmdType should support setting arbitrary Python attributes."""
        obj = ecf.ChildCmdType.init
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute should be deletable from a ChildCmdType member."""
        obj = ecf.ChildCmdType.init
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestAttrType(unittest.TestCase):
    """AttrType is a py::enum_ WITHOUT py::dynamic_attr().

    Expected to FAIL until the binding adds dynamic-attribute support.
    """

    def test_supports_dynamic_attribute(self):
        """AttrType should support setting arbitrary Python attributes."""
        obj = ecf.AttrType.event
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute should be deletable from an AttrType member."""
        obj = ecf.AttrType.event
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestZombieAttr(unittest.TestCase):
    """ZombieAttr is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """ZombieAttr supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.ZombieAttr(ecf.ZombieType.ecf, [], ecf.ZombieUserActionType.fob)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a ZombieAttr instance."""
        obj = ecf.ZombieAttr(ecf.ZombieType.ecf, [], ecf.ZombieUserActionType.fob)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestZombieVec(unittest.TestCase):
    """ZombieVec is a py::bind_vector type WITHOUT py::dynamic_attr().

    Expected to FAIL until the binding adds dynamic-attribute support.
    """

    def test_supports_dynamic_attribute(self):
        """ZombieVec should support setting arbitrary Python attributes."""
        obj = ecf.ZombieVec()
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute should be deletable from a ZombieVec instance."""
        obj = ecf.ZombieVec()
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestZombie(unittest.TestCase):
    """Zombie is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Zombie supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Zombie()
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Zombie instance."""
        obj = ecf.Zombie()
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestVariable(unittest.TestCase):
    """Variable is bound with py::dynamic_attr(); arbitrary attributes are allowed.

    (Moved here from py_u_TestExportNodeAttr.py.)
    """

    def test_supports_dynamic_attribute(self):
        """Variable supports setting arbitrary Python attributes (py::dynamic_attr())."""
        v = ecf.Variable("VAR", "val")
        v.my_custom = "sentinel"
        self.assertEqual(v.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Variable instance."""
        v = ecf.Variable("VAR", "val")
        v.tag = 99
        del v.tag
        self.assertFalse(hasattr(v, "tag"))


class TestVariableList(unittest.TestCase):
    """VariableList is a py::bind_vector type WITHOUT py::dynamic_attr().

    Expected to FAIL until the binding adds dynamic-attribute support.
    """

    def test_supports_dynamic_attribute(self):
        """VariableList should support setting arbitrary Python attributes."""
        obj = ecf.VariableList()
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute should be deletable from a VariableList instance."""
        obj = ecf.VariableList()
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestLabel(unittest.TestCase):
    """Label is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Label supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Label("MY_LABEL", "hello")
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Label instance."""
        obj = ecf.Label("MY_LABEL", "hello")
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestLimit(unittest.TestCase):
    """Limit is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Limit supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Limit("MY_LIMIT", 10)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Limit instance."""
        obj = ecf.Limit("MY_LIMIT", 10)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestInLimit(unittest.TestCase):
    """InLimit is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """InLimit supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.InLimit("my_limit")
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from an InLimit instance."""
        obj = ecf.InLimit("my_limit")
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestEvent(unittest.TestCase):
    """Event is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Event supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Event(1)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from an Event instance."""
        obj = ecf.Event(1)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestMeter(unittest.TestCase):
    """Meter is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Meter supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Meter("my_meter", 0, 100)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Meter instance."""
        obj = ecf.Meter("my_meter", 0, 100)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestQueue(unittest.TestCase):
    """Queue is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Queue supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Queue("my_queue", ["step1", "step2"])
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Queue instance."""
        obj = ecf.Queue("my_queue", ["step1", "step2"])
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestGeneric(unittest.TestCase):
    """Generic is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Generic supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Generic("my_generic", ["val1", "val2"])
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Generic instance."""
        obj = ecf.Generic("my_generic", ["val1", "val2"])
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestDate(unittest.TestCase):
    """Date is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Date supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Date(15, 6, 2024)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Date instance."""
        obj = ecf.Date(15, 6, 2024)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestDays(unittest.TestCase):
    """Days is a py::enum_ WITHOUT py::dynamic_attr().

    Expected to FAIL until the binding adds dynamic-attribute support.
    """

    def test_supports_dynamic_attribute(self):
        """Days should support setting arbitrary Python attributes."""
        obj = ecf.Days.monday
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute should be deletable from a Days member."""
        obj = ecf.Days.monday
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestDay(unittest.TestCase):
    """Day is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Day supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Day(ecf.Days.monday)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Day instance."""
        obj = ecf.Day(ecf.Days.monday)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestTime(unittest.TestCase):
    """Time is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Time supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Time(ecf.TimeSlot(12, 30))
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Time instance."""
        obj = ecf.Time(ecf.TimeSlot(12, 30))
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestToday(unittest.TestCase):
    """Today is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Today supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Today(ecf.TimeSlot(10, 0))
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Today instance."""
        obj = ecf.Today(ecf.TimeSlot(10, 0))
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestLate(unittest.TestCase):
    """Late is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Late supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Late()
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Late instance."""
        obj = ecf.Late()
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestAutocancel(unittest.TestCase):
    """Autocancel is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Autocancel supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Autocancel(1, 30, True)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from an Autocancel instance."""
        obj = ecf.Autocancel(1, 30, True)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestAutoarchive(unittest.TestCase):
    """Autoarchive is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Autoarchive supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Autoarchive(1, 30, True, False)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from an Autoarchive instance."""
        obj = ecf.Autoarchive(1, 30, True, False)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestAutorestore(unittest.TestCase):
    """Autorestore is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Autorestore supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Autorestore(["/suite/family"])
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from an Autorestore instance."""
        obj = ecf.Autorestore(["/suite/family"])
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestRepeatDate(unittest.TestCase):
    """RepeatDate is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """RepeatDate supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.RepeatDate("YMD", 20100101, 20100110)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a RepeatDate instance."""
        obj = ecf.RepeatDate("YMD", 20100101, 20100110)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestRepeatDateTime(unittest.TestCase):
    """RepeatDateTime is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """RepeatDateTime supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a RepeatDateTime instance."""
        obj = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestRepeatDateList(unittest.TestCase):
    """RepeatDateList is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """RepeatDateList supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.RepeatDateList("DL", [20100101])
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a RepeatDateList instance."""
        obj = ecf.RepeatDateList("DL", [20100101])
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestRepeatDateTimeList(unittest.TestCase):
    """RepeatDateTimeList is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """RepeatDateTimeList supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.RepeatDateTimeList("DTL", ["20100101T000000"])
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a RepeatDateTimeList instance."""
        obj = ecf.RepeatDateTimeList("DTL", ["20100101T000000"])
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestRepeatInteger(unittest.TestCase):
    """RepeatInteger is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """RepeatInteger supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.RepeatInteger("VAR", 0, 10)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a RepeatInteger instance."""
        obj = ecf.RepeatInteger("VAR", 0, 10)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestRepeatEnumerated(unittest.TestCase):
    """RepeatEnumerated is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """RepeatEnumerated supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.RepeatEnumerated("COLOR", ["red"])
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a RepeatEnumerated instance."""
        obj = ecf.RepeatEnumerated("COLOR", ["red"])
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestRepeatString(unittest.TestCase):
    """RepeatString is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """RepeatString supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.RepeatString("LABEL", ["only"])
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a RepeatString instance."""
        obj = ecf.RepeatString("LABEL", ["only"])
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestRepeatDay(unittest.TestCase):
    """RepeatDay is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """RepeatDay supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.RepeatDay()
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a RepeatDay instance."""
        obj = ecf.RepeatDay()
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestRepeat(unittest.TestCase):
    """Repeat is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Repeat supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Repeat(1)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Repeat instance."""
        obj = ecf.Repeat(1)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestCron(unittest.TestCase):
    """Cron is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Cron supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Cron()
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Cron instance."""
        obj = ecf.Cron()
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestVerify(unittest.TestCase):
    """Verify is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Verify supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Verify(ecf.State.complete, 3)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Verify instance."""
        obj = ecf.Verify(ecf.State.complete, 3)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestClock(unittest.TestCase):
    """Clock is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """Clock supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.Clock(1, 3, 2024)
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a Clock instance."""
        obj = ecf.Clock(1, 3, 2024)
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestAvisoAttr(unittest.TestCase):
    """AvisoAttr is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """AvisoAttr supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.AvisoAttr()
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from an AvisoAttr instance."""
        obj = ecf.AvisoAttr()
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


class TestMirrorAttr(unittest.TestCase):
    """MirrorAttr is bound with py::dynamic_attr(); arbitrary attributes are allowed."""

    def test_supports_dynamic_attribute(self):
        """MirrorAttr supports setting arbitrary Python attributes (py::dynamic_attr())."""
        obj = ecf.MirrorAttr()
        obj.my_custom = "sentinel"
        self.assertEqual(obj.my_custom, "sentinel")

    def test_dynamic_attribute_deletion(self):
        """A dynamic attribute can be deleted from a MirrorAttr instance."""
        obj = ecf.MirrorAttr()
        obj.tag = 99
        del obj.tag
        self.assertFalse(hasattr(obj, "tag"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
