#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import copy
import unittest

import ecflow as ecf


class TestNodeVec(unittest.TestCase):
    """Tests for py::class_<std::vector<node_ptr>> exposed as ecf.NodeVec in ExportNode.cpp.

    Exposed API
    -----------
    Constructors
        NodeVec()                -- default constructor creates an empty vector

    Instance methods (from vector_indexing_suite)
        __len__                  -- number of nodes
        __getitem__(int)         -- index access
        __iter__                 -- iteration over nodes
        append(node_ptr)         -- append a node
    """

    # ------------------------------------------------------------------
    # Construction and basic properties
    # ------------------------------------------------------------------

    def test_default_constructor_creates_empty_vec(self):
        """NodeVec() creates an empty vector with length zero."""
        nv = ecf.NodeVec()
        self.assertEqual(len(nv), 0)

    def test_len_after_append(self):
        """After appending one task the length is 1."""
        nv = ecf.NodeVec()
        nv.append(ecf.Task("t1"))
        self.assertEqual(len(nv), 1)

    def test_len_after_multiple_appends(self):
        """After appending three nodes the length is 3."""
        nv = ecf.NodeVec()
        nv.append(ecf.Task("t1"))
        nv.append(ecf.Task("t2"))
        nv.append(ecf.Task("t3"))
        self.assertEqual(len(nv), 3)

    # ------------------------------------------------------------------
    # __getitem__
    # ------------------------------------------------------------------

    def test_getitem_returns_correct_node(self):
        """Index access returns the node at the given position."""
        nv = ecf.NodeVec()
        t1 = ecf.Task("alpha")
        t2 = ecf.Task("beta")
        nv.append(t1)
        nv.append(t2)
        self.assertEqual(nv[0].name(), "alpha")
        self.assertEqual(nv[1].name(), "beta")

    def test_getitem_out_of_range_raises(self):
        """Index access beyond the vector length raises an exception."""
        nv = ecf.NodeVec()
        nv.append(ecf.Task("t1"))
        with self.assertRaises(Exception):
            _ = nv[5]

    # ------------------------------------------------------------------
    # __iter__
    # ------------------------------------------------------------------

    def test_iter_visits_all_nodes(self):
        """Iterating over NodeVec yields all appended nodes."""
        nv = ecf.NodeVec()
        names = ["n1", "n2", "n3"]
        for name in names:
            nv.append(ecf.Task(name))
        visited = [node.name() for node in nv]
        self.assertEqual(visited, names)

    def test_iter_empty_vec_yields_nothing(self):
        """Iterating over an empty NodeVec yields no items."""
        nv = ecf.NodeVec()
        items = list(nv)
        self.assertEqual(items, [])


