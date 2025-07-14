#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

from ecflow import Alias, AttrType, Autocancel, CheckPt, ChildCmdType, Client, Clock, Cron, DState, Date, Day, Days, \
    Defs, Ecf, Event, Expression, Family, FamilyVec, File, Flag, FlagType, FlagTypeVec, InLimit, \
    JobCreationCtrl, Label, Late, Limit, Meter, Node, NodeContainer, NodeVec, PartExpression, PrintStyle, \
    Repeat, RepeatDate, RepeatDateTime, RepeatDateList, RepeatDay, RepeatEnumerated, RepeatInteger, \
    RepeatString, SState, State, Style, Submittable, Suite, SuiteVec, Task, TaskVec, Time, TimeSeries, \
    TimeSlot, Today, UrlCmd, Variable, VariableList, Verify, WhyCmd, ZombieAttr, ZombieType, \
    ZombieUserActionType, Trigger, Complete, Edit, Defstatus
import unittest
import sys


class Test_dunder_rshift(unittest.TestCase):
    def test_node_dunder_rshift(self):
        # will ONLY work if we have starting NodeContainer
        suite = Suite('s') >> Task('t1') >> Task('t2') >> Task('t3') >> Task('t4')
        self.assertEqual(len(list(suite)), 4, "expected 4 children but found " + str(len(list(suite))))

        self.assertEqual(str(suite.t2.get_trigger()), "t1 == complete", "Trigger not as expected: " + str(suite.t2.get_trigger()))
        self.assertEqual(str(suite.t3.get_trigger()), "t2 == complete", "Trigger not as expected: " + str(suite.t3.get_trigger()))
        self.assertEqual(str(suite.t4.get_trigger()), "t3 == complete", "Trigger not as expected: " + str(suite.t4.get_trigger()))

        fam = Family("f1") >> Task('t1') >> Task('t2') >> Task('t3') >> Task('t4')
        self.assertEqual(len(list(fam)), 4, "expected 4 children but found " + str(len(list(fam))))

        self.assertEqual(str(fam.t2.get_trigger()), "t1 == complete", "Trigger not as expected: " + str(fam.t2.get_trigger()))
        self.assertEqual(str(fam.t3.get_trigger()), "t2 == complete", "Trigger not as expected: " + str(fam.t3.get_trigger()))
        self.assertEqual(str(fam.t4.get_trigger()), "t3 == complete", "Trigger not as expected: " + str(fam.t4.get_trigger()))

    def test_node_dunder_rshift_trigger_cat(self):
        suite = Suite('s')
        # will ONLY work if we have starting NodeContainer
        suite >> Task('t1') >> (Task('t2') + Trigger("x == 1")) >> (Task('t3') + Trigger("y==1")) >> Task('t4')
        self.assertEqual(len(list(suite)), 4, "expected 4 children but found " + str(len(list(suite))))

        self.assertEqual(str(suite.t2.get_trigger()), "x == 1 AND t1 == complete", "Trigger not as expected: " + str(suite.t2.get_trigger()))
        self.assertEqual(str(suite.t3.get_trigger()), "y==1 AND t2 == complete", "Trigger not as expected: " + str(suite.t3.get_trigger()))
        self.assertEqual(str(suite.t4.get_trigger()), "t3 == complete", "Trigger not as expected: " + str(suite.t4.get_trigger()))


class Test_dunder_lshift(unittest.TestCase):
    def test_node_dunder_lshift(self):
        # will ONLY work if we have starting NodeContainer
        suite = Suite('s') << Task('t1') << Task('t2') << Task('t3') << Task('t4')
        self.assertEqual(len(list(suite)), 4, "expected 4 children but found " + str(len(list(suite))))

        self.assertEqual(suite.t4.get_trigger(), None, "Trigger not as expected: " + str(suite.t4.get_trigger()))
        self.assertEqual(str(suite.t3.get_trigger()), "t4 == complete", "Trigger not as expected: " + str(suite.t3.get_trigger()))
        self.assertEqual(str(suite.t2.get_trigger()), "t3 == complete", "Trigger not as expected: " + str(suite.t2.get_trigger()))
        self.assertEqual(str(suite.t1.get_trigger()), "t2 == complete", "Trigger not as expected: " + str(suite.t1.get_trigger()))

        fam = Family("f1") << Task('t1') << Task('t2') << Task('t3') << Task('t4')
        self.assertEqual(len(list(fam)), 4, "expected 4 children but found " + str(len(list(fam))))

        self.assertEqual(fam.t4.get_trigger(), None, "Trigger not as expected: " + str(fam.t4.get_trigger()))
        self.assertEqual(str(fam.t3.get_trigger()), "t4 == complete", "Trigger not as expected: " + str(fam.t3.get_trigger()))
        self.assertEqual(str(fam.t2.get_trigger()), "t3 == complete", "Trigger not as expected: " + str(fam.t2.get_trigger()))
        self.assertEqual(str(fam.t1.get_trigger()), "t2 == complete", "Trigger not as expected: " + str(fam.t1.get_trigger()))

    def test_node_dunder_lshift_trigger_cat(self):
        suite = Suite('s')
        # will ONLY work if we have starting NodeContainer
        suite >> Task('t1') << (Task('t2') + Trigger("x == 1")) << (Task('t3') + Trigger("y==1")) << Task('t4')
        self.assertEqual(len(list(suite)), 4, "expected 4 children but found " + str(len(list(suite))))

        self.assertEqual(suite.t4.get_trigger(), None, "Trigger not as expected: " + str(suite.t4.get_trigger()))
        self.assertEqual(str(suite.t3.get_trigger()), "y==1 AND t4 == complete", "Trigger not as expected: " + str(suite.t3.get_trigger()))
        self.assertEqual(str(suite.t2.get_trigger()), "x == 1 AND t3 == complete", "Trigger not as expected: " + str(suite.t2.get_trigger()))
        self.assertEqual(str(suite.t1.get_trigger()), "t2 == complete", "Trigger not as expected: " + str(suite.t1.get_trigger()))


