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


class TestFamilyVec(unittest.TestCase):
    """Tests for py::class_<std::vector<family_ptr>> exposed as ecf.FamilyVec in ExportSuiteAndFamily.cpp.

    Exposed API
    -----------
    Constructors
        FamilyVec()              -- default constructor creates an empty vector

    Instance methods (from vector_indexing_suite)
        __len__                  -- number of family nodes
        __getitem__(int)         -- index access
        __iter__                 -- iteration over family nodes
        append(family_ptr)       -- append a family node
    """

    # ------------------------------------------------------------------
    # Construction and basic properties
    # ------------------------------------------------------------------

    def test_default_constructor_creates_empty_vec(self):
        """FamilyVec() creates an empty vector with length zero."""
        fv = ecf.FamilyVec()
        self.assertEqual(len(fv), 0)

    def test_len_after_append(self):
        """After appending one family the length is 1."""
        fv = ecf.FamilyVec()
        fv.append(ecf.Family("f1"))
        self.assertEqual(len(fv), 1)

    def test_len_after_multiple_appends(self):
        """After appending three families the length is 3."""
        fv = ecf.FamilyVec()
        fv.append(ecf.Family("f1"))
        fv.append(ecf.Family("f2"))
        fv.append(ecf.Family("f3"))
        self.assertEqual(len(fv), 3)

    # ------------------------------------------------------------------
    # __getitem__
    # ------------------------------------------------------------------

    def test_getitem_returns_correct_family(self):
        """Index access returns the family at the given position."""
        fv = ecf.FamilyVec()
        fv.append(ecf.Family("alpha"))
        fv.append(ecf.Family("beta"))
        self.assertEqual(fv[0].name(), "alpha")
        self.assertEqual(fv[1].name(), "beta")

    def test_getitem_out_of_range_raises(self):
        """Index access beyond the vector length raises IndexError."""
        fv = ecf.FamilyVec()
        fv.append(ecf.Family("f1"))
        with self.assertRaises(IndexError):
            _ = fv[5]

    # ------------------------------------------------------------------
    # __iter__
    # ------------------------------------------------------------------

    def test_iter_visits_all_families(self):
        """Iterating over FamilyVec yields all appended families in order."""
        fv = ecf.FamilyVec()
        names = ["f1", "f2", "f3"]
        for name in names:
            fv.append(ecf.Family(name))
        visited = [f.name() for f in fv]
        self.assertEqual(visited, names)

    def test_iter_empty_vec_yields_nothing(self):
        """Iterating over an empty FamilyVec yields no items."""
        fv = ecf.FamilyVec()
        self.assertEqual(list(fv), [])


class TestSuiteVec(unittest.TestCase):
    """Tests for py::class_<std::vector<suite_ptr>> exposed as ecf.SuiteVec in ExportSuiteAndFamily.cpp.

    Exposed API
    -----------
    Constructors
        SuiteVec()               -- default constructor creates an empty vector

    Instance methods (from vector_indexing_suite)
        __len__                  -- number of suite nodes
        __getitem__(int)         -- index access
        __iter__                 -- iteration over suite nodes
        append(suite_ptr)        -- append a suite node
    """

    # ------------------------------------------------------------------
    # Construction and basic properties
    # ------------------------------------------------------------------

    def test_default_constructor_creates_empty_vec(self):
        """SuiteVec() creates an empty vector with length zero."""
        sv = ecf.SuiteVec()
        self.assertEqual(len(sv), 0)

    def test_len_after_append(self):
        """After appending one suite the length is 1."""
        sv = ecf.SuiteVec()
        sv.append(ecf.Suite("s1"))
        self.assertEqual(len(sv), 1)

    def test_len_after_multiple_appends(self):
        """After appending three suites the length is 3."""
        sv = ecf.SuiteVec()
        sv.append(ecf.Suite("s1"))
        sv.append(ecf.Suite("s2"))
        sv.append(ecf.Suite("s3"))
        self.assertEqual(len(sv), 3)

    # ------------------------------------------------------------------
    # __getitem__
    # ------------------------------------------------------------------

    def test_getitem_returns_correct_suite(self):
        """Index access returns the suite at the given position."""
        sv = ecf.SuiteVec()
        sv.append(ecf.Suite("alpha"))
        sv.append(ecf.Suite("beta"))
        self.assertEqual(sv[0].name(), "alpha")
        self.assertEqual(sv[1].name(), "beta")

    def test_getitem_out_of_range_raises(self):
        """Index access beyond the vector length raises IndexError."""
        sv = ecf.SuiteVec()
        sv.append(ecf.Suite("s1"))
        with self.assertRaises(IndexError):
            _ = sv[5]

    # ------------------------------------------------------------------
    # __iter__
    # ------------------------------------------------------------------

    def test_iter_visits_all_suites(self):
        """Iterating over SuiteVec yields all appended suites in order."""
        sv = ecf.SuiteVec()
        names = ["s1", "s2", "s3"]
        for name in names:
            sv.append(ecf.Suite(name))
        visited = [s.name() for s in sv]
        self.assertEqual(visited, names)

    def test_iter_empty_vec_yields_nothing(self):
        """Iterating over an empty SuiteVec yields no items."""
        sv = ecf.SuiteVec()
        self.assertEqual(list(sv), [])