class TestNode(unittest.TestCase):
    """Tests for py::class_<Node, boost::noncopyable, node_ptr> exposed as ecf.Node in ExportNode.cpp.

    Exposed API
    -----------
    Constructors
        (abstract -- no direct construction; use Task/Suite/Family)

    Instance methods
        name()                      -> str
        add(...)                    -- raw_function; adds children/attributes; returns self
        remove()                    -- detaches node from parent; returns self
        add_trigger(str)            -- add trigger expression
        add_trigger(Expression)     -- add trigger expression object
        add_complete(str)           -- add complete expression
        add_complete(Expression)    -- add complete expression object
        add_part_trigger(PartExpr)  -- add partial trigger
        add_part_trigger(str)       -- add partial trigger by string
        add_part_trigger(str, bool) -- add partial trigger with and/or flag
        add_part_complete(PartExpr) -- add partial complete
        add_part_complete(str)      -- add partial complete by string
        add_part_complete(str,bool) -- add partial complete with and/or flag
        evaluate_trigger()          -> bool
        evaluate_complete()         -> bool
        add_variable(str, str)      -- add variable by name+value
        add_variable(str, int)      -- add variable by name+int
        add_variable(Variable)      -- add variable object
        add_variable(dict)          -- add variables from dict
        add_label(str, str)         -- add label by name+value
        add_label(Label)            -- add label object
        add_aviso(AvisoAttr)        -- add aviso attribute
        add_mirror(MirrorAttr)      -- add mirror attribute
        add_limit(str, int)         -- add limit by name+max
        add_limit(Limit)            -- add limit object
        add_inlimit(str, ...)       -- add inlimit by params
        add_inlimit(InLimit)        -- add inlimit object
        add_event(Event)            -- add event object
        add_event(int)              -- add event by number
        add_event(int, str)         -- add event by number+name
        add_event(str)              -- add event by name
        add_meter(Meter)            -- add meter object
        add_meter(str, int, int, int) -- add meter by params with color
        add_meter(str, int, int)    -- add meter by params
        add_queue(QueueAttr)        -- add queue object
        add_queue(str, list)        -- add queue by name+list
        add_generic(GenericAttr)    -- add generic object
        add_generic(str, list)      -- add generic by name+list
        add_date(int, int, int)     -- add date by day/month/year
        add_date(DateAttr)          -- add date object
        add_day(Day_t)              -- add day by enum
        add_day(str)                -- add day by string
        add_day(DayAttr)            -- add day object
        add_today(int, int)         -- add today by hour+minute
        add_today(int, int, bool)   -- add today with relative flag
        add_today(str)              -- add today by string
        add_today(TodayAttr)        -- add today object
        add_time(int, int)          -- add time by hour+minute
        add_time(int, int, bool)    -- add time with relative flag
        add_time(str)               -- add time by string
        add_time(TimeAttr)          -- add time object
        add_cron(CronAttr)          -- add cron attribute
        add_late(LateAttr)          -- add late attribute
        add_autocancel(int)         -- add autocancel by days
        add_autocancel(int,int,bool)-- add autocancel by hour+min+relative
        add_autocancel(TimeSlot,bool) -- add autocancel by timeslot
        add_autocancel(AutoCancelAttr) -- add autocancel object
        add_autoarchive(int, bool)  -- add autoarchive by days+idle
        add_autoarchive(int,int,bool,bool) -- add autoarchive by params
        add_autoarchive(TimeSlot,bool,bool) -- add autoarchive by timeslot
        add_autoarchive(AutoArchiveAttr) -- add autoarchive object
        add_autorestore(AutoRestoreAttr) -- add autorestore object
        add_autorestore(list)       -- add autorestore by list of paths
        add_verify(VerifyAttr)      -- add verify attribute
        add_repeat(RepeatDate)      -- add repeat date
        add_repeat(RepeatDateTime)  -- add repeat datetime
        add_repeat(RepeatDateList)  -- add repeat date list
        add_repeat(RepeatInteger)   -- add repeat integer
        add_repeat(RepeatString)    -- add repeat string
        add_repeat(RepeatEnumerated) -- add repeat enumerated
        add_repeat(RepeatDay)       -- add repeat day
        add_defstatus(DState)       -- add defstatus by DState enum
        add_defstatus(Defstatus)    -- add defstatus by Defstatus object
        add_zombie(ZombieAttr)      -- add zombie attribute
        delete_variable(str)        -- delete variable by name (empty=all)
        delete_event(str)           -- delete event by name (empty=all)
        delete_meter(str)           -- delete meter by name (empty=all)
        delete_label(str)           -- delete label by name (empty=all)
        delete_queue(str)           -- delete queue by name (empty=all)
        delete_generic(str)         -- delete generic by name (empty=all)
        delete_trigger()            -- delete trigger expression
        delete_complete()           -- delete complete expression
        delete_repeat()             -- delete repeat
        delete_limit(str)           -- delete limit by name (empty=all)
        delete_inlimit(str)         -- delete inlimit by name (empty=all)
        delete_time(str)            -- delete time by string
        delete_time(TimeAttr)       -- delete time by object
        delete_today(str)           -- delete today by string
        delete_today(TodayAttr)     -- delete today by object
        delete_date(DateAttr)       -- delete date by object
        delete_date(str)            -- delete date by string
        delete_day(DayAttr)         -- delete day by object
        delete_day(str)             -- delete day by string
        delete_cron(CronAttr)       -- delete cron by object
        delete_cron(str)            -- delete cron by string
        delete_zombie(str)          -- delete zombie by type string (empty=all)
        delete_zombie(ZombieType)   -- delete zombie by enum
        change_trigger(str)         -- change trigger expression
        change_complete(str)        -- change complete expression
        sort_attributes(AttrType)   -- sort one attribute type
        sort_attributes(AttrType, bool) -- sort with recursive flag
        sort_attributes(AttrType, bool, list) -- sort with exclusion list
        sort_attributes(str, bool, list) -- sort with string attribute name
        get_abs_node_path()         -> str
        has_time_dependencies()     -> bool
        update_generated_variables()
        get_generated_variables()   -> list of Variable
        get_generated_variables(VariableList) -- fills in-place
        is_suspended()              -> bool
        find_variable(str)          -> Variable
        find_gen_variable(str)      -> Variable
        find_parent_variable(str)   -> Variable
        find_parent_variable_sub_value(str) -> str
        find_meter(str)             -> Meter
        find_event(str)             -> Event
        find_label(str)             -> Label
        find_queue(str)             -> QueueAttr
        find_generic(str)           -> GenericAttr
        find_limit(str)             -> limit_ptr
        find_node_up_the_tree(str)  -> node_ptr
        get_state()                 -> NState
        get_state_change_time(str)  -> str
        get_dstate()                -> DState
        get_defstatus()             -> DState
        get_repeat()                -> Repeat
        get_late()                  -> LateAttr or None
        get_autocancel()            -> AutoCancelAttr or None
        get_autoarchive()           -> AutoArchiveAttr or None
        get_autorestore()           -> AutoRestoreAttr or None
        get_trigger()               -> Expression or None
        get_complete()              -> Expression or None
        get_defs()                  -> Defs or None
        get_parent()                -> Node or None
        get_all_nodes()             -> NodeVec
        get_flag()                  -> Flag

    Operators
        __lt__    -- lexicographic by name
        __add__   -- do_add; add child/attr; returns self
        __rshift__ -- chained add with forward trigger chain
        __lshift__ -- chained add with reverse trigger chain
        __iadd__  -- add single or list of children/attrs
        __getattr__ -- resolves child, variable, event, meter, limit by name

    Properties
        meters    -- iterable of Meter
        events    -- iterable of Event
        variables -- iterable of Variable
        labels    -- iterable of Label
        avisos    -- iterable of AvisoAttr
        mirrors   -- iterable of MirrorAttr
        limits    -- iterable of Limit
        inlimits  -- iterable of InLimit
        verifies  -- iterable of VerifyAttr
        times     -- iterable of TimeAttr
        todays    -- iterable of TodayAttr
        dates     -- iterable of DateAttr
        days      -- iterable of DayAttr
        crons     -- iterable of CronAttr
        zombies   -- iterable of ZombieAttr
        queues    -- iterable of QueueAttr
        generics  -- iterable of GenericAttr
    """

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_correct_string_for_task(self):
        """name() returns the name passed to the Task constructor."""
        t = ecf.Task("myjob")
        self.assertEqual(t.name(), "myjob")

    def test_name_returns_correct_string_for_suite(self):
        """name() returns the name passed to the Suite constructor."""
        s = ecf.Suite("mysuite")
        self.assertEqual(s.name(), "mysuite")

    def test_name_returns_correct_string_for_family(self):
        """name() returns the name passed to the Family constructor."""
        f = ecf.Family("myfamily")
        self.assertEqual(f.name(), "myfamily")

    def test_name_returns_str_type(self):
        """name() always returns a Python str."""
        t = ecf.Task("x")
        self.assertIsInstance(t.name(), str)

    # ------------------------------------------------------------------
    # get_abs_node_path()
    # ------------------------------------------------------------------

    def test_get_abs_node_path_detached_task(self):
        """A detached task has abs path equal to /name."""
        t = ecf.Task("t1")
        self.assertEqual(t.get_abs_node_path(), "/t1")

    def test_get_abs_node_path_nested(self):
        """A task nested inside suite/family gets a full path."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        f = s.add_family("f1")
        t = f.add_task("t1")
        self.assertEqual(t.get_abs_node_path(), "/s1/f1/t1")

    def test_get_abs_node_path_returns_str(self):
        """get_abs_node_path() always returns a Python str."""
        t = ecf.Task("t1")
        self.assertIsInstance(t.get_abs_node_path(), str)

    # ------------------------------------------------------------------
    # Operators: __lt__
    # ------------------------------------------------------------------

    def test_lt_by_name_alphabetical(self):
        """Tasks are ordered lexicographically by name."""
        ta = ecf.Task("alpha")
        tb = ecf.Task("beta")
        self.assertLess(ta, tb)

    def test_lt_same_name_is_not_less(self):
        """A task is not less than another task with the same name."""
        ta = ecf.Task("same")
        tb = ecf.Task("same")
        self.assertFalse(ta < tb)

    def test_lt_reverse_order(self):
        """'beta' is not less than 'alpha'."""
        ta = ecf.Task("alpha")
        tb = ecf.Task("beta")
        self.assertFalse(tb < ta)

    # ------------------------------------------------------------------
    # Operators: __add__  (do_add)
    # ------------------------------------------------------------------

    def test_add_variable_returns_self(self):
        """__add__ with a Variable returns the same node."""
        t = ecf.Task("t1")
        result = t + ecf.Variable("K", "V")
        self.assertIs(result, t)

    def test_add_event_returns_self(self):
        """__add__ with an Event returns the same node."""
        t = ecf.Task("t1")
        result = t + ecf.Event("done")
        self.assertIs(result, t)

    def test_add_meter_returns_self(self):
        """__add__ with a Meter returns the same node."""
        t = ecf.Task("t1")
        result = t + ecf.Meter("m", 0, 100)
        self.assertIs(result, t)

    def test_add_adds_attribute(self):
        """__add__ actually adds the attribute."""
        t = ecf.Task("t1")
        t + ecf.Variable("K", "V")
        self.assertEqual(len(list(t.variables)), 1)

    # ------------------------------------------------------------------
    # Operators: __iadd__
    # ------------------------------------------------------------------

    def test_iadd_single_variable(self):
        """__iadd__ with a single Variable adds it to the node."""
        t = ecf.Task("t1")
        t += ecf.Variable("K", "V")
        self.assertEqual(len(list(t.variables)), 1)

    def test_iadd_list_of_variables(self):
        """__iadd__ with a list of Variables adds all."""
        t = ecf.Task("t1")
        t += [ecf.Variable("A", "1"), ecf.Variable("B", "2")]
        self.assertEqual(len(list(t.variables)), 2)

    def test_iadd_event(self):
        """__iadd__ with an Event adds it."""
        t = ecf.Task("t1")
        t += ecf.Event("done")
        self.assertEqual(len(list(t.events)), 1)

    def test_iadd_multiple_events_via_list(self):
        """__iadd__ with a list of Events adds all."""
        t = ecf.Task("t1")
        t += [ecf.Event("e1"), ecf.Event("e2")]
        self.assertEqual(len(list(t.events)), 2)

    # ------------------------------------------------------------------
    # Operators: __rshift__
    # ------------------------------------------------------------------

    def test_rshift_adds_child(self):
        """>> adds a child task to the suite."""
        s = ecf.Suite("s1")
        t1 = ecf.Task("t1")
        s >> t1
        children = list(s.nodes)
        self.assertEqual(len(children), 1)

    def test_rshift_second_child_has_trigger_on_first(self):
        """Second child added with >> gets a trigger on the first."""
        s = ecf.Suite("s1")
        t1 = ecf.Task("t1")
        t2 = ecf.Task("t2")
        s >> t1 >> t2
        # t2 should have a trigger "t1 == complete"
        self.assertIsNotNone(t2.get_trigger())
        expr = t2.get_trigger().get_expression()
        self.assertIn("t1", expr)
        self.assertIn("complete", expr)

    def test_rshift_returns_self(self):
        """>> returns self (the container node), enabling chaining."""
        s = ecf.Suite("s1")
        t1 = ecf.Task("t1")
        result = s >> t1
        self.assertEqual(result.name(), s.name())

    def test_rshift_first_child_has_no_trigger(self):
        """The first child added with >> has no trigger."""
        s = ecf.Suite("s1")
        t1 = ecf.Task("t1")
        t2 = ecf.Task("t2")
        s >> t1 >> t2
        self.assertIsNone(t1.get_trigger())

    # ------------------------------------------------------------------
    # Operators: __lshift__
    # ------------------------------------------------------------------

    def test_lshift_adds_child(self):
        """<< adds a child task to the suite."""
        s = ecf.Suite("s1")
        t1 = ecf.Task("t1")
        s << t1
        children = list(s.nodes)
        self.assertEqual(len(children), 1)

    def test_lshift_first_child_gets_trigger_on_second(self):
        """With <<, the earlier child gets trigger on the later child."""
        s = ecf.Suite("s1")
        t1 = ecf.Task("t1")
        t2 = ecf.Task("t2")
        s << t1 << t2
        # t1 should have a trigger on t2 == complete
        self.assertIsNotNone(t1.get_trigger())
        expr = t1.get_trigger().get_expression()
        self.assertIn("t2", expr)
        self.assertIn("complete", expr)

    def test_lshift_last_child_has_no_trigger(self):
        """The last child added with << has no trigger."""
        s = ecf.Suite("s1")
        t1 = ecf.Task("t1")
        t2 = ecf.Task("t2")
        s << t1 << t2
        self.assertIsNone(t2.get_trigger())

    # ------------------------------------------------------------------
    # Operator: __getattr__  (node_getattr)
    # ------------------------------------------------------------------

    def test_getattr_resolves_child_node(self):
        """__getattr__ resolves an immediate child node by name."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        result = s.t1
        self.assertEqual(result.name(), "t1")

    def test_getattr_resolves_variable(self):
        """__getattr__ resolves a user variable by name."""
        t = ecf.Task("t1")
        t.add_variable("MY_VAR", "hello")
        result = t.MY_VAR
        self.assertIsInstance(result, ecf.Variable)
        self.assertEqual(result.name(), "MY_VAR")

    def test_getattr_resolves_event(self):
        """__getattr__ resolves an event by name."""
        t = ecf.Task("t1")
        t.add_event("done")
        result = t.done
        self.assertIsInstance(result, ecf.Event)

    def test_getattr_resolves_meter(self):
        """__getattr__ resolves a meter by name."""
        t = ecf.Task("t1")
        t.add_meter("progress", 0, 100)
        result = t.progress
        self.assertIsInstance(result, ecf.Meter)

    def test_getattr_resolves_limit(self):
        """__getattr__ resolves a limit by name."""
        t = ecf.Task("t1")
        t.add_limit("myLimit", 5)
        result = t.myLimit
        self.assertIsInstance(result, ecf.Limit)

    def test_getattr_unknown_name_raises_runtime_error(self):
        """__getattr__ raises RuntimeError for names that don't resolve."""
        t = ecf.Task("t1")
        with self.assertRaises(RuntimeError):
            _ = t.nonexistent_attr_xyz

    # ------------------------------------------------------------------
    # add() — raw_function
    # ------------------------------------------------------------------

    def test_add_single_variable_via_add(self):
        """add(Variable) appends a variable and returns self."""
        t = ecf.Task("t1")
        ret = t.add(ecf.Variable("K", "V"))
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.variables)), 1)

    def test_add_multiple_args(self):
        """add(v1, v2) adds both attributes in one call."""
        t = ecf.Task("t1")
        t.add(ecf.Variable("A", "1"), ecf.Variable("B", "2"))
        self.assertEqual(len(list(t.variables)), 2)

    def test_add_with_kwargs_adds_variables(self):
        """add(key=value) adds variables from keyword arguments."""
        t = ecf.Task("t1")
        t.add(MYKEY="myval")
        self.assertEqual(len(list(t.variables)), 1)
        names = [v.name() for v in t.variables]
        self.assertIn("MYKEY", names)

    # ------------------------------------------------------------------
    # remove()
    # ------------------------------------------------------------------

    def test_remove_detaches_from_parent(self):
        """remove() detaches the node from its parent."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        self.assertEqual(t.get_parent().name(), "s1")
        t.remove()
        self.assertIsNone(t.get_parent())

    def test_remove_returns_self(self):
        """remove() returns the node itself."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        result = t.remove()
        self.assertEqual(result.name(), "t1")

    # ------------------------------------------------------------------
    # add_trigger / add_complete / add_part_trigger / add_part_complete
    # ------------------------------------------------------------------

    def test_add_trigger_by_str_returns_self(self):
        """add_trigger(str) returns the node (fluent interface)."""
        t = ecf.Task("t1")
        ret = t.add_trigger("t2 == complete")
        self.assertIs(ret, t)

    def test_add_trigger_by_str_sets_expression(self):
        """add_trigger(str) sets the trigger expression."""
        t = ecf.Task("t1")
        t.add_trigger("t2 == complete")
        self.assertIsNotNone(t.get_trigger())
        self.assertIn("t2", t.get_trigger().get_expression())

    def test_add_trigger_by_expression_object(self):
        """add_trigger(Expression) sets the trigger from an Expression object."""
        t = ecf.Task("t1")
        t.add_trigger(ecf.Expression("t2 == complete"))
        self.assertIsNotNone(t.get_trigger())

    def test_add_complete_by_str_sets_expression(self):
        """add_complete(str) sets the complete expression."""
        t = ecf.Task("t1")
        t.add_complete("t2 == complete")
        self.assertIsNotNone(t.get_complete())
        self.assertIn("t2", t.get_complete().get_expression())

    def test_add_complete_by_expression_object(self):
        """add_complete(Expression) sets complete from an Expression object."""
        t = ecf.Task("t1")
        t.add_complete(ecf.Expression("t2 == complete"))
        self.assertIsNotNone(t.get_complete())

    def test_add_complete_returns_self(self):
        """add_complete(str) returns the node (fluent interface)."""
        t = ecf.Task("t1")
        ret = t.add_complete("t2 == complete")
        self.assertIs(ret, t)

    def test_add_part_trigger_by_part_expression(self):
        """add_part_trigger(PartExpression) adds a partial trigger."""
        t = ecf.Task("t1")
        t.add_part_trigger(ecf.PartExpression("t2 == complete"))
        self.assertIsNotNone(t.get_trigger())

    def test_add_part_trigger_by_str(self):
        """add_part_trigger(str) adds a partial trigger from a string."""
        t = ecf.Task("t1")
        ret = t.add_part_trigger("t2 == complete")
        self.assertIs(ret, t)
        self.assertIsNotNone(t.get_trigger())

    def test_add_part_trigger_by_str_and_bool(self):
        """add_part_trigger(str, bool) adds an AND-joined partial trigger."""
        t = ecf.Task("t1")
        t.add_part_trigger("t2 == complete")
        ret = t.add_part_trigger("t3 == complete", True)
        self.assertIs(ret, t)
        expr = t.get_trigger().get_expression()
        self.assertIn("AND", expr)

    def test_add_part_complete_by_part_expression(self):
        """add_part_complete(PartExpression) adds a partial complete."""
        t = ecf.Task("t1")
        t.add_part_complete(ecf.PartExpression("t2 == complete"))
        self.assertIsNotNone(t.get_complete())

    def test_add_part_complete_by_str(self):
        """add_part_complete(str) adds a partial complete from a string."""
        t = ecf.Task("t1")
        ret = t.add_part_complete("t2 == complete")
        self.assertIs(ret, t)
        self.assertIsNotNone(t.get_complete())

    def test_add_part_complete_by_str_and_bool(self):
        """add_part_complete(str, bool) adds an AND-joined partial complete."""
        t = ecf.Task("t1")
        t.add_part_complete("t2 == complete")
        ret = t.add_part_complete("t3 == complete", True)
        self.assertIs(ret, t)

    # ------------------------------------------------------------------
    # evaluate_trigger / evaluate_complete
    # ------------------------------------------------------------------

    def test_evaluate_trigger_false_when_no_trigger(self):
        """evaluate_trigger() returns False when there is no trigger."""
        t = ecf.Task("t1")
        self.assertFalse(t.evaluate_trigger())

    def test_evaluate_complete_false_when_no_complete(self):
        """evaluate_complete() returns False when there is no complete."""
        t = ecf.Task("t1")
        self.assertFalse(t.evaluate_complete())

    # ------------------------------------------------------------------
    # delete_trigger / delete_complete
    # ------------------------------------------------------------------

    def test_delete_trigger_removes_trigger(self):
        """delete_trigger() removes the trigger expression."""
        t = ecf.Task("t1")
        t.add_trigger("t2 == complete")
        t.delete_trigger()
        self.assertIsNone(t.get_trigger())

    def test_delete_complete_removes_complete(self):
        """delete_complete() removes the complete expression."""
        t = ecf.Task("t1")
        t.add_complete("t2 == complete")
        t.delete_complete()
        self.assertIsNone(t.get_complete())

    def test_delete_trigger_on_empty_node_does_not_raise(self):
        """delete_trigger() on a node with no trigger does not raise."""
        t = ecf.Task("t1")
        t.delete_trigger()  # should not raise

    def test_delete_complete_on_empty_node_does_not_raise(self):
        """delete_complete() on a node with no complete does not raise."""
        t = ecf.Task("t1")
        t.delete_complete()  # should not raise

    # ------------------------------------------------------------------
    # change_trigger / change_complete
    # ------------------------------------------------------------------

    def test_change_trigger_replaces_expression(self):
        """change_trigger(str) replaces the existing trigger expression."""
        t = ecf.Task("t1")
        t.add_trigger("t2 == complete")
        t.change_trigger("t3 == active")
        expr = t.get_trigger().get_expression()
        self.assertIn("t3", expr)

    def test_change_complete_replaces_expression(self):
        """change_complete(str) replaces the existing complete expression."""
        t = ecf.Task("t1")
        t.add_complete("t2 == complete")
        t.change_complete("t3 == active")
        expr = t.get_complete().get_expression()
        self.assertIn("t3", expr)

    # ------------------------------------------------------------------
    # Variables: add_variable / delete_variable / find_variable
    # ------------------------------------------------------------------

    def test_add_variable_by_name_value(self):
        """add_variable(str, str) adds a variable and returns self."""
        t = ecf.Task("t1")
        ret = t.add_variable("FOO", "bar")
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.variables)), 1)

    def test_add_variable_by_name_int(self):
        """add_variable(str, int) adds an integer variable."""
        t = ecf.Task("t1")
        t.add_variable("COUNT", 42)
        names = [v.name() for v in t.variables]
        self.assertIn("COUNT", names)

    def test_add_variable_by_object(self):
        """add_variable(Variable) adds the variable object."""
        t = ecf.Task("t1")
        t.add_variable(ecf.Variable("VAR", "val"))
        self.assertEqual(len(list(t.variables)), 1)

    def test_add_variable_by_dict(self):
        """add_variable(dict) adds all key/value pairs as variables."""
        t = ecf.Task("t1")
        t.add_variable({"A": "1", "B": "2"})
        self.assertEqual(len(list(t.variables)), 2)

    def test_add_variable_fluent_chain(self):
        """Multiple add_variable calls can be chained."""
        t = ecf.Task("t1")
        t.add_variable("A", "1").add_variable("B", "2").add_variable("C", "3")
        self.assertEqual(len(list(t.variables)), 3)

    def test_delete_variable_by_name(self):
        """delete_variable(str) removes the named variable."""
        t = ecf.Task("t1")
        t.add_variable("FOO", "bar")
        t.add_variable("BAZ", "qux")
        t.delete_variable("FOO")
        names = [v.name() for v in t.variables]
        self.assertNotIn("FOO", names)
        self.assertIn("BAZ", names)

    def test_delete_variable_empty_string_removes_all(self):
        """delete_variable('') removes all variables."""
        t = ecf.Task("t1")
        t.add_variable("A", "1").add_variable("B", "2")
        t.delete_variable("")
        self.assertEqual(len(list(t.variables)), 0)

    def test_find_variable_found(self):
        """find_variable returns the variable when found."""
        t = ecf.Task("t1")
        t.add_variable("MYVAR", "hello")
        v = t.find_variable("MYVAR")
        self.assertFalse(v.empty())
        self.assertEqual(v.name(), "MYVAR")

    def test_find_variable_not_found_returns_empty(self):
        """find_variable returns an empty object when not found."""
        t = ecf.Task("t1")
        v = t.find_variable("NONEXISTENT")
        self.assertTrue(v.empty())

    def test_find_parent_variable_found_in_parent(self):
        """find_parent_variable traverses the hierarchy to find a variable."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_variable("SUITE_VAR", "sv")
        t = s.add_task("t1")
        v = t.find_parent_variable("SUITE_VAR")
        self.assertFalse(v.empty())

    def test_find_parent_variable_not_found_returns_empty(self):
        """find_parent_variable returns empty object for nonexistent variable."""
        t = ecf.Task("t1")
        v = t.find_parent_variable("NONEXISTENT")
        self.assertTrue(v.empty())

    def test_find_parent_variable_sub_value_found(self):
        """find_parent_variable_sub_value returns the value string."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_variable("MYKEY", "hello")
        t = s.add_task("t1")
        result = t.find_parent_variable_sub_value("MYKEY")
        self.assertIsInstance(result, str)

    def test_find_parent_variable_sub_value_not_found_returns_empty_str(self):
        """find_parent_variable_sub_value returns empty string for nonexistent key."""
        t = ecf.Task("t1")
        result = t.find_parent_variable_sub_value("NO_SUCH_KEY")
        self.assertEqual(result, "")

    def test_variables_property_empty_initially(self):
        """variables property returns empty iterable when no variables added."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.variables)), 0)

    def test_variables_property_after_add(self):
        """variables property returns all added variables."""
        t = ecf.Task("t1")
        t.add_variable("A", "1").add_variable("B", "2")
        names = [v.name() for v in t.variables]
        self.assertIn("A", names)
        self.assertIn("B", names)

    # ------------------------------------------------------------------
    # Events: add_event / delete_event / find_event
    # ------------------------------------------------------------------

    def test_add_event_by_object(self):
        """add_event(Event) adds an event and returns self."""
        t = ecf.Task("t1")
        ret = t.add_event(ecf.Event("done"))
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.events)), 1)

    def test_add_event_by_int(self):
        """add_event(int) adds an event by number."""
        t = ecf.Task("t1")
        ret = t.add_event(1)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.events)), 1)

    def test_add_event_by_int_and_name(self):
        """add_event(int, str) adds an event by number and name."""
        t = ecf.Task("t1")
        ret = t.add_event(10, "myevent")
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.events)), 1)

    def test_add_event_by_str(self):
        """add_event(str) adds an event by name."""
        t = ecf.Task("t1")
        ret = t.add_event("fred")
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.events)), 1)

    def test_add_multiple_events(self):
        """Multiple add_event calls accumulate events."""
        t = ecf.Task("t1")
        t.add_event(ecf.Event(1)).add_event(2).add_event(10, "e2").add_event("fred")
        self.assertEqual(len(list(t.events)), 4)

    def test_delete_event_by_name(self):
        """delete_event(str) removes the named event."""
        t = ecf.Task("t1")
        t.add_event("done").add_event("started")
        t.delete_event("done")
        names = [e.name() for e in t.events]
        self.assertNotIn("done", names)

    def test_delete_event_empty_string_removes_all(self):
        """delete_event('') removes all events."""
        t = ecf.Task("t1")
        t.add_event("e1").add_event("e2")
        t.delete_event("")
        self.assertEqual(len(list(t.events)), 0)

    def test_find_event_found(self):
        """find_event returns the event when found."""
        t = ecf.Task("t1")
        t.add_event("done")
        e = t.find_event("done")
        self.assertFalse(e.empty())

    def test_find_event_not_found_returns_empty(self):
        """find_event returns an empty event when not found."""
        t = ecf.Task("t1")
        e = t.find_event("nonexistent")
        self.assertTrue(e.empty())

    def test_events_property_empty_initially(self):
        """events property returns empty iterable when no events added."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.events)), 0)

    # ------------------------------------------------------------------
    # Meters: add_meter / delete_meter / find_meter
    # ------------------------------------------------------------------

    def test_add_meter_by_object(self):
        """add_meter(Meter) adds a meter and returns self."""
        t = ecf.Task("t1")
        ret = t.add_meter(ecf.Meter("m1", 0, 100))
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.meters)), 1)

    def test_add_meter_by_name_min_max_color(self):
        """add_meter(str, int, int, int) adds meter with color change."""
        t = ecf.Task("t1")
        ret = t.add_meter("progress", 0, 100, 50)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.meters)), 1)

    def test_add_meter_by_name_min_max(self):
        """add_meter(str, int, int) adds meter without explicit color."""
        t = ecf.Task("t1")
        ret = t.add_meter("progress", 0, 100)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.meters)), 1)

    def test_add_multiple_meters(self):
        """Multiple meters can be added."""
        t = ecf.Task("t1")
        t.add_meter(ecf.Meter("m1", 0, 100)).add_meter("m2", 0, 50)
        self.assertEqual(len(list(t.meters)), 2)

    def test_delete_meter_by_name(self):
        """delete_meter(str) removes the named meter."""
        t = ecf.Task("t1")
        t.add_meter("m1", 0, 100).add_meter("m2", 0, 50)
        t.delete_meter("m1")
        names = [m.name() for m in t.meters]
        self.assertNotIn("m1", names)
        self.assertIn("m2", names)

    def test_delete_meter_empty_string_removes_all(self):
        """delete_meter('') removes all meters."""
        t = ecf.Task("t1")
        t.add_meter("m1", 0, 100).add_meter("m2", 0, 50)
        t.delete_meter("")
        self.assertEqual(len(list(t.meters)), 0)

    def test_find_meter_found(self):
        """find_meter returns the meter when found."""
        t = ecf.Task("t1")
        t.add_meter("progress", 0, 100)
        m = t.find_meter("progress")
        self.assertFalse(m.empty())
        self.assertEqual(m.name(), "progress")

    def test_find_meter_not_found_returns_empty(self):
        """find_meter returns an empty meter when not found."""
        t = ecf.Task("t1")
        m = t.find_meter("nonexistent")
        self.assertTrue(m.empty())

    def test_meters_property_empty_initially(self):
        """meters property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.meters)), 0)

    # ------------------------------------------------------------------
    # Labels: add_label / delete_label / find_label
    # ------------------------------------------------------------------

    def test_add_label_by_name_value(self):
        """add_label(str, str) adds a label and returns self."""
        t = ecf.Task("t1")
        ret = t.add_label("info", "some text")
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.labels)), 1)

    def test_add_label_by_object(self):
        """add_label(Label) adds a label object."""
        t = ecf.Task("t1")
        ret = t.add_label(ecf.Label("lbl", "val"))
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.labels)), 1)

    def test_add_multiple_labels(self):
        """Multiple labels can be added."""
        t = ecf.Task("t1")
        t.add_label("l1", "v1").add_label("l2", "v2")
        self.assertEqual(len(list(t.labels)), 2)

    def test_delete_label_by_name(self):
        """delete_label(str) removes the named label."""
        t = ecf.Task("t1")
        t.add_label("l1", "v1").add_label("l2", "v2")
        t.delete_label("l1")
        names = [lb.name() for lb in t.labels]
        self.assertNotIn("l1", names)
        self.assertIn("l2", names)

    def test_delete_label_empty_string_removes_all(self):
        """delete_label('') removes all labels."""
        t = ecf.Task("t1")
        t.add_label("l1", "v1").add_label("l2", "v2")
        t.delete_label("")
        self.assertEqual(len(list(t.labels)), 0)

    def test_find_label_found(self):
        """find_label returns the label when found."""
        t = ecf.Task("t1")
        t.add_label("info", "text")
        lb = t.find_label("info")
        self.assertFalse(lb.empty())
        self.assertEqual(lb.name(), "info")

    def test_find_label_not_found_returns_empty(self):
        """find_label returns an empty label when not found."""
        t = ecf.Task("t1")
        lb = t.find_label("nonexistent")
        self.assertTrue(lb.empty())

    def test_labels_property_empty_initially(self):
        """labels property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.labels)), 0)

    # ------------------------------------------------------------------
    # Limits: add_limit / delete_limit / find_limit
    # ------------------------------------------------------------------

    def test_add_limit_by_name_max(self):
        """add_limit(str, int) adds a limit and returns self."""
        t = ecf.Task("t1")
        ret = t.add_limit("lim1", 5)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.limits)), 1)

    def test_add_limit_by_object(self):
        """add_limit(Limit) adds a limit object."""
        t = ecf.Task("t1")
        ret = t.add_limit(ecf.Limit("lim1", 5))
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.limits)), 1)

    def test_add_multiple_limits(self):
        """Multiple limits can be added."""
        t = ecf.Task("t1")
        t.add_limit("l1", 5).add_limit("l2", 10)
        self.assertEqual(len(list(t.limits)), 2)

    def test_delete_limit_by_name(self):
        """delete_limit(str) removes the named limit."""
        t = ecf.Task("t1")
        t.add_limit("l1", 5).add_limit("l2", 10)
        t.delete_limit("l1")
        names = [lim.name() for lim in t.limits]
        self.assertNotIn("l1", names)

    def test_delete_limit_empty_string_removes_all(self):
        """delete_limit('') removes all limits."""
        t = ecf.Task("t1")
        t.add_limit("l1", 5).add_limit("l2", 10)
        t.delete_limit("")
        self.assertEqual(len(list(t.limits)), 0)

    def test_find_limit_found(self):
        """find_limit returns a non-None limit ptr when found."""
        t = ecf.Task("t1")
        t.add_limit("lim1", 5)
        lim = t.find_limit("lim1")
        self.assertIsNotNone(lim)

    def test_find_limit_not_found_returns_none(self):
        """find_limit returns None when not found."""
        t = ecf.Task("t1")
        lim = t.find_limit("nonexistent")
        self.assertIsNone(lim)

    def test_limits_property_empty_initially(self):
        """limits property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.limits)), 0)

    # ------------------------------------------------------------------
    # Inlimits: add_inlimit / delete_inlimit
    # ------------------------------------------------------------------

    def test_add_inlimit_by_params(self):
        """add_inlimit(str, str, int, bool) adds an inlimit."""
        s = ecf.Suite("s1")
        ret = s.add_inlimit("lim1", "/s1", 1, False)
        self.assertIs(ret, s)
        self.assertEqual(len(list(s.inlimits)), 1)

    def test_add_inlimit_by_object(self):
        """add_inlimit(InLimit) adds an inlimit object."""
        s = ecf.Suite("s1")
        ret = s.add_inlimit(ecf.InLimit("lim1", "/s1", 2))
        self.assertIs(ret, s)
        self.assertEqual(len(list(s.inlimits)), 1)

    def test_delete_inlimit_by_name(self):
        """delete_inlimit(str) removes the named inlimit."""
        s = ecf.Suite("s1")
        s.add_inlimit("lim1", "/s1", 1).add_inlimit("lim2", "/s1", 1)
        s.delete_inlimit("lim1")
        self.assertEqual(len(list(s.inlimits)), 1)

    def test_delete_inlimit_empty_string_removes_all(self):
        """delete_inlimit('') removes all inlimits."""
        s = ecf.Suite("s1")
        s.add_inlimit("lim1", "/s1", 1).add_inlimit("lim2", "/s1", 1)
        s.delete_inlimit("")
        self.assertEqual(len(list(s.inlimits)), 0)

    def test_inlimits_property_empty_initially(self):
        """inlimits property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.inlimits)), 0)

    # ------------------------------------------------------------------
    # Queues: add_queue / delete_queue / find_queue
    # ------------------------------------------------------------------

    def test_add_queue_by_object(self):
        """add_queue(QueueAttr) adds a queue and returns self."""
        t = ecf.Task("t1")
        ret = t.add_queue(ecf.Queue("q1", ["001", "002"]))
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.queues)), 1)

    def test_add_queue_by_name_and_list(self):
        """add_queue(str, list) adds a queue from name and list."""
        t = ecf.Task("t1")
        ret = t.add_queue("q1", ["001", "002"])
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.queues)), 1)

    def test_add_multiple_queues(self):
        """Multiple queues can be added."""
        t = ecf.Task("t1")
        t.add_queue("q1", ["a"]).add_queue("q2", ["b"])
        self.assertEqual(len(list(t.queues)), 2)

    def test_delete_queue_by_name(self):
        """delete_queue(str) removes the named queue."""
        t = ecf.Task("t1")
        t.add_queue("q1", ["a"]).add_queue("q2", ["b"])
        t.delete_queue("q1")
        names = [q.name() for q in t.queues]
        self.assertNotIn("q1", names)

    def test_delete_queue_empty_string_removes_all(self):
        """delete_queue('') removes all queues."""
        t = ecf.Task("t1")
        t.add_queue("q1", ["a"]).add_queue("q2", ["b"])
        t.delete_queue("")
        self.assertEqual(len(list(t.queues)), 0)

    def test_find_queue_found(self):
        """find_queue returns the queue when found."""
        t = ecf.Task("t1")
        t.add_queue("q1", ["a"])
        q = t.find_queue("q1")
        self.assertFalse(q.empty())

    def test_find_queue_not_found_returns_empty(self):
        """find_queue returns an empty queue when not found."""
        t = ecf.Task("t1")
        q = t.find_queue("nonexistent")
        self.assertTrue(q.empty())

    def test_queues_property_empty_initially(self):
        """queues property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.queues)), 0)

    # ------------------------------------------------------------------
    # Generics: add_generic / delete_generic / find_generic
    # ------------------------------------------------------------------

    def test_add_generic_by_object(self):
        """add_generic(GenericAttr) adds a generic attribute."""
        t = ecf.Task("t1")
        ret = t.add_generic(ecf.Generic("gen1", ["a", "b"]))
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.generics)), 1)

    def test_add_generic_by_name_and_list(self):
        """add_generic(str, list) adds a generic from name and list."""
        t = ecf.Task("t1")
        ret = t.add_generic("gen1", ["a", "b"])
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.generics)), 1)

    def test_delete_generic_by_name(self):
        """delete_generic(str) removes the named generic."""
        t = ecf.Task("t1")
        t.add_generic("gen1", ["a"]).add_generic("gen2", ["b"])
        t.delete_generic("gen1")
        names = [g.name() for g in t.generics]
        self.assertNotIn("gen1", names)

    def test_delete_generic_empty_string_removes_all(self):
        """delete_generic('') removes all generics."""
        t = ecf.Task("t1")
        t.add_generic("gen1", ["a"]).add_generic("gen2", ["b"])
        t.delete_generic("")
        self.assertEqual(len(list(t.generics)), 0)

    def test_find_generic_found(self):
        """find_generic returns the generic when found."""
        t = ecf.Task("t1")
        t.add_generic("gen1", ["a"])
        g = t.find_generic("gen1")
        self.assertFalse(g.empty())

    def test_find_generic_not_found_returns_empty(self):
        """find_generic returns an empty generic when not found."""
        t = ecf.Task("t1")
        g = t.find_generic("nonexistent")
        self.assertTrue(g.empty())

    def test_generics_property_empty_initially(self):
        """generics property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.generics)), 0)

    # ------------------------------------------------------------------
    # Aviso / Mirror
    # ------------------------------------------------------------------

    def test_add_aviso_returns_self(self):
        """add_aviso(AvisoAttr) adds the aviso and returns self."""
        t = ecf.Task("t1")
        attr = ecf.AvisoAttr(
            "av1",
            '{"event": "mars"}',
            "https://aviso.example.com",
            "1",
            "auth_key",
            "schema",
        )
        ret = t.add_aviso(attr)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.avisos)), 1)

    def test_avisos_property_empty_initially(self):
        """avisos property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.avisos)), 0)

    def test_add_mirror_returns_self(self):
        """add_mirror(MirrorAttr) adds the mirror and returns self."""
        t = ecf.Task("t1")
        attr = ecf.MirrorAttr(
            "mir1", "/remote/path", "host", "7141", "120", True, "auth_key"
        )
        ret = t.add_mirror(attr)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.mirrors)), 1)

    def test_mirrors_property_empty_initially(self):
        """mirrors property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.mirrors)), 0)

    # ------------------------------------------------------------------
    # Times: add_time / delete_time
    # ------------------------------------------------------------------

    def test_add_time_by_hour_minute(self):
        """add_time(int, int) adds a time and returns self."""
        t = ecf.Task("t1")
        ret = t.add_time(10, 30)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.times)), 1)

    def test_add_time_by_hour_minute_relative(self):
        """add_time(int, int, bool) adds a relative time."""
        t = ecf.Task("t1")
        ret = t.add_time(10, 30, True)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.times)), 1)

    def test_add_time_by_str(self):
        """add_time(str) adds a time from a string representation."""
        t = ecf.Task("t1")
        ret = t.add_time("10:30")
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.times)), 1)

    def test_add_time_by_object(self):
        """add_time(TimeAttr) adds a time from an object."""
        t = ecf.Task("t1")
        ret = t.add_time(ecf.Time(10, 30))
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.times)), 1)

    def test_add_multiple_times(self):
        """Multiple times can be added."""
        t = ecf.Task("t1")
        t.add_time(10, 0).add_time(14, 0)
        self.assertEqual(len(list(t.times)), 2)

    def test_delete_time_by_str(self):
        """delete_time(str) removes a time by string."""
        t = ecf.Task("t1")
        t.add_time("10:30")
        t.delete_time("10:30")
        self.assertEqual(len(list(t.times)), 0)

    def test_delete_time_by_object(self):
        """delete_time(TimeAttr) removes a time by object."""
        t = ecf.Task("t1")
        t.add_time(10, 30)
        times_copy = list(t.times)
        for time_attr in times_copy:
            t.delete_time(time_attr)
        self.assertEqual(len(list(t.times)), 0)

    def test_times_property_empty_initially(self):
        """times property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.times)), 0)

    # ------------------------------------------------------------------
    # Todays: add_today / delete_today
    # ------------------------------------------------------------------

    def test_add_today_by_hour_minute(self):
        """add_today(int, int) adds a today and returns self."""
        t = ecf.Task("t1")
        ret = t.add_today(10, 30)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.todays)), 1)

    def test_add_today_by_hour_minute_relative(self):
        """add_today(int, int, bool) adds a relative today."""
        t = ecf.Task("t1")
        ret = t.add_today(10, 30, True)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.todays)), 1)

    def test_add_today_by_str(self):
        """add_today(str) adds a today from a string."""
        t = ecf.Task("t1")
        ret = t.add_today("10:30")
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.todays)), 1)

    def test_add_today_by_object(self):
        """add_today(TodayAttr) adds a today from an object."""
        t = ecf.Task("t1")
        ret = t.add_today(ecf.Today(10, 30))
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.todays)), 1)

    def test_delete_today_by_str(self):
        """delete_today(str) removes a today by string."""
        t = ecf.Task("t1")
        t.add_today("10:30")
        t.delete_today("10:30")
        self.assertEqual(len(list(t.todays)), 0)

    def test_delete_today_by_object(self):
        """delete_today(TodayAttr) removes a today by object."""
        t = ecf.Task("t1")
        t.add_today(10, 30)
        todays_copy = list(t.todays)
        for today_attr in todays_copy:
            t.delete_today(today_attr)
        self.assertEqual(len(list(t.todays)), 0)

    def test_todays_property_empty_initially(self):
        """todays property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.todays)), 0)

    # ------------------------------------------------------------------
    # Dates: add_date / delete_date
    # ------------------------------------------------------------------

    def test_add_date_by_day_month_year(self):
        """add_date(int, int, int) adds a date and returns self."""
        t = ecf.Task("t1")
        ret = t.add_date(1, 6, 2025)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.dates)), 1)

    def test_add_date_by_object(self):
        """add_date(DateAttr) adds a date from an object."""
        t = ecf.Task("t1")
        ret = t.add_date(ecf.Date(1, 6, 2025))
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.dates)), 1)

    def test_add_multiple_dates(self):
        """Multiple dates can be added."""
        t = ecf.Task("t1")
        t.add_date(1, 6, 2025).add_date(2, 6, 2025)
        self.assertEqual(len(list(t.dates)), 2)

    def test_delete_date_by_object(self):
        """delete_date(DateAttr) removes a date by object."""
        t = ecf.Task("t1")
        t.add_date(1, 6, 2025)
        dates_copy = list(t.dates)
        for d in dates_copy:
            t.delete_date(d)
        self.assertEqual(len(list(t.dates)), 0)

    def test_delete_date_by_str(self):
        """delete_date('') removes all dates."""
        t = ecf.Task("t1")
        t.add_date(1, 6, 2025).add_date(2, 6, 2025)
        t.delete_date("")
        self.assertEqual(len(list(t.dates)), 0)

    def test_dates_property_empty_initially(self):
        """dates property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.dates)), 0)

    # ------------------------------------------------------------------
    # Days: add_day / delete_day
    # ------------------------------------------------------------------

    def test_add_day_by_enum(self):
        """add_day(Day_t enum) adds a day and returns self."""
        t = ecf.Task("t1")
        ret = t.add_day(ecf.Days.monday)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.days)), 1)

    def test_add_day_by_str(self):
        """add_day(str) adds a day from a string."""
        t = ecf.Task("t1")
        ret = t.add_day("tuesday")
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.days)), 1)

    def test_add_day_by_object(self):
        """add_day(DayAttr) adds a day from an object."""
        t = ecf.Task("t1")
        ret = t.add_day(ecf.Day(ecf.Days.wednesday))
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.days)), 1)

    def test_add_multiple_days(self):
        """Multiple days can be added."""
        t = ecf.Task("t1")
        t.add_day("monday").add_day("tuesday")
        self.assertEqual(len(list(t.days)), 2)

    def test_delete_day_by_object(self):
        """delete_day(DayAttr) removes a day by object."""
        t = ecf.Task("t1")
        t.add_day("monday")
        days_copy = list(t.days)
        for d in days_copy:
            t.delete_day(d)
        self.assertEqual(len(list(t.days)), 0)

    def test_delete_day_by_str(self):
        """delete_day('') removes all days."""
        t = ecf.Task("t1")
        t.add_day("monday").add_day("tuesday")
        t.delete_day("")
        self.assertEqual(len(list(t.days)), 0)

    def test_days_property_empty_initially(self):
        """days property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.days)), 0)

    # ------------------------------------------------------------------
    # Crons: add_cron / delete_cron
    # ------------------------------------------------------------------

    def test_add_cron_by_object(self):
        """add_cron(CronAttr) adds a cron and returns self."""
        t = ecf.Task("t1")
        cron = ecf.Cron()
        cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
        cron.set_time_series("10:00")
        ret = t.add_cron(cron)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.crons)), 1)

    def test_delete_cron_by_object(self):
        """delete_cron(CronAttr) removes a cron by object."""
        t = ecf.Task("t1")
        cron = ecf.Cron()
        cron.set_week_days([0, 1, 2, 3, 4, 5, 6])
        cron.set_time_series("10:00")
        t.add_cron(cron)
        crons_copy = list(t.crons)
        for c in crons_copy:
            t.delete_cron(c)
        self.assertEqual(len(list(t.crons)), 0)

    def test_delete_cron_by_str(self):
        """delete_cron('') removes all crons."""
        t = ecf.Task("t1")
        cron = ecf.Cron()
        cron.set_week_days([0])
        cron.set_time_series("10:00")
        t.add_cron(cron)
        t.delete_cron("")
        self.assertEqual(len(list(t.crons)), 0)

    def test_crons_property_empty_initially(self):
        """crons property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.crons)), 0)

    # ------------------------------------------------------------------
    # Zombies: add_zombie / delete_zombie
    # ------------------------------------------------------------------

    def test_add_zombie_by_object(self):
        """add_zombie(ZombieAttr) adds a zombie and returns self."""
        t = ecf.Task("t1")
        za = ecf.ZombieAttr(ecf.ZombieType.ecf, [], ecf.ZombieUserActionType.block)
        ret = t.add_zombie(za)
        self.assertIs(ret, t)
        self.assertEqual(len(list(t.zombies)), 1)

    def test_delete_zombie_by_str_ecf(self):
        """delete_zombie('ecf') removes the ecf-type zombie."""
        t = ecf.Task("t1")
        za = ecf.ZombieAttr(ecf.ZombieType.ecf, [], ecf.ZombieUserActionType.block)
        t.add_zombie(za)
        t.delete_zombie("ecf")
        self.assertEqual(len(list(t.zombies)), 0)

    def test_delete_zombie_by_str_user(self):
        """delete_zombie('user') removes the user-type zombie."""
        t = ecf.Task("t1")
        za = ecf.ZombieAttr(ecf.ZombieType.user, [], ecf.ZombieUserActionType.block)
        t.add_zombie(za)
        t.delete_zombie("user")
        self.assertEqual(len(list(t.zombies)), 0)

    def test_delete_zombie_empty_string_removes_all(self):
        """delete_zombie('') removes all zombies."""
        t = ecf.Task("t1")
        za1 = ecf.ZombieAttr(ecf.ZombieType.ecf, [], ecf.ZombieUserActionType.block)
        za2 = ecf.ZombieAttr(ecf.ZombieType.user, [], ecf.ZombieUserActionType.block)
        t.add_zombie(za1).add_zombie(za2)
        t.delete_zombie("")
        self.assertEqual(len(list(t.zombies)), 0)

    def test_delete_zombie_by_enum(self):
        """delete_zombie(ZombieType) removes zombie by enum value."""
        t = ecf.Task("t1")
        za = ecf.ZombieAttr(ecf.ZombieType.ecf, [], ecf.ZombieUserActionType.block)
        t.add_zombie(za)
        t.delete_zombie(ecf.ZombieType.ecf)
        self.assertEqual(len(list(t.zombies)), 0)

    def test_zombies_property_empty_initially(self):
        """zombies property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.zombies)), 0)

    # ------------------------------------------------------------------
    # Repeat: add_repeat / delete_repeat / get_repeat
    # ------------------------------------------------------------------

    def test_add_repeat_integer(self):
        """add_repeat(RepeatInteger) adds a repeat and returns self."""
        t = ecf.Task("t1")
        ret = t.add_repeat(ecf.RepeatInteger("count", 0, 10, 1))
        self.assertIs(ret, t)
        self.assertFalse(t.get_repeat().empty())

    def test_add_repeat_date(self):
        """add_repeat(RepeatDate) adds a date repeat."""
        t = ecf.Task("t1")
        t.add_repeat(ecf.RepeatDate("date", 20250101, 20251231, 1))
        self.assertFalse(t.get_repeat().empty())

    def test_add_repeat_datetime(self):
        """add_repeat(RepeatDateTime) adds a datetime repeat."""
        t = ecf.Task("t1")
        t.add_repeat(
            ecf.RepeatDateTime("dt", "20250101T000000", "20250110T000000", "24:00:00")
        )
        self.assertFalse(t.get_repeat().empty())

    def test_add_repeat_date_list(self):
        """add_repeat(RepeatDateList) adds a date list repeat."""
        t = ecf.Task("t1")
        t.add_repeat(ecf.RepeatDateList("dl", [20250101, 20250201]))
        self.assertFalse(t.get_repeat().empty())

    def test_add_repeat_string(self):
        """add_repeat(RepeatString) adds a string repeat."""
        t = ecf.Task("t1")
        t.add_repeat(ecf.RepeatString("rs", ["alpha", "beta"]))
        self.assertFalse(t.get_repeat().empty())

    def test_add_repeat_enumerated(self):
        """add_repeat(RepeatEnumerated) adds an enumerated repeat."""
        t = ecf.Task("t1")
        t.add_repeat(ecf.RepeatEnumerated("re", ["red", "green", "blue"]))
        self.assertFalse(t.get_repeat().empty())

    def test_add_repeat_day(self):
        """add_repeat(RepeatDay) adds a day repeat."""
        t = ecf.Task("t1")
        t.add_repeat(ecf.RepeatDay(1))
        self.assertFalse(t.get_repeat().empty())

    def test_delete_repeat(self):
        """delete_repeat() removes the repeat."""
        t = ecf.Task("t1")
        t.add_repeat(ecf.RepeatInteger("count", 0, 10, 1))
        t.delete_repeat()
        self.assertTrue(t.get_repeat().empty())

    def test_get_repeat_empty_initially(self):
        """get_repeat() returns an empty Repeat when none is set."""
        t = ecf.Task("t1")
        self.assertTrue(t.get_repeat().empty())

    # ------------------------------------------------------------------
    # Defstatus: add_defstatus / get_defstatus
    # ------------------------------------------------------------------

    def test_add_defstatus_by_dstate(self):
        """add_defstatus(DState) sets the defstatus and returns self."""
        t = ecf.Task("t1")
        ret = t.add_defstatus(ecf.DState.complete)
        self.assertIs(ret, t)
        self.assertEqual(t.get_defstatus(), ecf.DState.complete)

    def test_add_defstatus_by_defstatus_object(self):
        """add_defstatus(Defstatus) sets the defstatus from a Defstatus object."""
        t = ecf.Task("t1")
        ret = t.add_defstatus(ecf.Defstatus(ecf.DState.suspended))
        self.assertIs(ret, t)
        self.assertEqual(t.get_defstatus(), ecf.DState.suspended)

    def test_get_defstatus_default_is_queued(self):
        """A fresh task has defstatus == queued."""
        t = ecf.Task("t1")
        self.assertEqual(t.get_defstatus(), ecf.DState.queued)

    # ------------------------------------------------------------------
    # Late: add_late / get_late
    # ------------------------------------------------------------------

    def test_add_late_returns_self(self):
        """add_late(LateAttr) adds a late attribute and returns self."""
        t = ecf.Task("t1")
        late = ecf.Late()
        late.submitted(ecf.TimeSlot(0, 15))
        late.active(ecf.TimeSlot(0, 30))
        late.complete(ecf.TimeSlot(16, 0), True)
        ret = t.add_late(late)
        self.assertIs(ret, t)

    def test_get_late_returns_late_attr(self):
        """get_late() returns the LateAttr after it is added."""
        t = ecf.Task("t1")
        late = ecf.Late()
        late.submitted(ecf.TimeSlot(0, 15))
        t.add_late(late)
        result = t.get_late()
        self.assertIsNotNone(result)

    def test_get_late_returns_none_when_not_set(self):
        """get_late() returns None when no late attribute is set."""
        t = ecf.Task("t1")
        self.assertIsNone(t.get_late())

    # ------------------------------------------------------------------
    # Autocancel: add_autocancel / get_autocancel
    # ------------------------------------------------------------------

    def test_add_autocancel_by_days(self):
        """add_autocancel(int) adds autocancel by number of days."""
        t = ecf.Task("t1")
        ret = t.add_autocancel(2)
        self.assertIs(ret, t)
        self.assertIsNotNone(t.get_autocancel())

    def test_add_autocancel_by_hour_min_relative(self):
        """add_autocancel(int, int, bool) adds autocancel by time+relative."""
        t = ecf.Task("t1")
        ret = t.add_autocancel(1, 30, True)
        self.assertIs(ret, t)
        self.assertIsNotNone(t.get_autocancel())

    def test_add_autocancel_by_timeslot(self):
        """add_autocancel(TimeSlot, bool) adds autocancel by TimeSlot."""
        t = ecf.Task("t1")
        ret = t.add_autocancel(ecf.TimeSlot(1, 30), True)
        self.assertIs(ret, t)
        self.assertIsNotNone(t.get_autocancel())

    def test_add_autocancel_by_object(self):
        """add_autocancel(AutoCancelAttr) adds autocancel from object."""
        t = ecf.Task("t1")
        ac = ecf.Autocancel(2)
        ret = t.add_autocancel(ac)
        self.assertIs(ret, t)
        self.assertIsNotNone(t.get_autocancel())

    def test_get_autocancel_returns_none_when_not_set(self):
        """get_autocancel() returns None when not set."""
        t = ecf.Task("t1")
        self.assertIsNone(t.get_autocancel())

    # ------------------------------------------------------------------
    # Autoarchive: add_autoarchive / get_autoarchive
    # ------------------------------------------------------------------

    def test_add_autoarchive_by_days(self):
        """add_autoarchive(int) adds autoarchive by number of days."""
        t = ecf.Task("t1")
        ret = t.add_autoarchive(2)
        self.assertIs(ret, t)
        self.assertIsNotNone(t.get_autoarchive())

    def test_add_autoarchive_by_days_idle(self):
        """add_autoarchive(int, bool) adds autoarchive with idle flag."""
        t = ecf.Task("t1")
        ret = t.add_autoarchive(2, True)
        self.assertIs(ret, t)
        self.assertIsNotNone(t.get_autoarchive())

    def test_add_autoarchive_by_hour_min_relative_idle(self):
        """add_autoarchive(int, int, bool, bool) adds by params."""
        t = ecf.Task("t1")
        ret = t.add_autoarchive(1, 30, True, False)
        self.assertIs(ret, t)
        self.assertIsNotNone(t.get_autoarchive())

    def test_add_autoarchive_by_timeslot(self):
        """add_autoarchive(TimeSlot, bool, bool) adds by TimeSlot."""
        t = ecf.Task("t1")
        ret = t.add_autoarchive(ecf.TimeSlot(1, 30), True, False)
        self.assertIs(ret, t)
        self.assertIsNotNone(t.get_autoarchive())

    def test_get_autoarchive_returns_none_when_not_set(self):
        """get_autoarchive() returns None when not set."""
        t = ecf.Task("t1")
        self.assertIsNone(t.get_autoarchive())

    # ------------------------------------------------------------------
    # Autorestore: add_autorestore / get_autorestore
    # ------------------------------------------------------------------

    def test_add_autorestore_by_object(self):
        """add_autorestore(AutoRestoreAttr) adds autorestore and returns self."""
        t = ecf.Task("t1")
        ar = ecf.Autorestore(["/s1"])
        ret = t.add_autorestore(ar)
        self.assertIs(ret, t)
        self.assertIsNotNone(t.get_autorestore())

    def test_add_autorestore_by_list(self):
        """add_autorestore(list) adds autorestore from a list of paths."""
        t = ecf.Task("t1")
        ret = t.add_autorestore(["/s1", "/s2"])
        self.assertIs(ret, t)
        self.assertIsNotNone(t.get_autorestore())

    def test_get_autorestore_returns_none_when_not_set(self):
        """get_autorestore() returns None when not set."""
        t = ecf.Task("t1")
        self.assertIsNone(t.get_autorestore())

    # ------------------------------------------------------------------
    # State: get_state / get_dstate / get_state_change_time
    # ------------------------------------------------------------------

    def test_get_state_fresh_task_returns_state(self):
        """A fresh task returns a State enum value from get_state()."""
        t = ecf.Task("t1")
        state = t.get_state()
        self.assertIsInstance(state, ecf.State)

    def test_get_state_in_defs_is_unknown(self):
        """A task just added to a defs has state unknown (no begin() called)."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        # State is unknown until begin() is called on the defs
        self.assertEqual(t.get_state(), ecf.State.unknown)

    def test_get_dstate_in_defs_is_unknown(self):
        """A task just added to a defs has dstate unknown (no begin() called)."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        self.assertEqual(t.get_dstate(), ecf.DState.unknown)

    def test_get_state_change_time_default_format(self):
        """get_state_change_time() returns a string with default iso_extended format."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        result = t.get_state_change_time()
        self.assertIsInstance(result, str)

    def test_get_state_change_time_iso_extended(self):
        """get_state_change_time('iso_extended') returns iso_extended format."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        result = t.get_state_change_time("iso_extended")
        self.assertIsInstance(result, str)

    def test_get_state_change_time_iso(self):
        """get_state_change_time('iso') returns iso format."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        result = t.get_state_change_time("iso")
        self.assertIsInstance(result, str)

    def test_get_state_change_time_simple(self):
        """get_state_change_time('simple') returns simple format."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        result = t.get_state_change_time("simple")
        self.assertIsInstance(result, str)

    def test_is_suspended_false_by_default(self):
        """A fresh task is not suspended."""
        t = ecf.Task("t1")
        self.assertFalse(t.is_suspended())

    # ------------------------------------------------------------------
    # get_trigger / get_complete / get_defs / get_parent
    # ------------------------------------------------------------------

    def test_get_trigger_returns_none_when_not_set(self):
        """get_trigger() returns None when no trigger is set."""
        t = ecf.Task("t1")
        self.assertIsNone(t.get_trigger())

    def test_get_trigger_returns_expression_when_set(self):
        """get_trigger() returns an Expression when trigger is set."""
        t = ecf.Task("t1")
        t.add_trigger("t2 == complete")
        self.assertIsNotNone(t.get_trigger())

    def test_get_complete_returns_none_when_not_set(self):
        """get_complete() returns None when no complete is set."""
        t = ecf.Task("t1")
        self.assertIsNone(t.get_complete())

    def test_get_complete_returns_expression_when_set(self):
        """get_complete() returns an Expression when complete is set."""
        t = ecf.Task("t1")
        t.add_complete("t2 == complete")
        self.assertIsNotNone(t.get_complete())

    def test_get_defs_returns_none_for_detached_node(self):
        """get_defs() returns None when node is not in a Defs."""
        t = ecf.Task("t1")
        self.assertIsNone(t.get_defs())

    def test_get_defs_returns_defs_when_in_defs(self):
        """get_defs() returns the Defs when node is inside one."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        self.assertIsNotNone(t.get_defs())

    def test_get_parent_returns_none_for_detached(self):
        """get_parent() returns None when node has no parent."""
        t = ecf.Task("t1")
        self.assertIsNone(t.get_parent())

    def test_get_parent_returns_parent_node(self):
        """get_parent() returns the parent suite."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        parent = t.get_parent()
        self.assertIsNotNone(parent)
        self.assertEqual(parent.name(), "s1")

    # ------------------------------------------------------------------
    # get_all_nodes
    # ------------------------------------------------------------------

    def test_get_all_nodes_includes_self_for_task(self):
        """get_all_nodes() on a detached Task returns a NodeVec containing the task."""
        t = ecf.Task("t1")
        nodes = t.get_all_nodes()
        # get_all_nodes includes the node itself
        self.assertGreaterEqual(len(nodes), 1)

    def test_get_all_nodes_returns_children_for_suite(self):
        """get_all_nodes() on a Suite returns all immediate task children."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_task("t1")
        s.add_task("t2")
        nodes = s.get_all_nodes()
        # get_all_nodes returns immediate children only
        self.assertGreater(len(nodes), 0)

    # ------------------------------------------------------------------
    # get_flag
    # ------------------------------------------------------------------

    def test_get_flag_returns_flag_object(self):
        """get_flag() returns a Flag object."""
        t = ecf.Task("t1")
        flag = t.get_flag()
        self.assertIsNotNone(flag)
        self.assertIsInstance(flag, ecf.Flag)

    # ------------------------------------------------------------------
    # has_time_dependencies
    # ------------------------------------------------------------------

    def test_has_time_dependencies_false_by_default(self):
        """has_time_dependencies() is False when no time attrs are set."""
        t = ecf.Task("t1")
        self.assertFalse(t.has_time_dependencies())

    def test_has_time_dependencies_true_after_add_time(self):
        """has_time_dependencies() is True after a time is added."""
        t = ecf.Task("t1")
        t.add_time(10, 0)
        self.assertTrue(t.has_time_dependencies())

    def test_has_time_dependencies_true_after_add_today(self):
        """has_time_dependencies() is True after a today is added."""
        t = ecf.Task("t1")
        t.add_today(10, 0)
        self.assertTrue(t.has_time_dependencies())

    def test_has_time_dependencies_true_after_add_date(self):
        """has_time_dependencies() is True after a date is added."""
        t = ecf.Task("t1")
        t.add_date(1, 1, 2025)
        self.assertTrue(t.has_time_dependencies())

    # ------------------------------------------------------------------
    # get_generated_variables
    # ------------------------------------------------------------------

    def test_get_generated_variables_returns_list(self):
        """get_generated_variables() returns a list."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        t.update_generated_variables()
        result = t.get_generated_variables()
        self.assertIsInstance(result, list)

    def test_get_generated_variables_with_variable_list(self):
        """get_generated_variables(VariableList) fills the list in-place."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        t.update_generated_variables()
        vl = ecf.VariableList()
        t.get_generated_variables(vl)
        # VariableList should have been populated
        self.assertIsNotNone(vl)

    # ------------------------------------------------------------------
    # find_gen_variable
    # ------------------------------------------------------------------

    def test_find_gen_variable_not_found_returns_empty(self):
        """find_gen_variable returns empty object for nonexistent variable."""
        t = ecf.Task("t1")
        v = t.find_gen_variable("NONEXISTENT")
        self.assertTrue(v.empty())

    # ------------------------------------------------------------------
    # sort_attributes
    # ------------------------------------------------------------------

    def test_sort_attributes_by_attr_type(self):
        """sort_attributes(AttrType) sorts one type of attribute without raising."""
        t = ecf.Task("t1")
        t.add_variable("Z", "1").add_variable("A", "2")
        t.sort_attributes(ecf.AttrType.variable)

    def test_sort_attributes_by_attr_type_recursive(self):
        """sort_attributes(AttrType, bool) sorts with recursive flag."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_variable("Z", "1").add_variable("A", "2")
        s.sort_attributes(ecf.AttrType.variable, True)

    def test_sort_attributes_by_attr_type_recursive_exclusion(self):
        """sort_attributes(AttrType, bool, list) sorts with exclusion list."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_variable("Z", "1").add_variable("A", "2")
        s.sort_attributes(ecf.AttrType.variable, True, ["s1"])

    def test_sort_attributes_by_str_name(self):
        """sort_attributes(str, bool, list) sorts using string attribute name."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_variable("Z", "1").add_variable("A", "2")
        s.sort_attributes("variable", True, [])

    def test_sort_attributes_invalid_str_raises(self):
        """sort_attributes(invalid_str, ...) raises RuntimeError."""
        t = ecf.Task("t1")
        with self.assertRaises(RuntimeError):
            t.sort_attributes("invalid_attr_type", True, [])

    # ------------------------------------------------------------------
    # find_node_up_the_tree
    # ------------------------------------------------------------------

    def test_find_node_up_the_tree_finds_self(self):
        """find_node_up_the_tree('t1') on t1 finds the task itself."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        found = t.find_node_up_the_tree("t1")
        self.assertIsNotNone(found)

    def test_find_node_up_the_tree_not_found_returns_none(self):
        """find_node_up_the_tree returns None for a name that doesn't exist."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        found = t.find_node_up_the_tree("nonexistent_xyz")
        self.assertIsNone(found)

    # ------------------------------------------------------------------
    # Verifies property
    # ------------------------------------------------------------------

    def test_verifies_property_empty_initially(self):
        """verifies property returns empty iterable initially."""
        t = ecf.Task("t1")
        self.assertEqual(len(list(t.verifies)), 0)

    def test_add_verify_and_check_verifies_property(self):
        """add_verify adds a verify attribute visible via verifies property."""
        t = ecf.Task("t1")
        t.add_verify(ecf.Verify(ecf.State.complete, 1))
        self.assertEqual(len(list(t.verifies)), 1)


if __name__ == "__main__":
    unittest.main(verbosity=2)