class Test_dunder_add(unittest.TestCase):

    def test_node_dunder_add_variable(self):
        # will ONLY work if we have starting NodeContainer
        suite = Suite("s") + Family("f") + Family("f2") + Task("t3") + Edit(name="value")
        self.assertEqual(len(list(suite.variables)), 1, "expected suite to have 1 variable " + str(len(list(suite.variables))))

        suite = Suite("s") + Family("f") + Family("f2") + (Task("t3") + Edit(name="value"))
        self.assertEqual(len(list(suite.t3.variables)), 1, "expected task t3 to have 1 variable " + str(len(list(suite.t3.variables))))

    def test_defs_dunder_add(self):
        # will ONLY work if we have starting defs
        defs = Defs() + Suite("s") + Suite("s1")
        self.assertEqual(len(defs), 2, "expected 2 suites but found " + str(len(defs)))

        defs += Suite("sx")  # + Suite('x')  wont work,   Suite('sx').__add__(Suite('x') not defined
        self.assertEqual(len(defs), 3, "expected 3 suites but found " + str(len(defs)))

        defs + Suite("a") + Suite("b")
        self.assertEqual(len(defs), 5, "expected 5 suites but found " + str(len(defs)))

    def test_node_dunder_add_meter(self):
        # will ONLY work if we have starting NodeContainer
        suite = Suite("s") + Family("f") + Family("f2") + Task("t3") + Edit(name="value")
        suite.t3 + Event(11, "event") + Meter("meter", 0, 10, 10) + Label("label", "c") + Trigger("1==1")
        self.assertEqual(len(suite), 3, "expected 3 children but found " + str(len(suite)))
        self.assertEqual(len(list(suite.t3.events)), 1, "expected 1 event found " + str(len(list(suite.t3.events))))
        self.assertEqual(len(list(suite.t3.meters)), 1, "expected 1 event found " + str(len(list(suite.t3.meters))))
        self.assertEqual(len(list(suite.t3.labels)), 1, "expected 1 event found " + str(len(list(suite.t3.labels))))

    def test_node_dunder_add(self):
        suite = Suite("s")
        suite += Task("t3")  # + Task("t2") not allowed as Task("t1").__add__("t2") not defined
        suite.t3 += Event(11, "event")  # + Task("t2") not allowed as Task("t1").__add__("t2") not defined
        self.assertEqual(len(suite), 1, "expected 1 children but found " + str(len(suite)))
        self.assertEqual(len(list(suite.t3.events)), 1, "expected 1 event found " + str(len(list(suite.t3.events))))

        suite = Suite('s') + [Task("t{0}".format(i)) for i in range(1, 5)]
        self.assertEqual(len(suite), 4, "expected 4 children but found " + str(len(suite)))

    def test_dunder_add_all(self):
        defs = Defs() + Suite("s1")
        defs.s1 += Clock(1, 1, 2010, False)
        defs.s1 += Autocancel(1, 10, True)
        defs.s1 += Task('t1') + Edit({"a": "y", "b": "bb"}, c="v", d="b") + Edit({"e": 1, "f": "bb"}) + Edit(g="d") + Edit(h=1) + \
                   Event(1) + Event(11, "event") + Meter("meter", 0, 10, 10) + Label("label", "c") + Trigger("1==1") + \
                   Complete("1==1") + Limit("limit", 10) + Limit("limit2", 10) + InLimit("limitName", "/limit", 2) + \
                   Defstatus(DState.complete) + Today(0, 30) + Today("00:59") + Today("00:00 11:30 00:01") + \
                   Time(0, 30) + Time("00:59") + Time("00:00 11:30 00:01") + Day("sunday") + Day(Days.monday) + \
                   Date(1, 1, 0) + Date(28, 2, 1960) + Autocancel(3)

        t1 = defs.find_abs_node("/s1/t1")
        self.assertTrue(t1 is not None, "Can't find t1")
        self.assertEqual(len(defs.s1), 1, "Expected 1 nodes but found " + str(len(defs.s1)))
        self.assertEqual(len(list(t1.variables)), 8, "expected 8 variables but found " + str(len(list(t1.variables))))
        self.assertEqual(len(list(t1.limits)), 2, "expected 2 limits")
        self.assertEqual(len(list(t1.inlimits)), 1, "expected 1 inlimits")
        self.assertEqual(len(list(t1.events)), 2, "expected 2 events")
        self.assertEqual(len(list(t1.meters)), 1, "expected 1 meter")
        self.assertEqual(len(list(t1.labels)), 1, "expected 1 label")
        self.assertEqual(len(list(t1.times)), 3, "expected 3 times")
        self.assertEqual(len(list(t1.todays)), 3, "expected 3 times")
        self.assertEqual(len(list(t1.days)), 2, "expected 2 days")
        self.assertEqual(len(list(t1.dates)), 2, "expected 2 dates")
        self.assertTrue(t1.get_trigger(), "Can't find t1 trigger")
        self.assertTrue(t1.get_complete(), "Can't find t1 complete")
        self.assertTrue(t1.get_autocancel(), "Can't find t1 autocancel")
        # print(defs)