class TestNodeContainer(unittest.TestCase):
    """Tests for py::class_<NodeContainer, py::bases<Node>, boost::noncopyable> exposed as ecf.NodeContainer in ExportSuiteAndFamily.cpp.

    NodeContainer is abstract (py::no_init); concrete subclasses are Family and Suite.

    Exposed API
    -----------
    Constructors
        (abstract -- no direct construction; use Family or Suite)

    Instance methods
        __iter__                     -- iterate over immediate children
        add_family(str)  -> family_ptr  -- create and add a child family by name
        add_family(family_ptr) -> family_ptr  -- add an existing family_ptr
        add_task(str)    -> task_ptr    -- create and add a child task by name
        add_task(task_ptr) -> task_ptr  -- add an existing task_ptr
        find_node(str)   -> node_ptr or None  -- find immediate child by name
        find_task(str)   -> task_ptr or None  -- find immediate task child by name
        find_family(str) -> family_ptr or None  -- find immediate family child by name

    Properties
        nodes            -- range over immediate children (same as __iter__)
    """

    # ------------------------------------------------------------------
    # NodeContainer is abstract
    # ------------------------------------------------------------------

    def test_direct_construction_raises(self):
        """NodeContainer cannot be instantiated directly (py::no_init)."""
        with self.assertRaises(Exception):
            ecf.NodeContainer()

    # ------------------------------------------------------------------
    # add_family -- two overloads
    # ------------------------------------------------------------------

    def test_add_family_by_name_returns_family(self):
        """add_family(str) creates a family, adds it, and returns the family_ptr."""
        s = ecf.Suite("s")
        ret = s.add_family("child_fam")
        self.assertIsInstance(ret, ecf.Family)
        self.assertEqual(ret.name(), "child_fam")

    def test_add_family_by_name_is_immediately_findable(self):
        """A family added by name can be found with find_family."""
        s = ecf.Suite("s")
        s.add_family("f1")
        found = s.find_family("f1")
        self.assertIsNotNone(found)
        self.assertEqual(found.name(), "f1")

    def test_add_family_by_ptr_returns_same_family(self):
        """add_family(family_ptr) adds the existing family and returns it."""
        s = ecf.Suite("s")
        f = ecf.Family("existing")
        ret = s.add_family(f)
        self.assertIsInstance(ret, ecf.Family)
        self.assertEqual(ret.name(), "existing")

    def test_add_family_by_ptr_is_immediately_findable(self):
        """A family added by ptr can be found with find_family."""
        s = ecf.Suite("s")
        f = ecf.Family("existing")
        s.add_family(f)
        found = s.find_family("existing")
        self.assertIsNotNone(found)

    # ------------------------------------------------------------------
    # add_task -- two overloads
    # ------------------------------------------------------------------

    def test_add_task_by_name_returns_task(self):
        """add_task(str) creates a task, adds it, and returns the task_ptr."""
        s = ecf.Suite("s")
        ret = s.add_task("my_task")
        self.assertIsInstance(ret, ecf.Task)
        self.assertEqual(ret.name(), "my_task")

    def test_add_task_by_name_is_immediately_findable(self):
        """A task added by name can be found with find_task."""
        s = ecf.Suite("s")
        s.add_task("t1")
        found = s.find_task("t1")
        self.assertIsNotNone(found)
        self.assertEqual(found.name(), "t1")

    def test_add_task_by_ptr_returns_task(self):
        """add_task(task_ptr) adds the existing task and returns it."""
        s = ecf.Suite("s")
        t = ecf.Task("existing_task")
        ret = s.add_task(t)
        self.assertIsInstance(ret, ecf.Task)
        self.assertEqual(ret.name(), "existing_task")

    def test_add_task_by_ptr_is_immediately_findable(self):
        """A task added by ptr can be found with find_task."""
        s = ecf.Suite("s")
        t = ecf.Task("existing_task")
        s.add_task(t)
        found = s.find_task("existing_task")
        self.assertIsNotNone(found)

    # ------------------------------------------------------------------
    # find_node
    # ------------------------------------------------------------------

    def test_find_node_returns_task_node(self):
        """find_node returns a node_ptr for an existing task child."""
        s = ecf.Suite("s")
        s.add_task("t1")
        found = s.find_node("t1")
        self.assertIsNotNone(found)
        self.assertEqual(found.name(), "t1")

    def test_find_node_returns_family_node(self):
        """find_node returns a node_ptr for an existing family child."""
        s = ecf.Suite("s")
        s.add_family("f1")
        found = s.find_node("f1")
        self.assertIsNotNone(found)
        self.assertEqual(found.name(), "f1")

    def test_find_node_missing_returns_none(self):
        """find_node returns None when no immediate child has the given name."""
        s = ecf.Suite("s")
        s.add_task("t1")
        result = s.find_node("not_there")
        self.assertIsNone(result)

    # ------------------------------------------------------------------
    # find_task
    # ------------------------------------------------------------------

    def test_find_task_returns_existing_task(self):
        """find_task returns the task_ptr when a task with that name exists."""
        s = ecf.Suite("s")
        s.add_task("my_task")
        found = s.find_task("my_task")
        self.assertIsNotNone(found)
        self.assertEqual(found.name(), "my_task")

    def test_find_task_missing_returns_none(self):
        """find_task returns None when no task with that name exists."""
        s = ecf.Suite("s")
        result = s.find_task("ghost")
        self.assertIsNone(result)

    # ------------------------------------------------------------------
    # find_family
    # ------------------------------------------------------------------

    def test_find_family_returns_existing_family(self):
        """find_family returns the family_ptr when a family with that name exists."""
        s = ecf.Suite("s")
        s.add_family("my_fam")
        found = s.find_family("my_fam")
        self.assertIsNotNone(found)
        self.assertEqual(found.name(), "my_fam")

    def test_find_family_missing_returns_none(self):
        """find_family returns None when no family with that name exists."""
        s = ecf.Suite("s")
        result = s.find_family("ghost_fam")
        self.assertIsNone(result)

    # ------------------------------------------------------------------
    # __iter__ over immediate children
    # ------------------------------------------------------------------

    def test_iter_visits_all_immediate_children(self):
        """Iterating over a NodeContainer yields all immediate children in insertion order."""
        s = ecf.Suite("s")
        s.add_task("t1")
        s.add_family("f1")
        s.add_task("t2")
        names = [n.name() for n in s]
        self.assertEqual(names, ["t1", "f1", "t2"])

    def test_iter_empty_container_yields_nothing(self):
        """Iterating over a NodeContainer with no children yields nothing."""
        s = ecf.Suite("s")
        self.assertEqual(list(s), [])

    # ------------------------------------------------------------------
    # nodes property
    # ------------------------------------------------------------------

    def test_nodes_property_returns_same_as_iter(self):
        """The nodes property yields the same children as __iter__."""
        s = ecf.Suite("s")
        s.add_task("t1")
        s.add_family("f1")
        iter_names = [n.name() for n in s]
        nodes_names = [n.name() for n in s.nodes]
        self.assertEqual(iter_names, nodes_names)

    def test_nodes_property_empty_yields_nothing(self):
        """The nodes property on an empty NodeContainer yields nothing."""
        s = ecf.Suite("s")
        self.assertEqual(list(s.nodes), [])


