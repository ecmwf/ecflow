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


class TestDebugBuild(unittest.TestCase):
    """Tests for the free function debug_build() exposed in ExportCore.cpp.

    Exposed API
    -----------
    Functions
        debug_build() -> bool  -- returns True when built in debug mode, False in release mode
    """

    # ------------------------------------------------------------------
    # debug_build()
    # ------------------------------------------------------------------

    def test_returns_bool(self):
        """debug_build() returns a Python bool."""
        self.assertIsInstance(ecf.debug_build(), bool)

    def test_callable_with_no_arguments(self):
        """debug_build() takes no arguments."""
        result = ecf.debug_build()
        self.assertIn(result, (True, False))

    def test_result_is_stable(self):
        """Repeated calls return the same value (it is a build-time constant)."""
        self.assertEqual(ecf.debug_build(), ecf.debug_build())


class TestEdit(unittest.TestCase):
    """Tests for py::class_<Edit> exposed as ecf.Edit in ExportCore.cpp.

    Exposed API
    -----------
    Constructors
        Edit(dict)            -- create from a single mapping of name -> value
        Edit(dict, dict)      -- merge two mappings into one Edit
        Edit(**kwargs)        -- create from keyword arguments (via raw_function)
        Edit(dict, **kwargs)  -- merge positional dict with keyword arguments

    Note
    ----
    Edit is a helper used to pass variables into Node.add() and the >> / += operators.
    The C++ __str__ is a no-argument static method; calling str() on an instance raises
    Boost.Python.ArgumentError due to the static/instance signature mismatch.  This is a
    known binding limitation and is documented by the dedicated test below.
    """

    # ------------------------------------------------------------------
    # Constructor: Edit(dict)
    # ------------------------------------------------------------------

    def test_ctor_dict_is_accepted(self):
        """Edit(dict) constructs successfully."""
        e = ecf.Edit({"X": "1"})
        self.assertIsInstance(e, ecf.Edit)

    def test_ctor_empty_dict_is_accepted(self):
        """Edit({}) constructs without error."""
        e = ecf.Edit({})
        self.assertIsInstance(e, ecf.Edit)

    def test_ctor_dict_adds_variable_when_passed_to_node(self):
        """Edit(dict) variables are visible on the node after Node.add(Edit(...))."""
        t = ecf.Task("t1")
        t.add(ecf.Edit({"MY_VAR": "hello"}))
        self.assertEqual(t.find_variable("MY_VAR").value(), "hello")

    def test_ctor_dict_multiple_entries_all_added(self):
        """All key-value pairs in the dict become node variables."""
        t = ecf.Task("t1")
        t.add(ecf.Edit({"A": "1", "B": "2"}))
        self.assertEqual(t.find_variable("A").value(), "1")
        self.assertEqual(t.find_variable("B").value(), "2")

    # ------------------------------------------------------------------
    # Constructor: Edit(dict, dict)
    # ------------------------------------------------------------------

    def test_ctor_two_dicts_merges_both(self):
        """Edit(dict, dict) combines variables from both dicts."""
        t = ecf.Task("t1")
        t.add(ecf.Edit({"X": "10"}, {"Y": "20"}))
        self.assertEqual(t.find_variable("X").value(), "10")
        self.assertEqual(t.find_variable("Y").value(), "20")

    def test_ctor_two_empty_dicts_is_accepted(self):
        """Edit({}, {}) constructs without error."""
        e = ecf.Edit({}, {})
        self.assertIsInstance(e, ecf.Edit)

    # ------------------------------------------------------------------
    # Constructor: Edit(**kwargs)  via raw_function
    # ------------------------------------------------------------------

    def test_ctor_kwargs_adds_variables(self):
        """Edit(**kwargs) variables are visible on the node after Node.add(Edit(...))."""
        t = ecf.Task("t1")
        t.add(ecf.Edit(ALPHA="a", BETA="b"))
        self.assertEqual(t.find_variable("ALPHA").value(), "a")
        self.assertEqual(t.find_variable("BETA").value(), "b")

    # ------------------------------------------------------------------
    # Constructor: Edit(dict, **kwargs)
    # ------------------------------------------------------------------

    def test_ctor_dict_and_kwargs_merges_both(self):
        """Edit(dict, **kwargs) combines positional dict and keyword arguments."""
        t = ecf.Task("t1")
        t.add(ecf.Edit({"FROM_DICT": "d"}, FROM_KW="k"))
        self.assertEqual(t.find_variable("FROM_DICT").value(), "d")
        self.assertEqual(t.find_variable("FROM_KW").value(), "k")

    # ------------------------------------------------------------------
    # Invalid argument
    # ------------------------------------------------------------------

    def test_ctor_non_dict_positional_raises(self):
        """Edit accepts only dict positional arguments; a plain string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Edit("not_a_dict")

    # ------------------------------------------------------------------
    # __str__ — static/instance mismatch
    # ------------------------------------------------------------------

    def test_str_on_instance_raises_due_to_static_method_mismatch(self):
        """str(Edit(...)) raises because __str__ is bound to a static C++ method
        that takes no arguments; Boost.Python cannot pass the implicit self."""
        e = ecf.Edit({"A": "1"})
        with self.assertRaises(Exception):
            str(e)


class TestFile(unittest.TestCase):
    """Tests for py::class_<ecf::File, boost::noncopyable> exposed as ecf.File in ExportCore.cpp.

    Exposed API
    -----------
    Constructors
        (no_init -- cannot be instantiated from Python)

    Static methods
        find_server() -> str  -- path to the ecflow_server executable
        find_client() -> str  -- path to the ecflow_client executable
        source_dir()  -> str  -- path to the ecflow source root directory
        build_dir()   -> str  -- path to the ecflow build root directory
    """

    # ------------------------------------------------------------------
    # Constructor: no_init
    # ------------------------------------------------------------------

    def test_direct_construction_raises(self):
        """File cannot be instantiated from Python."""
        with self.assertRaises(RuntimeError):
            ecf.File()

    # ------------------------------------------------------------------
    # find_server()
    # ------------------------------------------------------------------

    def test_find_server_returns_str(self):
        """File.find_server() returns a Python str."""
        self.assertIsInstance(ecf.File.find_server(), str)

    def test_find_server_callable_without_instance(self):
        """find_server() is a static method callable on the class."""
        result = ecf.File.find_server()
        self.assertIsInstance(result, str)

    # ------------------------------------------------------------------
    # find_client()
    # ------------------------------------------------------------------

    def test_find_client_returns_str(self):
        """File.find_client() returns a Python str."""
        self.assertIsInstance(ecf.File.find_client(), str)

    def test_find_client_callable_without_instance(self):
        """find_client() is a static method callable on the class."""
        self.assertIsInstance(ecf.File.find_client(), str)

    # ------------------------------------------------------------------
    # source_dir()
    # ------------------------------------------------------------------

    def test_source_dir_returns_str(self):
        """File.source_dir() returns a Python str."""
        self.assertIsInstance(ecf.File.source_dir(), str)

    def test_source_dir_callable_without_instance(self):
        """source_dir() is a static method callable on the class."""
        self.assertIsInstance(ecf.File.source_dir(), str)

    # ------------------------------------------------------------------
    # build_dir()
    # ------------------------------------------------------------------

    def test_build_dir_returns_str(self):
        """File.build_dir() returns a Python str."""
        self.assertIsInstance(ecf.File.build_dir(), str)

    def test_build_dir_callable_without_instance(self):
        """build_dir() is a static method callable on the class."""
        self.assertIsInstance(ecf.File.build_dir(), str)


class TestStyle(unittest.TestCase):
    """Tests for py::enum_<PrintStyle::Type_t> exposed as ecf.Style in ExportCore.cpp.

    Exposed API
    -----------
    Enum values
        Style.NOTHING  -- suppress all output
        Style.DEFS     -- definition file format (parseable, re-loadable; default)
        Style.STATE    -- includes extra state info; excludes edit history
        Style.MIGRATE  -- includes structure, state, and edit history

    Operators
        __eq__   -- value comparison between Style members
        __ne__   -- implicit complement of __eq__
        __int__  -- integer value of the enum member
    """

    # ------------------------------------------------------------------
    # Enum values
    # ------------------------------------------------------------------

    def test_nothing_is_accessible(self):
        """Style.NOTHING is a valid enum member."""
        _ = ecf.Style.NOTHING

    def test_defs_is_accessible(self):
        """Style.DEFS is a valid enum member."""
        _ = ecf.Style.DEFS

    def test_state_is_accessible(self):
        """Style.STATE is a valid enum member."""
        _ = ecf.Style.STATE

    def test_migrate_is_accessible(self):
        """Style.MIGRATE is a valid enum member."""
        _ = ecf.Style.MIGRATE

    def test_all_four_values_are_distinct(self):
        """All four Style values are mutually distinct."""
        values = [ecf.Style.NOTHING, ecf.Style.DEFS, ecf.Style.STATE, ecf.Style.MIGRATE]
        for i, a in enumerate(values):
            for j, b in enumerate(values):
                if i != j:
                    self.assertNotEqual(a, b)

    # ------------------------------------------------------------------
    # __eq__ and __ne__
    # ------------------------------------------------------------------

    def test_eq_same_value(self):
        """A Style member compares equal to itself."""
        self.assertEqual(ecf.Style.DEFS, ecf.Style.DEFS)

    def test_ne_different_values(self):
        """Different Style members are not equal."""
        self.assertNotEqual(ecf.Style.DEFS, ecf.Style.STATE)

    # ------------------------------------------------------------------
    # __int__
    # ------------------------------------------------------------------

    def test_int_conversion(self):
        """Style values can be converted to int."""
        self.assertIsInstance(int(ecf.Style.DEFS), int)

    def test_nothing_has_int_value_zero(self):
        """Style.NOTHING has integer value 0."""
        self.assertEqual(int(ecf.Style.NOTHING), 0)

    # ------------------------------------------------------------------
    # py_finalize_enum — str / repr / hash / .values / .names
    # ------------------------------------------------------------------

    def test_str_returns_member_name_only(self):
        """str(Style.DEFS) returns 'DEFS', not 'Style.DEFS'."""
        self.assertEqual(str(ecf.Style.DEFS), "DEFS")
        self.assertEqual(str(ecf.Style.STATE), "STATE")

    def test_repr_is_module_qualified(self):
        """repr(Style.DEFS) returns 'ecflow.Style.DEFS'."""
        self.assertEqual(repr(ecf.Style.DEFS), "ecflow.Style.DEFS")

    def test_hash_is_integer(self):
        """hash(Style.DEFS) is a Python int."""
        self.assertIsInstance(hash(ecf.Style.DEFS), int)

    def test_hash_equals_underlying_int_value(self):
        """hash(member) equals member.value."""
        self.assertEqual(hash(ecf.Style.DEFS), ecf.Style.DEFS.value)

    def test_values_dict_maps_int_to_member(self):
        """Style.values is a dict mapping integer value -> enum member."""
        values = ecf.Style.values
        self.assertIsInstance(values, dict)
        self.assertIn(ecf.Style.DEFS.value, values)
        self.assertEqual(values[ecf.Style.DEFS.value], ecf.Style.DEFS)

    def test_names_dict_maps_str_to_member(self):
        """Style.names is a dict mapping name string -> enum member."""
        names = ecf.Style.names
        self.assertIsInstance(names, dict)
        self.assertIn("DEFS", names)
        self.assertEqual(names["DEFS"], ecf.Style.DEFS)


class TestPrintStyle(unittest.TestCase):
    """Tests for py::class_<PrintStyle, boost::noncopyable> exposed as ecf.PrintStyle in ExportCore.cpp.

    Exposed API
    -----------
    Constructors
        (no_init -- singleton, cannot be instantiated from Python)

    Static methods
        get_style() -> Style   -- returns the current global print style
        set_style(Style)       -- sets the global print style
    """

    def setUp(self):
        """Save current style so tests can restore it."""
        self._original = ecf.PrintStyle.get_style()

    def tearDown(self):
        """Restore the original style after each test."""
        ecf.PrintStyle.set_style(self._original)

    # ------------------------------------------------------------------
    # Constructor: no_init
    # ------------------------------------------------------------------

    def test_direct_construction_raises(self):
        """PrintStyle cannot be instantiated from Python."""
        with self.assertRaises(RuntimeError):
            ecf.PrintStyle()

    # ------------------------------------------------------------------
    # get_style()
    # ------------------------------------------------------------------

    def test_get_style_returns_a_style_value(self):
        """get_style() returns a member of the Style enum."""
        style = ecf.PrintStyle.get_style()
        self.assertIn(
            style,
            [ecf.Style.NOTHING, ecf.Style.DEFS, ecf.Style.STATE, ecf.Style.MIGRATE],
        )

    def test_get_style_callable_on_class(self):
        """get_style() is a static method callable without an instance."""
        _ = ecf.PrintStyle.get_style()

    # ------------------------------------------------------------------
    # set_style()
    # ------------------------------------------------------------------

    def test_set_style_defs(self):
        """set_style(Style.DEFS) makes get_style() return DEFS."""
        ecf.PrintStyle.set_style(ecf.Style.DEFS)
        self.assertEqual(ecf.PrintStyle.get_style(), ecf.Style.DEFS)

    def test_set_style_state(self):
        """set_style(Style.STATE) makes get_style() return STATE."""
        ecf.PrintStyle.set_style(ecf.Style.STATE)
        self.assertEqual(ecf.PrintStyle.get_style(), ecf.Style.STATE)

    def test_set_style_migrate(self):
        """set_style(Style.MIGRATE) makes get_style() return MIGRATE."""
        ecf.PrintStyle.set_style(ecf.Style.MIGRATE)
        self.assertEqual(ecf.PrintStyle.get_style(), ecf.Style.MIGRATE)

    def test_set_style_nothing(self):
        """set_style(Style.NOTHING) makes get_style() return NOTHING."""
        ecf.PrintStyle.set_style(ecf.Style.NOTHING)
        self.assertEqual(ecf.PrintStyle.get_style(), ecf.Style.NOTHING)

    def test_set_then_get_round_trips(self):
        """Each style value round-trips through set_style/get_style."""
        for style in [
            ecf.Style.DEFS,
            ecf.Style.STATE,
            ecf.Style.MIGRATE,
            ecf.Style.NOTHING,
        ]:
            ecf.PrintStyle.set_style(style)
            self.assertEqual(ecf.PrintStyle.get_style(), style)


class TestCheckPt(unittest.TestCase):
    """Tests for py::enum_<ecf::CheckPt::Mode> exposed as ecf.CheckPt in ExportCore.cpp.

    Exposed API
    -----------
    Enum values
        CheckPt.NEVER      -- disable check-pointing
        CheckPt.ON_TIME    -- save periodically (default)
        CheckPt.ALWAYS     -- save after every state change
        CheckPt.UNDEFINED  -- sentinel; used as a default argument
    """

    # ------------------------------------------------------------------
    # Enum values
    # ------------------------------------------------------------------

    def test_never_is_accessible(self):
        """CheckPt.NEVER is a valid enum member."""
        _ = ecf.CheckPt.NEVER

    def test_on_time_is_accessible(self):
        """CheckPt.ON_TIME is a valid enum member."""
        _ = ecf.CheckPt.ON_TIME

    def test_always_is_accessible(self):
        """CheckPt.ALWAYS is a valid enum member."""
        _ = ecf.CheckPt.ALWAYS

    def test_undefined_is_accessible(self):
        """CheckPt.UNDEFINED is a valid enum member."""
        _ = ecf.CheckPt.UNDEFINED

    def test_all_four_values_are_distinct(self):
        """All four CheckPt values are mutually distinct."""
        values = [
            ecf.CheckPt.NEVER,
            ecf.CheckPt.ON_TIME,
            ecf.CheckPt.ALWAYS,
            ecf.CheckPt.UNDEFINED,
        ]
        for i, a in enumerate(values):
            for j, b in enumerate(values):
                if i != j:
                    self.assertNotEqual(a, b)

    # ------------------------------------------------------------------
    # __eq__ and __ne__
    # ------------------------------------------------------------------

    def test_eq_same_value(self):
        """A CheckPt member compares equal to itself."""
        self.assertEqual(ecf.CheckPt.ON_TIME, ecf.CheckPt.ON_TIME)

    def test_ne_different_values(self):
        """Different CheckPt members are not equal."""
        self.assertNotEqual(ecf.CheckPt.NEVER, ecf.CheckPt.ALWAYS)

    # ------------------------------------------------------------------
    # py_finalize_enum — str / repr / hash / .values / .names
    # ------------------------------------------------------------------

    def test_str_returns_member_name_only(self):
        """str(CheckPt.NEVER) returns 'NEVER', not 'CheckPt.NEVER'."""
        self.assertEqual(str(ecf.CheckPt.NEVER), "NEVER")
        self.assertEqual(str(ecf.CheckPt.ALWAYS), "ALWAYS")

    def test_repr_is_module_qualified(self):
        """repr(CheckPt.ON_TIME) returns 'ecflow.CheckPt.ON_TIME'."""
        self.assertEqual(repr(ecf.CheckPt.ON_TIME), "ecflow.CheckPt.ON_TIME")

    def test_hash_is_integer(self):
        """hash(CheckPt.NEVER) is a Python int."""
        self.assertIsInstance(hash(ecf.CheckPt.NEVER), int)

    def test_hash_equals_underlying_int_value(self):
        """hash(member) equals member.value."""
        self.assertEqual(hash(ecf.CheckPt.ON_TIME), ecf.CheckPt.ON_TIME.value)

    def test_values_dict_maps_int_to_member(self):
        """CheckPt.values is a dict mapping integer value -> enum member."""
        values = ecf.CheckPt.values
        self.assertIsInstance(values, dict)
        self.assertIn(ecf.CheckPt.NEVER.value, values)
        self.assertEqual(values[ecf.CheckPt.NEVER.value], ecf.CheckPt.NEVER)

    def test_names_dict_maps_str_to_member(self):
        """CheckPt.names is a dict mapping name string -> enum member."""
        names = ecf.CheckPt.names
        self.assertIsInstance(names, dict)
        self.assertIn("ALWAYS", names)
        self.assertEqual(names["ALWAYS"], ecf.CheckPt.ALWAYS)


class TestEcf(unittest.TestCase):
    """Tests for py::class_<Ecf, boost::noncopyable> exposed as ecf.Ecf in ExportCore.cpp.

    Exposed API
    -----------
    Constructors
        (no_init -- singleton, cannot be instantiated from Python)

    Static methods
        debug_equality()           -> bool  -- True if equality debugging is enabled
        set_debug_equality(bool)           -- enable or disable equality debugging
        debug_level()              -> int   -- current debug level (0 = off)
        set_debug_level(int)               -- set the debug level
    """

    def setUp(self):
        """Save current Ecf state to restore after each test."""
        self._orig_eq = ecf.Ecf.debug_equality()
        self._orig_lvl = ecf.Ecf.debug_level()

    def tearDown(self):
        """Restore Ecf state after each test."""
        ecf.Ecf.set_debug_equality(self._orig_eq)
        ecf.Ecf.set_debug_level(self._orig_lvl)

    # ------------------------------------------------------------------
    # Constructor: no_init
    # ------------------------------------------------------------------

    def test_direct_construction_raises(self):
        """Ecf cannot be instantiated from Python."""
        with self.assertRaises(RuntimeError):
            ecf.Ecf()

    # ------------------------------------------------------------------
    # debug_equality() and set_debug_equality()
    # ------------------------------------------------------------------

    def test_debug_equality_returns_bool(self):
        """debug_equality() returns a Python bool."""
        self.assertIsInstance(ecf.Ecf.debug_equality(), bool)

    def test_set_debug_equality_true_is_reflected(self):
        """set_debug_equality(True) causes debug_equality() to return True."""
        ecf.Ecf.set_debug_equality(True)
        self.assertTrue(ecf.Ecf.debug_equality())

    def test_set_debug_equality_false_is_reflected(self):
        """set_debug_equality(False) causes debug_equality() to return False."""
        ecf.Ecf.set_debug_equality(False)
        self.assertFalse(ecf.Ecf.debug_equality())

    def test_set_debug_equality_toggles(self):
        """set_debug_equality can be toggled between True and False."""
        ecf.Ecf.set_debug_equality(True)
        self.assertTrue(ecf.Ecf.debug_equality())
        ecf.Ecf.set_debug_equality(False)
        self.assertFalse(ecf.Ecf.debug_equality())

    def test_debug_equality_callable_on_class(self):
        """debug_equality() is a static method callable without an instance."""
        _ = ecf.Ecf.debug_equality()

    # ------------------------------------------------------------------
    # debug_level() and set_debug_level()
    # ------------------------------------------------------------------

    def test_debug_level_returns_int(self):
        """debug_level() returns a Python int."""
        self.assertIsInstance(ecf.Ecf.debug_level(), int)

    def test_set_debug_level_zero(self):
        """set_debug_level(0) turns off debug output."""
        ecf.Ecf.set_debug_level(0)
        self.assertEqual(ecf.Ecf.debug_level(), 0)

    def test_set_debug_level_one(self):
        """set_debug_level(1) sets debug level to 1."""
        ecf.Ecf.set_debug_level(1)
        self.assertEqual(ecf.Ecf.debug_level(), 1)

    def test_set_debug_level_arbitrary_positive(self):
        """set_debug_level accepts arbitrary positive integers."""
        ecf.Ecf.set_debug_level(5)
        self.assertEqual(ecf.Ecf.debug_level(), 5)

    def test_debug_level_callable_on_class(self):
        """debug_level() is a static method callable without an instance."""
        _ = ecf.Ecf.debug_level()


class TestState(unittest.TestCase):
    """Tests for py::enum_<NState::State> exposed as ecf.State in ExportCore.cpp.

    Exposed API
    -----------
    Enum values
        State.unknown    -- initial state when a definition is loaded
        State.complete   -- task has completed successfully
        State.queued     -- task is waiting to be submitted
        State.aborted    -- task has aborted
        State.submitted  -- task has been submitted
        State.active     -- task is currently running
    """

    # ------------------------------------------------------------------
    # Enum values
    # ------------------------------------------------------------------

    def test_unknown_is_accessible(self):
        """State.unknown is a valid enum member."""
        _ = ecf.State.unknown

    def test_complete_is_accessible(self):
        """State.complete is a valid enum member."""
        _ = ecf.State.complete

    def test_queued_is_accessible(self):
        """State.queued is a valid enum member."""
        _ = ecf.State.queued

    def test_aborted_is_accessible(self):
        """State.aborted is a valid enum member."""
        _ = ecf.State.aborted

    def test_submitted_is_accessible(self):
        """State.submitted is a valid enum member."""
        _ = ecf.State.submitted

    def test_active_is_accessible(self):
        """State.active is a valid enum member."""
        _ = ecf.State.active

    def test_all_six_values_are_distinct(self):
        """All six State values are mutually distinct."""
        values = [
            ecf.State.unknown,
            ecf.State.complete,
            ecf.State.queued,
            ecf.State.aborted,
            ecf.State.submitted,
            ecf.State.active,
        ]
        for i, a in enumerate(values):
            for j, b in enumerate(values):
                if i != j:
                    self.assertNotEqual(a, b)

    # ------------------------------------------------------------------
    # __eq__ and __ne__
    # ------------------------------------------------------------------

    def test_eq_same_value(self):
        """A State member compares equal to itself."""
        self.assertEqual(ecf.State.queued, ecf.State.queued)

    def test_ne_different_values(self):
        """Different State members are not equal."""
        self.assertNotEqual(ecf.State.queued, ecf.State.active)

    # ------------------------------------------------------------------
    # py_finalize_enum — str / repr / hash / .values / .names
    # ------------------------------------------------------------------

    def test_str_returns_member_name_only(self):
        """str(State.active) returns 'active', not 'State.active'."""
        self.assertEqual(str(ecf.State.active), "active")
        self.assertEqual(str(ecf.State.complete), "complete")

    def test_repr_is_module_qualified(self):
        """repr(State.active) returns 'ecflow.State.active'."""
        self.assertEqual(repr(ecf.State.active), "ecflow.State.active")

    def test_hash_is_integer(self):
        """hash(State.active) is a Python int."""
        self.assertIsInstance(hash(ecf.State.active), int)

    def test_hash_equals_underlying_int_value(self):
        """hash(member) equals member.value."""
        self.assertEqual(hash(ecf.State.active), ecf.State.active.value)

    def test_values_dict_maps_int_to_member(self):
        """State.values is a dict mapping integer value -> enum member."""
        values = ecf.State.values
        self.assertIsInstance(values, dict)
        self.assertIn(ecf.State.active.value, values)
        self.assertEqual(values[ecf.State.active.value], ecf.State.active)

    def test_names_dict_maps_str_to_member(self):
        """State.names is a dict mapping name string -> enum member."""
        names = ecf.State.names
        self.assertIsInstance(names, dict)
        self.assertIn("active", names)
        self.assertEqual(names["active"], ecf.State.active)


class TestDState(unittest.TestCase):
    """Tests for py::enum_<DState::State> exposed as ecf.DState in ExportCore.cpp.

    Exposed API
    -----------
    Enum values
        DState.unknown    -- initial state
        DState.complete   -- completed successfully
        DState.queued     -- waiting to be submitted
        DState.aborted    -- aborted
        DState.submitted  -- submitted
        DState.suspended  -- suspended by user (job generation blocked)
        DState.active     -- currently running

    Note
    ----
    DState extends State with the additional 'suspended' value, used by
    add_defstatus() and the defstatus attribute.
    """

    # ------------------------------------------------------------------
    # Enum values
    # ------------------------------------------------------------------

    def test_unknown_is_accessible(self):
        """DState.unknown is a valid enum member."""
        _ = ecf.DState.unknown

    def test_complete_is_accessible(self):
        """DState.complete is a valid enum member."""
        _ = ecf.DState.complete

    def test_queued_is_accessible(self):
        """DState.queued is a valid enum member."""
        _ = ecf.DState.queued

    def test_aborted_is_accessible(self):
        """DState.aborted is a valid enum member."""
        _ = ecf.DState.aborted

    def test_submitted_is_accessible(self):
        """DState.submitted is a valid enum member."""
        _ = ecf.DState.submitted

    def test_suspended_is_accessible(self):
        """DState.suspended is a valid enum member (absent from State)."""
        _ = ecf.DState.suspended

    def test_active_is_accessible(self):
        """DState.active is a valid enum member."""
        _ = ecf.DState.active

    def test_all_seven_values_are_distinct(self):
        """All seven DState values are mutually distinct."""
        values = [
            ecf.DState.unknown,
            ecf.DState.complete,
            ecf.DState.queued,
            ecf.DState.aborted,
            ecf.DState.submitted,
            ecf.DState.suspended,
            ecf.DState.active,
        ]
        for i, a in enumerate(values):
            for j, b in enumerate(values):
                if i != j:
                    self.assertNotEqual(a, b)

    # ------------------------------------------------------------------
    # __eq__ and __ne__
    # ------------------------------------------------------------------

    def test_eq_same_value(self):
        """A DState member compares equal to itself."""
        self.assertEqual(ecf.DState.complete, ecf.DState.complete)

    def test_ne_different_values(self):
        """Different DState members are not equal."""
        self.assertNotEqual(ecf.DState.queued, ecf.DState.suspended)

    def test_suspended_not_in_state_enum(self):
        """ecf.State does not expose a 'suspended' member (only DState does)."""
        self.assertFalse(hasattr(ecf.State, "suspended"))

    # ------------------------------------------------------------------
    # py_finalize_enum — str / repr / hash / .values / .names
    # ------------------------------------------------------------------

    def test_str_returns_member_name_only(self):
        """str(DState.active) returns 'active', not 'DState.active'."""
        self.assertEqual(str(ecf.DState.active), "active")
        self.assertEqual(str(ecf.DState.suspended), "suspended")

    def test_repr_is_module_qualified(self):
        """repr(DState.complete) returns 'ecflow.DState.complete'."""
        self.assertEqual(repr(ecf.DState.complete), "ecflow.DState.complete")

    def test_hash_is_integer(self):
        """hash(DState.queued) is a Python int."""
        self.assertIsInstance(hash(ecf.DState.queued), int)

    def test_hash_equals_underlying_int_value(self):
        """hash(member) equals member.value."""
        self.assertEqual(hash(ecf.DState.active), ecf.DState.active.value)

    def test_values_dict_maps_int_to_member(self):
        """DState.values is a dict mapping integer value -> enum member."""
        values = ecf.DState.values
        self.assertIsInstance(values, dict)
        self.assertIn(ecf.DState.active.value, values)
        self.assertEqual(values[ecf.DState.active.value], ecf.DState.active)

    def test_names_dict_maps_str_to_member(self):
        """DState.names is a dict mapping name string -> enum member."""
        names = ecf.DState.names
        self.assertIsInstance(names, dict)
        self.assertIn("suspended", names)
        self.assertEqual(names["suspended"], ecf.DState.suspended)


class TestDefstatus(unittest.TestCase):
    """Tests for py::class_<Defstatus> exposed as ecf.Defstatus in ExportCore.cpp.

    Exposed API
    -----------
    Constructors
        Defstatus(DState)  -- create from a DState enum value
        Defstatus(str)     -- create from a state name string (e.g. 'complete')

    Instance methods
        state() -> DState  -- the stored DState value

    Operators
        __str__  -- returns the state name string (e.g. 'complete')
    """

    # ------------------------------------------------------------------
    # Constructor: Defstatus(DState)
    # ------------------------------------------------------------------

    def test_ctor_dstate_complete(self):
        """Defstatus(DState.complete) stores the complete state."""
        ds = ecf.Defstatus(ecf.DState.complete)
        self.assertEqual(ds.state(), ecf.DState.complete)

    def test_ctor_dstate_queued(self):
        """Defstatus(DState.queued) stores the queued state."""
        ds = ecf.Defstatus(ecf.DState.queued)
        self.assertEqual(ds.state(), ecf.DState.queued)

    def test_ctor_dstate_aborted(self):
        """Defstatus(DState.aborted) stores the aborted state."""
        ds = ecf.Defstatus(ecf.DState.aborted)
        self.assertEqual(ds.state(), ecf.DState.aborted)

    def test_ctor_dstate_suspended(self):
        """Defstatus(DState.suspended) stores the suspended state."""
        ds = ecf.Defstatus(ecf.DState.suspended)
        self.assertEqual(ds.state(), ecf.DState.suspended)

    def test_ctor_dstate_unknown(self):
        """Defstatus(DState.unknown) stores the unknown state."""
        ds = ecf.Defstatus(ecf.DState.unknown)
        self.assertEqual(ds.state(), ecf.DState.unknown)

    def test_ctor_dstate_active(self):
        """Defstatus(DState.active) stores the active state."""
        ds = ecf.Defstatus(ecf.DState.active)
        self.assertEqual(ds.state(), ecf.DState.active)

    def test_ctor_dstate_submitted(self):
        """Defstatus(DState.submitted) stores the submitted state."""
        ds = ecf.Defstatus(ecf.DState.submitted)
        self.assertEqual(ds.state(), ecf.DState.submitted)

    # ------------------------------------------------------------------
    # Constructor: Defstatus(str)
    # ------------------------------------------------------------------

    def test_ctor_str_complete(self):
        """Defstatus('complete') stores DState.complete."""
        ds = ecf.Defstatus("complete")
        self.assertEqual(ds.state(), ecf.DState.complete)

    def test_ctor_str_queued(self):
        """Defstatus('queued') stores DState.queued."""
        ds = ecf.Defstatus("queued")
        self.assertEqual(ds.state(), ecf.DState.queued)

    def test_ctor_str_aborted(self):
        """Defstatus('aborted') stores DState.aborted."""
        ds = ecf.Defstatus("aborted")
        self.assertEqual(ds.state(), ecf.DState.aborted)

    def test_ctor_str_suspended(self):
        """Defstatus('suspended') stores DState.suspended."""
        ds = ecf.Defstatus("suspended")
        self.assertEqual(ds.state(), ecf.DState.suspended)

    def test_ctor_str_unknown(self):
        """Defstatus('unknown') stores DState.unknown."""
        ds = ecf.Defstatus("unknown")
        self.assertEqual(ds.state(), ecf.DState.unknown)

    def test_ctor_str_bad_name_raises(self):
        """Defstatus with an unrecognised string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Defstatus("not_a_valid_state")

    def test_ctor_str_empty_string_raises(self):
        """Defstatus('') raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Defstatus("")

    # ------------------------------------------------------------------
    # state()
    # ------------------------------------------------------------------

    def test_state_returns_dstate_value(self):
        """state() returns the DState enum value passed at construction."""
        self.assertEqual(
            ecf.Defstatus(ecf.DState.complete).state(), ecf.DState.complete
        )

    def test_state_from_str_and_dstate_agree(self):
        """Defstatus(DState.complete).state() == Defstatus('complete').state()."""
        self.assertEqual(
            ecf.Defstatus(ecf.DState.complete).state(),
            ecf.Defstatus("complete").state(),
        )

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_returns_state_name(self):
        """str(Defstatus(...)) returns the state name string."""
        self.assertEqual(str(ecf.Defstatus(ecf.DState.complete)), "complete")

    def test_str_queued(self):
        """str(Defstatus(DState.queued)) returns 'queued'."""
        self.assertEqual(str(ecf.Defstatus(ecf.DState.queued)), "queued")

    def test_str_is_consistent_with_ctor_str(self):
        """The string used in Defstatus(str) matches str(Defstatus(DState))."""
        for name in [
            "complete",
            "queued",
            "aborted",
            "suspended",
            "unknown",
            "active",
            "submitted",
        ]:
            ds = ecf.Defstatus(name)
            self.assertEqual(str(ds), name)


class TestSState(unittest.TestCase):
    """Tests for py::enum_<SState::State> exposed as ecf.SState in ExportCore.cpp.

    Exposed API
    -----------
    Enum values
        SState.HALTED    -- server is halted
        SState.SHUTDOWN  -- server is shutdown
        SState.RUNNING   -- server is running normally
    """

    # ------------------------------------------------------------------
    # Enum values
    # ------------------------------------------------------------------

    def test_halted_is_accessible(self):
        """SState.HALTED is a valid enum member."""
        _ = ecf.SState.HALTED

    def test_shutdown_is_accessible(self):
        """SState.SHUTDOWN is a valid enum member."""
        _ = ecf.SState.SHUTDOWN

    def test_running_is_accessible(self):
        """SState.RUNNING is a valid enum member."""
        _ = ecf.SState.RUNNING

    def test_all_three_values_are_distinct(self):
        """All three SState values are mutually distinct."""
        values = [ecf.SState.HALTED, ecf.SState.SHUTDOWN, ecf.SState.RUNNING]
        for i, a in enumerate(values):
            for j, b in enumerate(values):
                if i != j:
                    self.assertNotEqual(a, b)

    # ------------------------------------------------------------------
    # __eq__ and __ne__
    # ------------------------------------------------------------------

    def test_eq_same_value(self):
        """An SState member compares equal to itself."""
        self.assertEqual(ecf.SState.RUNNING, ecf.SState.RUNNING)

    def test_ne_different_values(self):
        """Different SState members are not equal."""
        self.assertNotEqual(ecf.SState.HALTED, ecf.SState.RUNNING)

    # ------------------------------------------------------------------
    # py_finalize_enum — str / repr / hash / .values / .names
    # ------------------------------------------------------------------

    def test_str_returns_member_name_only(self):
        """str(SState.RUNNING) returns 'RUNNING', not 'SState.RUNNING'."""
        self.assertEqual(str(ecf.SState.RUNNING), "RUNNING")
        self.assertEqual(str(ecf.SState.HALTED), "HALTED")

    def test_repr_is_module_qualified(self):
        """repr(SState.RUNNING) returns 'ecflow.SState.RUNNING'."""
        self.assertEqual(repr(ecf.SState.RUNNING), "ecflow.SState.RUNNING")

    def test_hash_is_integer(self):
        """hash(SState.RUNNING) is a Python int."""
        self.assertIsInstance(hash(ecf.SState.RUNNING), int)

    def test_hash_equals_underlying_int_value(self):
        """hash(member) equals member.value."""
        self.assertEqual(hash(ecf.SState.RUNNING), ecf.SState.RUNNING.value)

    def test_values_dict_maps_int_to_member(self):
        """SState.values is a dict mapping integer value -> enum member."""
        values = ecf.SState.values
        self.assertIsInstance(values, dict)
        self.assertIn(ecf.SState.RUNNING.value, values)
        self.assertEqual(values[ecf.SState.RUNNING.value], ecf.SState.RUNNING)

    def test_names_dict_maps_str_to_member(self):
        """SState.names is a dict mapping name string -> enum member."""
        names = ecf.SState.names
        self.assertIsInstance(names, dict)
        self.assertIn("RUNNING", names)
        self.assertEqual(names["RUNNING"], ecf.SState.RUNNING)


class TestTimeSlot(unittest.TestCase):
    """Tests for py::class_<ecf::TimeSlot> exposed as ecf.TimeSlot in ExportCore.cpp.

    Exposed API
    -----------
    Constructors
        TimeSlot(int hour, int minute)  -- create from hour (0-23) and minute (0-59)

    Instance methods
        hour()   -> int   -- the hour component
        minute() -> int   -- the minute component
        empty()  -> bool  -- True only for a NULL/unset slot (not for 00:00)

    Operators
        __str__   -- 'HH:MM' format; '+HH:MM' when relative
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __hash__  -- identity-based (boost.python C-extension type)

    Note
    ----
    Passing negative values for hour or minute triggers a C-level assertion abort
    and cannot be tested safely from Python.  High values (>23 or >59) are accepted
    without error by the C++ constructor.
    """

    # ------------------------------------------------------------------
    # Constructor: TimeSlot(int, int)
    # ------------------------------------------------------------------

    def test_ctor_stores_hour_and_minute(self):
        """TimeSlot(h, m) stores h and m verbatim."""
        ts = ecf.TimeSlot(10, 30)
        self.assertEqual(ts.hour(), 10)
        self.assertEqual(ts.minute(), 30)

    def test_ctor_midnight(self):
        """TimeSlot(0, 0) constructs successfully."""
        ts = ecf.TimeSlot(0, 0)
        self.assertEqual(ts.hour(), 0)
        self.assertEqual(ts.minute(), 0)

    def test_ctor_end_of_day(self):
        """TimeSlot(23, 59) constructs successfully."""
        ts = ecf.TimeSlot(23, 59)
        self.assertEqual(ts.hour(), 23)
        self.assertEqual(ts.minute(), 59)

    def test_ctor_hour_zero_minute_nonzero(self):
        """TimeSlot(0, 45) correctly stores 0 hours and 45 minutes."""
        ts = ecf.TimeSlot(0, 45)
        self.assertEqual(ts.hour(), 0)
        self.assertEqual(ts.minute(), 45)

    # ------------------------------------------------------------------
    # hour() and minute()
    # ------------------------------------------------------------------

    def test_hour_returns_int(self):
        """hour() returns a Python int."""
        self.assertIsInstance(ecf.TimeSlot(5, 0).hour(), int)

    def test_minute_returns_int(self):
        """minute() returns a Python int."""
        self.assertIsInstance(ecf.TimeSlot(0, 15).minute(), int)

    # ------------------------------------------------------------------
    # empty()
    # ------------------------------------------------------------------

    def test_empty_is_false_for_normal_slot(self):
        """empty() returns False for a normally constructed TimeSlot."""
        self.assertFalse(ecf.TimeSlot(10, 30).empty())

    def test_empty_is_false_for_midnight(self):
        """empty() returns False even for TimeSlot(0, 0); NULL means unset, not zero."""
        self.assertFalse(ecf.TimeSlot(0, 0).empty())

    def test_empty_is_true_for_null_slot_from_single_series_finish(self):
        """A NULL TimeSlot obtained from TimeSeries.finish() on a single slot reports empty."""
        single = ecf.TimeSeries(ecf.TimeSlot(10, 30))
        self.assertTrue(single.finish().empty())

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format_hh_mm(self):
        """str(TimeSlot) uses 'HH:MM' format with zero-padded fields."""
        self.assertEqual(str(ecf.TimeSlot(10, 30)), "10:30")

    def test_str_midnight(self):
        """str(TimeSlot(0, 0)) is '00:00'."""
        self.assertEqual(str(ecf.TimeSlot(0, 0)), "00:00")

    def test_str_single_digit_hour_is_zero_padded(self):
        """Single-digit hours are zero-padded in str output."""
        self.assertEqual(str(ecf.TimeSlot(5, 7)), "05:07")

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_value_equal(self):
        """copy.copy(TimeSlot) returns a value-equal instance."""
        ts = ecf.TimeSlot(10, 30)
        self.assertEqual(copy.copy(ts), ts)

    def test_copy_is_identity_distinct(self):
        """copy.copy(TimeSlot) returns a different object."""
        ts = ecf.TimeSlot(10, 30)
        self.assertIsNot(copy.copy(ts), ts)

    # ------------------------------------------------------------------
    # __eq__ and __ne__
    # ------------------------------------------------------------------

    def test_eq_same_values(self):
        """Two TimeSlots with the same hour and minute are equal."""
        self.assertEqual(ecf.TimeSlot(10, 30), ecf.TimeSlot(10, 30))

    def test_ne_different_hour(self):
        """TimeSlots differing only in hour are not equal."""
        self.assertNotEqual(ecf.TimeSlot(10, 30), ecf.TimeSlot(11, 30))

    def test_ne_different_minute(self):
        """TimeSlots differing only in minute are not equal."""
        self.assertNotEqual(ecf.TimeSlot(10, 30), ecf.TimeSlot(10, 31))

    def test_eq_with_itself(self):
        """A TimeSlot is equal to itself."""
        ts = ecf.TimeSlot(10, 30)
        self.assertEqual(ts, ts)

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_hash_is_identity_based(self):
        """Two value-equal TimeSlots kept simultaneously have different hashes."""
        ts1 = ecf.TimeSlot(10, 30)
        ts2 = ecf.TimeSlot(10, 30)
        self.assertEqual(ts1, ts2)
        self.assertNotEqual(hash(ts1), hash(ts2))

    def test_hash_same_object_is_stable(self):
        """The hash of a single TimeSlot is the same across repeated calls."""
        ts = ecf.TimeSlot(10, 30)
        self.assertEqual(hash(ts), hash(ts))

    def test_hash_usable_as_dict_key(self):
        """A TimeSlot can be used as a dictionary key."""
        ts = ecf.TimeSlot(10, 30)
        d = {ts: "value"}
        self.assertEqual(d[ts], "value")


class TestTimeSeries(unittest.TestCase):
    """Tests for py::class_<ecf::TimeSeries> exposed as ecf.TimeSeries in ExportCore.cpp.

    Exposed API
    -----------
    Constructors
        TimeSeries(TimeSlot, [bool])                        -- single slot; optional relative flag
        TimeSeries(int hour, int minute, [bool])            -- single slot from integers
        TimeSeries(TimeSlot start, TimeSlot finish,
                   TimeSlot incr, [bool])                   -- full series with optional relative flag

    Instance methods
        has_increment() -> bool      -- True for a series (start/finish/incr), False for single slot
        start()         -> TimeSlot  -- the start time
        finish()        -> TimeSlot  -- the finish time; returns a NULL (empty) TimeSlot for single slot
        incr()          -> TimeSlot  -- the increment; returns a NULL (empty) TimeSlot for single slot
        relative()      -> bool      -- True when relative to suite start

    Operators
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __str__   -- 'HH:MM' or '+HH:MM' for single; 'HH:MM HH:MM HH:MM' for series
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __hash__  -- identity-based (boost.python C-extension type)
    """

    def setUp(self):
        self.start = ecf.TimeSlot(0, 0)
        self.finish = ecf.TimeSlot(23, 0)
        self.incr = ecf.TimeSlot(1, 0)

    # ------------------------------------------------------------------
    # Constructor: TimeSeries(TimeSlot, [bool])
    # ------------------------------------------------------------------

    def test_ctor_single_slot(self):
        """TimeSeries(TimeSlot) creates a single-slot series."""
        ts = ecf.TimeSeries(ecf.TimeSlot(10, 30))
        self.assertFalse(ts.has_increment())

    def test_ctor_single_slot_start_is_stored(self):
        """TimeSeries(TimeSlot) stores the slot as the start time."""
        ts = ecf.TimeSeries(ecf.TimeSlot(10, 30))
        self.assertEqual(ts.start(), ecf.TimeSlot(10, 30))

    def test_ctor_single_slot_finish_is_null(self):
        """TimeSeries(TimeSlot) has an empty (NULL) finish slot."""
        ts = ecf.TimeSeries(ecf.TimeSlot(10, 30))
        self.assertTrue(ts.finish().empty())

    def test_ctor_single_slot_incr_is_null(self):
        """TimeSeries(TimeSlot) has an empty (NULL) increment slot."""
        ts = ecf.TimeSeries(ecf.TimeSlot(10, 30))
        self.assertTrue(ts.incr().empty())

    def test_ctor_single_slot_default_not_relative(self):
        """TimeSeries(TimeSlot) defaults to non-relative (absolute time)."""
        ts = ecf.TimeSeries(ecf.TimeSlot(10, 30))
        self.assertFalse(ts.relative())

    def test_ctor_single_slot_relative_true(self):
        """TimeSeries(TimeSlot, True) marks the series as relative."""
        ts = ecf.TimeSeries(ecf.TimeSlot(10, 30), True)
        self.assertTrue(ts.relative())

    def test_ctor_single_slot_relative_false(self):
        """TimeSeries(TimeSlot, False) explicitly marks the series as non-relative."""
        ts = ecf.TimeSeries(ecf.TimeSlot(10, 30), False)
        self.assertFalse(ts.relative())

    def test_ctor_single_slot_str_absolute(self):
        """str(TimeSeries(TimeSlot)) renders as 'HH:MM'."""
        ts = ecf.TimeSeries(ecf.TimeSlot(10, 30))
        self.assertEqual(str(ts), "10:30")

    def test_ctor_single_slot_str_relative(self):
        """str(TimeSeries(TimeSlot, True)) renders as '+HH:MM'."""
        ts = ecf.TimeSeries(ecf.TimeSlot(10, 30), True)
        self.assertEqual(str(ts), "+10:30")

    # ------------------------------------------------------------------
    # Constructor: TimeSeries(int, int, [bool])
    # ------------------------------------------------------------------

    def test_ctor_hour_minute(self):
        """TimeSeries(h, m) creates a single-slot series from integers."""
        ts = ecf.TimeSeries(10, 30)
        self.assertFalse(ts.has_increment())
        self.assertEqual(ts.start(), ecf.TimeSlot(10, 30))

    def test_ctor_hour_minute_default_not_relative(self):
        """TimeSeries(h, m) defaults to non-relative."""
        self.assertFalse(ecf.TimeSeries(10, 30).relative())

    def test_ctor_hour_minute_relative_true(self):
        """TimeSeries(h, m, True) marks the series as relative."""
        self.assertTrue(ecf.TimeSeries(10, 30, True).relative())

    def test_ctor_hour_minute_relative_false(self):
        """TimeSeries(h, m, False) explicitly marks the series as non-relative."""
        self.assertFalse(ecf.TimeSeries(10, 30, False).relative())

    def test_ctor_hour_minute_str(self):
        """str(TimeSeries(h, m)) renders as 'HH:MM'."""
        self.assertEqual(str(ecf.TimeSeries(10, 30)), "10:30")

    # ------------------------------------------------------------------
    # Constructor: TimeSeries(start, finish, incr, [bool])
    # ------------------------------------------------------------------

    def test_ctor_series_has_increment(self):
        """TimeSeries(start, finish, incr) creates a series with has_increment True."""
        ts = ecf.TimeSeries(self.start, self.finish, self.incr)
        self.assertTrue(ts.has_increment())

    def test_ctor_series_start_is_stored(self):
        """TimeSeries(start, finish, incr) stores the start slot."""
        ts = ecf.TimeSeries(self.start, self.finish, self.incr)
        self.assertEqual(ts.start(), self.start)

    def test_ctor_series_finish_is_stored(self):
        """TimeSeries(start, finish, incr) stores the finish slot."""
        ts = ecf.TimeSeries(self.start, self.finish, self.incr)
        self.assertEqual(ts.finish(), self.finish)

    def test_ctor_series_incr_is_stored(self):
        """TimeSeries(start, finish, incr) stores the increment slot."""
        ts = ecf.TimeSeries(self.start, self.finish, self.incr)
        self.assertEqual(ts.incr(), self.incr)

    def test_ctor_series_default_not_relative(self):
        """TimeSeries(start, finish, incr) defaults to non-relative."""
        ts = ecf.TimeSeries(self.start, self.finish, self.incr)
        self.assertFalse(ts.relative())

    def test_ctor_series_relative_true(self):
        """TimeSeries(start, finish, incr, True) marks the series as relative."""
        ts = ecf.TimeSeries(self.start, self.finish, self.incr, True)
        self.assertTrue(ts.relative())

    def test_ctor_series_relative_false(self):
        """TimeSeries(start, finish, incr, False) explicitly non-relative."""
        ts = ecf.TimeSeries(self.start, self.finish, self.incr, False)
        self.assertFalse(ts.relative())

    def test_ctor_series_str_absolute(self):
        """str(TimeSeries series) renders as 'HH:MM HH:MM HH:MM'."""
        ts = ecf.TimeSeries(self.start, self.finish, self.incr)
        self.assertEqual(str(ts), "00:00 23:00 01:00")

    def test_ctor_series_str_relative(self):
        """str(TimeSeries series, relative=True) renders with leading '+'."""
        ts = ecf.TimeSeries(self.start, self.finish, self.incr, True)
        self.assertEqual(str(ts), "+00:00 23:00 01:00")

    # ------------------------------------------------------------------
    # has_increment()
    # ------------------------------------------------------------------

    def test_has_increment_false_for_single(self):
        """has_increment() is False for a single-slot TimeSeries."""
        self.assertFalse(ecf.TimeSeries(ecf.TimeSlot(10, 0)).has_increment())

    def test_has_increment_true_for_series(self):
        """has_increment() is True for a start/finish/incr TimeSeries."""
        self.assertTrue(
            ecf.TimeSeries(self.start, self.finish, self.incr).has_increment()
        )

    # ------------------------------------------------------------------
    # relative()
    # ------------------------------------------------------------------

    def test_relative_false_by_default(self):
        """relative() defaults to False."""
        self.assertFalse(ecf.TimeSeries(ecf.TimeSlot(10, 0)).relative())

    def test_relative_true_when_set(self):
        """relative() returns True when the relative flag was set at construction."""
        self.assertTrue(ecf.TimeSeries(ecf.TimeSlot(10, 0), True).relative())

    # ------------------------------------------------------------------
    # __eq__ and __ne__
    # ------------------------------------------------------------------

    def test_eq_identical_single_slots(self):
        """Two single-slot TimeSeries with the same slot are equal."""
        self.assertEqual(
            ecf.TimeSeries(ecf.TimeSlot(10, 30)), ecf.TimeSeries(ecf.TimeSlot(10, 30))
        )

    def test_ne_different_slots(self):
        """Single-slot TimeSeries with different slots are not equal."""
        self.assertNotEqual(
            ecf.TimeSeries(ecf.TimeSlot(10, 30)), ecf.TimeSeries(ecf.TimeSlot(10, 31))
        )

    def test_eq_identical_series(self):
        """Two series TimeSeries with the same parameters are equal."""
        ts1 = ecf.TimeSeries(self.start, self.finish, self.incr)
        ts2 = ecf.TimeSeries(self.start, self.finish, self.incr)
        self.assertEqual(ts1, ts2)

    def test_ne_different_relative_flag(self):
        """TimeSeries objects differing only in the relative flag are not equal."""
        ts1 = ecf.TimeSeries(ecf.TimeSlot(10, 30), False)
        ts2 = ecf.TimeSeries(ecf.TimeSlot(10, 30), True)
        self.assertNotEqual(ts1, ts2)

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_value_equal(self):
        """copy.copy(TimeSeries) returns a value-equal instance."""
        ts = ecf.TimeSeries(self.start, self.finish, self.incr)
        self.assertEqual(copy.copy(ts), ts)

    def test_copy_is_identity_distinct(self):
        """copy.copy(TimeSeries) returns a different object."""
        ts = ecf.TimeSeries(self.start, self.finish, self.incr)
        self.assertIsNot(copy.copy(ts), ts)

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_hash_is_identity_based(self):
        """Two value-equal TimeSeries kept simultaneously have different hashes."""
        ts1 = ecf.TimeSeries(ecf.TimeSlot(10, 30))
        ts2 = ecf.TimeSeries(ecf.TimeSlot(10, 30))
        self.assertEqual(ts1, ts2)
        self.assertNotEqual(hash(ts1), hash(ts2))

    def test_hash_same_object_is_stable(self):
        """The hash of a single TimeSeries is the same across repeated calls."""
        ts = ecf.TimeSeries(ecf.TimeSlot(10, 30))
        self.assertEqual(hash(ts), hash(ts))


if __name__ == "__main__":
    unittest.main(verbosity=2)