class Test_crash(unittest.TestCase):
    def test_trigger_node_list(self):
        suite = Suite("s")
        fam = suite.add_family("f")
        task = fam.add_task("t")
        self.assertEqual(task.get_abs_node_path(), "/s/f/t", "Path not as expected " + task.get_abs_node_path())

        suite = Suite("s")
        fam = Family("f")
        task = Task("t")
        suite.add_family(fam)
        task = fam.add_task(task)
        self.assertEqual(task.get_abs_node_path(), "/s/f/t", "Path not as expected " + task.get_abs_node_path())

        defs = Defs()
        task = defs.add_suite("s").add_family("f").add_task("t")
        self.assertEqual(task.get_abs_node_path(), "/s/f/t", "Path not as expected " + task.get_abs_node_path())

        # The following will crash or cause memory error. Since the Defs() is transitory/temp()
        # Unlike the previous example where we keep the defs in a variable.
        # task = Defs().add_suite("s").add_family("f").add_task("t")
        # self.assertEqual(task.get_abs_node_path(),"/s/f/t","Path not as expected " + task.get_abs_node_path() )

        # Like wise this will crash since the Suite() is transitory reference counted, and goes out of scope.


#         task = Suite("s").add_family("f").add_task("t")
#         self.assertEqual(task.get_abs_node_path(),"/s/f/t","Path not as expected " + task.get_abs_node_path() )

class TestListComprehension(unittest.TestCase):
    def test_suite_list(self):
        defs = Defs()
        defs += [Suite("s{0}".format(i)) for i in range(1, 6)]
        self.assertEqual(len(defs), 5, " expected 5 suites but found " + str(len(defs)))

        defs.add([Suite("s{0}".format(i)) for i in range(6, 11)])
        self.assertEqual(len(defs), 10, " expected 10 suites but found " + str(len(defs)))

        defs = Defs([Suite("s{0}".format(i)) for i in range(1, 6)])
        self.assertEqual(len(defs), 5, " expected 5 suites but found " + str(len(defs)))

    def test_family_list(self):
        defs = Defs()
        defs += [Suite("suite").add([Family("f{0}".format(i)) for i in range(1, 6)])]
        self.assertEqual(len(defs.suite), 5, " expected 5 familes but found " + str(len(defs.suite)))

        defs = Defs([Suite("xx", [Family("f{0}".format(i)) for i in range(1, 6)])])
        self.assertEqual(len(defs.xx), 5, " expected 5 familes but found " + str(len(defs.xx)))

    def test_task_list(self):
        defs = Defs()
        defs += [Suite("suite").add(Family("f").add([Task("t{0}".format(i)) for i in range(1, 6)]))]
        self.assertEqual(len(defs.suite.f), 5, " expected 5 task but found " + str(len(defs.suite.f)))

        defs = Defs([Suite("suite", Family("f", [Task("t{0}".format(i)) for i in range(1, 6)]))])
        self.assertEqual(len(defs.suite.f), 5, " expected 5 task but found " + str(len(defs.suite.f)))

    def test_task_list2(self):
        defs = Defs()
        defs += [Suite("suite").add(Task("x"),
                                    Family("f").add([Task("t{0}".format(i)) for i in range(1, 6)]),
                                    Task("y"),
                                    [Family("f{0}".format(i)) for i in range(1, 6)],
                                    Edit(a="b"),
                                    [Task("t{0}".format(i)) for i in range(1, 6)],
                                    )
                 ]
        self.assertEqual(len(defs.suite), 13, " expected 13 nodes but found " + str(len(defs.suite)))
        self.assertEqual(len(defs.suite.f), 5, " expected 5 nodes but found " + str(len(defs.suite.f)))
        self.assertEqual(len(list(defs.suite.variables)), 1, " expected 1 variable " + str(len(list(defs.suite.variables))))

    def test_5Suite_with_5families_with_5tasks(self):
        defs = Defs().add(
            [Suite("s{0}".format(i)).add(
                [Family("f{0}".format(i)).add(
                    [Task("t{0}".format(i))
                     for i in range(1, 6)])
                    for i in range(1, 6)])
                for i in range(1, 6)])
        # print defs
        self.assertEqual(len(defs), 5, " expected 5 suites but found " + str(len(defs)))
        for suite in defs:
            self.assertEqual(len(suite), 5, " expected 5 familes but found " + str(len(suite)))
            for fam in suite:
                self.assertEqual(len(fam), 5, " expected 5 tasks but found " + str(len(fam)))

    def test__5Suite_with_5families_with_5tasks(self):
        defs = Defs()
        defs += [Suite("s{0}".format(i)).add(
            [Family("f{0}".format(i)).add(
                [Task("t{0}".format(i)) for i in range(1, 6)])
                for i in range(1, 6)])
            for i in range(1, 6)
        ]
        # print defs
        self.assertEqual(len(defs), 5, " expected 5 suites but found " + str(len(defs)))
        for suite in defs:
            self.assertEqual(len(suite), 5, " expected 5 familes but found " + str(len(suite)))
            for fam in suite:
                self.assertEqual(len(fam), 5, " expected 5 tasks but found " + str(len(fam)))