class TestFamily(unittest.TestCase):
    """Tests for py::class_<Family, py::bases<NodeContainer>, family_ptr> exposed as ecf.Family in ExportSuiteAndFamily.cpp.

    Exposed API
    -----------
    Constructors
        Family(str)                        -- create from name string
        Family(str, list, **kwargs)        -- create with children and variables
        Family(str, list)                  -- create with children, no variables

    Instance methods (inherited from NodeContainer)
        add_family(str)   -> family_ptr    -- create+add child family
        add_family(family_ptr) -> family_ptr  -- add existing child family
        add_task(str)     -> task_ptr      -- create+add child task
        add_task(task_ptr) -> task_ptr     -- add existing child task
        find_node(str)    -> node_ptr or None
        find_task(str)    -> task_ptr or None
        find_family(str)  -> family_ptr or None

    Operators
        __eq__      -- value-based equality (compares full subtree)
        __ne__      -- implicit complement of __eq__
        __hash__    -- identity-based (boost.python C-extension type)
        __str__     -- serialised definition string
        __copy__    -- copy.copy() returns value-equal, identity-distinct instance
        __enter__   -- returns self (context manager support)
        __exit__    -- returns False (context manager support)
        __len__     -- number of immediate children
        __contains__ -- True if immediate child with given name exists
    """

    # ------------------------------------------------------------------
    # Constructor: Family(str)
    # ------------------------------------------------------------------

    def test_create_from_name_sets_name(self):
        """Family(str) creates a family with the given name."""
        f = ecf.Family("myfam")
        self.assertEqual(f.name(), "myfam")

    def test_create_from_name_is_empty(self):
        """Family(str) with no children produces an empty container."""
        f = ecf.Family("myfam")
        self.assertEqual(len(f), 0)

    def test_create_from_empty_name_raises(self):
        """Family('') raises RuntimeError because empty names are invalid."""
        with self.assertRaises(RuntimeError):
            ecf.Family("")

    def test_create_from_name_with_space_raises(self):
        """Family('has space') raises RuntimeError because spaces are not allowed."""
        with self.assertRaises(RuntimeError):
            ecf.Family("has space")

    # ------------------------------------------------------------------
    # Constructor: Family(str, list, **kwargs)
    # ------------------------------------------------------------------

    def test_create_with_children_sets_len(self):
        """Family(str, [Task, Task]) creates a family with two immediate children."""
        f = ecf.Family("f", [ecf.Task("t1"), ecf.Task("t2")])
        self.assertEqual(len(f), 2)

    def test_create_with_children_sets_variables(self):
        """Family(str, list, KEY=val) stores the keyword argument as a variable."""
        f = ecf.Family("f", [ecf.Task("t1")], MY_VAR="hello")
        self.assertEqual(f.find_variable("MY_VAR").value(), "hello")

    def test_create_with_nested_family_child(self):
        """Family(str, [Family(...)]) nests a child family."""
        f = ecf.Family("parent", [ecf.Family("child")])
        self.assertIsNotNone(f.find_family("child"))

    # ------------------------------------------------------------------
    # __eq__ / __ne__ / __hash__
    # ------------------------------------------------------------------

    def test_eq_same_name_empty_families_are_equal(self):
        """Two empty families with the same name compare equal."""
        f1 = ecf.Family("f")
        f2 = ecf.Family("f")
        self.assertEqual(f1, f2)

    def test_eq_different_names_are_not_equal(self):
        """Families with different names compare unequal."""
        f1 = ecf.Family("fa")
        f2 = ecf.Family("fb")
        self.assertNotEqual(f1, f2)

    def test_ne_is_complement_of_eq(self):
        """__ne__ is the complement of __eq__."""
        f1 = ecf.Family("fa")
        f2 = ecf.Family("fb")
        self.assertTrue(f1 != f2)
        f3 = ecf.Family("fa")
        self.assertFalse(f1 != f3)

    def test_hash_is_identity_based_not_value_based(self):
        """Two value-equal families have different hashes (identity-based hashing)."""
        f1 = ecf.Family("f")
        f2 = ecf.Family("f")
        self.assertEqual(f1, f2)
        self.assertNotEqual(hash(f1), hash(f2))

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_returns_non_empty_string(self):
        """str(Family) returns a non-empty string."""
        f = ecf.Family("myfam")
        self.assertIsInstance(str(f), str)
        self.assertGreater(len(str(f)), 0)

    def test_str_contains_family_name(self):
        """The serialised string contains the family name."""
        f = ecf.Family("special_name")
        self.assertIn("special_name", str(f))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_produces_value_equal_instance(self):
        """copy.copy(family) returns an instance that compares equal to the original."""
        f = ecf.Family("f")
        f.add_task("t1")
        f2 = copy.copy(f)
        self.assertEqual(f, f2)

    def test_copy_produces_identity_distinct_instance(self):
        """copy.copy(family) returns a distinct object (not the same reference)."""
        f = ecf.Family("f")
        f2 = copy.copy(f)
        self.assertIsNot(f, f2)

    # ------------------------------------------------------------------
    # __enter__ / __exit__ (context manager)
    # ------------------------------------------------------------------

    def test_context_manager_returns_self(self):
        """The with-statement binds the Family itself (not a wrapper)."""
        f = ecf.Family("f")
        with f as ctx:
            self.assertIs(ctx, f)

    def test_context_manager_allows_child_addition(self):
        """Children added inside a with-block are present after the block."""
        with ecf.Family("f") as f:
            f.add_task("t1")
            f.add_family("child")
        self.assertEqual(len(f), 2)

    # ------------------------------------------------------------------
    # __len__ (sized protocol)
    # ------------------------------------------------------------------

    def test_len_empty_family_is_zero(self):
        """A newly created Family with no children has len 0."""
        f = ecf.Family("f")
        self.assertEqual(len(f), 0)

    def test_len_counts_only_immediate_children(self):
        """__len__ counts only direct children, not grandchildren."""
        f = ecf.Family("f")
        child = f.add_family("child")
        child.add_task("grandchild")
        self.assertEqual(len(f), 1)

    def test_len_increases_with_each_child(self):
        """__len__ increases by one for each child added."""
        f = ecf.Family("f")
        f.add_task("t1")
        self.assertEqual(len(f), 1)
        f.add_family("sub")
        self.assertEqual(len(f), 2)

    # ------------------------------------------------------------------
    # __contains__ (container protocol)
    # ------------------------------------------------------------------

    def test_contains_returns_true_for_existing_child(self):
        """'name' in family is True when an immediate child with that name exists."""
        f = ecf.Family("f")
        f.add_task("t1")
        self.assertIn("t1", f)

    def test_contains_returns_false_for_missing_child(self):
        """'name' in family is False when no immediate child with that name exists."""
        f = ecf.Family("f")
        self.assertNotIn("ghost", f)

    def test_contains_checks_only_immediate_children(self):
        """__contains__ does not descend into grandchildren."""
        f = ecf.Family("f")
        child = f.add_family("child")
        child.add_task("grandchild")
        self.assertNotIn("grandchild", f)

    def test_contains_family_child_name(self):
        """__contains__ works for child families as well as tasks."""
        f = ecf.Family("f")
        f.add_family("subfam")
        self.assertIn("subfam", f)


