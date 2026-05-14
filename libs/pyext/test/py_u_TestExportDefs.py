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
import os
import tempfile
import unittest

import ecflow as ecf


class TestDefs(unittest.TestCase):
    """Tests for py::class_<Defs, defs_ptr> exposed as ecf.Defs in ExportDefs.cpp.

    Defs is the root container of an ecFlow suite definition.  It holds suites,
    user-defined server variables, and extern declarations.

    Exposed API
    -----------
    Constructors
        Defs()                        -- empty definition
        Defs(Suite, ...)              -- one or more suites passed positionally
        Defs(name=value, ...)         -- keyword arguments become user variables
        Defs(filename: str)           -- load a definition from a .def file on disk;
                                         mixing a path string with other arguments
                                         raises RuntimeError

    Instance methods
        add(Suite|dict|Edit|list, ...)        -- variadic add; kwargs become variables
        add_suite(suite_ptr)                  -- append a Suite object; returns the suite
        add_suite(name: str)                  -- create and append a Suite; returns it
        add_extern(path: str)                 -- declare an extern dependency path
        auto_add_externs(resolve: bool)       -- scan triggers and auto-declare externs
        add_variable(name, str)               -- add/update a user variable
        add_variable(name, int)               -- add/update a user variable (int coerced to str)
        add_variable(Variable)                -- add/update a user variable from a Variable
        add_variable(dict)                    -- add/update user variables from a mapping
        delete_variable(name: str)            -- remove a named user variable;
                                                 empty string removes all user variables
        sort_attributes(AttrType)             -- sort attributes of that type recursively
        sort_attributes(AttrType, bool)       -- with explicit recurse flag
        sort_attributes(AttrType, bool, list) -- with list of names to exclude from sorting
        sort_attributes(str, bool, list)      -- same but type given as a string;
                                                 unknown type string raises RuntimeError
        find_suite(name: str)        -> Suite | None
        find_abs_node(path: str)     -> Node | None
        find_node_path(type, name)   -> str   -- empty string if not found
        find_node(type, path)        -> Node | None
        get_all_nodes()              -> list[Node]
        get_all_tasks()              -> list[Task]
        has_time_dependencies()      -> bool
        check()                      -> str   -- empty on success; error message on failure
        check_job_creation(throw_on_error=False, verbose=False) -> str
        get_state()                  -> State
        get_server_state()           -> SState
        save_as_defs(filename)                -- write definition to file
        save_as_defs(filename, Style)         -- write with an explicit print style
        save_as_checkpt(filename)             -- write a checkpoint file (includes state)
        restore_from_checkpt(filename)        -- replace content from checkpoint file

    Properties (read-only iterables)
        suites           -- iterator over Suite objects
        externs          -- iterator over extern path strings
        user_variables   -- iterator over user-defined Variable objects
        server_variables -- iterator over built-in server Variable objects

    Operators
        __str__      -- text representation of the definition
        __copy__     -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__       -- value-based equality
        __ne__       -- implicit complement of __eq__
        __lt__       -- not exposed; raises TypeError
        __hash__     -- identity-based (boost.python C-extension type)
        __len__      -- number of top-level suites
        __contains__ -- True if a suite with the given name exists
        __iter__     -- iterate over top-level suites
        __getattr__  -- attribute lookup falls back to suites then user variables;
                        raises RuntimeError when neither is found
        __iadd__     -- defs += suite_or_list
        __add__      -- defs + suite
        __enter__    -- context manager: returns self
        __exit__     -- context manager: always returns False (does not suppress exceptions)
    """

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _simple_defs(self):
        """Return a Defs with one suite 's1' containing one task 't1'."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_task("t1")
        return d

    # ------------------------------------------------------------------
    # Constructor: Defs()
    # ------------------------------------------------------------------

    def test_ctor_empty_creates_instance(self):
        """Defs() creates an empty Defs instance."""
        self.assertIsInstance(ecf.Defs(), ecf.Defs)

    def test_ctor_empty_has_no_suites(self):
        """An empty Defs has zero suites."""
        self.assertEqual(len(ecf.Defs()), 0)

    def test_ctor_empty_str_contains_version(self):
        """str(Defs()) contains the ecFlow version header."""
        self.assertIn("#", str(ecf.Defs()))

    def test_ctor_empty_str_contains_enddef(self):
        """str(Defs()) contains the '# enddef' trailer."""
        self.assertIn("# enddef", str(ecf.Defs()))

    # ------------------------------------------------------------------
    # Constructor: Defs(Suite, ...)
    # ------------------------------------------------------------------

    def test_ctor_with_one_suite(self):
        """Defs(Suite) creates a Defs with one suite."""
        d = ecf.Defs(ecf.Suite("s1"))
        self.assertEqual(len(d), 1)

    def test_ctor_with_two_suites(self):
        """Defs(Suite, Suite) creates a Defs with two suites."""
        d = ecf.Defs(ecf.Suite("s1"), ecf.Suite("s2"))
        self.assertEqual(len(d), 2)

    def test_ctor_suite_name_preserved(self):
        """Suite name is preserved when passed to the constructor."""
        d = ecf.Defs(ecf.Suite("alpha"))
        self.assertIn("alpha", str(d))

    # ------------------------------------------------------------------
    # Constructor: Defs(name=value, ...)
    # ------------------------------------------------------------------

    def test_ctor_kwargs_become_user_variables(self):
        """Keyword arguments to Defs() are stored as user variables."""
        d = ecf.Defs(MY_VAR="hello")
        names = [v.name() for v in d.user_variables]
        self.assertIn("MY_VAR", names)

    def test_ctor_kwargs_value_preserved(self):
        """Keyword argument value is stored verbatim."""
        d = ecf.Defs(MY_VAR="hello")
        values = {v.name(): v.value() for v in d.user_variables}
        self.assertEqual(values["MY_VAR"], "hello")

    def test_ctor_multiple_kwargs(self):
        """Multiple keyword arguments each become user variables."""
        d = ecf.Defs(A="1", B="2")
        names = {v.name() for v in d.user_variables}
        self.assertIn("A", names)
        self.assertIn("B", names)

    # ------------------------------------------------------------------
    # Constructor: Defs(filename)
    # ------------------------------------------------------------------

    def test_ctor_from_file_loads_suites(self):
        """Defs(filename) loads suites from a .def file on disk."""
        src = ecf.Defs(ecf.Suite("loaded"))
        with tempfile.NamedTemporaryFile(suffix=".def", delete=False, mode="w") as f:
            f.write(str(src))
            fname = f.name
        try:
            d = ecf.Defs(fname)
            self.assertIn("loaded", [s.name() for s in d.suites])
        finally:
            os.unlink(fname)

    def test_ctor_nonexistent_file_raises(self):
        """Defs('/nonexistent') raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Defs("/nonexistent/path/that/does/not/exist.def")

    def test_ctor_mix_path_and_suite_raises(self):
        """Passing a filename string together with a Suite raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Defs("/some/path.def", ecf.Suite("s1"))

    # ------------------------------------------------------------------
    # add_suite
    # ------------------------------------------------------------------

    def test_add_suite_ptr_increments_len(self):
        """add_suite(suite_ptr) increments the suite count."""
        d = ecf.Defs()
        d.add_suite(ecf.Suite("s1"))
        self.assertEqual(len(d), 1)

    def test_add_suite_ptr_returns_suite(self):
        """add_suite(suite_ptr) returns the added Suite."""
        d = ecf.Defs()
        s = d.add_suite(ecf.Suite("s1"))
        self.assertIsInstance(s, ecf.Suite)

    def test_add_suite_str_creates_suite(self):
        """add_suite(name) creates a suite and returns it."""
        d = ecf.Defs()
        s = d.add_suite("new_suite")
        self.assertIsInstance(s, ecf.Suite)
        self.assertEqual(len(d), 1)

    def test_add_suite_str_name_preserved(self):
        """Suite added by name has the correct name."""
        d = ecf.Defs()
        s = d.add_suite("my_suite")
        self.assertEqual(s.name(), "my_suite")

    def test_add_multiple_suites(self):
        """Multiple add_suite calls accumulate suites."""
        d = ecf.Defs()
        d.add_suite("alpha")
        d.add_suite("beta")
        self.assertEqual(len(d), 2)

    # ------------------------------------------------------------------
    # add_extern / externs property
    # ------------------------------------------------------------------

    def test_add_extern_stored(self):
        """add_extern stores the path in the externs iterable."""
        d = ecf.Defs()
        d.add_extern("/suite/task:event")
        exts = list(d.externs)
        self.assertIn("/suite/task:event", exts)

    def test_add_multiple_externs(self):
        """Multiple add_extern calls accumulate."""
        d = ecf.Defs()
        d.add_extern("/a")
        d.add_extern("/b")
        self.assertEqual(len(list(d.externs)), 2)

    def test_externs_empty_on_fresh_defs(self):
        """A fresh Defs has no externs."""
        self.assertEqual(len(list(ecf.Defs().externs)), 0)

    # ------------------------------------------------------------------
    # auto_add_externs
    # ------------------------------------------------------------------

    def test_auto_add_externs_discovers_trigger_refs(self):
        """auto_add_externs(True) adds paths referenced in triggers."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        t.add_trigger("/other/task == complete")
        d.auto_add_externs(True)
        self.assertIn("/other/task", list(d.externs))

    def test_auto_add_externs_no_refs_no_externs(self):
        """auto_add_externs on a Defs with no external triggers adds nothing."""
        d = ecf.Defs()
        d.add_suite("s1")
        d.auto_add_externs(True)
        self.assertEqual(len(list(d.externs)), 0)

    # ------------------------------------------------------------------
    # add_variable / user_variables / delete_variable
    # ------------------------------------------------------------------

    def test_add_variable_str_stored(self):
        """add_variable(name, str) stores the variable."""
        d = ecf.Defs()
        d.add_variable("MY_VAR", "hello")
        names = [v.name() for v in d.user_variables]
        self.assertIn("MY_VAR", names)

    def test_add_variable_str_value(self):
        """add_variable(name, str) stores the correct value."""
        d = ecf.Defs()
        d.add_variable("MY_VAR", "hello")
        values = {v.name(): v.value() for v in d.user_variables}
        self.assertEqual(values["MY_VAR"], "hello")

    def test_add_variable_int_stored(self):
        """add_variable(name, int) coerces the int to a string."""
        d = ecf.Defs()
        d.add_variable("COUNT", 42)
        values = {v.name(): v.value() for v in d.user_variables}
        self.assertEqual(values["COUNT"], "42")

    def test_add_variable_variable_obj(self):
        """add_variable(Variable) stores the variable."""
        d = ecf.Defs()
        d.add_variable(ecf.Variable("X", "99"))
        names = [v.name() for v in d.user_variables]
        self.assertIn("X", names)

    def test_add_variable_dict(self):
        """add_variable(dict) stores all key/value pairs."""
        d = ecf.Defs()
        d.add_variable({"P": "1", "Q": "2"})
        names = {v.name() for v in d.user_variables}
        self.assertIn("P", names)
        self.assertIn("Q", names)

    def test_add_variable_updates_existing(self):
        """add_variable with an existing name updates the value."""
        d = ecf.Defs()
        d.add_variable("X", "old")
        d.add_variable("X", "new")
        values = {v.name(): v.value() for v in d.user_variables}
        self.assertEqual(values["X"], "new")

    def test_delete_variable_removes_named(self):
        """delete_variable(name) removes only the named variable."""
        d = ecf.Defs()
        d.add_variable("X", "1")
        d.add_variable("Y", "2")
        d.delete_variable("X")
        names = [v.name() for v in d.user_variables]
        self.assertNotIn("X", names)
        self.assertIn("Y", names)

    def test_delete_variable_empty_string_removes_all(self):
        """delete_variable('') removes all user variables."""
        d = ecf.Defs()
        d.add_variable("X", "1")
        d.add_variable("Y", "2")
        d.delete_variable("")
        self.assertEqual(len(list(d.user_variables)), 0)

    def test_user_variables_empty_on_fresh_defs(self):
        """A fresh Defs has no user variables."""
        self.assertEqual(len(list(ecf.Defs().user_variables)), 0)

    # ------------------------------------------------------------------
    # server_variables property
    # ------------------------------------------------------------------

    def test_server_variables_nonempty(self):
        """A fresh Defs has predefined server variables."""
        self.assertGreater(len(list(ecf.Defs().server_variables)), 0)

    def test_server_variables_contains_ecf_micro(self):
        """ECF_MICRO is among the predefined server variables."""
        svars = {v.name() for v in ecf.Defs().server_variables}
        self.assertIn("ECF_MICRO", svars)

    # ------------------------------------------------------------------
    # sort_attributes
    # ------------------------------------------------------------------

    def test_sort_attributes_by_attr_type(self):
        """sort_attributes(AttrType.variable) sorts variables alphabetically."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_variable("Z", "1")
        s.add_variable("A", "2")
        d.sort_attributes(ecf.AttrType.variable)
        names = [v.name() for v in s.variables]
        self.assertEqual(names, sorted(names))

    def test_sort_attributes_with_recurse_false_does_not_sort_child_vars(self):
        """sort_attributes(AttrType, False) does not recurse into suites; child variables remain unsorted."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_variable("Z", "1")
        s.add_variable("A", "2")
        d.sort_attributes(ecf.AttrType.variable, False)
        # recurse=False from Defs level means suite variables are NOT touched
        names = [v.name() for v in s.variables]
        self.assertEqual(names, ["Z", "A"])

    def test_sort_attributes_with_no_sort_list(self):
        """sort_attributes with a no-sort list excludes the listed names."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_variable("Z", "1")
        s.add_variable("A", "2")
        d.sort_attributes(ecf.AttrType.variable, True, [])
        names = [v.name() for v in s.variables]
        self.assertEqual(names, sorted(names))

    def test_sort_attributes_by_string_name(self):
        """sort_attributes('variable', ...) accepts the attribute type as a string."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_variable("Z", "1")
        s.add_variable("A", "2")
        d.sort_attributes("variable", True, [])
        names = [v.name() for v in s.variables]
        self.assertEqual(names, sorted(names))

    def test_sort_attributes_invalid_string_raises(self):
        """sort_attributes with an unknown attribute type string raises RuntimeError."""
        d = ecf.Defs()
        with self.assertRaises(RuntimeError):
            d.sort_attributes("not_a_real_attribute_type", True, [])

    # ------------------------------------------------------------------
    # find_suite
    # ------------------------------------------------------------------

    def test_find_suite_returns_suite(self):
        """find_suite returns the matching Suite."""
        d = ecf.Defs()
        d.add_suite("alpha")
        result = d.find_suite("alpha")
        self.assertIsInstance(result, ecf.Suite)

    def test_find_suite_name_matches(self):
        """find_suite returns a Suite with the queried name."""
        d = ecf.Defs()
        d.add_suite("alpha")
        self.assertEqual(d.find_suite("alpha").name(), "alpha")

    def test_find_suite_not_found_returns_none(self):
        """find_suite returns None when the name does not exist."""
        d = ecf.Defs()
        self.assertIsNone(d.find_suite("nonexistent"))

    def test_find_suite_empty_defs_returns_none(self):
        """find_suite on an empty Defs returns None."""
        self.assertIsNone(ecf.Defs().find_suite("any"))

    # ------------------------------------------------------------------
    # find_abs_node
    # ------------------------------------------------------------------

    def test_find_abs_node_task(self):
        """find_abs_node('/s1/f1/t1') returns the Task node."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        f = s.add_family("f1")
        f.add_task("t1")
        node = d.find_abs_node("/s1/f1/t1")
        self.assertIsNotNone(node)
        self.assertEqual(node.name(), "t1")

    def test_find_abs_node_family(self):
        """find_abs_node finds a Family node."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_family("f1")
        node = d.find_abs_node("/s1/f1")
        self.assertIsNotNone(node)
        self.assertEqual(node.name(), "f1")

    def test_find_abs_node_suite(self):
        """find_abs_node finds a Suite node."""
        d = ecf.Defs()
        d.add_suite("s1")
        node = d.find_abs_node("/s1")
        self.assertIsNotNone(node)
        self.assertEqual(node.name(), "s1")

    def test_find_abs_node_not_found_returns_none(self):
        """find_abs_node returns None for a non-existent path."""
        d = ecf.Defs()
        d.add_suite("s1")
        self.assertIsNone(d.find_abs_node("/s1/nonexistent"))

    # ------------------------------------------------------------------
    # find_node_path
    # ------------------------------------------------------------------

    def test_find_node_path_task(self):
        """find_node_path('task', 't1') returns the full path."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        f = s.add_family("f1")
        f.add_task("t1")
        self.assertEqual(d.find_node_path("task", "t1"), "/s1/f1/t1")

    def test_find_node_path_family(self):
        """find_node_path('family', 'f1') returns the full path."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_family("f1")
        self.assertEqual(d.find_node_path("family", "f1"), "/s1/f1")

    def test_find_node_path_not_found_returns_empty(self):
        """find_node_path returns an empty string when not found."""
        d = ecf.Defs()
        d.add_suite("s1")
        self.assertEqual(d.find_node_path("task", "nonexistent"), "")

    # ------------------------------------------------------------------
    # find_node
    # ------------------------------------------------------------------

    def test_find_node_task(self):
        """find_node('task', '/s1/f1/t1') returns the Task."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        f = s.add_family("f1")
        f.add_task("t1")
        node = d.find_node("task", "/s1/f1/t1")
        self.assertIsNotNone(node)
        self.assertEqual(node.name(), "t1")

    def test_find_node_family(self):
        """find_node('family', path) returns the Family."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_family("f1")
        node = d.find_node("family", "/s1/f1")
        self.assertIsNotNone(node)
        self.assertEqual(node.name(), "f1")

    # ------------------------------------------------------------------
    # get_all_nodes / get_all_tasks
    # ------------------------------------------------------------------

    def test_get_all_nodes_includes_all_levels(self):
        """get_all_nodes returns nodes at every level (suite, family, task)."""
        d = self._simple_defs()
        s = d.find_suite("s1")
        s.add_family("f1").add_task("t2")
        names = {n.name() for n in d.get_all_nodes()}
        self.assertIn("s1", names)
        self.assertIn("f1", names)
        self.assertIn("t1", names)
        self.assertIn("t2", names)

    def test_get_all_nodes_empty_defs(self):
        """get_all_nodes on an empty Defs returns an empty list."""
        self.assertEqual(
            d.get_all_nodes() if False else list(ecf.Defs().get_all_nodes()), []
        )

    def test_get_all_tasks_only_tasks(self):
        """get_all_tasks returns only Task nodes."""
        d = self._simple_defs()
        s = d.find_suite("s1")
        s.add_family("f1")
        tasks = d.get_all_tasks()
        self.assertEqual(len(tasks), 1)
        self.assertEqual(tasks[0].name(), "t1")

    def test_get_all_tasks_multiple(self):
        """get_all_tasks returns all tasks across all levels."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        s.add_task("t1")
        s.add_task("t2")
        self.assertEqual(len(d.get_all_tasks()), 2)

    def test_get_all_tasks_empty_defs(self):
        """get_all_tasks on an empty Defs returns an empty list."""
        self.assertEqual(list(ecf.Defs().get_all_tasks()), [])

    # ------------------------------------------------------------------
    # has_time_dependencies
    # ------------------------------------------------------------------

    def test_has_time_dependencies_false_when_none(self):
        """has_time_dependencies() is False when no time attributes exist."""
        self.assertFalse(self._simple_defs().has_time_dependencies())

    def test_has_time_dependencies_true_with_time(self):
        """has_time_dependencies() is True after adding a time attribute."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        t.add_time("08:00")
        self.assertTrue(d.has_time_dependencies())

    # ------------------------------------------------------------------
    # check
    # ------------------------------------------------------------------

    def test_check_valid_defs_returns_empty(self):
        """check() returns an empty string for a structurally valid Defs."""
        d = ecf.Defs()
        d.add_suite("s1")
        self.assertEqual(d.check(), "")

    def test_check_invalid_trigger_returns_error(self):
        """check() returns a non-empty error string when a trigger references a missing node."""
        d = ecf.Defs()
        s = d.add_suite("s1")
        t = s.add_task("t1")
        t.add_trigger("/nonexistent == complete")
        self.assertNotEqual(d.check(), "")

    # ------------------------------------------------------------------
    # check_job_creation
    # ------------------------------------------------------------------

    def test_check_job_creation_returns_string(self):
        """check_job_creation() always returns a string."""
        d = self._simple_defs()
        self.assertIsInstance(d.check_job_creation(), str)

    def test_check_job_creation_no_error_flag(self):
        """check_job_creation(throw_on_error=False) does not raise on missing scripts."""
        d = self._simple_defs()
        result = d.check_job_creation(throw_on_error=False)
        self.assertIsInstance(result, str)

    def test_check_job_creation_verbose_no_raise(self):
        """check_job_creation(verbose=True) runs without raising."""
        d = self._simple_defs()
        result = d.check_job_creation(throw_on_error=False, verbose=True)
        self.assertIsInstance(result, str)

    # ------------------------------------------------------------------
    # get_state / get_server_state
    # ------------------------------------------------------------------

    def test_get_state_returns_state_type(self):
        """get_state() returns an ecf.State value."""
        self.assertIsInstance(ecf.Defs().get_state(), ecf.State)

    def test_get_state_fresh_defs_is_unknown(self):
        """A fresh Defs has state 'unknown'."""
        self.assertEqual(ecf.Defs().get_state(), ecf.State.unknown)

    def test_get_server_state_returns_sstate_type(self):
        """get_server_state() returns an ecf.SState value."""
        self.assertIsInstance(ecf.Defs().get_server_state(), ecf.SState)

    def test_get_server_state_fresh_defs_is_running(self):
        """A fresh Defs reports server state RUNNING."""
        self.assertEqual(ecf.Defs().get_server_state(), ecf.SState.RUNNING)

    # ------------------------------------------------------------------
    # save_as_defs / save_as_checkpt / restore_from_checkpt
    # ------------------------------------------------------------------

    def test_save_as_defs_creates_file(self):
        """save_as_defs(filename) creates a file on disk."""
        d = self._simple_defs()
        with tempfile.NamedTemporaryFile(suffix=".def", delete=False) as f:
            fname = f.name
        try:
            d.save_as_defs(fname)
            self.assertTrue(os.path.exists(fname))
        finally:
            os.unlink(fname)

    def test_save_as_defs_content_is_readable(self):
        """The file written by save_as_defs is readable as a plain-text definition."""
        d = self._simple_defs()
        with tempfile.NamedTemporaryFile(suffix=".def", delete=False) as f:
            fname = f.name
        try:
            d.save_as_defs(fname)
            content = open(fname).read()
            self.assertIn("suite s1", content)
        finally:
            os.unlink(fname)

    def test_save_as_defs_with_style_defs(self):
        """save_as_defs(filename, Style.DEFS) writes a valid definition file."""
        d = self._simple_defs()
        with tempfile.NamedTemporaryFile(suffix=".def", delete=False) as f:
            fname = f.name
        try:
            d.save_as_defs(fname, ecf.Style.DEFS)
            content = open(fname).read()
            self.assertIn("suite s1", content)
        finally:
            os.unlink(fname)

    def test_save_as_defs_roundtrip(self):
        """A Defs written by save_as_defs can be reloaded with Defs(filename)."""
        d = self._simple_defs()
        with tempfile.NamedTemporaryFile(suffix=".def", delete=False) as f:
            fname = f.name
        try:
            d.save_as_defs(fname)
            d2 = ecf.Defs(fname)
            self.assertEqual([s.name() for s in d2.suites], ["s1"])
        finally:
            os.unlink(fname)

    def test_save_as_checkpt_creates_file(self):
        """save_as_checkpt(filename) creates a checkpoint file."""
        d = self._simple_defs()
        with tempfile.NamedTemporaryFile(suffix=".check", delete=False) as f:
            cname = f.name
        try:
            d.save_as_checkpt(cname)
            self.assertTrue(os.path.exists(cname))
        finally:
            os.unlink(cname)

    def test_restore_from_checkpt_recovers_suites(self):
        """restore_from_checkpt loads the suite names saved in a checkpoint."""
        d = self._simple_defs()
        with tempfile.NamedTemporaryFile(suffix=".check", delete=False) as f:
            cname = f.name
        try:
            d.save_as_checkpt(cname)
            d2 = ecf.Defs()
            d2.restore_from_checkpt(cname)
            self.assertIn("s1", [s.name() for s in d2.suites])
        finally:
            os.unlink(cname)

    # ------------------------------------------------------------------
    # add (variadic raw_function)
    # ------------------------------------------------------------------

    def test_add_single_suite(self):
        """add(suite) appends the suite."""
        d = ecf.Defs()
        d.add(ecf.Suite("s1"))
        self.assertEqual(len(d), 1)

    def test_add_two_suites_positional(self):
        """add(suite, suite) appends both suites."""
        d = ecf.Defs()
        d.add(ecf.Suite("s1"), ecf.Suite("s2"))
        self.assertEqual(len(d), 2)

    def test_add_kwarg_variable(self):
        """add(name=value) stores the keyword argument as a user variable."""
        d = ecf.Defs()
        d.add(MY_VAR="val")
        names = [v.name() for v in d.user_variables]
        self.assertIn("MY_VAR", names)

    def test_add_dict(self):
        """add({'key': 'value'}) stores the variable."""
        d = ecf.Defs()
        d.add({"MYKEY": "myval"})
        names = [v.name() for v in d.user_variables]
        self.assertIn("MYKEY", names)

    def test_add_variable_obj(self):
        """add(Variable) stores the variable."""
        d = ecf.Defs()
        d.add(ecf.Variable("X", "42"))
        names = [v.name() for v in d.user_variables]
        self.assertIn("X", names)

    def test_add_returns_defs(self):
        """add() returns the Defs instance (enables chaining)."""
        d = ecf.Defs()
        result = d.add(ecf.Suite("s1"))
        self.assertIsInstance(result, ecf.Defs)

    def test_add_none_is_ignored(self):
        """add(None) is silently ignored; suite count unchanged."""
        d = ecf.Defs()
        d.add(None)
        self.assertEqual(len(d), 0)

    # ------------------------------------------------------------------
    # __iadd__
    # ------------------------------------------------------------------

    def test_iadd_list_of_suites(self):
        """defs += [Suite, Suite] appends both suites."""
        d = ecf.Defs()
        d += [ecf.Suite("s1"), ecf.Suite("s2")]
        self.assertEqual(len(d), 2)

    def test_iadd_single_suite(self):
        """defs += Suite appends the suite."""
        d = ecf.Defs()
        d += ecf.Suite("s1")
        self.assertEqual(len(d), 1)

    def test_iadd_variable(self):
        """defs += [Variable] stores the variable."""
        d = ecf.Defs()
        d += [ecf.Variable("V", "val")]
        names = [v.name() for v in d.user_variables]
        self.assertIn("V", names)

    # ------------------------------------------------------------------
    # __add__
    # ------------------------------------------------------------------

    def test_add_operator_appends_suite(self):
        """defs + Suite appends the suite and returns Defs."""
        d = ecf.Defs()
        d = d + ecf.Suite("s1")
        self.assertEqual(len(d), 1)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_empty_defs(self):
        """str(Defs()) includes the version header and enddef marker."""
        s = str(ecf.Defs())
        self.assertIn("# enddef", s)

    def test_str_with_suite(self):
        """str(Defs) includes the suite block."""
        d = ecf.Defs()
        d.add_suite("s1")
        self.assertIn("suite s1", str(d))

    def test_str_user_variables_not_in_output(self):
        """User variables on Defs are server-side state and do not appear in str()."""
        d = ecf.Defs()
        d.add_variable("MY_VAR", "hello")
        # user_variables are accessible via the property but not serialised into str()
        self.assertNotIn("MY_VAR", str(d))
        self.assertIn("MY_VAR", [v.name() for v in d.user_variables])

    # ------------------------------------------------------------------
    # __len__
    # ------------------------------------------------------------------

    def test_len_empty(self):
        """len(Defs()) == 0."""
        self.assertEqual(len(ecf.Defs()), 0)

    def test_len_one_suite(self):
        """len increases to 1 after adding a suite."""
        d = ecf.Defs()
        d.add_suite("s1")
        self.assertEqual(len(d), 1)

    def test_len_two_suites(self):
        """len reflects the correct count after adding two suites."""
        d = ecf.Defs()
        d.add_suite("s1")
        d.add_suite("s2")
        self.assertEqual(len(d), 2)

    # ------------------------------------------------------------------
    # __contains__
    # ------------------------------------------------------------------

    def test_contains_existing_suite(self):
        """'name' in defs is True when the suite exists."""
        d = ecf.Defs()
        d.add_suite("s1")
        self.assertIn("s1", d)

    def test_contains_missing_suite(self):
        """'name' in defs is False when the suite does not exist."""
        d = ecf.Defs()
        self.assertNotIn("nonexistent", d)

    def test_not_contains_after_no_suites(self):
        """An empty Defs does not contain any name."""
        self.assertNotIn("s1", ecf.Defs())

    # ------------------------------------------------------------------
    # __iter__
    # ------------------------------------------------------------------

    def test_iter_yields_suites(self):
        """Iterating over Defs yields Suite objects."""
        d = ecf.Defs()
        d.add_suite("alpha")
        d.add_suite("beta")
        items = list(d)
        self.assertTrue(all(isinstance(s, ecf.Suite) for s in items))

    def test_iter_yields_in_insertion_order(self):
        """Iteration yields suites in insertion order."""
        d = ecf.Defs()
        d.add_suite("alpha")
        d.add_suite("beta")
        names = [s.name() for s in d]
        self.assertEqual(names, ["alpha", "beta"])

    def test_iter_empty_defs(self):
        """Iterating an empty Defs yields nothing."""
        self.assertEqual(list(ecf.Defs()), [])

    # ------------------------------------------------------------------
    # suites property
    # ------------------------------------------------------------------

    def test_suites_property_is_iterable(self):
        """The suites property is iterable."""
        d = ecf.Defs()
        d.add_suite("s1")
        self.assertGreater(len(list(d.suites)), 0)

    def test_suites_property_names(self):
        """The suites property yields Suite objects with correct names."""
        d = ecf.Defs()
        d.add_suite("s1")
        d.add_suite("s2")
        names = [s.name() for s in d.suites]
        self.assertIn("s1", names)
        self.assertIn("s2", names)

    # ------------------------------------------------------------------
    # __getattr__
    # ------------------------------------------------------------------

    def test_getattr_returns_suite(self):
        """Accessing defs.suite_name returns the Suite."""
        d = ecf.Defs()
        d.add_suite("myS")
        self.assertIsInstance(d.myS, ecf.Suite)

    def test_getattr_suite_name_matches(self):
        """The Suite returned by attribute access has the correct name."""
        d = ecf.Defs()
        d.add_suite("myS")
        self.assertEqual(d.myS.name(), "myS")

    def test_getattr_returns_variable(self):
        """Accessing defs.VAR_NAME returns the Variable when no suite of that name exists."""
        d = ecf.Defs()
        d.add_variable("MY_VAR", "42")
        result = d.MY_VAR
        self.assertIsInstance(result, ecf.Variable)

    def test_getattr_nonexistent_raises(self):
        """Accessing a non-existent attribute raises RuntimeError."""
        d = ecf.Defs()
        with self.assertRaises(RuntimeError):
            _ = d.nonexistent_attr

    # ------------------------------------------------------------------
    # Context manager (__enter__ / __exit__)
    # ------------------------------------------------------------------

    def test_context_manager_returns_defs(self):
        """The 'with Defs()' statement binds the Defs to the 'as' target."""
        with ecf.Defs() as d:
            self.assertIsInstance(d, ecf.Defs)

    def test_context_manager_mutations_persist(self):
        """Mutations inside a 'with' block are visible after the block exits."""
        with ecf.Defs() as d:
            d.add_suite("s_ctx")
        self.assertEqual(len(d), 1)

    def test_context_manager_does_not_suppress_exceptions(self):
        """Exceptions raised inside a 'with Defs()' block are not suppressed."""
        with self.assertRaises(ValueError):
            with ecf.Defs():
                raise ValueError("test error")

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_two_empty_defs(self):
        """Two empty Defs instances are equal."""
        self.assertEqual(ecf.Defs(), ecf.Defs())

    def test_eq_same_suite(self):
        """Two Defs with identical suites are equal."""
        d1 = ecf.Defs()
        d1.add_suite("s1")
        d2 = ecf.Defs()
        d2.add_suite("s1")
        self.assertEqual(d1, d2)

    def test_eq_different_suites(self):
        """Two Defs with different suite names are not equal."""
        d1 = ecf.Defs()
        d1.add_suite("s1")
        d2 = ecf.Defs()
        d2.add_suite("s2")
        self.assertNotEqual(d1, d2)

    def test_eq_one_has_suite_other_does_not(self):
        """A Defs with a suite is not equal to an empty Defs."""
        d1 = ecf.Defs()
        d1.add_suite("s1")
        self.assertNotEqual(d1, ecf.Defs())

    def test_eq_reflexive(self):
        """A Defs is equal to itself."""
        d = ecf.Defs()
        self.assertEqual(d, d)

    def test_ne_different_suites(self):
        """__ne__ returns True when suite names differ."""
        d1 = ecf.Defs()
        d1.add_suite("s1")
        d2 = ecf.Defs()
        d2.add_suite("s2")
        self.assertTrue(d1 != d2)

    def test_ne_same_defs(self):
        """__ne__ returns False for structurally identical Defs."""
        d1 = ecf.Defs()
        d1.add_suite("s1")
        d2 = ecf.Defs()
        d2.add_suite("s1")
        self.assertFalse(d1 != d2)

    # ------------------------------------------------------------------
    # __lt__  (not exposed)
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """Defs does not support < comparison."""
        d1 = ecf.Defs()
        d2 = ecf.Defs()
        with self.assertRaises(TypeError):
            _ = d1 < d2

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Defs is hashable."""
        self.assertIsInstance(hash(ecf.Defs()), int)

    def test_same_object_same_hash(self):
        """The same Defs object always returns the same hash."""
        d = ecf.Defs()
        self.assertEqual(hash(d), hash(d))

    def test_hash_is_identity_based(self):
        """Two value-equal Defs objects have different hashes."""
        d1 = ecf.Defs()
        d2 = ecf.Defs()
        self.assertEqual(d1, d2)
        self.assertNotEqual(hash(d1), hash(d2))

    def test_can_be_stored_in_set(self):
        """Defs instances can be stored in a Python set."""
        d1 = ecf.Defs()
        d1.add_suite("s1")
        d2 = ecf.Defs()
        d2.add_suite("s2")
        self.assertEqual(len({d1, d2}), 2)

    def test_can_be_used_as_dict_key(self):
        """Defs instances can be used as dictionary keys."""
        d = ecf.Defs()
        mapping = {d: "defs"}
        self.assertEqual(mapping[d], "defs")

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_returns_equal(self):
        """copy.copy(Defs) returns an equal Defs."""
        d = ecf.Defs()
        d.add_suite("s1")
        self.assertEqual(copy.copy(d), d)

    def test_copy_is_independent(self):
        """copy.copy(Defs) returns a distinct object."""
        d = ecf.Defs()
        d.add_suite("s1")
        self.assertIsNot(copy.copy(d), d)

    def test_copy_preserves_suites(self):
        """copy.copy preserves the suite list."""
        d = ecf.Defs()
        d.add_suite("s1")
        c = copy.copy(d)
        self.assertIn("s1", [s.name() for s in c.suites])

    def test_copy_preserves_user_variables(self):
        """copy.copy preserves user variables."""
        d = ecf.Defs()
        d.add_variable("MY_VAR", "hello")
        c = copy.copy(d)
        names = [v.name() for v in c.user_variables]
        self.assertIn("MY_VAR", names)

    def test_copy_preserves_str(self):
        """copy.copy(Defs) preserves str()."""
        d = ecf.Defs()
        d.add_suite("s1")
        self.assertEqual(str(copy.copy(d)), str(d))


if __name__ == "__main__":
    unittest.main(verbosity=2)