class TestTrigger(unittest.TestCase):
    def test_trigger(self):
        t = Trigger("a == complete")
        self.assertEqual(str(t), "a == complete", "Trigger not as expected:" + str(t))

    def test_trigger_string_list(self):
        t = Trigger(["a"])
        self.assertEqual(str(t), "a == complete", "Trigger not as expected: " + str(t))

        t = Trigger(["a == 1"])
        self.assertEqual(str(t), "a == 1", "Trigger not as expected: " + str(t))

        t = Trigger(["a", "b", "c"])
        self.assertEqual(str(t), "a == complete AND b == complete AND c == complete", "Trigger not as expected: " + str(t))

    def test_trigger_node_list(self):
        t = Trigger([Task("a")])
        self.assertEqual(str(t), "a == complete", "Trigger not as expected: " + str(t))

        defs = Defs()
        task = defs.add_suite("s").add_family("f").add_task("t")
        self.assertEqual(task.get_abs_node_path(), "/s/f/t", "Path not as expected " + task.get_abs_node_path())
        t = Trigger([task])
        self.assertEqual(str(t), "/s/f/t == complete", "Trigger not as expected: " + str(t))

    def test_trigger_string_and_node_list(self):
        defs = Defs()
        task = defs.add_suite("s").add_family("f").add_task("t")
        t = Trigger(["a", "b", task])
        self.assertEqual(str(t), "a == complete AND b == complete AND /s/f/t == complete", "Trigger not as expected: " + str(t))
        task += t
        self.assertEqual(str(task.get_trigger()), "a == complete AND b == complete AND /s/f/t == complete", "Trigger not as expected: " + str(task.get_trigger()))

    def test_add(self):
        defs = Defs()
        task = defs.add_suite("s").add_family("f").add_task("t")
        task.add(Trigger("1==1"))
        expr = task.get_trigger()
        self.assertEqual(str(expr), "1==1", "Trigger not as expected: " + expr.get_expression())

        task.add(Trigger("2==1"))
        self.assertEqual(str(expr), "1==1 AND 2==1", "Trigger not as expected: " + str(expr))

    def test_add_composition(self):
        defs = Defs()
        task = defs.add_suite("s").add_family("f").add_task("t")
        task.add(Trigger("1==1"))
        task.add(Trigger("1==2", False))
        task.add(Trigger("x==2"))
        expr = task.get_trigger()
        self.assertEqual(str(expr), "1==1 OR 1==2 AND x==2", "Trigger not as expected: " + expr.get_expression())


class TestComplete(unittest.TestCase):
    def test_trigger(self):
        t = Complete("a == complete")
        self.assertEqual(str(t), "a == complete", "Complete not as expected:" + str(t))

    def test_Complete_string_list(self):
        t = Complete(["a"])
        self.assertEqual(str(t), "a == complete", "Complete not as expected: " + str(t))

        t = Complete(["a", "b", "c"])
        self.assertEqual(str(t), "a == complete AND b == complete AND c == complete", "Complete not as expected: " + str(t))

    def test_Complete_node_list(self):
        t = Complete([Task("a")])
        self.assertEqual(str(t), "a == complete", "Complete not as expected: " + str(t))

        defs = Defs()
        task = defs.add_suite("s").add_family("f").add_task("t")
        self.assertEqual(task.get_abs_node_path(), "/s/f/t", "Path not as expected " + task.get_abs_node_path())
        t = Complete([task])
        self.assertEqual(str(t), "/s/f/t == complete", "Complete not as expected: " + str(t))

    def test_Complete_string_and_node_list(self):
        defs = Defs()
        task = defs.add_suite("s").add_family("f").add_task("t")
        t = Complete(["a", "b", task])
        self.assertEqual(str(t), "a == complete AND b == complete AND /s/f/t == complete", "Complete not as expected: " + str(t))

    def test_add(self):
        defs = Defs()
        task = defs.add_suite("s").add_family("f").add_task("t")
        task.add(Complete("1==1"))
        expr = task.get_complete()
        self.assertEqual(str(expr), "1==1", "Complete not as expected: " + expr.get_expression())

        task.add(Complete("2==1"))
        self.assertEqual(str(expr), "1==1 AND 2==1", "Complete not as expected: " + str(expr))

    def test_add_composition(self):
        defs = Defs()
        task = defs.add_suite("s").add_family("f").add_task("t")
        task.add(Complete("1==1"))
        task.add(Complete("1==2", False))
        task.add(Complete("x==2"))
        expr = task.get_complete()
        self.assertEqual(str(expr), "1==1 OR 1==2 AND x==2", "Trigger not as expected: " + expr.get_expression())