class TestSuite(unittest.TestCase):
    """Tests for py::class_<Suite, py::bases<NodeContainer>, suite_ptr> exposed as ecf.Suite in ExportSuiteAndFamily.cpp.

    Suite inherits all NodeContainer and Family-like behaviour, plus clock attributes.

    Exposed API
    -----------
    Constructors
        Suite(str)                         -- create from name string
        Suite(str, list, **kwargs)         -- create with children and variables
        Suite(str, list)                   -- create with children, no variables

    Instance methods (inherited from NodeContainer)
        add_family(str)   -> family_ptr
        add_family(family_ptr) -> family_ptr
        add_task(str)     -> task_ptr
        add_task(task_ptr) -> task_ptr
        find_node(str)    -> node_ptr or None
        find_task(str)    -> task_ptr or None
        find_family(str)  -> family_ptr or None

    Suite-specific methods
        add_clock(ClockAttr)     -> suite_ptr  -- fluent; returns self
        get_clock()              -> ClockAttr or None
        add_end_clock(ClockAttr) -> suite_ptr  -- fluent; returns self
        get_end_clock()          -> ClockAttr or None
        begun()                  -> bool

    Operators
        __eq__      -- value-based equality (compares full subtree)
        __ne__      -- implicit complement of __eq__
        __hash__    -- identity-based (boost.python C-extension type)
        __str__     -- serialised definition string
        __copy__    -- copy.copy() returns value-equal, identity-distinct instance
        __enter__   -- returns self (context manager support)
        __exit__    -- returns False (context manager support)
        __len__     -- number of immediate children
        __contains__ -- True if immediate child with given name exists
    """

    # ------------------------------------------------------------------
    # Constructor: Suite(str)
    # ------------------------------------------------------------------

    def test_create_from_name_sets_name(self):
        """Suite(str) creates a suite with the given name."""
        s = ecf.Suite("mysuite")
        self.assertEqual(s.name(), "mysuite")

    def test_create_from_name_is_empty(self):
        """Suite(str) with no children produces an empty container."""
        s = ecf.Suite("mysuite")
        self.assertEqual(len(s), 0)

    def test_create_from_empty_name_raises(self):
        """Suite('') raises RuntimeError because empty names are invalid."""
        with self.assertRaises(RuntimeError):
            ecf.Suite("")

    def test_create_from_name_with_space_raises(self):
        """Suite('has space') raises RuntimeError because spaces are not allowed."""
        with self.assertRaises(RuntimeError):
            ecf.Suite("has space")

    # ------------------------------------------------------------------
    # Constructor: Suite(str, list, **kwargs)
    # ------------------------------------------------------------------

    def test_create_with_children_sets_len(self):
        """Suite(str, [Task, Family]) creates a suite with two immediate children."""
        s = ecf.Suite("s", [ecf.Task("t1"), ecf.Family("f1")])
        self.assertEqual(len(s), 2)

    def test_create_with_children_sets_variables(self):
        """Suite(str, list, KEY=val) stores the keyword argument as a variable."""
        s = ecf.Suite("s", [ecf.Task("t1")], MY_VAR="world")
        self.assertEqual(s.find_variable("MY_VAR").value(), "world")

    # ------------------------------------------------------------------
    # __eq__ / __ne__ / __hash__
    # ------------------------------------------------------------------

    def test_eq_same_name_empty_suites_are_equal(self):
        """Two empty suites with the same name compare equal."""
        s1 = ecf.Suite("s")
        s2 = ecf.Suite("s")
        self.assertEqual(s1, s2)

    def test_eq_different_names_are_not_equal(self):
        """Suites with different names compare unequal."""
        s1 = ecf.Suite("sa")
        s2 = ecf.Suite("sb")
        self.assertNotEqual(s1, s2)

    def test_ne_is_complement_of_eq(self):
        """__ne__ is the complement of __eq__."""
        s1 = ecf.Suite("sa")
        s2 = ecf.Suite("sb")
        self.assertTrue(s1 != s2)
        s3 = ecf.Suite("sa")
        self.assertFalse(s1 != s3)

    def test_hash_is_identity_based_not_value_based(self):
        """Two value-equal suites have different hashes (identity-based hashing)."""
        s1 = ecf.Suite("s")
        s2 = ecf.Suite("s")
        self.assertEqual(s1, s2)
        self.assertNotEqual(hash(s1), hash(s2))

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_returns_non_empty_string(self):
        """str(Suite) returns a non-empty string."""
        s = ecf.Suite("mysuite")
        self.assertIsInstance(str(s), str)
        self.assertGreater(len(str(s)), 0)

    def test_str_contains_suite_name(self):
        """The serialised string contains the suite name."""
        s = ecf.Suite("special_suite")
        self.assertIn("special_suite", str(s))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_produces_value_equal_instance(self):
        """copy.copy(suite) returns an instance that compares equal to the original."""
        s = ecf.Suite("s")
        s.add_task("t1")
        s2 = copy.copy(s)
        self.assertEqual(s, s2)

    def test_copy_produces_identity_distinct_instance(self):
        """copy.copy(suite) returns a distinct object (not the same reference)."""
        s = ecf.Suite("s")
        s2 = copy.copy(s)
        self.assertIsNot(s, s2)

    # ------------------------------------------------------------------
    # __enter__ / __exit__ (context manager)
    # ------------------------------------------------------------------

    def test_context_manager_returns_self(self):
        """The with-statement binds the Suite itself (not a wrapper)."""
        s = ecf.Suite("s")
        with s as ctx:
            self.assertIs(ctx, s)

    def test_context_manager_allows_child_addition(self):
        """Children added inside a with-block are present after the block."""
        with ecf.Suite("s") as s:
            s.add_task("t1")
            s.add_family("f1")
        self.assertEqual(len(s), 2)

    # ------------------------------------------------------------------
    # __len__ (sized protocol)
    # ------------------------------------------------------------------

    def test_len_empty_suite_is_zero(self):
        """A newly created Suite with no children has len 0."""
        s = ecf.Suite("s")
        self.assertEqual(len(s), 0)

    def test_len_counts_only_immediate_children(self):
        """__len__ counts only direct children, not grandchildren."""
        s = ecf.Suite("s")
        child = s.add_family("f")
        child.add_task("grandchild")
        self.assertEqual(len(s), 1)

    def test_len_increases_with_each_child(self):
        """__len__ increases by one for each child added."""
        s = ecf.Suite("s")
        s.add_task("t1")
        self.assertEqual(len(s), 1)
        s.add_family("f1")
        self.assertEqual(len(s), 2)

    # ------------------------------------------------------------------
    # __contains__ (container protocol)
    # ------------------------------------------------------------------

    def test_contains_returns_true_for_existing_task_child(self):
        """'name' in suite is True when an immediate task child with that name exists."""
        s = ecf.Suite("s")
        s.add_task("t1")
        self.assertIn("t1", s)

    def test_contains_returns_true_for_existing_family_child(self):
        """'name' in suite is True when an immediate family child with that name exists."""
        s = ecf.Suite("s")
        s.add_family("f1")
        self.assertIn("f1", s)

    def test_contains_returns_false_for_missing_child(self):
        """'name' in suite is False when no immediate child with that name exists."""
        s = ecf.Suite("s")
        self.assertNotIn("ghost", s)

    def test_contains_checks_only_immediate_children(self):
        """__contains__ does not descend into grandchildren."""
        s = ecf.Suite("s")
        child = s.add_family("f")
        child.add_task("grandchild")
        self.assertNotIn("grandchild", s)

    # ------------------------------------------------------------------
    # add_clock / get_clock
    # ------------------------------------------------------------------

    def test_get_clock_returns_none_when_no_clock_set(self):
        """get_clock() returns None when no clock has been added."""
        s = ecf.Suite("s")
        self.assertIsNone(s.get_clock())

    def test_add_clock_returns_suite_self(self):
        """add_clock(ClockAttr) returns the suite itself (fluent interface)."""
        s = ecf.Suite("s")
        clk = ecf.Clock(1, 1, 2020)
        ret = s.add_clock(clk)
        self.assertIs(ret, s)

    def test_get_clock_returns_clock_after_add(self):
        """get_clock() returns a ClockAttr instance after add_clock."""
        s = ecf.Suite("s")
        clk = ecf.Clock(1, 1, 2020)
        s.add_clock(clk)
        result = s.get_clock()
        self.assertIsNotNone(result)
        self.assertIsInstance(result, ecf.Clock)

    # ------------------------------------------------------------------
    # add_end_clock / get_end_clock
    # ------------------------------------------------------------------

    def test_get_end_clock_returns_none_when_no_end_clock_set(self):
        """get_end_clock() returns None when no end clock has been added."""
        s = ecf.Suite("s")
        self.assertIsNone(s.get_end_clock())

    def test_add_end_clock_returns_suite_self(self):
        """add_end_clock(ClockAttr) returns the suite itself (fluent interface)."""
        s = ecf.Suite("s")
        end_clk = ecf.Clock(31, 12, 2020)
        ret = s.add_end_clock(end_clk)
        self.assertIs(ret, s)

    def test_get_end_clock_returns_clock_after_add(self):
        """get_end_clock() returns a ClockAttr instance after add_end_clock."""
        s = ecf.Suite("s")
        end_clk = ecf.Clock(31, 12, 2020)
        s.add_end_clock(end_clk)
        result = s.get_end_clock()
        self.assertIsNotNone(result)
        self.assertIsInstance(result, ecf.Clock)

    # ------------------------------------------------------------------
    # begun()
    # ------------------------------------------------------------------

    def test_begun_returns_false_for_new_suite(self):
        """begun() returns False for a freshly created suite that has not been started."""
        s = ecf.Suite("s")
        self.assertFalse(s.begun())

    def test_begun_returns_bool(self):
        """begun() returns a Python bool."""
        s = ecf.Suite("s")
        result = s.begun()
        self.assertIsInstance(result, (bool, int))


if __name__ == "__main__":
    unittest.main(verbosity=2)
