#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

import unittest

import ecflow as ecf


class TestClient(unittest.TestCase):
    """Tests for py::class_<ClientInvoker, ...> as exposed in ExportClient.cpp.

    The class is registered under the Python name ``Client``.
    All tests here are pure unit tests, as such, they do not require a running ecFlow server.
    Integration tests that rely on a live server are in py_s_TestClientApi.py.

    Exposed API (unit-testable subset)
    -----------------------------------
    Constructors
        Client()                             -- default; connects to localhost:3141
        Client(host_port: str)               -- "host:port" or "host@port" format
        Client(host: str, port: str)         -- explicit host + port strings
        Client(host: str, port: int)         -- explicit host + port as integer

    Configuration methods (no network I/O)
        set_host_port(host: str, port: str)  -- three overloads:
        set_host_port(host: str, port: int)  --   int port is converted to str
        set_host_port(host_port: str)        --   "host:port" or "host@port"
        get_host() -> str                    -- returns the configured host
        get_port() -> str                    -- returns the configured port
        set_user_name(name: str)             -- sets the authenticating user name
        set_retry_connection_period(secs: int) -- retry interval between attempts
        set_connection_attempts(n: int)      -- max attempts (clamped to >= 1)
        set_auto_sync(enable: bool)          -- toggle automatic sync after calls
        is_auto_sync_enabled() -> bool       -- query the auto-sync flag
        debug(flag: bool)                    -- toggle verbose client debug output
        enable_http()                        -- switch transport to HTTP
        enable_https()                       -- switch transport to HTTPS
        version() -> str                     -- local library version string
        reset()                              -- clear local defs + handle number
        in_sync() -> bool                    -- True if local defs == server defs
        get_defs() -> defs_ptr or None       -- local copy of the definition tree
        ch_handle() -> int                   -- registered client-handle (0 = none)
        wait_for_server_reply(timeout: int = 60) -> bool  -- False on timeout

    Child-command configuration (no network I/O)
        set_child_path(path: str)
        set_child_password(pwd: str)
        set_child_pid(pid: str)          -- string overload
        set_child_pid(pid: int)          -- int overload (converts to string)
        set_child_try_no(n: int)
        set_child_timeout(secs: int)
        set_child_init_add_vars(d: dict) -- dict overload
        set_child_init_add_vars(l: list) -- list-of-Variable overload
        set_child_complete_del_vars(l: list)
        set_zombie_child_timeout(secs: int)

    Iteration
        changed_node_paths               -- range; empty before first sync

    Context manager
        __enter__() -> Client            -- returns self
        __exit__(...)  -> False          -- drops handle; exceptions are re-raised

    Operators / implicit protocol
        __eq__                           -- identity-based (C++ noncopyable)
        __ne__                           -- identity-based complement
        __hash__                         -- identity-based (boost.python preserves
                                            tp_hash on noncopyable extension types)
    """

    # ------------------------------------------------------------------
    # Constructors
    # ------------------------------------------------------------------

    def test_default_constructor_gives_localhost_3141(self):
        """Client() defaults to localhost:3141."""
        ci = ecf.Client()
        self.assertEqual(ci.get_host(), "localhost")
        self.assertEqual(ci.get_port(), "3141")

    def test_constructor_host_colon_port_string(self):
        """Client('host:port') parses both components correctly."""
        ci = ecf.Client("myhost:4444")
        self.assertEqual(ci.get_host(), "myhost")
        self.assertEqual(ci.get_port(), "4444")

    def test_constructor_host_at_port_string(self):
        """Client('host@port') is also a valid combined form."""
        ci = ecf.Client("myhost@5555")
        self.assertEqual(ci.get_host(), "myhost")
        self.assertEqual(ci.get_port(), "5555")

    def test_constructor_separate_host_and_port_strings(self):
        """Client(host, port_str) sets host and port individually."""
        ci = ecf.Client("srv1", "6000")
        self.assertEqual(ci.get_host(), "srv1")
        self.assertEqual(ci.get_port(), "6000")

    def test_constructor_separate_host_and_port_int(self):
        """Client(host, port_int) accepts an integer port and converts it."""
        ci = ecf.Client("srv2", 7777)
        self.assertEqual(ci.get_host(), "srv2")
        self.assertEqual(ci.get_port(), "7777")

    # ------------------------------------------------------------------
    # get_host / get_port
    # ------------------------------------------------------------------

    def test_get_host_returns_str(self):
        """get_host() always returns a Python str."""
        ci = ecf.Client()
        self.assertIsInstance(ci.get_host(), str)

    def test_get_port_returns_str(self):
        """get_port() always returns a Python str."""
        ci = ecf.Client()
        self.assertIsInstance(ci.get_port(), str)

    # ------------------------------------------------------------------
    # set_host_port — three overloads
    # ------------------------------------------------------------------

    def test_set_host_port_str_str_overload(self):
        """set_host_port(host, port_str) sets host and port."""
        ci = ecf.Client()
        ci.set_host_port("testhost", "4242")
        self.assertEqual(ci.get_host(), "testhost")
        self.assertEqual(ci.get_port(), "4242")

    def test_set_host_port_str_int_overload(self):
        """set_host_port(host, port_int) accepts an integer port."""
        ci = ecf.Client()
        ci.set_host_port("inthost", 9999)
        self.assertEqual(ci.get_host(), "inthost")
        self.assertEqual(ci.get_port(), "9999")

    def test_set_host_port_colon_combined_string(self):
        """set_host_port('host:port') parses both components from one string."""
        ci = ecf.Client()
        ci.set_host_port("combined:3333")
        self.assertEqual(ci.get_host(), "combined")
        self.assertEqual(ci.get_port(), "3333")

    def test_set_host_port_at_combined_string(self):
        """set_host_port('host@port') is the alternative combined form."""
        ci = ecf.Client()
        ci.set_host_port("athost@2222")
        self.assertEqual(ci.get_host(), "athost")
        self.assertEqual(ci.get_port(), "2222")

    def test_set_host_port_replaces_previous_values(self):
        """A second set_host_port call overwrites the first."""
        ci = ecf.Client("first", "100")
        ci.set_host_port("second", "200")
        self.assertEqual(ci.get_host(), "second")
        self.assertEqual(ci.get_port(), "200")

    def test_set_host_port_empty_host_raises(self):
        """set_host_port('', port) raises RuntimeError for an empty host."""
        ci = ecf.Client()
        with self.assertRaises(RuntimeError):
            ci.set_host_port("", "3141")

    def test_set_host_port_empty_port_raises(self):
        """set_host_port(host, '') raises RuntimeError for an empty port."""
        ci = ecf.Client()
        with self.assertRaises(RuntimeError):
            ci.set_host_port("myhost", "")

    def test_set_host_port_non_numeric_port_raises(self):
        """set_host_port(host, 'notaport') raises RuntimeError for a non-numeric port."""
        ci = ecf.Client()
        with self.assertRaises(RuntimeError):
            ci.set_host_port("myhost", "notaport")

    def test_set_host_port_no_separator_raises(self):
        """set_host_port('hostonly') raises RuntimeError when no ':' or '@' separator."""
        ci = ecf.Client()
        with self.assertRaises(RuntimeError):
            ci.set_host_port("hostonly")

    def test_set_host_port_colon_only_host_raises(self):
        """set_host_port(':3141') raises RuntimeError for an empty host component."""
        ci = ecf.Client()
        with self.assertRaises(RuntimeError):
            ci.set_host_port(":3141")

    def test_set_host_port_colon_only_port_raises(self):
        """set_host_port('host:') raises RuntimeError for an empty port component."""
        ci = ecf.Client()
        with self.assertRaises(RuntimeError):
            ci.set_host_port("host:")

    # ------------------------------------------------------------------
    # version()
    # ------------------------------------------------------------------

    def test_version_returns_non_empty_string(self):
        """version() returns a non-empty string (e.g. '5.14.0')."""
        ci = ecf.Client()
        v = ci.version()
        self.assertIsInstance(v, str)
        self.assertGreater(len(v), 0)

    def test_version_is_idempotent(self):
        """Repeated calls to version() return the same value."""
        ci = ecf.Client()
        self.assertEqual(ci.version(), ci.version())

    def test_version_matches_across_instances(self):
        """Two independent Client instances report the same library version."""
        ci1 = ecf.Client()
        ci2 = ecf.Client()
        self.assertEqual(ci1.version(), ci2.version())

    # ------------------------------------------------------------------
    # set_user_name
    # ------------------------------------------------------------------

    def test_set_user_name_accepts_non_empty_string(self):
        """set_user_name(str) succeeds silently for a valid user name."""
        ci = ecf.Client()
        ci.set_user_name("alice")  # must not raise

    def test_set_user_name_accepts_empty_string(self):
        """set_user_name('') is accepted (cleared / reset to default)."""
        ci = ecf.Client()
        ci.set_user_name("")  # must not raise

    # ------------------------------------------------------------------
    # set_retry_connection_period
    # ------------------------------------------------------------------

    def test_set_retry_connection_period_positive(self):
        """set_retry_connection_period(n) accepts a positive integer."""
        ci = ecf.Client()
        ci.set_retry_connection_period(10)  # must not raise

    def test_set_retry_connection_period_zero(self):
        """set_retry_connection_period(0) is accepted (no delay)."""
        ci = ecf.Client()
        ci.set_retry_connection_period(0)  # must not raise

    # ------------------------------------------------------------------
    # set_connection_attempts
    # ------------------------------------------------------------------

    def test_set_connection_attempts_one(self):
        """set_connection_attempts(1) is the minimum meaningful value."""
        ci = ecf.Client()
        ci.set_connection_attempts(1)  # must not raise

    def test_set_connection_attempts_larger_value(self):
        """set_connection_attempts(n > 1) is accepted."""
        ci = ecf.Client()
        ci.set_connection_attempts(5)  # must not raise

    def test_set_connection_attempts_zero_clamped_silently(self):
        """set_connection_attempts(0) is internally clamped to 1 without raising."""
        ci = ecf.Client()
        ci.set_connection_attempts(0)  # C++ does max(1u, 0) — no exception

    # ------------------------------------------------------------------
    # set_auto_sync / is_auto_sync_enabled
    # ------------------------------------------------------------------

    def test_auto_sync_disabled_by_default(self):
        """is_auto_sync_enabled() returns False for a freshly constructed Client."""
        ci = ecf.Client()
        self.assertFalse(ci.is_auto_sync_enabled())

    def test_set_auto_sync_true_enables_it(self):
        """set_auto_sync(True) makes is_auto_sync_enabled() return True."""
        ci = ecf.Client()
        ci.set_auto_sync(True)
        self.assertTrue(ci.is_auto_sync_enabled())

    def test_set_auto_sync_false_disables_it(self):
        """set_auto_sync(False) makes is_auto_sync_enabled() return False."""
        ci = ecf.Client()
        ci.set_auto_sync(True)
        ci.set_auto_sync(False)
        self.assertFalse(ci.is_auto_sync_enabled())

    def test_set_auto_sync_toggle(self):
        """is_auto_sync_enabled() reflects each toggle of set_auto_sync."""
        ci = ecf.Client()
        for expected in [True, False, True, False]:
            ci.set_auto_sync(expected)
            self.assertEqual(ci.is_auto_sync_enabled(), expected)

    def test_is_auto_sync_enabled_returns_bool(self):
        """is_auto_sync_enabled() always returns a Python bool (or int-subclass)."""
        ci = ecf.Client()
        result = ci.is_auto_sync_enabled()
        self.assertIsInstance(result, (bool, int))

    # ------------------------------------------------------------------
    # get_defs
    # ------------------------------------------------------------------

    def test_get_defs_returns_none_before_sync(self):
        """get_defs() returns None for a freshly constructed Client (no sync done)."""
        ci = ecf.Client()
        self.assertIsNone(ci.get_defs())

    # ------------------------------------------------------------------
    # reset
    # ------------------------------------------------------------------

    def test_reset_does_not_raise(self):
        """reset() completes without raising, even before any server interaction."""
        ci = ecf.Client()
        ci.reset()  # must not raise

    def test_reset_clears_defs(self):
        """get_defs() remains None after reset() (nothing to clear, still None)."""
        ci = ecf.Client()
        ci.reset()
        self.assertIsNone(ci.get_defs())

    def test_reset_clears_handle(self):
        """ch_handle() is 0 after reset(), just as it was at construction."""
        ci = ecf.Client()
        ci.reset()
        self.assertEqual(ci.ch_handle(), 0)

    # ------------------------------------------------------------------
    # in_sync
    # ------------------------------------------------------------------

    def test_in_sync_returns_false_before_sync(self):
        """in_sync() returns False for a freshly constructed Client."""
        ci = ecf.Client()
        self.assertFalse(ci.in_sync())

    def test_in_sync_returns_bool(self):
        """in_sync() returns a Python bool (or int-subclass)."""
        ci = ecf.Client()
        self.assertIsInstance(ci.in_sync(), (bool, int))

    # ------------------------------------------------------------------
    # ch_handle
    # ------------------------------------------------------------------

    def test_ch_handle_is_zero_by_default(self):
        """ch_handle() returns 0 before any ch_register call."""
        ci = ecf.Client()
        self.assertEqual(ci.ch_handle(), 0)

    def test_ch_handle_returns_int(self):
        """ch_handle() always returns a Python int."""
        ci = ecf.Client()
        self.assertIsInstance(ci.ch_handle(), int)

    # ------------------------------------------------------------------
    # wait_for_server_reply
    # ------------------------------------------------------------------

    def test_wait_for_server_reply_zero_timeout_returns_false(self):
        """wait_for_server_reply(0), without a running server, times out (almost) immediately and returns False."""
        ci = ecf.Client("supercalifragilisticexpialidocious", "50000")
        result = ci.wait_for_server_reply(0)
        self.assertFalse(result)

    def test_wait_for_server_reply_returns_bool(self):
        """wait_for_server_reply() always returns a Python bool (or int-subclass)."""
        ci = ecf.Client()
        result = ci.wait_for_server_reply(0)
        self.assertIsInstance(result, (bool, int))

    # ------------------------------------------------------------------
    # changed_node_paths
    # ------------------------------------------------------------------

    def test_changed_node_paths_empty_before_sync(self):
        """Iterating changed_node_paths before any sync yields no items."""
        ci = ecf.Client()
        paths = list(ci.changed_node_paths)
        self.assertEqual(paths, [])

    # ------------------------------------------------------------------
    # debug
    # ------------------------------------------------------------------

    def test_debug_true_does_not_raise(self):
        """debug(True) enables verbose output; must not raise."""
        ci = ecf.Client()
        ci.debug(True)   # must not raise
        ci.debug(False)  # restore to silent

    def test_debug_false_does_not_raise(self):
        """debug(False) disables verbose output; must not raise."""
        ci = ecf.Client()
        ci.debug(False)  # must not raise

    def test_debug_toggle_does_not_raise(self):
        """Toggling debug on and off repeatedly does not raise."""
        ci = ecf.Client()
        for _ in range(3):
            ci.debug(True)
            ci.debug(False)

    # ------------------------------------------------------------------
    # enable_http / enable_https
    # ------------------------------------------------------------------

    def test_enable_http_does_not_raise(self):
        """enable_http() switches the transport mode without raising."""
        ci = ecf.Client()
        ci.enable_http()  # must not raise

    def test_enable_https_does_not_raise(self):
        """enable_https() switches the transport mode without raising."""
        ci = ecf.Client()
        ci.enable_https()  # must not raise

    def test_enable_http_and_https_can_be_called_sequentially(self):
        """Calling enable_http() then enable_https() does not raise."""
        ci = ecf.Client()
        ci.enable_http()
        ci.enable_https()  # must not raise

    # ------------------------------------------------------------------
    # Child-command configuration setters
    # ------------------------------------------------------------------

    def test_set_child_path_accepts_absolute_path(self):
        """set_child_path('/suite/family/task') is accepted without error."""
        ci = ecf.Client()
        ci.set_child_path("/suite/family/task")  # must not raise

    def test_set_child_path_accepts_empty_string(self):
        """set_child_path('') is accepted (clears the child path)."""
        ci = ecf.Client()
        ci.set_child_path("")  # must not raise

    def test_set_child_password_accepts_string(self):
        """set_child_password(str) is accepted without error."""
        ci = ecf.Client()
        ci.set_child_password("secret123")  # must not raise

    def test_set_child_password_accepts_empty_string(self):
        """set_child_password('') is accepted (clears the password)."""
        ci = ecf.Client()
        ci.set_child_password("")  # must not raise

    def test_set_child_pid_str_overload(self):
        """set_child_pid(str) stores the PID string without error."""
        ci = ecf.Client()
        ci.set_child_pid("12345")  # must not raise

    def test_set_child_pid_int_overload(self):
        """set_child_pid(int) converts the integer to string without error."""
        ci = ecf.Client()
        ci.set_child_pid(99999)  # must not raise

    def test_set_child_pid_int_zero(self):
        """set_child_pid(0) is accepted."""
        ci = ecf.Client()
        ci.set_child_pid(0)  # must not raise

    def test_set_child_try_no_positive(self):
        """set_child_try_no(n) is accepted for positive values."""
        ci = ecf.Client()
        ci.set_child_try_no(1)  # must not raise

    def test_set_child_try_no_zero(self):
        """set_child_try_no(0) is accepted."""
        ci = ecf.Client()
        ci.set_child_try_no(0)  # must not raise

    def test_set_child_timeout_positive(self):
        """set_child_timeout(n) accepts a positive number of seconds."""
        ci = ecf.Client()
        ci.set_child_timeout(120)  # must not raise

    def test_set_child_timeout_zero(self):
        """set_child_timeout(0) is accepted."""
        ci = ecf.Client()
        ci.set_child_timeout(0)  # must not raise

    def test_set_child_init_add_vars_dict_overload(self):
        """set_child_init_add_vars(dict) converts each entry to a Variable."""
        ci = ecf.Client()
        ci.set_child_init_add_vars({"VAR1": "val1", "VAR2": "val2"})

    def test_set_child_init_add_vars_dict_empty(self):
        """set_child_init_add_vars({}) (empty dict) is accepted."""
        ci = ecf.Client()
        ci.set_child_init_add_vars({})  # must not raise

    def test_set_child_init_add_vars_list_of_variable_overload(self):
        """set_child_init_add_vars([Variable(...)]) list overload is accepted."""
        ci = ecf.Client()
        ci.set_child_init_add_vars([ecf.Variable("MY_VAR", "my_value")])

    def test_set_child_init_add_vars_list_empty(self):
        """set_child_init_add_vars([]) (empty list) is accepted."""
        ci = ecf.Client()
        ci.set_child_init_add_vars([])  # must not raise

    def test_set_child_complete_del_vars_list(self):
        """set_child_complete_del_vars(['V1', 'V2']) is accepted."""
        ci = ecf.Client()
        ci.set_child_complete_del_vars(["V1", "V2"])  # must not raise

    def test_set_child_complete_del_vars_empty_list(self):
        """set_child_complete_del_vars([]) (empty list) is accepted."""
        ci = ecf.Client()
        ci.set_child_complete_del_vars([])  # must not raise

    def test_set_zombie_child_timeout_positive(self):
        """set_zombie_child_timeout(n) accepts a positive number of seconds."""
        ci = ecf.Client()
        ci.set_zombie_child_timeout(3600)  # must not raise

    def test_set_zombie_child_timeout_zero(self):
        """set_zombie_child_timeout(0) is accepted."""
        ci = ecf.Client()
        ci.set_zombie_child_timeout(0)  # must not raise

    # ------------------------------------------------------------------
    # Context manager (__enter__ / __exit__)
    # ------------------------------------------------------------------

    def test_context_manager_enter_returns_self(self):
        """__enter__ returns the Client itself (not a proxy or wrapper)."""
        ci = ecf.Client()
        with ci as ctx:
            self.assertIs(ctx, ci)

    def test_context_manager_inline_construction_enters_self(self):
        """'with Client() as c' binds the freshly created Client."""
        with ecf.Client() as c:
            self.assertIsInstance(c, ecf.Client)
            # The instance is functional inside the block.
            self.assertIsInstance(c.version(), str)

    def test_context_manager_exit_returns_false(self):
        """__exit__ returns False so that exceptions propagate out of the block."""

        class _Sentinel(Exception):
            pass

        caught_outside = False
        try:
            with ecf.Client():
                raise _Sentinel("propagated")
        except _Sentinel:
            caught_outside = True
        self.assertTrue(caught_outside)

    def test_context_manager_clean_exit_does_not_raise(self):
        """The with-block exits cleanly (no exception) without error."""
        with ecf.Client():
            pass  # must not raise

    # ------------------------------------------------------------------
    # __hash__ — identity-based hashing for noncopyable boost.python types
    # ------------------------------------------------------------------

    def test_client_is_hashable(self):
        """Client instances are hashable; hash() returns an int."""
        ci = ecf.Client()
        self.assertIsInstance(hash(ci), int)

    def test_client_can_be_inserted_in_set(self):
        """Client is hashable, so instances can be stored in a Python set."""
        ci = ecf.Client()
        s = {ci}
        self.assertIn(ci, s)

    def test_client_can_be_used_as_dict_key(self):
        """Client is hashable, so instances can be used as dictionary keys."""
        ci = ecf.Client()
        d = {ci: "value"}
        self.assertEqual(d[ci], "value")

    def test_two_clients_have_different_hashes(self):
        """Two independently created Clients have different identity-based hashes."""
        ci1 = ecf.Client()
        ci2 = ecf.Client()
        self.assertIsNot(ci1, ci2)
        self.assertNotEqual(hash(ci1), hash(ci2))

    def test_same_client_has_stable_hash(self):
        """hash() of the same Client object returns the same value on repeated calls."""
        ci = ecf.Client()
        self.assertEqual(hash(ci), hash(ci))

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — identity-based (noncopyable type)
    # ------------------------------------------------------------------

    def test_eq_same_object_is_true(self):
        """A Client compares equal to itself."""
        ci = ecf.Client()
        self.assertTrue(ci == ci)

    def test_eq_two_distinct_clients_is_false(self):
        """Two distinct Client objects with the same configuration are not equal
        (identity semantics — noncopyable C++ object)."""
        ci1 = ecf.Client()
        ci2 = ecf.Client()
        self.assertFalse(ci1 == ci2)

    def test_ne_same_object_is_false(self):
        """A Client is not not-equal to itself."""
        ci = ecf.Client()
        self.assertFalse(ci != ci)

    def test_ne_two_distinct_clients_is_true(self):
        """Two distinct Client objects are not-equal under identity semantics."""
        ci1 = ecf.Client()
        ci2 = ecf.Client()
        self.assertTrue(ci1 != ci2)