class TestDefsAdd(unittest.TestCase):
    def test_add_suite1(self):
        defs = Defs().add(Suite("a"))
        self.assertEqual(len(defs), 1, "Expected 1 suite to be added")

    def test_add_suite3(self):
        defs = Defs().add(Suite("a"), Suite("b"), Suite("c").add(Edit(a="b")))
        self.assertEqual(len(defs), 3, "Expected 3 suite to be added")

    def test_add_variables(self):
        defs = Defs().add(Edit(a="b", b='c', d='e'))
        self.assertEqual(len(list(defs.user_variables)), 3, "Expected 3 variables to be added")

    def test_add_variables2(self):
        defs = Defs(Edit(a="b", b='c', d='e'))
        self.assertEqual(len(list(defs.user_variables)), 3, "Expected 3 variables to be added")


class TestSuiteAdd(unittest.TestCase):
    def test_add_suite1(self):
        s = Suite("a").add(Family("a").add(Edit(a="b")))
        self.assertEqual(len(s), 1, "Expected 1 child to be added")


class TestFamilyAdd(unittest.TestCase):
    def test_add_task(self):
        s = Family("a").add(Task("a").add(Edit(a="b")))
        self.assertEqual(len(s), 1, "Expected 1 child to be added")


class TestDefstatus(unittest.TestCase):
    def test_illegal_defstatus(self):
        if sys.version_info[0] == 2 and sys.version_info[1] >= 7:
            with self.assertRaises(RuntimeError):
                Defstatus("fred")


class TestEdit(unittest.TestCase):
    def test_add_variable(self):
        defs = Defs().add(Suite("s").add(Edit(a=1, b=1), Edit(c="c")))
        self.assertEqual(len(list(defs.s.variables)), 3, "expected 3 variables")

    def test_edit(self):
        t = Task("t1").add(
            Edit({"a": "y", "b": "bb"}, c="v", d="b"),
            Edit({"e": "1", "f": "bb"}),
            Edit(g="d"),
            Edit(h="1"))
        self.assertEqual(len(list(t.variables)), 8, "expected 8 variables but found " + str(len(list(t.variables))))


class TestAddAll(unittest.TestCase):
    def test_add(self):
        defs = Defs().add(
            Suite("s1").add(
                Clock(1, 1, 2010, False),
                Autocancel(1, 10, True),
                Task("t1").add(
                    Edit({"a": "y", "b": "bb"}, c="v", d="b"),
                    Edit({"e": 1, "f": "bb"}),
                    Edit(g="d"),
                    Edit(h=1),
                    Event(1),
                    Event(11, "event"),
                    Meter("meter", 0, 10, 10),
                    Label("label", "c"),
                    Trigger("1==1"),
                    Complete("1==1"),
                    Limit("limit", 10), Limit("limit2", 10),
                    InLimit("limitName", "/limit", 2),
                    Defstatus(DState.complete),
                    Today(0, 30), Today("00:59"), Today("00:00 11:30 00:01"),
                    Time(0, 30), Time("00:59"), Time("00:00 11:30 00:01"),
                    Day("sunday"), Day(Days.monday),
                    Date(1, 1, 0), Date(28, 2, 1960),
                    Autocancel(3)
                ),
                [Family("f{0}".format(i)) for i in range(1, 6)]
            )
        )
        t1 = defs.find_abs_node("/s1/t1")
        self.assertTrue(t1 is not None, "Can't find t1")
        self.assertEqual(len(defs.s1), 6, "Expected 6 nodes but found " + str(len(defs.s1)))
        self.assertEqual(len(list(t1.variables)), 8, "expected 8 variables")
        self.assertEqual(len(list(t1.limits)), 2, "expected 2 limits")
        self.assertEqual(len(list(t1.inlimits)), 1, "expected 1 inlimits")
        self.assertEqual(len(list(t1.events)), 2, "expected 2 events")
        self.assertEqual(len(list(t1.meters)), 1, "expected 1 meter")
        self.assertEqual(len(list(t1.labels)), 1, "expected 1 label")
        self.assertEqual(len(list(t1.times)), 3, "expected 3 times")
        self.assertEqual(len(list(t1.todays)), 3, "expected 3 times")
        self.assertEqual(len(list(t1.days)), 2, "expected 2 days")
        self.assertEqual(len(list(t1.dates)), 2, "expected 2 dates")
        self.assertTrue(t1.get_trigger(), "Can't find t1 trigger")
        self.assertTrue(t1.get_complete(), "Can't find t1 complete")
        self.assertTrue(t1.get_autocancel(), "Can't find t1 autocancel")
        # print(defs)

    def test_raw_constructor(self):
        defs = Defs().add(
            Suite("s1",
                  Clock(1, 1, 2010, False),
                  Autocancel(1, 10, True),
                  Task("t1",
                       Edit({"a": "y", "b": "bb"}, c="v", d="b"),
                       Edit({"e": 1, "f": "bb"}),
                       Edit(g="d"),
                       Edit(h=1),
                       Event(1),
                       Event(11, "event"),
                       Meter("meter", 0, 10, 10),
                       Label("label", "c"),
                       Trigger("1==1"),
                       Complete("1==1"),
                       Limit("limit", 10), Limit("limit2", 10),
                       InLimit("limitName", "/limit", 2),
                       Defstatus(DState.complete),
                       Today(0, 30), Today("00:59"), Today("00:00 11:30 00:01"),
                       Time(0, 30), Time("00:59"), Time("00:00 11:30 00:01"),
                       Day("sunday"), Day(Days.monday),
                       Date(1, 1, 0), Date(28, 2, 1960),
                       Autocancel(3)
                       ),
                  [Family("f{0}".format(i)) for i in range(1, 6)]
                  )
        )
        print(defs)
        t1 = defs.find_abs_node("/s1/t1")
        self.assertTrue(t1 is not None, "Can't find t1")
        self.assertEqual(len(defs.s1), 6, "Expected 6 nodes but found " + str(len(defs.s1)))
        self.assertEqual(len(list(t1.variables)), 8, "expected 8 variables")
        self.assertEqual(len(list(t1.limits)), 2, "expected 2 limits")
        self.assertEqual(len(list(t1.inlimits)), 1, "expected 1 inlimits")
        self.assertEqual(len(list(t1.events)), 2, "expected 2 events")
        self.assertEqual(len(list(t1.meters)), 1, "expected 1 meter")
        self.assertEqual(len(list(t1.labels)), 1, "expected 1 label")
        self.assertEqual(len(list(t1.times)), 3, "expected 3 times")
        self.assertEqual(len(list(t1.todays)), 3, "expected 3 times")
        self.assertEqual(len(list(t1.days)), 2, "expected 2 days")
        self.assertEqual(len(list(t1.dates)), 2, "expected 2 dates")
        self.assertTrue(t1.get_trigger(), "Can't find t1 trigger")
        self.assertTrue(t1.get_complete(), "Can't find t1 complete")
        self.assertTrue(t1.get_autocancel(), "Can't find t1 autocancel")


class TestIAdd(unittest.TestCase):
    def test_iadd_with_getattr(self):
        defs = Defs();
        s1 = defs.add_suite("s1")
        s1 += [Task("a"), Task("b")]
        defs.s1.a += [Edit({"x1": "y", "aa1": "bb"}, a="v", b="b", ),
                      Edit({"x": "y", "aa": "bb"}),
                      Edit(d="d")
                      ]
        defs.s1.b += [Event(1),
                      Event(11, "event"),
                      Meter("meter", 0, 10, 10),
                      Label("label", "c"),
                      ]
        self.assertEqual(len(list(defs.s1.a.variables)), 7, "expected 7 variables")
        self.assertEqual(len(list(defs.s1.b.events)), 2, "expected 2 events")
        self.assertEqual(len(list(defs.s1.b.meters)), 1, "expected 1 meter")
        self.assertEqual(len(list(defs.s1.b.labels)), 1, "expected 1 label")

    def test_iadd(self):
        defs = Defs();
        defs += [Suite("s2"), Edit({"x1": "y", "aa1": "bb"}, a="v", b="b", )]
        s1 = defs.add_suite("s1")
        s1 += [Clock(1, 1, 2010, False), Autocancel(1, 10, True)]  # + returns the same node back
        t1 = s1.add_task("t1")
        t1 += [Edit({"x1": "y", "aa1": "bb"}, a="v", b="b", ),
               Edit({"x": "y", "aa": "bb"}),
               Edit(d="d"),
               Event(1),
               Event(11, "event"),
               Meter("meter", 0, 10, 10),
               Label("label", "c"),
               Trigger("1==1"),
               Complete("1==1"),
               Limit("limit", 10), Limit("limit2", 10),
               InLimit("limitName", "/limit", 2),
               Defstatus(DState.complete),
               Today(0, 30), Today("00:59"), Today("00:00 11:30 00:01"),
               Time(0, 30), Time("00:59"), Time("00:00 11:30 00:01"),
               Day("sunday"), Day(Days.monday),
               Date(1, 1, 0), Date(28, 2, 1960),
               Autocancel(3)
               ]
        self.assertTrue(t1 is not None, "Can't find t1")
        self.assertEqual(len(list(defs.suites)), 2, "expected 7 suites")
        self.assertEqual(len(list(t1.variables)), 7, "expected 7 variables")
        self.assertEqual(len(list(t1.limits)), 2, "expected 2 limits")
        self.assertEqual(len(list(t1.inlimits)), 1, "expected 1 inlimits")
        self.assertEqual(len(list(t1.events)), 2, "expected 2 events")
        self.assertEqual(len(list(t1.meters)), 1, "expected 1 meter")
        self.assertEqual(len(list(t1.labels)), 1, "expected 1 label")
        self.assertEqual(len(list(t1.times)), 3, "expected 3 times")
        self.assertEqual(len(list(t1.todays)), 3, "expected 3 times")
        self.assertEqual(len(list(t1.days)), 2, "expected 2 days")
        self.assertEqual(len(list(t1.dates)), 2, "expected 2 dates")
        self.assertTrue(t1.get_trigger(), "Can't find t1 trigger")
        self.assertTrue(t1.get_complete(), "Can't find t1 complete")
        self.assertTrue(t1.get_autocancel(), "Can't find t1 autocancel")
        # print(defs)