class TestWhyCmd(unittest.TestCase):
    """Tests for py::class_<WhyCmd, boost::noncopyable> as exposed in ExportClient.cpp.

    Exposed API
    -----------
    Constructor
        WhyCmd(defs_ptr defs, str node_path)
            - defs   : pointer to a loaded definition structure
            - path   : path of the node to interrogate

    Methods
        why() -> str    -- '\n'-separated list of reasons the node is not running

    Operators / implicit protocol
        __hash__        -- identity-based (boost.python noncopyable extension type)

    Exceptions
        RuntimeError if node_path cannot be found in the definition
        RuntimeError if the definition object is a null/empty pointer
    """

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _make_defs_with_task():
        """Return a minimal Defs containing /s/f/t."""
        defs = ecf.Defs()
        s = defs.add_suite("s")
        f = s.add_family("f")
        f.add_task("t")
        return defs

    # ------------------------------------------------------------------
    # Constructor — valid paths
    # ------------------------------------------------------------------

    def test_construct_with_task_path(self):
        """WhyCmd(defs, path_to_task) constructs without raising."""
        defs = self._make_defs_with_task()
        ecf.WhyCmd(defs, "/s/f/t")  # must not raise

    def test_construct_with_family_path(self):
        """WhyCmd(defs, path_to_family) constructs without raising."""
        defs = self._make_defs_with_task()
        ecf.WhyCmd(defs, "/s/f")  # must not raise

    def test_construct_with_suite_path(self):
        """WhyCmd(defs, path_to_suite) constructs without raising."""
        defs = self._make_defs_with_task()
        ecf.WhyCmd(defs, "/s")  # must not raise

    def test_construct_with_empty_path_does_not_raise(self):
        """WhyCmd(defs, '') is accepted; why() then reports on the definition root."""
        defs = self._make_defs_with_task()
        ecf.WhyCmd(defs, "")  # must not raise (empty path -> root)

    # ------------------------------------------------------------------
    # Constructor — invalid paths
    # ------------------------------------------------------------------

    def test_construct_with_nonexistent_path_raises(self):
        """WhyCmd(defs, '/no/such/node') raises RuntimeError."""
        defs = self._make_defs_with_task()
        with self.assertRaises(RuntimeError):
            ecf.WhyCmd(defs, "/no/such/node")

    def test_construct_with_null_defs_raises(self):
        """WhyCmd(None, path) raises RuntimeError because the defs pointer is null."""
        with self.assertRaises(RuntimeError):
            ecf.WhyCmd(None, "/s/f/t")

    # ------------------------------------------------------------------
    # why()
    # ------------------------------------------------------------------

    def test_why_returns_str(self):
        """why() always returns a Python str."""
        defs = self._make_defs_with_task()
        cmd = ecf.WhyCmd(defs, "/s/f/t")
        self.assertIsInstance(cmd.why(), str)

    def test_why_returns_non_empty_string(self):
        """why() returns a non-empty explanatory string (nodes not yet begun)."""
        defs = self._make_defs_with_task()
        cmd = ecf.WhyCmd(defs, "/s/f/t")
        result = cmd.why()
        self.assertGreater(len(result), 0)

    def test_why_is_idempotent(self):
        """Calling why() twice returns the same result."""
        defs = self._make_defs_with_task()
        cmd = ecf.WhyCmd(defs, "/s/f/t")
        self.assertEqual(cmd.why(), cmd.why())

    def test_why_on_suite_path(self):
        """why() is callable when WhyCmd was constructed with a suite path."""
        defs = self._make_defs_with_task()
        cmd = ecf.WhyCmd(defs, "/s")
        result = cmd.why()
        self.assertIsInstance(result, str)

    def test_why_on_empty_path_returns_root_analysis(self):
        """why() on a WhyCmd constructed with an empty path returns root analysis."""
        defs = self._make_defs_with_task()
        cmd = ecf.WhyCmd(defs, "")
        result = cmd.why()
        self.assertIsInstance(result, str)
        self.assertGreater(len(result), 0)

    # ------------------------------------------------------------------
    # __hash__ — identity-based (noncopyable boost.python type)
    # ------------------------------------------------------------------

    def test_why_cmd_is_hashable(self):
        """WhyCmd instances are hashable; hash() returns an int."""
        defs = self._make_defs_with_task()
        cmd = ecf.WhyCmd(defs, "/s/f/t")
        self.assertIsInstance(hash(cmd), int)

    def test_why_cmd_can_be_inserted_in_set(self):
        """WhyCmd is hashable, so instances can be stored in a Python set."""
        defs = self._make_defs_with_task()
        cmd = ecf.WhyCmd(defs, "/s/f/t")
        s = {cmd}
        self.assertIn(cmd, s)

    def test_two_why_cmds_have_different_hashes(self):
        """Two independently created WhyCmd objects have different identity hashes."""
        defs = self._make_defs_with_task()
        cmd1 = ecf.WhyCmd(defs, "/s/f/t")
        cmd2 = ecf.WhyCmd(defs, "/s/f/t")
        self.assertNotEqual(hash(cmd1), hash(cmd2))


class TestUrlCmd(unittest.TestCase):
    """Tests for py::class_<UrlCmd, boost::noncopyable> as exposed in ExportClient.cpp.

    Exposed API
    -----------
    Constructor
        UrlCmd(defs_ptr defs, str node_path)
            - defs   : pointer to a loaded definition structure
            - path   : path of the node to open in a browser

    Methods
        execute() -> str or None  -- runs the ECF_URL_CMD shell command
                                     (returns None when ECF_URL_CMD undefined)

    Operators / implicit protocol
        __hash__                  -- identity-based (boost.python noncopyable)

    Exceptions
        RuntimeError if node_path is empty
        RuntimeError if node_path cannot be found in the definition
    """

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    @staticmethod
    def _make_defs_with_url():
        """Return a Defs where the task has ECF_URL defined."""
        defs = ecf.Defs()
        s = defs.add_suite("s")
        s.add_variable("ECF_URL_CMD", "echo %ECF_URL_BASE%/%ECF_URL%")
        s.add_variable("ECF_URL_BASE", "http://example.com")
        f = s.add_family("f")
        t = f.add_task("t")
        t.add_variable("ECF_URL", "manual.html")
        return defs

    @staticmethod
    def _make_minimal_defs():
        """Return a Defs with a task but NO ECF_URL_CMD defined."""
        defs = ecf.Defs()
        s = defs.add_suite("s")
        f = s.add_family("f")
        f.add_task("t")
        return defs

    # ------------------------------------------------------------------
    # Constructor — valid paths
    # ------------------------------------------------------------------

    def test_construct_with_task_path(self):
        """UrlCmd(defs, path_to_task) constructs without raising."""
        defs = self._make_defs_with_url()
        ecf.UrlCmd(defs, "/s/f/t")  # must not raise

    def test_construct_with_family_path(self):
        """UrlCmd(defs, path_to_family) constructs without raising."""
        defs = self._make_defs_with_url()
        ecf.UrlCmd(defs, "/s/f")  # must not raise

    def test_construct_without_url_cmd_variable(self):
        """UrlCmd can be constructed even when ECF_URL_CMD is not set on the node."""
        defs = self._make_minimal_defs()
        ecf.UrlCmd(defs, "/s/f/t")  # construction succeeds; execute() may fail

    # ------------------------------------------------------------------
    # Constructor — invalid paths
    # ------------------------------------------------------------------

    def test_construct_with_empty_path_raises(self):
        """UrlCmd(defs, '') raises RuntimeError for an empty node path."""
        defs = self._make_minimal_defs()
        with self.assertRaises(RuntimeError):
            ecf.UrlCmd(defs, "")

    def test_construct_with_nonexistent_path_raises(self):
        """UrlCmd(defs, '/no/such/node') raises RuntimeError."""
        defs = self._make_minimal_defs()
        with self.assertRaises(RuntimeError):
            ecf.UrlCmd(defs, "/no/such/node")

    def test_construct_with_null_defs_raises(self):
        """UrlCmd(None, path) raises RuntimeError because the defs pointer is null."""
        with self.assertRaises(RuntimeError):
            ecf.UrlCmd(None, "/s/f/t")

    # ------------------------------------------------------------------
    # execute() — ECF_URL_CMD not defined
    # ------------------------------------------------------------------

    def test_execute_without_url_cmd_returns_none(self):
        """execute() returns None when ECF_URL_CMD is not defined on the node."""
        defs = self._make_minimal_defs()
        cmd = ecf.UrlCmd(defs, "/s/f/t")
        result = cmd.execute()
        self.assertIsNone(result)

    # ------------------------------------------------------------------
    # __hash__ — identity-based (noncopyable boost.python type)
    # ------------------------------------------------------------------

    def test_url_cmd_is_hashable(self):
        """UrlCmd instances are hashable; hash() returns an int."""
        defs = self._make_minimal_defs()
        cmd = ecf.UrlCmd(defs, "/s/f/t")
        self.assertIsInstance(hash(cmd), int)

    def test_url_cmd_can_be_inserted_in_set(self):
        """UrlCmd is hashable, so instances can be stored in a Python set."""
        defs = self._make_minimal_defs()
        cmd = ecf.UrlCmd(defs, "/s/f/t")
        s = {cmd}
        self.assertIn(cmd, s)

    def test_two_url_cmds_have_different_hashes(self):
        """Two independently created UrlCmd objects have different identity hashes."""
        defs = self._make_minimal_defs()
        cmd1 = ecf.UrlCmd(defs, "/s/f/t")
        cmd2 = ecf.UrlCmd(defs, "/s/f/t")
        self.assertNotEqual(hash(cmd1), hash(cmd2))


if __name__ == "__main__":
    unittest.main(verbosity=2)