class TestComparison(unittest.TestCase):
    def setUp(self):
        self.cron = Cron()
        self.cron.set_time_series(TimeSeries(TimeSlot(23, 0), True))  # True means relative to suite start
        self.late = Late()
        self.late.submitted(TimeSlot(20, 10))
        self.late.active(TimeSlot(20, 10))
        self.late.complete(TimeSlot(20, 10), True)
        self.late2 = Late()
        self.late2.submitted(20, 10)
        self.late2.active(20, 10)
        self.late2.complete(20, 10, True)

        self.defs1 = Defs()
        self.defs1.add_suite("s1").add_task("t1").add_variable("a", "v").add_event(1).add_event(11, "event").add_meter("meter", 0, 10, 10).add_label("label", "c")
        self.defs1.add_suite("s2").add_family("f1").add_task("t1").add_trigger("1==1").add_complete("1==1")
        self.defs1.add_suite("s3").add_family("f1").add_family("f2").add_task("t1").add_variable("var", "v")
        self.defs1.add_suite("edit").add_variable("a", "a").add_variable("b", "b")
        self.defs1.add_suite("limit").add_limit("limit", 10).add_limit("limit2", 10)
        self.defs1.add_suite("inlimit").add_inlimit("limitName", "/limit", 2)
        self.defs1.add_suite("RepeatInteger").add_repeat(RepeatInteger("integer", 0, 100, 2))
        self.defs1.add_suite("RepeatEnumerated").add_repeat(RepeatEnumerated("enum", ["red", "green", "blue"]))
        self.defs1.add_suite("RepeatDate").add_repeat(RepeatDate("ymd", 20100111, 20100115, 2))
        self.defs1.add_suite("RepeatDateTime").add_repeat(RepeatDateTime("ymd", "20100111T000000", "20100115T000000", "48:00:00"))
        self.defs1.add_suite("RepeatDateList").add_repeat(RepeatDateList("ymd", [20100111, 20100115]))
        self.defs1.add_suite("RepeatString").add_repeat(RepeatString("string", ["a", "b", "c"]))
        self.defs1.add_suite("RepeatDay").add_repeat(RepeatDay(1))
        self.defs1.add_suite("defstatus").add_defstatus(DState.active)
        self.defs1.add_suite("today").add_task("today").add_today("00:30").add_today(0, 59).add_today("00:00 11:30 00:01")
        self.defs1.add_suite("time").add_task("time").add_time("00:30").add_time(0, 59).add_time("00:00 11:30 00:01")
        self.defs1.add_suite("day").add_task("day").add_day("sunday").add_day(Days.monday)
        self.defs1.add_suite("date").add_task("date").add_date(1, 1, 0).add_date(28, 2, 1960)
        self.defs1.add_suite("cron").add_task("cron").add_cron(self.cron)
        self.defs1.add_suite("late").add_family("late").add_late(self.late).add_task("late").add_late(self.late2)
        self.defs1.add_suite("clock").add_clock(Clock(1, 1, 2010, False))
        self.defs1.add_suite("autocancel").add_autocancel(3)

        zombie_suite = self.defs1.add_suite("zombie")
        self.zombie_life_time_in_server = 800
        self.child_list = [ChildCmdType.init, ChildCmdType.event, ChildCmdType.meter, ChildCmdType.label, ChildCmdType.wait, ChildCmdType.abort, ChildCmdType.complete]
        zombie_type_list = [ZombieType.ecf, ZombieType.user, ZombieType.path]
        for zombie_type in zombie_type_list:
            zombie_attr = ZombieAttr(zombie_type, self.child_list, ZombieUserActionType.block, self.zombie_life_time_in_server)
            zombie_suite.add_zombie(zombie_attr)

    def test_compare_with_add(self):
        defs = Defs().add(
            Suite("s1").add(
                Task("t1").add(
                    Edit(a="v"),
                    Event(1),
                    Event(11, "event"),
                    Meter("meter", 0, 10, 10),
                    Label("label", "c")
                )
            ),
            Suite("s2").add(
                Family("f1").add(
                    Task("t1").add(
                        Trigger("1==1"),
                        Complete("1==1")
                    )
                )
            ),
            Suite("s3").add(
                Family("f1").add(
                    Family("f2").add(
                        Task("t1").add(
                            Edit(var="v")
                        )
                    )
                )
            ),
            Suite("edit").add(Edit({"a": "a"}), Edit({"b": "b"})),  # add separate, otherwise with dict order is undefined
            Suite("limit").add(Limit("limit", 10), Limit("limit2", 10)),
            Suite("inlimit").add(InLimit("limitName", "/limit", 2)),
            Suite("RepeatInteger").add(RepeatInteger("integer", 0, 100, 2)),
            Suite("RepeatEnumerated").add(RepeatEnumerated("enum", ["red", "green", "blue"])),
            Suite("RepeatDate").add(RepeatDate("ymd", 20100111, 20100115, 2)),
            Suite("RepeatDateTime").add(RepeatDateTime("ymd", "20100111T000000", "20100115T000000", "48:00:00")),
            Suite("RepeatDateList").add(RepeatDateList("ymd", [20100111, 20100115])),
            Suite("RepeatString").add(RepeatString("string", ["a", "b", "c"])),
            Suite("RepeatDay").add(RepeatDay(1)),
            Suite("defstatus").add(Defstatus(DState.active)),
            Suite("today").add(
                Task("today").add(
                    Today(0, 30), Today("00:59"), Today("00:00 11:30 00:01")
                )
            ),
            Suite("time").add(
                Task("time").add(
                    Time(0, 30), Time("00:59"), Time("00:00 11:30 00:01")
                )
            ),
            Suite("day").add(
                Task("day").add(
                    Day("sunday"), Day(Days.monday)
                )
            ),
            Suite("date").add(
                Task("date").add(
                    Date(1, 1, 0), Date(28, 2, 1960)
                )
            ),
            Suite("cron").add(
                Task("cron").add(
                    self.cron
                )
            ),
            Suite("late").add(
                Family("late").add(
                    self.late,
                    Task("late").add(
                        self.late2
                    )
                )
            ),
            Suite("clock").add(Clock(1, 1, 2010, False)),
            Suite("autocancel").add(Autocancel(3)),
            Suite("zombie").add(
                ZombieAttr(ZombieType.ecf, self.child_list, ZombieUserActionType.block, self.zombie_life_time_in_server),
                ZombieAttr(ZombieType.user, self.child_list, ZombieUserActionType.block, self.zombie_life_time_in_server),
                ZombieAttr(ZombieType.path, self.child_list, ZombieUserActionType.block, self.zombie_life_time_in_server)
            )
        )

        self.assertEqual(self.defs1, defs, "defs not equal\n" + str(self.defs1) + "\n\n" + str(defs))

    def test_compare_with_raw_constructor(self):
        defs = Defs()
        defs += Suite('s1', Task('t1', Event(1), Event(11, 'event'), Meter("meter", 0, 10, 10), Label("label", "c"), a='v'))
        defs += Suite('s2', Family('f1', Task('t1', Trigger("1==1"), Complete("1==1"))))
        defs += Suite('s3', Family('f1', Family('f2', Task('t1', var='v'))))
        defs += Suite('edit', Edit(a='a'), Edit(b='b'))
        defs += Suite('limit', Limit("limit", 10), Limit("limit2", 10))
        defs += Suite('inlimit', InLimit("limitName", "/limit", 2))
        defs += Suite('RepeatInteger', RepeatInteger("integer", 0, 100, 2))
        defs += Suite('RepeatEnumerated', RepeatEnumerated("enum", ["red", "green", "blue"]))
        defs += Suite('RepeatDate', RepeatDate("ymd", 20100111, 20100115, 2))
        defs += Suite('RepeatDateTime', RepeatDateTime("ymd", "20100111T000000", "20100115T000000", "48:00:00"))
        defs += Suite('RepeatDateList', RepeatDateList("ymd", [20100111, 20100115]))
        defs += Suite('RepeatString', RepeatString("string", ["a", "b", "c"]))
        defs += Suite('RepeatDay', RepeatDay(1))
        defs += Suite('defstatus', Defstatus('active'))
        defs += Suite('today', Task('today', Today("00:30"), Today(0, 59), Today("00:00 11:30 00:01")))
        defs += Suite('time', Task('time', Time("00:30"), Time(0, 59), Time("00:00 11:30 00:01")))
        defs += Suite('day', Task('day', Day("sunday"), Day(Days.monday)))
        defs += Suite('date', Task('date', Date(1, 1, 0), Date(28, 2, 1960)))
        defs += Suite('cron', Task('cron', self.cron))
        defs += Suite('late', Family('late', self.late, Task('late', self.late2)))
        defs += Suite('clock', Clock(1, 1, 2010, False))
        defs += Suite('autocancel', Autocancel(3))

        defs += Suite("zombie", ZombieAttr(ZombieType.ecf, self.child_list, ZombieUserActionType.block, self.zombie_life_time_in_server),
                      ZombieAttr(ZombieType.user, self.child_list, ZombieUserActionType.block, self.zombie_life_time_in_server),
                      ZombieAttr(ZombieType.path, self.child_list, ZombieUserActionType.block, self.zombie_life_time_in_server))

        self.assertEqual(self.defs1, defs, "defs not equal\n" + str(self.defs1) + "\n\n" + str(defs))


if __name__ == "__main__":
    unittest.main()
    print("All Tests pass")
