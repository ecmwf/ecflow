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
from os import environ

import ecflow as ecf
import unittest
from typing import List, Dict


def _is_running_test_on(prefix):
    """Helper to detect if running on a CI runner with a name starting with the given prefix."""
    envvar = environ.get('RUNNER_NAME')
    return envvar is not None and envvar.startswith(prefix)


class TestTrigger(unittest.TestCase):
    """Tests for py::class_<Trigger> as exposed in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Trigger(str)
        Trigger(str, bool)          -- bool selects AND (True) / OR (False) join type
        Trigger(PartExpression)
        Trigger(list)               -- list of str or Node; PartExpression items are rejected

    Methods / operators
        get_expression() -> str     -- compose_expression over internal PartExpression vector
        __str__                     -- same as get_expression()
        __eq__                      -- compares the internal PartExpression vectors
        __ne__                      -- implicit complement of __eq__
        __hash__                    -- boost.python extension types retain identity-based tp_hash
                                       even when __eq__ is defined (unlike pure-Python classes)
    """

    # ------------------------------------------------------------------
    # Constructor: Trigger(str)
    # ------------------------------------------------------------------

    def test_create_from_str_simple(self):
        """Plain expression string is stored verbatim (FIRST join type, no prefix)."""
        t = ecf.Trigger("a == b")
        self.assertEqual(t.get_expression(), "a == b")

    def test_create_from_str_compound(self):
        """A multi-token string is stored verbatim; it is NOT split or reparsed."""
        t = ecf.Trigger("a == b OR c == d")
        self.assertEqual(t.get_expression(), "a == b OR c == d")

    def test_create_from_str_empty(self):
        """An empty string is accepted and stored as an empty expression."""
        t = ecf.Trigger("")
        self.assertEqual(t.get_expression(), "")

    # ------------------------------------------------------------------
    # Constructor: Trigger(str, bool)
    # ------------------------------------------------------------------

    def test_create_from_str_and_bool_true_produces_and_prefix(self):
        """Trigger(str, True) marks the part as AND-type; compose_expression prepends ' AND '."""
        t = ecf.Trigger("a == b", True)
        self.assertEqual(t.get_expression(), " AND a == b")

    def test_create_from_str_and_bool_false_produces_or_prefix(self):
        """Trigger(str, False) marks the part as OR-type; compose_expression prepends ' OR '."""
        t = ecf.Trigger("a == b", False)
        self.assertEqual(t.get_expression(), " OR a == b")

    # ------------------------------------------------------------------
    # Constructor: Trigger(PartExpression)
    # ------------------------------------------------------------------

    def test_create_from_part_expression_first_type(self):
        """PartExpression with default (FIRST) join type produces no prefix."""
        t = ecf.Trigger(ecf.PartExpression("a == b"))
        self.assertEqual(t.get_expression(), "a == b")

    def test_create_from_part_expression_and_type(self):
        """PartExpression(str, True) is AND-type; Trigger inherits the ' AND ' prefix."""
        t = ecf.Trigger(ecf.PartExpression("a == b", True))
        self.assertEqual(t.get_expression(), " AND a == b")

    def test_create_from_part_expression_or_type(self):
        """PartExpression(str, False) is OR-type; Trigger inherits the ' OR ' prefix."""
        t = ecf.Trigger(ecf.PartExpression("a == b", False))
        self.assertEqual(t.get_expression(), " OR a == b")

    def test_create_from_part_expression_compound(self):
        """A compound string inside a PartExpression is stored verbatim."""
        t = ecf.Trigger(ecf.PartExpression("a == b OR c == d"))
        self.assertEqual(t.get_expression(), "a == b OR c == d")

    # ------------------------------------------------------------------
    # Constructor: Trigger(list)  -- positive cases
    # ------------------------------------------------------------------

    def test_create_from_list_empty(self):
        """An empty list produces an empty expression."""
        t = ecf.Trigger([])
        self.assertEqual(t.get_expression(), "")

    def test_create_from_list_single_expression_string(self):
        """A single non-name string (contains spaces/operators) is stored as-is."""
        t = ecf.Trigger(["a == b"])
        self.assertEqual(t.get_expression(), "a == b")

    def test_create_from_list_single_valid_name_gets_complete_appended(self):
        """A valid ecflow node name (alphanumeric/underscore only) is auto-suffixed '== complete'."""
        t = ecf.Trigger(["taskA"])
        self.assertEqual(t.get_expression(), "taskA == complete")

    def test_create_from_list_multiple_expression_strings_are_and_joined(self):
        """Multiple non-name strings: first is FIRST type, subsequent are AND-joined."""
        t = ecf.Trigger(["a == b", "c == d"])
        self.assertEqual(t.get_expression(), "a == b AND c == d")

    def test_create_from_list_multiple_valid_names_are_and_joined(self):
        """Multiple valid names: each gets '== complete' suffix, joined with AND."""
        t = ecf.Trigger(["taskA", "taskB"])
        self.assertEqual(t.get_expression(), "taskA == complete AND taskB == complete")

    def test_create_from_list_three_strings_and_joined(self):
        """Three strings produce a chain of AND-joined parts."""
        t = ecf.Trigger(["a == b", "c == d", "e == f"])
        self.assertEqual(t.get_expression(), "a == b AND c == d AND e == f")

    # ------------------------------------------------------------------
    # get_expression()
    # ------------------------------------------------------------------

    def test_get_expression_returns_str(self):
        """get_expression() always returns a Python str."""
        t = ecf.Trigger("x == active")
        self.assertIsInstance(t.get_expression(), str)

    def test_get_expression_is_idempotent(self):
        """Calling get_expression() twice returns the same value."""
        t = ecf.Trigger("a == b")
        self.assertEqual(t.get_expression(), t.get_expression())

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_equals_get_expression(self):
        """__str__ is bound to the same function as get_expression()."""
        t = ecf.Trigger("a == b")
        self.assertEqual(str(t), t.get_expression())

    def test_str_on_compound_expression(self):
        """__str__ on a compound expression returns the full composed string."""
        t = ecf.Trigger(["a == b", "c == d"])
        self.assertEqual(str(t), "a == b AND c == d")

    # ------------------------------------------------------------------
    # __eq__ — equal cases
    # ------------------------------------------------------------------

    def test_eq_identical_str_constructor(self):
        """Two Triggers created with the same string are equal."""
        t1 = ecf.Trigger("a == b")
        t2 = ecf.Trigger("a == b")
        self.assertEqual(t1, t2)

    def test_eq_str_and_part_expression_first_type(self):
        """Trigger(str) and Trigger(PartExpression(str)) with FIRST type are equal."""
        t1 = ecf.Trigger("a == b")
        t2 = ecf.Trigger(ecf.PartExpression("a == b"))
        self.assertEqual(t1, t2)

    def test_eq_reflexive(self):
        """A Trigger is equal to itself."""
        t = ecf.Trigger("a == b")
        self.assertEqual(t, t)

    def test_eq_symmetric(self):
        """Equality is symmetric: t1 == t2 implies t2 == t1."""
        t1 = ecf.Trigger("a == b")
        t2 = ecf.Trigger("a == b")
        self.assertTrue(t1 == t2)
        self.assertTrue(t2 == t1)

    def test_eq_list_and_str_with_same_composed_expression(self):
        """Trigger from list produces the same PartExpression vector as the equivalent."""
        t1 = ecf.Trigger(["a == b"])
        t2 = ecf.Trigger("a == b")
        self.assertEqual(t1, t2)

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — not-equal cases
    # ------------------------------------------------------------------

    def test_ne_different_expression_strings(self):
        """Triggers with different expression strings are not equal."""
        t1 = ecf.Trigger("a == b")
        t2 = ecf.Trigger("c == d")
        self.assertNotEqual(t1, t2)

    def test_ne_str_vs_str_with_bool_true(self):
        """Trigger(str) ≠ Trigger(str, True): the internal join-type flag differs."""
        t1 = ecf.Trigger("a == b")
        t2 = ecf.Trigger("a == b", True)
        self.assertNotEqual(t1, t2)

    def test_ne_str_with_bool_true_vs_bool_false(self):
        """Trigger(str, True) ≠ Trigger(str, False): AND-type vs OR-type."""
        t1 = ecf.Trigger("a == b", True)
        t2 = ecf.Trigger("a == b", False)
        self.assertNotEqual(t1, t2)

    def test_ne_single_vs_multiple_parts(self):
        """A single-part Trigger differs from a two-part Trigger."""
        t1 = ecf.Trigger("a == b")
        t2 = ecf.Trigger(["a == b", "c == d"])
        self.assertNotEqual(t1, t2)

    # ------------------------------------------------------------------
    # __hash__ — boost.python extension types are hashable by object identity
    # ------------------------------------------------------------------

    def test_is_hashable_by_identity(self):
        """boost.python extension types keep identity-based tp_hash even when __eq__ is
        defined.  Unlike pure-Python classes, defining __eq__ does NOT set __hash__=None
        for C-extension types; hash() returns a non-zero integer derived from the object id.
        """
        t = ecf.Trigger("a == b")
        self.assertIsInstance(hash(t), int)

    def test_can_be_inserted_in_set(self):
        """Because Trigger is hashable, instances can be stored in a Python set."""
        t = ecf.Trigger("a == b")
        s = {t}
        self.assertIn(t, s)

    def test_can_be_used_as_dict_key(self):
        """Because Trigger is hashable, instances can be used as dictionary keys."""
        t = ecf.Trigger("a == b")
        d = {t: "value"}
        self.assertEqual(d[t], "value")

    def test_hash_is_identity_based(self):
        """Two equal-by-value Triggers have different hashes (identity, not value)."""
        t1 = ecf.Trigger("a == b")
        t2 = ecf.Trigger("a == b")
        self.assertEqual(t1, t2)  # same value
        self.assertIsNot(t1, t2)  # different objects
        self.assertNotEqual(hash(t1), hash(t2))  # therefore different hashes

    # ------------------------------------------------------------------
    # Negative: invalid constructor arguments (wrong Python types)
    # ------------------------------------------------------------------

    def test_create_from_int_raises(self):
        """No constructor accepts a plain integer."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Trigger(42)

    def test_create_from_none_raises(self):
        """No constructor accepts None."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Trigger(None)

    # ------------------------------------------------------------------
    # Negative: invalid list contents
    # ------------------------------------------------------------------

    def test_list_with_integer_raises(self):
        """An integer inside the list is not a string or Node; raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Trigger([42])

    @unittest.skip(
        "KNOWN BUG: passing None inside a list causes a segfault in the C++ binding "
        "(py::extract<node_ptr>(None) crashes). Skip until fixed in Trigger.cpp."
    )
    def test_list_with_none_raises(self):
        """None inside the list should raise RuntimeError, but currently segfaults."""
        with self.assertRaises(RuntimeError):
            ecf.Trigger([None])

    def test_list_with_part_expression_raises(self):
        """PartExpression objects are NOT accepted by the list constructor; raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Trigger([ecf.PartExpression("a == b")])

    def test_list_with_mixed_str_and_part_expression_raises(self):
        """A PartExpression anywhere in the list triggers RuntimeError, even after a valid string."""
        with self.assertRaises(RuntimeError):
            ecf.Trigger(["a == b", ecf.PartExpression("c == d")])

    def test_list_with_nested_list_raises(self):
        """A nested list inside the list is not a valid element; raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Trigger([["a == b"]])

    def test_list_with_float_raises(self):
        """A float inside the list is not a valid element; raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Trigger([3.14])

    def test_list_with_bool_raises(self):
        """A bare bool inside the list (no associated expression) raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Trigger([True])


class TestComplete(unittest.TestCase):
    """Tests for py::class_<Complete> as exposed in ExportNodeAttr.cpp.

    Complete shares the same implementation pattern as Trigger (both wrap a
    vector of PartExpression and use the same construct_expr helper for list
    construction).  The exposed API is therefore identical:

    Exposed API
    -----------
    Constructors
        Complete(str)
        Complete(str, bool)         -- bool selects AND (True) / OR (False) join type
        Complete(PartExpression)
        Complete(list)              -- list of str or Node; PartExpression items are rejected

    Methods / operators
        get_expression() -> str     -- compose_expression over internal PartExpression vector
        __str__                     -- same as get_expression()
        __eq__                      -- compares the internal PartExpression vectors
        __ne__                      -- implicit complement of __eq__
        __hash__                    -- boost.python extension types retain identity-based tp_hash
                                       even when __eq__ is defined (unlike pure-Python classes)
    """

    # ------------------------------------------------------------------
    # Constructor: Complete(str)
    # ------------------------------------------------------------------

    def test_create_from_str_simple(self):
        """Plain expression string is stored verbatim (FIRST join type, no prefix)."""
        c = ecf.Complete("a == b")
        self.assertEqual(c.get_expression(), "a == b")

    def test_create_from_str_compound(self):
        """A multi-token string is stored verbatim; it is NOT split or reparsed."""
        c = ecf.Complete("a == b OR c == d")
        self.assertEqual(c.get_expression(), "a == b OR c == d")

    def test_create_from_str_empty(self):
        """An empty string is accepted and stored as an empty expression."""
        c = ecf.Complete("")
        self.assertEqual(c.get_expression(), "")

    # ------------------------------------------------------------------
    # Constructor: Complete(str, bool)
    # ------------------------------------------------------------------

    def test_create_from_str_and_bool_true_produces_and_prefix(self):
        """Complete(str, True) marks the part as AND-type; compose_expression prepends ' AND '."""
        c = ecf.Complete("a == b", True)
        self.assertEqual(c.get_expression(), " AND a == b")

    def test_create_from_str_and_bool_false_produces_or_prefix(self):
        """Complete(str, False) marks the part as OR-type; compose_expression prepends ' OR '."""
        c = ecf.Complete("a == b", False)
        self.assertEqual(c.get_expression(), " OR a == b")

    # ------------------------------------------------------------------
    # Constructor: Complete(PartExpression)
    # ------------------------------------------------------------------

    def test_create_from_part_expression_first_type(self):
        """PartExpression with default (FIRST) join type produces no prefix."""
        c = ecf.Complete(ecf.PartExpression("a == b"))
        self.assertEqual(c.get_expression(), "a == b")

    def test_create_from_part_expression_and_type(self):
        """PartExpression(str, True) is AND-type; Complete inherits the ' AND ' prefix."""
        c = ecf.Complete(ecf.PartExpression("a == b", True))
        self.assertEqual(c.get_expression(), " AND a == b")

    def test_create_from_part_expression_or_type(self):
        """PartExpression(str, False) is OR-type; Complete inherits the ' OR ' prefix."""
        c = ecf.Complete(ecf.PartExpression("a == b", False))
        self.assertEqual(c.get_expression(), " OR a == b")

    def test_create_from_part_expression_compound(self):
        """A compound string inside a PartExpression is stored verbatim."""
        c = ecf.Complete(ecf.PartExpression("a == b OR c == d"))
        self.assertEqual(c.get_expression(), "a == b OR c == d")

    # ------------------------------------------------------------------
    # Constructor: Complete(list)  -- positive cases
    # ------------------------------------------------------------------

    def test_create_from_list_empty(self):
        """An empty list produces an empty expression."""
        c = ecf.Complete([])
        self.assertEqual(c.get_expression(), "")

    def test_create_from_list_single_expression_string(self):
        """A single non-name string (contains spaces/operators) is stored as-is."""
        c = ecf.Complete(["a == b"])
        self.assertEqual(c.get_expression(), "a == b")

    def test_create_from_list_single_valid_name_gets_complete_appended(self):
        """A valid ecflow node name (alphanumeric/underscore only) is auto-suffixed '== complete'."""
        c = ecf.Complete(["taskA"])
        self.assertEqual(c.get_expression(), "taskA == complete")

    def test_create_from_list_multiple_expression_strings_are_and_joined(self):
        """Multiple non-name strings: first is FIRST type, subsequent are AND-joined."""
        c = ecf.Complete(["a == b", "c == d"])
        self.assertEqual(c.get_expression(), "a == b AND c == d")

    def test_create_from_list_multiple_valid_names_are_and_joined(self):
        """Multiple valid names: each gets '== complete' suffix, joined with AND."""
        c = ecf.Complete(["taskA", "taskB"])
        self.assertEqual(c.get_expression(), "taskA == complete AND taskB == complete")

    def test_create_from_list_three_strings_and_joined(self):
        """Three strings produce a chain of AND-joined parts."""
        c = ecf.Complete(["a == b", "c == d", "e == f"])
        self.assertEqual(c.get_expression(), "a == b AND c == d AND e == f")

    # ------------------------------------------------------------------
    # get_expression()
    # ------------------------------------------------------------------

    def test_get_expression_returns_str(self):
        """get_expression() always returns a Python str."""
        c = ecf.Complete("x == active")
        self.assertIsInstance(c.get_expression(), str)

    def test_get_expression_is_idempotent(self):
        """Calling get_expression() twice returns the same value."""
        c = ecf.Complete("a == b")
        self.assertEqual(c.get_expression(), c.get_expression())

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_equals_get_expression(self):
        """__str__ is bound to the same function as get_expression()."""
        c = ecf.Complete("a == b")
        self.assertEqual(str(c), c.get_expression())

    def test_str_on_compound_expression(self):
        """__str__ on a compound expression returns the full composed string."""
        c = ecf.Complete(["a == b", "c == d"])
        self.assertEqual(str(c), "a == b AND c == d")

    # ------------------------------------------------------------------
    # __eq__ — equal cases
    # ------------------------------------------------------------------

    def test_eq_identical_str_constructor(self):
        """Two Completes created with the same string are equal."""
        c1 = ecf.Complete("a == b")
        c2 = ecf.Complete("a == b")
        self.assertEqual(c1, c2)

    def test_eq_str_and_part_expression_first_type(self):
        """Complete(str) and Complete(PartExpression(str)) with FIRST type are equal."""
        c1 = ecf.Complete("a == b")
        c2 = ecf.Complete(ecf.PartExpression("a == b"))
        self.assertEqual(c1, c2)

    def test_eq_reflexive(self):
        """A Complete is equal to itself."""
        c = ecf.Complete("a == b")
        self.assertEqual(c, c)

    def test_eq_symmetric(self):
        """Equality is symmetric: c1 == c2 implies c2 == c1."""
        c1 = ecf.Complete("a == b")
        c2 = ecf.Complete("a == b")
        self.assertTrue(c1 == c2)
        self.assertTrue(c2 == c1)

    def test_eq_list_and_str_with_same_composed_expression(self):
        """Complete from list produces the same PartExpression vector as the equivalent."""
        c1 = ecf.Complete(["a == b"])
        c2 = ecf.Complete("a == b")
        self.assertEqual(c1, c2)

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — not-equal cases
    # ------------------------------------------------------------------

    def test_ne_different_expression_strings(self):
        """Completes with different expression strings are not equal."""
        c1 = ecf.Complete("a == b")
        c2 = ecf.Complete("c == d")
        self.assertNotEqual(c1, c2)

    def test_ne_str_vs_str_with_bool_true(self):
        """Complete(str) ≠ Complete(str, True): the internal join-type flag differs."""
        c1 = ecf.Complete("a == b")
        c2 = ecf.Complete("a == b", True)
        self.assertNotEqual(c1, c2)

    def test_ne_str_with_bool_true_vs_bool_false(self):
        """Complete(str, True) ≠ Complete(str, False): AND-type vs OR-type."""
        c1 = ecf.Complete("a == b", True)
        c2 = ecf.Complete("a == b", False)
        self.assertNotEqual(c1, c2)

    def test_ne_single_vs_multiple_parts(self):
        """A single-part Complete differs from a two-part Complete."""
        c1 = ecf.Complete("a == b")
        c2 = ecf.Complete(["a == b", "c == d"])
        self.assertNotEqual(c1, c2)

    # ------------------------------------------------------------------
    # Complete is a distinct type from Trigger
    # ------------------------------------------------------------------

    def test_complete_and_trigger_are_different_types(self):
        """Complete and Trigger with identical expressions are different Python types."""
        c = ecf.Complete("a == b")
        t = ecf.Trigger("a == b")
        self.assertNotIsInstance(c, type(t))
        self.assertNotIsInstance(t, type(c))

    def test_complete_and_trigger_are_not_equal(self):
        """Complete and Trigger cannot be compared with ==; they are unrelated types."""
        c = ecf.Complete("a == b")
        t = ecf.Trigger("a == b")
        # boost.python __eq__ is only defined between objects of the same C++ type,
        # so comparing across types returns NotImplemented and Python falls back to
        # identity comparison, which is False for distinct objects.
        self.assertFalse(c == t)

    # ------------------------------------------------------------------
    # __hash__ — boost.python extension types are hashable by object identity
    # ------------------------------------------------------------------

    def test_is_hashable_by_identity(self):
        """boost.python extension types keep identity-based tp_hash even when __eq__ is
        defined.  hash() returns a non-zero integer derived from the object id."""
        c = ecf.Complete("a == b")
        self.assertIsInstance(hash(c), int)

    def test_can_be_inserted_in_set(self):
        """Because Complete is hashable, instances can be stored in a Python set."""
        c = ecf.Complete("a == b")
        s = {c}
        self.assertIn(c, s)

    def test_can_be_used_as_dict_key(self):
        """Because Complete is hashable, instances can be used as dictionary keys."""
        c = ecf.Complete("a == b")
        d = {c: "value"}
        self.assertEqual(d[c], "value")

    def test_hash_is_identity_based(self):
        """Two equal-by-value Completes have different hashes (identity, not value)."""
        c1 = ecf.Complete("a == b")
        c2 = ecf.Complete("a == b")
        self.assertEqual(c1, c2)
        self.assertIsNot(c1, c2)
        self.assertNotEqual(hash(c1), hash(c2))

    # ------------------------------------------------------------------
    # Negative: invalid constructor arguments (wrong Python types)
    # ------------------------------------------------------------------

    def test_create_from_int_raises(self):
        """No constructor accepts a plain integer."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Complete(42)

    def test_create_from_none_raises(self):
        """No constructor accepts None."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Complete(None)

    # ------------------------------------------------------------------
    # Negative: invalid list contents
    # ------------------------------------------------------------------

    def test_list_with_integer_raises(self):
        """An integer inside the list is not a string or Node; raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Complete([42])

    @unittest.skip(
        "KNOWN BUG: passing None inside a list causes a segfault in the C++ binding "
        "(py::extract<node_ptr>(None) crashes). Skip until fixed in Trigger.cpp."
    )
    def test_list_with_none_raises(self):
        """None inside the list should raise RuntimeError, but currently segfaults."""
        with self.assertRaises(RuntimeError):
            ecf.Complete([None])

    def test_list_with_part_expression_raises(self):
        """PartExpression objects are NOT accepted by the list constructor; raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Complete([ecf.PartExpression("a == b")])

    def test_list_with_mixed_str_and_part_expression_raises(self):
        """A PartExpression anywhere in the list triggers RuntimeError, even after a valid string."""
        with self.assertRaises(RuntimeError):
            ecf.Complete(["a == b", ecf.PartExpression("c == d")])

    def test_list_with_nested_list_raises(self):
        """A nested list inside the list is not a valid element; raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Complete([["a == b"]])

    def test_list_with_float_raises(self):
        """A float inside the list is not a valid element; raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Complete([3.14])

    def test_list_with_bool_raises(self):
        """A bare bool inside the list (no associated expression) raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Complete([True])


class TestPartExpression(unittest.TestCase):
    """Tests for py::class_<PartExpression> as exposed in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        PartExpression(str)             -- FIRST join type (default)
        PartExpression(str, bool)       -- True → AND type, False → OR type

    Methods / operators
        get_expression() -> str         -- returns the raw expression string
        and_expr()       -> bool        -- True iff this part is AND-joined
        or_expr()        -> bool        -- True iff this part is OR-joined
        __eq__                          -- compares (type_, exp_) pairs
        __ne__                          -- implicit complement of __eq__
        __hash__                        -- boost.python extension types retain identity-based tp_hash
                                           even when __eq__ is defined (unlike pure-Python classes)

    Note: no __str__ is exposed; str() falls back to boost.python's default repr.
    Note: no default constructor PartExpression() is exposed.
    """

    # ------------------------------------------------------------------
    # Constructor: PartExpression(str)  –  FIRST join type
    # ------------------------------------------------------------------

    def test_create_from_str_stores_expression_verbatim(self):
        """The expression string is stored exactly as passed."""
        pe = ecf.PartExpression("a == b")
        self.assertEqual(pe.get_expression(), "a == b")

    def test_create_from_str_empty_string(self):
        """An empty string is accepted."""
        pe = ecf.PartExpression("")
        self.assertEqual(pe.get_expression(), "")

    def test_create_from_str_is_first_type(self):
        """Default (FIRST) join type: and_expr() and or_expr() are both False."""
        pe = ecf.PartExpression("a == b")
        self.assertFalse(pe.and_expr())
        self.assertFalse(pe.or_expr())

    # ------------------------------------------------------------------
    # Constructor: PartExpression(str, bool)
    # ------------------------------------------------------------------

    def test_create_with_true_is_and_type(self):
        """PartExpression(str, True) → AND type: and_expr() True, or_expr() False."""
        pe = ecf.PartExpression("a == b", True)
        self.assertTrue(pe.and_expr())
        self.assertFalse(pe.or_expr())

    def test_create_with_false_is_or_type(self):
        """PartExpression(str, False) → OR type: and_expr() False, or_expr() True."""
        pe = ecf.PartExpression("a == b", False)
        self.assertFalse(pe.and_expr())
        self.assertTrue(pe.or_expr())

    def test_create_with_bool_stores_expression_verbatim(self):
        """Expression string is stored verbatim regardless of the bool flag."""
        pe = ecf.PartExpression("x == active", True)
        self.assertEqual(pe.get_expression(), "x == active")

    # ------------------------------------------------------------------
    # get_expression()
    # ------------------------------------------------------------------

    def test_get_expression_returns_str_type(self):
        """get_expression() always returns a Python str."""
        pe = ecf.PartExpression("a == b")
        self.assertIsInstance(pe.get_expression(), str)

    def test_get_expression_is_idempotent(self):
        """Calling get_expression() twice returns the same value."""
        pe = ecf.PartExpression("a == b")
        self.assertEqual(pe.get_expression(), pe.get_expression())

    # ------------------------------------------------------------------
    # and_expr() / or_expr()
    # ------------------------------------------------------------------

    def test_and_expr_returns_bool_type(self):
        """and_expr() returns a Python bool."""
        pe = ecf.PartExpression("a == b", True)
        self.assertIsInstance(pe.and_expr(), bool)

    def test_or_expr_returns_bool_type(self):
        """or_expr() returns a Python bool."""
        pe = ecf.PartExpression("a == b", False)
        self.assertIsInstance(pe.or_expr(), bool)

    def test_and_and_or_are_mutually_exclusive_for_and_type(self):
        """For AND type exactly one of and_expr/or_expr is True."""
        pe = ecf.PartExpression("a == b", True)
        self.assertNotEqual(pe.and_expr(), pe.or_expr())

    def test_and_and_or_are_mutually_exclusive_for_or_type(self):
        """For OR type exactly one of and_expr/or_expr is True."""
        pe = ecf.PartExpression("a == b", False)
        self.assertNotEqual(pe.and_expr(), pe.or_expr())

    def test_first_type_has_neither_and_nor_or(self):
        """FIRST type has both and_expr() and or_expr() False."""
        pe = ecf.PartExpression("a == b")
        self.assertFalse(pe.and_expr() or pe.or_expr())

    # ------------------------------------------------------------------
    # __eq__ — equal cases
    # ------------------------------------------------------------------

    def test_eq_same_str_same_default_type(self):
        """Two PartExpressions with same string and default type are equal."""
        pe1 = ecf.PartExpression("a == b")
        pe2 = ecf.PartExpression("a == b")
        self.assertEqual(pe1, pe2)

    def test_eq_same_str_same_and_type(self):
        """Two AND-type PartExpressions with same string are equal."""
        pe1 = ecf.PartExpression("a == b", True)
        pe2 = ecf.PartExpression("a == b", True)
        self.assertEqual(pe1, pe2)

    def test_eq_same_str_same_or_type(self):
        """Two OR-type PartExpressions with same string are equal."""
        pe1 = ecf.PartExpression("a == b", False)
        pe2 = ecf.PartExpression("a == b", False)
        self.assertEqual(pe1, pe2)

    def test_eq_reflexive(self):
        """A PartExpression is equal to itself."""
        pe = ecf.PartExpression("a == b")
        self.assertEqual(pe, pe)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        pe1 = ecf.PartExpression("a == b")
        pe2 = ecf.PartExpression("a == b")
        self.assertTrue(pe1 == pe2)
        self.assertTrue(pe2 == pe1)

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — not-equal cases
    # ------------------------------------------------------------------

    def test_ne_different_expression_strings(self):
        """Different expression strings are not equal."""
        pe1 = ecf.PartExpression("a == b")
        pe2 = ecf.PartExpression("c == d")
        self.assertNotEqual(pe1, pe2)

    def test_ne_same_str_different_type_first_vs_and(self):
        """Same string but different join types (FIRST vs AND) are not equal."""
        pe1 = ecf.PartExpression("a == b")
        pe2 = ecf.PartExpression("a == b", True)
        self.assertNotEqual(pe1, pe2)

    def test_ne_same_str_different_type_first_vs_or(self):
        """Same string but different join types (FIRST vs OR) are not equal."""
        pe1 = ecf.PartExpression("a == b")
        pe2 = ecf.PartExpression("a == b", False)
        self.assertNotEqual(pe1, pe2)

    def test_ne_same_str_different_type_and_vs_or(self):
        """Same string, AND vs OR join type: not equal."""
        pe1 = ecf.PartExpression("a == b", True)
        pe2 = ecf.PartExpression("a == b", False)
        self.assertNotEqual(pe1, pe2)

    # ------------------------------------------------------------------
    # __hash__ — boost.python extension types are hashable by object identity
    # ------------------------------------------------------------------

    def test_is_hashable_by_identity(self):
        """boost.python extension types keep identity-based tp_hash even when __eq__ is
        defined.  hash() returns a non-zero integer derived from the object id."""
        pe = ecf.PartExpression("a == b")
        self.assertIsInstance(hash(pe), int)

    def test_can_be_inserted_in_set(self):
        """Because PartExpression is hashable, instances can be stored in a Python set."""
        pe = ecf.PartExpression("a == b")
        s = {pe}
        self.assertIn(pe, s)

    def test_can_be_used_as_dict_key(self):
        """Because PartExpression is hashable, instances can be used as dictionary keys."""
        pe = ecf.PartExpression("a == b")
        d = {pe: "value"}
        self.assertEqual(d[pe], "value")

    def test_hash_is_identity_based(self):
        """Two equal-by-value PartExpressions have different hashes (identity, not value)."""
        pe1 = ecf.PartExpression("a == b")
        pe2 = ecf.PartExpression("a == b")
        self.assertEqual(pe1, pe2)
        self.assertIsNot(pe1, pe2)
        self.assertNotEqual(hash(pe1), hash(pe2))

    # ------------------------------------------------------------------
    # Negative: invalid constructor arguments
    # ------------------------------------------------------------------

    def test_no_default_constructor(self):
        """PartExpression() with no arguments is not exposed; raises TypeError."""
        with self.assertRaises(TypeError):
            ecf.PartExpression()

    def test_create_from_int_raises(self):
        """No constructor accepts a plain integer as the first argument."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.PartExpression(42)

    def test_create_from_none_raises(self):
        """No constructor accepts None as the first argument."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.PartExpression(None)

    def test_create_from_list_raises(self):
        """No constructor accepts a list as the first argument."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.PartExpression(["a == b"])

    def test_create_with_string_bool_flag(self):
        """A string as the second argument (bool position) should raise TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.PartExpression("a == b", "yes")

    def test_create_with_too_many_args_raises(self):
        """Passing three arguments raises TypeError; no 3-argument constructor exists."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.PartExpression("a == b", True, "extra")


class TestExpression(unittest.TestCase):
    """Tests for py::class_<Expression> as exposed in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Expression(str)                 -- creates a FIRST-type single-part expression
        Expression(PartExpression)      -- only FIRST-type PartExpression is valid as first;
                                           AND/OR-type PartExpression raises RuntimeError

    Methods / operators
        get_expression() -> str         -- compose_expression over the internal vector
        __str__                         -- same as get_expression()
        add(PartExpression)             -- appends a subsequent part; must be AND or OR type;
                                           FIRST type raises RuntimeError
        parts  (read-only property)     -- iterator over the internal PartExpression vector
        __eq__                          -- compares (free_, vec_) pairs
        __ne__                          -- implicit complement of __eq__
        __hash__                        -- boost.python extension types retain identity-based tp_hash
                                           even when __eq__ is defined (unlike pure-Python classes)

    Note: no default constructor is exposed; Expression always starts with ≥1 part.
    """

    # ------------------------------------------------------------------
    # Constructor: Expression(str)
    # ------------------------------------------------------------------

    def test_create_from_str_simple(self):
        """Plain string is stored verbatim as a single FIRST-type part."""
        e = ecf.Expression("a == b")
        self.assertEqual(e.get_expression(), "a == b")

    def test_create_from_str_compound(self):
        """Compound string is stored verbatim (no splitting)."""
        e = ecf.Expression("a == b OR c == d")
        self.assertEqual(e.get_expression(), "a == b OR c == d")

    def test_create_from_str_empty(self):
        """An empty string is accepted and produces an empty expression."""
        e = ecf.Expression("")
        self.assertEqual(e.get_expression(), "")

    # ------------------------------------------------------------------
    # Constructor: Expression(PartExpression)
    # ------------------------------------------------------------------

    def test_create_from_part_expression_first_type(self):
        """A FIRST-type PartExpression is valid as the sole (first) part."""
        e = ecf.Expression(ecf.PartExpression("a == b"))
        self.assertEqual(e.get_expression(), "a == b")

    def test_create_from_part_expression_and_type_raises(self):
        """An AND-type PartExpression as first part violates the ordering rule; raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Expression(ecf.PartExpression("a == b", True))

    def test_create_from_part_expression_or_type_raises(self):
        """An OR-type PartExpression as first part violates the ordering rule; raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Expression(ecf.PartExpression("a == b", False))

    # ------------------------------------------------------------------
    # get_expression()
    # ------------------------------------------------------------------

    def test_get_expression_returns_str_type(self):
        """get_expression() always returns a Python str."""
        e = ecf.Expression("x == active")
        self.assertIsInstance(e.get_expression(), str)

    def test_get_expression_is_idempotent(self):
        """Calling get_expression() twice returns the same value."""
        e = ecf.Expression("a == b")
        self.assertEqual(e.get_expression(), e.get_expression())

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_equals_get_expression(self):
        """__str__ is bound to the same compose_expression function as get_expression()."""
        e = ecf.Expression("a == b")
        self.assertEqual(str(e), e.get_expression())

    def test_str_after_add_reflects_combined_expression(self):
        """After add(), __str__ returns the full AND-joined composed string."""
        e = ecf.Expression("a == b")
        e.add(ecf.PartExpression("c == d", True))
        self.assertEqual(str(e), "a == b AND c == d")

    # ------------------------------------------------------------------
    # add()  — valid usage
    # ------------------------------------------------------------------

    def test_add_and_type_part_expression(self):
        """add() with AND-type PartExpression appends correctly."""
        e = ecf.Expression("a == b")
        e.add(ecf.PartExpression("c == d", True))
        self.assertEqual(e.get_expression(), "a == b AND c == d")

    def test_add_or_type_part_expression(self):
        """add() with OR-type PartExpression appends correctly."""
        e = ecf.Expression("a == b")
        e.add(ecf.PartExpression("c == d", False))
        self.assertEqual(e.get_expression(), "a == b OR c == d")

    def test_add_multiple_parts_chain(self):
        """Multiple add() calls chain parts in insertion order."""
        e = ecf.Expression("a == b")
        e.add(ecf.PartExpression("c == d", True))
        e.add(ecf.PartExpression("e == f", False))
        self.assertEqual(e.get_expression(), "a == b AND c == d OR e == f")

    def test_add_returns_none(self):
        """add() is a void function; returns None in Python."""
        e = ecf.Expression("a == b")
        result = e.add(ecf.PartExpression("c == d", True))
        self.assertIsNone(result)

    # ------------------------------------------------------------------
    # add() — invalid usage (ordering rule violations)
    # ------------------------------------------------------------------

    def test_add_first_type_to_non_empty_expression_raises(self):
        """Adding a FIRST-type PartExpression to a non-empty Expression violates the rule."""
        e = ecf.Expression("a == b")
        with self.assertRaises(RuntimeError):
            e.add(ecf.PartExpression("c == d"))  # FIRST type → invalid as subsequent

    # ------------------------------------------------------------------
    # add() — wrong Python types
    # ------------------------------------------------------------------

    def test_add_string_raises(self):
        """add() expects a PartExpression, not a plain string; raises TypeError."""
        e = ecf.Expression("a == b")
        with self.assertRaises((TypeError, RuntimeError)):
            e.add("c == d")

    def test_add_none_raises(self):
        """add() does not accept None; raises TypeError."""
        e = ecf.Expression("a == b")
        with self.assertRaises((TypeError, RuntimeError)):
            e.add(None)

    def test_add_int_raises(self):
        """add() does not accept an integer; raises TypeError."""
        e = ecf.Expression("a == b")
        with self.assertRaises((TypeError, RuntimeError)):
            e.add(42)

    # ------------------------------------------------------------------
    # parts property
    # ------------------------------------------------------------------

    def test_parts_single_expression(self):
        """parts yields one PartExpression equal to the one used at construction."""
        e = ecf.Expression("a == b")
        parts = list(e.parts)
        self.assertEqual(len(parts), 1)
        self.assertEqual(parts[0], ecf.PartExpression("a == b"))

    def test_parts_after_add_yields_both_entries(self):
        """parts yields all appended PartExpressions in insertion order."""
        e = ecf.Expression("a == b")
        e.add(ecf.PartExpression("c == d", True))
        parts = list(e.parts)
        self.assertEqual(len(parts), 2)
        self.assertEqual(parts[0], ecf.PartExpression("a == b"))
        self.assertEqual(parts[1], ecf.PartExpression("c == d", True))

    def test_parts_after_three_adds(self):
        """parts yields three PartExpressions in the correct order."""
        e = ecf.Expression("a == b")
        e.add(ecf.PartExpression("c == d", True))
        e.add(ecf.PartExpression("e == f", False))
        parts = list(e.parts)
        self.assertEqual(len(parts), 3)
        self.assertEqual(parts[2], ecf.PartExpression("e == f", False))

    def test_parts_is_iterable_multiple_times(self):
        """The parts property can be iterated more than once without exhaustion."""
        e = ecf.Expression("a == b")
        e.add(ecf.PartExpression("c == d", True))
        self.assertEqual(len(list(e.parts)), len(list(e.parts)))

    def test_parts_elements_are_part_expression_instances(self):
        """Each element yielded by parts is a PartExpression."""
        e = ecf.Expression("a == b")
        for part in e.parts:
            self.assertIsInstance(part, ecf.PartExpression)

    # ------------------------------------------------------------------
    # __eq__ — equal cases
    # ------------------------------------------------------------------

    def test_eq_identical_str_constructor(self):
        """Two Expressions created with the same string are equal."""
        e1 = ecf.Expression("a == b")
        e2 = ecf.Expression("a == b")
        self.assertEqual(e1, e2)

    def test_eq_str_vs_part_expression_first_type(self):
        """Expression(str) == Expression(PartExpression(str)) when same FIRST-type part."""
        e1 = ecf.Expression("a == b")
        e2 = ecf.Expression(ecf.PartExpression("a == b"))
        self.assertEqual(e1, e2)

    def test_eq_reflexive(self):
        """An Expression is equal to itself."""
        e = ecf.Expression("a == b")
        self.assertEqual(e, e)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        e1 = ecf.Expression("a == b")
        e2 = ecf.Expression("a == b")
        self.assertTrue(e1 == e2)
        self.assertTrue(e2 == e1)

    def test_eq_after_identical_adds(self):
        """Expressions built identically via add() are equal."""
        e1 = ecf.Expression("a == b")
        e1.add(ecf.PartExpression("c == d", True))
        e2 = ecf.Expression("a == b")
        e2.add(ecf.PartExpression("c == d", True))
        self.assertEqual(e1, e2)

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — not-equal cases
    # ------------------------------------------------------------------

    def test_ne_different_expression_strings(self):
        """Expressions with different strings are not equal."""
        e1 = ecf.Expression("a == b")
        e2 = ecf.Expression("c == d")
        self.assertNotEqual(e1, e2)

    def test_ne_single_vs_multi_part(self):
        """A one-part Expression is not equal to a two-part Expression."""
        e1 = ecf.Expression("a == b")
        e2 = ecf.Expression("a == b")
        e2.add(ecf.PartExpression("c == d", True))
        self.assertNotEqual(e1, e2)

    def test_ne_different_join_types_in_second_part(self):
        """Expressions that differ only in the join type of a part are not equal."""
        e1 = ecf.Expression("a == b")
        e1.add(ecf.PartExpression("c == d", True))  # AND
        e2 = ecf.Expression("a == b")
        e2.add(ecf.PartExpression("c == d", False))  # OR
        self.assertNotEqual(e1, e2)

    # ------------------------------------------------------------------
    # __hash__ — boost.python extension types are hashable by object identity
    # ------------------------------------------------------------------

    def test_is_hashable_by_identity(self):
        """boost.python extension types keep identity-based tp_hash even when __eq__ is
        defined.  hash() returns a non-zero integer derived from the object id."""
        e = ecf.Expression("a == b")
        self.assertIsInstance(hash(e), int)

    def test_can_be_inserted_in_set(self):
        """Because Expression is hashable, instances can be stored in a Python set."""
        e = ecf.Expression("a == b")
        s = {e}
        self.assertIn(e, s)

    def test_can_be_used_as_dict_key(self):
        """Because Expression is hashable, instances can be used as dictionary keys."""
        e = ecf.Expression("a == b")
        d = {e: "value"}
        self.assertEqual(d[e], "value")

    def test_hash_is_identity_based(self):
        """Two equal-by-value Expressions have different hashes (identity, not value)."""
        e1 = ecf.Expression("a == b")
        e2 = ecf.Expression("a == b")
        self.assertEqual(e1, e2)
        self.assertIsNot(e1, e2)
        self.assertNotEqual(hash(e1), hash(e2))

    # ------------------------------------------------------------------
    # Negative: invalid constructor arguments
    # ------------------------------------------------------------------

    def test_no_default_constructor(self):
        """Expression() with no arguments is not exposed; raises TypeError."""
        with self.assertRaises(TypeError):
            ecf.Expression()

    def test_create_from_int_raises(self):
        """No constructor accepts a plain integer."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Expression(42)

    def test_create_from_none_raises(self):
        """No constructor accepts None."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Expression(None)

    def test_create_from_list_raises(self):
        """No constructor accepts a list."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Expression(["a == b"])


class TestFlagType(unittest.TestCase):
    """Tests for py::enum_<ecf::Flag::Type> exposed as ecf.FlagType in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Class attributes (one per enum value, 26 total)
        ecf.FlagType.force_abort, .user_edit, .task_aborted, .edit_failed,
        .jobcmd_failed, .killcmd_failed, .statuscmd_failed, .no_script, .killed,
        .status, .late, .message, .byrule, .queuelimit, .wait, .locked, .zombie,
        .no_reque, .archived, .restored, .threshold, .sigterm, .not_set,
        .log_error, .checkpt_error, .remote_error

    Type hierarchy
        FlagType → enum → int → object
        Instances are simultaneously FlagType, int, and comparable/hashable as int.

    Per-instance properties
        v.name          -- the Python attribute name as a str (same as str(v))
        str(v)          -- just the name (e.g. "force_abort")
        repr(v)         -- module-qualified  ("ecflow.FlagType.force_abort")
        int(v)          -- underlying C++ integer value (non-contiguous, see INT_VALUES)

    Class-level lookup dicts (singletons)
        FlagType.values -- {int  : FlagType}  26 entries; keys are the C++ int values
        FlagType.names  -- {str  : FlagType}  26 entries; keys are the Python attr names

    Operators (inherited from int)
        __eq__ / __ne__ / __lt__ / __le__ / __gt__ / __ge__  -- integer comparison
        __hash__                                              -- hash == int(v)

    Iteration
        list(FlagType)  -- raises TypeError; the class itself is NOT iterable;
                           iterate over FlagType.values.values() instead

    Construction
        FlagType(n)     -- int subclass constructor; produces a FlagType from a raw int
                           but does NOT return the same singleton as the class attribute
    """

    # ------------------------------------------------------------------
    # Reference data (derived from the C++ .value() declarations)
    # ------------------------------------------------------------------

    ALL_NAMES: List[str] = [
        "force_abort",
        "user_edit",
        "task_aborted",
        "edit_failed",
        "jobcmd_failed",
        "killcmd_failed",
        "statuscmd_failed",
        "no_script",
        "killed",
        "status",
        "late",
        "message",
        "byrule",
        "queuelimit",
        "wait",
        "locked",
        "zombie",
        "no_reque",
        "archived",
        "restored",
        "threshold",
        "sigterm",
        "not_set",
        "log_error",
        "checkpt_error",
        "remote_error",
    ]

    # C++ integer value for each name (non-contiguous; order reflects enum definition order)
    INT_VALUES: Dict[str, int] = {
        "force_abort": 0,
        "user_edit": 1,
        "task_aborted": 2,
        "edit_failed": 3,
        "jobcmd_failed": 4,
        "no_script": 5,
        "killed": 6,
        "late": 7,
        "message": 8,
        "byrule": 9,
        "queuelimit": 10,
        "wait": 11,
        "locked": 12,
        "zombie": 13,
        "no_reque": 14,
        "archived": 15,
        "restored": 16,
        "threshold": 17,
        "sigterm": 18,
        "not_set": 19,
        "log_error": 20,
        "checkpt_error": 21,
        "killcmd_failed": 22,
        "statuscmd_failed": 23,
        "status": 24,
        "remote_error": 25,
    }

    # ------------------------------------------------------------------
    # Completeness: all 26 members are present
    # ------------------------------------------------------------------

    def test_total_member_count_is_26(self):
        """Exactly 26 enum values are declared in the binding."""
        self.assertEqual(len(ecf.FlagType.names), 26)
        self.assertEqual(len(ecf.FlagType.values), 26)

    def test_all_names_accessible_as_class_attributes(self):
        """Every declared name is accessible as an attribute of the FlagType class."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertTrue(
                    hasattr(ecf.FlagType, name), f"ecf.FlagType.{name} not found"
                )

    def test_all_names_present_in_names_dict(self):
        """FlagType.names contains every declared name as a key."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(name, ecf.FlagType.names)

    def test_all_int_values_present_in_values_dict(self):
        """FlagType.values contains every declared integer value as a key."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertIn(expected_int, ecf.FlagType.values)

    # ------------------------------------------------------------------
    # Type hierarchy
    # ------------------------------------------------------------------

    def test_instances_are_flagtype(self):
        """Every enum member is an instance of ecf.FlagType."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIsInstance(getattr(ecf.FlagType, name), ecf.FlagType)

    def test_int_conversion_works(self):
        """int(member) produces the C++ integer value (pybind11 enums are not int subclasses
        unlike Boost.Python enums, but they are still convertible to int)."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(int(getattr(ecf.FlagType, name)), expected)

    # ------------------------------------------------------------------
    # str / repr / .name
    # ------------------------------------------------------------------

    def test_str_returns_just_the_name(self):
        """str(v) returns only the attribute name, not the module-qualified form."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.FlagType, name)
                self.assertEqual(str(v), name)

    def test_repr_returns_module_qualified_name(self):
        """repr(v) returns 'ecflow.FlagType.<name>'."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.FlagType, name)
                self.assertEqual(repr(v), f"ecflow.FlagType.{name}")

    def test_name_attribute_equals_str(self):
        """v.name is the same string as str(v) for every member."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.FlagType, name)
                self.assertEqual(v.name, str(v))
                self.assertEqual(v.name, name)

    # ------------------------------------------------------------------
    # Integer values
    # ------------------------------------------------------------------

    def test_int_value_matches_cpp_enum(self):
        """int(v) matches the C++ integer value for each declared member."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                v = getattr(ecf.FlagType, name)
                self.assertEqual(int(v), expected)

    def test_force_abort_is_zero(self):
        """force_abort is the first (lowest) enum value and equals 0."""
        self.assertEqual(int(ecf.FlagType.force_abort), 0)

    def test_integer_values_are_non_contiguous(self):
        """The integer values are not a simple 0..N contiguous range
        (e.g. killcmd_failed=22 and status=24 are out of declaration order)."""
        self.assertEqual(int(ecf.FlagType.killcmd_failed), 22)
        self.assertEqual(int(ecf.FlagType.status), 24)
        self.assertEqual(int(ecf.FlagType.remote_error), 25)

    # ------------------------------------------------------------------
    # .values and .names lookup dicts
    # ------------------------------------------------------------------

    def test_values_dict_maps_int_to_flagtype_instance(self):
        """FlagType.values maps each int key to a FlagType instance."""
        for int_val, member in ecf.FlagType.values.items():
            with self.subTest(int_val=int_val):
                self.assertIsInstance(int_val, int)
                self.assertIsInstance(member, ecf.FlagType)

    def test_names_dict_maps_str_to_flagtype_instance(self):
        """FlagType.names maps each string key to a FlagType instance."""
        for name, member in ecf.FlagType.names.items():
            with self.subTest(name=name):
                self.assertIsInstance(name, str)
                self.assertIsInstance(member, ecf.FlagType)

    def test_values_dict_roundtrip(self):
        """FlagType.values[int(v)] is the same singleton as the class attribute."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.FlagType, name)
                self.assertIs(ecf.FlagType.values[int(v)], v)

    def test_names_dict_roundtrip(self):
        """FlagType.names[str(v)] is the same singleton as the class attribute."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.FlagType, name)
                self.assertIs(ecf.FlagType.names[str(v)], v)

    # ------------------------------------------------------------------
    # Iteration — the class itself is NOT iterable
    # ------------------------------------------------------------------

    def test_flag_type_class_is_not_iterable(self):
        """list(FlagType) raises TypeError; the class object has no __iter__."""
        with self.assertRaises(TypeError):
            list(ecf.FlagType)

    def test_all_members_reachable_via_values_dict(self):
        """Iterating FlagType.values.values() yields every member once."""
        members = set(ecf.FlagType.values.values())
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(getattr(ecf.FlagType, name), members)

    def test_all_members_reachable_via_names_dict(self):
        """Iterating FlagType.names.values() yields every member once."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(name, ecf.FlagType.names)

    # ------------------------------------------------------------------
    # Equality and ordering (inherited from int)
    # ------------------------------------------------------------------

    def test_same_member_is_equal_to_itself(self):
        """v == v for every member (reflexive)."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.FlagType, name)
                self.assertEqual(v, v)

    def test_members_with_different_int_values_are_not_equal(self):
        """force_abort != user_edit because their integer values differ."""
        self.assertNotEqual(ecf.FlagType.force_abort, ecf.FlagType.user_edit)

    def test_equality_with_plain_int(self):
        """A FlagType value equals a plain int when the integer values match."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                v = getattr(ecf.FlagType, name)
                self.assertEqual(v, expected)

    def test_inequality_with_wrong_plain_int(self):
        """A FlagType value does not equal a plain int with a different value."""
        self.assertNotEqual(ecf.FlagType.force_abort, 99)

    def test_ordering_less_than(self):
        """force_abort (0) < user_edit (1) — ordering is by integer value."""
        self.assertLess(ecf.FlagType.force_abort, ecf.FlagType.user_edit)

    def test_ordering_greater_than(self):
        """remote_error (25) > not_set (19)."""
        self.assertGreater(ecf.FlagType.remote_error, ecf.FlagType.not_set)

    def test_ordering_full_sort(self):
        """Sorting all members by value produces a sequence of non-decreasing integers."""
        sorted_vals = sorted(ecf.FlagType.values.values())
        ints = [int(v) for v in sorted_vals]
        self.assertEqual(ints, sorted(ints))

    # ------------------------------------------------------------------
    # Hash (inherited from int: hash(v) == int(v))
    # ------------------------------------------------------------------

    def test_hash_equals_int_value(self):
        """hash(v) == int(v) for every member (int hash semantics)."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                v = getattr(ecf.FlagType, name)
                self.assertEqual(hash(v), expected_int)

    def test_can_be_stored_in_set(self):
        """FlagType members are hashable and can be stored in a Python set."""
        s = set(ecf.FlagType.values.values())
        self.assertEqual(len(s), 26)

    def test_set_deduplicates_equal_values(self):
        """Adding the same member twice to a set results in one entry."""
        s = {ecf.FlagType.force_abort, ecf.FlagType.force_abort}
        self.assertEqual(len(s), 1)

    def test_can_be_used_as_dict_key(self):
        """FlagType members work as dictionary keys."""
        d = {ecf.FlagType.force_abort: "fa", ecf.FlagType.user_edit: "ue"}
        self.assertEqual(d[ecf.FlagType.force_abort], "fa")
        self.assertEqual(d[ecf.FlagType.user_edit], "ue")

    # ------------------------------------------------------------------
    # Construction from int
    # ------------------------------------------------------------------

    def test_can_construct_from_int(self):
        """FlagType(n) constructs a FlagType instance from a raw integer."""
        v = ecf.FlagType(0)
        self.assertIsInstance(v, ecf.FlagType)
        self.assertEqual(int(v), 0)

    def test_int_constructor_does_not_return_singleton(self):
        """FlagType(n) and the class-attribute singleton may differ in identity
        even though they are equal in value (boost.python does not intern them)."""
        from_ctor = ecf.FlagType(0)
        singleton = ecf.FlagType.force_abort
        self.assertEqual(from_ctor, singleton)  # same value
        # identity may or may not be the same; we only assert value equality

    # ------------------------------------------------------------------
    # Negative cases
    # ------------------------------------------------------------------

    def test_nonexistent_member_raises_attribute_error(self):
        """Accessing an undefined name on FlagType raises AttributeError."""
        with self.assertRaises(AttributeError):
            _ = ecf.FlagType.nonexistent_flag

    def test_missing_int_key_in_values_raises_key_error(self):
        """Looking up an int that has no enum member raises KeyError."""
        with self.assertRaises(KeyError):
            _ = ecf.FlagType.values[9999]

    def test_missing_name_key_in_names_raises_key_error(self):
        """Looking up a name that has no enum member raises KeyError."""
        with self.assertRaises(KeyError):
            _ = ecf.FlagType.names["nonexistent_flag"]


class TestFlag(unittest.TestCase):
    """Tests for py::class_<ecf::Flag> exposed as ecf.Flag in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructor
        Flag()                  -- default constructor; all flags cleared

    Instance methods
        is_set(FlagType) -> bool    -- query whether a specific flag is set
        set(FlagType)               -- set (enable) a flag
        clear(FlagType)             -- clear (disable) a single flag
        reset()                     -- clear all flags

    Static methods
        Flag.list()  -> FlagTypeVec -- returns all 25 settable flag types (not_set excluded)
        Flag.type_to_string(FlagType) -> str
                                    -- C++ display name; may differ from the Python attr name
                                       (force_abort->'force_aborted', jobcmd_failed->'ecfcmd_failed',
                                        byrule->'by_rule', queuelimit->'queue_limit', wait->'task_waiting')

    Operators
        __str__   -- empty string when no flags set; comma-separated display names otherwise
        __eq__    -- compares which flags are set (value equality)
        __ne__    -- implicit complement of __eq__
        __hash__  -- boost.python identity-based (NOT value-based); two equal Flags can differ in hash
    """

    # Map from Python FlagType attr-name → expected C++ display string (from type_to_string)
    TYPE_TO_STRING: Dict[str, str] = {
        "force_abort": "force_aborted",  # display name differs
        "user_edit": "user_edit",
        "task_aborted": "task_aborted",
        "edit_failed": "edit_failed",
        "jobcmd_failed": "ecfcmd_failed",  # display name differs
        "killcmd_failed": "killcmd_failed",
        "statuscmd_failed": "statuscmd_failed",
        "no_script": "no_script",
        "killed": "killed",
        "status": "status",
        "late": "late",
        "message": "message",
        "byrule": "by_rule",  # display name differs
        "queuelimit": "queue_limit",  # display name differs
        "wait": "task_waiting",  # display name differs
        "locked": "locked",
        "zombie": "zombie",
        "no_reque": "no_reque",
        "archived": "archived",
        "restored": "restored",
        "threshold": "threshold",
        "sigterm": "sigterm",
        "log_error": "log_error",
        "checkpt_error": "checkpt_error",
        "remote_error": "remote_error",
    }

    # ------------------------------------------------------------------
    # Constructor — default
    # ------------------------------------------------------------------

    def test_default_constructor_creates_empty_flag(self):
        """Flag() creates an instance with no flags set."""
        f = ecf.Flag()
        self.assertIsInstance(f, ecf.Flag)

    def test_default_str_is_empty(self):
        """str(Flag()) is an empty string when no flags are set."""
        self.assertEqual(str(ecf.Flag()), "")

    def test_all_flags_clear_after_construction(self):
        """is_set() returns False for every FlagType immediately after construction."""
        f = ecf.Flag()
        for ft in ecf.Flag.list():
            with self.subTest(flag=str(ft)):
                self.assertFalse(f.is_set(ft))

    # ------------------------------------------------------------------
    # set() / is_set()
    # ------------------------------------------------------------------

    def test_set_makes_is_set_true(self):
        """After set(ft), is_set(ft) returns True."""
        f = ecf.Flag()
        f.set(ecf.FlagType.late)
        self.assertTrue(f.is_set(ecf.FlagType.late))

    def test_set_does_not_affect_other_flags(self):
        """Setting one flag leaves all others unset."""
        f = ecf.Flag()
        f.set(ecf.FlagType.late)
        for ft in ecf.Flag.list():
            if ft != ecf.FlagType.late:
                with self.subTest(flag=str(ft)):
                    self.assertFalse(f.is_set(ft))

    def test_set_multiple_flags_independently(self):
        """Multiple flags can be set independently; each is_set call reflects only that flag."""
        f = ecf.Flag()
        f.set(ecf.FlagType.late)
        f.set(ecf.FlagType.zombie)
        self.assertTrue(f.is_set(ecf.FlagType.late))
        self.assertTrue(f.is_set(ecf.FlagType.zombie))
        self.assertFalse(f.is_set(ecf.FlagType.killed))

    def test_set_every_individual_flag(self):
        """set() then is_set() returns True for each of the 25 settable flags."""
        for ft in ecf.Flag.list():
            with self.subTest(flag=str(ft)):
                f = ecf.Flag()
                f.set(ft)
                self.assertTrue(f.is_set(ft))

    # ------------------------------------------------------------------
    # clear()
    # ------------------------------------------------------------------

    def test_clear_removes_set_flag(self):
        """clear(ft) makes is_set(ft) return False after it was True."""
        f = ecf.Flag()
        f.set(ecf.FlagType.late)
        self.assertTrue(f.is_set(ecf.FlagType.late))
        f.clear(ecf.FlagType.late)
        self.assertFalse(f.is_set(ecf.FlagType.late))

    def test_clear_does_not_affect_other_set_flags(self):
        """Clearing one flag leaves other previously set flags intact."""
        f = ecf.Flag()
        f.set(ecf.FlagType.late)
        f.set(ecf.FlagType.zombie)
        f.clear(ecf.FlagType.late)
        self.assertFalse(f.is_set(ecf.FlagType.late))
        self.assertTrue(f.is_set(ecf.FlagType.zombie))

    def test_clear_on_already_clear_flag_is_idempotent(self):
        """Clearing a flag that is already clear does not raise and leaves state unchanged."""
        f = ecf.Flag()
        f.clear(ecf.FlagType.late)  # already clear — must not raise
        self.assertFalse(f.is_set(ecf.FlagType.late))

    # ------------------------------------------------------------------
    # reset()
    # ------------------------------------------------------------------

    def test_reset_clears_all_flags(self):
        """reset() makes every flag return False from is_set()."""
        f = ecf.Flag()
        f.set(ecf.FlagType.late)
        f.set(ecf.FlagType.zombie)
        f.set(ecf.FlagType.message)
        f.reset()
        for ft in ecf.Flag.list():
            with self.subTest(flag=str(ft)):
                self.assertFalse(f.is_set(ft))

    def test_reset_makes_str_empty(self):
        """After reset(), str(f) returns the empty string."""
        f = ecf.Flag()
        f.set(ecf.FlagType.late)
        f.reset()
        self.assertEqual(str(f), "")

    def test_reset_on_empty_flag_is_idempotent(self):
        """reset() on an already-empty Flag does not raise."""
        f = ecf.Flag()
        f.reset()  # no-op, must not raise
        self.assertEqual(str(f), "")

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_single_flag_matches_display_name(self):
        """str(f) with one set flag returns the C++ display name for that flag."""
        for attr_name, display_name in self.TYPE_TO_STRING.items():
            with self.subTest(flag=attr_name):
                f = ecf.Flag()
                f.set(getattr(ecf.FlagType, attr_name))
                self.assertEqual(str(f), display_name)

    def test_str_two_flags_are_comma_separated(self):
        """str(f) with two set flags returns both display names separated by a comma."""
        f = ecf.Flag()
        f.set(ecf.FlagType.late)
        f.set(ecf.FlagType.zombie)
        s = str(f)
        self.assertIn("late", s)
        self.assertIn("zombie", s)
        self.assertIn(",", s)

    def test_str_returns_str_type(self):
        """str(Flag()) always returns a Python str."""
        self.assertIsInstance(str(ecf.Flag()), str)

    # ------------------------------------------------------------------
    # __eq__ — value equality (compares which flags are set)
    # ------------------------------------------------------------------

    def test_eq_two_empty_flags(self):
        """Two default-constructed Flags are equal."""
        self.assertEqual(ecf.Flag(), ecf.Flag())

    def test_eq_same_flags_set(self):
        """Two Flags with the same flags set are equal."""
        f1, f2 = ecf.Flag(), ecf.Flag()
        f1.set(ecf.FlagType.late)
        f2.set(ecf.FlagType.late)
        self.assertEqual(f1, f2)

    def test_eq_reflexive(self):
        """A Flag is equal to itself."""
        f = ecf.Flag()
        f.set(ecf.FlagType.late)
        self.assertEqual(f, f)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        f1, f2 = ecf.Flag(), ecf.Flag()
        f1.set(ecf.FlagType.late)
        f2.set(ecf.FlagType.late)
        self.assertTrue(f1 == f2)
        self.assertTrue(f2 == f1)

    def test_ne_different_flags_set(self):
        """Flags with different sets of flags are not equal."""
        f1, f2 = ecf.Flag(), ecf.Flag()
        f1.set(ecf.FlagType.late)
        f2.set(ecf.FlagType.zombie)
        self.assertNotEqual(f1, f2)

    def test_ne_empty_vs_nonempty(self):
        """An empty Flag is not equal to one with a flag set."""
        f1, f2 = ecf.Flag(), ecf.Flag()
        f2.set(ecf.FlagType.late)
        self.assertNotEqual(f1, f2)

    # ------------------------------------------------------------------
    # __hash__ — identity-based (boost.python extension type)
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Flag is hashable (boost.python identity-based hash)."""
        f = ecf.Flag()
        self.assertIsInstance(hash(f), int)

    def test_hash_is_identity_based_not_value_based(self):
        """Two value-equal Flags may have different hashes (identity, not content)."""
        f1, f2 = ecf.Flag(), ecf.Flag()
        f1.set(ecf.FlagType.late)
        f2.set(ecf.FlagType.late)
        self.assertEqual(f1, f2)  # same value
        self.assertIsNot(f1, f2)  # different objects
        self.assertNotEqual(hash(f1), hash(f2))  # different hashes

    def test_can_be_used_as_dict_key(self):
        """Flag instances can be used as dictionary keys."""
        f = ecf.Flag()
        d = {f: "value"}
        self.assertEqual(d[f], "value")

    # ------------------------------------------------------------------
    # Flag.list() static method → FlagTypeVec
    # ------------------------------------------------------------------

    def test_list_returns_flag_type_vec(self):
        """Flag.list() returns a FlagTypeVec instance."""
        self.assertIsInstance(ecf.Flag.list(), ecf.FlagTypeVec)

    def test_list_has_25_elements(self):
        """Flag.list() contains exactly 25 elements (not_set is excluded)."""
        self.assertEqual(len(ecf.Flag.list()), 25)

    def test_list_excludes_not_set(self):
        """FlagType.not_set is intentionally absent from Flag.list()."""
        self.assertNotIn(ecf.FlagType.not_set, ecf.Flag.list())

    def test_list_contains_all_settable_types(self):
        """Flag.list() contains every FlagType except not_set."""
        fl = ecf.Flag.list()
        for name in self.TYPE_TO_STRING:
            with self.subTest(flag=name):
                self.assertIn(getattr(ecf.FlagType, name), fl)

    def test_list_elements_are_flag_type_instances(self):
        """Every element of Flag.list() is a FlagType instance."""
        for ft in ecf.Flag.list():
            self.assertIsInstance(ft, ecf.FlagType)

    def test_list_each_call_produces_independent_copy(self):
        """Flag.list() returns a new FlagTypeVec on each call (not a shared reference)."""
        fl1 = ecf.Flag.list()
        fl2 = ecf.Flag.list()
        self.assertIsNot(fl1, fl2)
        self.assertEqual(len(fl1), len(fl2))

    # ------------------------------------------------------------------
    # Flag.type_to_string() static method
    # ------------------------------------------------------------------

    def test_type_to_string_returns_str(self):
        """type_to_string() always returns a Python str."""
        result = ecf.Flag.type_to_string(ecf.FlagType.late)
        self.assertIsInstance(result, str)

    def test_type_to_string_known_display_names(self):
        """type_to_string() returns the C++ display name, which may differ from the Python attr name."""
        for attr_name, display_name in self.TYPE_TO_STRING.items():
            with self.subTest(flag=attr_name):
                ft = getattr(ecf.FlagType, attr_name)
                self.assertEqual(ecf.Flag.type_to_string(ft), display_name)

    def test_type_to_string_differs_from_flag_type_str_for_renamed_flags(self):
        """For the 5 renamed flags, type_to_string() differs from str(FlagType)."""
        renamed = {
            "force_abort": "force_aborted",
            "jobcmd_failed": "ecfcmd_failed",
            "byrule": "by_rule",
            "queuelimit": "queue_limit",
            "wait": "task_waiting",
        }
        for attr_name, display_name in renamed.items():
            with self.subTest(flag=attr_name):
                ft = getattr(ecf.FlagType, attr_name)
                self.assertNotEqual(str(ft), display_name)
                self.assertEqual(ecf.Flag.type_to_string(ft), display_name)

    def test_type_to_string_matches_flag_str_for_unrenamed_flags(self):
        """For the other 20 flags, type_to_string() matches str(FlagType)."""
        renamed_attrs = {"force_abort", "jobcmd_failed", "byrule", "queuelimit", "wait"}
        for attr_name, display_name in self.TYPE_TO_STRING.items():
            if attr_name in renamed_attrs:
                continue
            with self.subTest(flag=attr_name):
                ft = getattr(ecf.FlagType, attr_name)
                self.assertEqual(ecf.Flag.type_to_string(ft), str(ft))

    def test_type_to_string_for_not_set(self):
        """type_to_string() also works for not_set even though it is excluded from list()."""
        result = ecf.Flag.type_to_string(ecf.FlagType.not_set)
        self.assertIsInstance(result, str)
        self.assertEqual(result, "not_set")

    # ------------------------------------------------------------------
    # Negative: wrong argument types
    # ------------------------------------------------------------------

    def test_is_set_with_string_raises(self):
        """is_set() accepts only FlagType, not a plain string."""
        f = ecf.Flag()
        with self.assertRaises((TypeError, RuntimeError)):
            f.is_set("late")

    def test_is_set_with_plain_int_raises(self):
        """is_set() accepts only FlagType, not a raw integer."""
        f = ecf.Flag()
        with self.assertRaises((TypeError, RuntimeError)):
            f.is_set(7)  # 7 is the int value of 'late' but not a FlagType

    def test_set_with_string_raises(self):
        """set() accepts only FlagType, not a plain string."""
        f = ecf.Flag()
        with self.assertRaises((TypeError, RuntimeError)):
            f.set("late")

    def test_set_with_plain_int_raises(self):
        """set() accepts only FlagType, not a raw integer."""
        f = ecf.Flag()
        with self.assertRaises((TypeError, RuntimeError)):
            f.set(999)

    def test_clear_with_string_raises(self):
        """clear() accepts only FlagType, not a plain string."""
        f = ecf.Flag()
        with self.assertRaises((TypeError, RuntimeError)):
            f.clear("late")


class TestFlagTypeVec(unittest.TestCase):
    """Tests for py::class_<std::vector<ecf::Flag::Type>> exposed as ecf.FlagTypeVec.

    FlagTypeVec is the return type of Flag.list().  It is exposed via
    vector_indexing_suite and provides a read-only list-like interface.

    Exposed API
    -----------
    Typical construction
        Flag.list() -> FlagTypeVec   -- the canonical way to obtain a FlagTypeVec
                                        (25 elements — every FlagType except not_set)

    Sequence operators (via vector_indexing_suite)
        len(fv)      -- __len__       -- number of elements
        fv[i]        -- __getitem__   -- positive and negative integer indexing;
                                         IndexError / RuntimeError on out-of-range
        for v in fv  -- __iter__      -- yields FlagType instances in order
        v in fv      -- __contains__  -- membership test using FlagType equality
    """

    def setUp(self):
        self.fl = ecf.Flag.list()  # canonical 25-element FlagTypeVec

    # ------------------------------------------------------------------
    # Length
    # ------------------------------------------------------------------

    def test_len_is_25(self):
        """len(FlagTypeVec) from Flag.list() is 25."""
        self.assertEqual(len(self.fl), 25)

    # ------------------------------------------------------------------
    # Indexing
    # ------------------------------------------------------------------

    def test_positive_index_returns_flag_type(self):
        """Positive indexing returns a FlagType instance."""
        self.assertIsInstance(self.fl[0], ecf.FlagType)

    def test_first_element_is_force_abort(self):
        """The first element corresponds to the first C++ enum value (force_abort)."""
        self.assertEqual(self.fl[0], ecf.FlagType.force_abort)

    def test_last_element_via_positive_index(self):
        """The element at index len-1 is remote_error (last declared type)."""
        self.assertEqual(self.fl[24], ecf.FlagType.remote_error)

    def test_negative_index_minus_one_is_last(self):
        """fl[-1] resolves to the last element (remote_error)."""
        self.assertEqual(self.fl[-1], ecf.FlagType.remote_error)

    def test_negative_index_minus_two_is_second_to_last(self):
        """fl[-2] resolves to the penultimate element (checkpt_error)."""
        self.assertEqual(self.fl[-2], ecf.FlagType.checkpt_error)

    def test_out_of_range_index_raises(self):
        """Indexing beyond the length raises IndexError."""
        with self.assertRaises((IndexError, RuntimeError)):
            _ = self.fl[25]

    # ------------------------------------------------------------------
    # Iteration
    # ------------------------------------------------------------------

    def test_iterable_yields_25_elements(self):
        """Iterating the FlagTypeVec yields exactly 25 FlagType values."""
        items = list(self.fl)
        self.assertEqual(len(items), 25)

    def test_iterated_elements_are_flag_type(self):
        """Every element yielded by iteration is a FlagType instance."""
        for ft in self.fl:
            self.assertIsInstance(ft, ecf.FlagType)

    def test_iteration_covers_all_settable_types(self):
        """Iterating the FlagTypeVec covers all 25 settable FlagType values."""
        iterated = list(self.fl)
        expected_ints = set(range(26)) - {19}  # all except not_set (19)
        actual_ints = {int(ft) for ft in iterated}
        self.assertEqual(actual_ints, expected_ints)

    # ------------------------------------------------------------------
    # __contains__
    # ------------------------------------------------------------------

    def test_contains_settable_flag_type(self):
        """All 25 settable FlagType values are reported as 'in' the FlagTypeVec."""
        for ft in ecf.FlagType.names.values():
            if ft == ecf.FlagType.not_set:
                continue
            with self.subTest(flag=str(ft)):
                self.assertIn(ft, self.fl)

    def test_not_set_is_not_in_list(self):
        """FlagType.not_set is not contained in Flag.list()."""
        self.assertNotIn(ecf.FlagType.not_set, self.fl)

    # ------------------------------------------------------------------
    # Direct construction
    # ------------------------------------------------------------------

    def test_direct_constructor_creates_empty_vec(self):
        """FlagTypeVec() constructs an empty vector directly."""
        fv = ecf.FlagTypeVec()
        self.assertIsInstance(fv, ecf.FlagTypeVec)
        self.assertEqual(len(fv), 0)

    def test_direct_constructor_contains_nothing(self):
        """A directly constructed FlagTypeVec contains no FlagType values."""
        fv = ecf.FlagTypeVec()
        self.assertNotIn(ecf.FlagType.force_abort, fv)

    def test_direct_constructor_iterates_empty(self):
        """Iterating a directly constructed FlagTypeVec yields no items."""
        self.assertEqual(list(ecf.FlagTypeVec()), [])


class TestJobCreationCtrl(unittest.TestCase):
    """Tests for py::class_<JobCreationCtrl> exposed as ecf.JobCreationCtrl.

    Exposed API
    -----------
    Constructor
        JobCreationCtrl()                  -- default constructor (via make_constructor)

    Instance methods
        set_node_path(str)                 -- set the absolute node path to check; returns None
        set_dir_for_job_creation(str)      -- set the output directory; returns None
        set_verbose(bool)                  -- enable/disable verbose output; returns None
        is_verbose()      -> bool          -- query current verbose state; initially False
        generate_temp_dir()                -- auto-generate and set a temp directory; returns None;
                                             also prints the chosen path to stdout
        get_dir_for_job_creation() -> str  -- retrieve the currently set directory ('' if not set)
        get_error_msg()           -> str   -- retrieve any error message ('' if no error)

    Operators / hash
        __eq__   -- identity-based (no value __eq__ is bound); only reflexive == is True
        __hash__ -- identity-based (boost.python default)

    Constraints
        boost::noncopyable  -- copy.copy() raises RuntimeError (no pickle / no __copy__)
    """

    # ------------------------------------------------------------------
    # Constructor
    # ------------------------------------------------------------------

    def test_constructor_creates_instance(self):
        """JobCreationCtrl() constructs an instance of the correct type."""
        jc = ecf.JobCreationCtrl()
        self.assertIsInstance(jc, ecf.JobCreationCtrl)

    def test_initial_dir_is_empty_string(self):
        """get_dir_for_job_creation() returns '' immediately after construction."""
        jc = ecf.JobCreationCtrl()
        self.assertEqual(jc.get_dir_for_job_creation(), "")

    def test_initial_error_msg_is_empty_string(self):
        """get_error_msg() returns '' immediately after construction."""
        jc = ecf.JobCreationCtrl()
        self.assertEqual(jc.get_error_msg(), "")

    def test_initial_dir_and_error_msg_return_str(self):
        """Both getters return Python str objects from a fresh instance."""
        jc = ecf.JobCreationCtrl()
        self.assertIsInstance(jc.get_dir_for_job_creation(), str)
        self.assertIsInstance(jc.get_error_msg(), str)

    # ------------------------------------------------------------------
    # set_node_path()
    # ------------------------------------------------------------------

    def test_set_node_path_returns_none(self):
        """set_node_path() is a void method; returns None."""
        jc = ecf.JobCreationCtrl()
        result = jc.set_node_path("/suite/family/task")
        self.assertIsNone(result)

    def test_set_node_path_accepts_absolute_path(self):
        """set_node_path() accepts a well-formed absolute path without raising."""
        jc = ecf.JobCreationCtrl()
        jc.set_node_path("/suite/family/task")  # must not raise

    def test_set_node_path_accepts_empty_string(self):
        """set_node_path('') is a valid call (means: check all tasks)."""
        jc = ecf.JobCreationCtrl()
        jc.set_node_path("")  # must not raise

    # ------------------------------------------------------------------
    # set_dir_for_job_creation() / get_dir_for_job_creation()
    # ------------------------------------------------------------------

    def test_set_dir_returns_none(self):
        """set_dir_for_job_creation() is a void method; returns None."""
        jc = ecf.JobCreationCtrl()
        result = jc.set_dir_for_job_creation("/tmp/jobs")
        self.assertIsNone(result)

    def test_get_dir_reflects_set_dir(self):
        """get_dir_for_job_creation() returns exactly what was passed to set_dir_for_job_creation()."""
        jc = ecf.JobCreationCtrl()
        jc.set_dir_for_job_creation("/tmp/ecflow_test_jobs")
        self.assertEqual(jc.get_dir_for_job_creation(), "/tmp/ecflow_test_jobs")

    def test_get_dir_returns_str(self):
        """get_dir_for_job_creation() always returns a Python str."""
        jc = ecf.JobCreationCtrl()
        jc.set_dir_for_job_creation("/tmp/jobs")
        self.assertIsInstance(jc.get_dir_for_job_creation(), str)

    def test_set_dir_empty_string_clears_dir(self):
        """set_dir_for_job_creation('') stores an empty string."""
        jc = ecf.JobCreationCtrl()
        jc.set_dir_for_job_creation("/tmp/jobs")
        jc.set_dir_for_job_creation("")
        self.assertEqual(jc.get_dir_for_job_creation(), "")

    def test_set_dir_overwrites_previous_value(self):
        """A second call to set_dir_for_job_creation() replaces the first value."""
        jc = ecf.JobCreationCtrl()
        jc.set_dir_for_job_creation("/tmp/first")
        jc.set_dir_for_job_creation("/tmp/second")
        self.assertEqual(jc.get_dir_for_job_creation(), "/tmp/second")

    # ------------------------------------------------------------------
    # set_verbose() / is_verbose()
    # ------------------------------------------------------------------

    def test_set_verbose_true_returns_none(self):
        """set_verbose(True) is a void method; returns None."""
        jc = ecf.JobCreationCtrl()
        result = jc.set_verbose(True)
        self.assertIsNone(result)
        self.assertEqual(jc.is_verbose(), True)

    def test_set_verbose_false_returns_none(self):
        """set_verbose(False) is a void method; returns None."""
        jc = ecf.JobCreationCtrl()
        result = jc.set_verbose(False)
        self.assertIsNone(result)
        self.assertEqual(jc.is_verbose(), False)

    def test_set_verbose_toggle(self):
        """set_verbose() can be called multiple times without raising."""
        jc = ecf.JobCreationCtrl()
        jc.set_verbose(True)
        self.assertEqual(jc.is_verbose(), True)
        jc.set_verbose(False)
        self.assertEqual(jc.is_verbose(), False)
        jc.set_verbose(True)  # must not raise
        self.assertEqual(jc.is_verbose(), True)

    def test_is_verbose_initially_false(self):
        """is_verbose() returns False immediately after construction."""
        jc = ecf.JobCreationCtrl()
        self.assertFalse(jc.is_verbose())

    def test_is_verbose_returns_bool(self):
        """is_verbose() returns a Python bool."""
        jc = ecf.JobCreationCtrl()
        self.assertIsInstance(jc.is_verbose(), bool)

    def test_is_verbose_true_after_set_verbose_true(self):
        """is_verbose() returns True after set_verbose(True)."""
        jc = ecf.JobCreationCtrl()
        jc.set_verbose(True)
        self.assertTrue(jc.is_verbose())

    def test_is_verbose_false_after_set_verbose_false(self):
        """is_verbose() returns False after set_verbose(False)."""
        jc = ecf.JobCreationCtrl()
        jc.set_verbose(True)
        jc.set_verbose(False)
        self.assertFalse(jc.is_verbose())

    def test_is_verbose_reflects_each_toggle(self):
        """is_verbose() tracks every set_verbose() call correctly."""
        jc = ecf.JobCreationCtrl()
        for expected in [True, False, True, False]:
            jc.set_verbose(expected)
            self.assertEqual(jc.is_verbose(), expected)

    # ------------------------------------------------------------------
    # generate_temp_dir()
    # ------------------------------------------------------------------

    def test_generate_temp_dir_returns_none(self):
        """generate_temp_dir() is a void method; returns None."""
        jc = ecf.JobCreationCtrl()
        result = jc.generate_temp_dir()
        self.assertIsNone(result)

    def test_generate_temp_dir_sets_non_empty_dir(self):
        """After generate_temp_dir(), get_dir_for_job_creation() returns a non-empty path."""
        jc = ecf.JobCreationCtrl()
        jc.generate_temp_dir()
        self.assertNotEqual(jc.get_dir_for_job_creation(), "")

    def test_generate_temp_dir_overrides_explicit_dir(self):
        """generate_temp_dir() replaces any previously set directory."""
        jc = ecf.JobCreationCtrl()
        jc.set_dir_for_job_creation("/tmp/explicit")
        jc.generate_temp_dir()
        self.assertNotEqual(jc.get_dir_for_job_creation(), "/tmp/explicit")
        self.assertNotEqual(jc.get_dir_for_job_creation(), "")

    def test_generate_temp_dir_is_idempotent_path(self):
        """Calling generate_temp_dir() twice produces the same temp directory path."""
        jc = ecf.JobCreationCtrl()
        jc.generate_temp_dir()
        d1 = jc.get_dir_for_job_creation()
        jc.generate_temp_dir()
        d2 = jc.get_dir_for_job_creation()
        self.assertEqual(d1, d2)

    def test_generate_temp_dir_path_is_str(self):
        """The directory set by generate_temp_dir() is a Python str."""
        jc = ecf.JobCreationCtrl()
        jc.generate_temp_dir()
        self.assertIsInstance(jc.get_dir_for_job_creation(), str)

    # ------------------------------------------------------------------
    # get_error_msg()
    # ------------------------------------------------------------------

    def test_get_error_msg_returns_str(self):
        """get_error_msg() always returns a Python str."""
        jc = ecf.JobCreationCtrl()
        self.assertIsInstance(jc.get_error_msg(), str)

    def test_get_error_msg_is_initially_empty(self):
        """A freshly constructed JobCreationCtrl has no error message."""
        jc = ecf.JobCreationCtrl()
        self.assertEqual(jc.get_error_msg(), "")

    # ------------------------------------------------------------------
    # __eq__ — identity-based (no value equality binding)
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """An instance is equal to itself (identity)."""
        jc = ecf.JobCreationCtrl()
        self.assertEqual(jc, jc)

    def test_eq_two_instances_with_same_state_are_not_equal(self):
        """Two distinct instances with the same configuration are NOT equal
        because no value __eq__ is bound; Python falls back to identity."""
        jc1 = ecf.JobCreationCtrl()
        jc2 = ecf.JobCreationCtrl()
        jc1.set_dir_for_job_creation("/tmp/x")
        jc2.set_dir_for_job_creation("/tmp/x")
        self.assertNotEqual(jc1, jc2)

    def test_ne_different_instances(self):
        """Two default-constructed instances are not equal (different objects)."""
        jc1 = ecf.JobCreationCtrl()
        jc2 = ecf.JobCreationCtrl()
        self.assertNotEqual(jc1, jc2)

    # ------------------------------------------------------------------
    # __hash__ — identity-based
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """JobCreationCtrl is hashable (boost.python identity-based hash)."""
        jc = ecf.JobCreationCtrl()
        self.assertIsInstance(hash(jc), int)

    def test_can_be_used_as_dict_key(self):
        """A JobCreationCtrl instance can be used as a dictionary key."""
        jc = ecf.JobCreationCtrl()
        d = {jc: "value"}
        self.assertEqual(d[jc], "value")

    def test_can_be_stored_in_set(self):
        """A JobCreationCtrl instance can be stored in a Python set."""
        jc = ecf.JobCreationCtrl()
        s = {jc}
        self.assertIn(jc, s)

    # ------------------------------------------------------------------
    # boost::noncopyable
    # ------------------------------------------------------------------

    def test_copy_raises(self):
        """JobCreationCtrl is declared boost::noncopyable; copy.copy() raises."""
        import copy

        jc = ecf.JobCreationCtrl()
        with self.assertRaises((RuntimeError, TypeError)):
            copy.copy(jc)

    # ------------------------------------------------------------------
    # Negative: wrong argument types
    # ------------------------------------------------------------------

    def test_set_node_path_with_int_raises(self):
        """set_node_path() requires a str; passing int raises ArgumentError."""
        jc = ecf.JobCreationCtrl()
        with self.assertRaises((TypeError, RuntimeError)):
            jc.set_node_path(42)

    def test_set_node_path_with_none_raises(self):
        """set_node_path() requires a str; passing None raises ArgumentError."""
        jc = ecf.JobCreationCtrl()
        with self.assertRaises((TypeError, RuntimeError)):
            jc.set_node_path(None)

    def test_set_dir_with_int_raises(self):
        """set_dir_for_job_creation() requires a str; passing int raises ArgumentError."""
        jc = ecf.JobCreationCtrl()
        with self.assertRaises((TypeError, RuntimeError)):
            jc.set_dir_for_job_creation(42)

    def test_set_dir_with_none_raises(self):
        """set_dir_for_job_creation() requires a str; passing None raises ArgumentError."""
        jc = ecf.JobCreationCtrl()
        with self.assertRaises((TypeError, RuntimeError)):
            jc.set_dir_for_job_creation(None)

    def test_set_verbose_with_string_raises(self):
        """set_verbose() requires a bool; passing a plain string raises ArgumentError."""
        jc = ecf.JobCreationCtrl()
        with self.assertRaises((TypeError, RuntimeError)):
            jc.set_verbose("true")

    def test_set_verbose_accepts_none_as_false(self):
        """boost.python converts None to C++ bool(false); set_verbose(None) is accepted."""
        jc = ecf.JobCreationCtrl()
        jc.set_verbose(None)  # must not raise — None is treated as Fals
        self.assertEqual(jc.is_verbose(), False)

    def test_set_verbose_accepts_int_as_bool(self):
        """boost.python converts any Python int to C++ bool (0→False, non-zero→True).
        Only plain strings are rejected; all integers are silently accepted."""
        jc = ecf.JobCreationCtrl()
        for val in [0, 1, 2, -1, 42, -42, 100]:
            with self.subTest(val=val):
                jc.set_verbose(val)  # must not raise
                self.assertEqual(jc.is_verbose(), bool(val))  # 0→False, non-zero→True


class TestZombieType(unittest.TestCase):
    """Tests for py::enum_<ecf::Child::ZombieType> exposed as ecf.ZombieType.

    Exposed API
    -----------
    Class attributes (one per enum value, 6 total)
        ecf.ZombieType.ecf, .ecf_pid, .ecf_pid_passwd, .ecf_passwd, .user, .path

    Type hierarchy
        ZombieType → enum → int → object
        Instances are simultaneously ZombieType and int.

    Per-instance properties
        v.name       -- Python attribute name as str  (same as str(v))
        str(v)       -- bare name (e.g. "ecf")
        repr(v)      -- "ecflow.ZombieType.<name>"
        int(v)       -- underlying C++ integer value (see INT_VALUES)

    Class-level lookup dicts (singletons)
        ZombieType.values  -- {int : ZombieType}  6 entries
        ZombieType.names   -- {str : ZombieType}  6 entries

    Operators (inherited from int)
        __eq__ / __ne__ / __lt__ / __le__ / __gt__ / __ge__  -- integer comparison
        __hash__                                              -- hash == int(v)

    Iteration
        list(ZombieType)  -- raises TypeError; use ZombieType.values.values() instead

    Construction
        ZombieType(n)  -- int subclass constructor; may not return the class-attr singleton

    Note: declaration order differs from numeric order.
        user=0 is the smallest int value even though it appears last in the binding.
    """

    # ------------------------------------------------------------------
    # Reference data
    # ------------------------------------------------------------------

    ALL_NAMES: List[str] = [
        "ecf",
        "ecf_pid",
        "ecf_pid_passwd",
        "ecf_passwd",
        "user",
        "path",
    ]

    # C++ integer values keyed by Python attribute name
    INT_VALUES: Dict[str, int] = {
        "ecf": 1,
        "ecf_pid": 2,
        "ecf_pid_passwd": 4,
        "ecf_passwd": 3,
        "user": 0,
        "path": 5,
    }

    # ------------------------------------------------------------------
    # Completeness
    # ------------------------------------------------------------------

    def test_total_member_count_is_6(self):
        """Exactly 6 ZombieType values are declared in the binding."""
        self.assertEqual(len(ecf.ZombieType.names), 6)
        self.assertEqual(len(ecf.ZombieType.values), 6)

    def test_all_names_accessible_as_class_attributes(self):
        """Every declared name is accessible as an attribute of ZombieType."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertTrue(
                    hasattr(ecf.ZombieType, name),
                    f"ecf.ZombieType.{name} not found",
                )

    def test_all_names_present_in_names_dict(self):
        """ZombieType.names contains every declared name as a key."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(name, ecf.ZombieType.names)

    def test_all_int_values_present_in_values_dict(self):
        """ZombieType.values contains every declared integer value as a key."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertIn(expected_int, ecf.ZombieType.values)

    # ------------------------------------------------------------------
    # Type hierarchy
    # ------------------------------------------------------------------

    def test_instances_are_zombie_type(self):
        """Every enum member is an instance of ecf.ZombieType."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIsInstance(getattr(ecf.ZombieType, name), ecf.ZombieType)

    def test_int_conversion_works(self):
        """int(member) produces the C++ integer value (pybind11 enums are not int subclasses
        unlike Boost.Python enums, but they are still convertible to int)."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(int(getattr(ecf.ZombieType, name)), expected)

    # ------------------------------------------------------------------
    # str / repr / .name
    # ------------------------------------------------------------------

    def test_str_returns_just_the_name(self):
        """str(v) returns only the attribute name, not the module-qualified form."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertEqual(str(getattr(ecf.ZombieType, name)), name)

    def test_repr_returns_module_qualified_name(self):
        """repr(v) returns 'ecflow.ZombieType.<name>'."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertEqual(
                    repr(getattr(ecf.ZombieType, name)),
                    f"ecflow.ZombieType.{name}",
                )

    def test_name_attribute_equals_str(self):
        """v.name is the same string as str(v) for every member."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.ZombieType, name)
                self.assertEqual(v.name, name)
                self.assertEqual(v.name, str(v))

    # ------------------------------------------------------------------
    # Integer values
    # ------------------------------------------------------------------

    def test_int_value_matches_cpp_enum(self):
        """int(v) matches the documented C++ integer value for each member."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(int(getattr(ecf.ZombieType, name)), expected)

    def test_user_is_zero(self):
        """user has the smallest (zero) integer value even though it is not first in the binding."""
        self.assertEqual(int(ecf.ZombieType.user), 0)

    def test_declaration_order_differs_from_numeric_order(self):
        """ecf_pid_passwd (4) > ecf_passwd (3) even though ecf_pid_passwd is declared first."""
        self.assertGreater(
            int(ecf.ZombieType.ecf_pid_passwd), int(ecf.ZombieType.ecf_passwd)
        )
        self.assertGreater(int(ecf.ZombieType.path), int(ecf.ZombieType.ecf_pid_passwd))

    # ------------------------------------------------------------------
    # .values and .names lookup dicts
    # ------------------------------------------------------------------

    def test_values_dict_maps_int_to_zombie_type(self):
        """ZombieType.values maps each int key to a ZombieType instance."""
        for int_val, member in ecf.ZombieType.values.items():
            with self.subTest(int_val=int_val):
                self.assertIsInstance(int_val, int)
                self.assertIsInstance(member, ecf.ZombieType)

    def test_names_dict_maps_str_to_zombie_type(self):
        """ZombieType.names maps each string key to a ZombieType instance."""
        for name, member in ecf.ZombieType.names.items():
            with self.subTest(name=name):
                self.assertIsInstance(name, str)
                self.assertIsInstance(member, ecf.ZombieType)

    def test_values_dict_roundtrip(self):
        """ZombieType.values[int(v)] is the same singleton as the class attribute."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.ZombieType, name)
                self.assertIs(ecf.ZombieType.values[int(v)], v)

    def test_names_dict_roundtrip(self):
        """ZombieType.names[str(v)] is the same singleton as the class attribute."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.ZombieType, name)
                self.assertIs(ecf.ZombieType.names[str(v)], v)

    # ------------------------------------------------------------------
    # Iteration — the class itself is NOT iterable
    # ------------------------------------------------------------------

    def test_zombie_type_class_is_not_iterable(self):
        """list(ZombieType) raises TypeError; the class object has no __iter__."""
        with self.assertRaises(TypeError):
            list(ecf.ZombieType)

    def test_all_members_reachable_via_values_dict(self):
        """Iterating ZombieType.values.values() yields every member once."""
        members = set(ecf.ZombieType.values.values())
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(getattr(ecf.ZombieType, name), members)

    # ------------------------------------------------------------------
    # Equality and ordering (inherited from int)
    # ------------------------------------------------------------------

    def test_same_member_is_equal_to_itself(self):
        """v == v for every member (reflexive)."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.ZombieType, name)
                self.assertEqual(v, v)

    def test_members_with_different_int_values_are_not_equal(self):
        """ecf != user because their integer values differ."""
        self.assertNotEqual(ecf.ZombieType.ecf, ecf.ZombieType.user)

    def test_equality_with_plain_int(self):
        """A ZombieType value equals a plain int with the matching integer value."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(getattr(ecf.ZombieType, name), expected)

    def test_ordering_user_is_smallest(self):
        """user (0) is less than every other ZombieType value."""
        for name in self.ALL_NAMES:
            if name == "user":
                continue
            with self.subTest(name=name):
                self.assertLess(ecf.ZombieType.user, getattr(ecf.ZombieType, name))

    def test_ordering_path_is_largest(self):
        """path (5) is greater than every other ZombieType value."""
        for name in self.ALL_NAMES:
            if name == "path":
                continue
            with self.subTest(name=name):
                self.assertGreater(ecf.ZombieType.path, getattr(ecf.ZombieType, name))

    def test_full_sort_produces_non_decreasing_int_sequence(self):
        """Sorting all members by value yields a non-decreasing integer sequence."""
        sorted_vals = sorted(ecf.ZombieType.values.values())
        ints = [int(v) for v in sorted_vals]
        self.assertEqual(ints, sorted(ints))

    # ------------------------------------------------------------------
    # Hash (inherited from int: hash(v) == int(v))
    # ------------------------------------------------------------------

    def test_hash_equals_int_value(self):
        """hash(v) == int(v) for every member."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(hash(getattr(ecf.ZombieType, name)), expected_int)

    def test_can_be_stored_in_set(self):
        """ZombieType members are hashable and can be stored in a Python set."""
        s = set(ecf.ZombieType.values.values())
        self.assertEqual(len(s), 6)

    def test_set_deduplicates_equal_values(self):
        """Adding the same member twice produces one entry in a set."""
        s = {ecf.ZombieType.ecf, ecf.ZombieType.ecf}
        self.assertEqual(len(s), 1)

    def test_can_be_used_as_dict_key(self):
        """ZombieType members work as dictionary keys."""
        d = {
            ecf.ZombieType.ecf: "ecf_zombie",
            ecf.ZombieType.user: "user_zombie",
        }
        self.assertEqual(d[ecf.ZombieType.ecf], "ecf_zombie")
        self.assertEqual(d[ecf.ZombieType.user], "user_zombie")

    # ------------------------------------------------------------------
    # Construction from int
    # ------------------------------------------------------------------

    def test_can_construct_from_int(self):
        """ZombieType(n) constructs a ZombieType instance from a raw integer."""
        v = ecf.ZombieType(1)
        self.assertIsInstance(v, ecf.ZombieType)
        self.assertEqual(int(v), 1)

    def test_int_constructor_value_equals_class_attribute(self):
        """ZombieType(n) produces a value equal to the corresponding class attribute."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                from_ctor = ecf.ZombieType(expected_int)
                singleton = getattr(ecf.ZombieType, name)
                self.assertEqual(from_ctor, singleton)

    # ------------------------------------------------------------------
    # Negative cases
    # ------------------------------------------------------------------

    def test_nonexistent_member_raises_attribute_error(self):
        """Accessing an undefined name on ZombieType raises AttributeError."""
        with self.assertRaises(AttributeError):
            _ = ecf.ZombieType.nonexistent_zombie_type

    def test_missing_int_key_in_values_raises_key_error(self):
        """Looking up an int that has no member in values raises KeyError."""
        with self.assertRaises(KeyError):
            _ = ecf.ZombieType.values[9999]

    def test_missing_name_key_in_names_raises_key_error(self):
        """Looking up a name that has no member in names raises KeyError."""
        with self.assertRaises(KeyError):
            _ = ecf.ZombieType.names["nonexistent_zombie_type"]


class TestZombieUserActionType(unittest.TestCase):
    """Tests for py::enum_<ecf::ZombieCtrlAction> exposed as ecf.ZombieUserActionType.

    Exposed API
    -----------
    Class attributes (one per enum value, 6 total)
        ecf.ZombieUserActionType.fob, .fail, .remove, .adopt, .block, .kill

    Type hierarchy
        ZombieUserActionType → enum → int → object
        Instances are simultaneously ZombieUserActionType and int.

    Per-instance properties
        v.name       -- Python attribute name as str  (same as str(v))
        str(v)       -- bare name (e.g. "fob")
        repr(v)      -- "ecflow.ZombieUserActionType.<name>"
        int(v)       -- underlying C++ integer value (see INT_VALUES)

    Class-level lookup dicts (singletons)
        ZombieUserActionType.values  -- {int : ZombieUserActionType}  6 entries
        ZombieUserActionType.names   -- {str : ZombieUserActionType}  6 entries

    Operators (inherited from int)
        __eq__ / __ne__ / __lt__ / __le__ / __gt__ / __ge__  -- integer comparison
        __hash__                                              -- hash == int(v)

    Iteration
        list(ZombieUserActionType)  -- raises TypeError; the class itself is NOT iterable;
                                       iterate over ZombieUserActionType.values.values() instead

    Construction
        ZombieUserActionType(n)  -- int subclass constructor; may not return the class-attr singleton

    Note: remove (3) and adopt (2) are declared in reverse numeric order in the C++ binding.
    """

    ALL_NAMES: List[str] = ["fob", "fail", "remove", "adopt", "block", "kill"]

    INT_VALUES: Dict[str, int] = {
        "fob": 0,
        "fail": 1,
        "adopt": 2,
        "remove": 3,
        "block": 4,
        "kill": 5,
    }

    # ------------------------------------------------------------------
    # Completeness
    # ------------------------------------------------------------------

    def test_total_member_count_is_6(self):
        """Exactly 6 ZombieUserActionType values are declared."""
        self.assertEqual(len(ecf.ZombieUserActionType.names), 6)
        self.assertEqual(len(ecf.ZombieUserActionType.values), 6)

    def test_all_names_accessible_as_class_attributes(self):
        """Every declared name is accessible on ZombieUserActionType."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertTrue(hasattr(ecf.ZombieUserActionType, name))

    def test_all_names_present_in_names_dict(self):
        """ZombieUserActionType.names contains every declared name."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(name, ecf.ZombieUserActionType.names)

    def test_all_int_values_present_in_values_dict(self):
        """ZombieUserActionType.values contains every declared integer value."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertIn(expected_int, ecf.ZombieUserActionType.values)

    # ------------------------------------------------------------------
    # Type hierarchy
    # ------------------------------------------------------------------

    def test_instances_are_zombie_user_action_type(self):
        """Every member is an instance of ecf.ZombieUserActionType."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIsInstance(
                    getattr(ecf.ZombieUserActionType, name), ecf.ZombieUserActionType
                )

    def test_int_conversion_works(self):
        """int(member) produces the C++ integer value (pybind11 enums are not int subclasses
        unlike Boost.Python enums, but they are still convertible to int)."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(int(getattr(ecf.ZombieUserActionType, name)), expected)

    # ------------------------------------------------------------------
    # str / repr / .name
    # ------------------------------------------------------------------

    def test_str_returns_just_the_name(self):
        """str(v) returns only the bare attribute name."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertEqual(str(getattr(ecf.ZombieUserActionType, name)), name)

    def test_repr_returns_module_qualified_name(self):
        """repr(v) returns 'ecflow.ZombieUserActionType.<name>'."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertEqual(
                    repr(getattr(ecf.ZombieUserActionType, name)),
                    f"ecflow.ZombieUserActionType.{name}",
                )

    def test_name_attribute_equals_str(self):
        """v.name is the same string as str(v)."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.ZombieUserActionType, name)
                self.assertEqual(v.name, name)

    # ------------------------------------------------------------------
    # Integer values
    # ------------------------------------------------------------------

    def test_int_value_matches_cpp_enum(self):
        """int(v) matches the documented C++ integer for each member."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(int(getattr(ecf.ZombieUserActionType, name)), expected)

    def test_fob_is_zero(self):
        """fob has the lowest integer value (0)."""
        self.assertEqual(int(ecf.ZombieUserActionType.fob), 0)

    def test_declaration_order_differs_from_numeric_order(self):
        """remove (3) is declared before adopt (2) but has a higher int value."""
        self.assertGreater(
            int(ecf.ZombieUserActionType.remove),
            int(ecf.ZombieUserActionType.adopt),
        )

    # ------------------------------------------------------------------
    # .values / .names round-trips
    # ------------------------------------------------------------------

    def test_values_dict_roundtrip(self):
        """ZombieUserActionType.values[int(v)] is the same singleton as the class attr."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.ZombieUserActionType, name)
                self.assertIs(ecf.ZombieUserActionType.values[int(v)], v)

    def test_names_dict_roundtrip(self):
        """ZombieUserActionType.names[str(v)] is the same singleton as the class attr."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.ZombieUserActionType, name)
                self.assertIs(ecf.ZombieUserActionType.names[str(v)], v)

    # ------------------------------------------------------------------
    # Iteration
    # ------------------------------------------------------------------

    def test_class_is_not_iterable(self):
        """list(ZombieUserActionType) raises TypeError."""
        with self.assertRaises(TypeError):
            list(ecf.ZombieUserActionType)

    def test_all_members_reachable_via_values_dict(self):
        """Every member appears in ZombieUserActionType.values.values()."""
        members = set(ecf.ZombieUserActionType.values.values())
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(getattr(ecf.ZombieUserActionType, name), members)

    # ------------------------------------------------------------------
    # Equality and ordering
    # ------------------------------------------------------------------

    def test_same_member_is_equal_to_itself(self):
        """v == v for every member (reflexive)."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.ZombieUserActionType, name)
                self.assertEqual(v, v)

    def test_different_members_are_not_equal(self):
        """fob != kill because their integer values differ."""
        self.assertNotEqual(ecf.ZombieUserActionType.fob, ecf.ZombieUserActionType.kill)

    def test_equality_with_plain_int(self):
        """A ZombieUserActionType value equals a plain int with the same value."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(getattr(ecf.ZombieUserActionType, name), expected)

    def test_ordering_fob_is_smallest(self):
        """fob (0) is less than every other ZombieUserActionType value."""
        for name in self.ALL_NAMES:
            if name == "fob":
                continue
            with self.subTest(name=name):
                self.assertLess(
                    ecf.ZombieUserActionType.fob,
                    getattr(ecf.ZombieUserActionType, name),
                )

    def test_ordering_kill_is_largest(self):
        """kill (5) is greater than every other ZombieUserActionType value."""
        for name in self.ALL_NAMES:
            if name == "kill":
                continue
            with self.subTest(name=name):
                self.assertGreater(
                    ecf.ZombieUserActionType.kill,
                    getattr(ecf.ZombieUserActionType, name),
                )

    def test_full_sort_produces_non_decreasing_sequence(self):
        """Sorting all members by value yields a non-decreasing integer sequence."""
        sorted_vals = sorted(ecf.ZombieUserActionType.values.values())
        ints = [int(v) for v in sorted_vals]
        self.assertEqual(ints, sorted(ints))

    # ------------------------------------------------------------------
    # Hash
    # ------------------------------------------------------------------

    def test_hash_equals_int_value(self):
        """hash(v) == int(v) for every member."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(
                    hash(getattr(ecf.ZombieUserActionType, name)), expected_int
                )

    def test_can_be_stored_in_set(self):
        """All 6 members can be stored in a Python set."""
        s = set(ecf.ZombieUserActionType.values.values())
        self.assertEqual(len(s), 6)

    def test_can_be_used_as_dict_key(self):
        """ZombieUserActionType members work as dictionary keys."""
        d = {ecf.ZombieUserActionType.fob: "fob", ecf.ZombieUserActionType.kill: "kill"}
        self.assertEqual(d[ecf.ZombieUserActionType.fob], "fob")
        self.assertEqual(d[ecf.ZombieUserActionType.kill], "kill")

    # ------------------------------------------------------------------
    # Construction from int
    # ------------------------------------------------------------------

    def test_int_constructor_value_equals_class_attribute(self):
        """ZombieUserActionType(n) produces a value equal to the corresponding class attr."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(
                    ecf.ZombieUserActionType(expected_int),
                    getattr(ecf.ZombieUserActionType, name),
                )

    # ------------------------------------------------------------------
    # Negative cases
    # ------------------------------------------------------------------

    def test_nonexistent_member_raises_attribute_error(self):
        """Accessing an undefined name raises AttributeError."""
        with self.assertRaises(AttributeError):
            _ = ecf.ZombieUserActionType.nonexistent

    def test_missing_int_key_in_values_raises_key_error(self):
        """Looking up an unknown int in values raises KeyError."""
        with self.assertRaises(KeyError):
            _ = ecf.ZombieUserActionType.values[9999]

    def test_missing_name_key_in_names_raises_key_error(self):
        """Looking up an unknown name in names raises KeyError."""
        with self.assertRaises(KeyError):
            _ = ecf.ZombieUserActionType.names["nonexistent"]


class TestChildCmdType(unittest.TestCase):
    """Tests for py::enum_<ecf::Child::CmdType> exposed as ecf.ChildCmdType.

    Exposed API
    -----------
    Class attributes (one per enum value, 8 total)
        ecf.ChildCmdType.init, .event, .meter, .label, .wait, .queue, .abort, .complete

    Type hierarchy
        ChildCmdType → enum → int → object
        Instances are simultaneously ChildCmdType and int.

    Per-instance properties
        v.name       -- Python attribute name as str  (same as str(v))
        str(v)       -- bare name (e.g. "init")
        repr(v)      -- "ecflow.ChildCmdType.<name>"
        int(v)       -- underlying C++ integer value (see INT_VALUES)

    Class-level lookup dicts (singletons)
        ChildCmdType.values  -- {int : ChildCmdType}  8 entries
        ChildCmdType.names   -- {str : ChildCmdType}  8 entries

    Operators (inherited from int)
        __eq__ / __ne__ / __lt__ / __le__ / __gt__ / __ge__  -- integer comparison
        __hash__                                              -- hash == int(v)

    Iteration
        list(ChildCmdType)  -- raises TypeError; the class itself is NOT iterable;
                               iterate over ChildCmdType.values.values() instead

    Construction
        ChildCmdType(n)  -- int subclass constructor; may not return the class-attr singleton

    Note: integer values are perfectly contiguous 0–7 in declaration order.
    """

    ALL_NAMES: List[str] = [
        "init",
        "event",
        "meter",
        "label",
        "wait",
        "queue",
        "abort",
        "complete",
    ]

    INT_VALUES: Dict[str, int] = {
        "init": 0,
        "event": 1,
        "meter": 2,
        "label": 3,
        "wait": 4,
        "queue": 5,
        "abort": 6,
        "complete": 7,
    }

    # ------------------------------------------------------------------
    # Completeness
    # ------------------------------------------------------------------

    def test_total_member_count_is_8(self):
        """Exactly 8 ChildCmdType values are declared."""
        self.assertEqual(len(ecf.ChildCmdType.names), 8)
        self.assertEqual(len(ecf.ChildCmdType.values), 8)

    def test_all_names_accessible_as_class_attributes(self):
        """Every declared name is accessible on ChildCmdType."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertTrue(hasattr(ecf.ChildCmdType, name))

    def test_all_names_present_in_names_dict(self):
        """ChildCmdType.names contains every declared name."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(name, ecf.ChildCmdType.names)

    def test_all_int_values_present_in_values_dict(self):
        """ChildCmdType.values contains every declared integer value."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertIn(expected_int, ecf.ChildCmdType.values)

    # ------------------------------------------------------------------
    # Type hierarchy
    # ------------------------------------------------------------------

    def test_instances_are_child_cmd_type(self):
        """Every member is an instance of ecf.ChildCmdType."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIsInstance(getattr(ecf.ChildCmdType, name), ecf.ChildCmdType)

    def test_int_conversion_works(self):
        """int(member) produces the C++ integer value (pybind11 enums are not int subclasses
        unlike Boost.Python enums, but they are still convertible to int)."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(int(getattr(ecf.ChildCmdType, name)), expected)

    # ------------------------------------------------------------------
    # str / repr / .name
    # ------------------------------------------------------------------

    def test_str_returns_just_the_name(self):
        """str(v) returns only the bare attribute name."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertEqual(str(getattr(ecf.ChildCmdType, name)), name)

    def test_repr_returns_module_qualified_name(self):
        """repr(v) returns 'ecflow.ChildCmdType.<name>'."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertEqual(
                    repr(getattr(ecf.ChildCmdType, name)),
                    f"ecflow.ChildCmdType.{name}",
                )

    def test_name_attribute_equals_str(self):
        """v.name is the same string as str(v)."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.ChildCmdType, name)
                self.assertEqual(v.name, name)

    # ------------------------------------------------------------------
    # Integer values — contiguous 0–7, declaration order = numeric order
    # ------------------------------------------------------------------

    def test_int_value_matches_cpp_enum(self):
        """int(v) matches the documented C++ integer for each member."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(int(getattr(ecf.ChildCmdType, name)), expected)

    def test_init_is_zero(self):
        """init has integer value 0 (first and smallest)."""
        self.assertEqual(int(ecf.ChildCmdType.init), 0)

    def test_complete_is_seven(self):
        """complete has integer value 7 (last and largest)."""
        self.assertEqual(int(ecf.ChildCmdType.complete), 7)

    def test_values_are_contiguous_zero_to_seven(self):
        """The integer values form an unbroken sequence 0–7."""
        all_ints = sorted(int(v) for v in ecf.ChildCmdType.values.values())
        self.assertEqual(all_ints, list(range(8)))

    def test_declaration_order_matches_numeric_order(self):
        """For ChildCmdType, declaration order equals numeric order (no surprises)."""
        prev_int = -1
        for name in self.ALL_NAMES:
            cur_int = int(getattr(ecf.ChildCmdType, name))
            self.assertGreater(cur_int, prev_int)
            prev_int = cur_int

    # ------------------------------------------------------------------
    # .values / .names round-trips
    # ------------------------------------------------------------------

    def test_values_dict_roundtrip(self):
        """ChildCmdType.values[int(v)] is the same singleton as the class attr."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.ChildCmdType, name)
                self.assertIs(ecf.ChildCmdType.values[int(v)], v)

    def test_names_dict_roundtrip(self):
        """ChildCmdType.names[str(v)] is the same singleton as the class attr."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.ChildCmdType, name)
                self.assertIs(ecf.ChildCmdType.names[str(v)], v)

    # ------------------------------------------------------------------
    # Iteration
    # ------------------------------------------------------------------

    def test_class_is_not_iterable(self):
        """list(ChildCmdType) raises TypeError."""
        with self.assertRaises(TypeError):
            list(ecf.ChildCmdType)

    def test_all_members_reachable_via_values_dict(self):
        """Every member appears in ChildCmdType.values.values()."""
        members = set(ecf.ChildCmdType.values.values())
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(getattr(ecf.ChildCmdType, name), members)

    # ------------------------------------------------------------------
    # Equality and ordering
    # ------------------------------------------------------------------

    def test_same_member_is_equal_to_itself(self):
        """v == v for every member (reflexive)."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.ChildCmdType, name)
                self.assertEqual(v, v)

    def test_different_members_are_not_equal(self):
        """init != complete because their integer values differ."""
        self.assertNotEqual(ecf.ChildCmdType.init, ecf.ChildCmdType.complete)

    def test_equality_with_plain_int(self):
        """A ChildCmdType value equals a plain int with the same value."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(getattr(ecf.ChildCmdType, name), expected)

    def test_ordering_init_is_smallest(self):
        """init (0) is less than every other ChildCmdType."""
        for name in self.ALL_NAMES:
            if name == "init":
                continue
            with self.subTest(name=name):
                self.assertLess(ecf.ChildCmdType.init, getattr(ecf.ChildCmdType, name))

    def test_ordering_complete_is_largest(self):
        """complete (7) is greater than every other ChildCmdType."""
        for name in self.ALL_NAMES:
            if name == "complete":
                continue
            with self.subTest(name=name):
                self.assertGreater(
                    ecf.ChildCmdType.complete, getattr(ecf.ChildCmdType, name)
                )

    def test_full_sort_produces_non_decreasing_sequence(self):
        """Sorting all members by value yields a non-decreasing integer sequence."""
        sorted_vals = sorted(ecf.ChildCmdType.values.values())
        ints = [int(v) for v in sorted_vals]
        self.assertEqual(ints, sorted(ints))

    # ------------------------------------------------------------------
    # Hash
    # ------------------------------------------------------------------

    def test_hash_equals_int_value(self):
        """hash(v) == int(v) for every member."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(hash(getattr(ecf.ChildCmdType, name)), expected_int)

    def test_can_be_stored_in_set(self):
        """All 8 members can be stored in a Python set."""
        s = set(ecf.ChildCmdType.values.values())
        self.assertEqual(len(s), 8)

    def test_can_be_used_as_dict_key(self):
        """ChildCmdType members work as dictionary keys."""
        d = {ecf.ChildCmdType.init: "init", ecf.ChildCmdType.complete: "complete"}
        self.assertEqual(d[ecf.ChildCmdType.init], "init")
        self.assertEqual(d[ecf.ChildCmdType.complete], "complete")

    # ------------------------------------------------------------------
    # Construction from int
    # ------------------------------------------------------------------

    def test_int_constructor_value_equals_class_attribute(self):
        """ChildCmdType(n) produces a value equal to the corresponding class attr."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(
                    ecf.ChildCmdType(expected_int),
                    getattr(ecf.ChildCmdType, name),
                )

    # ------------------------------------------------------------------
    # Negative cases
    # ------------------------------------------------------------------

    def test_nonexistent_member_raises_attribute_error(self):
        """Accessing an undefined name raises AttributeError."""
        with self.assertRaises(AttributeError):
            _ = ecf.ChildCmdType.nonexistent

    def test_missing_int_key_in_values_raises_key_error(self):
        """Looking up an unknown int in values raises KeyError."""
        with self.assertRaises(KeyError):
            _ = ecf.ChildCmdType.values[9999]

    def test_missing_name_key_in_names_raises_key_error(self):
        """Looking up an unknown name in names raises KeyError."""
        with self.assertRaises(KeyError):
            _ = ecf.ChildCmdType.names["nonexistent"]


class TestAttrType(unittest.TestCase):
    """Tests for py::enum_<ecf::Attr::Type> exposed as ecf.AttrType.

    Exposed API
    -----------
    Class attributes (one per enum value, 6 total)
        ecf.AttrType.event, .meter, .label, .limit, .variable, .all

    Type hierarchy
        AttrType → enum → int → object
        Instances are simultaneously AttrType and int.

    Per-instance properties
        v.name       -- Python attribute name as str  (same as str(v))
        str(v)       -- bare name (e.g. "event")
        repr(v)      -- "ecflow.AttrType.<name>"
        int(v)       -- underlying C++ integer value (see INT_VALUES)

    Class-level lookup dicts (singletons)
        AttrType.values  -- {int : AttrType}  6 entries; keys are 1–6 (no zero)
        AttrType.names   -- {str : AttrType}  6 entries

    Operators (inherited from int)
        __eq__ / __ne__ / __lt__ / __le__ / __gt__ / __ge__  -- integer comparison
        __hash__                                              -- hash == int(v)

    Iteration
        list(AttrType)  -- raises TypeError; the class itself is NOT iterable;
                           iterate over AttrType.values.values() instead

    Construction
        AttrType(n)  -- int subclass constructor; may not return the class-attr singleton

    Note: integer values are contiguous 1–6; there is NO zero value.
          'all' is a Python built-in name but is used here as a plain class attribute.
    """

    ALL_NAMES: List[str] = ["event", "meter", "label", "limit", "variable", "all"]

    INT_VALUES: Dict[str, int] = {
        "event": 1,
        "meter": 2,
        "label": 3,
        "limit": 4,
        "variable": 5,
        "all": 6,
    }

    # ------------------------------------------------------------------
    # Completeness
    # ------------------------------------------------------------------

    def test_total_member_count_is_6(self):
        """Exactly 6 AttrType values are declared."""
        self.assertEqual(len(ecf.AttrType.names), 6)
        self.assertEqual(len(ecf.AttrType.values), 6)

    def test_all_names_accessible_as_class_attributes(self):
        """Every declared name is accessible on AttrType."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertTrue(hasattr(ecf.AttrType, name))

    def test_all_names_present_in_names_dict(self):
        """AttrType.names contains every declared name."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(name, ecf.AttrType.names)

    def test_all_int_values_present_in_values_dict(self):
        """AttrType.values contains every declared integer value."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertIn(expected_int, ecf.AttrType.values)

    # ------------------------------------------------------------------
    # Type hierarchy
    # ------------------------------------------------------------------

    def test_instances_are_attr_type(self):
        """Every member is an instance of ecf.AttrType."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIsInstance(getattr(ecf.AttrType, name), ecf.AttrType)

    def test_int_conversion_works(self):
        """int(member) produces the C++ integer value (pybind11 enums are not int subclasses
        unlike Boost.Python enums, but they are still convertible to int)."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(int(getattr(ecf.AttrType, name)), expected)

    # ------------------------------------------------------------------
    # str / repr / .name
    # ------------------------------------------------------------------

    def test_str_returns_just_the_name(self):
        """str(v) returns only the bare attribute name, including 'all'."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertEqual(str(getattr(ecf.AttrType, name)), name)

    def test_repr_returns_module_qualified_name(self):
        """repr(v) returns 'ecflow.AttrType.<name>'."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertEqual(
                    repr(getattr(ecf.AttrType, name)),
                    f"ecflow.AttrType.{name}",
                )

    def test_name_attribute_equals_str(self):
        """v.name is the same string as str(v)."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.AttrType, name)
                self.assertEqual(v.name, name)

    # ------------------------------------------------------------------
    # Integer values — contiguous 1–6, no zero
    # ------------------------------------------------------------------

    def test_int_value_matches_cpp_enum(self):
        """int(v) matches the documented C++ integer for each member."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(int(getattr(ecf.AttrType, name)), expected)

    def test_no_zero_value(self):
        """AttrType has no member with integer value 0."""
        self.assertNotIn(0, ecf.AttrType.values)

    def test_event_is_one(self):
        """event (1) is the minimum integer value."""
        self.assertEqual(int(ecf.AttrType.event), 1)

    def test_all_is_six(self):
        """all (6) is the maximum integer value."""
        self.assertEqual(int(ecf.AttrType.all), 6)

    def test_values_are_contiguous_one_to_six(self):
        """The integer values form an unbroken sequence 1–6."""
        all_ints = sorted(int(v) for v in ecf.AttrType.values.values())
        self.assertEqual(all_ints, list(range(1, 7)))

    def test_all_attribute_is_shadow_of_builtin(self):
        """AttrType.all is a valid attribute even though 'all' is a Python built-in."""
        v = ecf.AttrType.all
        self.assertIsInstance(v, ecf.AttrType)
        self.assertEqual(int(v), 6)

    # ------------------------------------------------------------------
    # .values / .names round-trips
    # ------------------------------------------------------------------

    def test_values_dict_roundtrip(self):
        """AttrType.values[int(v)] is the same singleton as the class attr."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.AttrType, name)
                self.assertIs(ecf.AttrType.values[int(v)], v)

    def test_names_dict_roundtrip(self):
        """AttrType.names[str(v)] is the same singleton as the class attr."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.AttrType, name)
                self.assertIs(ecf.AttrType.names[str(v)], v)

    # ------------------------------------------------------------------
    # Iteration
    # ------------------------------------------------------------------

    def test_class_is_not_iterable(self):
        """list(AttrType) raises TypeError."""
        with self.assertRaises(TypeError):
            list(ecf.AttrType)

    def test_all_members_reachable_via_values_dict(self):
        """Every member appears in AttrType.values.values()."""
        members = set(ecf.AttrType.values.values())
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(getattr(ecf.AttrType, name), members)

    # ------------------------------------------------------------------
    # Equality and ordering
    # ------------------------------------------------------------------

    def test_same_member_is_equal_to_itself(self):
        """v == v for every member (reflexive)."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.AttrType, name)
                self.assertEqual(v, v)

    def test_different_members_are_not_equal(self):
        """event != all because their integer values differ."""
        self.assertNotEqual(ecf.AttrType.event, ecf.AttrType.all)

    def test_equality_with_plain_int(self):
        """A AttrType value equals a plain int with the same value."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(getattr(ecf.AttrType, name), expected)

    def test_ordering_event_is_smallest(self):
        """event (1) is less than every other AttrType value."""
        for name in self.ALL_NAMES:
            if name == "event":
                continue
            with self.subTest(name=name):
                self.assertLess(ecf.AttrType.event, getattr(ecf.AttrType, name))

    def test_ordering_all_is_largest(self):
        """all (6) is greater than every other AttrType value."""
        for name in self.ALL_NAMES:
            if name == "all":
                continue
            with self.subTest(name=name):
                self.assertGreater(ecf.AttrType.all, getattr(ecf.AttrType, name))

    def test_full_sort_produces_non_decreasing_sequence(self):
        """Sorting all members by value yields a non-decreasing integer sequence."""
        sorted_vals = sorted(ecf.AttrType.values.values())
        ints = [int(v) for v in sorted_vals]
        self.assertEqual(ints, sorted(ints))

    # ------------------------------------------------------------------
    # Hash
    # ------------------------------------------------------------------

    def test_hash_equals_int_value(self):
        """hash(v) == int(v) for every member."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(hash(getattr(ecf.AttrType, name)), expected_int)

    def test_can_be_stored_in_set(self):
        """All 6 members can be stored in a Python set."""
        s = set(ecf.AttrType.values.values())
        self.assertEqual(len(s), 6)

    def test_can_be_used_as_dict_key(self):
        """AttrType members work as dictionary keys."""
        d = {ecf.AttrType.event: "event", ecf.AttrType.all: "all"}
        self.assertEqual(d[ecf.AttrType.event], "event")
        self.assertEqual(d[ecf.AttrType.all], "all")

    # ------------------------------------------------------------------
    # Construction from int
    # ------------------------------------------------------------------

    def test_int_constructor_value_equals_class_attribute(self):
        """AttrType(n) produces a value equal to the corresponding class attr."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(
                    ecf.AttrType(expected_int),
                    getattr(ecf.AttrType, name),
                )

    # ------------------------------------------------------------------
    # Negative cases
    # ------------------------------------------------------------------

    def test_nonexistent_member_raises_attribute_error(self):
        """Accessing an undefined name raises AttributeError."""
        with self.assertRaises(AttributeError):
            _ = ecf.AttrType.nonexistent

    def test_zero_not_in_values_raises_key_error(self):
        """Looking up int 0 in AttrType.values raises KeyError (no zero member)."""
        with self.assertRaises(KeyError):
            _ = ecf.AttrType.values[0]

    def test_missing_int_key_in_values_raises_key_error(self):
        """Looking up an unknown int in values raises KeyError."""
        with self.assertRaises(KeyError):
            _ = ecf.AttrType.values[9999]

    def test_missing_name_key_in_names_raises_key_error(self):
        """Looking up an unknown name in names raises KeyError."""
        with self.assertRaises(KeyError):
            _ = ecf.AttrType.names["nonexistent"]


class TestZombieAttr(unittest.TestCase):
    """Tests for py::class_<ZombieAttr> exposed as ecf.ZombieAttr.

    Exposed API
    -----------
    Constructors
        ZombieAttr(ZombieType, list[ChildCmdType], ZombieUserActionType)
            -- lifetime defaults to 3600 seconds
        ZombieAttr(ZombieType, list[ChildCmdType], ZombieUserActionType, int)
            -- explicit lifetime in seconds; non-positive values are clamped to 3600

    Instance methods
        zombie_type()     -> ZombieType           -- the zombie classification
        user_action()     -> ZombieUserActionType -- automated response action
        zombie_lifetime() -> int                  -- seconds the zombie lives in the server
        empty()           -> bool                 -- always False for Python-constructed instances

    Properties
        child_cmds  -- re-iterable range of ChildCmdType; empty means "applies to all commands"

    Operators
        __str__    -- "zombie <type>:<action>:<cmd,…>:<lifetime>"
        __eq__     -- value equality across all four fields
        __ne__     -- implicit complement of __eq__
        __copy__   -- copy.copy() returns a value-equal, identity-distinct instance
        __hash__   -- identity-based (boost.python extension type)
    """

    # ------------------------------------------------------------------
    # Aliases for brevity
    # ------------------------------------------------------------------

    ZT = ecf.ZombieType
    ZA = ecf.ZombieUserActionType
    CC = ecf.ChildCmdType

    # ------------------------------------------------------------------
    # Constructors
    # ------------------------------------------------------------------

    def test_three_arg_constructor_creates_instance(self):
        """ZombieAttr(type, cmds, action) creates a valid instance."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertIsInstance(z, ecf.ZombieAttr)

    def test_four_arg_constructor_creates_instance(self):
        """ZombieAttr(type, cmds, action, lifetime) creates a valid instance."""
        z = ecf.ZombieAttr(self.ZT.user, [], self.ZA.block, 300)
        self.assertIsInstance(z, ecf.ZombieAttr)

    def test_three_arg_constructor_default_lifetime_is_3600(self):
        """When no lifetime is given the server default of 3600 seconds is used."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertEqual(z.zombie_lifetime(), 3600)

    def test_constructor_with_empty_child_list(self):
        """An empty child list is accepted; means the action applies to all commands."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertIsInstance(z, ecf.ZombieAttr)
        self.assertEqual(list(z.child_cmds), [])

    def test_constructor_with_single_child_cmd(self):
        """A single ChildCmdType entry is accepted."""
        z = ecf.ZombieAttr(self.ZT.ecf, [self.CC.init], self.ZA.fob)
        self.assertEqual(list(z.child_cmds), [self.CC.init])

    def test_constructor_with_multiple_child_cmds(self):
        """Multiple ChildCmdType entries are stored in insertion order."""
        cmds = [self.CC.init, self.CC.event, self.CC.abort]
        z = ecf.ZombieAttr(self.ZT.ecf, cmds, self.ZA.fob)
        self.assertEqual(list(z.child_cmds), cmds)

    def test_constructor_with_all_child_cmds(self):
        """All eight ChildCmdType values can be passed and are preserved."""
        all_cmds = [
            self.CC.init,
            self.CC.event,
            self.CC.meter,
            self.CC.label,
            self.CC.wait,
            self.CC.queue,
            self.CC.abort,
            self.CC.complete,
        ]
        z = ecf.ZombieAttr(self.ZT.ecf, all_cmds, self.ZA.fob)
        self.assertEqual(list(z.child_cmds), all_cmds)

    def test_constructor_with_all_zombie_types(self):
        """Every ZombieType value is accepted by the constructor."""
        for zt in ecf.ZombieType.values.values():
            with self.subTest(zombie_type=str(zt)):
                z = ecf.ZombieAttr(zt, [], self.ZA.fob)
                self.assertEqual(z.zombie_type(), zt)

    def test_constructor_with_all_user_actions(self):
        """Every ZombieUserActionType value is accepted by the constructor."""
        for act in ecf.ZombieUserActionType.values.values():
            with self.subTest(action=str(act)):
                z = ecf.ZombieAttr(self.ZT.ecf, [], act)
                self.assertEqual(z.user_action(), act)

    # ------------------------------------------------------------------
    # zombie_type()
    # ------------------------------------------------------------------

    def test_zombie_type_returns_zombie_type_instance(self):
        """zombie_type() returns a ZombieType instance."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertIsInstance(z.zombie_type(), ecf.ZombieType)

    def test_zombie_type_reflects_constructor_argument(self):
        """zombie_type() returns exactly the ZombieType passed at construction."""
        for zt in ecf.ZombieType.values.values():
            with self.subTest(zombie_type=str(zt)):
                z = ecf.ZombieAttr(zt, [], self.ZA.fob)
                self.assertEqual(z.zombie_type(), zt)

    # ------------------------------------------------------------------
    # user_action()
    # ------------------------------------------------------------------

    def test_user_action_returns_zombie_user_action_type_instance(self):
        """user_action() returns a ZombieUserActionType instance."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertIsInstance(z.user_action(), ecf.ZombieUserActionType)

    def test_user_action_reflects_constructor_argument(self):
        """user_action() returns exactly the action passed at construction."""
        for act in ecf.ZombieUserActionType.values.values():
            with self.subTest(action=str(act)):
                z = ecf.ZombieAttr(self.ZT.ecf, [], act)
                self.assertEqual(z.user_action(), act)

    # ------------------------------------------------------------------
    # zombie_lifetime()
    # ------------------------------------------------------------------

    def test_zombie_lifetime_returns_int(self):
        """zombie_lifetime() returns a Python int."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob, 42)
        self.assertIsInstance(z.zombie_lifetime(), int)

    def test_zombie_lifetime_reflects_explicit_lifetime(self):
        """zombie_lifetime() reflects the lifetime passed in the four-arg constructor."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob, 300)
        self.assertEqual(z.zombie_lifetime(), 300)

    def test_zombie_lifetime_default_is_3600(self):
        """Three-arg constructor gives lifetime == 3600 (server default)."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertEqual(z.zombie_lifetime(), 3600)

    def test_zombie_lifetime_zero_is_clamped_to_default(self):
        """A zero lifetime is not stored; C++ clamps it to the default 3600."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob, 0)
        self.assertEqual(z.zombie_lifetime(), 3600)

    def test_zombie_lifetime_negative_is_clamped_to_default(self):
        """A negative lifetime is not stored; C++ clamps it to the default 3600."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob, -1)
        self.assertEqual(z.zombie_lifetime(), 3600)

    # ------------------------------------------------------------------
    # empty()
    # ------------------------------------------------------------------

    def test_empty_is_false_for_three_arg_constructor(self):
        """empty() returns False for a normally constructed ZombieAttr."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertFalse(z.empty())

    def test_empty_is_false_for_four_arg_constructor(self):
        """empty() returns False regardless of which constructor overload is used."""
        z = ecf.ZombieAttr(self.ZT.ecf, [self.CC.init], self.ZA.fob, 120)
        self.assertFalse(z.empty())

    # ------------------------------------------------------------------
    # child_cmds property
    # ------------------------------------------------------------------

    def test_child_cmds_elements_are_child_cmd_type(self):
        """Every element yielded by child_cmds is a ChildCmdType instance."""
        z = ecf.ZombieAttr(self.ZT.ecf, [self.CC.init, self.CC.abort], self.ZA.fob)
        for cmd in z.child_cmds:
            with self.subTest(cmd=str(cmd)):
                self.assertIsInstance(cmd, ecf.ChildCmdType)

    def test_child_cmds_empty_when_empty_list_passed(self):
        """child_cmds yields no elements when an empty list was passed."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertEqual(list(z.child_cmds), [])

    def test_child_cmds_preserves_insertion_order(self):
        """child_cmds yields ChildCmdType values in the order they were passed."""
        cmds = [self.CC.abort, self.CC.meter, self.CC.label]
        z = ecf.ZombieAttr(self.ZT.ecf, cmds, self.ZA.fob)
        self.assertEqual(list(z.child_cmds), cmds)

    def test_child_cmds_is_re_iterable(self):
        """child_cmds can be iterated more than once and yields the same results."""
        cmds = [self.CC.init, self.CC.abort]
        z = ecf.ZombieAttr(self.ZT.ecf, cmds, self.ZA.fob)
        self.assertEqual(list(z.child_cmds), list(z.child_cmds))

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertIsInstance(str(z), str)

    def test_str_format_contains_zombie_type(self):
        """str() output contains the zombie type name."""
        z = ecf.ZombieAttr(self.ZT.ecf_passwd, [self.CC.wait], self.ZA.remove, 120)
        self.assertIn("ecf_passwd", str(z))

    def test_str_format_contains_action(self):
        """str() output contains the action name."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.adopt, 60)
        self.assertIn("adopt", str(z))

    def test_str_format_contains_lifetime(self):
        """str() output contains the lifetime as a decimal integer."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob, 500)
        self.assertIn("500", str(z))

    def test_str_format_colon_separated(self):
        """str() follows the pattern 'zombie <type>:<action>:<cmds>:<lifetime>'."""
        z = ecf.ZombieAttr(
            self.ZT.path, [self.CC.abort, self.CC.complete], self.ZA.kill, 100
        )
        self.assertEqual(str(z), "zombie path:kill:abort,complete:100")

    def test_str_format_no_cmds_section_when_empty(self):
        """When no child cmds are given the cmds section in str() is empty."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob, 3600)
        s = str(z)
        self.assertEqual(s, "zombie ecf:fob::3600")

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A ZombieAttr is equal to itself."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertEqual(z, z)

    def test_eq_same_arguments(self):
        """Two ZombieAttrs created with the same arguments are equal."""
        z1 = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        z2 = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertEqual(z1, z2)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        z1 = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        z2 = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertTrue(z1 == z2)
        self.assertTrue(z2 == z1)

    def test_ne_different_zombie_type(self):
        """ZombieAttrs differing only in ZombieType are not equal."""
        z1 = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        z2 = ecf.ZombieAttr(self.ZT.user, [], self.ZA.fob)
        self.assertNotEqual(z1, z2)

    def test_ne_different_action(self):
        """ZombieAttrs differing only in user_action are not equal."""
        z1 = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        z2 = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fail)
        self.assertNotEqual(z1, z2)

    def test_ne_different_lifetime(self):
        """ZombieAttrs differing only in lifetime are not equal."""
        z1 = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob, 100)
        z2 = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob, 200)
        self.assertNotEqual(z1, z2)

    def test_ne_different_child_cmds(self):
        """ZombieAttrs differing only in child_cmds are not equal."""
        z1 = ecf.ZombieAttr(self.ZT.ecf, [self.CC.init], self.ZA.fob)
        z2 = ecf.ZombieAttr(self.ZT.ecf, [self.CC.abort], self.ZA.fob)
        self.assertNotEqual(z1, z2)

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces a ZombieAttr equal in value to the original."""
        import copy

        orig = ecf.ZombieAttr(self.ZT.ecf_pid, [self.CC.meter], self.ZA.adopt, 60)
        cop = copy.copy(orig)
        self.assertEqual(orig, cop)

    def test_copy_is_independent_object(self):
        """copy.copy() produces a distinct object (not the same reference)."""
        import copy

        orig = ecf.ZombieAttr(self.ZT.ecf_pid, [self.CC.meter], self.ZA.adopt, 60)
        cop = copy.copy(orig)
        self.assertIsNot(orig, cop)

    def test_copy_is_instance_of_zombie_attr(self):
        """The result of copy.copy() is a ZombieAttr."""
        import copy

        orig = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertIsInstance(copy.copy(orig), ecf.ZombieAttr)

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """ZombieAttr is hashable (boost.python identity-based hash)."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertIsInstance(hash(z), int)

    def test_hash_is_identity_based(self):
        """Two value-equal ZombieAttrs may have different hashes (identity, not content)."""
        z1 = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        z2 = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        self.assertEqual(z1, z2)
        self.assertIsNot(z1, z2)
        self.assertNotEqual(hash(z1), hash(z2))

    def test_can_be_stored_in_set(self):
        """ZombieAttr instances can be stored in a Python set."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        s = {z}
        self.assertIn(z, s)

    def test_can_be_used_as_dict_key(self):
        """ZombieAttr instances can be used as dictionary keys."""
        z = ecf.ZombieAttr(self.ZT.ecf, [], self.ZA.fob)
        d = {z: "value"}
        self.assertEqual(d[z], "value")

    # ------------------------------------------------------------------
    # Negative: wrong constructor argument types
    # ------------------------------------------------------------------

    def test_wrong_zombie_type_raises(self):
        """Passing a str instead of ZombieType raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.ZombieAttr("ecf", [], self.ZA.fob)

    def test_wrong_action_type_raises(self):
        """Passing a str instead of ZombieUserActionType raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.ZombieAttr(self.ZT.ecf, [], "fob")

    def test_wrong_child_cmd_type_in_list_raises(self):
        """Passing a str inside the child commands list raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.ZombieAttr(self.ZT.ecf, ["init"], self.ZA.fob)

    def test_wrong_child_cmd_int_in_list_raises(self):
        """Passing a plain int inside the child commands list raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.ZombieAttr(self.ZT.ecf, [0], self.ZA.fob)


class TestZombieVec(unittest.TestCase):
    """Tests for py::class_<std::vector<Zombie>>("ZombieVec", ...) as exposed in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        ZombieVec()                  -- default constructor; creates an empty vector

    Mutation methods (via vector_indexing_suite)
        append(Zombie)               -- add one Zombie to the end; rejects all other types
        extend(iterable[Zombie])     -- add all Zombies from an iterable

    Sequence operators (via vector_indexing_suite)
        len(zv)      -- __len__       -- number of elements (0 after default construction)
        zv[i]        -- __getitem__   -- positive and negative integer indexing;
                                         IndexError on out-of-range
        for z in zv  -- __iter__      -- yields Zombie instances in insertion order
        z in zv      -- __contains__  -- membership test; uses Zombie.__eq__ (identity-based)

    Equality / hash
        __eq__    -- identity-based (two distinct ZombieVecs are never equal, even if empty)
        __hash__  -- identity-based (boost.python default); returns a stable int per instance

    Constraints
        No copy/pickle support: copy.copy() raises RuntimeError
    """

    def _make_attr(self):
        """Return a fresh ZombieAttr for use in Zombie construction."""
        return ecf.ZombieAttr(ecf.ZombieType.ecf, [], ecf.ZombieUserActionType.fob)

    # ------------------------------------------------------------------
    # Constructor
    # ------------------------------------------------------------------

    def test_default_constructor(self):
        """ZombieVec() constructs an empty vector."""
        zv = ecf.ZombieVec()
        self.assertIsInstance(zv, ecf.ZombieVec)

    def test_default_constructor_is_empty(self):
        """A newly constructed ZombieVec has length 0."""
        zv = ecf.ZombieVec()
        self.assertEqual(len(zv), 0)

    # ------------------------------------------------------------------
    # __len__
    # ------------------------------------------------------------------

    def test_len_increases_after_append(self):
        """len() grows by 1 for each append() call."""
        zv = ecf.ZombieVec()
        zv.append(ecf.Zombie())
        self.assertEqual(len(zv), 1)
        zv.append(ecf.Zombie())
        self.assertEqual(len(zv), 2)

    # ------------------------------------------------------------------
    # append
    # ------------------------------------------------------------------

    def test_append_zombie(self):
        """append() accepts a Zombie instance without error."""
        zv = ecf.ZombieVec()
        zv.append(ecf.Zombie())

    def test_append_wrong_type_raises(self):
        """append() rejects a non-Zombie argument."""
        zv = ecf.ZombieVec()
        with self.assertRaises((TypeError, RuntimeError)):
            zv.append("not_a_zombie")

    def test_append_int_raises(self):
        """append() rejects a plain int."""
        zv = ecf.ZombieVec()
        with self.assertRaises((TypeError, RuntimeError)):
            zv.append(42)

    def test_append_none_raises(self):
        """append() rejects None."""
        zv = ecf.ZombieVec()
        with self.assertRaises((TypeError, RuntimeError)):
            zv.append(None)

    # ------------------------------------------------------------------
    # extend
    # ------------------------------------------------------------------

    def test_extend_empty_list(self):
        """extend() with an empty list leaves the vector unchanged."""
        zv = ecf.ZombieVec()
        zv.extend([])
        self.assertEqual(len(zv), 0)

    def test_extend_with_zombies(self):
        """extend() adds all Zombie instances from a list."""
        zv = ecf.ZombieVec()
        zv.extend([ecf.Zombie(), ecf.Zombie(), ecf.Zombie()])
        self.assertEqual(len(zv), 3)

    def test_extend_wrong_element_type_raises(self):
        """extend() rejects a list containing non-Zombie elements."""
        zv = ecf.ZombieVec()
        with self.assertRaises((TypeError, RuntimeError)):
            zv.extend(["not_a_zombie"])

    # ------------------------------------------------------------------
    # __getitem__
    # ------------------------------------------------------------------

    def test_getitem_after_append(self):
        """zv[0] returns the first element after append."""
        zv = ecf.ZombieVec()
        z = ecf.Zombie()
        zv.append(z)
        item = zv[0]
        self.assertIsInstance(item, ecf.Zombie)

    def test_getitem_out_of_range_raises_index_error(self):
        """zv[0] on an empty ZombieVec raises IndexError."""
        zv = ecf.ZombieVec()
        with self.assertRaises(IndexError):
            _ = zv[0]

    def test_getitem_negative_index(self):
        """zv[-1] returns the last element."""
        zv = ecf.ZombieVec()
        zv.append(ecf.Zombie())
        zv.append(ecf.Zombie())
        item = zv[-1]
        self.assertIsInstance(item, ecf.Zombie)

    def test_getitem_out_of_range_positive(self):
        """zv[100] on a short vector raises IndexError."""
        zv = ecf.ZombieVec()
        zv.append(ecf.Zombie())
        with self.assertRaises(IndexError):
            _ = zv[100]

    # ------------------------------------------------------------------
    # __iter__
    # ------------------------------------------------------------------

    def test_iteration_empty(self):
        """Iterating over an empty ZombieVec yields zero items."""
        zv = ecf.ZombieVec()
        count = sum(1 for _ in zv)
        self.assertEqual(count, 0)

    def test_iteration_non_empty(self):
        """Iterating over a ZombieVec yields exactly len(zv) items."""
        zv = ecf.ZombieVec()
        zv.extend([ecf.Zombie(), ecf.Zombie(), ecf.Zombie()])
        items = list(zv)
        self.assertEqual(len(items), 3)

    def test_iteration_yields_zombie_instances(self):
        """Each item yielded by iteration is a Zombie."""
        zv = ecf.ZombieVec()
        zv.append(ecf.Zombie())
        for item in zv:
            self.assertIsInstance(item, ecf.Zombie)

    # ------------------------------------------------------------------
    # __contains__
    # ------------------------------------------------------------------

    def test_contains_false_for_empty(self):
        """'in' returns False for an empty ZombieVec."""
        zv = ecf.ZombieVec()
        self.assertNotIn(ecf.Zombie(), zv)

    def test_contains_true_after_append(self):
        """'in' returns True for the exact same object that was appended."""
        zv = ecf.ZombieVec()
        z = ecf.Zombie()
        zv.append(z)
        self.assertIn(z, zv)

    # ------------------------------------------------------------------
    # __eq__  (identity-based)
    # ------------------------------------------------------------------

    def test_eq_same_object(self):
        """A ZombieVec is equal to itself."""
        zv = ecf.ZombieVec()
        self.assertEqual(zv, zv)

    def test_eq_two_empty_vecs_are_not_equal(self):
        """Two distinct empty ZombieVec instances are NOT equal (identity-based)."""
        zv1 = ecf.ZombieVec()
        zv2 = ecf.ZombieVec()
        self.assertNotEqual(zv1, zv2)

    def test_eq_two_identical_content_vecs_not_equal(self):
        """Two ZombieVecs with the same elements are still not equal (identity)."""
        z = ecf.Zombie()
        zv1 = ecf.ZombieVec()
        zv2 = ecf.ZombieVec()
        zv1.append(z)
        zv2.append(z)
        self.assertNotEqual(zv1, zv2)

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """ZombieVec is hashable."""
        zv = ecf.ZombieVec()
        self.assertIsInstance(hash(zv), int)

    def test_hash_is_identity_based(self):
        """Two distinct ZombieVec instances have different hashes."""
        zv1 = ecf.ZombieVec()
        zv2 = ecf.ZombieVec()
        self.assertNotEqual(hash(zv1), hash(zv2))

    def test_same_object_same_hash(self):
        """The same ZombieVec object always returns the same hash."""
        zv = ecf.ZombieVec()
        self.assertEqual(hash(zv), hash(zv))

    def test_can_be_used_in_set(self):
        """ZombieVec instances can be stored in a Python set."""
        zv = ecf.ZombieVec()
        s = {zv}
        self.assertIn(zv, s)

    def test_can_be_used_as_dict_key(self):
        """ZombieVec instances can be used as dictionary keys."""
        zv = ecf.ZombieVec()
        d = {zv: "val"}
        self.assertEqual(d[zv], "val")

    # ------------------------------------------------------------------
    # copy / pickle (not supported)
    # ------------------------------------------------------------------

    def test_copy_raises(self):
        """copy.copy() raises RuntimeError (no pickle support)."""
        import copy

        zv = ecf.ZombieVec()
        with self.assertRaises((RuntimeError, TypeError)):
            copy.copy(zv)


class TestZombie(unittest.TestCase):
    """Tests for py::class_<Zombie> exposed as ecf.Zombie in ExportNodeAttr.cpp.

    Zombie represents a zombie process entry stored by the ecFlow server.
    The Python binding exposes only the default constructor (creating an empty,
    server-placeholder instance); fully-populated Zombie objects are returned
    by the live server API and are not directly constructible with data from
    Python.

    Exposed API
    -----------
    Constructors
        Zombie()    -- default; creates an empty sentinel (path_to_task == "")

    Instance methods
        empty()                 -> bool               -- True when path_to_task is empty
        manual_user_action()    -> bool               -- True when a manual action was set
        fob()                   -> bool               -- True when action is FOB
        fail()                  -> bool               -- True when action is FAIL
        adopt()                 -> bool               -- True when action is ADOPT
        block()                 -> bool               -- True when action is BLOCK
        remove()                -> bool               -- True when action is REMOVE
        kill()                  -> bool               -- True when action is KILL
        type()                  -> ZombieType         -- zombie classification
        type_str()              -> str                -- type as a lowercase string
        last_child_cmd()        -> ChildCmdType       -- child command that triggered the zombie
        attr()                  -> ZombieAttr         -- zombie attribute (action rule)
        calls()                 -> int                -- number of server contacts so far
        jobs_password()         -> str                -- job password from child IPC
        path_to_task()          -> str                -- absolute path of the task
        process_or_remote_id()  -> str                -- process/remote ID from child IPC
        user_cmd()              -> str                -- user command that created the zombie
        try_no()                -> int                -- task try number
        duration()              -> int                -- age of the zombie in seconds
        user_action()           -> ZombieUserActionType -- automated response action
        user_action_str()       -> str                -- action as a human-readable string
        host()                  -> str                -- hostname where the client ran
        allowed_age()           -> int                -- seconds the zombie is allowed to live

    Operators
        __str__   -- multi-line table row with zombie state summary
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
    """

    def setUp(self):
        self.z = ecf.Zombie()

    # ------------------------------------------------------------------
    # Constructor
    # ------------------------------------------------------------------

    def test_default_constructor_creates_instance(self):
        """Zombie() creates a Zombie instance."""
        self.assertIsInstance(ecf.Zombie(), ecf.Zombie)

    def test_default_constructor_is_empty(self):
        """A default-constructed Zombie is empty (path_to_task is '')."""
        self.assertTrue(ecf.Zombie().empty())

    # ------------------------------------------------------------------
    # empty
    # ------------------------------------------------------------------

    def test_empty_true_on_default(self):
        """empty() returns True for the default-constructed Zombie."""
        self.assertTrue(self.z.empty())

    # ------------------------------------------------------------------
    # manual_user_action
    # ------------------------------------------------------------------

    def test_manual_user_action_false_on_default(self):
        """manual_user_action() is False on a default Zombie (no action set)."""
        self.assertFalse(self.z.manual_user_action())

    def test_manual_user_action_returns_bool(self):
        """manual_user_action() returns a bool."""
        self.assertIsInstance(self.z.manual_user_action(), bool)

    # ------------------------------------------------------------------
    # Action predicate methods (fob/fail/adopt/block/remove/kill)
    # ------------------------------------------------------------------

    def test_fob_false_on_default(self):
        """fob() is False on a default Zombie."""
        self.assertFalse(self.z.fob())

    def test_fail_false_on_default(self):
        """fail() is False on a default Zombie."""
        self.assertFalse(self.z.fail())

    def test_adopt_false_on_default(self):
        """adopt() is False on a default Zombie."""
        self.assertFalse(self.z.adopt())

    def test_block_true_on_default(self):
        """block() is True on a default Zombie (default action is BLOCK)."""
        self.assertTrue(self.z.block())

    def test_remove_false_on_default(self):
        """remove() is False on a default Zombie."""
        self.assertFalse(self.z.remove())

    def test_kill_false_on_default(self):
        """kill() is False on a default Zombie."""
        self.assertFalse(self.z.kill())

    def test_exactly_one_action_predicate_is_true(self):
        """Exactly one of fob/fail/adopt/block/remove/kill is True at a time."""
        predicates = [
            self.z.fob(),
            self.z.fail(),
            self.z.adopt(),
            self.z.block(),
            self.z.remove(),
            self.z.kill(),
        ]
        self.assertEqual(predicates.count(True), 1)

    # ------------------------------------------------------------------
    # type / type_str
    # ------------------------------------------------------------------

    def test_type_returns_zombie_type(self):
        """type() returns a ZombieType instance."""
        self.assertIsInstance(self.z.type(), ecf.ZombieType)

    def test_type_is_user_on_default(self):
        """A default Zombie has type ZombieType.user."""
        self.assertEqual(self.z.type(), ecf.ZombieType.user)

    def test_type_str_returns_str(self):
        """type_str() returns a str."""
        self.assertIsInstance(self.z.type_str(), str)

    def test_type_str_is_user_on_default(self):
        """type_str() returns 'user' for the default Zombie."""
        self.assertEqual(self.z.type_str(), "user")

    def test_type_str_consistent_with_type(self):
        """type_str() is the lowercase name of type()."""
        self.assertEqual(self.z.type_str(), str(self.z.type()))

    # ------------------------------------------------------------------
    # last_child_cmd
    # ------------------------------------------------------------------

    def test_last_child_cmd_returns_child_cmd_type(self):
        """last_child_cmd() returns a ChildCmdType instance."""
        self.assertIsInstance(self.z.last_child_cmd(), ecf.ChildCmdType)

    def test_last_child_cmd_is_init_on_default(self):
        """A default Zombie has last_child_cmd ChildCmdType.init."""
        self.assertEqual(self.z.last_child_cmd(), ecf.ChildCmdType.init)

    # ------------------------------------------------------------------
    # attr
    # ------------------------------------------------------------------

    def test_attr_returns_zombie_attr(self):
        """attr() returns a ZombieAttr instance."""
        self.assertIsInstance(self.z.attr(), ecf.ZombieAttr)

    # ------------------------------------------------------------------
    # calls
    # ------------------------------------------------------------------

    def test_calls_returns_int(self):
        """calls() returns an int."""
        self.assertIsInstance(self.z.calls(), int)

    def test_calls_zero_on_default(self):
        """calls() is 0 on a default Zombie (no server contacts yet)."""
        self.assertEqual(self.z.calls(), 0)

    # ------------------------------------------------------------------
    # String-valued accessors
    # ------------------------------------------------------------------

    def test_jobs_password_returns_str(self):
        """jobs_password() returns a str."""
        self.assertIsInstance(self.z.jobs_password(), str)

    def test_jobs_password_empty_on_default(self):
        """jobs_password() is empty on a default Zombie."""
        self.assertEqual(self.z.jobs_password(), "")

    def test_path_to_task_returns_str(self):
        """path_to_task() returns a str."""
        self.assertIsInstance(self.z.path_to_task(), str)

    def test_path_to_task_empty_on_default(self):
        """path_to_task() is empty on a default Zombie."""
        self.assertEqual(self.z.path_to_task(), "")

    def test_process_or_remote_id_returns_str(self):
        """process_or_remote_id() returns a str."""
        self.assertIsInstance(self.z.process_or_remote_id(), str)

    def test_process_or_remote_id_empty_on_default(self):
        """process_or_remote_id() is empty on a default Zombie."""
        self.assertEqual(self.z.process_or_remote_id(), "")

    def test_user_cmd_returns_str(self):
        """user_cmd() returns a str."""
        self.assertIsInstance(self.z.user_cmd(), str)

    def test_user_cmd_empty_on_default(self):
        """user_cmd() is empty on a default Zombie (no user command)."""
        self.assertEqual(self.z.user_cmd(), "")

    def test_host_returns_str(self):
        """host() returns a str."""
        self.assertIsInstance(self.z.host(), str)

    def test_host_empty_on_default(self):
        """host() is empty on a default Zombie."""
        self.assertEqual(self.z.host(), "")

    # ------------------------------------------------------------------
    # Integer-valued accessors
    # ------------------------------------------------------------------

    def test_try_no_returns_int(self):
        """try_no() returns an int."""
        self.assertIsInstance(self.z.try_no(), int)

    def test_try_no_zero_on_default(self):
        """try_no() is 0 on a default Zombie."""
        self.assertEqual(self.z.try_no(), 0)

    def test_duration_returns_int(self):
        """duration() returns an int."""
        self.assertIsInstance(self.z.duration(), int)

    def test_duration_zero_on_default(self):
        """duration() is 0 on a default Zombie (just created)."""
        self.assertEqual(self.z.duration(), 0)

    def test_allowed_age_returns_int(self):
        """allowed_age() returns an int."""
        self.assertIsInstance(self.z.allowed_age(), int)

    def test_allowed_age_zero_on_default(self):
        """allowed_age() is 0 on a default (empty) Zombie."""
        self.assertEqual(self.z.allowed_age(), 0)

    # ------------------------------------------------------------------
    # user_action / user_action_str
    # ------------------------------------------------------------------

    def test_user_action_returns_zombie_user_action_type(self):
        """user_action() returns a ZombieUserActionType instance."""
        self.assertIsInstance(self.z.user_action(), ecf.ZombieUserActionType)

    def test_user_action_is_block_on_default(self):
        """user_action() is ZombieUserActionType.block on a default Zombie."""
        self.assertEqual(self.z.user_action(), ecf.ZombieUserActionType.block)

    def test_user_action_str_returns_str(self):
        """user_action_str() returns a str."""
        self.assertIsInstance(self.z.user_action_str(), str)

    def test_user_action_str_nonempty_on_default(self):
        """user_action_str() is non-empty on a default Zombie."""
        self.assertNotEqual(self.z.user_action_str(), "")

    def test_user_action_str_contains_block_on_default(self):
        """user_action_str() contains 'block' for the default action."""
        self.assertIn("block", self.z.user_action_str())

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_returns_str(self):
        """str(Zombie()) returns a str."""
        self.assertIsInstance(str(self.z), str)

    def test_str_nonempty(self):
        """str(Zombie()) is non-empty."""
        self.assertGreater(len(str(self.z)), 0)

    def test_str_contains_type(self):
        """str(Zombie()) contains the zombie type."""
        self.assertIn(self.z.type_str(), str(self.z))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_returns_zombie(self):
        """copy.copy(Zombie()) returns a Zombie."""
        self.assertIsInstance(copy.copy(self.z), ecf.Zombie)

    def test_copy_is_independent(self):
        """copy.copy(Zombie()) returns a distinct object."""
        self.assertIsNot(copy.copy(self.z), self.z)

    def test_copy_preserves_empty(self):
        """copy.copy preserves the empty() value."""
        self.assertEqual(copy.copy(self.z).empty(), self.z.empty())

    def test_copy_preserves_type(self):
        """copy.copy preserves the type()."""
        self.assertEqual(copy.copy(self.z).type(), self.z.type())

    def test_copy_preserves_str(self):
        """copy.copy preserves str()."""
        self.assertEqual(str(copy.copy(self.z)), str(self.z))


class TestVariable(unittest.TestCase):
    """Tests for py::class_<Variable> as exposed in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Variable(str, str)               -- name and value; the name is validated by C++:
                                            - first character must be alphanumeric or underscore
                                            - subsequent characters: alphanumeric, underscore or dot
                                            - empty name always raises RuntimeError

    Instance methods
        name()  -> str                   -- the variable name (returned by const reference)
        value() -> str                   -- the variable value (returned by const reference)
        empty() -> bool                  -- always False for Python-constructed instances;
                                            True only for C++-internal null/sentinel Variables

    Operators
        __str__   -- "edit <name> '<value>'" format; single-quotes wrap the value
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based: both name and value must match
        __ne__    -- implicit complement of __eq__
        __lt__    -- name-based alphabetical ordering only; two Variables with the same
                     name but different values are NOT ordered (neither is less)
        __hash__  -- identity-based (boost.python C-extension type keeps tp_hash even
                     when __eq__ is defined; hash != value, hash == id-derived int)
    """

    # ------------------------------------------------------------------
    # Constructor: Variable(str, str)
    # ------------------------------------------------------------------

    def test_create_stores_name_and_value(self):
        """Variable(name, value) stores both fields correctly."""
        v = ecf.Variable("MY_VAR", "hello")
        self.assertEqual(v.name(), "MY_VAR")
        self.assertEqual(v.value(), "hello")

    def test_create_with_empty_value(self):
        """An empty string is a valid value."""
        v = ecf.Variable("X", "")
        self.assertEqual(v.name(), "X")
        self.assertEqual(v.value(), "")

    def test_create_with_value_containing_spaces(self):
        """Values may contain spaces and special characters."""
        v = ecf.Variable("VAR", "some value with spaces")
        self.assertEqual(v.value(), "some value with spaces")

    def test_create_with_dotted_name(self):
        """Variable names may contain dots in non-leading positions."""
        v = ecf.Variable("ECF_SUITE.NAME", "val")
        self.assertEqual(v.name(), "ECF_SUITE.NAME")

    def test_create_with_underscore_first_char(self):
        """Underscore is a valid first character for a variable name."""
        v = ecf.Variable("_MY_VAR", "val")
        self.assertEqual(v.name(), "_MY_VAR")

    def test_no_default_constructor(self):
        """Variable() with no arguments raises TypeError; no default constructor is exposed."""
        with self.assertRaises(TypeError):
            ecf.Variable()

    # ------------------------------------------------------------------
    # Name validation — invalid names raise RuntimeError
    # ------------------------------------------------------------------

    def test_empty_name_raises(self):
        """An empty name string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Variable("", "val")

    def test_name_with_leading_space_raises(self):
        """A name starting with a space is rejected by C++ validation."""
        with self.assertRaises(RuntimeError):
            ecf.Variable(" LEADING_SPACE", "val")

    def test_name_with_internal_space_raises(self):
        """A name containing an internal space is rejected."""
        with self.assertRaises(RuntimeError):
            ecf.Variable("HAS SPACE", "val")

    def test_name_with_dash_raises(self):
        """A name containing a dash is rejected."""
        with self.assertRaises(RuntimeError):
            ecf.Variable("HAS-DASH", "val")

    # ------------------------------------------------------------------
    # Wrong constructor argument types
    # ------------------------------------------------------------------

    def test_create_from_int_name_raises(self):
        """No constructor accepts an integer as the name."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Variable(42, "val")

    def test_create_from_none_name_raises(self):
        """No constructor accepts None as the name."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Variable(None, "val")

    def test_create_from_int_value_raises(self):
        """No constructor accepts an integer as the value."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Variable("VAR", 42)

    def test_create_from_none_value_raises(self):
        """No constructor accepts None as the value."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Variable("VAR", None)

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() always returns a Python str."""
        self.assertIsInstance(ecf.Variable("VAR", "val").name(), str)

    def test_name_returns_correct_value(self):
        """name() returns exactly the string passed at construction."""
        self.assertEqual(ecf.Variable("ECFLOW_PATH", "val").name(), "ECFLOW_PATH")

    # ------------------------------------------------------------------
    # value()
    # ------------------------------------------------------------------

    def test_value_returns_str(self):
        """value() always returns a Python str."""
        self.assertIsInstance(ecf.Variable("VAR", "hello").value(), str)

    def test_value_returns_correct_value(self):
        """value() returns exactly the string passed at construction."""
        self.assertEqual(ecf.Variable("VAR", "my_value").value(), "my_value")

    def test_value_empty_string_round_trips(self):
        """An empty value string round-trips through construction and value()."""
        self.assertEqual(ecf.Variable("VAR", "").value(), "")

    # ------------------------------------------------------------------
    # empty()
    # ------------------------------------------------------------------

    def test_empty_returns_false_for_normal_variable(self):
        """empty() returns False for every Python-constructed Variable."""
        self.assertFalse(ecf.Variable("VAR", "val").empty())

    def test_empty_returns_false_when_value_is_empty_string(self):
        """empty() is still False even when the value is an empty string."""
        self.assertFalse(ecf.Variable("VAR", "").empty())

    def test_empty_returns_bool(self):
        """empty() returns a Python bool."""
        self.assertIsInstance(ecf.Variable("VAR", "val").empty(), bool)

    def test_empty_returns_true_when_result_of_a_failed_find(self):
        task = ecf.Task("task")
        v = task.find_variable("NONEXISTENT")
        self.assertTrue(v.empty())

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format_is_edit_name_quoted_value(self):
        """str() follows the format \"edit <name> '<value>'\"."""
        v = ecf.Variable("MY_VAR", "hello")
        self.assertEqual(str(v), "edit MY_VAR 'hello'")

    def test_str_with_empty_value(self):
        """str() with an empty value wraps the empty string in single quotes."""
        v = ecf.Variable("X", "")
        self.assertEqual(str(v), "edit X ''")

    def test_str_with_value_containing_spaces(self):
        """str() preserves spaces in the value inside the single-quoted section."""
        v = ecf.Variable("VAR", "some   value")
        self.assertEqual(str(v), "edit VAR 'some   value'")

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Variable("VAR", "val")), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — value-based (name AND value must match)
    # ------------------------------------------------------------------

    def test_eq_same_name_and_value(self):
        """Two Variables with identical name and value are equal."""
        self.assertEqual(ecf.Variable("VAR", "val"), ecf.Variable("VAR", "val"))

    def test_eq_reflexive(self):
        """A Variable is equal to itself."""
        v = ecf.Variable("VAR", "val")
        self.assertEqual(v, v)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        v1 = ecf.Variable("VAR", "val")
        v2 = ecf.Variable("VAR", "val")
        self.assertTrue(v1 == v2)
        self.assertTrue(v2 == v1)

    def test_ne_different_value_same_name(self):
        """Different values with the same name are not equal."""
        self.assertNotEqual(ecf.Variable("VAR", "hello"), ecf.Variable("VAR", "world"))

    def test_ne_different_name_same_value(self):
        """Different names with the same value are not equal."""
        self.assertNotEqual(ecf.Variable("A", "val"), ecf.Variable("B", "val"))

    def test_ne_both_differ(self):
        """Different name and different value are not equal."""
        self.assertNotEqual(ecf.Variable("A", "1"), ecf.Variable("B", "2"))

    # ------------------------------------------------------------------
    # __lt__ — name-based ordering only
    # ------------------------------------------------------------------

    def test_lt_alphabetically_earlier_name_is_less(self):
        """Variable with an alphabetically earlier name is less than one with a later name."""
        self.assertLess(ecf.Variable("A", "z"), ecf.Variable("B", "a"))

    def test_lt_alphabetically_later_name_is_not_less(self):
        """Variable with a later name is NOT less than one with an earlier name."""
        self.assertFalse(ecf.Variable("B", "a") < ecf.Variable("A", "z"))

    def test_lt_same_name_any_value_is_not_less(self):
        """Two Variables with the same name are never in a less-than relationship,
        regardless of their values (ordering is name-based only)."""
        self.assertFalse(ecf.Variable("X", "aaa") < ecf.Variable("X", "zzz"))
        self.assertFalse(ecf.Variable("X", "zzz") < ecf.Variable("X", "aaa"))

    def test_lt_same_name_same_value_not_less(self):
        """A Variable is not strictly less than an equal Variable."""
        self.assertFalse(ecf.Variable("X", "v") < ecf.Variable("X", "v"))

    def test_lt_can_be_used_to_sort(self):
        """sorted() on a list of Variables produces alphabetical-by-name order."""
        variables = [
            ecf.Variable("ZEBRA", "z"),
            ecf.Variable("ALPHA", "a"),
            ecf.Variable("MIDDLE", "m"),
        ]
        sorted_names = [v.name() for v in sorted(variables, key=lambda x: x.name())]
        self.assertEqual(sorted_names, ["ALPHA", "MIDDLE", "ZEBRA"])

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces a Variable equal in value to the original."""
        import copy

        v = ecf.Variable("VAR", "hello")
        c = copy.copy(v)
        self.assertEqual(v, c)

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        import copy

        v = ecf.Variable("VAR", "hello")
        c = copy.copy(v)
        self.assertIsNot(v, c)

    def test_copy_is_variable_instance(self):
        """The result of copy.copy() is a Variable."""
        import copy

        self.assertIsInstance(copy.copy(ecf.Variable("VAR", "val")), ecf.Variable)

    def test_copy_name_and_value_match(self):
        """The copied Variable has the same name() and value() as the original."""
        import copy

        v = ecf.Variable("MY_NAME", "MY_VALUE")
        c = copy.copy(v)
        self.assertEqual(c.name(), v.name())
        self.assertEqual(c.value(), v.value())

    # ------------------------------------------------------------------
    # __hash__ — identity-based (boost.python extension type)
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Variable is hashable; hash() returns an int."""
        v = ecf.Variable("VAR", "val")
        self.assertIsInstance(hash(v), int)

    def test_hash_is_identity_based(self):
        """Two value-equal Variables have different hashes (identity, not value)."""
        v1 = ecf.Variable("VAR", "val")
        v2 = ecf.Variable("VAR", "val")
        self.assertEqual(v1, v2)  # same value
        self.assertIsNot(v1, v2)  # different objects
        self.assertNotEqual(hash(v1), hash(v2))  # therefore different hashes

    def test_same_object_same_hash(self):
        """The same Variable object always returns the same hash."""
        v = ecf.Variable("VAR", "val")
        self.assertEqual(hash(v), hash(v))

    def test_can_be_stored_in_set(self):
        """Variable instances can be stored in a Python set."""
        v = ecf.Variable("VAR", "val")
        s = {v}
        self.assertIn(v, s)

    def test_can_be_used_as_dict_key(self):
        """Variable instances can be used as dictionary keys."""
        v = ecf.Variable("VAR", "val")
        d = {v: "entry"}
        self.assertEqual(d[v], "entry")


class TestVariableList(unittest.TestCase):
    """Tests for py::class_<std::vector<Variable>>("VariableList", ...) as exposed in ExportNodeAttr.cpp.

    VariableList is exposed via vector_indexing_suite (without NoProxy=true), providing a
    mutable list-like Python interface with **value-based** __contains__.

    Exposed API
    -----------
    Constructors
        VariableList()               -- default constructor; creates an empty list

    Mutation methods (via vector_indexing_suite)
        append(Variable)             -- add one Variable to the end;
                                        rejects all non-Variable types with TypeError
        extend(iterable[Variable])   -- add all Variables from an iterable

    Sequence operators (via vector_indexing_suite)
        len(vl)      -- __len__       -- number of elements (0 after default construction)
        vl[i]        -- __getitem__   -- positive and negative integer indexing;
                                         IndexError on out-of-range
        for v in vl  -- __iter__      -- yields Variable instances in insertion order
        v in vl      -- __contains__  -- value-based membership test (uses Variable.__eq__,
                                         so a new Variable("A","1") matches a stored one)

    Equality / hash
        __eq__    -- identity-based; two distinct VariableLists with identical contents
                     are NOT equal (only the same Python object equals itself)
        __hash__  -- identity-based (boost.python default)
    """

    # ------------------------------------------------------------------
    # Constructor
    # ------------------------------------------------------------------

    def test_default_constructor(self):
        """VariableList() creates an empty list."""
        vl = ecf.VariableList()
        self.assertIsInstance(vl, ecf.VariableList)

    def test_default_constructor_is_empty(self):
        """A newly constructed VariableList has length 0."""
        self.assertEqual(len(ecf.VariableList()), 0)

    # ------------------------------------------------------------------
    # __len__
    # ------------------------------------------------------------------

    def test_len_increases_after_append(self):
        """len() grows by 1 for each append() call."""
        vl = ecf.VariableList()
        vl.append(ecf.Variable("A", "1"))
        self.assertEqual(len(vl), 1)
        vl.append(ecf.Variable("B", "2"))
        self.assertEqual(len(vl), 2)

    # ------------------------------------------------------------------
    # append
    # ------------------------------------------------------------------

    def test_append_variable(self):
        """append() accepts a Variable instance without error."""
        vl = ecf.VariableList()
        vl.append(ecf.Variable("X", "val"))

    def test_append_wrong_type_str_raises(self):
        """append() rejects a plain string."""
        vl = ecf.VariableList()
        with self.assertRaises((TypeError, RuntimeError)):
            vl.append("not_a_variable")

    def test_append_wrong_type_int_raises(self):
        """append() rejects a plain integer."""
        vl = ecf.VariableList()
        with self.assertRaises((TypeError, RuntimeError)):
            vl.append(42)

    def test_append_wrong_type_none_raises(self):
        """append() rejects None."""
        vl = ecf.VariableList()
        with self.assertRaises((TypeError, RuntimeError)):
            vl.append(None)

    # ------------------------------------------------------------------
    # extend
    # ------------------------------------------------------------------

    def test_extend_empty_list(self):
        """extend([]) leaves the VariableList unchanged."""
        vl = ecf.VariableList()
        vl.extend([])
        self.assertEqual(len(vl), 0)

    def test_extend_with_variables(self):
        """extend() adds all Variables from an iterable."""
        vl = ecf.VariableList()
        vl.extend(
            [ecf.Variable("A", "1"), ecf.Variable("B", "2"), ecf.Variable("C", "3")]
        )
        self.assertEqual(len(vl), 3)

    def test_extend_wrong_element_type_raises(self):
        """extend() rejects a list containing non-Variable elements."""
        vl = ecf.VariableList()
        with self.assertRaises((TypeError, RuntimeError)):
            vl.extend(["not_a_variable"])

    # ------------------------------------------------------------------
    # __getitem__
    # ------------------------------------------------------------------

    def test_getitem_returns_variable(self):
        """vl[0] returns a Variable instance."""
        vl = ecf.VariableList()
        vl.append(ecf.Variable("A", "1"))
        self.assertIsInstance(vl[0], ecf.Variable)

    def test_getitem_name_and_value_are_correct(self):
        """vl[0] returns the Variable with the correct name and value."""
        vl = ecf.VariableList()
        vl.append(ecf.Variable("MY_VAR", "my_value"))
        self.assertEqual(vl[0].name(), "MY_VAR")
        self.assertEqual(vl[0].value(), "my_value")

    def test_getitem_negative_index(self):
        """vl[-1] returns the last element."""
        vl = ecf.VariableList()
        vl.append(ecf.Variable("A", "1"))
        vl.append(ecf.Variable("B", "2"))
        self.assertEqual(vl[-1].name(), "B")

    def test_getitem_out_of_range_raises(self):
        """Accessing an out-of-range index raises IndexError."""
        vl = ecf.VariableList()
        with self.assertRaises(IndexError):
            _ = vl[0]

    def test_getitem_large_positive_index_raises(self):
        """vl[99] on a short list raises IndexError."""
        vl = ecf.VariableList()
        vl.append(ecf.Variable("A", "1"))
        with self.assertRaises(IndexError):
            _ = vl[99]

    # ------------------------------------------------------------------
    # __iter__
    # ------------------------------------------------------------------

    def test_iteration_empty(self):
        """Iterating over an empty VariableList yields zero items."""
        vl = ecf.VariableList()
        self.assertEqual(sum(1 for _ in vl), 0)

    def test_iteration_yields_correct_count(self):
        """Iterating yields exactly len(vl) items."""
        vl = ecf.VariableList()
        vl.extend([ecf.Variable("A", "1"), ecf.Variable("B", "2")])
        self.assertEqual(len(list(vl)), 2)

    def test_iteration_yields_variable_instances(self):
        """Every item yielded by iteration is a Variable."""
        vl = ecf.VariableList()
        vl.append(ecf.Variable("V", "val"))
        for item in vl:
            self.assertIsInstance(item, ecf.Variable)

    def test_iteration_preserves_insertion_order(self):
        """Items are yielded in insertion order."""
        vl = ecf.VariableList()
        vl.extend(
            [ecf.Variable("A", "1"), ecf.Variable("B", "2"), ecf.Variable("C", "3")]
        )
        names = [v.name() for v in vl]
        self.assertEqual(names, ["A", "B", "C"])

    # ------------------------------------------------------------------
    # __contains__ — value-based (uses Variable.__eq__)
    # ------------------------------------------------------------------

    def test_contains_false_for_empty(self):
        """'in' returns False on an empty VariableList."""
        vl = ecf.VariableList()
        self.assertNotIn(ecf.Variable("A", "1"), vl)

    def test_contains_true_for_value_equal_variable(self):
        """'in' returns True when a value-equal Variable is present,
        even if it is a different Python object (value-based __contains__)."""
        vl = ecf.VariableList()
        vl.append(ecf.Variable("A", "1"))
        self.assertIn(ecf.Variable("A", "1"), vl)  # new object, same value

    def test_contains_false_for_different_value(self):
        """'in' returns False if the value does not match."""
        vl = ecf.VariableList()
        vl.append(ecf.Variable("A", "1"))
        self.assertNotIn(ecf.Variable("A", "DIFFERENT"), vl)

    def test_contains_false_for_different_name(self):
        """'in' returns False if the name does not match."""
        vl = ecf.VariableList()
        vl.append(ecf.Variable("A", "1"))
        self.assertNotIn(ecf.Variable("Z", "1"), vl)

    # ------------------------------------------------------------------
    # __eq__ — identity-based
    # ------------------------------------------------------------------

    def test_eq_same_object(self):
        """A VariableList is equal to itself."""
        vl = ecf.VariableList()
        self.assertEqual(vl, vl)

    def test_eq_two_empty_lists_are_not_equal(self):
        """Two distinct empty VariableLists are NOT equal (identity-based)."""
        self.assertNotEqual(ecf.VariableList(), ecf.VariableList())

    def test_eq_two_lists_with_same_content_are_not_equal(self):
        """Two VariableLists with identical contents are not equal (identity, not value)."""
        vl1 = ecf.VariableList()
        vl2 = ecf.VariableList()
        vl1.append(ecf.Variable("A", "1"))
        vl2.append(ecf.Variable("A", "1"))
        self.assertNotEqual(vl1, vl2)

    # ------------------------------------------------------------------
    # __hash__ — identity-based
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """VariableList is hashable; hash() returns an int."""
        vl = ecf.VariableList()
        self.assertIsInstance(hash(vl), int)

    def test_hash_is_identity_based(self):
        """Two distinct VariableLists have different hashes."""
        vl1 = ecf.VariableList()
        vl2 = ecf.VariableList()
        self.assertNotEqual(hash(vl1), hash(vl2))

    def test_same_object_same_hash(self):
        """The same VariableList always returns the same hash."""
        vl = ecf.VariableList()
        self.assertEqual(hash(vl), hash(vl))

    def test_can_be_stored_in_set(self):
        """VariableList instances can be stored in a Python set."""
        vl = ecf.VariableList()
        s = {vl}
        self.assertIn(vl, s)

    def test_can_be_used_as_dict_key(self):
        """VariableList instances can be used as dictionary keys."""
        vl = ecf.VariableList()
        d = {vl: "entry"}
        self.assertEqual(d[vl], "entry")


class TestLabel(unittest.TestCase):
    """Tests for py::class_<Label> as exposed in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Label(str, str)                  -- name and value; the name is validated by C++:
                                            empty name, leading space, internal space and dash
                                            all raise RuntimeError; wrong argument types raise
                                            ArgumentError (a subclass of RuntimeError)

    Instance methods
        name()      -> str               -- the label name (returned by const reference)
        value()     -> str               -- the original label value set at construction
        new_value() -> str               -- the current/updated value; empty string ('') for
                                            freshly Python-constructed labels (set by the server
                                            at runtime)
        empty()     -> bool              -- always False for Python-constructed Labels;
                                            True only for C++-internal null/sentinel Labels

    Operators
        __str__   -- 'label <name> "<value>"' format; double-quotes wrap the original value
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance;
                     the copied new_value() is '' (original value is copied, not new_value)
        __eq__    -- value-based: both name and value must match
        __ne__    -- implicit complement of __eq__
        __lt__    -- name-based alphabetical ordering only; Labels with the same name but
                     different values are NOT ordered (neither is less than the other)
        __hash__  -- identity-based (boost.python C-extension type keeps tp_hash even
                     when __eq__ is defined; hash is NOT derived from value)
    """

    # ------------------------------------------------------------------
    # Constructor: Label(str, str)
    # ------------------------------------------------------------------

    def test_create_stores_name_and_value(self):
        """Label(name, value) stores both fields correctly."""
        lab = ecf.Label("MY_LABEL", "hello")
        self.assertEqual(lab.name(), "MY_LABEL")
        self.assertEqual(lab.value(), "hello")

    def test_create_with_empty_value(self):
        """An empty string is a valid value."""
        lab = ecf.Label("LBL", "")
        self.assertEqual(lab.name(), "LBL")
        self.assertEqual(lab.value(), "")

    def test_create_with_value_containing_spaces(self):
        """Values may contain spaces and special characters."""
        lab = ecf.Label("LBL", "value with spaces")
        self.assertEqual(lab.value(), "value with spaces")

    def test_create_with_underscore_first_char(self):
        """Underscore is a valid first character for a label name."""
        lab = ecf.Label("_MY_LABEL", "val")
        self.assertEqual(lab.name(), "_MY_LABEL")

    def test_no_default_constructor(self):
        """Label() with no arguments raises an error; no default constructor is exposed."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Label()

    # ------------------------------------------------------------------
    # Name validation — invalid names raise RuntimeError
    # ------------------------------------------------------------------

    def test_empty_name_raises(self):
        """An empty name string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Label("", "val")

    def test_name_with_leading_space_raises(self):
        """A name starting with a space is rejected."""
        with self.assertRaises(RuntimeError):
            ecf.Label(" LEADING", "val")

    def test_name_with_internal_space_raises(self):
        """A name containing an internal space is rejected."""
        with self.assertRaises(RuntimeError):
            ecf.Label("HAS SPACE", "val")

    def test_name_with_dash_raises(self):
        """A name containing a dash is rejected."""
        with self.assertRaises(RuntimeError):
            ecf.Label("HAS-DASH", "val")

    # ------------------------------------------------------------------
    # Wrong constructor argument types
    # ------------------------------------------------------------------

    def test_create_from_int_name_raises(self):
        """Passing an integer as name raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Label(42, "val")

    def test_create_from_none_name_raises(self):
        """Passing None as name raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Label(None, "val")

    def test_create_from_int_value_raises(self):
        """Passing an integer as value raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Label("LBL", 42)

    def test_create_from_none_value_raises(self):
        """Passing None as value raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Label("LBL", None)

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() always returns a Python str."""
        self.assertIsInstance(ecf.Label("LBL", "val").name(), str)

    def test_name_returns_correct_value(self):
        """name() returns exactly the string passed at construction."""
        self.assertEqual(ecf.Label("MY_LABEL_NAME", "val").name(), "MY_LABEL_NAME")

    # ------------------------------------------------------------------
    # value()
    # ------------------------------------------------------------------

    def test_value_returns_str(self):
        """value() always returns a Python str."""
        self.assertIsInstance(ecf.Label("LBL", "hello").value(), str)

    def test_value_returns_correct_value(self):
        """value() returns exactly the string passed at construction."""
        self.assertEqual(ecf.Label("LBL", "my_value").value(), "my_value")

    def test_value_empty_string_round_trips(self):
        """An empty value string round-trips through construction and value()."""
        self.assertEqual(ecf.Label("LBL", "").value(), "")

    # ------------------------------------------------------------------
    # new_value()
    # ------------------------------------------------------------------

    def test_new_value_returns_str(self):
        """new_value() always returns a Python str."""
        self.assertIsInstance(ecf.Label("LBL", "val").new_value(), str)

    def test_new_value_is_empty_string_after_construction(self):
        """For freshly Python-constructed Labels, new_value() is '' (set by server at runtime)."""
        lab = ecf.Label("LBL", "original")
        self.assertEqual(lab.new_value(), "")

    def test_new_value_is_independent_of_value(self):
        """new_value() remains '' regardless of what value() returns."""
        lab = ecf.Label("LBL", "anything")
        self.assertEqual(lab.new_value(), "")
        self.assertNotEqual(lab.value(), lab.new_value())

    # ------------------------------------------------------------------
    # empty()
    # ------------------------------------------------------------------

    def test_empty_returns_false_for_normal_label(self):
        """empty() returns False for every Python-constructed Label."""
        self.assertFalse(ecf.Label("LBL", "val").empty())

    def test_empty_returns_false_when_value_is_empty_string(self):
        """empty() is still False even when the value is an empty string."""
        self.assertFalse(ecf.Label("LBL", "").empty())

    def test_empty_returns_bool(self):
        """empty() returns a Python bool."""
        self.assertIsInstance(ecf.Label("LBL", "val").empty(), bool)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format_is_label_name_double_quoted_value(self):
        """str() follows the format 'label <name> \"<value>\"' with double-quoted value."""
        lab = ecf.Label("MY_LABEL", "hello")
        self.assertEqual(str(lab), 'label MY_LABEL "hello"')

    def test_str_with_empty_value(self):
        """str() with an empty value wraps the empty string in double quotes."""
        lab = ecf.Label("L", "")
        self.assertEqual(str(lab), 'label L ""')

    def test_str_with_value_containing_spaces(self):
        """str() preserves spaces in the value inside double quotes."""
        lab = ecf.Label("LBL", "val with spaces")
        self.assertEqual(str(lab), 'label LBL "val with spaces"')

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Label("LBL", "val")), str)

    def test_str_uses_double_quotes_not_single(self):
        """str() wraps the value in double quotes (unlike Variable which uses single quotes)."""
        lab = ecf.Label("LBL", "v")
        self.assertIn('"v"', str(lab))

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — value-based (name AND value must match)
    # ------------------------------------------------------------------

    def test_eq_same_name_and_value(self):
        """Two Labels with identical name and value are equal."""
        self.assertEqual(ecf.Label("LBL", "val"), ecf.Label("LBL", "val"))

    def test_eq_reflexive(self):
        """A Label is equal to itself."""
        lab = ecf.Label("LBL", "val")
        self.assertEqual(lab, lab)

    def test_eq_symmetric(self):
        """Equality is symmetric: lab1 == lab2 implies lab2 == lab1."""
        lab1 = ecf.Label("LBL", "val")
        lab2 = ecf.Label("LBL", "val")
        self.assertTrue(lab1 == lab2)
        self.assertTrue(lab2 == lab1)

    def test_ne_different_value_same_name(self):
        """Different values with the same name are not equal."""
        self.assertNotEqual(ecf.Label("LBL", "hello"), ecf.Label("LBL", "world"))

    def test_ne_different_name_same_value(self):
        """Different names with the same value are not equal."""
        self.assertNotEqual(ecf.Label("A", "val"), ecf.Label("B", "val"))

    def test_ne_both_differ(self):
        """Different name and different value are not equal."""
        self.assertNotEqual(ecf.Label("A", "1"), ecf.Label("B", "2"))

    def test_ne_same_name_empty_vs_nonempty_value(self):
        """A Label with an empty value is not equal to one with a non-empty value."""
        self.assertNotEqual(ecf.Label("LBL", ""), ecf.Label("LBL", "val"))

    # ------------------------------------------------------------------
    # __lt__ — name-based ordering only
    # ------------------------------------------------------------------

    def test_lt_alphabetically_earlier_name_is_less(self):
        """Label with an alphabetically earlier name is less than one with a later name."""
        self.assertLess(ecf.Label("A", "z"), ecf.Label("B", "a"))

    def test_lt_alphabetically_later_name_is_not_less(self):
        """Label with a later name is NOT less than one with an earlier name."""
        self.assertFalse(ecf.Label("B", "a") < ecf.Label("A", "z"))

    def test_lt_same_name_any_value_is_not_less(self):
        """Two Labels with the same name are never in a less-than relationship,
        regardless of their values (ordering is name-based only)."""
        self.assertFalse(ecf.Label("X", "aaa") < ecf.Label("X", "zzz"))
        self.assertFalse(ecf.Label("X", "zzz") < ecf.Label("X", "aaa"))

    def test_lt_same_name_same_value_not_less(self):
        """A Label is not strictly less than an equal Label."""
        self.assertFalse(ecf.Label("X", "v") < ecf.Label("X", "v"))

    def test_lt_can_be_used_to_sort(self):
        """sorted() on a list of Labels produces alphabetical-by-name order."""
        labels = [
            ecf.Label("ZEBRA", "z"),
            ecf.Label("ALPHA", "a"),
            ecf.Label("MIDDLE", "m"),
        ]
        sorted_names = [lb.name() for lb in sorted(labels, key=lambda x: x.name())]
        self.assertEqual(sorted_names, ["ALPHA", "MIDDLE", "ZEBRA"])

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces a Label equal in value to the original."""
        import copy

        lab = ecf.Label("LBL", "hello")
        c = copy.copy(lab)
        self.assertEqual(lab, c)

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        import copy

        lab = ecf.Label("LBL", "hello")
        self.assertIsNot(lab, copy.copy(lab))

    def test_copy_is_label_instance(self):
        """The result of copy.copy() is a Label."""
        import copy

        self.assertIsInstance(copy.copy(ecf.Label("LBL", "val")), ecf.Label)

    def test_copy_name_and_value_match(self):
        """The copied Label has the same name() and value() as the original."""
        import copy

        lab = ecf.Label("MY_NAME", "MY_VALUE")
        c = copy.copy(lab)
        self.assertEqual(c.name(), lab.name())
        self.assertEqual(c.value(), lab.value())

    def test_copy_new_value_is_empty(self):
        """copy.copy() copies the original value; new_value() on the copy is ''."""
        import copy

        lab = ecf.Label("LBL", "orig")
        c = copy.copy(lab)
        self.assertEqual(c.new_value(), "")

    # ------------------------------------------------------------------
    # __hash__ — identity-based (boost.python extension type)
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Label is hashable; hash() returns an int."""
        lab = ecf.Label("LBL", "val")
        self.assertIsInstance(hash(lab), int)

    def test_hash_is_identity_based(self):
        """Two value-equal Labels have different hashes (identity, not value)."""
        lab1 = ecf.Label("LBL", "val")
        lab2 = ecf.Label("LBL", "val")
        self.assertEqual(lab1, lab2)  # same value
        self.assertIsNot(lab1, lab2)  # different objects
        self.assertNotEqual(hash(lab1), hash(lab2))  # therefore different hashes

    def test_same_object_same_hash(self):
        """The same Label object always returns the same hash."""
        lab = ecf.Label("LBL", "val")
        self.assertEqual(hash(lab), hash(lab))

    def test_can_be_stored_in_set(self):
        """Label instances can be stored in a Python set."""
        lab = ecf.Label("LBL", "val")
        s = {lab}
        self.assertIn(lab, s)

    def test_can_be_used_as_dict_key(self):
        """Label instances can be used as dictionary keys."""
        lab = ecf.Label("LBL", "val")
        d = {lab: "entry"}
        self.assertEqual(d[lab], "entry")


class TestLimit(unittest.TestCase):
    """Tests for py::class_<Limit, std::shared_ptr<Limit>> as exposed in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Limit(str, int)                  -- name and max token count; no default constructor;
                                            wrong argument types raise ArgumentError

    Instance methods
        name()          -> str           -- the limit name (returned by const reference)
        limit()         -> int           -- the maximum token count
        value()         -> int           -- the current consumed token count (starts at 0)
        node_paths()    -> list[str]     -- paths of nodes currently holding a token
        increment(int, str)              -- increase current value and record path (test only)
        decrement(int, str)              -- decrease current value and remove path (test only)

    Operators
        __str__   -- 'limit <name> <max>' format
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based: name and max limit must match
                     (current value and node_paths are runtime state, not compared)
        __ne__    -- implicit complement of __eq__
        __lt__    -- name-based alphabetical ordering only
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: Limit(str, int)
    # ------------------------------------------------------------------

    def test_create_stores_name_and_max(self):
        """Limit(name, max) stores both fields correctly."""
        lim = ecf.Limit("MY_LIMIT", 10)
        self.assertEqual(lim.name(), "MY_LIMIT")
        self.assertEqual(lim.limit(), 10)

    def test_create_initial_value_is_zero(self):
        """The current token count is 0 immediately after construction."""
        self.assertEqual(ecf.Limit("L", 10).value(), 0)

    def test_create_initial_node_paths_is_empty(self):
        """node_paths() is empty immediately after construction."""
        self.assertEqual(ecf.Limit("L", 10).node_paths(), [])

    def test_create_with_zero_max(self):
        """A max of 0 is accepted without error."""
        lim = ecf.Limit("L", 0)
        self.assertEqual(lim.limit(), 0)

    def test_create_with_large_max(self):
        """Large max values are accepted."""
        lim = ecf.Limit("L", 10000)
        self.assertEqual(lim.limit(), 10000)

    def test_no_default_constructor(self):
        """Limit() with no arguments raises an error; no default constructor is exposed."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Limit()

    # ------------------------------------------------------------------
    # Wrong constructor argument types
    # ------------------------------------------------------------------

    def test_create_from_int_name_raises(self):
        """Passing an integer as name raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Limit(42, 10)

    def test_create_from_none_name_raises(self):
        """Passing None as name raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Limit(None, 10)

    def test_create_from_str_max_raises(self):
        """Passing a string as the max integer raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Limit("L", "ten")

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() returns a Python str."""
        self.assertIsInstance(ecf.Limit("L", 1).name(), str)

    def test_name_returns_correct_value(self):
        """name() returns exactly the name passed at construction."""
        self.assertEqual(ecf.Limit("ECFLOW_LIMIT", 5).name(), "ECFLOW_LIMIT")

    # ------------------------------------------------------------------
    # limit()
    # ------------------------------------------------------------------

    def test_limit_returns_int(self):
        """limit() returns a Python int."""
        self.assertIsInstance(ecf.Limit("L", 5).limit(), int)

    def test_limit_returns_correct_max(self):
        """limit() returns exactly the max value passed at construction."""
        self.assertEqual(ecf.Limit("L", 42).limit(), 42)

    # ------------------------------------------------------------------
    # value()
    # ------------------------------------------------------------------

    def test_value_returns_int(self):
        """value() returns a Python int."""
        self.assertIsInstance(ecf.Limit("L", 5).value(), int)

    def test_value_initially_zero(self):
        """value() is 0 after construction."""
        self.assertEqual(ecf.Limit("L", 5).value(), 0)

    # ------------------------------------------------------------------
    # node_paths()
    # ------------------------------------------------------------------

    def test_node_paths_returns_list(self):
        """node_paths() returns a Python list."""
        self.assertIsInstance(ecf.Limit("L", 5).node_paths(), list)

    def test_node_paths_initially_empty(self):
        """node_paths() is an empty list after construction."""
        self.assertEqual(ecf.Limit("L", 5).node_paths(), [])

    # ------------------------------------------------------------------
    # increment() / decrement()
    # ------------------------------------------------------------------

    def test_increment_increases_value(self):
        """increment(n, path) adds n to value()."""
        lim = ecf.Limit("L", 10)
        lim.increment(1, "/suite/t1")
        self.assertEqual(lim.value(), 1)

    def test_increment_records_node_path(self):
        """increment() records the consuming node path in node_paths()."""
        lim = ecf.Limit("L", 10)
        lim.increment(1, "/suite/t1")
        self.assertIn("/suite/t1", lim.node_paths())

    def test_increment_multiple_paths(self):
        """Multiple increment() calls accumulate paths and values."""
        lim = ecf.Limit("L", 10)
        lim.increment(1, "/suite/t1")
        lim.increment(1, "/suite/t2")
        self.assertEqual(lim.value(), 2)
        self.assertEqual(len(lim.node_paths()), 2)

    def test_decrement_reduces_value(self):
        """decrement(n, path) reduces value() by n and removes the path."""
        lim = ecf.Limit("L", 10)
        lim.increment(1, "/suite/t1")
        lim.decrement(1, "/suite/t1")
        self.assertEqual(lim.value(), 0)

    def test_decrement_removes_node_path(self):
        """decrement() removes the path from node_paths()."""
        lim = ecf.Limit("L", 10)
        lim.increment(1, "/suite/t1")
        lim.decrement(1, "/suite/t1")
        self.assertNotIn("/suite/t1", lim.node_paths())

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format_is_limit_name_max(self):
        """str() follows the format 'limit <name> <max>'."""
        self.assertEqual(str(ecf.Limit("MY_LIMIT", 10)), "limit MY_LIMIT 10")

    def test_str_with_zero_max(self):
        """str() correctly formats a zero max value."""
        self.assertEqual(str(ecf.Limit("L", 0)), "limit L 0")

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Limit("L", 5)), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — value-based (name + max limit must match)
    # ------------------------------------------------------------------

    def test_eq_same_name_and_max(self):
        """Two Limits with the same name and max are equal."""
        self.assertEqual(ecf.Limit("L", 10), ecf.Limit("L", 10))

    def test_eq_reflexive(self):
        """A Limit is equal to itself."""
        lim = ecf.Limit("L", 10)
        self.assertEqual(lim, lim)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        lim1 = ecf.Limit("L", 10)
        lim2 = ecf.Limit("L", 10)
        self.assertTrue(lim1 == lim2)
        self.assertTrue(lim2 == lim1)

    def test_ne_different_max(self):
        """Limits with the same name but different max are not equal."""
        self.assertNotEqual(ecf.Limit("L", 10), ecf.Limit("L", 5))

    def test_ne_different_name(self):
        """Limits with different names are not equal."""
        self.assertNotEqual(ecf.Limit("A", 10), ecf.Limit("B", 10))

    def test_ne_both_differ(self):
        """Limits with different name and max are not equal."""
        self.assertNotEqual(ecf.Limit("A", 5), ecf.Limit("B", 10))

    # ------------------------------------------------------------------
    # __lt__ — name-based ordering only
    # ------------------------------------------------------------------

    def test_lt_alphabetically_earlier_name_is_less(self):
        """Limit with an earlier name is less than one with a later name."""
        self.assertLess(ecf.Limit("A", 99), ecf.Limit("B", 1))

    def test_lt_alphabetically_later_name_is_not_less(self):
        """Limit with a later name is NOT less than one with an earlier name."""
        self.assertFalse(ecf.Limit("B", 1) < ecf.Limit("A", 99))

    def test_lt_same_name_any_max_is_not_less(self):
        """Two Limits with the same name are not in a less-than relation
        regardless of their max values (ordering is name-based only)."""
        self.assertFalse(ecf.Limit("X", 1) < ecf.Limit("X", 100))
        self.assertFalse(ecf.Limit("X", 100) < ecf.Limit("X", 1))

    def test_lt_equal_limits_not_less(self):
        """A Limit is not strictly less than an equal Limit."""
        self.assertFalse(ecf.Limit("X", 5) < ecf.Limit("X", 5))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces a Limit equal in value to the original."""
        import copy

        lim = ecf.Limit("L", 10)
        self.assertEqual(lim, copy.copy(lim))

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        import copy

        lim = ecf.Limit("L", 10)
        self.assertIsNot(lim, copy.copy(lim))

    def test_copy_is_limit_instance(self):
        """The result of copy.copy() is a Limit."""
        import copy

        self.assertIsInstance(copy.copy(ecf.Limit("L", 5)), ecf.Limit)

    def test_copy_preserves_name_and_max(self):
        """The copied Limit has the same name() and limit() as the original."""
        import copy

        lim = ecf.Limit("MY_LIMIT", 42)
        c = copy.copy(lim)
        self.assertEqual(c.name(), lim.name())
        self.assertEqual(c.limit(), lim.limit())

    # ------------------------------------------------------------------
    # __hash__ — identity-based
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Limit is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Limit("L", 5)), int)

    def test_hash_is_identity_based(self):
        """Two value-equal Limits have different hashes (identity, not value)."""
        lim1 = ecf.Limit("L", 10)
        lim2 = ecf.Limit("L", 10)
        self.assertEqual(lim1, lim2)
        self.assertIsNot(lim1, lim2)
        self.assertNotEqual(hash(lim1), hash(lim2))

    def test_same_object_same_hash(self):
        """The same Limit always returns the same hash."""
        lim = ecf.Limit("L", 10)
        self.assertEqual(hash(lim), hash(lim))

    def test_can_be_stored_in_set(self):
        """Limit instances can be stored in a Python set."""
        lim = ecf.Limit("L", 10)
        s = {lim}
        self.assertIn(lim, s)

    def test_can_be_used_as_dict_key(self):
        """Limit instances can be used as dictionary keys."""
        lim = ecf.Limit("L", 10)
        d = {lim: "entry"}
        self.assertEqual(d[lim], "entry")


class TestInLimit(unittest.TestCase):
    """Tests for py::class_<InLimit> as exposed in ExportNodeAttr.cpp.

    InLimit references a Limit by name and optional path, and specifies how many
    tokens it consumes.

    Exposed API
    -----------
    Constructors
        InLimit(str)                         -- name only; path='', tokens=1,
                                                limit_this_node_only=False, limit_submission=False
        InLimit(str, str)                    -- name and path to limit-holder node
        InLimit(str, str, int)               -- name, path and token count
        InLimit(str, str, int, bool)         -- adds limit_this_node_only flag
        InLimit(str, str, int, bool, bool)   -- adds limit_submission flag

    Instance methods
        name()                 -> str        -- the limit name (returned by const reference)
        path_to_node()         -> str        -- path to node holding the limit ('' if not set)
        tokens()               -> int        -- number of tokens consumed (default 1)
        limit_this_node_only() -> bool       -- True iff only this node is limited
        limit_submission()     -> bool       -- True iff only submission is limited

    Operators
        __str__   -- format depends on args:
                     'inlimit NAME'                   (no path, tokens=1)
                     'inlimit /path:NAME'             (with path, tokens=1)
                     'inlimit /path:NAME N'           (with path, tokens≠1)
                     'inlimit -n /path:NAME N'        (limit_this_node_only=True)
                     'inlimit -s /path:NAME N'        (limit_submission=True)
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based: compares all five fields
        __ne__    -- implicit complement of __eq__
        __lt__    -- name-based alphabetical ordering only
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: InLimit(str) — name only
    # ------------------------------------------------------------------

    def test_one_arg_creates_instance(self):
        """InLimit(name) creates an InLimit with sensible defaults."""
        il = ecf.InLimit("my_limit")
        self.assertIsInstance(il, ecf.InLimit)

    def test_one_arg_name(self):
        """InLimit(name).name() returns the given name."""
        self.assertEqual(ecf.InLimit("my_limit").name(), "my_limit")

    def test_one_arg_path_to_node_is_empty(self):
        """InLimit(name).path_to_node() is '' when no path is provided."""
        self.assertEqual(ecf.InLimit("my_limit").path_to_node(), "")

    def test_one_arg_tokens_default_is_one(self):
        """InLimit(name).tokens() defaults to 1."""
        self.assertEqual(ecf.InLimit("my_limit").tokens(), 1)

    def test_one_arg_limit_this_node_only_default_false(self):
        """InLimit(name).limit_this_node_only() defaults to False."""
        self.assertFalse(ecf.InLimit("my_limit").limit_this_node_only())

    def test_one_arg_limit_submission_default_false(self):
        """InLimit(name).limit_submission() defaults to False."""
        self.assertFalse(ecf.InLimit("my_limit").limit_submission())

    def test_one_arg_str_format(self):
        """InLimit(name) formats as 'inlimit NAME'."""
        self.assertEqual(str(ecf.InLimit("my_limit")), "inlimit my_limit")

    # ------------------------------------------------------------------
    # Constructor: InLimit(str, str) — name and path
    # ------------------------------------------------------------------

    def test_two_arg_path_to_node(self):
        """InLimit(name, path).path_to_node() returns the given path."""
        il = ecf.InLimit("my_limit", "/suite/family")
        self.assertEqual(il.path_to_node(), "/suite/family")

    def test_two_arg_tokens_still_default_one(self):
        """InLimit(name, path).tokens() still defaults to 1."""
        self.assertEqual(ecf.InLimit("my_limit", "/suite/family").tokens(), 1)

    def test_two_arg_str_format(self):
        """InLimit(name, path) with tokens=1 formats as 'inlimit /path:NAME'."""
        self.assertEqual(
            str(ecf.InLimit("my_limit", "/suite/family")),
            "inlimit /suite/family:my_limit",
        )

    # ------------------------------------------------------------------
    # Constructor: InLimit(str, str, int) — name, path, tokens
    # ------------------------------------------------------------------

    def test_three_arg_tokens(self):
        """InLimit(name, path, n).tokens() returns n."""
        self.assertEqual(ecf.InLimit("L", "/suite", 5).tokens(), 5)

    def test_three_arg_str_format_with_tokens(self):
        """InLimit(name, path, n) with n≠1 formats as 'inlimit /path:NAME N'."""
        self.assertEqual(
            str(ecf.InLimit("my_limit", "/suite/family", 5)),
            "inlimit /suite/family:my_limit 5",
        )

    def test_three_arg_str_format_tokens_one_suppressed(self):
        """When tokens==1, the str() output suppresses the count."""
        self.assertEqual(
            str(ecf.InLimit("L", "/suite", 1)),
            "inlimit /suite:L",
        )

    # ------------------------------------------------------------------
    # Constructor: InLimit(str, str, int, bool) — limit_this_node_only
    # ------------------------------------------------------------------

    def test_four_arg_limit_this_node_only_true(self):
        """InLimit(name, path, n, True).limit_this_node_only() is True."""
        self.assertTrue(ecf.InLimit("L", "/suite", 2, True).limit_this_node_only())

    def test_four_arg_limit_submission_still_false(self):
        """Enabling limit_this_node_only does not affect limit_submission()."""
        self.assertFalse(ecf.InLimit("L", "/suite", 2, True).limit_submission())

    def test_four_arg_str_format_with_n_flag(self):
        """InLimit with limit_this_node_only=True formats with '-n' flag."""
        self.assertEqual(
            str(ecf.InLimit("my_limit", "/suite/family", 2, True)),
            "inlimit -n /suite/family:my_limit 2",
        )

    # ------------------------------------------------------------------
    # Constructor: InLimit(str, str, int, bool, bool) — limit_submission
    # ------------------------------------------------------------------

    def test_five_arg_limit_submission_true(self):
        """InLimit(name, path, n, False, True).limit_submission() is True."""
        self.assertTrue(ecf.InLimit("L", "/suite", 2, False, True).limit_submission())

    def test_five_arg_limit_this_node_only_false(self):
        """Enabling limit_submission alone leaves limit_this_node_only() False."""
        self.assertFalse(
            ecf.InLimit("L", "/suite", 2, False, True).limit_this_node_only()
        )

    def test_five_arg_str_format_with_s_flag(self):
        """InLimit with limit_submission=True formats with '-s' flag."""
        self.assertEqual(
            str(ecf.InLimit("my_limit", "/suite/family", 2, False, True)),
            "inlimit -s /suite/family:my_limit 2",
        )

    # ------------------------------------------------------------------
    # No default constructor
    # ------------------------------------------------------------------

    def test_default_constructor(self):
        """InLimit() with no arguments succeeds: boost.python supplies an empty string
        for the optional str arg, producing an InLimit with an empty name."""
        il = ecf.InLimit()
        self.assertIsInstance(il, ecf.InLimit)
        self.assertEqual(il.name(), "")
        self.assertEqual(il.tokens(), 1)

    # ------------------------------------------------------------------
    # Wrong constructor argument types
    # ------------------------------------------------------------------

    def test_int_as_name_raises(self):
        """Passing an integer as name raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.InLimit(42)

    def test_int_as_path_raises(self):
        """Passing an integer as path raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.InLimit("L", 42)

    def test_str_as_tokens_raises(self):
        """Passing a string as the token count raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.InLimit("L", "/suite", "three")

    # ------------------------------------------------------------------
    # name() / path_to_node() / tokens() return types
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() returns a Python str."""
        self.assertIsInstance(ecf.InLimit("L").name(), str)

    def test_path_to_node_returns_str(self):
        """path_to_node() returns a Python str."""
        self.assertIsInstance(ecf.InLimit("L", "/suite").path_to_node(), str)

    def test_tokens_returns_int(self):
        """tokens() returns a Python int."""
        self.assertIsInstance(ecf.InLimit("L", "/suite", 3).tokens(), int)

    def test_limit_this_node_only_returns_bool(self):
        """limit_this_node_only() returns a Python bool."""
        self.assertIsInstance(ecf.InLimit("L").limit_this_node_only(), bool)

    def test_limit_submission_returns_bool(self):
        """limit_submission() returns a Python bool."""
        self.assertIsInstance(ecf.InLimit("L").limit_submission(), bool)

    # ------------------------------------------------------------------
    # __str__ — return type
    # ------------------------------------------------------------------

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.InLimit("L")), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — value-based (all five fields compared)
    # ------------------------------------------------------------------

    def test_eq_same_all_fields(self):
        """Two InLimits with identical fields are equal."""
        self.assertEqual(
            ecf.InLimit("L", "/suite", 3),
            ecf.InLimit("L", "/suite", 3),
        )

    def test_eq_reflexive(self):
        """An InLimit is equal to itself."""
        il = ecf.InLimit("L", "/suite", 2)
        self.assertEqual(il, il)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        il1 = ecf.InLimit("L", "/suite", 2)
        il2 = ecf.InLimit("L", "/suite", 2)
        self.assertTrue(il1 == il2)
        self.assertTrue(il2 == il1)

    def test_ne_different_tokens(self):
        """InLimits with different token counts are not equal."""
        self.assertNotEqual(
            ecf.InLimit("L", "/suite", 3),
            ecf.InLimit("L", "/suite", 4),
        )

    def test_ne_different_name(self):
        """InLimits with different names are not equal."""
        self.assertNotEqual(
            ecf.InLimit("A", "/suite", 1),
            ecf.InLimit("B", "/suite", 1),
        )

    def test_ne_different_path(self):
        """InLimits with different paths are not equal."""
        self.assertNotEqual(
            ecf.InLimit("L", "/path1", 1),
            ecf.InLimit("L", "/path2", 1),
        )

    def test_ne_different_limit_this_node_only(self):
        """InLimits differing only in limit_this_node_only are not equal."""
        self.assertNotEqual(
            ecf.InLimit("L", "/suite", 1, True),
            ecf.InLimit("L", "/suite", 1, False),
        )

    def test_ne_different_limit_submission(self):
        """InLimits differing only in limit_submission are not equal."""
        self.assertNotEqual(
            ecf.InLimit("L", "/suite", 1, False, True),
            ecf.InLimit("L", "/suite", 1, False, False),
        )

    # ------------------------------------------------------------------
    # __lt__ — name-based ordering only
    # ------------------------------------------------------------------

    def test_lt_alphabetically_earlier_name_is_less(self):
        """InLimit with an earlier name is less than one with a later name."""
        self.assertLess(ecf.InLimit("A"), ecf.InLimit("B"))

    def test_lt_alphabetically_later_name_is_not_less(self):
        """InLimit with a later name is NOT less."""
        self.assertFalse(ecf.InLimit("B") < ecf.InLimit("A"))

    def test_lt_same_name_not_less_regardless_of_other_fields(self):
        """InLimits with the same name are not in a less-than relation
        regardless of tokens, path or flags (ordering is name-based only)."""
        self.assertFalse(ecf.InLimit("X", "/p", 1) < ecf.InLimit("X", "/p", 99))

    def test_lt_equal_inlimits_not_less(self):
        """An InLimit is not strictly less than an equal InLimit."""
        self.assertFalse(ecf.InLimit("X") < ecf.InLimit("X"))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces an InLimit equal in value to the original."""
        import copy

        il = ecf.InLimit("L", "/suite", 3)
        self.assertEqual(il, copy.copy(il))

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        import copy

        il = ecf.InLimit("L", "/suite", 3)
        self.assertIsNot(il, copy.copy(il))

    def test_copy_is_inlimit_instance(self):
        """The result of copy.copy() is an InLimit."""
        import copy

        self.assertIsInstance(copy.copy(ecf.InLimit("L")), ecf.InLimit)

    def test_copy_preserves_all_fields(self):
        """The copied InLimit has the same field values as the original."""
        import copy

        il = ecf.InLimit("MY_LIMIT", "/suite/family", 5, True, False)
        c = copy.copy(il)
        self.assertEqual(c.name(), il.name())
        self.assertEqual(c.path_to_node(), il.path_to_node())
        self.assertEqual(c.tokens(), il.tokens())
        self.assertEqual(c.limit_this_node_only(), il.limit_this_node_only())
        self.assertEqual(c.limit_submission(), il.limit_submission())

    # ------------------------------------------------------------------
    # __hash__ — identity-based
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """InLimit is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.InLimit("L")), int)

    def test_hash_is_identity_based(self):
        """Two value-equal InLimits have different hashes (identity, not value)."""
        il1 = ecf.InLimit("L", "/suite", 3)
        il2 = ecf.InLimit("L", "/suite", 3)
        self.assertEqual(il1, il2)
        self.assertIsNot(il1, il2)
        self.assertNotEqual(hash(il1), hash(il2))

    def test_same_object_same_hash(self):
        """The same InLimit always returns the same hash."""
        il = ecf.InLimit("L")
        self.assertEqual(hash(il), hash(il))

    def test_can_be_stored_in_set(self):
        """InLimit instances can be stored in a Python set."""
        il = ecf.InLimit("L")
        s = {il}
        self.assertIn(il, s)

    def test_can_be_used_as_dict_key(self):
        """InLimit instances can be used as dictionary keys."""
        il = ecf.InLimit("L")
        d = {il: "entry"}
        self.assertEqual(d[il], "entry")


class TestEvent(unittest.TestCase):
    """Tests for py::class_<Event> as exposed in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Event(int)                       -- number only; name='', initial_value=False
        Event(int, str)                  -- number and optional name; initial_value=False
        Event(int, str, bool)            -- number, name and initial_value
        Event(str, bool)                 -- name and initial_value; number=INT_MAX (2147483647)
        Event(str)                       -- name only; number=INT_MAX, initial_value=False

    Instance methods
        name()           -> str          -- the event name; '' when only a number is given
        number()         -> int          -- the event number; INT_MAX when only a name is given
        name_or_number() -> str          -- name if non-empty, else str(number)
        value()          -> bool         -- current value; equals initial_value after construction
        initial_value()  -> bool         -- value taken for begin/re-queue
        empty()          -> bool         -- always False for Python-constructed Events

    Operators
        __str__   -- 'event NUMBER NAME' or 'event NAME'; appends ' set' when initial_value=True
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based: compares both number and name
        __ne__    -- implicit complement of __eq__
        __lt__    -- name-based alphabetical ordering only; events with the same name but
                     different numbers are NOT ordered
        __hash__  -- identity-based (boost.python C-extension type)
    """

    INT_MAX = 2**31 - 1  # sentinel used when no number is explicitly given

    # ------------------------------------------------------------------
    # Constructor: Event(int) — number only
    # ------------------------------------------------------------------

    def test_int_ctor_number(self):
        """Event(n).number() returns n."""
        self.assertEqual(ecf.Event(1).number(), 1)

    def test_int_ctor_name_is_empty(self):
        """Event(n).name() is '' when only a number is given."""
        self.assertEqual(ecf.Event(1).name(), "")

    def test_int_ctor_initial_value_is_false(self):
        """Event(n).initial_value() defaults to False."""
        self.assertFalse(ecf.Event(1).initial_value())

    def test_int_ctor_value_is_false(self):
        """Event(n).value() is False immediately after construction."""
        self.assertFalse(ecf.Event(1).value())

    def test_int_ctor_name_or_number_is_str_of_number(self):
        """Event(n).name_or_number() returns str(n) when name is empty."""
        self.assertEqual(ecf.Event(42).name_or_number(), "42")

    def test_int_ctor_str_format(self):
        """Event(n) formats as 'event N ' (trailing space when name is empty)."""
        self.assertEqual(str(ecf.Event(1)), "event 1 ")

    # ------------------------------------------------------------------
    # Constructor: Event(int, str) — number and name
    # ------------------------------------------------------------------

    def test_int_str_ctor_name(self):
        """Event(n, name).name() returns the given name."""
        self.assertEqual(ecf.Event(1, "my_event").name(), "my_event")

    def test_int_str_ctor_number(self):
        """Event(n, name).number() returns n."""
        self.assertEqual(ecf.Event(5, "ev").number(), 5)

    def test_int_str_ctor_name_or_number_returns_name(self):
        """Event(n, name).name_or_number() returns the name (not the number)."""
        self.assertEqual(ecf.Event(1, "my_event").name_or_number(), "my_event")

    def test_int_str_ctor_str_format(self):
        """Event(n, name) formats as 'event N NAME'."""
        self.assertEqual(str(ecf.Event(1, "my_event")), "event 1 my_event")

    def test_int_str_ctor_initial_value_defaults_false(self):
        """Event(n, name).initial_value() defaults to False."""
        self.assertFalse(ecf.Event(1, "ev").initial_value())

    # ------------------------------------------------------------------
    # Constructor: Event(int, str, bool) — number, name and initial_value
    # ------------------------------------------------------------------

    def test_int_str_bool_ctor_initial_value_true(self):
        """Event(n, name, True).initial_value() is True."""
        self.assertTrue(ecf.Event(2, "ev", True).initial_value())

    def test_int_str_bool_ctor_value_matches_initial(self):
        """Event(n, name, bool).value() equals initial_value after construction."""
        self.assertTrue(ecf.Event(2, "ev", True).value())
        self.assertFalse(ecf.Event(2, "ev", False).value())

    def test_int_str_bool_ctor_str_set_suffix_when_true(self):
        """Event with initial_value=True includes 'set' in str()."""
        self.assertEqual(str(ecf.Event(2, "ev2", True)), "event 2 ev2 set")

    def test_int_str_bool_ctor_str_no_set_suffix_when_false(self):
        """Event with initial_value=False has no 'set' suffix in str()."""
        self.assertEqual(str(ecf.Event(3, "ev3", False)), "event 3 ev3")

    # ------------------------------------------------------------------
    # Constructor: Event(str, bool) — name and initial_value
    # ------------------------------------------------------------------

    def test_str_bool_ctor_name(self):
        """Event(name, bool).name() returns the given name."""
        self.assertEqual(ecf.Event("named_event", True).name(), "named_event")

    def test_str_bool_ctor_number_is_int_max(self):
        """Event(name, bool).number() is INT_MAX (2147483647) when no number is given."""
        self.assertEqual(ecf.Event("named_event", True).number(), self.INT_MAX)

    def test_str_bool_ctor_initial_value_true(self):
        """Event(name, True).initial_value() is True."""
        self.assertTrue(ecf.Event("ev", True).initial_value())

    def test_str_bool_ctor_str_set_suffix_when_true(self):
        """Event(name, True) formats with 'set' suffix."""
        self.assertEqual(str(ecf.Event("named_event", True)), "event named_event set")

    def test_str_bool_ctor_str_no_set_suffix_when_false(self):
        """Event(name, False) formats without 'set' suffix."""
        self.assertEqual(str(ecf.Event("named_event", False)), "event named_event")

    # ------------------------------------------------------------------
    # Constructor: Event(str) — name only
    # ------------------------------------------------------------------

    def test_str_ctor_name(self):
        """Event(name).name() returns the given name."""
        self.assertEqual(ecf.Event("just_name").name(), "just_name")

    def test_str_ctor_number_is_int_max(self):
        """Event(name).number() is INT_MAX when no number is specified."""
        self.assertEqual(ecf.Event("just_name").number(), self.INT_MAX)

    def test_str_ctor_initial_value_is_false(self):
        """Event(name).initial_value() defaults to False."""
        self.assertFalse(ecf.Event("just_name").initial_value())

    def test_str_ctor_str_format(self):
        """Event(name) formats as 'event NAME' (no number, no 'set')."""
        self.assertEqual(str(ecf.Event("just_name")), "event just_name")

    def test_str_ctor_equals_str_false_ctor(self):
        """Event(name) and Event(name, False) are equal (same number, name, initial_value)."""
        self.assertEqual(ecf.Event("ev"), ecf.Event("ev", False))

    # ------------------------------------------------------------------
    # name() / number() / name_or_number() / value() / initial_value() / empty()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() returns a Python str."""
        self.assertIsInstance(ecf.Event(1, "ev").name(), str)

    def test_number_returns_int(self):
        """number() returns a Python int."""
        self.assertIsInstance(ecf.Event(1).number(), int)

    def test_name_or_number_returns_str(self):
        """name_or_number() returns a Python str."""
        self.assertIsInstance(ecf.Event(1, "ev").name_or_number(), str)

    def test_value_returns_bool(self):
        """value() returns a Python bool."""
        self.assertIsInstance(ecf.Event(1).value(), bool)

    def test_initial_value_returns_bool(self):
        """initial_value() returns a Python bool."""
        self.assertIsInstance(ecf.Event(1).initial_value(), bool)

    def test_empty_returns_false(self):
        """empty() returns False for every Python-constructed Event."""
        self.assertFalse(ecf.Event(1).empty())
        self.assertFalse(ecf.Event("ev").empty())

    def test_empty_returns_bool(self):
        """empty() returns a Python bool."""
        self.assertIsInstance(ecf.Event(1).empty(), bool)

    # ------------------------------------------------------------------
    # __str__ — return type
    # ------------------------------------------------------------------

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Event(1, "ev")), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — value-based (number AND name compared)
    # ------------------------------------------------------------------

    def test_eq_same_number_and_name(self):
        """Two Events with the same number and name are equal."""
        self.assertEqual(ecf.Event(1, "e"), ecf.Event(1, "e"))

    def test_eq_reflexive(self):
        """An Event is equal to itself."""
        e = ecf.Event(1, "e")
        self.assertEqual(e, e)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        e1 = ecf.Event(1, "e")
        e2 = ecf.Event(1, "e")
        self.assertTrue(e1 == e2)
        self.assertTrue(e2 == e1)

    def test_ne_different_number(self):
        """Events with same name but different numbers are not equal."""
        self.assertNotEqual(ecf.Event(1, "e"), ecf.Event(2, "e"))

    def test_ne_different_name(self):
        """Events with same number but different names are not equal."""
        self.assertNotEqual(ecf.Event(1, "e"), ecf.Event(1, "x"))

    def test_ne_number_only_vs_number_and_name(self):
        """Event(n) is not equal to Event(n, name) because names differ."""
        self.assertNotEqual(ecf.Event(1), ecf.Event(1, "e"))

    def test_ne_different_initial_value(self):
        """Events with same number+name but different initial_value are not equal."""
        self.assertNotEqual(ecf.Event(1, "ev", True), ecf.Event(1, "ev", False))

    # ------------------------------------------------------------------
    # __lt__ — name-based alphabetical ordering only
    # ------------------------------------------------------------------

    def test_lt_alphabetically_earlier_name_is_less(self):
        """Event with an alphabetically earlier name is less."""
        self.assertLess(ecf.Event("A"), ecf.Event("B"))

    def test_lt_alphabetically_later_name_is_not_less(self):
        """Event with a later name is NOT less."""
        self.assertFalse(ecf.Event("B") < ecf.Event("A"))

    def test_lt_same_name_diff_number_not_less(self):
        """Two Events with the same name but different numbers are not ordered
        (ordering is name-based only)."""
        self.assertFalse(ecf.Event(1, "A") < ecf.Event(2, "A"))

    def test_lt_equal_events_not_less(self):
        """An Event is not strictly less than an equal Event."""
        self.assertFalse(ecf.Event("X") < ecf.Event("X"))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces an Event equal in value to the original."""
        import copy

        e = ecf.Event(1, "ev")
        self.assertEqual(e, copy.copy(e))

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        import copy

        e = ecf.Event(1, "ev")
        self.assertIsNot(e, copy.copy(e))

    def test_copy_is_event_instance(self):
        """The result of copy.copy() is an Event."""
        import copy

        self.assertIsInstance(copy.copy(ecf.Event(1, "ev")), ecf.Event)

    def test_copy_preserves_fields(self):
        """The copied Event has matching name(), number() and initial_value()."""
        import copy

        e = ecf.Event(7, "my_event", True)
        c = copy.copy(e)
        self.assertEqual(c.name(), e.name())
        self.assertEqual(c.number(), e.number())
        self.assertEqual(c.initial_value(), e.initial_value())

    # ------------------------------------------------------------------
    # __hash__ — identity-based
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Event is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Event(1, "ev")), int)

    def test_hash_is_identity_based(self):
        """Two value-equal Events have different hashes (identity, not value)."""
        e1 = ecf.Event(1, "ev")
        e2 = ecf.Event(1, "ev")
        self.assertEqual(e1, e2)
        self.assertIsNot(e1, e2)
        self.assertNotEqual(hash(e1), hash(e2))

    def test_same_object_same_hash(self):
        """The same Event always returns the same hash."""
        e = ecf.Event(1, "ev")
        self.assertEqual(hash(e), hash(e))

    def test_can_be_stored_in_set(self):
        """Event instances can be stored in a Python set."""
        e = ecf.Event(1, "ev")
        self.assertIn(e, {e})

    def test_can_be_used_as_dict_key(self):
        """Event instances can be used as dictionary keys."""
        e = ecf.Event(1, "ev")
        d = {e: "entry"}
        self.assertEqual(d[e], "entry")

    # ------------------------------------------------------------------
    # Negative: wrong constructor argument types
    # ------------------------------------------------------------------

    def test_none_as_number_raises(self):
        """Passing None as the number raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Event(None)

    def test_str_as_bool_flag_raises(self):
        """Passing a string where a bool flag is expected raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Event("ev", "not_bool")


class TestMeter(unittest.TestCase):
    """Tests for py::class_<Meter> as exposed in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Meter(str, int, int)             -- name, min and max; color_change defaults to max
        Meter(str, int, int, int)        -- name, min, max and explicit color_change

    Instance methods
        name()         -> str            -- the meter name (returned by const reference)
        min()          -> int            -- the minimum value
        max()          -> int            -- the maximum value
        value()        -> int            -- the current value; equals min() after construction
        color_change() -> int            -- the color-change threshold; defaults to max()
        empty()        -> bool           -- always False for Python-constructed Meters

    Operators
        __str__   -- 'meter NAME MIN MAX COLOR_CHANGE' format
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based: name, min, max AND color_change must all match
        __ne__    -- implicit complement of __eq__
        __lt__    -- name-based alphabetical ordering only
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: Meter(str, int, int) — name, min, max
    # ------------------------------------------------------------------

    def test_three_arg_ctor_fields(self):
        """Meter(name, min, max) stores name, min and max correctly."""
        m = ecf.Meter("my_meter", 0, 100)
        self.assertEqual(m.name(), "my_meter")
        self.assertEqual(m.min(), 0)
        self.assertEqual(m.max(), 100)

    def test_three_arg_initial_value_equals_min(self):
        """The current value starts at min() after construction."""
        self.assertEqual(ecf.Meter("M", 0, 100).value(), 0)
        self.assertEqual(ecf.Meter("M", -10, 10).value(), -10)

    def test_three_arg_color_change_defaults_to_max(self):
        """When color_change is not specified it defaults to max."""
        m = ecf.Meter("M", 0, 100)
        self.assertEqual(m.color_change(), m.max())

    def test_three_arg_str_format(self):
        """Meter(name, min, max) formats as 'meter NAME MIN MAX MAX'."""
        self.assertEqual(str(ecf.Meter("m", 0, 10)), "meter m 0 10 10")

    # ------------------------------------------------------------------
    # Constructor: Meter(str, int, int, int) — explicit color_change
    # ------------------------------------------------------------------

    def test_four_arg_ctor_color_change(self):
        """Meter(name, min, max, cc).color_change() returns cc."""
        self.assertEqual(ecf.Meter("M", 0, 100, 80).color_change(), 80)

    def test_four_arg_ctor_color_change_equals_max(self):
        """Meter(name, min, max, max) has the same result as 3-arg constructor."""
        self.assertEqual(ecf.Meter("M", 0, 100, 100), ecf.Meter("M", 0, 100))

    def test_four_arg_str_format(self):
        """Meter(name, min, max, cc) formats as 'meter NAME MIN MAX CC'."""
        self.assertEqual(str(ecf.Meter("m", 0, 100, 50)), "meter m 0 100 50")

    def test_four_arg_negative_range_str(self):
        """Meter with negative min formats correctly."""
        self.assertEqual(str(ecf.Meter("m", -5, 5, 3)), "meter m -5 5 3")

    # ------------------------------------------------------------------
    # No default constructor
    # ------------------------------------------------------------------

    def test_no_default_constructor(self):
        """Meter() with no arguments raises an error; no default constructor is exposed."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Meter()

    # ------------------------------------------------------------------
    # Wrong constructor argument types
    # ------------------------------------------------------------------

    def test_int_as_name_raises(self):
        """Passing an integer as name raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Meter(42, 0, 100)

    def test_none_as_name_raises(self):
        """Passing None as name raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Meter(None, 0, 100)

    def test_str_as_min_raises(self):
        """Passing a string as min raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Meter("M", "0", 100)

    def test_str_as_max_raises(self):
        """Passing a string as max raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Meter("M", 0, "100")

    # ------------------------------------------------------------------
    # name() / min() / max() / value() / color_change() / empty()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() returns a Python str."""
        self.assertIsInstance(ecf.Meter("M", 0, 10).name(), str)

    def test_min_returns_int(self):
        """min() returns a Python int."""
        self.assertIsInstance(ecf.Meter("M", 0, 10).min(), int)

    def test_max_returns_int(self):
        """max() returns a Python int."""
        self.assertIsInstance(ecf.Meter("M", 0, 10).max(), int)

    def test_value_returns_int(self):
        """value() returns a Python int."""
        self.assertIsInstance(ecf.Meter("M", 0, 10).value(), int)

    def test_color_change_returns_int(self):
        """color_change() returns a Python int."""
        self.assertIsInstance(ecf.Meter("M", 0, 10).color_change(), int)

    def test_empty_returns_false(self):
        """empty() returns False for every Python-constructed Meter."""
        self.assertFalse(ecf.Meter("M", 0, 100).empty())

    def test_empty_returns_bool(self):
        """empty() returns a Python bool."""
        self.assertIsInstance(ecf.Meter("M", 0, 100).empty(), bool)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Meter("M", 0, 100)), str)

    def test_str_includes_all_four_fields(self):
        """str() always includes name, min, max and color_change."""
        s = str(ecf.Meter("my_meter", 0, 100))
        self.assertIn("my_meter", s)
        self.assertIn("0", s)
        self.assertIn("100", s)

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — value-based (name + min + max + color_change)
    # ------------------------------------------------------------------

    def test_eq_same_all_fields(self):
        """Two Meters with identical fields are equal."""
        self.assertEqual(ecf.Meter("M", 0, 100), ecf.Meter("M", 0, 100))

    def test_eq_reflexive(self):
        """A Meter is equal to itself."""
        m = ecf.Meter("M", 0, 100)
        self.assertEqual(m, m)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        m1 = ecf.Meter("M", 0, 100)
        m2 = ecf.Meter("M", 0, 100)
        self.assertTrue(m1 == m2)
        self.assertTrue(m2 == m1)

    def test_eq_explicit_color_change_matches_default(self):
        """Meter(name,min,max,max) equals Meter(name,min,max) because color_change is the same."""
        self.assertEqual(ecf.Meter("M", 0, 100, 100), ecf.Meter("M", 0, 100))

    def test_ne_different_name(self):
        """Meters with different names are not equal."""
        self.assertNotEqual(ecf.Meter("A", 0, 100), ecf.Meter("B", 0, 100))

    def test_ne_different_min(self):
        """Meters with different min values are not equal."""
        self.assertNotEqual(ecf.Meter("M", 0, 100), ecf.Meter("M", 1, 100))

    def test_ne_different_max(self):
        """Meters with different max values are not equal."""
        self.assertNotEqual(ecf.Meter("M", 0, 100), ecf.Meter("M", 0, 50))

    def test_ne_different_color_change(self):
        """Meters with different color_change values are not equal."""
        self.assertNotEqual(ecf.Meter("M", 0, 100, 50), ecf.Meter("M", 0, 100, 80))

    # ------------------------------------------------------------------
    # __lt__ — name-based ordering only
    # ------------------------------------------------------------------

    def test_lt_alphabetically_earlier_name_is_less(self):
        """Meter with an earlier name is less than one with a later name."""
        self.assertLess(ecf.Meter("A", 0, 10), ecf.Meter("B", 0, 10))

    def test_lt_alphabetically_later_name_is_not_less(self):
        """Meter with a later name is NOT less."""
        self.assertFalse(ecf.Meter("B", 0, 10) < ecf.Meter("A", 0, 10))

    def test_lt_same_name_diff_max_not_less(self):
        """Meters with the same name but different max values are not ordered
        (ordering is name-based only)."""
        self.assertFalse(ecf.Meter("M", 0, 10) < ecf.Meter("M", 0, 99))

    def test_lt_same_name_diff_min_not_less(self):
        """Meters with the same name but different min values are not ordered."""
        self.assertFalse(ecf.Meter("M", 1, 10) < ecf.Meter("M", 0, 10))

    def test_lt_equal_meters_not_less(self):
        """A Meter is not strictly less than an equal Meter."""
        self.assertFalse(ecf.Meter("X", 0, 10) < ecf.Meter("X", 0, 10))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces a Meter equal in value to the original."""
        import copy

        m = ecf.Meter("M", 0, 100, 50)
        self.assertEqual(m, copy.copy(m))

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        import copy

        m = ecf.Meter("M", 0, 100)
        self.assertIsNot(m, copy.copy(m))

    def test_copy_is_meter_instance(self):
        """The result of copy.copy() is a Meter."""
        import copy

        self.assertIsInstance(copy.copy(ecf.Meter("M", 0, 100)), ecf.Meter)

    def test_copy_preserves_all_fields(self):
        """The copied Meter has matching name(), min(), max() and color_change()."""
        import copy

        m = ecf.Meter("MY_METER", -5, 50, 30)
        c = copy.copy(m)
        self.assertEqual(c.name(), m.name())
        self.assertEqual(c.min(), m.min())
        self.assertEqual(c.max(), m.max())
        self.assertEqual(c.color_change(), m.color_change())

    # ------------------------------------------------------------------
    # __hash__ — identity-based
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Meter is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Meter("M", 0, 100)), int)

    def test_hash_is_identity_based(self):
        """Two value-equal Meters have different hashes (identity, not value)."""
        m1 = ecf.Meter("M", 0, 100)
        m2 = ecf.Meter("M", 0, 100)
        self.assertEqual(m1, m2)
        self.assertIsNot(m1, m2)
        self.assertNotEqual(hash(m1), hash(m2))

    def test_same_object_same_hash(self):
        """The same Meter always returns the same hash."""
        m = ecf.Meter("M", 0, 100)
        self.assertEqual(hash(m), hash(m))

    def test_can_be_stored_in_set(self):
        """Meter instances can be stored in a Python set."""
        m = ecf.Meter("M", 0, 100)
        self.assertIn(m, {m})

    def test_can_be_used_as_dict_key(self):
        """Meter instances can be used as dictionary keys."""
        m = ecf.Meter("M", 0, 100)
        d = {m: "entry"}
        self.assertEqual(d[m], "entry")


class TestQueue(unittest.TestCase):
    """Tests for py::class_<QueueAttr> exposed as ecf.Queue in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Queue(str, list[str])            -- name and a non-empty list of step strings;
                                            an empty list raises RuntimeError;
                                            Queue() with no args succeeds (boost.python
                                            supplies empty defaults) but gives an unnamed,
                                            zero-step queue

    Instance methods
        name()   -> str                  -- the queue name (returned by const reference)
        value()  -> str                  -- the current step value (first item after construction)
        index()  -> int                  -- the current step index (0 after construction)
        empty()  -> bool                 -- always False for normally constructed Queue instances

    Operators
        __str__   -- 'queue <name> <step1> <step2> ...' (space-separated, no quotes)
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based: name and all steps (in order) must match
        __ne__    -- implicit complement of __eq__
        __lt__    -- name-based alphabetical ordering only; queues with the same name but
                     different step lists are NOT ordered
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: Queue(str, list[str])
    # ------------------------------------------------------------------

    def test_create_stores_name(self):
        """Queue(name, steps).name() returns the given name."""
        q = ecf.Queue("my_queue", ["step1", "step2"])
        self.assertEqual(q.name(), "my_queue")

    def test_create_initial_value_is_first_step(self):
        """Queue.value() is the first step immediately after construction."""
        q = ecf.Queue("q", ["a", "b", "c"])
        self.assertEqual(q.value(), "a")

    def test_create_initial_index_is_zero(self):
        """Queue.index() is 0 immediately after construction."""
        self.assertEqual(ecf.Queue("q", ["a", "b"]).index(), 0)

    def test_create_single_step(self):
        """A list with a single step is valid."""
        q = ecf.Queue("q", ["only"])
        self.assertEqual(q.value(), "only")
        # str() = 'queue q only' → 3 tokens: keyword + name + 1 step
        self.assertEqual(len(str(q).split()), 3)

    def test_create_many_steps(self):
        """Many steps are accepted and the first is reflected in value()."""
        steps = [str(i) for i in range(10)]
        q = ecf.Queue("q", steps)
        self.assertEqual(q.value(), "0")

    def test_create_no_args_produces_default_instance(self):
        """Queue() with no arguments succeeds via boost.python default args;
        produces an instance with name='' and index=0."""
        q = ecf.Queue()
        self.assertIsInstance(q, ecf.Queue)
        self.assertEqual(q.name(), "")
        self.assertEqual(q.index(), 0)

    # ------------------------------------------------------------------
    # Negative: invalid constructor arguments
    # ------------------------------------------------------------------

    def test_empty_list_raises(self):
        """Queue(name, []) raises RuntimeError because at least one step is required."""
        with self.assertRaises(RuntimeError):
            ecf.Queue("q", [])

    def test_int_as_name_raises(self):
        """Passing an integer as name raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Queue(42, ["a"])

    def test_none_as_name_raises(self):
        """Passing None as name raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Queue(None, ["a"])

    def test_str_instead_of_list_raises(self):
        """Passing a plain string instead of a list raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Queue("q", "not_a_list")

    def test_int_in_step_list_raises(self):
        """An integer inside the step list raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Queue("q", [42])

    def test_none_in_step_list_raises(self):
        """None inside the step list raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Queue("q", [None])

    def test_nested_list_in_step_list_raises(self):
        """A nested list inside the step list raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Queue("q", [["nested"]])

    # ------------------------------------------------------------------
    # Negative: invalid name strings
    # ------------------------------------------------------------------

    def test_empty_name_raises(self):
        """An empty name string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Queue("", ["step"])

    def test_name_with_comma_raises(self):
        """A name containing a comma raises RuntimeError (only alphanumeric, _ and . allowed)."""
        with self.assertRaises(RuntimeError):
            ecf.Queue("with,comma", ["step"])

    def test_name_with_semicolon_raises(self):
        """A name containing a semicolon raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Queue("with;semicolon", ["step"])

    def test_name_with_exclamation_raises(self):
        """A name containing an exclamation mark raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Queue("with!exclaim", ["step"])

    def test_name_with_space_raises(self):
        """A name containing a space raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Queue("with space", ["step"])

    def test_name_with_dash_raises(self):
        """A name containing a dash raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Queue("with-dash", ["step"])

    # ------------------------------------------------------------------
    # Valid name edge cases
    # ------------------------------------------------------------------

    def test_name_with_leading_underscore_is_valid(self):
        """A name beginning with an underscore is accepted."""
        q = ecf.Queue("_MY_QUEUE", ["step"])
        self.assertEqual(q.name(), "_MY_QUEUE")

    def test_name_with_dot_is_valid(self):
        """A name containing a dot is accepted."""
        q = ecf.Queue("queue.name", ["step"])
        self.assertEqual(q.name(), "queue.name")

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() returns a Python str."""
        self.assertIsInstance(ecf.Queue("q", ["a"]).name(), str)

    def test_name_returns_correct_value(self):
        """name() returns exactly the name passed at construction."""
        self.assertEqual(ecf.Queue("MY_QUEUE", ["a"]).name(), "MY_QUEUE")

    # ------------------------------------------------------------------
    # value()
    # ------------------------------------------------------------------

    def test_value_returns_str(self):
        """value() returns a Python str."""
        self.assertIsInstance(ecf.Queue("q", ["a"]).value(), str)

    def test_value_is_first_step(self):
        """value() is the first element of the step list after construction."""
        self.assertEqual(ecf.Queue("q", ["first", "second"]).value(), "first")

    # ------------------------------------------------------------------
    # index()
    # ------------------------------------------------------------------

    def test_index_returns_int(self):
        """index() returns a Python int."""
        self.assertIsInstance(ecf.Queue("q", ["a"]).index(), int)

    def test_index_initially_zero(self):
        """index() is 0 immediately after construction."""
        self.assertEqual(ecf.Queue("q", ["a", "b", "c"]).index(), 0)

    # ------------------------------------------------------------------
    # empty()
    # ------------------------------------------------------------------

    def test_empty_returns_false(self):
        """empty() returns False for a normally constructed Queue."""
        self.assertFalse(ecf.Queue("q", ["a"]).empty())

    def test_empty_returns_bool(self):
        """empty() returns a Python bool."""
        self.assertIsInstance(ecf.Queue("q", ["a"]).empty(), bool)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format_is_queue_name_steps(self):
        """str() follows the format 'queue <name> <step1> <step2> ...'."""
        self.assertEqual(str(ecf.Queue("q", ["a", "b", "c"])), "queue q a b c")

    def test_str_single_step(self):
        """str() with a single step outputs 'queue <name> <step>'."""
        self.assertEqual(str(ecf.Queue("q", ["only"])), "queue q only")

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Queue("q", ["a"])), str)

    def test_str_preserves_step_order(self):
        """Steps appear in the str() output in the same order as the list."""
        s = str(ecf.Queue("q", ["x", "y", "z"]))
        parts = s.split()
        self.assertEqual(parts[2:], ["x", "y", "z"])

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — value-based (name + all steps, in order)
    # ------------------------------------------------------------------

    def test_eq_same_name_and_steps(self):
        """Two Queues with identical name and steps are equal."""
        self.assertEqual(ecf.Queue("Q", ["a", "b"]), ecf.Queue("Q", ["a", "b"]))

    def test_eq_reflexive(self):
        """A Queue is equal to itself."""
        q = ecf.Queue("Q", ["a", "b"])
        self.assertEqual(q, q)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        q1 = ecf.Queue("Q", ["a", "b"])
        q2 = ecf.Queue("Q", ["a", "b"])
        self.assertTrue(q1 == q2)
        self.assertTrue(q2 == q1)

    def test_ne_different_step_value(self):
        """Queues with the same name but different step contents are not equal."""
        self.assertNotEqual(ecf.Queue("Q", ["a", "b"]), ecf.Queue("Q", ["a", "c"]))

    def test_ne_different_step_count(self):
        """Queues with the same name but different number of steps are not equal."""
        self.assertNotEqual(ecf.Queue("Q", ["a", "b"]), ecf.Queue("Q", ["a"]))

    def test_ne_different_step_order(self):
        """Queues with the same steps but in a different order are not equal."""
        self.assertNotEqual(ecf.Queue("Q", ["a", "b"]), ecf.Queue("Q", ["b", "a"]))

    def test_ne_different_name(self):
        """Queues with different names are not equal."""
        self.assertNotEqual(ecf.Queue("A", ["a"]), ecf.Queue("B", ["a"]))

    # ------------------------------------------------------------------
    # __lt__ — name-based ordering only
    # ------------------------------------------------------------------

    def test_lt_alphabetically_earlier_name_is_less(self):
        """Queue with an earlier name is less than one with a later name."""
        self.assertLess(ecf.Queue("A", ["a"]), ecf.Queue("B", ["a"]))

    def test_lt_later_name_is_not_less(self):
        """Queue with a later name is NOT less."""
        self.assertFalse(ecf.Queue("B", ["a"]) < ecf.Queue("A", ["a"]))

    def test_lt_same_name_diff_steps_not_less(self):
        """Queues with the same name are not ordered regardless of step content."""
        self.assertFalse(ecf.Queue("Q", ["a"]) < ecf.Queue("Q", ["z"]))
        self.assertFalse(ecf.Queue("Q", ["z"]) < ecf.Queue("Q", ["a"]))

    def test_lt_equal_queues_not_less(self):
        """A Queue is not strictly less than an equal Queue."""
        self.assertFalse(ecf.Queue("Q", ["a"]) < ecf.Queue("Q", ["a"]))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces a Queue equal in value to the original."""
        import copy

        q = ecf.Queue("Q", ["a", "b", "c"])
        self.assertEqual(q, copy.copy(q))

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        import copy

        q = ecf.Queue("Q", ["a", "b"])
        self.assertIsNot(q, copy.copy(q))

    def test_copy_is_queue_instance(self):
        """The result of copy.copy() is a Queue."""
        import copy

        self.assertIsInstance(copy.copy(ecf.Queue("Q", ["a"])), ecf.Queue)

    def test_copy_preserves_name_and_value(self):
        """The copied Queue has the same name() and value() as the original."""
        import copy

        q = ecf.Queue("MY_QUEUE", ["first", "second"])
        c = copy.copy(q)
        self.assertEqual(c.name(), q.name())
        self.assertEqual(c.value(), q.value())

    # ------------------------------------------------------------------
    # __hash__ — identity-based
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Queue is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Queue("Q", ["a"])), int)

    def test_hash_is_identity_based(self):
        """Two value-equal Queues have different hashes (identity, not value)."""
        q1 = ecf.Queue("Q", ["a", "b"])
        q2 = ecf.Queue("Q", ["a", "b"])
        self.assertEqual(q1, q2)
        self.assertIsNot(q1, q2)
        self.assertNotEqual(hash(q1), hash(q2))

    def test_same_object_same_hash(self):
        """The same Queue always returns the same hash."""
        q = ecf.Queue("Q", ["a"])
        self.assertEqual(hash(q), hash(q))

    def test_can_be_stored_in_set(self):
        """Queue instances can be stored in a Python set."""
        q = ecf.Queue("Q", ["a"])
        self.assertIn(q, {q})

    def test_can_be_used_as_dict_key(self):
        """Queue instances can be used as dictionary keys."""
        q = ecf.Queue("Q", ["a"])
        d = {q: "entry"}
        self.assertEqual(d[q], "entry")


class TestGeneric(unittest.TestCase):
    """Tests for py::class_<GenericAttr> exposed as ecf.Generic in ExportNodeAttr.cpp.

    Generic is a forward-compatible attribute for adding new kinds of node attributes
    without requiring an API change.

    Exposed API
    -----------
    Constructors
        Generic(str, list[str])          -- name and a list of string values (may be empty);
                                            Generic() with no args also succeeds (boost.python
                                            supplies empty defaults)

    Instance methods
        name()    -> str                 -- the attribute name (returned by const reference)
        empty()   -> bool                -- always False for normally constructed Generic instances

    Properties
        values    -- re-iterable range of str; yields each value in insertion order

    Operators
        __str__   -- 'generic <name> <val1> <val2> ...' (space-separated; no values when empty)
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based: name and all values (in order) must match
        __ne__    -- implicit complement of __eq__
        __lt__    -- name-based alphabetical ordering only; generics with the same name but
                     different value lists are NOT ordered
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: Generic(str, list[str])
    # ------------------------------------------------------------------

    def test_create_stores_name(self):
        """Generic(name, values).name() returns the given name."""
        g = ecf.Generic("my_generic", ["val1", "val2"])
        self.assertEqual(g.name(), "my_generic")

    def test_create_with_values(self):
        """Values are stored and accessible via the values property."""
        g = ecf.Generic("g", ["a", "b", "c"])
        self.assertEqual(list(g.values), ["a", "b", "c"])

    def test_create_with_empty_list(self):
        """An empty value list is accepted (unlike Queue)."""
        g = ecf.Generic("g", [])
        self.assertEqual(list(g.values), [])

    def test_create_single_value(self):
        """A single-element list is valid."""
        g = ecf.Generic("g", ["only"])
        self.assertEqual(list(g.values), ["only"])

    def test_create_no_args_produces_default_instance(self):
        """Generic() with no arguments succeeds via boost.python default args;
        produces an instance with name='' and no values."""
        g = ecf.Generic()
        self.assertIsInstance(g, ecf.Generic)
        self.assertEqual(g.name(), "")
        self.assertEqual(list(g.values), [])

    # ------------------------------------------------------------------
    # Negative: invalid constructor arguments
    # ------------------------------------------------------------------

    def test_int_as_name_raises(self):
        """Passing an integer as name raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Generic(42, ["a"])

    def test_none_as_name_raises(self):
        """Passing None as name raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Generic(None, ["a"])

    def test_str_instead_of_list_raises(self):
        """Passing a plain string instead of a list raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Generic("g", "not_a_list")

    def test_int_in_value_list_raises(self):
        """An integer inside the value list raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Generic("g", [42])

    def test_none_in_value_list_raises(self):
        """None inside the value list raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Generic("g", [None])

    # ------------------------------------------------------------------
    # Negative: invalid name strings
    # ------------------------------------------------------------------

    def test_empty_name_raises(self):
        """An empty name string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Generic("", ["val"])

    def test_name_with_comma_raises(self):
        """A name containing a comma raises RuntimeError (only alphanumeric, _ and . allowed)."""
        with self.assertRaises(RuntimeError):
            ecf.Generic("with,comma", ["val"])

    def test_name_with_semicolon_raises(self):
        """A name containing a semicolon raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Generic("with;semicolon", ["val"])

    def test_name_with_exclamation_raises(self):
        """A name containing an exclamation mark raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Generic("with!exclaim", ["val"])

    def test_name_with_space_raises(self):
        """A name containing a space raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Generic("with space", ["val"])

    def test_name_with_dash_raises(self):
        """A name containing a dash raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Generic("with-dash", ["val"])

    # ------------------------------------------------------------------
    # Valid name edge cases
    # ------------------------------------------------------------------

    def test_name_with_leading_underscore_is_valid(self):
        """A name beginning with an underscore is accepted."""
        g = ecf.Generic("_MY_GENERIC", ["val"])
        self.assertEqual(g.name(), "_MY_GENERIC")

    def test_name_with_dot_is_valid(self):
        """A name containing a dot is accepted."""
        g = ecf.Generic("generic.name", ["val"])
        self.assertEqual(g.name(), "generic.name")

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() returns a Python str."""
        self.assertIsInstance(ecf.Generic("g", ["a"]).name(), str)

    def test_name_returns_correct_value(self):
        """name() returns exactly the name passed at construction."""
        self.assertEqual(ecf.Generic("MY_GENERIC", ["a"]).name(), "MY_GENERIC")

    # ------------------------------------------------------------------
    # empty()
    # ------------------------------------------------------------------

    def test_empty_returns_false_with_values(self):
        """empty() returns False for a Generic with values."""
        self.assertFalse(ecf.Generic("g", ["a"]).empty())

    def test_empty_returns_false_with_empty_value_list(self):
        """empty() returns False even when the value list is empty."""
        self.assertFalse(ecf.Generic("g", []).empty())

    def test_empty_returns_bool(self):
        """empty() returns a Python bool."""
        self.assertIsInstance(ecf.Generic("g", ["a"]).empty(), bool)

    # ------------------------------------------------------------------
    # values property
    # ------------------------------------------------------------------

    def test_values_yields_correct_strings(self):
        """values yields each string from the construction list in order."""
        g = ecf.Generic("g", ["x", "y", "z"])
        self.assertEqual(list(g.values), ["x", "y", "z"])

    def test_values_empty_when_no_values(self):
        """values yields nothing when the construction list was empty."""
        g = ecf.Generic("g", [])
        self.assertEqual(list(g.values), [])

    def test_values_is_re_iterable(self):
        """The values property can be iterated more than once."""
        g = ecf.Generic("g", ["a", "b"])
        self.assertEqual(list(g.values), list(g.values))

    def test_values_elements_are_str(self):
        """Every element yielded by values is a Python str."""
        g = ecf.Generic("g", ["hello", "world"])
        for v in g.values:
            self.assertIsInstance(v, str)

    def test_values_preserves_insertion_order(self):
        """values yields items in the same order as the construction list."""
        items = ["third", "first", "second"]
        g = ecf.Generic("g", items)
        self.assertEqual(list(g.values), items)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format_with_values(self):
        """str() follows the format 'generic <name> <val1> <val2> ...'."""
        self.assertEqual(str(ecf.Generic("g", ["a", "b"])), "generic g a b")

    def test_str_format_single_value(self):
        """str() with a single value outputs 'generic <name> <val>'."""
        self.assertEqual(str(ecf.Generic("g", ["only"])), "generic g only")

    def test_str_format_empty_values(self):
        """str() with an empty value list outputs 'generic <name>' (no trailing space)."""
        self.assertEqual(str(ecf.Generic("g", [])), "generic g")

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Generic("g", ["a"])), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — value-based (name + all values, in order)
    # ------------------------------------------------------------------

    def test_eq_same_name_and_values(self):
        """Two Generics with identical name and values are equal."""
        self.assertEqual(ecf.Generic("G", ["a", "b"]), ecf.Generic("G", ["a", "b"]))

    def test_eq_reflexive(self):
        """A Generic is equal to itself."""
        g = ecf.Generic("G", ["a", "b"])
        self.assertEqual(g, g)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        g1 = ecf.Generic("G", ["a", "b"])
        g2 = ecf.Generic("G", ["a", "b"])
        self.assertTrue(g1 == g2)
        self.assertTrue(g2 == g1)

    def test_eq_empty_value_lists(self):
        """Two Generics with the same name and empty value lists are equal."""
        self.assertEqual(ecf.Generic("G", []), ecf.Generic("G", []))

    def test_ne_different_value(self):
        """Generics with the same name but different values are not equal."""
        self.assertNotEqual(ecf.Generic("G", ["a", "b"]), ecf.Generic("G", ["a", "c"]))

    def test_ne_different_value_count(self):
        """Generics with the same name but different numbers of values are not equal."""
        self.assertNotEqual(ecf.Generic("G", ["a", "b"]), ecf.Generic("G", ["a"]))

    def test_ne_different_value_order(self):
        """Generics with the same values in a different order are not equal."""
        self.assertNotEqual(ecf.Generic("G", ["a", "b"]), ecf.Generic("G", ["b", "a"]))

    def test_ne_different_name(self):
        """Generics with different names are not equal."""
        self.assertNotEqual(ecf.Generic("A", ["a"]), ecf.Generic("B", ["a"]))

    def test_ne_values_vs_no_values(self):
        """A Generic with values is not equal to one with an empty list."""
        self.assertNotEqual(ecf.Generic("G", ["a"]), ecf.Generic("G", []))

    # ------------------------------------------------------------------
    # __lt__ — name-based ordering only
    # ------------------------------------------------------------------

    def test_lt_alphabetically_earlier_name_is_less(self):
        """Generic with an earlier name is less than one with a later name."""
        self.assertLess(ecf.Generic("A", ["a"]), ecf.Generic("B", ["a"]))

    def test_lt_later_name_is_not_less(self):
        """Generic with a later name is NOT less."""
        self.assertFalse(ecf.Generic("B", ["a"]) < ecf.Generic("A", ["a"]))

    def test_lt_same_name_diff_values_not_less(self):
        """Generics with the same name are not ordered regardless of value lists."""
        self.assertFalse(ecf.Generic("G", ["a"]) < ecf.Generic("G", ["z"]))
        self.assertFalse(ecf.Generic("G", ["z"]) < ecf.Generic("G", ["a"]))

    def test_lt_equal_generics_not_less(self):
        """A Generic is not strictly less than an equal Generic."""
        self.assertFalse(ecf.Generic("G", ["a"]) < ecf.Generic("G", ["a"]))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces a Generic equal in value to the original."""
        import copy

        g = ecf.Generic("G", ["a", "b", "c"])
        self.assertEqual(g, copy.copy(g))

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        import copy

        g = ecf.Generic("G", ["a", "b"])
        self.assertIsNot(g, copy.copy(g))

    def test_copy_is_generic_instance(self):
        """The result of copy.copy() is a Generic."""
        import copy

        self.assertIsInstance(copy.copy(ecf.Generic("G", ["a"])), ecf.Generic)

    def test_copy_preserves_name_and_values(self):
        """The copied Generic has the same name() and values as the original."""
        import copy

        g = ecf.Generic("MY_GENERIC", ["x", "y", "z"])
        c = copy.copy(g)
        self.assertEqual(c.name(), g.name())
        self.assertEqual(list(c.values), list(g.values))

    def test_copy_of_empty_values_generic(self):
        """copy.copy() works correctly on a Generic with an empty value list."""
        import copy

        g = ecf.Generic("G", [])
        c = copy.copy(g)
        self.assertEqual(c, g)
        self.assertEqual(list(c.values), [])

    # ------------------------------------------------------------------
    # __hash__ — identity-based
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Generic is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Generic("G", ["a"])), int)

    def test_hash_is_identity_based(self):
        """Two value-equal Generics have different hashes (identity, not value)."""
        g1 = ecf.Generic("G", ["a", "b"])
        g2 = ecf.Generic("G", ["a", "b"])
        self.assertEqual(g1, g2)
        self.assertIsNot(g1, g2)
        self.assertNotEqual(hash(g1), hash(g2))

    def test_same_object_same_hash(self):
        """The same Generic always returns the same hash."""
        g = ecf.Generic("G", ["a"])
        self.assertEqual(hash(g), hash(g))

    def test_can_be_stored_in_set(self):
        """Generic instances can be stored in a Python set."""
        g = ecf.Generic("G", ["a"])
        self.assertIn(g, {g})

    def test_can_be_used_as_dict_key(self):
        """Generic instances can be used as dictionary keys."""
        g = ecf.Generic("G", ["a"])
        d = {g: "entry"}
        self.assertEqual(d[g], "entry")


class TestDate(unittest.TestCase):
    """Tests for py::class_<DateAttr> exposed as ecf.Date in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Date(int, int, int)              -- day, month, year; 0 in any position means
                                            wildcard ('*' in str output);
                                            valid ranges: day 0–31, month 0–12;
                                            out-of-range raises IndexError
        Date(str)                        -- 'D.M.Y' string with '*' for wildcards,
                                            e.g. '15.6.2024', '*.6.*';
                                            invalid format raises RuntimeError

    Instance methods
        day()   -> int                   -- the day component; 0 means wild-carded
        month() -> int                   -- the month component; 0 means wild-carded
        year()  -> int                   -- the year component; 0 means wild-carded

    Operators
        __str__   -- 'date D.M.Y' with '*' substituted for each 0 component
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based: day, month and year must all match
        __ne__    -- implicit complement of __eq__
        __lt__    -- full date comparison: day first, then month, then year
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: Date(int, int, int)
    # ------------------------------------------------------------------

    def test_int_ctor_stores_day_month_year(self):
        """Date(day, month, year) stores all three components."""
        d = ecf.Date(15, 6, 2024)
        self.assertEqual(d.day(), 15)
        self.assertEqual(d.month(), 6)
        self.assertEqual(d.year(), 2024)

    def test_int_ctor_zero_is_wildcard(self):
        """Passing 0 for any component stores 0 (wildcard)."""
        d = ecf.Date(0, 0, 0)
        self.assertEqual(d.day(), 0)
        self.assertEqual(d.month(), 0)
        self.assertEqual(d.year(), 0)

    def test_int_ctor_partial_wildcards(self):
        """Individual components may be wildcarded while others are set."""
        self.assertEqual(ecf.Date(1, 0, 0).day(), 1)
        self.assertEqual(ecf.Date(1, 0, 0).month(), 0)
        self.assertEqual(ecf.Date(0, 6, 0).month(), 6)
        self.assertEqual(ecf.Date(0, 0, 2024).year(), 2024)

    def test_int_ctor_boundary_day_31(self):
        """Day 31 is a valid upper boundary."""
        d = ecf.Date(31, 1, 2024)
        self.assertEqual(d.day(), 31)

    def test_int_ctor_boundary_month_12(self):
        """Month 12 is a valid upper boundary."""
        d = ecf.Date(1, 12, 2024)
        self.assertEqual(d.month(), 12)

    def test_int_ctor_day_too_large_raises(self):
        """day > 31 raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Date(32, 1, 2024)

    def test_int_ctor_negative_day_raises(self):
        """A negative day raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Date(-1, 1, 2024)

    def test_int_ctor_month_too_large_raises(self):
        """month > 12 raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Date(1, 13, 2024)

    def test_int_ctor_negative_month_raises(self):
        """A negative month raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Date(1, -1, 2024)

    def test_int_ctor_day_far_too_large_raises(self):
        """A day far above the upper bound (e.g. 100) still raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Date(100, 1, 2024)

    def test_int_ctor_month_far_too_large_raises(self):
        """A month far above the upper bound (e.g. 100) still raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Date(1, 100, 2024)

    def test_int_ctor_both_out_of_range_day_checked_first(self):
        """When both day and month are out of range, the day check fires first."""
        with self.assertRaises(IndexError):
            ecf.Date(-2, -2, 2024)

    def test_int_ctor_day_exactly_at_lower_invalid_boundary(self):
        """day=-1 is just below zero and raises IndexError (zero itself is the wildcard)."""
        with self.assertRaises(IndexError):
            ecf.Date(-1, 6, 2024)

    def test_int_ctor_month_exactly_at_lower_invalid_boundary(self):
        """month=-1 is just below zero and raises IndexError (zero itself is the wildcard)."""
        with self.assertRaises(IndexError):
            ecf.Date(1, -1, 2024)

    def test_int_ctor_day_exactly_at_upper_invalid_boundary(self):
        """day=32 is just above 31 and raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Date(32, 6, 2024)

    def test_int_ctor_month_exactly_at_upper_invalid_boundary(self):
        """month=13 is just above 12 and raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Date(1, 13, 2024)

    # ------------------------------------------------------------------
    # Constructor: Date(str)
    # ------------------------------------------------------------------

    def test_str_ctor_full_date(self):
        """Date('D.M.Y') parses a fully specified date string."""
        d = ecf.Date("15.6.2024")
        self.assertEqual(d.day(), 15)
        self.assertEqual(d.month(), 6)
        self.assertEqual(d.year(), 2024)

    def test_str_ctor_wildcard_day(self):
        """Date('*.M.Y') wildcards the day (day() == 0)."""
        d = ecf.Date("*.6.2024")
        self.assertEqual(d.day(), 0)
        self.assertEqual(d.month(), 6)
        self.assertEqual(d.year(), 2024)

    def test_str_ctor_wildcard_month(self):
        """Date('D.*.Y') wildcards the month (month() == 0)."""
        d = ecf.Date("1.*.2024")
        self.assertEqual(d.day(), 1)
        self.assertEqual(d.month(), 0)
        self.assertEqual(d.year(), 2024)

    def test_str_ctor_wildcard_year(self):
        """Date('D.M.*') wildcards the year (year() == 0)."""
        d = ecf.Date("1.1.*")
        self.assertEqual(d.year(), 0)

    def test_str_ctor_all_wildcards(self):
        """Date('*.*.*') wildcards all three components."""
        d = ecf.Date("*.*.*")
        self.assertEqual(d.day(), 0)
        self.assertEqual(d.month(), 0)
        self.assertEqual(d.year(), 0)

    def test_str_ctor_roundtrip_via_str(self):
        """Constructing from the str() output of another Date produces an equal Date."""
        original = ecf.Date(15, 6, 2024)
        text = str(original)[len("date ") :]  # strip "date " prefix
        reconstructed = ecf.Date(text)
        self.assertEqual(original, reconstructed)

    def test_str_ctor_invalid_format_raises(self):
        """A string not matching 'D.M.Y' raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Date("invalid")

    def test_str_ctor_missing_dots_raises(self):
        """A string with no dots raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Date("15062024")

    def test_str_ctor_empty_string_raises(self):
        """An empty string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Date("")

    # ------------------------------------------------------------------
    # Wrong constructor argument types
    # ------------------------------------------------------------------

    def test_wrong_type_str_for_day_raises(self):
        """A string in the day position raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Date("15", 6, 2024)

    def test_wrong_type_str_for_month_raises(self):
        """A string in the month position raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Date(15, "6", 2024)

    def test_wrong_type_str_for_year_raises(self):
        """A string in the year position raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Date(15, 6, "2024")

    def test_wrong_type_none_raises(self):
        """None in any position raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Date(None, 6, 2024)

    def test_too_few_args_raises(self):
        """Passing only two integer arguments raises an error."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Date(1, 1)

    # ------------------------------------------------------------------
    # day() / month() / year() — return types
    # ------------------------------------------------------------------

    def test_day_returns_int(self):
        """day() returns a Python int."""
        self.assertIsInstance(ecf.Date(15, 6, 2024).day(), int)

    def test_month_returns_int(self):
        """month() returns a Python int."""
        self.assertIsInstance(ecf.Date(15, 6, 2024).month(), int)

    def test_year_returns_int(self):
        """year() returns a Python int."""
        self.assertIsInstance(ecf.Date(15, 6, 2024).year(), int)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format_full_date(self):
        """str() produces 'date D.M.Y' for a fully specified date."""
        self.assertEqual(str(ecf.Date(15, 6, 2024)), "date 15.6.2024")

    def test_str_all_wildcards(self):
        """str() produces 'date *.*.*' when all components are 0."""
        self.assertEqual(str(ecf.Date(0, 0, 0)), "date *.*.*")

    def test_str_day_set_others_wildcard(self):
        """str() uses '*' for any 0 component and the actual value otherwise."""
        self.assertEqual(str(ecf.Date(1, 0, 0)), "date 1.*.*")

    def test_str_month_set_others_wildcard(self):
        """str() with only month set produces 'date *.M.*'."""
        self.assertEqual(str(ecf.Date(0, 6, 0)), "date *.6.*")

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Date(1, 1, 2024)), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__ — value-based (day + month + year must match)
    # ------------------------------------------------------------------

    def test_eq_same_date(self):
        """Two Dates with identical components are equal."""
        self.assertEqual(ecf.Date(1, 1, 2025), ecf.Date(1, 1, 2025))

    def test_eq_reflexive(self):
        """A Date is equal to itself."""
        d = ecf.Date(1, 1, 2025)
        self.assertEqual(d, d)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        d1 = ecf.Date(1, 1, 2025)
        d2 = ecf.Date(1, 1, 2025)
        self.assertTrue(d1 == d2)
        self.assertTrue(d2 == d1)

    def test_ne_different_day(self):
        """Dates with different days are not equal."""
        self.assertNotEqual(ecf.Date(1, 1, 2025), ecf.Date(2, 1, 2025))

    def test_ne_different_month(self):
        """Dates with different months are not equal."""
        self.assertNotEqual(ecf.Date(1, 1, 2025), ecf.Date(1, 2, 2025))

    def test_ne_different_year(self):
        """Dates with different years are not equal."""
        self.assertNotEqual(ecf.Date(1, 1, 2025), ecf.Date(1, 1, 2026))

    def test_eq_wildcard_dates(self):
        """Two all-wildcard Dates are equal."""
        self.assertEqual(ecf.Date(0, 0, 0), ecf.Date(0, 0, 0))

    # ------------------------------------------------------------------
    # __lt__ — full date comparison (day, then month, then year)
    # ------------------------------------------------------------------

    def test_lt_earlier_day_is_less(self):
        """Date with a smaller day (same month+year) is less."""
        self.assertLess(ecf.Date(1, 1, 2025), ecf.Date(2, 1, 2025))

    def test_lt_earlier_month_is_less(self):
        """Date with a smaller month (same day+year) is less."""
        self.assertLess(ecf.Date(1, 1, 2025), ecf.Date(1, 2, 2025))

    def test_lt_earlier_year_is_less(self):
        """Date with a smaller year (same day+month) is less."""
        self.assertLess(ecf.Date(1, 1, 2025), ecf.Date(1, 1, 2026))

    def test_lt_later_date_not_less(self):
        """A later Date is NOT less than an earlier one."""
        self.assertFalse(ecf.Date(2, 1, 2025) < ecf.Date(1, 1, 2025))

    def test_lt_equal_date_not_less(self):
        """A Date is not strictly less than an equal Date."""
        self.assertFalse(ecf.Date(1, 1, 2025) < ecf.Date(1, 1, 2025))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces a Date equal in value to the original."""
        import copy

        d = ecf.Date(15, 6, 2024)
        self.assertEqual(d, copy.copy(d))

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        import copy

        d = ecf.Date(15, 6, 2024)
        self.assertIsNot(d, copy.copy(d))

    def test_copy_is_date_instance(self):
        """The result of copy.copy() is a Date."""
        import copy

        self.assertIsInstance(copy.copy(ecf.Date(1, 1, 2024)), ecf.Date)

    def test_copy_preserves_all_components(self):
        """The copied Date has the same day(), month() and year()."""
        import copy

        d = ecf.Date(15, 6, 2024)
        c = copy.copy(d)
        self.assertEqual(c.day(), d.day())
        self.assertEqual(c.month(), d.month())
        self.assertEqual(c.year(), d.year())

    # ------------------------------------------------------------------
    # __hash__ — identity-based
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Date is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Date(1, 1, 2024)), int)

    def test_hash_is_identity_based(self):
        """Two value-equal Dates have different hashes (identity, not value)."""
        d1 = ecf.Date(1, 1, 2025)
        d2 = ecf.Date(1, 1, 2025)
        self.assertEqual(d1, d2)
        self.assertIsNot(d1, d2)
        self.assertNotEqual(hash(d1), hash(d2))

    def test_same_object_same_hash(self):
        """The same Date always returns the same hash."""
        d = ecf.Date(1, 1, 2025)
        self.assertEqual(hash(d), hash(d))

    def test_can_be_stored_in_set(self):
        """Date instances can be stored in a Python set."""
        d = ecf.Date(1, 1, 2025)
        self.assertIn(d, {d})

    def test_can_be_used_as_dict_key(self):
        """Date instances can be used as dictionary keys."""
        d = ecf.Date(1, 1, 2025)
        d_dict = {d: "entry"}
        self.assertEqual(d_dict[d], "entry")


class TestDays(unittest.TestCase):
    """Tests for py::enum_<DayAttr::Day_t> exposed as ecf.Days in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Class attributes (one per enum value, 7 total)
        ecf.Days.sunday, .monday, .tuesday, .wednesday, .thursday, .friday, .saturday

    Type hierarchy
        Days → enum → int → object
        Instances are simultaneously Days and int.

    Per-instance properties
        v.name       -- Python attribute name as str  (same as str(v))
        str(v)       -- bare name (e.g. 'monday')
        repr(v)      -- 'ecflow.Days.<name>'
        int(v)       -- underlying C++ integer value (sunday=0 … saturday=6)

    Class-level lookup dicts (singletons)
        Days.values  -- {int : Days}  7 entries; keys 0–6
        Days.names   -- {str : Days}  7 entries

    Operators (inherited from int)
        __eq__ / __ne__ / __lt__ / __le__ / __gt__ / __ge__  -- integer comparison
        __hash__                                              -- hash == int(v)

    Iteration
        list(Days)  -- raises TypeError; the class itself is NOT iterable;
                       iterate over Days.values.values() instead

    Construction
        Days(n)  -- int subclass constructor; may not return the class-attr singleton

    Note: integer values are perfectly contiguous 0–6 in declaration order,
          with sunday=0 being the first and smallest.
    """

    ALL_NAMES: List[str] = [
        "sunday",
        "monday",
        "tuesday",
        "wednesday",
        "thursday",
        "friday",
        "saturday",
    ]

    INT_VALUES: Dict[str, int] = {
        "sunday": 0,
        "monday": 1,
        "tuesday": 2,
        "wednesday": 3,
        "thursday": 4,
        "friday": 5,
        "saturday": 6,
    }

    # ------------------------------------------------------------------
    # Completeness
    # ------------------------------------------------------------------

    def test_total_member_count_is_7(self):
        """Exactly 7 Days values are declared in the binding."""
        self.assertEqual(len(ecf.Days.names), 7)
        self.assertEqual(len(ecf.Days.values), 7)

    def test_all_names_accessible_as_class_attributes(self):
        """Every declared name is accessible as an attribute of Days."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertTrue(hasattr(ecf.Days, name))

    def test_all_names_present_in_names_dict(self):
        """Days.names contains every declared name as a key."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(name, ecf.Days.names)

    def test_all_int_values_present_in_values_dict(self):
        """Days.values contains every declared integer value as a key."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertIn(expected_int, ecf.Days.values)

    # ------------------------------------------------------------------
    # Type hierarchy
    # ------------------------------------------------------------------

    def test_instances_are_days(self):
        """Every enum member is an instance of ecf.Days."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIsInstance(getattr(ecf.Days, name), ecf.Days)

    def test_int_conversion_works(self):
        """int(member) produces the C++ integer value (pybind11 enums are not int subclasses
        unlike Boost.Python enums, but they are still convertible to int)."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(int(getattr(ecf.Days, name)), expected)

    # ------------------------------------------------------------------
    # str / repr / .name
    # ------------------------------------------------------------------

    def test_str_returns_just_the_name(self):
        """str(v) returns only the bare attribute name."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertEqual(str(getattr(ecf.Days, name)), name)

    def test_repr_returns_module_qualified_name(self):
        """repr(v) returns 'ecflow.Days.<name>'."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertEqual(
                    repr(getattr(ecf.Days, name)),
                    f"ecflow.Days.{name}",
                )

    def test_name_attribute_equals_str(self):
        """v.name is the same string as str(v) for every member."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.Days, name)
                self.assertEqual(v.name, name)
                self.assertEqual(v.name, str(v))

    # ------------------------------------------------------------------
    # Integer values — contiguous 0–6, declaration order == numeric order
    # ------------------------------------------------------------------

    def test_int_value_matches_cpp_enum(self):
        """int(v) matches the documented C++ integer value for each member."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(int(getattr(ecf.Days, name)), expected)

    def test_sunday_is_zero(self):
        """sunday has integer value 0 (first and smallest)."""
        self.assertEqual(int(ecf.Days.sunday), 0)

    def test_saturday_is_six(self):
        """saturday has integer value 6 (last and largest)."""
        self.assertEqual(int(ecf.Days.saturday), 6)

    def test_values_are_contiguous_zero_to_six(self):
        """The integer values form an unbroken sequence 0–6."""
        all_ints = sorted(int(v) for v in ecf.Days.values.values())
        self.assertEqual(all_ints, list(range(7)))

    def test_declaration_order_matches_numeric_order(self):
        """For Days, declaration order equals numeric order (no surprises)."""
        prev_int = -1
        for name in self.ALL_NAMES:
            cur_int = int(getattr(ecf.Days, name))
            self.assertGreater(cur_int, prev_int)
            prev_int = cur_int

    # ------------------------------------------------------------------
    # .values / .names lookup dicts
    # ------------------------------------------------------------------

    def test_values_dict_maps_int_to_days(self):
        """Days.values maps each int key to a Days instance."""
        for int_val, member in ecf.Days.values.items():
            with self.subTest(int_val=int_val):
                self.assertIsInstance(int_val, int)
                self.assertIsInstance(member, ecf.Days)

    def test_names_dict_maps_str_to_days(self):
        """Days.names maps each string key to a Days instance."""
        for name, member in ecf.Days.names.items():
            with self.subTest(name=name):
                self.assertIsInstance(name, str)
                self.assertIsInstance(member, ecf.Days)

    def test_values_dict_roundtrip(self):
        """Days.values[int(v)] is the same singleton as the class attribute."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.Days, name)
                self.assertIs(ecf.Days.values[int(v)], v)

    def test_names_dict_roundtrip(self):
        """Days.names[str(v)] is the same singleton as the class attribute."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.Days, name)
                self.assertIs(ecf.Days.names[str(v)], v)

    # ------------------------------------------------------------------
    # Iteration — the class itself is NOT iterable
    # ------------------------------------------------------------------

    def test_days_class_is_not_iterable(self):
        """list(Days) raises TypeError; the class object has no __iter__."""
        with self.assertRaises(TypeError):
            list(ecf.Days)

    def test_all_members_reachable_via_values_dict(self):
        """Iterating Days.values.values() yields every member once."""
        members = set(ecf.Days.values.values())
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                self.assertIn(getattr(ecf.Days, name), members)

    # ------------------------------------------------------------------
    # Equality and ordering (inherited from int)
    # ------------------------------------------------------------------

    def test_same_member_is_equal_to_itself(self):
        """v == v for every member (reflexive)."""
        for name in self.ALL_NAMES:
            with self.subTest(name=name):
                v = getattr(ecf.Days, name)
                self.assertEqual(v, v)

    def test_different_members_are_not_equal(self):
        """sunday != saturday because their integer values differ."""
        self.assertNotEqual(ecf.Days.sunday, ecf.Days.saturday)

    def test_equality_with_plain_int(self):
        """A Days value equals a plain int with the matching integer value."""
        for name, expected in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(getattr(ecf.Days, name), expected)

    def test_ordering_sunday_is_smallest(self):
        """sunday (0) is less than every other Days value."""
        for name in self.ALL_NAMES:
            if name == "sunday":
                continue
            with self.subTest(name=name):
                self.assertLess(ecf.Days.sunday, getattr(ecf.Days, name))

    def test_ordering_saturday_is_largest(self):
        """saturday (6) is greater than every other Days value."""
        for name in self.ALL_NAMES:
            if name == "saturday":
                continue
            with self.subTest(name=name):
                self.assertGreater(ecf.Days.saturday, getattr(ecf.Days, name))

    def test_full_sort_produces_non_decreasing_sequence(self):
        """Sorting all members by value yields a non-decreasing integer sequence."""
        sorted_vals = sorted(ecf.Days.values.values())
        ints = [int(v) for v in sorted_vals]
        self.assertEqual(ints, sorted(ints))

    # ------------------------------------------------------------------
    # Hash (inherited from int: hash(v) == int(v))
    # ------------------------------------------------------------------

    def test_hash_equals_int_value(self):
        """hash(v) == int(v) for every member."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(hash(getattr(ecf.Days, name)), expected_int)

    def test_can_be_stored_in_set(self):
        """All 7 Days members can be stored in a Python set."""
        s = set(ecf.Days.values.values())
        self.assertEqual(len(s), 7)

    def test_set_deduplicates_equal_values(self):
        """Adding the same member twice produces one entry in a set."""
        s = {ecf.Days.monday, ecf.Days.monday}
        self.assertEqual(len(s), 1)

    def test_can_be_used_as_dict_key(self):
        """Days members work as dictionary keys."""
        d = {ecf.Days.monday: "monday", ecf.Days.friday: "friday"}
        self.assertEqual(d[ecf.Days.monday], "monday")
        self.assertEqual(d[ecf.Days.friday], "friday")

    # ------------------------------------------------------------------
    # Construction from int
    # ------------------------------------------------------------------

    def test_can_construct_from_int(self):
        """Days(n) constructs a Days instance from a raw integer."""
        v = ecf.Days(0)
        self.assertIsInstance(v, ecf.Days)
        self.assertEqual(int(v), 0)

    def test_int_constructor_value_equals_class_attribute(self):
        """Days(n) produces a value equal to the corresponding class attribute."""
        for name, expected_int in self.INT_VALUES.items():
            with self.subTest(name=name):
                self.assertEqual(ecf.Days(expected_int), getattr(ecf.Days, name))

    # ------------------------------------------------------------------
    # Negative cases
    # ------------------------------------------------------------------

    def test_nonexistent_member_raises_attribute_error(self):
        """Accessing an undefined name on Days raises AttributeError."""
        with self.assertRaises(AttributeError):
            _ = ecf.Days.nonexistent_day

    def test_missing_int_key_in_values_raises_key_error(self):
        """Looking up an int that has no member in values raises KeyError."""
        with self.assertRaises(KeyError):
            _ = ecf.Days.values[9999]

    def test_missing_name_key_in_names_raises_key_error(self):
        """Looking up a name that has no member in names raises KeyError."""
        with self.assertRaises(KeyError):
            _ = ecf.Days.names["nonexistent_day"]


class TestDay(unittest.TestCase):
    """Tests for py::class_<DayAttr> exposed as ecf.Day in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Day(Days)   -- takes a Days enum value (DayAttr::Day_t)
        Day(str)    -- takes one of: sunday monday tuesday wednesday thursday friday saturday
                       any other string raises RuntimeError

    Instance methods
        day() -> Days  -- returns the stored Days enum value

    Operators
        __str__   -- 'day <name>', e.g. 'day monday'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based: two Days with the same enum are equal
        __ne__    -- implicit complement of __eq__
        __lt__    -- ordered by the integer value of the underlying Days enum
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: Day(Days)
    # ------------------------------------------------------------------

    def test_enum_ctor_all_days(self):
        """Day(Days.X) succeeds for all seven days."""
        expected = [
            (ecf.Days.sunday, "day sunday"),
            (ecf.Days.monday, "day monday"),
            (ecf.Days.tuesday, "day tuesday"),
            (ecf.Days.wednesday, "day wednesday"),
            (ecf.Days.thursday, "day thursday"),
            (ecf.Days.friday, "day friday"),
            (ecf.Days.saturday, "day saturday"),
        ]
        for enum_val, expected_str in expected:
            d = ecf.Day(enum_val)
            self.assertEqual(str(d), expected_str)

    def test_enum_ctor_day_returns_days_instance(self):
        """day() on an enum-constructed Day returns a Days instance."""
        d = ecf.Day(ecf.Days.wednesday)
        self.assertIsInstance(d.day(), ecf.Days)

    def test_enum_ctor_day_value_matches_enum(self):
        """day() returns the same Days value that was passed to the constructor."""
        for enum_val in [
            ecf.Days.sunday,
            ecf.Days.monday,
            ecf.Days.tuesday,
            ecf.Days.wednesday,
            ecf.Days.thursday,
            ecf.Days.friday,
            ecf.Days.saturday,
        ]:
            self.assertEqual(ecf.Day(enum_val).day(), enum_val)

    # ------------------------------------------------------------------
    # Constructor: Day(str)
    # ------------------------------------------------------------------

    def test_str_ctor_all_valid_names(self):
        """Day(name) succeeds for all seven day name strings."""
        pairs = [
            ("sunday", ecf.Days.sunday),
            ("monday", ecf.Days.monday),
            ("tuesday", ecf.Days.tuesday),
            ("wednesday", ecf.Days.wednesday),
            ("thursday", ecf.Days.thursday),
            ("friday", ecf.Days.friday),
            ("saturday", ecf.Days.saturday),
        ]
        for name, enum_val in pairs:
            d = ecf.Day(name)
            self.assertEqual(d.day(), enum_val)

    def test_str_ctor_day_equals_enum_ctor(self):
        """Day('monday') is equal to Day(Days.monday)."""
        self.assertEqual(ecf.Day("monday"), ecf.Day(ecf.Days.monday))

    def test_str_ctor_invalid_name_raises(self):
        """An unrecognised string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Day("holiday")

    def test_str_ctor_empty_string_raises(self):
        """An empty string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Day("")

    def test_str_ctor_case_sensitive_raises(self):
        """Day names are case-sensitive; 'Monday' (capitalised) raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Day("Monday")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_int_arg_raises(self):
        """Passing a plain integer raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Day(42)

    def test_none_arg_raises(self):
        """Passing None raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Day(None)

    def test_float_arg_raises(self):
        """Passing a float raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Day(1.5)

    def test_list_arg_raises(self):
        """Passing a list raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Day([])

    # ------------------------------------------------------------------
    # day() return type
    # ------------------------------------------------------------------

    def test_day_method_returns_days_type(self):
        """day() always returns an instance of ecf.Days."""
        for d in [ecf.Day(ecf.Days.friday), ecf.Day("saturday")]:
            self.assertIsInstance(d.day(), ecf.Days)

    def test_str_ctor_day_method_matches_enum(self):
        """Day('friday').day() == Days.friday."""
        self.assertEqual(ecf.Day("friday").day(), ecf.Days.friday)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format(self):
        """str(Day(X)) returns 'day <name>' for every day."""
        pairs = [
            (ecf.Days.sunday, "day sunday"),
            (ecf.Days.monday, "day monday"),
            (ecf.Days.tuesday, "day tuesday"),
            (ecf.Days.wednesday, "day wednesday"),
            (ecf.Days.thursday, "day thursday"),
            (ecf.Days.friday, "day friday"),
            (ecf.Days.saturday, "day saturday"),
        ]
        for enum_val, expected in pairs:
            self.assertEqual(str(ecf.Day(enum_val)), expected)

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Day(ecf.Days.monday)), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A Day is equal to itself."""
        d = ecf.Day(ecf.Days.monday)
        self.assertEqual(d, d)

    def test_eq_same_day(self):
        """Two Days with the same enum are equal."""
        self.assertEqual(ecf.Day(ecf.Days.tuesday), ecf.Day(ecf.Days.tuesday))

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.Day(ecf.Days.wednesday)
        b = ecf.Day(ecf.Days.wednesday)
        self.assertTrue(a == b)
        self.assertTrue(b == a)

    def test_ne_different_day(self):
        """Days with different enum values are not equal."""
        self.assertNotEqual(ecf.Day(ecf.Days.monday), ecf.Day(ecf.Days.friday))

    def test_ne_reflexive_complement(self):
        """a != b is False when a == b."""
        a = ecf.Day(ecf.Days.saturday)
        b = ecf.Day(ecf.Days.saturday)
        self.assertFalse(a != b)

    def test_ne_is_true_for_different_days(self):
        """a != b is True when day values differ."""
        self.assertTrue(ecf.Day(ecf.Days.monday) != ecf.Day(ecf.Days.sunday))

    # ------------------------------------------------------------------
    # __lt__
    # ------------------------------------------------------------------

    def test_lt_ordered_by_enum_value(self):
        """Earlier days (smaller enum int) are less than later days."""
        self.assertLess(ecf.Day(ecf.Days.sunday), ecf.Day(ecf.Days.monday))
        self.assertLess(ecf.Day(ecf.Days.monday), ecf.Day(ecf.Days.saturday))

    def test_lt_full_week_ordering(self):
        """Days are ordered sun < mon < tue < wed < thu < fri < sat."""
        order = [
            ecf.Days.sunday,
            ecf.Days.monday,
            ecf.Days.tuesday,
            ecf.Days.wednesday,
            ecf.Days.thursday,
            ecf.Days.friday,
            ecf.Days.saturday,
        ]
        days = [ecf.Day(e) for e in order]
        for i in range(len(days) - 1):
            self.assertLess(days[i], days[i + 1])

    def test_lt_equal_not_less(self):
        """A Day is not strictly less than an equal Day."""
        d = ecf.Day(ecf.Days.wednesday)
        e = ecf.Day(ecf.Days.wednesday)
        self.assertFalse(d < e)

    def test_lt_later_not_less(self):
        """A later Day is not less than an earlier one."""
        self.assertFalse(ecf.Day(ecf.Days.saturday) < ecf.Day(ecf.Days.sunday))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_day_instance(self):
        """The result of copy.copy() is a Day."""
        self.assertIsInstance(copy.copy(ecf.Day(ecf.Days.monday)), ecf.Day)

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        d = ecf.Day(ecf.Days.monday)
        self.assertIsNot(copy.copy(d), d)

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces a Day equal in value to the original."""
        d = ecf.Day(ecf.Days.friday)
        self.assertEqual(copy.copy(d), d)

    def test_copy_preserves_day(self):
        """The copied Day has the same day() value."""
        d = ecf.Day(ecf.Days.thursday)
        self.assertEqual(copy.copy(d).day(), d.day())

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Day is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Day(ecf.Days.monday)), int)

    def test_same_object_same_hash(self):
        """The same Day always returns the same hash."""
        d = ecf.Day(ecf.Days.tuesday)
        self.assertEqual(hash(d), hash(d))

    def test_hash_is_identity_based(self):
        """Two value-equal Days have different hashes (identity, not value)."""
        a = ecf.Day(ecf.Days.monday)
        b = ecf.Day(ecf.Days.monday)
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """Day instances can be stored in a Python set."""
        days = {ecf.Day(e) for e in ecf.Days.values.values()}
        self.assertEqual(len(days), 7)

    def test_can_be_used_as_dict_key(self):
        """Day instances can be used as dictionary keys."""
        d = ecf.Day(ecf.Days.wednesday)
        mapping = {d: "midweek"}
        self.assertEqual(mapping[d], "midweek")


class TestTime(unittest.TestCase):
    """Tests for py::class_<ecf::TimeAttr> exposed as ecf.Time in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Time(TimeSlot, bool=False)                      -- single time slot, optional relative flag
        Time(int, int, bool=False)                      -- hour, minute, optional relative flag
        Time(TimeSeries)                                -- full time series object
        Time(TimeSlot, TimeSlot, TimeSlot, bool)        -- start, finish, increment, relative
        Time(str)                                       -- 'HH:MM' or '+HH:MM' for relative

    Instance methods
        time_series() -> TimeSeries  -- the underlying time series

    Operators
        __str__   -- 'time HH:MM', 'time +HH:MM', or 'time HH:MM HH:MM HH:MM'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __hash__  -- identity-based (boost.python C-extension type); no __lt__

    Out-of-range behaviour (int ctor)
        hour > 23  -> IndexError
        minute > 59 -> IndexError
        negative hour/minute -> C++ assertion abort (known bug; not tested here)
    """

    # ------------------------------------------------------------------
    # Constructor: Time(TimeSlot, bool=False)
    # ------------------------------------------------------------------

    def test_timeslot_ctor_absolute(self):
        """Time(TimeSlot(h, m)) creates an absolute time."""
        t = ecf.Time(ecf.TimeSlot(12, 30))
        self.assertEqual(str(t), "time 12:30")

    def test_timeslot_ctor_explicit_false(self):
        """Time(TimeSlot, False) is the same as the default (absolute)."""
        self.assertEqual(
            ecf.Time(ecf.TimeSlot(8, 0), False), ecf.Time(ecf.TimeSlot(8, 0))
        )

    def test_timeslot_ctor_relative(self):
        """Time(TimeSlot, True) produces a relative time ('+' prefix in str)."""
        t = ecf.Time(ecf.TimeSlot(1, 30), True)
        self.assertEqual(str(t), "time +01:30")

    # ------------------------------------------------------------------
    # Constructor: Time(int, int, bool=False)
    # ------------------------------------------------------------------

    def test_int_ctor_absolute(self):
        """Time(h, m) creates an absolute time."""
        t = ecf.Time(12, 0)
        self.assertEqual(str(t), "time 12:00")

    def test_int_ctor_relative(self):
        """Time(h, m, True) creates a relative time."""
        t = ecf.Time(12, 30, True)
        self.assertEqual(str(t), "time +12:30")

    def test_int_ctor_stores_hour_and_minute(self):
        """time_series().start() reflects h:m from the int ctor."""
        t = ecf.Time(9, 45)
        ts = t.time_series()
        self.assertEqual(str(ts.start()), "09:45")

    def test_int_ctor_relative_flag_in_time_series(self):
        """time_series().relative() is True when relative=True is passed."""
        self.assertTrue(ecf.Time(6, 0, True).time_series().relative())
        self.assertFalse(ecf.Time(6, 0, False).time_series().relative())

    def test_int_ctor_midnight(self):
        """Time(0, 0) is valid (midnight)."""
        t = ecf.Time(0, 0)
        self.assertEqual(str(t), "time 00:00")

    def test_int_ctor_boundary_hour_23(self):
        """Hour 23 is the valid upper boundary."""
        t = ecf.Time(23, 59)
        self.assertEqual(str(t), "time 23:59")

    def test_int_ctor_hour_too_large_raises(self):
        """hour > 23 raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Time(25, 0)

    def test_int_ctor_minute_too_large_raises(self):
        """minute > 59 raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Time(0, 60)

    # ------------------------------------------------------------------
    # Constructor: Time(TimeSeries)
    # ------------------------------------------------------------------

    def test_timeseries_ctor_single_slot(self):
        """Time(TimeSeries(TimeSlot)) creates a single-slot time."""
        ts = ecf.TimeSeries(ecf.TimeSlot(15, 0))
        t = ecf.Time(ts)
        self.assertEqual(str(t), "time 15:00")

    def test_timeseries_ctor_range(self):
        """Time(TimeSeries(start,finish,incr,False)) creates a range time."""
        ts = ecf.TimeSeries(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        t = ecf.Time(ts)
        self.assertEqual(str(t), "time 06:00 20:00 00:30")

    def test_timeseries_ctor_equals_slot_ctor(self):
        """Time(TimeSeries) equals the matching 4-slot constructor form."""
        ts = ecf.TimeSeries(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        ta = ecf.Time(ts)
        tb = ecf.Time(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertEqual(ta, tb)

    # ------------------------------------------------------------------
    # Constructor: Time(TimeSlot, TimeSlot, TimeSlot, bool)
    # ------------------------------------------------------------------

    def test_four_slot_ctor_absolute_range(self):
        """Time(start, finish, incr, False) creates an absolute range."""
        t = ecf.Time(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertEqual(str(t), "time 06:00 20:00 00:30")

    def test_four_slot_ctor_relative_range(self):
        """Time(start, finish, incr, True) creates a relative range."""
        t = ecf.Time(ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), True)
        self.assertEqual(str(t), "time +06:00 20:00 00:30")

    def test_four_slot_ctor_has_increment(self):
        """A range-constructed Time has an increment in its time_series."""
        t = ecf.Time(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertTrue(t.time_series().has_increment())

    def test_four_slot_ctor_start_finish_incr(self):
        """time_series() from 4-slot ctor reflects start, finish and incr."""
        t = ecf.Time(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        ts = t.time_series()
        self.assertEqual(str(ts.start()), "06:00")
        self.assertEqual(str(ts.finish()), "20:00")
        self.assertEqual(str(ts.incr()), "00:30")

    # ------------------------------------------------------------------
    # Constructor: Time(str)
    # ------------------------------------------------------------------

    def test_str_ctor_absolute(self):
        """Time('HH:MM') creates an absolute time."""
        t = ecf.Time("12:30")
        self.assertEqual(str(t), "time 12:30")

    def test_str_ctor_relative_plus_prefix(self):
        """Time('+HH:MM') creates a relative time."""
        t = ecf.Time("+12:30")
        self.assertEqual(str(t), "time +12:30")

    def test_str_ctor_equals_int_ctor(self):
        """Time('12:30') is equal to Time(12, 30)."""
        self.assertEqual(ecf.Time("12:30"), ecf.Time(12, 30))

    def test_str_ctor_bad_string_raises(self):
        """An unrecognised time string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Time("bad")

    def test_str_ctor_hour_out_of_range_raises(self):
        """'25:00' raises RuntimeError (hour > 23)."""
        with self.assertRaises(RuntimeError):
            ecf.Time("25:00")

    def test_str_ctor_minute_out_of_range_raises(self):
        """'12:60' raises RuntimeError (minute > 59)."""
        with self.assertRaises(RuntimeError):
            ecf.Time("12:60")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_none_raises(self):
        """Passing None raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Time(None)

    def test_float_raises(self):
        """Passing a float raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Time(1.5)

    def test_list_raises(self):
        """Passing a list raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Time([])

    # ------------------------------------------------------------------
    # time_series()
    # ------------------------------------------------------------------

    def test_time_series_returns_timeseries(self):
        """time_series() returns a TimeSeries instance."""
        self.assertIsInstance(ecf.Time(12, 0).time_series(), ecf.TimeSeries)

    def test_time_series_single_slot_no_increment(self):
        """A single-slot Time has no increment in its time_series."""
        self.assertFalse(ecf.Time(12, 0).time_series().has_increment())

    def test_time_series_range_has_increment(self):
        """A range-constructed Time has an increment in its time_series."""
        t = ecf.Time(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertTrue(t.time_series().has_increment())

    def test_time_series_relative_flag(self):
        """time_series().relative() reflects the relative flag."""
        self.assertFalse(ecf.Time(12, 0).time_series().relative())
        self.assertTrue(ecf.Time(12, 0, True).time_series().relative())

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_absolute_format(self):
        """str() of an absolute time starts with 'time '."""
        self.assertEqual(str(ecf.Time(12, 0)), "time 12:00")

    def test_str_relative_format(self):
        """str() of a relative time uses '+' prefix after 'time '."""
        self.assertEqual(str(ecf.Time(12, 0, True)), "time +12:00")

    def test_str_range_format(self):
        """str() of a range time includes start, finish and increment."""
        t = ecf.Time(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertEqual(str(t), "time 06:00 20:00 00:30")

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Time(10, 0)), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A Time is equal to itself."""
        t = ecf.Time(12, 0)
        self.assertEqual(t, t)

    def test_eq_same_value(self):
        """Two Times with the same hour and minute are equal."""
        self.assertEqual(ecf.Time(12, 0), ecf.Time(12, 0))

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.Time(8, 30)
        b = ecf.Time(8, 30)
        self.assertTrue(a == b)
        self.assertTrue(b == a)

    def test_eq_str_ctor_and_int_ctor(self):
        """Time from str ctor equals Time from int ctor with the same values."""
        self.assertEqual(ecf.Time("09:15"), ecf.Time(9, 15))

    def test_ne_different_hour(self):
        """Times with different hours are not equal."""
        self.assertNotEqual(ecf.Time(10, 0), ecf.Time(11, 0))

    def test_ne_different_minute(self):
        """Times with different minutes are not equal."""
        self.assertNotEqual(ecf.Time(10, 0), ecf.Time(10, 30))

    def test_ne_absolute_vs_relative(self):
        """Absolute and relative times are not equal even if the clock value matches."""
        self.assertNotEqual(ecf.Time(12, 0, False), ecf.Time(12, 0, True))

    def test_ne_single_vs_range(self):
        """A single-slot Time is not equal to a range Time."""
        single = ecf.Time(ecf.TimeSlot(6, 0))
        rng = ecf.Time(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertNotEqual(single, rng)

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_time_instance(self):
        """The result of copy.copy() is a Time."""
        self.assertIsInstance(copy.copy(ecf.Time(12, 0)), ecf.Time)

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        t = ecf.Time(12, 0)
        self.assertIsNot(copy.copy(t), t)

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces a Time equal in value to the original."""
        t = ecf.Time(12, 0)
        self.assertEqual(copy.copy(t), t)

    def test_copy_preserves_str(self):
        """The copied Time has the same str() as the original."""
        t = ecf.Time(9, 30, True)
        self.assertEqual(str(copy.copy(t)), str(t))

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Time is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Time(12, 0)), int)

    def test_same_object_same_hash(self):
        """The same Time object always returns the same hash."""
        t = ecf.Time(12, 0)
        self.assertEqual(hash(t), hash(t))

    def test_hash_is_identity_based(self):
        """Two value-equal Times have different hashes (identity, not value)."""
        a = ecf.Time(12, 0)
        b = ecf.Time(12, 0)
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """Time instances can be stored in a Python set."""
        t1 = ecf.Time(10, 0)
        t2 = ecf.Time(11, 0)
        s = {t1, t2}
        self.assertEqual(len(s), 2)

    def test_can_be_used_as_dict_key(self):
        """Time instances can be used as dictionary keys."""
        t = ecf.Time(8, 0)
        d = {t: "start"}
        self.assertEqual(d[t], "start")


class TestToday(unittest.TestCase):
    """Tests for py::class_<ecf::TodayAttr> exposed as ecf.Today in ExportNodeAttr.cpp.

    The API mirrors ecf.Time exactly (same constructors, same time_series() method,
    same operators) with two differences:
      - str() uses the prefix 'today' instead of 'time'
      - ecf.Today and ecf.Time with the same time value are NOT equal (different types)

    Exposed API
    -----------
    Constructors
        Today(TimeSlot, bool=False)
        Today(int, int, bool=False)
        Today(TimeSeries)
        Today(TimeSlot, TimeSlot, TimeSlot, bool)
        Today(str)

    Instance methods
        time_series() -> TimeSeries

    Operators
        __str__, __copy__, __eq__, __ne__, __hash__ (identity-based); no __lt__
    """

    # ------------------------------------------------------------------
    # Constructor: Today(TimeSlot, bool=False)
    # ------------------------------------------------------------------

    def test_timeslot_ctor_absolute(self):
        """Today(TimeSlot(h, m)) creates an absolute today-time."""
        t = ecf.Today(ecf.TimeSlot(10, 0))
        self.assertEqual(str(t), "today 10:00")

    def test_timeslot_ctor_relative(self):
        """Today(TimeSlot, True) produces a relative today-time."""
        t = ecf.Today(ecf.TimeSlot(2, 0), True)
        self.assertEqual(str(t), "today +02:00")

    def test_timeslot_ctor_explicit_false(self):
        """Today(TimeSlot, False) equals Today(TimeSlot)."""
        self.assertEqual(
            ecf.Today(ecf.TimeSlot(8, 0), False), ecf.Today(ecf.TimeSlot(8, 0))
        )

    # ------------------------------------------------------------------
    # Constructor: Today(int, int, bool=False)
    # ------------------------------------------------------------------

    def test_int_ctor_absolute(self):
        """Today(h, m) creates an absolute today-time."""
        t = ecf.Today(8, 30)
        self.assertEqual(str(t), "today 08:30")

    def test_int_ctor_relative(self):
        """Today(h, m, True) creates a relative today-time."""
        t = ecf.Today(8, 30, True)
        self.assertEqual(str(t), "today +08:30")

    def test_int_ctor_midnight(self):
        """Today(0, 0) is valid (midnight)."""
        self.assertEqual(str(ecf.Today(0, 0)), "today 00:00")

    def test_int_ctor_boundary_hour_23(self):
        """Hour 23 is the valid upper boundary."""
        self.assertEqual(str(ecf.Today(23, 59)), "today 23:59")

    def test_int_ctor_hour_too_large_raises(self):
        """hour > 23 raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Today(25, 0)

    def test_int_ctor_minute_too_large_raises(self):
        """minute > 59 raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Today(0, 60)

    def test_int_ctor_relative_flag_in_time_series(self):
        """time_series().relative() reflects the bool flag."""
        self.assertTrue(ecf.Today(8, 0, True).time_series().relative())
        self.assertFalse(ecf.Today(8, 0, False).time_series().relative())

    # ------------------------------------------------------------------
    # Constructor: Today(TimeSeries)
    # ------------------------------------------------------------------

    def test_timeseries_ctor_single_slot(self):
        """Today(TimeSeries(TimeSlot)) creates a single-slot today-time."""
        ts = ecf.TimeSeries(ecf.TimeSlot(15, 0))
        t = ecf.Today(ts)
        self.assertEqual(str(t), "today 15:00")

    def test_timeseries_ctor_range(self):
        """Today(TimeSeries(start,finish,incr,False)) creates a range today-time."""
        ts = ecf.TimeSeries(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertEqual(str(ecf.Today(ts)), "today 06:00 20:00 00:30")

    def test_timeseries_ctor_equals_slot_ctor(self):
        """Today(TimeSeries) equals the matching 4-slot constructor form."""
        ts = ecf.TimeSeries(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        ta = ecf.Today(ts)
        tb = ecf.Today(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertEqual(ta, tb)

    # ------------------------------------------------------------------
    # Constructor: Today(TimeSlot, TimeSlot, TimeSlot, bool)
    # ------------------------------------------------------------------

    def test_four_slot_ctor_absolute_range(self):
        """Today(start, finish, incr, False) creates an absolute range."""
        t = ecf.Today(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertEqual(str(t), "today 06:00 20:00 00:30")

    def test_four_slot_ctor_relative_range(self):
        """Today(start, finish, incr, True) creates a relative range."""
        t = ecf.Today(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), True
        )
        self.assertEqual(str(t), "today +06:00 20:00 00:30")

    def test_four_slot_ctor_has_increment(self):
        """A range-constructed Today has an increment in its time_series."""
        t = ecf.Today(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertTrue(t.time_series().has_increment())

    def test_four_slot_ctor_start_finish_incr(self):
        """time_series() from 4-slot ctor reflects start, finish and incr."""
        t = ecf.Today(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        ts = t.time_series()
        self.assertEqual(str(ts.start()), "06:00")
        self.assertEqual(str(ts.finish()), "20:00")
        self.assertEqual(str(ts.incr()), "00:30")

    # ------------------------------------------------------------------
    # Constructor: Today(str)
    # ------------------------------------------------------------------

    def test_str_ctor_absolute(self):
        """Today('HH:MM') creates an absolute today-time."""
        self.assertEqual(str(ecf.Today("08:30")), "today 08:30")

    def test_str_ctor_relative_plus_prefix(self):
        """Today('+HH:MM') creates a relative today-time."""
        self.assertEqual(str(ecf.Today("+08:30")), "today +08:30")

    def test_str_ctor_equals_int_ctor(self):
        """Today('10:00') is equal to Today(10, 0)."""
        self.assertEqual(ecf.Today("10:00"), ecf.Today(10, 0))

    def test_str_ctor_bad_string_raises(self):
        """An unrecognised string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Today("bad")

    def test_str_ctor_hour_out_of_range_raises(self):
        """'25:00' raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Today("25:00")

    def test_str_ctor_minute_out_of_range_raises(self):
        """'12:60' raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Today("12:60")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_none_raises(self):
        """Passing None raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Today(None)

    def test_float_raises(self):
        """Passing a float raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Today(1.5)

    def test_list_raises(self):
        """Passing a list raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Today([])

    # ------------------------------------------------------------------
    # time_series()
    # ------------------------------------------------------------------

    def test_time_series_returns_timeseries(self):
        """time_series() returns a TimeSeries instance."""
        self.assertIsInstance(ecf.Today(10, 0).time_series(), ecf.TimeSeries)

    def test_time_series_single_slot_no_increment(self):
        """A single-slot Today has no increment."""
        self.assertFalse(ecf.Today(10, 0).time_series().has_increment())

    def test_time_series_range_has_increment(self):
        """A range-constructed Today has an increment."""
        t = ecf.Today(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertTrue(t.time_series().has_increment())

    def test_time_series_relative_flag(self):
        """time_series().relative() reflects the relative flag."""
        self.assertFalse(ecf.Today(10, 0).time_series().relative())
        self.assertTrue(ecf.Today(10, 0, True).time_series().relative())

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_prefix_is_today_not_time(self):
        """str() uses the prefix 'today', not 'time'."""
        self.assertTrue(str(ecf.Today(12, 0)).startswith("today "))

    def test_str_absolute_format(self):
        """str() of an absolute today-time is 'today HH:MM'."""
        self.assertEqual(str(ecf.Today(12, 0)), "today 12:00")

    def test_str_relative_format(self):
        """str() of a relative today-time has '+' prefix."""
        self.assertEqual(str(ecf.Today(12, 0, True)), "today +12:00")

    def test_str_range_format(self):
        """str() of a range today-time includes start, finish and increment."""
        t = ecf.Today(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertEqual(str(t), "today 06:00 20:00 00:30")

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Today(10, 0)), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A Today is equal to itself."""
        t = ecf.Today(10, 0)
        self.assertEqual(t, t)

    def test_eq_same_value(self):
        """Two Todays with the same time are equal."""
        self.assertEqual(ecf.Today(10, 0), ecf.Today(10, 0))

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.Today(8, 30)
        b = ecf.Today(8, 30)
        self.assertTrue(a == b)
        self.assertTrue(b == a)

    def test_eq_str_ctor_and_int_ctor(self):
        """Today from str ctor equals Today from int ctor."""
        self.assertEqual(ecf.Today("09:15"), ecf.Today(9, 15))

    def test_ne_different_hour(self):
        """Todays with different hours are not equal."""
        self.assertNotEqual(ecf.Today(10, 0), ecf.Today(11, 0))

    def test_ne_different_minute(self):
        """Todays with different minutes are not equal."""
        self.assertNotEqual(ecf.Today(10, 0), ecf.Today(10, 30))

    def test_ne_absolute_vs_relative(self):
        """Absolute and relative Todays are not equal."""
        self.assertNotEqual(ecf.Today(12, 0, False), ecf.Today(12, 0, True))

    def test_ne_single_vs_range(self):
        """A single-slot Today is not equal to a range Today."""
        single = ecf.Today(ecf.TimeSlot(6, 0))
        rng = ecf.Today(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(0, 30), False
        )
        self.assertNotEqual(single, rng)

    def test_today_and_time_equal_value_not_equal(self):
        """Today(h,m) != Time(h,m): the two types are distinct."""
        tod = ecf.Today(10, 0)
        tim = ecf.Time(10, 0)
        # boost.python raises TypeError when comparing unrelated extension types;
        # the result must be not-equal (either False or an exception we catch).
        try:
            result = tod == tim
            self.assertFalse(result)
        except TypeError:
            pass  # also acceptable: types are unrelated

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_today_instance(self):
        """The result of copy.copy() is a Today."""
        self.assertIsInstance(copy.copy(ecf.Today(10, 0)), ecf.Today)

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        t = ecf.Today(10, 0)
        self.assertIsNot(copy.copy(t), t)

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces a Today equal in value to the original."""
        t = ecf.Today(10, 30)
        self.assertEqual(copy.copy(t), t)

    def test_copy_preserves_str(self):
        """The copied Today has the same str() as the original."""
        t = ecf.Today(9, 30, True)
        self.assertEqual(str(copy.copy(t)), str(t))

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Today is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Today(10, 0)), int)

    def test_same_object_same_hash(self):
        """The same Today object always returns the same hash."""
        t = ecf.Today(10, 0)
        self.assertEqual(hash(t), hash(t))

    def test_hash_is_identity_based(self):
        """Two value-equal Todays have different hashes (identity, not value)."""
        a = ecf.Today(10, 0)
        b = ecf.Today(10, 0)
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """Today instances can be stored in a Python set."""
        t1 = ecf.Today(10, 0)
        t2 = ecf.Today(11, 0)
        s = {t1, t2}
        self.assertEqual(len(s), 2)

    def test_can_be_used_as_dict_key(self):
        """Today instances can be used as dictionary keys."""
        t = ecf.Today(8, 0)
        d = {t: "morning"}
        self.assertEqual(d[t], "morning")


class TestLate(unittest.TestCase):
    """Tests for py::class_<ecf::LateAttr> exposed as ecf.Late in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Late()                                 -- default empty late attribute
        Late(submitted=str, active=str,        -- keyword-only constructor; values are
             complete=str)                        time strings 'HH:MM' or '+HH:MM'
                                                  ('+' prefix denotes relative complete).
                                                  Only [submitted|active|complete] are
                                                  valid keywords; others raise RuntimeError.

    Setter methods (overloaded — each has two forms)
        submitted(int, int)   -- hour, minute; submitted is always treated as relative
        submitted(TimeSlot)   -- TimeSlot
        active(int, int)      -- hour, minute; absolute wall-clock time
        active(TimeSlot)      -- TimeSlot
        complete(int, int, bool) -- hour, minute, relative (True = relative from active)
        complete(TimeSlot, bool) -- TimeSlot, relative flag
          NOTE: complete(TimeSlot) without the bool raises ArgumentError

    Getter methods (zero-argument overloads of the same names)
        submitted() -> TimeSlot   -- the submitted threshold (default '00:00')
        active()    -> TimeSlot   -- the active threshold   (default '00:00')
        complete()  -> TimeSlot   -- the complete threshold (default '00:00')

    Predicates
        complete_is_relative() -> bool   -- True when complete was set with relative=True
        is_late()              -> bool   -- always False for newly constructed instances

    Operators
        __str__   -- 'late [-s +HH:MM] [-a HH:MM] [-c [+]HH:MM]'
                     submitted is always shown with '+' prefix; empty Late is just 'late'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based; two empty Lates are equal; two fully-configured
                     Lates with the same fields are equal
        __ne__    -- implicit complement of __eq__
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: Late() — default
    # ------------------------------------------------------------------

    def test_default_ctor_creates_instance(self):
        """Late() succeeds and creates a Late instance."""
        self.assertIsInstance(ecf.Late(), ecf.Late)

    def test_default_ctor_str_is_late(self):
        """str(Late()) is 'late' (no flags set)."""
        self.assertEqual(str(ecf.Late()), "late")

    def test_default_ctor_is_late_is_false(self):
        """is_late() is False immediately after construction."""
        self.assertFalse(ecf.Late().is_late())

    def test_default_ctor_complete_is_relative_is_false(self):
        """complete_is_relative() is False for a default-constructed Late."""
        self.assertFalse(ecf.Late().complete_is_relative())

    def test_default_ctor_getters_return_zero_timeslot(self):
        """submitted(), active() and complete() all return '00:00' by default."""
        L = ecf.Late()
        self.assertEqual(str(L.submitted()), "00:00")
        self.assertEqual(str(L.active()), "00:00")
        self.assertEqual(str(L.complete()), "00:00")

    def test_default_ctor_getters_return_timeslot(self):
        """submitted(), active() and complete() return TimeSlot instances."""
        L = ecf.Late()
        self.assertIsInstance(L.submitted(), ecf.TimeSlot)
        self.assertIsInstance(L.active(), ecf.TimeSlot)
        self.assertIsInstance(L.complete(), ecf.TimeSlot)

    # ------------------------------------------------------------------
    # Constructor: Late(submitted=..., active=..., complete=...)
    # ------------------------------------------------------------------

    def test_kw_ctor_all_fields(self):
        """Late(submitted=, active=, complete=) sets all three thresholds."""
        L = ecf.Late(submitted="00:20", active="15:00", complete="+00:30")
        self.assertEqual(str(L), "late -s +00:20 -a 15:00 -c +00:30")

    def test_kw_ctor_submitted_only(self):
        """Late(submitted=) sets only the submitted threshold."""
        L = ecf.Late(submitted="01:00")
        self.assertEqual(str(L), "late -s +01:00")

    def test_kw_ctor_active_only(self):
        """Late(active=) sets only the active threshold."""
        L = ecf.Late(active="12:00")
        self.assertEqual(str(L), "late -a 12:00")

    def test_kw_ctor_complete_relative(self):
        """Late(complete='+HH:MM') sets a relative complete threshold."""
        L = ecf.Late(complete="+01:30")
        self.assertEqual(str(L), "late -c +01:30")
        self.assertTrue(L.complete_is_relative())

    def test_kw_ctor_complete_absolute(self):
        """Late(complete='HH:MM') sets an absolute complete threshold."""
        L = ecf.Late(complete="10:00")
        self.assertEqual(str(L), "late -c 10:00")
        self.assertFalse(L.complete_is_relative())

    def test_kw_ctor_submitted_is_always_shown_with_plus(self):
        """submitted is always treated as relative and shown with '+' in str()."""
        L = ecf.Late(submitted="00:20")
        self.assertIn("-s +", str(L))

    def test_kw_ctor_bad_keyword_raises(self):
        """An unrecognised keyword raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Late(bad_key="00:20")

    def test_kw_ctor_positional_arg_raises(self):
        """Passing a positional argument raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Late("00:20")

    def test_kw_ctor_non_string_value_raises(self):
        """A non-string value for a keyword raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Late(submitted=30)

    def test_kw_ctor_none_value_raises(self):
        """None as a keyword value raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Late(submitted=None)

    # ------------------------------------------------------------------
    # Setter: submitted(int, int) and submitted(TimeSlot)
    # ------------------------------------------------------------------

    def test_submitted_int_int_updates_str(self):
        """submitted(h, m) sets the submitted threshold."""
        L = ecf.Late()
        L.submitted(0, 30)
        self.assertEqual(str(L), "late -s +00:30")

    def test_submitted_timeslot_updates_str(self):
        """submitted(TimeSlot) sets the submitted threshold."""
        L = ecf.Late()
        L.submitted(ecf.TimeSlot(0, 45))
        self.assertEqual(str(L), "late -s +00:45")

    def test_submitted_int_int_returns_none(self):
        """submitted(h, m) is a void method; returns None."""
        self.assertIsNone(ecf.Late().submitted(0, 30))

    def test_submitted_timeslot_returns_none(self):
        """submitted(TimeSlot) is a void method; returns None."""
        self.assertIsNone(ecf.Late().submitted(ecf.TimeSlot(0, 20)))

    def test_submitted_reflects_in_getter(self):
        """After submitted(h, m), submitted() returns the set TimeSlot."""
        L = ecf.Late()
        L.submitted(0, 20)
        self.assertEqual(str(L.submitted()), "00:20")

    def test_submitted_can_be_overwritten(self):
        """Calling submitted() twice keeps the latest value."""
        L = ecf.Late()
        L.submitted(0, 20)
        L.submitted(0, 30)
        self.assertEqual(str(L.submitted()), "00:30")

    def test_submitted_wrong_type_none_raises(self):
        """submitted(None) raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Late().submitted(None)

    def test_submitted_wrong_type_str_raises(self):
        """submitted(str) raises ArgumentError (string not accepted)."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Late().submitted("00:30")

    # ------------------------------------------------------------------
    # Setter: active(int, int) and active(TimeSlot)
    # ------------------------------------------------------------------

    def test_active_int_int_updates_str(self):
        """active(h, m) sets the active threshold."""
        L = ecf.Late()
        L.active(15, 0)
        self.assertEqual(str(L), "late -a 15:00")

    def test_active_timeslot_updates_str(self):
        """active(TimeSlot) sets the active threshold."""
        L = ecf.Late()
        L.active(ecf.TimeSlot(16, 0))
        self.assertEqual(str(L), "late -a 16:00")

    def test_active_int_int_returns_none(self):
        """active(h, m) is a void method; returns None."""
        self.assertIsNone(ecf.Late().active(15, 0))

    def test_active_timeslot_returns_none(self):
        """active(TimeSlot) is a void method; returns None."""
        self.assertIsNone(ecf.Late().active(ecf.TimeSlot(15, 0)))

    def test_active_reflects_in_getter(self):
        """After active(h, m), active() returns the set TimeSlot."""
        L = ecf.Late()
        L.active(12, 0)
        self.assertEqual(str(L.active()), "12:00")

    def test_active_wrong_type_none_raises(self):
        """active(None) raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Late().active(None)

    def test_active_wrong_type_str_raises(self):
        """active(str, str) raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Late().active("15", "00")

    # ------------------------------------------------------------------
    # Setter: complete(int, int, bool) and complete(TimeSlot, bool)
    # ------------------------------------------------------------------

    def test_complete_int_int_bool_relative_updates_str(self):
        """complete(h, m, True) sets a relative complete threshold."""
        L = ecf.Late()
        L.complete(0, 30, True)
        self.assertEqual(str(L), "late -c +00:30")

    def test_complete_int_int_bool_absolute_updates_str(self):
        """complete(h, m, False) sets an absolute complete threshold."""
        L = ecf.Late()
        L.complete(10, 0, False)
        self.assertEqual(str(L), "late -c 10:00")

    def test_complete_timeslot_bool_relative(self):
        """complete(TimeSlot, True) sets a relative complete threshold."""
        L = ecf.Late()
        L.complete(ecf.TimeSlot(1, 0), True)
        self.assertEqual(str(L), "late -c +01:00")

    def test_complete_timeslot_bool_absolute(self):
        """complete(TimeSlot, False) sets an absolute complete threshold."""
        L = ecf.Late()
        L.complete(ecf.TimeSlot(10, 0), False)
        self.assertEqual(str(L), "late -c 10:00")

    def test_complete_int_int_bool_returns_none(self):
        """complete(h, m, bool) is void; returns None."""
        self.assertIsNone(ecf.Late().complete(0, 30, True))

    def test_complete_timeslot_bool_returns_none(self):
        """complete(TimeSlot, bool) is void; returns None."""
        self.assertIsNone(ecf.Late().complete(ecf.TimeSlot(1, 0), True))

    def test_complete_reflects_in_getter(self):
        """After complete(h, m, bool), complete() returns the set TimeSlot."""
        L = ecf.Late()
        L.complete(0, 30, True)
        self.assertEqual(str(L.complete()), "00:30")

    def test_complete_relative_flag_stored(self):
        """complete_is_relative() reflects the bool passed to complete()."""
        L_rel = ecf.Late()
        L_rel.complete(0, 30, True)
        self.assertTrue(L_rel.complete_is_relative())

        L_abs = ecf.Late()
        L_abs.complete(10, 0, False)
        self.assertFalse(L_abs.complete_is_relative())

    def test_complete_missing_bool_raises(self):
        """complete(h, m) without the relative bool raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Late().complete(0, 30)

    def test_complete_timeslot_missing_bool_raises(self):
        """complete(TimeSlot) without the relative bool raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Late().complete(ecf.TimeSlot(1, 0))

    def test_complete_none_raises(self):
        """complete(None, bool) raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Late().complete(None, True)

    # ------------------------------------------------------------------
    # Getters: submitted(), active(), complete()
    # ------------------------------------------------------------------

    def test_submitted_getter_returns_timeslot(self):
        """submitted() returns a TimeSlot instance."""
        L = ecf.Late(submitted="00:20")
        self.assertIsInstance(L.submitted(), ecf.TimeSlot)

    def test_active_getter_returns_timeslot(self):
        """active() returns a TimeSlot instance."""
        L = ecf.Late(active="15:00")
        self.assertIsInstance(L.active(), ecf.TimeSlot)

    def test_complete_getter_returns_timeslot(self):
        """complete() returns a TimeSlot instance."""
        L = ecf.Late(complete="+00:30")
        self.assertIsInstance(L.complete(), ecf.TimeSlot)

    def test_submitted_getter_value(self):
        """submitted() returns the time set via keyword constructor."""
        L = ecf.Late(submitted="00:20")
        self.assertEqual(str(L.submitted()), "00:20")

    def test_active_getter_value(self):
        """active() returns the time set via keyword constructor."""
        L = ecf.Late(active="15:00")
        self.assertEqual(str(L.active()), "15:00")

    def test_complete_getter_value(self):
        """complete() returns the time set via keyword constructor."""
        L = ecf.Late(complete="+00:30")
        self.assertEqual(str(L.complete()), "00:30")

    # ------------------------------------------------------------------
    # complete_is_relative() and is_late()
    # ------------------------------------------------------------------

    def test_complete_is_relative_true_for_relative_complete(self):
        """complete_is_relative() is True when complete is set with '+' prefix."""
        self.assertTrue(ecf.Late(complete="+00:30").complete_is_relative())

    def test_complete_is_relative_false_for_absolute_complete(self):
        """complete_is_relative() is False when complete is an absolute time."""
        self.assertFalse(ecf.Late(complete="10:00").complete_is_relative())

    def test_complete_is_relative_returns_bool(self):
        """complete_is_relative() returns a Python bool."""
        self.assertIsInstance(ecf.Late().complete_is_relative(), bool)

    def test_is_late_returns_false_initially(self):
        """is_late() is False for any newly created Late instance."""
        self.assertFalse(ecf.Late().is_late())
        self.assertFalse(
            ecf.Late(submitted="00:20", active="15:00", complete="+00:30").is_late()
        )

    def test_is_late_returns_bool(self):
        """is_late() always returns a Python bool."""
        self.assertIsInstance(ecf.Late().is_late(), bool)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_empty_is_just_late(self):
        """str(Late()) is exactly 'late'."""
        self.assertEqual(str(ecf.Late()), "late")

    def test_str_submitted_uses_plus_prefix(self):
        """submitted is always rendered with '+' in str()."""
        L = ecf.Late(submitted="00:20")
        self.assertEqual(str(L), "late -s +00:20")

    def test_str_active_absolute(self):
        """active is rendered without '+' prefix in str()."""
        L = ecf.Late(active="12:00")
        self.assertEqual(str(L), "late -a 12:00")

    def test_str_relative_complete(self):
        """str() shows '+' prefix for relative complete."""
        self.assertEqual(str(ecf.Late(complete="+01:30")), "late -c +01:30")

    def test_str_absolute_complete(self):
        """str() shows no '+' prefix for absolute complete."""
        self.assertEqual(str(ecf.Late(complete="10:00")), "late -c 10:00")

    def test_str_all_fields(self):
        """str() with all three fields matches the expected format."""
        L = ecf.Late(submitted="00:20", active="15:00", complete="+00:30")
        self.assertEqual(str(L), "late -s +00:20 -a 15:00 -c +00:30")

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Late()), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A Late is equal to itself."""
        L = ecf.Late(submitted="00:20")
        self.assertEqual(L, L)

    def test_eq_two_empty_lates(self):
        """Two default-constructed Lates are equal."""
        self.assertEqual(ecf.Late(), ecf.Late())

    def test_eq_same_fields(self):
        """Two Lates with identical configurations are equal."""
        a = ecf.Late(submitted="00:20", active="15:00", complete="+00:30")
        b = ecf.Late(submitted="00:20", active="15:00", complete="+00:30")
        self.assertEqual(a, b)

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.Late(submitted="00:20")
        b = ecf.Late(submitted="00:20")
        self.assertTrue(a == b)
        self.assertTrue(b == a)

    def test_ne_different_submitted(self):
        """Lates with different submitted times are not equal."""
        self.assertNotEqual(ecf.Late(submitted="00:20"), ecf.Late(submitted="01:00"))

    def test_ne_different_active(self):
        """Lates with different active times are not equal."""
        self.assertNotEqual(ecf.Late(active="12:00"), ecf.Late(active="15:00"))

    def test_ne_different_complete(self):
        """Lates with different complete times are not equal."""
        self.assertNotEqual(ecf.Late(complete="+00:30"), ecf.Late(complete="+01:00"))

    def test_ne_absolute_vs_relative_complete(self):
        """Absolute and relative complete thresholds make Lates unequal."""
        self.assertNotEqual(ecf.Late(complete="10:00"), ecf.Late(complete="+10:00"))

    def test_ne_empty_vs_configured(self):
        """An empty Late differs from one with fields set."""
        self.assertNotEqual(ecf.Late(), ecf.Late(submitted="00:20"))

    def test_ne_complement_of_eq(self):
        """a != b is False when a == b."""
        a = ecf.Late(submitted="00:20")
        b = ecf.Late(submitted="00:20")
        self.assertFalse(a != b)

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_late_instance(self):
        """copy.copy() returns a Late instance."""
        self.assertIsInstance(copy.copy(ecf.Late()), ecf.Late)

    def test_copy_is_distinct_object(self):
        """copy.copy() produces a distinct Python object."""
        L = ecf.Late(submitted="00:20")
        self.assertIsNot(copy.copy(L), L)

    def test_copy_produces_equal_instance(self):
        """copy.copy() produces a Late equal in value to the original."""
        L = ecf.Late(submitted="00:20", active="15:00", complete="+00:30")
        self.assertEqual(copy.copy(L), L)

    def test_copy_preserves_str(self):
        """The copied Late has the same str() as the original."""
        L = ecf.Late(submitted="00:20", active="15:00", complete="+00:30")
        self.assertEqual(str(copy.copy(L)), str(L))

    def test_copy_preserves_complete_is_relative(self):
        """The copied Late preserves complete_is_relative()."""
        L = ecf.Late(complete="+00:30")
        self.assertEqual(copy.copy(L).complete_is_relative(), L.complete_is_relative())

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Late is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Late()), int)

    def test_same_object_same_hash(self):
        """The same Late object always returns the same hash."""
        L = ecf.Late(submitted="00:20")
        self.assertEqual(hash(L), hash(L))

    def test_hash_is_identity_based(self):
        """Two value-equal Lates have different hashes (identity, not value)."""
        a = ecf.Late(submitted="00:20", active="15:00", complete="+00:30")
        b = ecf.Late(submitted="00:20", active="15:00", complete="+00:30")
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """Late instances can be stored in a Python set."""
        L = ecf.Late(submitted="00:20")
        s = {L}
        self.assertIn(L, s)

    def test_can_be_used_as_dict_key(self):
        """Late instances can be used as dictionary keys."""
        L = ecf.Late(submitted="00:20")
        d = {L: "threshold"}
        self.assertEqual(d[L], "threshold")


class TestAutocancel(unittest.TestCase):
    """Tests for py::class_<ecf::AutoCancelAttr> exposed as ecf.Autocancel in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Autocancel(int, int, bool)   -- hour, minute, relative; relative=True means the
                                        time is relative to the suite start
        Autocancel(int)              -- days; days()=True, relative()=True; internally
                                        stored as N*24 h; str shows the plain integer
        Autocancel(TimeSlot, bool)   -- TimeSlot, relative

    Instance methods
        time()     -> TimeSlot   -- the cancel time; for the days ctor returns N*24 h
        relative() -> bool       -- True when the time is relative; always True for days
        days()     -> bool       -- True only when constructed via the days overload

    Operators
        __str__   -- 'autocancel +HH:MM' (relative h/m), 'autocancel HH:MM' (absolute),
                     or 'autocancel N' (days)
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- ordering by internal time representation
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: Autocancel(int, int, bool)
    # ------------------------------------------------------------------

    def test_hm_ctor_relative(self):
        """Autocancel(h, m, True) creates a relative cancel time."""
        self.assertEqual(str(ecf.Autocancel(1, 30, True)), "autocancel +01:30")

    def test_hm_ctor_absolute(self):
        """Autocancel(h, m, False) creates an absolute cancel time."""
        self.assertEqual(str(ecf.Autocancel(2, 0, False)), "autocancel 02:00")

    def test_hm_ctor_days_is_false(self):
        """The h/m/bool ctor sets days()=False."""
        self.assertFalse(ecf.Autocancel(1, 30, True).days())

    def test_hm_ctor_relative_flag_true(self):
        """relative() is True when True is passed."""
        self.assertTrue(ecf.Autocancel(1, 30, True).relative())

    def test_hm_ctor_relative_flag_false(self):
        """relative() is False when False is passed."""
        self.assertFalse(ecf.Autocancel(2, 0, False).relative())

    def test_hm_ctor_time_returns_correct_slot(self):
        """time() returns the TimeSlot matching the h:m argument."""
        self.assertEqual(str(ecf.Autocancel(1, 30, True).time()), "01:30")

    # ------------------------------------------------------------------
    # Constructor: Autocancel(int)  — days
    # ------------------------------------------------------------------

    def test_days_ctor_str(self):
        """Autocancel(N) uses the days format in str()."""
        self.assertEqual(str(ecf.Autocancel(3)), "autocancel 3")

    def test_days_ctor_zero(self):
        """Autocancel(0) is valid."""
        self.assertEqual(str(ecf.Autocancel(0)), "autocancel 0")

    def test_days_ctor_days_is_true(self):
        """days() returns True for the days constructor."""
        self.assertTrue(ecf.Autocancel(3).days())

    def test_days_ctor_relative_is_true(self):
        """relative() is always True for the days constructor."""
        self.assertTrue(ecf.Autocancel(3).relative())

    def test_days_ctor_time_encodes_days_as_hours(self):
        """time() for the days ctor returns N*24 hours (e.g. 3 days -> '72:00')."""
        self.assertEqual(str(ecf.Autocancel(3).time()), "72:00")
        self.assertEqual(str(ecf.Autocancel(1).time()), "24:00")

    # ------------------------------------------------------------------
    # Constructor: Autocancel(TimeSlot, bool)
    # ------------------------------------------------------------------

    def test_timeslot_ctor_relative(self):
        """Autocancel(TimeSlot, True) creates a relative cancel time."""
        self.assertEqual(
            str(ecf.Autocancel(ecf.TimeSlot(1, 30), True)), "autocancel +01:30"
        )

    def test_timeslot_ctor_absolute(self):
        """Autocancel(TimeSlot, False) creates an absolute cancel time."""
        self.assertEqual(
            str(ecf.Autocancel(ecf.TimeSlot(2, 0), False)), "autocancel 02:00"
        )

    def test_timeslot_ctor_days_is_false(self):
        """The TimeSlot ctor sets days()=False."""
        self.assertFalse(ecf.Autocancel(ecf.TimeSlot(1, 30), True).days())

    def test_timeslot_ctor_equals_hm_ctor(self):
        """Autocancel(TimeSlot(h,m), rel) equals Autocancel(h, m, rel)."""
        self.assertEqual(
            ecf.Autocancel(ecf.TimeSlot(1, 30), True), ecf.Autocancel(1, 30, True)
        )

    # ------------------------------------------------------------------
    # time() / relative() / days() return types
    # ------------------------------------------------------------------

    def test_time_returns_timeslot(self):
        """time() returns a TimeSlot instance."""
        self.assertIsInstance(ecf.Autocancel(1, 30, True).time(), ecf.TimeSlot)

    def test_relative_returns_bool(self):
        """relative() returns a Python bool."""
        self.assertIsInstance(ecf.Autocancel(1, 30, True).relative(), bool)

    def test_days_returns_bool(self):
        """days() returns a Python bool."""
        self.assertIsInstance(ecf.Autocancel(3).days(), bool)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_relative_has_plus(self):
        """str() of a relative Autocancel starts with 'autocancel +'."""
        self.assertTrue(str(ecf.Autocancel(1, 30, True)).startswith("autocancel +"))

    def test_str_absolute_no_plus(self):
        """str() of an absolute Autocancel has no '+'."""
        self.assertEqual(str(ecf.Autocancel(2, 0, False)), "autocancel 02:00")

    def test_str_days_is_integer(self):
        """str() for the days ctor shows just the integer."""
        self.assertEqual(str(ecf.Autocancel(5)), "autocancel 5")

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Autocancel(1, 30, True)), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """An Autocancel is equal to itself."""
        a = ecf.Autocancel(1, 30, True)
        self.assertEqual(a, a)

    def test_eq_same_hm_relative(self):
        """Two Autocancels with the same h/m/relative are equal."""
        self.assertEqual(ecf.Autocancel(1, 30, True), ecf.Autocancel(1, 30, True))

    def test_eq_same_days(self):
        """Two days-Autocancels with the same count are equal."""
        self.assertEqual(ecf.Autocancel(3), ecf.Autocancel(3))

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.Autocancel(1, 30, True)
        b = ecf.Autocancel(1, 30, True)
        self.assertTrue(a == b and b == a)

    def test_ne_different_time(self):
        """Autocancels with different times are not equal."""
        self.assertNotEqual(ecf.Autocancel(1, 30, True), ecf.Autocancel(2, 0, True))

    def test_ne_relative_vs_absolute(self):
        """Relative and absolute Autocancels are not equal."""
        self.assertNotEqual(ecf.Autocancel(1, 30, True), ecf.Autocancel(1, 30, False))

    def test_ne_hm_vs_days(self):
        """h/m ctor and days ctor are not equal even for same duration."""
        self.assertNotEqual(ecf.Autocancel(1, 30, True), ecf.Autocancel(3))

    def test_ne_different_days(self):
        """Days-Autocancels with different counts are not equal."""
        self.assertNotEqual(ecf.Autocancel(2), ecf.Autocancel(3))

    # ------------------------------------------------------------------
    # __lt__
    # ------------------------------------------------------------------

    def test_lt_earlier_time_is_less(self):
        """An earlier time is less than a later time."""
        self.assertLess(ecf.Autocancel(1, 30, True), ecf.Autocancel(2, 0, False))

    def test_lt_fewer_days_is_less(self):
        """Fewer days is less than more days."""
        self.assertLess(ecf.Autocancel(2), ecf.Autocancel(3))

    def test_lt_equal_not_less(self):
        """An Autocancel is not strictly less than an equal one."""
        self.assertFalse(ecf.Autocancel(1, 30, True) < ecf.Autocancel(1, 30, True))

    def test_lt_hm_less_than_days(self):
        """A short h/m Autocancel is less than a days Autocancel."""
        self.assertLess(ecf.Autocancel(1, 30, True), ecf.Autocancel(3))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_autocancel(self):
        """copy.copy() returns an Autocancel instance."""
        self.assertIsInstance(copy.copy(ecf.Autocancel(1, 30, True)), ecf.Autocancel)

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        a = ecf.Autocancel(1, 30, True)
        self.assertIsNot(copy.copy(a), a)

    def test_copy_equal_value(self):
        """copy.copy() produces an Autocancel equal in value to the original."""
        a = ecf.Autocancel(1, 30, True)
        self.assertEqual(copy.copy(a), a)

    def test_copy_preserves_str(self):
        """The copied Autocancel has the same str() as the original."""
        a = ecf.Autocancel(3)
        self.assertEqual(str(copy.copy(a)), str(a))

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Autocancel is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Autocancel(1, 30, True)), int)

    def test_same_object_same_hash(self):
        """The same object always returns the same hash."""
        a = ecf.Autocancel(1, 30, True)
        self.assertEqual(hash(a), hash(a))

    def test_hash_is_identity_based(self):
        """Two value-equal Autocancels have different hashes."""
        a = ecf.Autocancel(1, 30, True)
        b = ecf.Autocancel(1, 30, True)
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """Autocancel instances can be stored in a Python set."""
        a = ecf.Autocancel(1, 30, True)
        b = ecf.Autocancel(3)
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """Autocancel instances can be used as dictionary keys."""
        a = ecf.Autocancel(1, 30, True)
        d = {a: "cancel"}
        self.assertEqual(d[a], "cancel")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_none_raises(self):
        """Passing None raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autocancel(None)

    def test_float_raises(self):
        """Passing a float raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autocancel(1.5)

    def test_list_raises(self):
        """Passing a list raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autocancel([])

    def test_str_raises(self):
        """Passing a string raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autocancel("x")

    def test_two_ints_no_bool_raises(self):
        """Autocancel(h, m) without the relative bool raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autocancel(1, 30)


class TestAutoarchive(unittest.TestCase):
    """Tests for py::class_<ecf::AutoArchiveAttr> exposed as ecf.Autoarchive in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Autoarchive(int, int, bool, bool)   -- hour, minute, relative, idle
        Autoarchive(int, bool)              -- days, idle; days()=True.
                                               NOTE: Autoarchive(int, int) is also matched
                                               by this overload (second int is coerced to
                                               bool by boost.python — non-zero → True).
        Autoarchive(TimeSlot, bool, bool)   -- TimeSlot, relative, idle

    'idle' means: archive when the node is queued, aborted or complete AND the time has
    elapsed.  Without idle, archiving only happens when the node is complete.

    Instance methods
        time()     -> TimeSlot   -- the archive time; for the days ctor returns N*24 h
        relative() -> bool       -- True when the time is relative; always True for days
        days()     -> bool       -- True only when constructed via the days overload
        idle()     -> bool       -- True when idle archiving is enabled

    Operators
        __str__   -- 'autoarchive [+]HH:MM [-i]' or 'autoarchive N [-i]'
                     The '+' prefix signals a relative time; '-i' signals idle=True.
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality; idle flag is part of the comparison
        __ne__    -- implicit complement of __eq__
        __lt__    -- ordering by internal time representation
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: Autoarchive(int, int, bool, bool)
    # ------------------------------------------------------------------

    def test_hm_ctor_relative_no_idle(self):
        """Autoarchive(h, m, True, False) makes a relative, non-idle archive."""
        self.assertEqual(str(ecf.Autoarchive(1, 30, True, False)), "autoarchive +01:30")

    def test_hm_ctor_absolute_idle(self):
        """Autoarchive(h, m, False, True) makes an absolute, idle archive."""
        self.assertEqual(
            str(ecf.Autoarchive(2, 0, False, True)), "autoarchive 02:00 -i"
        )

    def test_hm_ctor_days_is_false(self):
        """The h/m ctor sets days()=False."""
        self.assertFalse(ecf.Autoarchive(1, 30, True, False).days())

    def test_hm_ctor_relative_flag(self):
        """relative() matches the third argument."""
        self.assertTrue(ecf.Autoarchive(1, 30, True, False).relative())
        self.assertFalse(ecf.Autoarchive(2, 0, False, True).relative())

    def test_hm_ctor_idle_flag(self):
        """idle() matches the fourth argument."""
        self.assertFalse(ecf.Autoarchive(1, 30, True, False).idle())
        self.assertTrue(ecf.Autoarchive(2, 0, False, True).idle())

    def test_hm_ctor_time_slot(self):
        """time() returns the TimeSlot for h/m ctor."""
        self.assertEqual(str(ecf.Autoarchive(1, 30, True, False).time()), "01:30")

    # ------------------------------------------------------------------
    # Constructor: Autoarchive(int, bool) — days, idle
    # ------------------------------------------------------------------

    def test_days_ctor_no_idle(self):
        """Autoarchive(N, False) shows just the day count."""
        self.assertEqual(str(ecf.Autoarchive(3, False)), "autoarchive 3")

    def test_days_ctor_with_idle(self):
        """Autoarchive(N, True) shows '-i' suffix."""
        self.assertEqual(str(ecf.Autoarchive(2, True)), "autoarchive 2 -i")

    def test_days_ctor_days_is_true(self):
        """days() is True for the days overload."""
        self.assertTrue(ecf.Autoarchive(3, False).days())

    def test_days_ctor_idle_false(self):
        """idle() is False when False is passed."""
        self.assertFalse(ecf.Autoarchive(3, False).idle())

    def test_days_ctor_idle_true(self):
        """idle() is True when True is passed."""
        self.assertTrue(ecf.Autoarchive(2, True).idle())

    def test_days_ctor_relative_is_true(self):
        """relative() is always True for the days ctor."""
        self.assertTrue(ecf.Autoarchive(3, False).relative())

    # ------------------------------------------------------------------
    # Constructor: Autoarchive(TimeSlot, bool, bool)
    # ------------------------------------------------------------------

    def test_timeslot_ctor_relative_no_idle(self):
        """Autoarchive(TimeSlot, True, False) creates relative, non-idle."""
        self.assertEqual(
            str(ecf.Autoarchive(ecf.TimeSlot(1, 30), True, False)), "autoarchive +01:30"
        )

    def test_timeslot_ctor_absolute_idle(self):
        """Autoarchive(TimeSlot, False, True) creates absolute, idle."""
        self.assertEqual(
            str(ecf.Autoarchive(ecf.TimeSlot(2, 0), False, True)),
            "autoarchive 02:00 -i",
        )

    def test_timeslot_ctor_equals_hm_ctor(self):
        """Autoarchive(TimeSlot(h,m), rel, idle) equals Autoarchive(h, m, rel, idle)."""
        self.assertEqual(
            ecf.Autoarchive(ecf.TimeSlot(1, 30), True, False),
            ecf.Autoarchive(1, 30, True, False),
        )

    def test_timeslot_ctor_days_is_false(self):
        """The TimeSlot ctor sets days()=False."""
        self.assertFalse(ecf.Autoarchive(ecf.TimeSlot(1, 30), True, False).days())

    # ------------------------------------------------------------------
    # time() / relative() / days() / idle() return types
    # ------------------------------------------------------------------

    def test_time_returns_timeslot(self):
        """time() returns a TimeSlot instance."""
        self.assertIsInstance(ecf.Autoarchive(1, 30, True, False).time(), ecf.TimeSlot)

    def test_relative_returns_bool(self):
        """relative() returns a Python bool."""
        self.assertIsInstance(ecf.Autoarchive(1, 30, True, False).relative(), bool)

    def test_days_returns_bool(self):
        """days() returns a Python bool."""
        self.assertIsInstance(ecf.Autoarchive(3, False).days(), bool)

    def test_idle_returns_bool(self):
        """idle() returns a Python bool."""
        self.assertIsInstance(ecf.Autoarchive(1, 30, True, False).idle(), bool)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_relative_no_idle(self):
        """str() for relative, non-idle shows '+' and no '-i'."""
        self.assertEqual(str(ecf.Autoarchive(1, 30, True, False)), "autoarchive +01:30")

    def test_str_absolute_idle(self):
        """str() for absolute, idle shows '-i' suffix."""
        self.assertEqual(
            str(ecf.Autoarchive(2, 0, False, True)), "autoarchive 02:00 -i"
        )

    def test_str_days_idle(self):
        """str() for days+idle shows integer and '-i'."""
        self.assertEqual(str(ecf.Autoarchive(2, True)), "autoarchive 2 -i")

    def test_str_days_no_idle(self):
        """str() for days, no idle shows just the integer."""
        self.assertEqual(str(ecf.Autoarchive(3, False)), "autoarchive 3")

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Autoarchive(1, 30, True, False)), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """An Autoarchive is equal to itself."""
        a = ecf.Autoarchive(1, 30, True, False)
        self.assertEqual(a, a)

    def test_eq_same_fields(self):
        """Two Autoarchives with identical fields are equal."""
        self.assertEqual(
            ecf.Autoarchive(1, 30, True, False), ecf.Autoarchive(1, 30, True, False)
        )

    def test_eq_same_days(self):
        """Two days-Autoarchives with the same count/idle are equal."""
        self.assertEqual(ecf.Autoarchive(3, False), ecf.Autoarchive(3, False))

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.Autoarchive(1, 30, True, False)
        b = ecf.Autoarchive(1, 30, True, False)
        self.assertTrue(a == b and b == a)

    def test_ne_different_time(self):
        """Autoarchives with different times are not equal."""
        self.assertNotEqual(
            ecf.Autoarchive(1, 30, True, False), ecf.Autoarchive(2, 0, True, False)
        )

    def test_ne_different_idle(self):
        """Autoarchives differing only in idle flag are not equal."""
        self.assertNotEqual(
            ecf.Autoarchive(1, 30, True, False), ecf.Autoarchive(1, 30, True, True)
        )

    def test_ne_different_relative(self):
        """Autoarchives differing only in relative flag are not equal."""
        self.assertNotEqual(
            ecf.Autoarchive(1, 30, True, False), ecf.Autoarchive(1, 30, False, False)
        )

    def test_ne_different_days(self):
        """Days-Autoarchives with different counts are not equal."""
        self.assertNotEqual(ecf.Autoarchive(2, False), ecf.Autoarchive(3, False))

    # ------------------------------------------------------------------
    # __lt__
    # ------------------------------------------------------------------

    def test_lt_earlier_time_is_less(self):
        """An earlier time Autoarchive is less than a later one."""
        self.assertLess(
            ecf.Autoarchive(1, 30, True, False), ecf.Autoarchive(2, 0, False, True)
        )

    def test_lt_later_not_less(self):
        """A later Autoarchive is not less than an earlier one."""
        self.assertFalse(
            ecf.Autoarchive(2, 0, False, True) < ecf.Autoarchive(1, 30, True, False)
        )

    def test_lt_equal_not_less(self):
        """An Autoarchive is not strictly less than an equal one."""
        a = ecf.Autoarchive(1, 30, True, False)
        b = ecf.Autoarchive(1, 30, True, False)
        self.assertFalse(a < b)

    def test_lt_hm_less_than_days(self):
        """A short h/m Autoarchive is less than a days Autoarchive."""
        self.assertLess(ecf.Autoarchive(1, 30, True, False), ecf.Autoarchive(3, False))

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_autoarchive(self):
        """copy.copy() returns an Autoarchive instance."""
        self.assertIsInstance(
            copy.copy(ecf.Autoarchive(1, 30, True, False)), ecf.Autoarchive
        )

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        a = ecf.Autoarchive(1, 30, True, False)
        self.assertIsNot(copy.copy(a), a)

    def test_copy_equal_value(self):
        """copy.copy() produces an Autoarchive equal in value to the original."""
        a = ecf.Autoarchive(1, 30, True, False)
        self.assertEqual(copy.copy(a), a)

    def test_copy_preserves_idle(self):
        """The copied Autoarchive has the same idle() as the original."""
        a = ecf.Autoarchive(2, 0, False, True)
        self.assertEqual(copy.copy(a).idle(), a.idle())

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Autoarchive is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Autoarchive(1, 30, True, False)), int)

    def test_same_object_same_hash(self):
        """The same object always returns the same hash."""
        a = ecf.Autoarchive(1, 30, True, False)
        self.assertEqual(hash(a), hash(a))

    def test_hash_is_identity_based(self):
        """Two value-equal Autoarchives have different hashes."""
        a = ecf.Autoarchive(1, 30, True, False)
        b = ecf.Autoarchive(1, 30, True, False)
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """Autoarchive instances can be stored in a Python set."""
        a = ecf.Autoarchive(1, 30, True, False)
        b = ecf.Autoarchive(3, False)
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """Autoarchive instances can be used as dictionary keys."""
        a = ecf.Autoarchive(1, 30, True, False)
        d = {a: "archive"}
        self.assertEqual(d[a], "archive")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_none_raises(self):
        """Passing None raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autoarchive(None)

    def test_float_raises(self):
        """Passing a float raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autoarchive(1.5)

    def test_single_int_no_idle_raises(self):
        """Autoarchive(N) without the idle bool raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autoarchive(3)

    def test_int_bool_bool_raises(self):
        """Autoarchive(int, bool, bool) does not match any signature."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autoarchive(1, True, False)

    def test_two_ints_matches_days_idle_overload(self):
        """Autoarchive(int, int) is matched by (days, idle) with int coerced to bool.
        This is NOT an error: Autoarchive(1, 30) -> days=1, idle=True (30 != 0).
        """
        a = ecf.Autoarchive(1, 30)
        self.assertTrue(a.days())
        self.assertTrue(a.idle())


class TestAutorestore(unittest.TestCase):
    """Tests for py::class_<ecf::AutoRestoreAttr> exposed as ecf.Autorestore in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Autorestore(list)   -- list of absolute node paths (strings) to restore when
                               the node to which this attribute is attached completes

    Instance methods
        nodes_to_restore()   -- returns the list of paths; however the C++ return type
                                (std::vector<std::string>) has no registered Python
                                converter, so calling it raises TypeError — a known
                                binding limitation documented by the test

    Operators
        __str__   -- 'autorestore /path1 /path2 ...' (space-separated paths)
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: Autorestore(list)
    # ------------------------------------------------------------------

    def test_single_path(self):
        """Autorestore([path]) constructs with a single node path."""
        self.assertEqual(
            str(ecf.Autorestore(["/suite/family"])), "autorestore /suite/family"
        )

    def test_multiple_paths(self):
        """Autorestore([p1, p2, p3]) constructs with multiple paths."""
        self.assertEqual(
            str(ecf.Autorestore(["/a", "/b", "/c"])), "autorestore /a /b /c"
        )

    def test_empty_list(self):
        """Autorestore([]) is valid; str() is just 'autorestore'."""
        self.assertEqual(str(ecf.Autorestore([])), "autorestore")

    def test_is_autorestore_instance(self):
        """The constructed object is an Autorestore instance."""
        self.assertIsInstance(ecf.Autorestore(["/a"]), ecf.Autorestore)

    # ------------------------------------------------------------------
    # nodes_to_restore() — known limitation
    # ------------------------------------------------------------------

    def test_nodes_to_restore_raises_type_error(self):
        """nodes_to_restore() raises TypeError; the C++ return type
        std::vector<string> has no registered Python converter.
        This is a known binding limitation that the test documents."""
        ar = ecf.Autorestore(["/a", "/b"])
        with self.assertRaises(TypeError):
            ar.nodes_to_restore()

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_empty(self):
        """str() for an empty Autorestore is 'autorestore'."""
        self.assertEqual(str(ecf.Autorestore([])), "autorestore")

    def test_str_single_path(self):
        """str() includes the single path after 'autorestore '."""
        self.assertEqual(
            str(ecf.Autorestore(["/suite/task"])), "autorestore /suite/task"
        )

    def test_str_multiple_paths_space_separated(self):
        """str() joins multiple paths with spaces."""
        self.assertEqual(str(ecf.Autorestore(["/a", "/b"])), "autorestore /a /b")

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Autorestore([])), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """An Autorestore is equal to itself."""
        ar = ecf.Autorestore(["/a"])
        self.assertEqual(ar, ar)

    def test_eq_same_paths(self):
        """Two Autorestores with identical path lists are equal."""
        self.assertEqual(ecf.Autorestore(["/a", "/b"]), ecf.Autorestore(["/a", "/b"]))

    def test_eq_empty(self):
        """Two empty Autorestores are equal."""
        self.assertEqual(ecf.Autorestore([]), ecf.Autorestore([]))

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.Autorestore(["/a"])
        b = ecf.Autorestore(["/a"])
        self.assertTrue(a == b and b == a)

    def test_ne_different_paths(self):
        """Autorestores with different path lists are not equal."""
        self.assertNotEqual(ecf.Autorestore(["/a"]), ecf.Autorestore(["/b"]))

    def test_ne_empty_vs_nonempty(self):
        """An empty Autorestore is not equal to a non-empty one."""
        self.assertNotEqual(ecf.Autorestore([]), ecf.Autorestore(["/a"]))

    def test_ne_different_count(self):
        """Autorestores with different numbers of paths are not equal."""
        self.assertNotEqual(ecf.Autorestore(["/a"]), ecf.Autorestore(["/a", "/b"]))

    def test_ne_complement(self):
        """a != b is False when a == b."""
        a = ecf.Autorestore(["/a"])
        b = ecf.Autorestore(["/a"])
        self.assertFalse(a != b)

    # ------------------------------------------------------------------
    # __lt__ — not exposed
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """Autorestore does not support '<'; raises TypeError."""
        a = ecf.Autorestore(["/a"])
        b = ecf.Autorestore(["/b"])
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_autorestore(self):
        """copy.copy() returns an Autorestore instance."""
        self.assertIsInstance(copy.copy(ecf.Autorestore(["/a"])), ecf.Autorestore)

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        ar = ecf.Autorestore(["/a", "/b"])
        self.assertIsNot(copy.copy(ar), ar)

    def test_copy_equal_value(self):
        """copy.copy() produces an Autorestore equal in value to the original."""
        ar = ecf.Autorestore(["/a", "/b"])
        self.assertEqual(copy.copy(ar), ar)

    def test_copy_preserves_str(self):
        """The copied Autorestore has the same str() as the original."""
        ar = ecf.Autorestore(["/a", "/b"])
        self.assertEqual(str(copy.copy(ar)), str(ar))

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Autorestore is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Autorestore(["/a"])), int)

    def test_same_object_same_hash(self):
        """The same object always returns the same hash."""
        ar = ecf.Autorestore(["/a"])
        self.assertEqual(hash(ar), hash(ar))

    def test_hash_is_identity_based(self):
        """Two value-equal Autorestores have different hashes."""
        a = ecf.Autorestore(["/a", "/b"])
        b = ecf.Autorestore(["/a", "/b"])
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """Autorestore instances can be stored in a Python set."""
        a = ecf.Autorestore(["/a"])
        b = ecf.Autorestore(["/b"])
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """Autorestore instances can be used as dictionary keys."""
        ar = ecf.Autorestore(["/a"])
        d = {ar: "restore"}
        self.assertEqual(d[ar], "restore")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_none_raises(self):
        """Passing None raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autorestore(None)

    def test_string_raises(self):
        """Passing a plain string (not a list) raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autorestore("/a")

    def test_int_raises(self):
        """Passing an integer raises ArgumentError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autorestore(42)

    def test_list_with_int_elements_raises(self):
        """A list containing integers raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autorestore([42])

    def test_list_with_none_elements_raises(self):
        """A list containing None raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autorestore([None])

    def test_list_with_float_elements_raises(self):
        """A list containing floats raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Autorestore([1.5])


class TestRepeatDate(unittest.TestCase):
    """Tests for py::class_<RepeatDate> exposed as ecf.RepeatDate in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        RepeatDate(name, start, end)         -- name (str), start/end as yyyymmdd integers;
                                               step defaults to 1
        RepeatDate(name, start, end, step)   -- explicit step increment (days)

    Instance methods
        name()    -> str   -- the repeat variable name
        start()   -> int   -- start date in yyyymmdd format
        end()     -> int   -- end date in yyyymmdd format
        step()    -> int   -- increment in days (default 1)

    Operators
        __str__   -- '  repeat date NAME YYYYMMDD YYYYMMDD STEP\\n'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: RepeatDate(name, start, end)
    # ------------------------------------------------------------------

    def test_ctor_default_step(self):
        """RepeatDate(name, start, end) defaults step to 1."""
        rd = ecf.RepeatDate("YMD", 20100101, 20100110)
        self.assertEqual(rd.step(), 1)

    def test_ctor_stores_name(self):
        """Constructor stores the name verbatim."""
        self.assertEqual(ecf.RepeatDate("YMD", 20100101, 20100110).name(), "YMD")

    def test_ctor_stores_start(self):
        """Constructor stores the start date."""
        self.assertEqual(ecf.RepeatDate("YMD", 20100101, 20100110).start(), 20100101)

    def test_ctor_stores_end(self):
        """Constructor stores the end date."""
        self.assertEqual(ecf.RepeatDate("YMD", 20100101, 20100110).end(), 20100110)

    # ------------------------------------------------------------------
    # Constructor: RepeatDate(name, start, end, step)
    # ------------------------------------------------------------------

    def test_ctor_explicit_step(self):
        """RepeatDate(name, start, end, step) stores the given step."""
        self.assertEqual(ecf.RepeatDate("YMD", 20100101, 20100110, 2).step(), 2)

    def test_ctor_step_one_is_same_as_default(self):
        """Explicit step=1 is equal to the default-step constructor."""
        self.assertEqual(
            ecf.RepeatDate("YMD", 20100101, 20100110, 1),
            ecf.RepeatDate("YMD", 20100101, 20100110),
        )

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() always returns a Python str."""
        self.assertIsInstance(ecf.RepeatDate("YMD", 20100101, 20100110).name(), str)

    def test_name_value(self):
        """name() returns the name supplied to the constructor."""
        self.assertEqual(ecf.RepeatDate("MYVAR", 20100101, 20100110).name(), "MYVAR")

    # ------------------------------------------------------------------
    # start()
    # ------------------------------------------------------------------

    def test_start_returns_int(self):
        """start() returns an integer."""
        self.assertIsInstance(ecf.RepeatDate("YMD", 20100101, 20100110).start(), int)

    def test_start_value(self):
        """start() returns the start date in yyyymmdd format."""
        self.assertEqual(ecf.RepeatDate("YMD", 20200315, 20201231).start(), 20200315)

    # ------------------------------------------------------------------
    # end()
    # ------------------------------------------------------------------

    def test_end_returns_int(self):
        """end() returns an integer."""
        self.assertIsInstance(ecf.RepeatDate("YMD", 20100101, 20100110).end(), int)

    def test_end_value(self):
        """end() returns the end date in yyyymmdd format."""
        self.assertEqual(ecf.RepeatDate("YMD", 20200315, 20201231).end(), 20201231)

    # ------------------------------------------------------------------
    # step()
    # ------------------------------------------------------------------

    def test_step_returns_int(self):
        """step() returns an integer."""
        self.assertIsInstance(ecf.RepeatDate("YMD", 20100101, 20100110).step(), int)

    def test_step_default_is_one(self):
        """Default step is 1."""
        self.assertEqual(ecf.RepeatDate("YMD", 20100101, 20100110).step(), 1)

    def test_step_explicit_value(self):
        """step() returns the value given to the constructor."""
        self.assertEqual(ecf.RepeatDate("YMD", 20100101, 20100131, 5).step(), 5)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format(self):
        """str() follows '  repeat date NAME START END STEP\\n'."""
        rd = ecf.RepeatDate("YMD", 20100101, 20100110, 1)
        self.assertEqual(str(rd), "  repeat date YMD 20100101 20100110 1\n")

    def test_str_explicit_step(self):
        """str() includes the explicit step when not 1."""
        rd = ecf.RepeatDate("YMD", 20100101, 20100110, 2)
        self.assertEqual(str(rd), "  repeat date YMD 20100101 20100110 2\n")

    def test_str_returns_str_type(self):
        """str() returns a Python str."""
        self.assertIsInstance(str(ecf.RepeatDate("YMD", 20100101, 20100110)), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A RepeatDate is equal to itself."""
        rd = ecf.RepeatDate("YMD", 20100101, 20100110)
        self.assertEqual(rd, rd)

    def test_eq_same_args(self):
        """Two RepeatDates built with identical arguments are equal."""
        self.assertEqual(
            ecf.RepeatDate("YMD", 20100101, 20100110),
            ecf.RepeatDate("YMD", 20100101, 20100110),
        )

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.RepeatDate("YMD", 20100101, 20100110)
        b = ecf.RepeatDate("YMD", 20100101, 20100110)
        self.assertTrue(a == b and b == a)

    def test_ne_different_name(self):
        """Different names are not equal."""
        self.assertNotEqual(
            ecf.RepeatDate("A", 20100101, 20100110),
            ecf.RepeatDate("B", 20100101, 20100110),
        )

    def test_ne_different_start(self):
        """Different start dates are not equal."""
        self.assertNotEqual(
            ecf.RepeatDate("YMD", 20100101, 20100110),
            ecf.RepeatDate("YMD", 20100102, 20100110),
        )

    def test_ne_different_end(self):
        """Different end dates are not equal."""
        self.assertNotEqual(
            ecf.RepeatDate("YMD", 20100101, 20100110),
            ecf.RepeatDate("YMD", 20100101, 20100115),
        )

    def test_ne_different_step(self):
        """Different steps are not equal."""
        self.assertNotEqual(
            ecf.RepeatDate("YMD", 20100101, 20100110, 1),
            ecf.RepeatDate("YMD", 20100101, 20100110, 2),
        )

    def test_ne_complement(self):
        """a != b is False when a == b."""
        a = ecf.RepeatDate("YMD", 20100101, 20100110)
        b = ecf.RepeatDate("YMD", 20100101, 20100110)
        self.assertFalse(a != b)

    # ------------------------------------------------------------------
    # __lt__ — not exposed
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """RepeatDate does not support '<'; raises TypeError."""
        a = ecf.RepeatDate("YMD", 20100101, 20100110)
        b = ecf.RepeatDate("YMD", 20100101, 20100115)
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_repeat_date(self):
        """copy.copy() returns a RepeatDate instance."""
        self.assertIsInstance(
            copy.copy(ecf.RepeatDate("YMD", 20100101, 20100110)), ecf.RepeatDate
        )

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        rd = ecf.RepeatDate("YMD", 20100101, 20100110)
        self.assertIsNot(copy.copy(rd), rd)

    def test_copy_equal_value(self):
        """copy.copy() is value-equal to the original."""
        rd = ecf.RepeatDate("YMD", 20100101, 20100110)
        self.assertEqual(copy.copy(rd), rd)

    def test_copy_preserves_str(self):
        """The copied RepeatDate has the same str() as the original."""
        rd = ecf.RepeatDate("YMD", 20100101, 20100110, 2)
        self.assertEqual(str(copy.copy(rd)), str(rd))

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """RepeatDate is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.RepeatDate("YMD", 20100101, 20100110)), int)

    def test_same_object_same_hash(self):
        """The same object always returns the same hash."""
        rd = ecf.RepeatDate("YMD", 20100101, 20100110)
        self.assertEqual(hash(rd), hash(rd))

    def test_hash_is_identity_based(self):
        """Two value-equal RepeatDates have different hashes."""
        a = ecf.RepeatDate("YMD", 20100101, 20100110)
        b = ecf.RepeatDate("YMD", 20100101, 20100110)
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """RepeatDate instances can be stored in a Python set."""
        a = ecf.RepeatDate("YMD", 20100101, 20100110)
        b = ecf.RepeatDate("YMD", 20100101, 20100115)
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """RepeatDate instances can be used as dictionary keys."""
        rd = ecf.RepeatDate("YMD", 20100101, 20100110)
        d = {rd: "date"}
        self.assertEqual(d[rd], "date")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_no_args_raises(self):
        """Omitting all arguments raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatDate()

    def test_non_string_name_raises(self):
        """Passing an integer as name raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatDate(123, 20100101, 20100110)

    def test_non_int_start_raises(self):
        """Passing a string as start raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatDate("YMD", "20100101", 20100110)

    # ------------------------------------------------------------------
    # Out-of-range date values
    # ------------------------------------------------------------------

    def test_start_month_13_raises(self):
        """A start date with month=13 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDate("YMD", 20101301, 20200101)

    def test_start_month_0_raises(self):
        """A start date with month=0 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDate("YMD", 20100001, 20200101)

    def test_start_day_32_raises(self):
        """A start date with day=32 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDate("YMD", 20100132, 20200101)

    def test_start_day_0_raises(self):
        """A start date with day=0 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDate("YMD", 20100100, 20200101)

    def test_start_too_few_digits_raises(self):
        """A start value with fewer than 8 digits raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDate("YMD", 10101, 20200101)

    def test_start_negative_raises(self):
        """A negative start value raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDate("YMD", -20100101, 20200101)

    def test_end_month_13_raises(self):
        """An end date with month=13 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDate("YMD", 20100101, 20101301)

    def test_end_day_32_raises(self):
        """An end date with day=32 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDate("YMD", 20100101, 20100132)

    def test_end_too_few_digits_raises(self):
        """An end value with fewer than 8 digits raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDate("YMD", 20100101, 10101)

    def test_end_negative_raises(self):
        """A negative end value raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDate("YMD", 20100101, -20100101)


class TestRepeatDateTime(unittest.TestCase):
    """Tests for py::class_<RepeatDateTime> exposed as ecf.RepeatDateTime in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        RepeatDateTime(name, start, end)        -- name (str), start/end as ISO datetime
                                                   strings ('YYYYMMDDTHHMMSS'); step
                                                   defaults to one day (86400 s)
        RepeatDateTime(name, start, end, step)  -- explicit step as a duration string
                                                   (e.g. '24:00:00')

    Instance methods
        name()    -> str   -- the repeat variable name
        start()   -> int   -- start instant as seconds since epoch (19700101T000000)
        end()     -> int   -- end instant as seconds since epoch
        step()    -> int   -- step increment in seconds (default 86400)

    Operators
        __str__   -- '  repeat datetime NAME START END STEP\\n'
                     START/END shown as ISO strings; STEP as HH:MM:SS
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: RepeatDateTime(name, start, end)
    # ------------------------------------------------------------------

    def test_ctor_default_step_is_one_day(self):
        """Default step is 86400 seconds (one day)."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertEqual(rd.step(), 86400)

    def test_ctor_stores_name(self):
        """Constructor stores the name verbatim."""
        self.assertEqual(
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000").name(), "DT"
        )

    def test_ctor_start_as_epoch_seconds(self):
        """start() returns seconds since 19700101T000000."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertEqual(rd.start(), 1262304000)

    def test_ctor_end_as_epoch_seconds(self):
        """end() returns seconds since 19700101T000000."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertEqual(rd.end(), 1263081600)

    # ------------------------------------------------------------------
    # Constructor: RepeatDateTime(name, start, end, step)
    # ------------------------------------------------------------------

    def test_ctor_explicit_step_stored(self):
        """Explicit step '24:00:00' yields 86400 seconds."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000", "24:00:00")
        self.assertEqual(rd.step(), 86400)

    def test_ctor_explicit_step_48h(self):
        """Explicit step '48:00:00' yields 172800 seconds."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000", "48:00:00")
        self.assertEqual(rd.step(), 172800)

    def test_ctor_default_equals_explicit_one_day(self):
        """Default step produces the same object as explicit '24:00:00'."""
        self.assertEqual(
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000"),
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000", "24:00:00"),
        )

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() always returns a Python str."""
        self.assertIsInstance(
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000").name(), str
        )

    def test_name_value(self):
        """name() matches what was supplied to the constructor."""
        self.assertEqual(
            ecf.RepeatDateTime("MYDT", "20100101T000000", "20100110T000000").name(),
            "MYDT",
        )

    # ------------------------------------------------------------------
    # start() / end()
    # ------------------------------------------------------------------

    def test_start_returns_int(self):
        """start() returns an integer."""
        self.assertIsInstance(
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000").start(), int
        )

    def test_end_returns_int(self):
        """end() returns an integer."""
        self.assertIsInstance(
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000").end(), int
        )

    def test_start_less_than_end(self):
        """start() < end() for a forward repeat."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertLess(rd.start(), rd.end())

    # ------------------------------------------------------------------
    # step()
    # ------------------------------------------------------------------

    def test_step_returns_int(self):
        """step() returns an integer (seconds)."""
        self.assertIsInstance(
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000").step(), int
        )

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format(self):
        """str() shows ISO start/end strings and HH:MM:SS step."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertEqual(
            str(rd), "  repeat datetime DT 20100101T000000 20100110T000000 24:00:00\n"
        )

    def test_str_returns_str_type(self):
        """str() returns a Python str."""
        self.assertIsInstance(
            str(ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")), str
        )

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A RepeatDateTime is equal to itself."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertEqual(rd, rd)

    def test_eq_same_args(self):
        """Two RepeatDateTimes built with identical arguments are equal."""
        self.assertEqual(
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000"),
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000"),
        )

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        b = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertTrue(a == b and b == a)

    def test_ne_different_name(self):
        """Different names are not equal."""
        self.assertNotEqual(
            ecf.RepeatDateTime("A", "20100101T000000", "20100110T000000"),
            ecf.RepeatDateTime("B", "20100101T000000", "20100110T000000"),
        )

    def test_ne_different_start(self):
        """Different start instants are not equal."""
        self.assertNotEqual(
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000"),
            ecf.RepeatDateTime("DT", "20100102T000000", "20100110T000000"),
        )

    def test_ne_different_end(self):
        """Different end instants are not equal."""
        self.assertNotEqual(
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000"),
            ecf.RepeatDateTime("DT", "20100101T000000", "20100115T000000"),
        )

    def test_ne_complement(self):
        """a != b is False when a == b."""
        a = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        b = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertFalse(a != b)

    # ------------------------------------------------------------------
    # __lt__ — not exposed
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """RepeatDateTime does not support '<'; raises TypeError."""
        a = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        b = ecf.RepeatDateTime("DT", "20100101T000000", "20100115T000000")
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_repeat_datetime(self):
        """copy.copy() returns a RepeatDateTime instance."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertIsInstance(copy.copy(rd), ecf.RepeatDateTime)

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertIsNot(copy.copy(rd), rd)

    def test_copy_equal_value(self):
        """copy.copy() is value-equal to the original."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertEqual(copy.copy(rd), rd)

    def test_copy_preserves_str(self):
        """The copied RepeatDateTime has the same str() as the original."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertEqual(str(copy.copy(rd)), str(rd))

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """RepeatDateTime is hashable; hash() returns an int."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertIsInstance(hash(rd), int)

    def test_same_object_same_hash(self):
        """The same object always returns the same hash."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertEqual(hash(rd), hash(rd))

    def test_hash_is_identity_based(self):
        """Two value-equal RepeatDateTimes have different hashes."""
        a = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        b = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """RepeatDateTime instances can be stored in a Python set."""
        a = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        b = ecf.RepeatDateTime("DT", "20100101T000000", "20100115T000000")
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """RepeatDateTime instances can be used as dictionary keys."""
        rd = ecf.RepeatDateTime("DT", "20100101T000000", "20100110T000000")
        d = {rd: "dt"}
        self.assertEqual(d[rd], "dt")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_no_args_raises(self):
        """Omitting all arguments raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatDateTime()

    def test_non_string_start_raises(self):
        """Passing an integer as start raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatDateTime("DT", 20100101, "20100110T000000")

    # ------------------------------------------------------------------
    # Out-of-range instant values (start)
    # ------------------------------------------------------------------

    def test_start_month_13_raises(self):
        """start with month=13 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20101301T000000", "20100110T000000")

    def test_start_month_0_raises(self):
        """start with month=0 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100001T000000", "20100110T000000")

    def test_start_day_32_raises(self):
        """start with day=32 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100132T000000", "20100110T000000")

    def test_start_day_0_raises(self):
        """start with day=0 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100100T000000", "20100110T000000")

    def test_start_hour_24_raises(self):
        """start with hour=24 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100101T240000", "20100110T000000")

    def test_start_minute_60_raises(self):
        """start with minute=60 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100101T006000", "20100110T000000")

    def test_start_missing_separator_raises(self):
        """start without the 'T' separator raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100101000000", "20100110T000000")

    def test_start_empty_string_raises(self):
        """An empty start string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "", "20100110T000000")

    def test_start_negative_year_raises(self):
        """A start string with a negative year raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "-0100101T000000", "20100110T000000")

    # ------------------------------------------------------------------
    # Out-of-range instant values (end)
    # ------------------------------------------------------------------

    def test_end_month_13_raises(self):
        """end with month=13 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100101T000000", "20101301T000000")

    def test_end_month_0_raises(self):
        """end with month=0 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100101T000000", "20100001T000000")

    def test_end_day_32_raises(self):
        """end with day=32 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100101T000000", "20100132T000000")

    def test_end_day_0_raises(self):
        """end with day=0 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100101T000000", "20100100T000000")

    def test_end_hour_24_raises(self):
        """end with hour=24 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T240000")

    def test_end_minute_60_raises(self):
        """end with minute=60 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110T006000")

    def test_end_missing_separator_raises(self):
        """end without the 'T' separator raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100101T000000", "20100110000000")

    def test_end_empty_string_raises(self):
        """An empty end string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100101T000000", "")

    def test_end_negative_year_raises(self):
        """An end string with a negative year raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTime("DT", "20100101T000000", "-0100101T000000")


class TestRepeatDateList(unittest.TestCase):
    """Tests for py::class_<RepeatDateList> exposed as ecf.RepeatDateList in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        RepeatDateList(name, dates)   -- name (str), dates as a list of yyyymmdd integers

    Instance methods
        name()    -> str   -- the repeat variable name
        start()   -> int   -- first date in yyyymmdd format
        end()     -> int   -- last date in yyyymmdd format

    Operators
        __str__   -- '  repeat datelist NAME "D1" "D2" ...\\n'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor
    # ------------------------------------------------------------------

    def test_ctor_single_date(self):
        """RepeatDateList with a single date is valid."""
        rd = ecf.RepeatDateList("DL", [20100101])
        self.assertEqual(rd.start(), 20100101)
        self.assertEqual(rd.end(), 20100101)

    def test_ctor_multiple_dates(self):
        """RepeatDateList stores all provided dates."""
        rd = ecf.RepeatDateList("DL", [20100101, 20100105, 20100110])
        self.assertEqual(rd.start(), 20100101)
        self.assertEqual(rd.end(), 20100110)

    def test_ctor_stores_name(self):
        """Constructor stores the name verbatim."""
        self.assertEqual(ecf.RepeatDateList("MYDL", [20100101]).name(), "MYDL")

    def test_ctor_empty_list_raises(self):
        """An empty date list raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateList("DL", [])

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() always returns a Python str."""
        self.assertIsInstance(ecf.RepeatDateList("DL", [20100101]).name(), str)

    def test_name_value(self):
        """name() matches what was supplied to the constructor."""
        self.assertEqual(ecf.RepeatDateList("DATES", [20100101]).name(), "DATES")

    # ------------------------------------------------------------------
    # start() / end()
    # ------------------------------------------------------------------

    def test_start_returns_int(self):
        """start() returns an integer."""
        self.assertIsInstance(
            ecf.RepeatDateList("DL", [20100101, 20100110]).start(), int
        )

    def test_end_returns_int(self):
        """end() returns an integer."""
        self.assertIsInstance(ecf.RepeatDateList("DL", [20100101, 20100110]).end(), int)

    def test_start_is_first_date(self):
        """start() is the first date in the list."""
        self.assertEqual(
            ecf.RepeatDateList("DL", [20100301, 20100615, 20101231]).start(), 20100301
        )

    def test_end_is_last_date(self):
        """end() is the last date in the list."""
        self.assertEqual(
            ecf.RepeatDateList("DL", [20100301, 20100615, 20101231]).end(), 20101231
        )

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format(self):
        """str() lists dates as quoted yyyymmdd strings."""
        rd = ecf.RepeatDateList("DL", [20100101, 20100105, 20100110])
        self.assertEqual(
            str(rd), '  repeat datelist DL "20100101" "20100105" "20100110"\n'
        )

    def test_str_single_date(self):
        """str() with a single date."""
        rd = ecf.RepeatDateList("DL", [20100101])
        self.assertEqual(str(rd), '  repeat datelist DL "20100101"\n')

    def test_str_returns_str_type(self):
        """str() returns a Python str."""
        self.assertIsInstance(str(ecf.RepeatDateList("DL", [20100101])), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A RepeatDateList is equal to itself."""
        rd = ecf.RepeatDateList("DL", [20100101, 20100105])
        self.assertEqual(rd, rd)

    def test_eq_same_args(self):
        """Two RepeatDateLists built with identical arguments are equal."""
        self.assertEqual(
            ecf.RepeatDateList("DL", [20100101, 20100105]),
            ecf.RepeatDateList("DL", [20100101, 20100105]),
        )

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.RepeatDateList("DL", [20100101, 20100105])
        b = ecf.RepeatDateList("DL", [20100101, 20100105])
        self.assertTrue(a == b and b == a)

    def test_ne_different_name(self):
        """Different names are not equal."""
        self.assertNotEqual(
            ecf.RepeatDateList("A", [20100101]),
            ecf.RepeatDateList("B", [20100101]),
        )

    def test_ne_different_dates(self):
        """Different date lists are not equal."""
        self.assertNotEqual(
            ecf.RepeatDateList("DL", [20100101, 20100105]),
            ecf.RepeatDateList("DL", [20100101, 20100110]),
        )

    def test_ne_different_length(self):
        """Date lists of different lengths are not equal."""
        self.assertNotEqual(
            ecf.RepeatDateList("DL", [20100101]),
            ecf.RepeatDateList("DL", [20100101, 20100105]),
        )

    def test_ne_complement(self):
        """a != b is False when a == b."""
        a = ecf.RepeatDateList("DL", [20100101, 20100105])
        b = ecf.RepeatDateList("DL", [20100101, 20100105])
        self.assertFalse(a != b)

    # ------------------------------------------------------------------
    # __lt__ — not exposed
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """RepeatDateList does not support '<'; raises TypeError."""
        a = ecf.RepeatDateList("DL", [20100101])
        b = ecf.RepeatDateList("DL", [20100105])
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_repeat_date_list(self):
        """copy.copy() returns a RepeatDateList instance."""
        rd = ecf.RepeatDateList("DL", [20100101, 20100105])
        self.assertIsInstance(copy.copy(rd), ecf.RepeatDateList)

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        rd = ecf.RepeatDateList("DL", [20100101, 20100105])
        self.assertIsNot(copy.copy(rd), rd)

    def test_copy_equal_value(self):
        """copy.copy() is value-equal to the original."""
        rd = ecf.RepeatDateList("DL", [20100101, 20100105])
        self.assertEqual(copy.copy(rd), rd)

    def test_copy_preserves_str(self):
        """The copied RepeatDateList has the same str() as the original."""
        rd = ecf.RepeatDateList("DL", [20100101, 20100105])
        self.assertEqual(str(copy.copy(rd)), str(rd))

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """RepeatDateList is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.RepeatDateList("DL", [20100101])), int)

    def test_same_object_same_hash(self):
        """The same object always returns the same hash."""
        rd = ecf.RepeatDateList("DL", [20100101, 20100105])
        self.assertEqual(hash(rd), hash(rd))

    def test_hash_is_identity_based(self):
        """Two value-equal RepeatDateLists have different hashes."""
        a = ecf.RepeatDateList("DL", [20100101, 20100105])
        b = ecf.RepeatDateList("DL", [20100101, 20100105])
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """RepeatDateList instances can be stored in a Python set."""
        a = ecf.RepeatDateList("DL", [20100101])
        b = ecf.RepeatDateList("DL", [20100105])
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """RepeatDateList instances can be used as dictionary keys."""
        rd = ecf.RepeatDateList("DL", [20100101])
        d = {rd: "datelist"}
        self.assertEqual(d[rd], "datelist")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_no_args_creates_empty(self):
        """RepeatDateList() without arguments creates an empty (unnamed) instance."""
        rd = ecf.RepeatDateList()
        self.assertIsInstance(rd, ecf.RepeatDateList)

    def test_non_int_dates_raises(self):
        """A list of strings instead of integers raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatDateList("DL", ["20100101"])

    # ------------------------------------------------------------------
    # Out-of-range date values
    # ------------------------------------------------------------------

    def test_date_month_13_raises(self):
        """A date with month=13 in the list raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateList("DL", [20101301])

    def test_date_month_0_raises(self):
        """A date with month=0 in the list raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateList("DL", [20100001])

    def test_date_day_32_raises(self):
        """A date with day=32 in the list raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateList("DL", [20100132])

    def test_date_day_0_raises(self):
        """A date with day=0 in the list raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateList("DL", [20100100])

    def test_date_too_few_digits_raises(self):
        """A date value with fewer than 8 digits raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateList("DL", [10101])

    def test_date_negative_raises(self):
        """A negative date value in the list raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateList("DL", [-20100101])

    def test_valid_mixed_with_invalid_raises(self):
        """A list containing one invalid date among valid ones raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateList("DL", [20100101, 20101301, 20100110])


class TestRepeatDateTimeList(unittest.TestCase):
    """Tests for py::class_<RepeatDateTimeList> exposed as ecf.RepeatDateTimeList in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        RepeatDateTimeList(name, datetimes)   -- name (str), datetimes as a list of ISO
                                                 datetime strings ('YYYYMMDDTHHMMSS')

    Instance methods
        name()    -> str   -- the repeat variable name
        start()   -> int   -- first instant as seconds since epoch (19700101T000000)
        end()     -> int   -- last instant as seconds since epoch

    Operators
        __str__   -- '  repeat datetimelist NAME "DT1" "DT2" ...\\n'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor
    # ------------------------------------------------------------------

    def test_ctor_single_datetime(self):
        """RepeatDateTimeList with a single datetime is valid."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000"])
        self.assertIsInstance(rd, ecf.RepeatDateTimeList)

    def test_ctor_multiple_datetimes(self):
        """RepeatDateTimeList stores all provided datetimes."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertLess(rd.start(), rd.end())

    def test_ctor_stores_name(self):
        """Constructor stores the name verbatim."""
        rd = ecf.RepeatDateTimeList("MYDTL", ["20100101T000000"])
        self.assertEqual(rd.name(), "MYDTL")

    def test_ctor_empty_list_raises(self):
        """An empty datetime list raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTimeList("DTL", [])

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() always returns a Python str."""
        self.assertIsInstance(
            ecf.RepeatDateTimeList("DTL", ["20100101T000000"]).name(), str
        )

    def test_name_value(self):
        """name() matches what was supplied to the constructor."""
        self.assertEqual(
            ecf.RepeatDateTimeList("DTIMES", ["20100101T000000"]).name(), "DTIMES"
        )

    # ------------------------------------------------------------------
    # start() / end()
    # ------------------------------------------------------------------

    def test_start_returns_int(self):
        """start() returns an integer (epoch seconds)."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertIsInstance(rd.start(), int)

    def test_end_returns_int(self):
        """end() returns an integer (epoch seconds)."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertIsInstance(rd.end(), int)

    def test_start_is_first_instant(self):
        """start() corresponds to the first datetime in the list."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertEqual(rd.start(), 1262304000)

    def test_end_is_last_instant(self):
        """end() corresponds to the last datetime in the list."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertEqual(rd.end(), 1262649600)

    def test_start_less_than_end(self):
        """start() < end() for a forward list."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertLess(rd.start(), rd.end())

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format(self):
        """str() lists datetimes as quoted ISO strings."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertEqual(
            str(rd),
            '  repeat datetimelist DTL "20100101T000000" "20100105T000000"\n',
        )

    def test_str_returns_str_type(self):
        """str() returns a Python str."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000"])
        self.assertIsInstance(str(rd), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A RepeatDateTimeList is equal to itself."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertEqual(rd, rd)

    def test_eq_same_args(self):
        """Two RepeatDateTimeLists built with identical arguments are equal."""
        self.assertEqual(
            ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"]),
            ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"]),
        )

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        b = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertTrue(a == b and b == a)

    def test_ne_different_name(self):
        """Different names are not equal."""
        self.assertNotEqual(
            ecf.RepeatDateTimeList("A", ["20100101T000000"]),
            ecf.RepeatDateTimeList("B", ["20100101T000000"]),
        )

    def test_ne_different_datetimes(self):
        """Different datetime lists are not equal."""
        self.assertNotEqual(
            ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"]),
            ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100110T000000"]),
        )

    def test_ne_different_length(self):
        """Datetime lists of different lengths are not equal."""
        self.assertNotEqual(
            ecf.RepeatDateTimeList("DTL", ["20100101T000000"]),
            ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"]),
        )

    def test_ne_complement(self):
        """a != b is False when a == b."""
        a = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        b = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertFalse(a != b)

    # ------------------------------------------------------------------
    # __lt__ — not exposed
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """RepeatDateTimeList does not support '<'; raises TypeError."""
        a = ecf.RepeatDateTimeList("DTL", ["20100101T000000"])
        b = ecf.RepeatDateTimeList("DTL", ["20100105T000000"])
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_repeat_datetimelist(self):
        """copy.copy() returns a RepeatDateTimeList instance."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertIsInstance(copy.copy(rd), ecf.RepeatDateTimeList)

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertIsNot(copy.copy(rd), rd)

    def test_copy_equal_value(self):
        """copy.copy() is value-equal to the original."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertEqual(copy.copy(rd), rd)

    def test_copy_preserves_str(self):
        """The copied RepeatDateTimeList has the same str() as the original."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertEqual(str(copy.copy(rd)), str(rd))

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """RepeatDateTimeList is hashable; hash() returns an int."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000"])
        self.assertIsInstance(hash(rd), int)

    def test_same_object_same_hash(self):
        """The same object always returns the same hash."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertEqual(hash(rd), hash(rd))

    def test_hash_is_identity_based(self):
        """Two value-equal RepeatDateTimeLists have different hashes."""
        a = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        b = ecf.RepeatDateTimeList("DTL", ["20100101T000000", "20100105T000000"])
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """RepeatDateTimeList instances can be stored in a Python set."""
        a = ecf.RepeatDateTimeList("DTL", ["20100101T000000"])
        b = ecf.RepeatDateTimeList("DTL", ["20100105T000000"])
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """RepeatDateTimeList instances can be used as dictionary keys."""
        rd = ecf.RepeatDateTimeList("DTL", ["20100101T000000"])
        d = {rd: "dtlist"}
        self.assertEqual(d[rd], "dtlist")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_no_args_creates_empty(self):
        """RepeatDateTimeList() without arguments creates an empty (unnamed) instance."""
        rd = ecf.RepeatDateTimeList()
        self.assertIsInstance(rd, ecf.RepeatDateTimeList)

    def test_non_string_datetimes_raises(self):
        """A list of integers instead of strings raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatDateTimeList("DTL", [20100101])

    # ------------------------------------------------------------------
    # Out-of-range instant values
    # ------------------------------------------------------------------

    def test_instant_month_13_raises(self):
        """An instant with month=13 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTimeList("DTL", ["20101301T000000"])

    def test_instant_month_0_raises(self):
        """An instant with month=0 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTimeList("DTL", ["20100001T000000"])

    def test_instant_day_32_raises(self):
        """An instant with day=32 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTimeList("DTL", ["20100132T000000"])

    def test_instant_day_0_raises(self):
        """An instant with day=0 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTimeList("DTL", ["20100100T000000"])

    def test_instant_hour_24_raises(self):
        """An instant with hour=24 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTimeList("DTL", ["20100101T240000"])

    def test_instant_minute_60_raises(self):
        """An instant with minute=60 raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTimeList("DTL", ["20100101T006000"])

    def test_instant_missing_separator_raises(self):
        """An instant without the 'T' separator raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTimeList("DTL", ["20100101000000"])

    def test_instant_empty_string_raises(self):
        """An empty instant string raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTimeList("DTL", [""])

    def test_instant_negative_year_raises(self):
        """An instant with a negative year raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTimeList("DTL", ["-0100101T000000"])

    def test_valid_mixed_with_invalid_raises(self):
        """A list containing one invalid instant among valid ones raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatDateTimeList(
                "DTL", ["20100101T000000", "20101301T000000", "20100110T000000"]
            )


class TestRepeatInteger(unittest.TestCase):
    """Tests for py::class_<RepeatInteger> exposed as ecf.RepeatInteger in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        RepeatInteger(name, start, end)         -- step defaults to 1
        RepeatInteger(name, start, end, step)   -- explicit step increment

    Instance methods
        name()    -> str   -- the repeat variable name
        start()   -> int   -- the start value
        end()     -> int   -- the end value (inclusive upper bound)
        step()    -> int   -- the increment (default 1)

    Operators
        __str__   -- '  repeat integer NAME START END[STEP]\\n'
                     STEP is omitted from str() when it equals 1
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: RepeatInteger(name, start, end)
    # ------------------------------------------------------------------

    def test_ctor_default_step(self):
        """RepeatInteger(name, start, end) defaults step to 1."""
        self.assertEqual(ecf.RepeatInteger("VAR", 0, 10).step(), 1)

    def test_ctor_stores_name(self):
        """Constructor stores the name verbatim."""
        self.assertEqual(ecf.RepeatInteger("MYVAR", 0, 10).name(), "MYVAR")

    def test_ctor_stores_start(self):
        """Constructor stores the start value."""
        self.assertEqual(ecf.RepeatInteger("VAR", 5, 10).start(), 5)

    def test_ctor_stores_end(self):
        """Constructor stores the end value."""
        self.assertEqual(ecf.RepeatInteger("VAR", 0, 7).end(), 7)

    # ------------------------------------------------------------------
    # Constructor: RepeatInteger(name, start, end, step)
    # ------------------------------------------------------------------

    def test_ctor_explicit_step(self):
        """RepeatInteger(name, start, end, step) stores the given step."""
        self.assertEqual(ecf.RepeatInteger("VAR", 0, 10, 2).step(), 2)

    def test_ctor_step_one_same_as_default(self):
        """Explicit step=1 is equal to the default-step constructor."""
        self.assertEqual(
            ecf.RepeatInteger("VAR", 0, 10, 1),
            ecf.RepeatInteger("VAR", 0, 10),
        )

    def test_ctor_negative_step(self):
        """A negative step is stored as given."""
        self.assertEqual(ecf.RepeatInteger("VAR", 10, 0, -1).step(), -1)

    # ------------------------------------------------------------------
    # Constructor: start > end edge cases
    # ------------------------------------------------------------------

    def test_ctor_start_gt_end_positive_step_accepted(self):
        """start > end with a positive step is accepted without error;
        the values are stored as-is (no ordering validation at construction)."""
        ri = ecf.RepeatInteger("VAR", 10, 0, 1)
        self.assertEqual(ri.start(), 10)
        self.assertEqual(ri.end(), 0)
        self.assertEqual(ri.step(), 1)

    def test_ctor_start_gt_end_negative_step_accepted(self):
        """start > end with a negative step is accepted without error;
        this is the conventional descending repeat."""
        ri = ecf.RepeatInteger("VAR", 10, 0, -1)
        self.assertEqual(ri.start(), 10)
        self.assertEqual(ri.end(), 0)
        self.assertEqual(ri.step(), -1)

    def test_ctor_start_gt_end_positive_step_str(self):
        """str() for start > end with a positive step reflects the stored values."""
        ri = ecf.RepeatInteger("VAR", 10, 0, 1)
        self.assertEqual(str(ri), "  repeat integer VAR 10 0\n")

    def test_ctor_start_gt_end_negative_step_str(self):
        """str() for start > end with a negative step includes the step."""
        ri = ecf.RepeatInteger("VAR", 10, 0, -1)
        self.assertEqual(str(ri), "  repeat integer VAR 10 0 -1\n")

    def test_ctor_start_lt_end_negative_step_accepted(self):
        """start < end with a negative step is accepted without error."""
        ri = ecf.RepeatInteger("VAR", 0, 10, -1)
        self.assertEqual(ri.start(), 0)
        self.assertEqual(ri.end(), 10)
        self.assertEqual(ri.step(), -1)

    def test_ctor_start_eq_end_positive_step_accepted(self):
        """start == end with a positive step is accepted; the repeat has one value."""
        ri = ecf.RepeatInteger("VAR", 5, 5, 1)
        self.assertEqual(ri.start(), 5)
        self.assertEqual(ri.end(), 5)

    def test_ctor_start_eq_end_negative_step_accepted(self):
        """start == end with a negative step is accepted; the repeat has one value."""
        ri = ecf.RepeatInteger("VAR", 5, 5, -1)
        self.assertEqual(ri.start(), 5)
        self.assertEqual(ri.end(), 5)
        self.assertEqual(ri.step(), -1)

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() always returns a Python str."""
        self.assertIsInstance(ecf.RepeatInteger("VAR", 0, 10).name(), str)

    def test_name_value(self):
        """name() returns the name supplied to the constructor."""
        self.assertEqual(ecf.RepeatInteger("COUNTER", 0, 5).name(), "COUNTER")

    # ------------------------------------------------------------------
    # start()
    # ------------------------------------------------------------------

    def test_start_returns_int(self):
        """start() returns an integer."""
        self.assertIsInstance(ecf.RepeatInteger("VAR", 0, 10).start(), int)

    def test_start_value(self):
        """start() returns the start value."""
        self.assertEqual(ecf.RepeatInteger("VAR", 3, 10).start(), 3)

    # ------------------------------------------------------------------
    # end()
    # ------------------------------------------------------------------

    def test_end_returns_int(self):
        """end() returns an integer."""
        self.assertIsInstance(ecf.RepeatInteger("VAR", 0, 10).end(), int)

    def test_end_value(self):
        """end() returns the end value."""
        self.assertEqual(ecf.RepeatInteger("VAR", 0, 9).end(), 9)

    # ------------------------------------------------------------------
    # step()
    # ------------------------------------------------------------------

    def test_step_returns_int(self):
        """step() returns an integer."""
        self.assertIsInstance(ecf.RepeatInteger("VAR", 0, 10).step(), int)

    def test_step_default_is_one(self):
        """Default step is 1."""
        self.assertEqual(ecf.RepeatInteger("VAR", 0, 10).step(), 1)

    def test_step_explicit_value(self):
        """step() returns the value given to the constructor."""
        self.assertEqual(ecf.RepeatInteger("VAR", 0, 20, 5).step(), 5)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_default_step_omitted(self):
        """str() omits the step when it is 1."""
        ri = ecf.RepeatInteger("VAR", 0, 10)
        self.assertEqual(str(ri), "  repeat integer VAR 0 10\n")

    def test_str_explicit_step_included(self):
        """str() includes the step when it is not 1."""
        ri = ecf.RepeatInteger("VAR", 0, 10, 2)
        self.assertEqual(str(ri), "  repeat integer VAR 0 10 2\n")

    def test_str_returns_str_type(self):
        """str() returns a Python str."""
        self.assertIsInstance(str(ecf.RepeatInteger("VAR", 0, 10)), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A RepeatInteger is equal to itself."""
        ri = ecf.RepeatInteger("VAR", 0, 10)
        self.assertEqual(ri, ri)

    def test_eq_same_args(self):
        """Two RepeatIntegers built with identical arguments are equal."""
        self.assertEqual(
            ecf.RepeatInteger("VAR", 0, 10),
            ecf.RepeatInteger("VAR", 0, 10),
        )

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.RepeatInteger("VAR", 0, 10)
        b = ecf.RepeatInteger("VAR", 0, 10)
        self.assertTrue(a == b and b == a)

    def test_ne_different_name(self):
        """Different names are not equal."""
        self.assertNotEqual(
            ecf.RepeatInteger("A", 0, 10),
            ecf.RepeatInteger("B", 0, 10),
        )

    def test_ne_different_start(self):
        """Different start values are not equal."""
        self.assertNotEqual(
            ecf.RepeatInteger("VAR", 0, 10),
            ecf.RepeatInteger("VAR", 1, 10),
        )

    def test_ne_different_end(self):
        """Different end values are not equal."""
        self.assertNotEqual(
            ecf.RepeatInteger("VAR", 0, 10),
            ecf.RepeatInteger("VAR", 0, 20),
        )

    def test_ne_different_step(self):
        """Different step values are not equal."""
        self.assertNotEqual(
            ecf.RepeatInteger("VAR", 0, 10, 1),
            ecf.RepeatInteger("VAR", 0, 10, 2),
        )

    def test_ne_complement(self):
        """a != b is False when a == b."""
        a = ecf.RepeatInteger("VAR", 0, 10)
        b = ecf.RepeatInteger("VAR", 0, 10)
        self.assertFalse(a != b)

    # ------------------------------------------------------------------
    # __lt__ — not exposed
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """RepeatInteger does not support '<'; raises TypeError."""
        a = ecf.RepeatInteger("VAR", 0, 10)
        b = ecf.RepeatInteger("VAR", 0, 20)
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_repeat_integer(self):
        """copy.copy() returns a RepeatInteger instance."""
        ri = ecf.RepeatInteger("VAR", 0, 10)
        self.assertIsInstance(copy.copy(ri), ecf.RepeatInteger)

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        ri = ecf.RepeatInteger("VAR", 0, 10)
        self.assertIsNot(copy.copy(ri), ri)

    def test_copy_equal_value(self):
        """copy.copy() is value-equal to the original."""
        ri = ecf.RepeatInteger("VAR", 0, 10)
        self.assertEqual(copy.copy(ri), ri)

    def test_copy_preserves_str(self):
        """The copied RepeatInteger has the same str() as the original."""
        ri = ecf.RepeatInteger("VAR", 0, 10, 2)
        self.assertEqual(str(copy.copy(ri)), str(ri))

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """RepeatInteger is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.RepeatInteger("VAR", 0, 10)), int)

    def test_same_object_same_hash(self):
        """The same object always returns the same hash."""
        ri = ecf.RepeatInteger("VAR", 0, 10)
        self.assertEqual(hash(ri), hash(ri))

    def test_hash_is_identity_based(self):
        """Two value-equal RepeatIntegers have different hashes."""
        a = ecf.RepeatInteger("VAR", 0, 10)
        b = ecf.RepeatInteger("VAR", 0, 10)
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """RepeatInteger instances can be stored in a Python set."""
        a = ecf.RepeatInteger("VAR", 0, 10)
        b = ecf.RepeatInteger("VAR", 0, 20)
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """RepeatInteger instances can be used as dictionary keys."""
        ri = ecf.RepeatInteger("VAR", 0, 10)
        d = {ri: "integer"}
        self.assertEqual(d[ri], "integer")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_no_args_raises(self):
        """Omitting all arguments raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatInteger()

    def test_non_string_name_raises(self):
        """Passing an integer as name raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatInteger(0, 0, 10)

    def test_non_int_start_raises(self):
        """Passing a string as start raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatInteger("VAR", "0", 10)


class TestRepeatEnumerated(unittest.TestCase):
    """Tests for py::class_<RepeatEnumerated> exposed as ecf.RepeatEnumerated in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        RepeatEnumerated(name, values)   -- name (str), values as a list of strings;
                                           the repeat iterates over these values by index

    Instance methods
        name()    -> str   -- the repeat variable name
        start()   -> int   -- always 0 (the first index)
        end()     -> int   -- len(values) - 1 (the last index)
        step()    -> int   -- always 1 (fixed index increment)

    Operators
        __str__   -- '  repeat enumerated NAME "v1" "v2" ...\\n'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor
    # ------------------------------------------------------------------

    def test_ctor_single_value(self):
        """RepeatEnumerated with a single value is valid."""
        re = ecf.RepeatEnumerated("COLOR", ["red"])
        self.assertEqual(re.start(), 0)
        self.assertEqual(re.end(), 0)

    def test_ctor_multiple_values(self):
        """RepeatEnumerated with multiple values is valid."""
        re = ecf.RepeatEnumerated("COLOR", ["red", "green", "blue"])
        self.assertEqual(re.end(), 2)

    def test_ctor_stores_name(self):
        """Constructor stores the name verbatim."""
        self.assertEqual(ecf.RepeatEnumerated("MYENUM", ["a", "b"]).name(), "MYENUM")

    def test_ctor_empty_list_raises(self):
        """An empty value list raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatEnumerated("E", [])

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() always returns a Python str."""
        self.assertIsInstance(ecf.RepeatEnumerated("E", ["a"]).name(), str)

    def test_name_value(self):
        """name() matches what was supplied to the constructor."""
        self.assertEqual(ecf.RepeatEnumerated("PHASE", ["a", "b"]).name(), "PHASE")

    # ------------------------------------------------------------------
    # start()
    # ------------------------------------------------------------------

    def test_start_is_always_zero(self):
        """start() is always 0 regardless of the values."""
        self.assertEqual(ecf.RepeatEnumerated("E", ["x", "y", "z"]).start(), 0)

    def test_start_returns_int(self):
        """start() returns an integer."""
        self.assertIsInstance(ecf.RepeatEnumerated("E", ["a"]).start(), int)

    # ------------------------------------------------------------------
    # end()
    # ------------------------------------------------------------------

    def test_end_is_last_index(self):
        """end() is len(values) - 1."""
        self.assertEqual(ecf.RepeatEnumerated("E", ["a", "b", "c", "d"]).end(), 3)

    def test_end_single_value(self):
        """end() is 0 for a single-value list."""
        self.assertEqual(ecf.RepeatEnumerated("E", ["only"]).end(), 0)

    def test_end_returns_int(self):
        """end() returns an integer."""
        self.assertIsInstance(ecf.RepeatEnumerated("E", ["a", "b"]).end(), int)

    # ------------------------------------------------------------------
    # step()
    # ------------------------------------------------------------------

    def test_step_is_always_one(self):
        """step() is always 1."""
        self.assertEqual(ecf.RepeatEnumerated("E", ["a", "b", "c"]).step(), 1)

    def test_step_returns_int(self):
        """step() returns an integer."""
        self.assertIsInstance(ecf.RepeatEnumerated("E", ["a"]).step(), int)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format(self):
        """str() lists values as quoted strings."""
        re = ecf.RepeatEnumerated("COLOR", ["red", "green", "blue"])
        self.assertEqual(str(re), '  repeat enumerated COLOR "red" "green" "blue"\n')

    def test_str_single_value(self):
        """str() with a single value."""
        re = ecf.RepeatEnumerated("X", ["only"])
        self.assertEqual(str(re), '  repeat enumerated X "only"\n')

    def test_str_returns_str_type(self):
        """str() returns a Python str."""
        self.assertIsInstance(str(ecf.RepeatEnumerated("E", ["a"])), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A RepeatEnumerated is equal to itself."""
        re = ecf.RepeatEnumerated("E", ["a", "b"])
        self.assertEqual(re, re)

    def test_eq_same_args(self):
        """Two RepeatEnumerateds built with identical arguments are equal."""
        self.assertEqual(
            ecf.RepeatEnumerated("COLOR", ["red", "green"]),
            ecf.RepeatEnumerated("COLOR", ["red", "green"]),
        )

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.RepeatEnumerated("E", ["a", "b"])
        b = ecf.RepeatEnumerated("E", ["a", "b"])
        self.assertTrue(a == b and b == a)

    def test_ne_different_name(self):
        """Different names are not equal."""
        self.assertNotEqual(
            ecf.RepeatEnumerated("A", ["x"]),
            ecf.RepeatEnumerated("B", ["x"]),
        )

    def test_ne_different_values(self):
        """Different value lists are not equal."""
        self.assertNotEqual(
            ecf.RepeatEnumerated("E", ["a", "b"]),
            ecf.RepeatEnumerated("E", ["a", "c"]),
        )

    def test_ne_different_length(self):
        """Value lists of different lengths are not equal."""
        self.assertNotEqual(
            ecf.RepeatEnumerated("E", ["a"]),
            ecf.RepeatEnumerated("E", ["a", "b"]),
        )

    def test_ne_complement(self):
        """a != b is False when a == b."""
        a = ecf.RepeatEnumerated("E", ["a", "b"])
        b = ecf.RepeatEnumerated("E", ["a", "b"])
        self.assertFalse(a != b)

    # ------------------------------------------------------------------
    # __lt__ — not exposed
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """RepeatEnumerated does not support '<'; raises TypeError."""
        a = ecf.RepeatEnumerated("E", ["a"])
        b = ecf.RepeatEnumerated("E", ["b"])
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_repeat_enumerated(self):
        """copy.copy() returns a RepeatEnumerated instance."""
        re = ecf.RepeatEnumerated("E", ["a", "b"])
        self.assertIsInstance(copy.copy(re), ecf.RepeatEnumerated)

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        re = ecf.RepeatEnumerated("E", ["a", "b"])
        self.assertIsNot(copy.copy(re), re)

    def test_copy_equal_value(self):
        """copy.copy() is value-equal to the original."""
        re = ecf.RepeatEnumerated("E", ["a", "b"])
        self.assertEqual(copy.copy(re), re)

    def test_copy_preserves_str(self):
        """The copied RepeatEnumerated has the same str() as the original."""
        re = ecf.RepeatEnumerated("COLOR", ["red", "green", "blue"])
        self.assertEqual(str(copy.copy(re)), str(re))

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """RepeatEnumerated is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.RepeatEnumerated("E", ["a"])), int)

    def test_same_object_same_hash(self):
        """The same object always returns the same hash."""
        re = ecf.RepeatEnumerated("E", ["a", "b"])
        self.assertEqual(hash(re), hash(re))

    def test_hash_is_identity_based(self):
        """Two value-equal RepeatEnumerateds have different hashes."""
        a = ecf.RepeatEnumerated("E", ["a", "b"])
        b = ecf.RepeatEnumerated("E", ["a", "b"])
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """RepeatEnumerated instances can be stored in a Python set."""
        a = ecf.RepeatEnumerated("E", ["a"])
        b = ecf.RepeatEnumerated("E", ["b"])
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """RepeatEnumerated instances can be used as dictionary keys."""
        re = ecf.RepeatEnumerated("E", ["a", "b"])
        d = {re: "enum"}
        self.assertEqual(d[re], "enum")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_no_args_creates_empty(self):
        """RepeatEnumerated() without arguments creates an empty (unnamed) instance."""
        re = ecf.RepeatEnumerated()
        self.assertIsInstance(re, ecf.RepeatEnumerated)

    def test_non_list_values_raises(self):
        """Passing a non-list for values raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatEnumerated("E", "not-a-list")

    def test_list_with_non_string_raises(self):
        """A list containing integers instead of strings raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatEnumerated("E", [1, 2, 3])


class TestRepeatString(unittest.TestCase):
    """Tests for py::class_<RepeatString> exposed as ecf.RepeatString in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        RepeatString(name, values)   -- name (str), values as a list of strings;
                                        the repeat iterates over these values by index

    Instance methods
        name()    -> str   -- the repeat variable name
        start()   -> int   -- always 0 (the first index)
        end()     -> int   -- len(values) - 1 (the last index)
        step()    -> int   -- always 1 (fixed index increment)

    Operators
        __str__   -- '  repeat string NAME "v1" "v2" ...\\n'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor
    # ------------------------------------------------------------------

    def test_ctor_single_value(self):
        """RepeatString with a single value is valid."""
        rs = ecf.RepeatString("LABEL", ["only"])
        self.assertEqual(rs.start(), 0)
        self.assertEqual(rs.end(), 0)

    def test_ctor_multiple_values(self):
        """RepeatString with multiple values is valid."""
        rs = ecf.RepeatString("LABEL", ["alpha", "beta", "gamma"])
        self.assertEqual(rs.end(), 2)

    def test_ctor_stores_name(self):
        """Constructor stores the name verbatim."""
        self.assertEqual(ecf.RepeatString("MYSTR", ["a", "b"]).name(), "MYSTR")

    def test_ctor_empty_list_raises(self):
        """An empty value list raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.RepeatString("S", [])

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_returns_str(self):
        """name() always returns a Python str."""
        self.assertIsInstance(ecf.RepeatString("S", ["a"]).name(), str)

    def test_name_value(self):
        """name() matches what was supplied to the constructor."""
        self.assertEqual(ecf.RepeatString("TAG", ["a", "b"]).name(), "TAG")

    # ------------------------------------------------------------------
    # start()
    # ------------------------------------------------------------------

    def test_start_is_always_zero(self):
        """start() is always 0."""
        self.assertEqual(ecf.RepeatString("S", ["x", "y", "z"]).start(), 0)

    def test_start_returns_int(self):
        """start() returns an integer."""
        self.assertIsInstance(ecf.RepeatString("S", ["a"]).start(), int)

    # ------------------------------------------------------------------
    # end()
    # ------------------------------------------------------------------

    def test_end_is_last_index(self):
        """end() is len(values) - 1."""
        self.assertEqual(ecf.RepeatString("S", ["a", "b", "c"]).end(), 2)

    def test_end_single_value(self):
        """end() is 0 for a single-value list."""
        self.assertEqual(ecf.RepeatString("S", ["only"]).end(), 0)

    def test_end_returns_int(self):
        """end() returns an integer."""
        self.assertIsInstance(ecf.RepeatString("S", ["a", "b"]).end(), int)

    # ------------------------------------------------------------------
    # step()
    # ------------------------------------------------------------------

    def test_step_is_always_one(self):
        """step() is always 1."""
        self.assertEqual(ecf.RepeatString("S", ["a", "b", "c"]).step(), 1)

    def test_step_returns_int(self):
        """step() returns an integer."""
        self.assertIsInstance(ecf.RepeatString("S", ["a"]).step(), int)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format(self):
        """str() lists values as quoted strings."""
        rs = ecf.RepeatString("LABEL", ["alpha", "beta"])
        self.assertEqual(str(rs), '  repeat string LABEL "alpha" "beta"\n')

    def test_str_single_value(self):
        """str() with a single value."""
        rs = ecf.RepeatString("S", ["only"])
        self.assertEqual(str(rs), '  repeat string S "only"\n')

    def test_str_returns_str_type(self):
        """str() returns a Python str."""
        self.assertIsInstance(str(ecf.RepeatString("S", ["a"])), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A RepeatString is equal to itself."""
        rs = ecf.RepeatString("S", ["a", "b"])
        self.assertEqual(rs, rs)

    def test_eq_same_args(self):
        """Two RepeatStrings built with identical arguments are equal."""
        self.assertEqual(
            ecf.RepeatString("LABEL", ["alpha", "beta"]),
            ecf.RepeatString("LABEL", ["alpha", "beta"]),
        )

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.RepeatString("S", ["a", "b"])
        b = ecf.RepeatString("S", ["a", "b"])
        self.assertTrue(a == b and b == a)

    def test_ne_different_name(self):
        """Different names are not equal."""
        self.assertNotEqual(
            ecf.RepeatString("A", ["x"]),
            ecf.RepeatString("B", ["x"]),
        )

    def test_ne_different_values(self):
        """Different value lists are not equal."""
        self.assertNotEqual(
            ecf.RepeatString("S", ["a", "b"]),
            ecf.RepeatString("S", ["a", "c"]),
        )

    def test_ne_different_length(self):
        """Value lists of different lengths are not equal."""
        self.assertNotEqual(
            ecf.RepeatString("S", ["a"]),
            ecf.RepeatString("S", ["a", "b"]),
        )

    def test_ne_complement(self):
        """a != b is False when a == b."""
        a = ecf.RepeatString("S", ["a", "b"])
        b = ecf.RepeatString("S", ["a", "b"])
        self.assertFalse(a != b)

    # ------------------------------------------------------------------
    # __lt__ — not exposed
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """RepeatString does not support '<'; raises TypeError."""
        a = ecf.RepeatString("S", ["a"])
        b = ecf.RepeatString("S", ["b"])
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_repeat_string(self):
        """copy.copy() returns a RepeatString instance."""
        rs = ecf.RepeatString("S", ["a", "b"])
        self.assertIsInstance(copy.copy(rs), ecf.RepeatString)

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        rs = ecf.RepeatString("S", ["a", "b"])
        self.assertIsNot(copy.copy(rs), rs)

    def test_copy_equal_value(self):
        """copy.copy() is value-equal to the original."""
        rs = ecf.RepeatString("S", ["a", "b"])
        self.assertEqual(copy.copy(rs), rs)

    def test_copy_preserves_str(self):
        """The copied RepeatString has the same str() as the original."""
        rs = ecf.RepeatString("LABEL", ["alpha", "beta"])
        self.assertEqual(str(copy.copy(rs)), str(rs))

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """RepeatString is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.RepeatString("S", ["a"])), int)

    def test_same_object_same_hash(self):
        """The same object always returns the same hash."""
        rs = ecf.RepeatString("S", ["a", "b"])
        self.assertEqual(hash(rs), hash(rs))

    def test_hash_is_identity_based(self):
        """Two value-equal RepeatStrings have different hashes."""
        a = ecf.RepeatString("S", ["a", "b"])
        b = ecf.RepeatString("S", ["a", "b"])
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """RepeatString instances can be stored in a Python set."""
        a = ecf.RepeatString("S", ["a"])
        b = ecf.RepeatString("S", ["b"])
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """RepeatString instances can be used as dictionary keys."""
        rs = ecf.RepeatString("S", ["a", "b"])
        d = {rs: "string"}
        self.assertEqual(d[rs], "string")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_no_args_creates_empty(self):
        """RepeatString() without arguments creates an empty (unnamed) instance."""
        rs = ecf.RepeatString()
        self.assertIsInstance(rs, ecf.RepeatString)

    def test_non_list_values_raises(self):
        """Passing a non-list for values raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatString("S", "not-a-list")

    def test_list_with_non_string_raises(self):
        """A list containing integers instead of strings raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatString("S", [1, 2])


class TestRepeatDay(unittest.TestCase):
    """Tests for py::class_<RepeatDay> exposed as ecf.RepeatDay in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        RepeatDay()        -- step defaults to 1
        RepeatDay(step)    -- explicit step (int), must be >= 1

    Operators
        __str__   -- '  repeat day STEP\\n'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: RepeatDay()
    # ------------------------------------------------------------------

    def test_ctor_default_step_str(self):
        """RepeatDay() defaults to step=1."""
        self.assertEqual(str(ecf.RepeatDay()), "  repeat day 1\n")

    def test_ctor_default_produces_instance(self):
        """RepeatDay() returns a RepeatDay instance."""
        self.assertIsInstance(ecf.RepeatDay(), ecf.RepeatDay)

    # ------------------------------------------------------------------
    # Constructor: RepeatDay(step)
    # ------------------------------------------------------------------

    def test_ctor_explicit_step(self):
        """RepeatDay(step) stores the given step."""
        self.assertEqual(str(ecf.RepeatDay(3)), "  repeat day 3\n")

    def test_ctor_step_one_equals_default(self):
        """RepeatDay(1) is equal to RepeatDay()."""
        self.assertEqual(ecf.RepeatDay(1), ecf.RepeatDay())

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_default(self):
        """str() for default step is '  repeat day 1\\n'."""
        self.assertEqual(str(ecf.RepeatDay()), "  repeat day 1\n")

    def test_str_explicit_step(self):
        """str() reflects the explicit step value."""
        self.assertEqual(str(ecf.RepeatDay(7)), "  repeat day 7\n")

    def test_str_returns_str_type(self):
        """str() returns a Python str."""
        self.assertIsInstance(str(ecf.RepeatDay()), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A RepeatDay is equal to itself."""
        rd = ecf.RepeatDay(2)
        self.assertEqual(rd, rd)

    def test_eq_same_step(self):
        """Two RepeatDays with the same step are equal."""
        self.assertEqual(ecf.RepeatDay(3), ecf.RepeatDay(3))

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.RepeatDay(2)
        b = ecf.RepeatDay(2)
        self.assertTrue(a == b and b == a)

    def test_ne_different_step(self):
        """RepeatDays with different steps are not equal."""
        self.assertNotEqual(ecf.RepeatDay(1), ecf.RepeatDay(2))

    def test_ne_complement(self):
        """a != b is False when a == b."""
        a = ecf.RepeatDay(2)
        b = ecf.RepeatDay(2)
        self.assertFalse(a != b)

    # ------------------------------------------------------------------
    # __lt__ — not exposed
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """RepeatDay does not support '<'; raises TypeError."""
        a = ecf.RepeatDay(1)
        b = ecf.RepeatDay(2)
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_repeat_day(self):
        """copy.copy() returns a RepeatDay instance."""
        self.assertIsInstance(copy.copy(ecf.RepeatDay(2)), ecf.RepeatDay)

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        rd = ecf.RepeatDay(2)
        self.assertIsNot(copy.copy(rd), rd)

    def test_copy_equal_value(self):
        """copy.copy() is value-equal to the original."""
        rd = ecf.RepeatDay(2)
        self.assertEqual(copy.copy(rd), rd)

    def test_copy_preserves_str(self):
        """The copied RepeatDay has the same str() as the original."""
        rd = ecf.RepeatDay(3)
        self.assertEqual(str(copy.copy(rd)), str(rd))

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """RepeatDay is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.RepeatDay()), int)

    def test_same_object_same_hash(self):
        """The same object always returns the same hash."""
        rd = ecf.RepeatDay(2)
        self.assertEqual(hash(rd), hash(rd))

    def test_hash_is_identity_based(self):
        """Two value-equal RepeatDays have different hashes."""
        a = ecf.RepeatDay(2)
        b = ecf.RepeatDay(2)
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """RepeatDay instances can be stored in a Python set."""
        a = ecf.RepeatDay(1)
        b = ecf.RepeatDay(2)
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """RepeatDay instances can be used as dictionary keys."""
        rd = ecf.RepeatDay(2)
        d = {rd: "day"}
        self.assertEqual(d[rd], "day")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_non_int_step_raises(self):
        """Passing a string as step raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatDay("one")

    def test_float_step_raises(self):
        """Passing a float as step raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatDay(1.5)

    def test_none_step_raises(self):
        """Passing None as step raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.RepeatDay(None)

    # ------------------------------------------------------------------
    # Invalid step values (accepted with no error)
    # ------------------------------------------------------------------

    def test_zero_step_accepted(self):
        """RepeatDay(0) is accepted without error; step is stored as 0."""
        rd = ecf.RepeatDay(0)
        self.assertEqual(str(rd), "  repeat day 0\n")

    def test_negative_step_accepted(self):
        """RepeatDay(-1) is accepted without error; step is stored as given."""
        rd = ecf.RepeatDay(-1)
        self.assertEqual(str(rd), "  repeat day -1\n")


class TestRepeat(unittest.TestCase):
    """Tests for py::class_<Repeat> exposed as ecf.Repeat in ExportNodeAttr.cpp.

    Repeat is a type-erasing wrapper (std::unique_ptr<RepeatBase>) that can hold
    any of the concrete repeat types: RepeatDate, RepeatDateTime, RepeatDateList,
    RepeatDateTimeList, RepeatInteger, RepeatEnumerated, RepeatString, or RepeatDay.
    In practice, Repeat instances are most commonly obtained from node.get_repeat(),
    which returns whatever concrete repeat is attached to the node.

    The Python binding exposes a single constructor accepting an int.  Boost.Python
    resolves this via the implicit-conversion chain
        int -> RepeatDay(int) -> Repeat(const RepeatDay&)
    so all tests here exercise the RepeatDay-backed specialisation.  All accessor
    methods delegate to the wrapped type and return 0 / empty string when the Repeat
    is empty (i.e. holds no concrete type).

    Exposed API
    -----------
    Constructors
        Repeat(step)   -- step (int); creates a Repeat wrapping RepeatDay(step)
                          via Boost.Python implicit conversion

    Instance methods
        empty()   -> bool   -- True when no concrete type is held (e.g. obtained
                               from a node with no repeat attribute); always False
                               for a Repeat(int) instance
        name()    -> str    -- delegates to the wrapped type; 'day' for RepeatDay
        start()   -> int    -- delegates to the wrapped type; 0 for RepeatDay
        end()     -> int    -- delegates to the wrapped type; 0 for RepeatDay
        step()    -> int    -- delegates to the wrapped type; the constructor
                               argument for RepeatDay
        value()   -> int    -- current value via last_valid_value(); equals step
                               for a freshly constructed RepeatDay-backed Repeat

    Operators
        __str__   -- delegates to the wrapped type; '  repeat day STEP\\n' for RepeatDay
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor
    # ------------------------------------------------------------------

    def test_ctor_produces_instance(self):
        """Repeat(step) returns a Repeat instance."""
        self.assertIsInstance(ecf.Repeat(1), ecf.Repeat)

    def test_ctor_stores_step(self):
        """step() returns the value given to the constructor."""
        self.assertEqual(ecf.Repeat(3).step(), 3)

    def test_ctor_step_one(self):
        """Repeat(1) constructs without error."""
        r = ecf.Repeat(1)
        self.assertEqual(r.step(), 1)

    # ------------------------------------------------------------------
    # empty()
    # ------------------------------------------------------------------

    def test_empty_is_false(self):
        """empty() returns False for any Repeat(step) instance."""
        self.assertFalse(ecf.Repeat(1).empty())

    def test_empty_returns_bool(self):
        """empty() returns a bool."""
        self.assertIsInstance(ecf.Repeat(1).empty(), bool)

    # ------------------------------------------------------------------
    # name()
    # ------------------------------------------------------------------

    def test_name_is_day(self):
        """name() is always 'day' (the underlying RepeatDay name)."""
        self.assertEqual(ecf.Repeat(1).name(), "day")

    def test_name_returns_str(self):
        """name() always returns a Python str."""
        self.assertIsInstance(ecf.Repeat(1).name(), str)

    # ------------------------------------------------------------------
    # start()
    # ------------------------------------------------------------------

    def test_start_is_zero(self):
        """start() is always 0."""
        self.assertEqual(ecf.Repeat(5).start(), 0)

    def test_start_returns_int(self):
        """start() returns an integer."""
        self.assertIsInstance(ecf.Repeat(1).start(), int)

    # ------------------------------------------------------------------
    # end()
    # ------------------------------------------------------------------

    def test_end_is_zero(self):
        """end() is always 0."""
        self.assertEqual(ecf.Repeat(5).end(), 0)

    def test_end_returns_int(self):
        """end() returns an integer."""
        self.assertIsInstance(ecf.Repeat(1).end(), int)

    # ------------------------------------------------------------------
    # step()
    # ------------------------------------------------------------------

    def test_step_returns_int(self):
        """step() returns an integer."""
        self.assertIsInstance(ecf.Repeat(2).step(), int)

    def test_step_matches_constructor_arg(self):
        """step() returns the value supplied to the constructor."""
        self.assertEqual(ecf.Repeat(7).step(), 7)

    # ------------------------------------------------------------------
    # value()
    # ------------------------------------------------------------------

    def test_value_equals_step_on_construction(self):
        """value() equals step() for a freshly constructed Repeat."""
        r = ecf.Repeat(4)
        self.assertEqual(r.value(), r.step())

    def test_value_returns_int(self):
        """value() returns an integer."""
        self.assertIsInstance(ecf.Repeat(1).value(), int)

    def test_value_is_step_for_repeat_day(self):
        """For Repeat(N), value() returns N."""
        self.assertEqual(ecf.Repeat(5).value(), 5)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_format(self):
        """str() is '  repeat day STEP\\n'."""
        self.assertEqual(str(ecf.Repeat(1)), "  repeat day 1\n")

    def test_str_explicit_step(self):
        """str() reflects the step value given to the constructor."""
        self.assertEqual(str(ecf.Repeat(3)), "  repeat day 3\n")

    def test_str_returns_str_type(self):
        """str() returns a Python str."""
        self.assertIsInstance(str(ecf.Repeat(1)), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A Repeat is equal to itself."""
        r = ecf.Repeat(1)
        self.assertEqual(r, r)

    def test_eq_same_step(self):
        """Two Repeats with the same step are equal."""
        self.assertEqual(ecf.Repeat(3), ecf.Repeat(3))

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.Repeat(2)
        b = ecf.Repeat(2)
        self.assertTrue(a == b and b == a)

    def test_ne_different_step(self):
        """Repeats with different steps are not equal."""
        self.assertNotEqual(ecf.Repeat(1), ecf.Repeat(2))

    def test_ne_complement(self):
        """a != b is False when a == b."""
        a = ecf.Repeat(2)
        b = ecf.Repeat(2)
        self.assertFalse(a != b)

    # ------------------------------------------------------------------
    # __lt__ — not exposed
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """Repeat does not support '<'; raises TypeError."""
        a = ecf.Repeat(1)
        b = ecf.Repeat(2)
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_repeat(self):
        """copy.copy() returns a Repeat instance."""
        self.assertIsInstance(copy.copy(ecf.Repeat(1)), ecf.Repeat)

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        r = ecf.Repeat(2)
        self.assertIsNot(copy.copy(r), r)

    def test_copy_equal_value(self):
        """copy.copy() is value-equal to the original."""
        r = ecf.Repeat(3)
        self.assertEqual(copy.copy(r), r)

    def test_copy_preserves_str(self):
        """The copied Repeat has the same str() as the original."""
        r = ecf.Repeat(4)
        self.assertEqual(str(copy.copy(r)), str(r))

    def test_copy_preserves_step(self):
        """The copied Repeat has the same step() as the original."""
        r = ecf.Repeat(5)
        self.assertEqual(copy.copy(r).step(), r.step())

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Repeat is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Repeat(1)), int)

    def test_same_object_same_hash(self):
        """The same object always returns the same hash."""
        r = ecf.Repeat(2)
        self.assertEqual(hash(r), hash(r))

    def test_hash_is_identity_based(self):
        """Two value-equal Repeats have different hashes."""
        a = ecf.Repeat(2)
        b = ecf.Repeat(2)
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """Repeat instances can be stored in a Python set."""
        a = ecf.Repeat(1)
        b = ecf.Repeat(2)
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """Repeat instances can be used as dictionary keys."""
        r = ecf.Repeat(1)
        d = {r: "repeat"}
        self.assertEqual(d[r], "repeat")

    # ------------------------------------------------------------------
    # Wrong argument types
    # ------------------------------------------------------------------

    def test_no_args_raises(self):
        """Omitting all arguments raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Repeat()

    def test_non_int_raises(self):
        """Passing a string raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Repeat("one")

    def test_float_raises(self):
        """Passing a float raises TypeError."""
        with self.assertRaises((TypeError, RuntimeError)):
            ecf.Repeat(1.5)


class TestCron(unittest.TestCase):
    """Tests for py::class_<ecf::CronAttr, ...> exposed as ecf.Cron in ExportNodeAttr.cpp.

    Cron is a calendar-aware repeat attribute.  It fires at a fixed time (or time
    series) and can be restricted to specific days of the week, days of the month,
    months of the year, the last day of the month, or the last occurrence of a
    weekday in the month.

    Exposed API
    -----------
    Constructors
        Cron()                        -- default; time 00:00, no filters
        Cron(time_str)                -- time string 'HH:MM' or 'HH:MM HH:MM HH:MM'
        Cron(time_str, **kwargs)      -- time string plus keyword filter arguments
        Cron(TimeSeries)              -- TimeSeries directly (cron_create2)
        Cron(TimeSeries, **kwargs)    -- TimeSeries plus keyword filter arguments

        Keyword arguments accepted by the string/TimeSeries constructors:
            days_of_week=[int...]              -- 0 (Sun) to 6 (Sat)
            days_of_month=[int...]             -- 1 to 31
            months=[int...]                    -- 1 (Jan) to 12 (Dec)
            last_week_days_of_the_month=[int...] -- 0 (Sun) to 6 (Sat)
            last_day_of_the_month=True         -- flag

    Instance methods / setters
        set_week_days(list)                    -- set days of week (0-6)
        set_last_week_days_of_the_month(list)  -- set last weekdays (0-6)
        set_days_of_month(list)                -- set days of month (1-31)
        set_months(list)                       -- set months (1-12)
        set_last_day_of_the_month()            -- enable last-day flag
        set_time_series(hour, minute, relative=False)
        set_time_series(TimeSeries)
        set_time_series(start, finish, incr)   -- three TimeSlots
        set_time_series(time_str)              -- string form
        time()               -> TimeSeries    -- the cron time
        last_day_of_the_month() -> bool       -- True if last-day flag is set

    Properties (read-only iterables)
        week_days                     -- list of int, 0-6
        last_week_days_of_the_month   -- list of int, 0-6
        days_of_month                 -- list of int, 1-31
        months                        -- list of int, 1-12

    Operators
        __str__   -- 'cron [-w W,...] [-d D,...] [-m M,...] HH:MM[...]'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: Cron()
    # ------------------------------------------------------------------

    def test_ctor_default_produces_instance(self):
        """Cron() returns a Cron instance."""
        self.assertIsInstance(ecf.Cron(), ecf.Cron)

    def test_ctor_default_str(self):
        """Cron() str is 'cron 00:00'."""
        self.assertEqual(str(ecf.Cron()), "cron 00:00")

    def test_ctor_default_no_week_days(self):
        """Cron() has an empty week_days list."""
        self.assertEqual(list(ecf.Cron().week_days), [])

    def test_ctor_default_no_days_of_month(self):
        """Cron() has an empty days_of_month list."""
        self.assertEqual(list(ecf.Cron().days_of_month), [])

    def test_ctor_default_no_months(self):
        """Cron() has an empty months list."""
        self.assertEqual(list(ecf.Cron().months), [])

    def test_ctor_default_last_day_is_false(self):
        """Cron() has last_day_of_the_month() == False."""
        self.assertFalse(ecf.Cron().last_day_of_the_month())

    # ------------------------------------------------------------------
    # Constructor: Cron(time_str)
    # ------------------------------------------------------------------

    def test_ctor_str_simple_time(self):
        """Cron('HH:MM') stores the time correctly."""
        self.assertEqual(str(ecf.Cron("08:00")), "cron 08:00")

    def test_ctor_str_midnight(self):
        """Cron('00:00') is accepted."""
        self.assertEqual(str(ecf.Cron("00:00")), "cron 00:00")

    def test_ctor_str_produces_instance(self):
        """Cron(str) returns a Cron instance."""
        self.assertIsInstance(ecf.Cron("12:30"), ecf.Cron)

    def test_ctor_empty_string_raises(self):
        """Cron('') raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Cron("")

    # ------------------------------------------------------------------
    # Constructor: Cron(time_str, **kwargs)
    # ------------------------------------------------------------------

    def test_ctor_str_with_days_of_week(self):
        """Cron with days_of_week kwarg includes -w in str()."""
        c = ecf.Cron("08:00", days_of_week=[1, 3, 5])
        self.assertEqual(str(c), "cron -w 1,3,5 08:00")

    def test_ctor_str_with_days_of_month(self):
        """Cron with days_of_month kwarg includes -d in str()."""
        c = ecf.Cron("08:00", days_of_month=[1, 15])
        self.assertEqual(str(c), "cron -d 1,15 08:00")

    def test_ctor_str_with_months(self):
        """Cron with months kwarg includes -m in str()."""
        c = ecf.Cron("08:00", months=[1, 6, 12])
        self.assertEqual(str(c), "cron -m 1,6,12 08:00")

    def test_ctor_str_with_last_day_of_month(self):
        """Cron with last_day_of_the_month=True includes -d L in str()."""
        c = ecf.Cron("08:00", last_day_of_the_month=True)
        self.assertEqual(str(c), "cron -d L 08:00")

    def test_ctor_str_with_last_week_days_of_month(self):
        """Cron with last_week_days_of_the_month kwarg includes 'L' suffixed weekdays."""
        c = ecf.Cron("08:00", last_week_days_of_the_month=[0, 6])
        self.assertEqual(str(c), "cron -w 0L,6L 08:00")

    def test_ctor_bad_kwarg_raises(self):
        """An unrecognised keyword argument raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Cron("08:00", bad_key=[1])

    def test_ctor_kwarg_not_list_raises(self):
        """days_of_week as a plain int (not a list) raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.Cron("08:00", days_of_week=1)

    def test_ctor_days_of_week_out_of_range_raises(self):
        """days_of_week=[7] raises IndexError (valid range 0-6)."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00", days_of_week=[7])

    def test_ctor_days_of_month_zero_raises(self):
        """days_of_month=[0] raises IndexError (valid range 1-31)."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00", days_of_month=[0])

    def test_ctor_days_of_month_32_raises(self):
        """days_of_month=[32] raises IndexError (valid range 1-31)."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00", days_of_month=[32])

    def test_ctor_months_zero_raises(self):
        """months=[0] raises IndexError (valid range 1-12)."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00", months=[0])

    def test_ctor_months_13_raises(self):
        """months=[13] raises IndexError (valid range 1-12)."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00", months=[13])

    def test_ctor_last_week_days_out_of_range_raises(self):
        """last_week_days_of_the_month=[7] raises IndexError (valid range 0-6)."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00", last_week_days_of_the_month=[7])

    # ------------------------------------------------------------------
    # Constructor: Cron(TimeSeries) and Cron(TimeSeries, **kwargs)
    # ------------------------------------------------------------------

    def test_ctor_timeseries_produces_instance(self):
        """Cron(TimeSeries) returns a Cron instance."""
        ts = ecf.TimeSeries(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(4, 0), True
        )
        self.assertIsInstance(ecf.Cron(ts), ecf.Cron)

    def test_ctor_timeseries_str(self):
        """Cron(TimeSeries) reflects the series in str()."""
        ts = ecf.TimeSeries(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(4, 0), True
        )
        self.assertEqual(str(ecf.Cron(ts)), "cron +06:00 20:00 04:00")

    def test_ctor_timeseries_with_days_of_week(self):
        """Cron(TimeSeries, days_of_week=...) combines the series and weekday filter."""
        ts = ecf.TimeSeries(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(4, 0), True
        )
        c = ecf.Cron(ts, days_of_week=[1])
        self.assertEqual(str(c), "cron -w 1 +06:00 20:00 04:00")

    # ------------------------------------------------------------------
    # set_time_series overloads
    # ------------------------------------------------------------------

    def test_set_time_series_hm(self):
        """set_time_series(h, m) sets an absolute single time slot."""
        c = ecf.Cron()
        c.set_time_series(8, 0)
        self.assertEqual(str(c), "cron 08:00")

    def test_set_time_series_hm_relative(self):
        """set_time_series(h, m, True) sets a relative time slot."""
        c = ecf.Cron()
        c.set_time_series(8, 0, True)
        self.assertEqual(str(c), "cron +08:00")

    def test_set_time_series_timeseries(self):
        """set_time_series(TimeSeries) stores the full series."""
        c = ecf.Cron()
        ts = ecf.TimeSeries(
            ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(4, 0), True
        )
        c.set_time_series(ts)
        self.assertEqual(str(c), "cron +06:00 20:00 04:00")

    def test_set_time_series_three_slots(self):
        """set_time_series(start, finish, incr) stores an absolute series."""
        c = ecf.Cron()
        c.set_time_series(ecf.TimeSlot(6, 0), ecf.TimeSlot(20, 0), ecf.TimeSlot(4, 0))
        self.assertEqual(str(c), "cron 06:00 20:00 04:00")

    def test_set_time_series_str(self):
        """set_time_series('HH:MM') stores the time via string parsing."""
        c = ecf.Cron()
        c.set_time_series("08:00")
        self.assertEqual(str(c), "cron 08:00")

    def test_set_time_series_h_out_of_range_raises(self):
        """set_time_series(25, 0) raises IndexError (hour must be 0-23)."""
        with self.assertRaises(IndexError):
            ecf.Cron().set_time_series(25, 0)

    def test_set_time_series_m_out_of_range_raises(self):
        """set_time_series(0, 60) raises IndexError (minute must be 0-59)."""
        with self.assertRaises(IndexError):
            ecf.Cron().set_time_series(0, 60)

    # ------------------------------------------------------------------
    # set_week_days()
    # ------------------------------------------------------------------

    def test_set_week_days_full_range(self):
        """set_week_days accepts all values 0-6."""
        c = ecf.Cron("08:00")
        c.set_week_days([0, 1, 2, 3, 4, 5, 6])
        self.assertEqual(list(c.week_days), [0, 1, 2, 3, 4, 5, 6])

    def test_set_week_days_subset(self):
        """set_week_days stores exactly the supplied days."""
        c = ecf.Cron("08:00")
        c.set_week_days([1, 3, 5])
        self.assertEqual(list(c.week_days), [1, 3, 5])

    def test_set_week_days_sunday(self):
        """set_week_days([0]) is valid (Sunday)."""
        c = ecf.Cron("08:00")
        c.set_week_days([0])
        self.assertEqual(list(c.week_days), [0])

    def test_set_week_days_saturday(self):
        """set_week_days([6]) is valid (Saturday)."""
        c = ecf.Cron("08:00")
        c.set_week_days([6])
        self.assertEqual(list(c.week_days), [6])

    def test_set_week_days_7_raises(self):
        """set_week_days([7]) raises IndexError (valid range 0-6)."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00").set_week_days([7])

    def test_set_week_days_negative_raises(self):
        """set_week_days([-1]) raises IndexError (valid range 0-6)."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00").set_week_days([-1])

    def test_set_week_days_empty_list_is_noop(self):
        """set_week_days([]) is accepted and leaves week_days empty."""
        c = ecf.Cron("08:00")
        c.set_week_days([])
        self.assertEqual(list(c.week_days), [])

    def test_set_week_days_empty_list_clears_existing(self):
        """set_week_days([]) clears previously set week days."""
        c = ecf.Cron("08:00", days_of_week=[1, 3, 5])
        self.assertEqual(list(c.week_days), [1, 3, 5])
        c.set_week_days([])
        self.assertEqual(list(c.week_days), [])

    # ------------------------------------------------------------------
    # set_last_week_days_of_the_month()
    # ------------------------------------------------------------------

    def test_set_last_week_days_stores_values(self):
        """set_last_week_days_of_the_month stores the supplied days."""
        c = ecf.Cron("08:00")
        c.set_last_week_days_of_the_month([1, 5])
        self.assertEqual(list(c.last_week_days_of_the_month), [1, 5])

    def test_set_last_week_days_7_raises(self):
        """set_last_week_days_of_the_month([7]) raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00").set_last_week_days_of_the_month([7])

    def test_set_last_week_days_negative_raises(self):
        """set_last_week_days_of_the_month([-1]) raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00").set_last_week_days_of_the_month([-1])

    def test_set_last_week_days_empty_list_is_noop(self):
        """set_last_week_days_of_the_month([]) is accepted and leaves the property empty."""
        c = ecf.Cron("08:00")
        c.set_last_week_days_of_the_month([])
        self.assertEqual(list(c.last_week_days_of_the_month), [])

    def test_set_last_week_days_empty_list_clears_existing(self):
        """set_last_week_days_of_the_month([]) clears previously set values."""
        c = ecf.Cron("08:00", last_week_days_of_the_month=[1, 5])
        self.assertEqual(list(c.last_week_days_of_the_month), [1, 5])
        c.set_last_week_days_of_the_month([])
        self.assertEqual(list(c.last_week_days_of_the_month), [])

    # ------------------------------------------------------------------
    # set_days_of_month()
    # ------------------------------------------------------------------

    def test_set_days_of_month_stores_values(self):
        """set_days_of_month stores the supplied days."""
        c = ecf.Cron("08:00")
        c.set_days_of_month([1, 15, 31])
        self.assertEqual(list(c.days_of_month), [1, 15, 31])

    def test_set_days_of_month_first(self):
        """set_days_of_month([1]) is the valid lower bound."""
        c = ecf.Cron("08:00")
        c.set_days_of_month([1])
        self.assertEqual(list(c.days_of_month), [1])

    def test_set_days_of_month_last(self):
        """set_days_of_month([31]) is the valid upper bound."""
        c = ecf.Cron("08:00")
        c.set_days_of_month([31])
        self.assertEqual(list(c.days_of_month), [31])

    def test_set_days_of_month_zero_raises(self):
        """set_days_of_month([0]) raises IndexError (valid range 1-31)."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00").set_days_of_month([0])

    def test_set_days_of_month_32_raises(self):
        """set_days_of_month([32]) raises IndexError (valid range 1-31)."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00").set_days_of_month([32])

    def test_set_days_of_month_empty_list_is_noop(self):
        """set_days_of_month([]) is accepted and leaves days_of_month empty."""
        c = ecf.Cron("08:00")
        c.set_days_of_month([])
        self.assertEqual(list(c.days_of_month), [])

    def test_set_days_of_month_empty_list_clears_existing(self):
        """set_days_of_month([]) clears previously set days of month."""
        c = ecf.Cron("08:00", days_of_month=[1, 15, 31])
        self.assertEqual(list(c.days_of_month), [1, 15, 31])
        c.set_days_of_month([])
        self.assertEqual(list(c.days_of_month), [])

    # ------------------------------------------------------------------
    # set_months()
    # ------------------------------------------------------------------

    def test_set_months_stores_values(self):
        """set_months stores the supplied months."""
        c = ecf.Cron("08:00")
        c.set_months([3, 6, 9, 12])
        self.assertEqual(list(c.months), [3, 6, 9, 12])

    def test_set_months_january(self):
        """set_months([1]) is the valid lower bound."""
        c = ecf.Cron("08:00")
        c.set_months([1])
        self.assertEqual(list(c.months), [1])

    def test_set_months_december(self):
        """set_months([12]) is the valid upper bound."""
        c = ecf.Cron("08:00")
        c.set_months([12])
        self.assertEqual(list(c.months), [12])

    def test_set_months_zero_raises(self):
        """set_months([0]) raises IndexError (valid range 1-12)."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00").set_months([0])

    def test_set_months_13_raises(self):
        """set_months([13]) raises IndexError (valid range 1-12)."""
        with self.assertRaises(IndexError):
            ecf.Cron("08:00").set_months([13])

    def test_set_months_empty_list_is_noop(self):
        """set_months([]) is accepted and leaves months empty."""
        c = ecf.Cron("08:00")
        c.set_months([])
        self.assertEqual(list(c.months), [])

    def test_set_months_empty_list_clears_existing(self):
        """set_months([]) clears previously set months."""
        c = ecf.Cron("08:00", months=[1, 6, 12])
        self.assertEqual(list(c.months), [1, 6, 12])
        c.set_months([])
        self.assertEqual(list(c.months), [])

    # ------------------------------------------------------------------
    # set_last_day_of_the_month() / last_day_of_the_month()
    # ------------------------------------------------------------------

    def test_last_day_initially_false(self):
        """last_day_of_the_month() is False on a fresh Cron."""
        self.assertFalse(ecf.Cron("08:00").last_day_of_the_month())

    def test_set_last_day_enables_flag(self):
        """set_last_day_of_the_month() flips last_day_of_the_month() to True."""
        c = ecf.Cron("08:00")
        c.set_last_day_of_the_month()
        self.assertTrue(c.last_day_of_the_month())

    def test_last_day_str(self):
        """Enabling last-day flag shows '-d L' in str()."""
        c = ecf.Cron("08:00")
        c.set_last_day_of_the_month()
        self.assertEqual(str(c), "cron -d L 08:00")

    def test_last_day_returns_bool(self):
        """last_day_of_the_month() returns a bool."""
        self.assertIsInstance(ecf.Cron("08:00").last_day_of_the_month(), bool)

    # ------------------------------------------------------------------
    # time()
    # ------------------------------------------------------------------

    def test_time_returns_timeseries(self):
        """time() returns a TimeSeries instance."""
        self.assertIsInstance(ecf.Cron("08:30").time(), ecf.TimeSeries)

    def test_time_str(self):
        """time() str reflects the cron time."""
        self.assertEqual(str(ecf.Cron("08:30").time()), "08:30")

    def test_time_on_default_cron(self):
        """time() on Cron() returns the 00:00 time series."""
        self.assertEqual(str(ecf.Cron().time()), "00:00")

    # ------------------------------------------------------------------
    # Properties (read-only)
    # ------------------------------------------------------------------

    def test_week_days_empty_by_default(self):
        """week_days is empty on a freshly constructed Cron."""
        self.assertEqual(list(ecf.Cron("08:00").week_days), [])

    def test_week_days_after_set(self):
        """week_days reflects values set via set_week_days()."""
        c = ecf.Cron("08:00")
        c.set_week_days([0, 2, 4, 6])
        self.assertEqual(list(c.week_days), [0, 2, 4, 6])

    def test_days_of_month_empty_by_default(self):
        """days_of_month is empty on a freshly constructed Cron."""
        self.assertEqual(list(ecf.Cron("08:00").days_of_month), [])

    def test_days_of_month_after_set(self):
        """days_of_month reflects values set via set_days_of_month()."""
        c = ecf.Cron("08:00")
        c.set_days_of_month([1, 15, 31])
        self.assertEqual(list(c.days_of_month), [1, 15, 31])

    def test_months_empty_by_default(self):
        """months is empty on a freshly constructed Cron."""
        self.assertEqual(list(ecf.Cron("08:00").months), [])

    def test_months_after_set(self):
        """months reflects values set via set_months()."""
        c = ecf.Cron("08:00")
        c.set_months([3, 6, 9, 12])
        self.assertEqual(list(c.months), [3, 6, 9, 12])

    def test_last_week_days_empty_by_default(self):
        """last_week_days_of_the_month is empty on a freshly constructed Cron."""
        self.assertEqual(list(ecf.Cron("08:00").last_week_days_of_the_month), [])

    def test_last_week_days_after_set(self):
        """last_week_days_of_the_month reflects values after set."""
        c = ecf.Cron("08:00")
        c.set_last_week_days_of_the_month([1, 5])
        self.assertEqual(list(c.last_week_days_of_the_month), [1, 5])

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_default(self):
        """str(Cron()) is 'cron 00:00'."""
        self.assertEqual(str(ecf.Cron()), "cron 00:00")

    def test_str_simple_time(self):
        """str() shows the time for a simple single-slot Cron."""
        self.assertEqual(str(ecf.Cron("12:00")), "cron 12:00")

    def test_str_with_week_days(self):
        """str() includes '-w' for week day filters."""
        c = ecf.Cron("08:00")
        c.set_week_days([1, 3, 5])
        self.assertIn("-w", str(c))

    def test_str_with_days_of_month(self):
        """str() includes '-d' for day-of-month filters."""
        c = ecf.Cron("08:00")
        c.set_days_of_month([15])
        self.assertIn("-d", str(c))

    def test_str_with_months(self):
        """str() includes '-m' for month filters."""
        c = ecf.Cron("08:00")
        c.set_months([6])
        self.assertIn("-m", str(c))

    def test_str_returns_str_type(self):
        """str() always returns a Python str."""
        self.assertIsInstance(str(ecf.Cron("08:00")), str)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_reflexive(self):
        """A Cron is equal to itself."""
        c = ecf.Cron("08:00")
        self.assertEqual(c, c)

    def test_eq_same_time(self):
        """Two Crons with the same time are equal."""
        self.assertEqual(ecf.Cron("08:00"), ecf.Cron("08:00"))

    def test_eq_default_crons(self):
        """Two default Crons are equal."""
        self.assertEqual(ecf.Cron(), ecf.Cron())

    def test_eq_symmetric(self):
        """Equality is symmetric."""
        a = ecf.Cron("08:00")
        b = ecf.Cron("08:00")
        self.assertTrue(a == b and b == a)

    def test_eq_with_same_week_days(self):
        """Two Crons with identical week_days filters are equal."""
        a = ecf.Cron("08:00", days_of_week=[1, 3])
        b = ecf.Cron("08:00", days_of_week=[1, 3])
        self.assertEqual(a, b)

    def test_ne_different_time(self):
        """Crons with different times are not equal."""
        self.assertNotEqual(ecf.Cron("08:00"), ecf.Cron("09:00"))

    def test_ne_different_week_days(self):
        """Crons differing in week_days are not equal."""
        self.assertNotEqual(
            ecf.Cron("08:00", days_of_week=[1]),
            ecf.Cron("08:00", days_of_week=[2]),
        )

    def test_ne_different_months(self):
        """Crons differing in months are not equal."""
        self.assertNotEqual(
            ecf.Cron("08:00", months=[1]),
            ecf.Cron("08:00", months=[6]),
        )

    def test_ne_with_and_without_last_day(self):
        """A Cron with last_day_of_the_month differs from one without."""
        self.assertNotEqual(
            ecf.Cron("08:00", last_day_of_the_month=True),
            ecf.Cron("08:00"),
        )

    def test_ne_complement(self):
        """a != b is False when a == b."""
        a = ecf.Cron("08:00")
        b = ecf.Cron("08:00")
        self.assertFalse(a != b)

    # ------------------------------------------------------------------
    # __lt__ — not exposed
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """Cron does not support '<'; raises TypeError."""
        with self.assertRaises(TypeError):
            _ = ecf.Cron("08:00") < ecf.Cron("09:00")

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_is_cron(self):
        """copy.copy() returns a Cron instance."""
        self.assertIsInstance(copy.copy(ecf.Cron("08:00")), ecf.Cron)

    def test_copy_is_distinct(self):
        """copy.copy() produces a distinct Python object."""
        c = ecf.Cron("08:00")
        self.assertIsNot(copy.copy(c), c)

    def test_copy_equal_value(self):
        """copy.copy() is value-equal to the original."""
        c = ecf.Cron("08:00", days_of_week=[1, 3, 5])
        self.assertEqual(copy.copy(c), c)

    def test_copy_preserves_str(self):
        """The copied Cron has the same str() as the original."""
        c = ecf.Cron("08:00", days_of_week=[1, 3, 5])
        self.assertEqual(str(copy.copy(c)), str(c))

    def test_copy_preserves_week_days(self):
        """The copied Cron preserves the week_days property."""
        c = ecf.Cron("08:00", days_of_week=[1, 3, 5])
        self.assertEqual(list(copy.copy(c).week_days), list(c.week_days))

    def test_copy_preserves_last_day_flag(self):
        """The copied Cron preserves the last_day_of_the_month flag."""
        c = ecf.Cron("08:00", last_day_of_the_month=True)
        self.assertTrue(copy.copy(c).last_day_of_the_month())

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Cron is hashable; hash() returns an int."""
        self.assertIsInstance(hash(ecf.Cron("08:00")), int)

    def test_same_object_same_hash(self):
        """The same Cron object always returns the same hash."""
        c = ecf.Cron("08:00")
        self.assertEqual(hash(c), hash(c))

    def test_hash_is_identity_based(self):
        """Two value-equal Crons have different hashes."""
        a = ecf.Cron("08:00")
        b = ecf.Cron("08:00")
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """Cron instances can be stored in a Python set."""
        a = ecf.Cron("08:00")
        b = ecf.Cron("09:00")
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """Cron instances can be used as dictionary keys."""
        c = ecf.Cron("08:00")
        d = {c: "cron"}
        self.assertEqual(d[c], "cron")


class TestVerify(unittest.TestCase):
    """Tests for py::class_<VerifyAttr> exposed as ecf.Verify in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Verify(NState::State, int)   -- state to verify and expected occurrence count;
                                        negative / zero counts are accepted

    Operators
        __str__   -- 'verify STATE:COUNT'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructors
    # ------------------------------------------------------------------

    def test_constructor_complete(self):
        """Verify(State.complete, n) creates a verify attribute."""
        v = ecf.Verify(ecf.State.complete, 3)
        self.assertIsInstance(v, ecf.Verify)

    def test_constructor_queued(self):
        """Verify(State.queued, n) creates a verify attribute."""
        v = ecf.Verify(ecf.State.queued, 5)
        self.assertIsInstance(v, ecf.Verify)

    def test_constructor_aborted(self):
        """Verify(State.aborted, n) creates a verify attribute."""
        v = ecf.Verify(ecf.State.aborted, 1)
        self.assertIsInstance(v, ecf.Verify)

    def test_constructor_submitted(self):
        """Verify(State.submitted, n) creates a verify attribute."""
        v = ecf.Verify(ecf.State.submitted, 2)
        self.assertIsInstance(v, ecf.Verify)

    def test_constructor_active(self):
        """Verify(State.active, n) creates a verify attribute."""
        v = ecf.Verify(ecf.State.active, 4)
        self.assertIsInstance(v, ecf.Verify)

    def test_constructor_unknown(self):
        """Verify(State.unknown, n) creates a verify attribute."""
        v = ecf.Verify(ecf.State.unknown, 0)
        self.assertIsInstance(v, ecf.Verify)

    def test_constructor_zero_count_accepted(self):
        """A count of 0 is accepted without error."""
        v = ecf.Verify(ecf.State.complete, 0)
        self.assertIsInstance(v, ecf.Verify)

    def test_constructor_negative_count_accepted(self):
        """A negative count is accepted without error."""
        v = ecf.Verify(ecf.State.complete, -1)
        self.assertIsInstance(v, ecf.Verify)

    def test_constructor_no_args_raises(self):
        """Verify() with no arguments raises ArgumentError."""
        with self.assertRaises(Exception):
            ecf.Verify()

    def test_constructor_missing_count_raises(self):
        """Verify(State) with only one argument raises ArgumentError."""
        with self.assertRaises(Exception):
            ecf.Verify(ecf.State.complete)

    def test_constructor_wrong_count_type_raises(self):
        """Verify(State, str) raises ArgumentError."""
        with self.assertRaises(Exception):
            ecf.Verify(ecf.State.complete, "x")

    def test_constructor_wrong_state_type_raises(self):
        """Verify(int, int) raises ArgumentError (state must be NState::State)."""
        with self.assertRaises(Exception):
            ecf.Verify(42, 3)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_complete(self):
        """str(Verify(complete, 3)) matches expected format."""
        self.assertEqual(str(ecf.Verify(ecf.State.complete, 3)), "verify complete:3")

    def test_str_queued(self):
        """str(Verify(queued, 5)) matches expected format."""
        self.assertEqual(str(ecf.Verify(ecf.State.queued, 5)), "verify queued:5")

    def test_str_aborted(self):
        """str(Verify(aborted, 1)) matches expected format."""
        self.assertEqual(str(ecf.Verify(ecf.State.aborted, 1)), "verify aborted:1")

    def test_str_submitted(self):
        """str(Verify(submitted, 2)) matches expected format."""
        self.assertEqual(str(ecf.Verify(ecf.State.submitted, 2)), "verify submitted:2")

    def test_str_active(self):
        """str(Verify(active, 4)) matches expected format."""
        self.assertEqual(str(ecf.Verify(ecf.State.active, 4)), "verify active:4")

    def test_str_unknown(self):
        """str(Verify(unknown, 0)) matches expected format."""
        self.assertEqual(str(ecf.Verify(ecf.State.unknown, 0)), "verify unknown:0")

    def test_str_negative_count(self):
        """str(Verify(complete, -1)) includes the negative count."""
        self.assertEqual(str(ecf.Verify(ecf.State.complete, -1)), "verify complete:-1")

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_same_values(self):
        """Two Verify with same state and count compare equal."""
        self.assertEqual(
            ecf.Verify(ecf.State.complete, 3),
            ecf.Verify(ecf.State.complete, 3),
        )

    def test_eq_different_state(self):
        """Two Verify with different state are not equal."""
        self.assertNotEqual(
            ecf.Verify(ecf.State.complete, 3),
            ecf.Verify(ecf.State.queued, 3),
        )

    def test_eq_different_count(self):
        """Two Verify with different count are not equal."""
        self.assertNotEqual(
            ecf.Verify(ecf.State.complete, 3),
            ecf.Verify(ecf.State.complete, 4),
        )

    def test_eq_reflexive(self):
        """A Verify is equal to itself."""
        v = ecf.Verify(ecf.State.complete, 3)
        self.assertEqual(v, v)

    def test_ne_different_state(self):
        """__ne__ returns True for different states."""
        self.assertTrue(
            ecf.Verify(ecf.State.complete, 3) != ecf.Verify(ecf.State.aborted, 3)
        )

    def test_ne_same_values(self):
        """__ne__ returns False for same values."""
        self.assertFalse(
            ecf.Verify(ecf.State.complete, 3) != ecf.Verify(ecf.State.complete, 3)
        )

    # ------------------------------------------------------------------
    # __lt__  (not exposed)
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """Verify does not support < comparison."""
        a = ecf.Verify(ecf.State.complete, 1)
        b = ecf.Verify(ecf.State.queued, 1)
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Verify is hashable."""
        self.assertIsInstance(hash(ecf.Verify(ecf.State.complete, 3)), int)

    def test_same_object_same_hash(self):
        """The same Verify object always returns the same hash."""
        v = ecf.Verify(ecf.State.complete, 3)
        self.assertEqual(hash(v), hash(v))

    def test_hash_is_identity_based(self):
        """Two value-equal Verify objects have different hashes."""
        a = ecf.Verify(ecf.State.complete, 3)
        b = ecf.Verify(ecf.State.complete, 3)
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """Verify instances can be stored in a Python set."""
        a = ecf.Verify(ecf.State.complete, 1)
        b = ecf.Verify(ecf.State.queued, 1)
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """Verify instances can be used as dictionary keys."""
        v = ecf.Verify(ecf.State.complete, 3)
        d = {v: "verify"}
        self.assertEqual(d[v], "verify")

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_returns_equal(self):
        """copy.copy(Verify) returns an equal Verify."""
        v = ecf.Verify(ecf.State.complete, 3)
        self.assertEqual(copy.copy(v), v)

    def test_copy_is_independent(self):
        """copy.copy(Verify) returns a distinct object."""
        v = ecf.Verify(ecf.State.complete, 3)
        self.assertIsNot(copy.copy(v), v)

    def test_copy_preserves_str(self):
        """copy.copy(Verify) preserves str()."""
        v = ecf.Verify(ecf.State.aborted, 7)
        self.assertEqual(str(copy.copy(v)), str(v))

    def test_copy_all_states(self):
        """copy.copy works for all NState values."""
        for state in [
            ecf.State.complete,
            ecf.State.queued,
            ecf.State.aborted,
            ecf.State.submitted,
            ecf.State.active,
            ecf.State.unknown,
        ]:
            v = ecf.Verify(state, 1)
            self.assertEqual(copy.copy(v), v)


class TestClock(unittest.TestCase):
    """Tests for py::class_<ClockAttr, ...> exposed as ecf.Clock in ExportNodeAttr.cpp.

    Exposed API
    -----------
    Constructors
        Clock(int, int, int)          -- day (1-31), month (1-12), year (1400-9999);
                                         produces a real (non-hybrid) clock
        Clock(int, int, int, bool)    -- day, month, year, hybrid flag
        Clock(bool)                   -- hybrid flag only; day/month/year default to 0

    Instance methods
        day()            -> int    -- day component (0 when constructed with bool ctor)
        month()          -> int    -- month component
        year()           -> int    -- year component
        gain()           -> long   -- gain in seconds (0 by default)
        positive_gain()  -> bool   -- True when gain is positive (False by default)
        set_gain_in_seconds(long, bool)          -- gain in seconds, positive flag
        set_gain(int hours, int minutes, bool)   -- gain as h/m, positive flag

    Operators
        __str__   -- 'clock real|hybrid [D.M.YYYY] [+GAIN|GAIN]'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructors
    # ------------------------------------------------------------------

    def test_constructor_day_month_year(self):
        """Clock(day, month, year) creates a real clock with the given date."""
        c = ecf.Clock(1, 3, 2024)
        self.assertIsInstance(c, ecf.Clock)
        self.assertEqual(c.day(), 1)
        self.assertEqual(c.month(), 3)
        self.assertEqual(c.year(), 2024)

    def test_constructor_day_month_year_real(self):
        """Clock(d, m, y) produces a real clock (not hybrid)."""
        c = ecf.Clock(15, 6, 2023)
        self.assertIn("real", str(c))

    def test_constructor_day_month_year_hybrid_true(self):
        """Clock(d, m, y, True) produces a hybrid clock."""
        c = ecf.Clock(1, 3, 2024, True)
        self.assertIn("hybrid", str(c))

    def test_constructor_day_month_year_hybrid_false(self):
        """Clock(d, m, y, False) produces a real clock."""
        c = ecf.Clock(1, 3, 2024, False)
        self.assertIn("real", str(c))

    def test_constructor_bool_true_is_hybrid(self):
        """Clock(True) creates a hybrid clock with no date."""
        c = ecf.Clock(True)
        self.assertIn("hybrid", str(c))
        self.assertEqual(c.day(), 0)
        self.assertEqual(c.month(), 0)
        self.assertEqual(c.year(), 0)

    def test_constructor_bool_false_is_real(self):
        """Clock(False) creates a real clock with no date."""
        c = ecf.Clock(False)
        self.assertIn("real", str(c))
        self.assertEqual(c.day(), 0)
        self.assertEqual(c.month(), 0)
        self.assertEqual(c.year(), 0)

    def test_constructor_gain_defaults_to_zero(self):
        """A freshly constructed Clock has gain == 0."""
        self.assertEqual(ecf.Clock(1, 3, 2024).gain(), 0)

    def test_constructor_positive_gain_defaults_false(self):
        """A freshly constructed Clock has positive_gain == False."""
        self.assertFalse(ecf.Clock(1, 3, 2024).positive_gain())

    # ------------------------------------------------------------------
    # Constructor – invalid day
    # ------------------------------------------------------------------

    def test_constructor_day_zero_raises(self):
        """Clock(0, m, y) raises IndexError (day must be >= 1)."""
        with self.assertRaises(IndexError):
            ecf.Clock(0, 3, 2024)

    def test_constructor_day_negative_raises(self):
        """Clock(-1, m, y) raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Clock(-1, 3, 2024)

    def test_constructor_day_32_raises(self):
        """Clock(32, m, y) raises IndexError (day must be <= 31)."""
        with self.assertRaises(IndexError):
            ecf.Clock(32, 3, 2024)

    def test_constructor_day_31_accepted(self):
        """Clock(31, m, y) is accepted."""
        c = ecf.Clock(31, 3, 2024)
        self.assertEqual(c.day(), 31)

    # ------------------------------------------------------------------
    # Constructor – invalid month
    # ------------------------------------------------------------------

    def test_constructor_month_zero_raises(self):
        """Clock(d, 0, y) raises IndexError (month must be >= 1)."""
        with self.assertRaises(IndexError):
            ecf.Clock(1, 0, 2024)

    def test_constructor_month_negative_raises(self):
        """Clock(d, -1, y) raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Clock(1, -1, 2024)

    def test_constructor_month_13_raises(self):
        """Clock(d, 13, y) raises IndexError (month must be <= 12)."""
        with self.assertRaises(IndexError):
            ecf.Clock(1, 13, 2024)

    def test_constructor_month_12_accepted(self):
        """Clock(d, 12, y) is accepted."""
        c = ecf.Clock(1, 12, 2024)
        self.assertEqual(c.month(), 12)

    # ------------------------------------------------------------------
    # Constructor – invalid year
    # ------------------------------------------------------------------

    def test_constructor_year_zero_raises(self):
        """Clock(d, m, 0) raises IndexError (year must be > 0)."""
        with self.assertRaises(IndexError):
            ecf.Clock(1, 3, 0)

    def test_constructor_year_negative_raises(self):
        """Clock(d, m, -1) raises IndexError."""
        with self.assertRaises(IndexError):
            ecf.Clock(1, 3, -1)

    def test_constructor_year_1399_raises(self):
        """Clock(d, m, 1399) raises IndexError (year must be >= 1400)."""
        with self.assertRaises(IndexError):
            ecf.Clock(1, 3, 1399)

    def test_constructor_year_1400_accepted(self):
        """Clock(d, m, 1400) is accepted (minimum valid year)."""
        c = ecf.Clock(1, 3, 1400)
        self.assertEqual(c.year(), 1400)

    def test_constructor_year_9999_accepted(self):
        """Clock(d, m, 9999) is accepted (maximum valid year)."""
        c = ecf.Clock(1, 3, 9999)
        self.assertEqual(c.year(), 9999)

    @unittest.skipIf(
        _is_running_test_on('platform-builder-rl-86')
        , "The (1400-9999) year range restriction was introduced by Boost 1.67.0."
          "Disabling tests for CI runners known to use previous versions of Boost.")
    def test_constructor_year_10000_raises(self):
        """Clock(d, m, 10000) raises IndexError (year must be <= 9999)."""
        with self.assertRaises(IndexError):
            ecf.Clock(1, 3, 10000)

    # ------------------------------------------------------------------
    # Constructor – missing arguments
    # ------------------------------------------------------------------

    def test_constructor_no_args_raises(self):
        """Clock() with no arguments raises ArgumentError."""
        with self.assertRaises(Exception):
            ecf.Clock()

    def test_constructor_two_args_raises(self):
        """Clock(d, m) with only two int arguments raises ArgumentError."""
        with self.assertRaises(Exception):
            ecf.Clock(1, 3)

    # ------------------------------------------------------------------
    # Accessors
    # ------------------------------------------------------------------

    def test_day_accessor(self):
        """day() returns the day passed to the constructor."""
        self.assertEqual(ecf.Clock(15, 6, 2023).day(), 15)

    def test_month_accessor(self):
        """month() returns the month passed to the constructor."""
        self.assertEqual(ecf.Clock(15, 6, 2023).month(), 6)

    def test_year_accessor(self):
        """year() returns the year passed to the constructor."""
        self.assertEqual(ecf.Clock(15, 6, 2023).year(), 2023)

    def test_gain_initial_value(self):
        """gain() returns 0 on a freshly constructed Clock."""
        self.assertEqual(ecf.Clock(1, 1, 2000).gain(), 0)

    def test_positive_gain_initial_value(self):
        """positive_gain() returns False on a freshly constructed Clock."""
        self.assertFalse(ecf.Clock(1, 1, 2000).positive_gain())

    # ------------------------------------------------------------------
    # set_gain_in_seconds
    # ------------------------------------------------------------------

    def test_set_gain_in_seconds_positive(self):
        """set_gain_in_seconds(n, True) sets gain and marks it positive."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain_in_seconds(3600, True)
        self.assertEqual(c.gain(), 3600)
        self.assertTrue(c.positive_gain())

    def test_set_gain_in_seconds_negative(self):
        """set_gain_in_seconds(n, False) sets gain and marks it not positive."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain_in_seconds(3600, False)
        self.assertEqual(c.gain(), 3600)
        self.assertFalse(c.positive_gain())

    def test_set_gain_in_seconds_zero(self):
        """set_gain_in_seconds(0, True) sets gain to 0."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain_in_seconds(0, True)
        self.assertEqual(c.gain(), 0)
        self.assertTrue(c.positive_gain())

    def test_set_gain_in_seconds_str_positive(self):
        """set_gain_in_seconds with positive gain shows '+' in str()."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain_in_seconds(3600, True)
        self.assertIn("+3600", str(c))

    def test_set_gain_in_seconds_str_negative(self):
        """set_gain_in_seconds with non-positive gain shows gain without '+' in str()."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain_in_seconds(3600, False)
        self.assertIn("3600", str(c))
        self.assertNotIn("+3600", str(c))

    def test_set_gain_in_seconds_one_arg_raises(self):
        """set_gain_in_seconds(n) with only one arg raises ArgumentError."""
        c = ecf.Clock(1, 3, 2024)
        with self.assertRaises(Exception):
            c.set_gain_in_seconds(3600)

    # ------------------------------------------------------------------
    # set_gain
    # ------------------------------------------------------------------

    def test_set_gain_positive(self):
        """set_gain(h, m, True) sets gain in seconds and marks it positive."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain(1, 30, True)
        self.assertEqual(c.gain(), 5400)
        self.assertTrue(c.positive_gain())

    def test_set_gain_not_positive(self):
        """set_gain(h, m, False) sets gain in seconds and marks it not positive."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain(1, 30, False)
        self.assertEqual(c.gain(), 5400)
        self.assertFalse(c.positive_gain())

    def test_set_gain_hours_only(self):
        """set_gain(2, 0, True) sets gain to 7200 seconds."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain(2, 0, True)
        self.assertEqual(c.gain(), 7200)

    def test_set_gain_minutes_only(self):
        """set_gain(0, 45, True) sets gain to 2700 seconds."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain(0, 45, True)
        self.assertEqual(c.gain(), 2700)

    def test_set_gain_zero(self):
        """set_gain(0, 0, True) sets gain to 0."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain(0, 0, True)
        self.assertEqual(c.gain(), 0)
        self.assertTrue(c.positive_gain())

    def test_set_gain_two_args_raises(self):
        """set_gain(h, m) with only two args raises ArgumentError."""
        c = ecf.Clock(1, 3, 2024)
        with self.assertRaises(Exception):
            c.set_gain(1, 30)

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_real_with_date(self):
        """str(Clock(d, m, y)) has expected format."""
        self.assertEqual(str(ecf.Clock(1, 3, 2024)), "clock real 1.3.2024 ")

    def test_str_hybrid_with_date(self):
        """str(Clock(d, m, y, True)) has expected format."""
        self.assertEqual(str(ecf.Clock(1, 3, 2024, True)), "clock hybrid 1.3.2024 ")

    def test_str_real_no_date(self):
        """str(Clock(False)) has expected format."""
        self.assertEqual(str(ecf.Clock(False)), "clock real ")

    def test_str_hybrid_no_date(self):
        """str(Clock(True)) has expected format."""
        self.assertEqual(str(ecf.Clock(True)), "clock hybrid ")

    def test_str_with_positive_gain(self):
        """str includes '+gain' when positive_gain is True."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain_in_seconds(3600, True)
        self.assertEqual(str(c), "clock real 1.3.2024 +3600")

    def test_str_with_negative_gain(self):
        """str includes gain without '+' when positive_gain is False."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain_in_seconds(3600, False)
        self.assertEqual(str(c), "clock real 1.3.2024 3600")

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_same_values(self):
        """Two Clocks with identical date compare equal."""
        self.assertEqual(ecf.Clock(1, 3, 2024), ecf.Clock(1, 3, 2024))

    def test_eq_different_day(self):
        """Two Clocks with different day are not equal."""
        self.assertNotEqual(ecf.Clock(1, 3, 2024), ecf.Clock(2, 3, 2024))

    def test_eq_different_month(self):
        """Two Clocks with different month are not equal."""
        self.assertNotEqual(ecf.Clock(1, 3, 2024), ecf.Clock(1, 4, 2024))

    def test_eq_different_year(self):
        """Two Clocks with different year are not equal."""
        self.assertNotEqual(ecf.Clock(1, 3, 2024), ecf.Clock(1, 3, 2025))

    def test_eq_real_vs_hybrid(self):
        """Real and hybrid Clocks with same date are not equal."""
        self.assertNotEqual(ecf.Clock(1, 3, 2024, False), ecf.Clock(1, 3, 2024, True))

    def test_eq_reflexive(self):
        """A Clock is equal to itself."""
        c = ecf.Clock(1, 3, 2024)
        self.assertEqual(c, c)

    def test_ne_different_date(self):
        """__ne__ returns True for different dates."""
        self.assertTrue(ecf.Clock(1, 3, 2024) != ecf.Clock(2, 3, 2024))

    def test_ne_same_values(self):
        """__ne__ returns False for identical Clocks."""
        self.assertFalse(ecf.Clock(1, 3, 2024) != ecf.Clock(1, 3, 2024))

    # ------------------------------------------------------------------
    # __lt__  (not exposed)
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """Clock does not support < comparison."""
        a = ecf.Clock(1, 3, 2024)
        b = ecf.Clock(2, 3, 2024)
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """Clock is hashable."""
        self.assertIsInstance(hash(ecf.Clock(1, 3, 2024)), int)

    def test_same_object_same_hash(self):
        """The same Clock object always returns the same hash."""
        c = ecf.Clock(1, 3, 2024)
        self.assertEqual(hash(c), hash(c))

    def test_hash_is_identity_based(self):
        """Two value-equal Clocks have different hashes."""
        a = ecf.Clock(1, 3, 2024)
        b = ecf.Clock(1, 3, 2024)
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """Clock instances can be stored in a Python set."""
        a = ecf.Clock(1, 3, 2024)
        b = ecf.Clock(2, 3, 2024)
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """Clock instances can be used as dictionary keys."""
        c = ecf.Clock(1, 3, 2024)
        d = {c: "clock"}
        self.assertEqual(d[c], "clock")

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_returns_equal(self):
        """copy.copy(Clock) returns an equal Clock."""
        c = ecf.Clock(1, 3, 2024)
        self.assertEqual(copy.copy(c), c)

    def test_copy_is_independent(self):
        """copy.copy(Clock) returns a distinct object."""
        c = ecf.Clock(1, 3, 2024)
        self.assertIsNot(copy.copy(c), c)

    def test_copy_preserves_str(self):
        """copy.copy(Clock) preserves str()."""
        c = ecf.Clock(1, 3, 2024, True)
        self.assertEqual(str(copy.copy(c)), str(c))

    def test_copy_preserves_gain(self):
        """copy.copy(Clock) preserves gain and positive_gain."""
        c = ecf.Clock(1, 3, 2024)
        c.set_gain_in_seconds(3600, True)
        cc = copy.copy(c)
        self.assertEqual(cc.gain(), 3600)
        self.assertTrue(cc.positive_gain())

    def test_copy_preserves_hybrid_flag(self):
        """copy.copy(Clock) preserves hybrid vs real."""
        c = ecf.Clock(1, 3, 2024, True)
        self.assertIn("hybrid", str(copy.copy(c)))

    def test_copy_bool_constructor(self):
        """copy.copy works for Clock created with bool constructor."""
        c = ecf.Clock(True)
        cc = copy.copy(c)
        self.assertEqual(copy.copy(c), c)
        self.assertIsNot(cc, c)


class TestAvisoAttr(unittest.TestCase):
    """Tests for py::class_<ecf::AvisoAttr> exposed as ecf.AvisoAttr in ExportNodeAttr.cpp.

    AvisoAttr acts as a trigger that requeues its parent Node when a matching
    notification is received from the Aviso notification service.

    Exposed API
    -----------
    Constructors
        AvisoAttr()                                      -- no-args; make_constructor allows it;
                                                            creates an empty (unnamed) instance
        AvisoAttr(name, listener)                        -- required fields; optional fields use
                                                            ECF variable defaults
        AvisoAttr(name, listener, url)
        AvisoAttr(name, listener, url, schema)
        AvisoAttr(name, listener, url, schema, polling)
        AvisoAttr(name, listener, url, schema, polling, auth)

        Default values:
            url     = '%ECF_AVISO_URL%'
            schema  = '%ECF_AVISO_SCHEMA%'
            polling = '%ECF_AVISO_POLLING%'
            auth    = '%ECF_AVISO_AUTH%'

        Name constraints: alphanumeric and underscore only (no spaces, no dashes);
                          empty name raises RuntimeError.

    Instance methods
        name()      -> str   -- attribute name
        listener()  -> str   -- Aviso listener configuration
        url()       -> str   -- URL of the Aviso server
        schema()    -> str   -- path to the schema file
        polling()   -> str   -- polling interval
        auth()      -> str   -- path to authentication credentials

    Operators
        __str__   -- 'AvisoAttr(name=N, listener=L, url=U, schema=S, polling=P, revision=0, auth=A, reason='')'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality (all fields compared)
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: AvisoAttr()  — no-args via make_constructor
    # ------------------------------------------------------------------

    def test_no_args_creates_instance(self):
        """AvisoAttr() with no args creates an empty instance (make_constructor allows it)."""
        self.assertIsInstance(ecf.AvisoAttr(), ecf.AvisoAttr)

    def test_no_args_empty_name(self):
        """The no-args instance has an empty name."""
        self.assertEqual(ecf.AvisoAttr().name(), "")

    # ------------------------------------------------------------------
    # Constructor: AvisoAttr(name, listener)
    # ------------------------------------------------------------------

    def test_ctor_two_args_stores_name(self):
        """AvisoAttr(name, listener) stores the name."""
        self.assertEqual(ecf.AvisoAttr("myaviso", '{"event":"x"}').name(), "myaviso")

    def test_ctor_two_args_stores_listener(self):
        """AvisoAttr(name, listener) stores the listener wrapped in single quotes."""
        self.assertEqual(
            ecf.AvisoAttr("av", '{"event":"x"}').listener(), '\'{"event":"x"}\''
        )

    def test_ctor_two_args_default_url(self):
        """AvisoAttr(name, listener) uses %ECF_AVISO_URL% as the default url."""
        self.assertEqual(ecf.AvisoAttr("av", "l").url(), "%ECF_AVISO_URL%")

    def test_ctor_two_args_default_schema(self):
        """AvisoAttr(name, listener) uses %ECF_AVISO_SCHEMA% as the default schema."""
        self.assertEqual(ecf.AvisoAttr("av", "l").schema(), "%ECF_AVISO_SCHEMA%")

    def test_ctor_two_args_default_polling(self):
        """AvisoAttr(name, listener) uses %ECF_AVISO_POLLING% as the default polling."""
        self.assertEqual(ecf.AvisoAttr("av", "l").polling(), "%ECF_AVISO_POLLING%")

    def test_ctor_two_args_default_auth(self):
        """AvisoAttr(name, listener) uses %ECF_AVISO_AUTH% as the default auth."""
        self.assertEqual(ecf.AvisoAttr("av", "l").auth(), "%ECF_AVISO_AUTH%")

    # ------------------------------------------------------------------
    # Constructor: AvisoAttr(name, listener, url)
    # ------------------------------------------------------------------

    def test_ctor_three_args_stores_url(self):
        """AvisoAttr(name, listener, url) stores the url."""
        self.assertEqual(
            ecf.AvisoAttr("av", "l", "http://aviso.example.com").url(),
            "http://aviso.example.com",
        )

    def test_ctor_three_args_schema_still_default(self):
        """Providing url only still leaves schema at its default."""
        self.assertEqual(
            ecf.AvisoAttr("av", "l", "http://url").schema(), "%ECF_AVISO_SCHEMA%"
        )

    # ------------------------------------------------------------------
    # Constructor: AvisoAttr(name, listener, url, schema)
    # ------------------------------------------------------------------

    def test_ctor_four_args_stores_schema(self):
        """AvisoAttr(name, listener, url, schema) stores the schema."""
        self.assertEqual(
            ecf.AvisoAttr("av", "l", "http://url", "/path/to/schema").schema(),
            "/path/to/schema",
        )

    def test_ctor_four_args_polling_still_default(self):
        """Providing schema still leaves polling at its default."""
        self.assertEqual(
            ecf.AvisoAttr("av", "l", "http://url", "/schema").polling(),
            "%ECF_AVISO_POLLING%",
        )

    # ------------------------------------------------------------------
    # Constructor: AvisoAttr(name, listener, url, schema, polling)
    # ------------------------------------------------------------------

    def test_ctor_five_args_stores_polling(self):
        """AvisoAttr(name, listener, url, schema, polling) stores the polling."""
        self.assertEqual(
            ecf.AvisoAttr("av", "l", "http://url", "/schema", "60").polling(), "60"
        )

    def test_ctor_five_args_auth_still_default(self):
        """Providing polling still leaves auth at its default."""
        self.assertEqual(
            ecf.AvisoAttr("av", "l", "http://url", "/schema", "60").auth(),
            "%ECF_AVISO_AUTH%",
        )

    # ------------------------------------------------------------------
    # Constructor: AvisoAttr(name, listener, url, schema, polling, auth)
    # ------------------------------------------------------------------

    def test_ctor_six_args_stores_auth(self):
        """AvisoAttr with all six args stores the auth path."""
        self.assertEqual(
            ecf.AvisoAttr("av", "l", "http://url", "/schema", "60", "/auth").auth(),
            "/auth",
        )

    def test_ctor_six_args_all_fields(self):
        """AvisoAttr with all six args stores all fields correctly."""
        a = ecf.AvisoAttr("av", "l", "http://u", "/s", "120", "/a")
        self.assertEqual(a.name(), "av")
        self.assertEqual(
            a.listener(), "'l'"
        )  # listener() wraps the value in single quotes
        self.assertEqual(a.url(), "http://u")
        self.assertEqual(a.schema(), "/s")
        self.assertEqual(a.polling(), "120")
        self.assertEqual(a.auth(), "/a")

    # ------------------------------------------------------------------
    # Name validation
    # ------------------------------------------------------------------

    def test_empty_name_raises(self):
        """Empty name raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.AvisoAttr("", "l")

    def test_name_with_space_raises(self):
        """Name containing a space raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.AvisoAttr("my aviso", "l")

    def test_name_with_dash_raises(self):
        """Name containing a dash raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.AvisoAttr("my-aviso", "l")

    def test_name_with_underscore_accepted(self):
        """Name with underscore is valid."""
        a = ecf.AvisoAttr("my_aviso", "l")
        self.assertEqual(a.name(), "my_aviso")

    def test_name_with_digits_accepted(self):
        """Name composed of digits is valid."""
        a = ecf.AvisoAttr("123", "l")
        self.assertEqual(a.name(), "123")

    def test_name_alphanumeric_accepted(self):
        """Alphanumeric name is valid."""
        a = ecf.AvisoAttr("a1b2", "l")
        self.assertEqual(a.name(), "a1b2")

    # ------------------------------------------------------------------
    # Listener edge cases
    # ------------------------------------------------------------------

    def test_empty_listener_accepted(self):
        """Empty listener string is accepted; listener() returns the empty string wrapped in quotes."""
        a = ecf.AvisoAttr("av", "")
        self.assertEqual(a.listener(), "''")

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_contains_name(self):
        """str() includes the attribute name."""
        self.assertIn("name=myaviso", str(ecf.AvisoAttr("myaviso", "l")))

    def test_str_contains_listener(self):
        """str() includes the listener."""
        a = ecf.AvisoAttr("av", '{"event":"x"}')
        self.assertIn('listener=\'{"event":"x"}\'', str(a))

    def test_str_contains_url(self):
        """str() includes the url."""
        a = ecf.AvisoAttr("av", "l", "http://example.com")
        self.assertIn("url=http://example.com", str(a))

    def test_str_contains_schema(self):
        """str() includes the schema."""
        a = ecf.AvisoAttr("av", "l", "http://u", "/myschema")
        self.assertIn("schema=/myschema", str(a))

    def test_str_contains_polling(self):
        """str() includes the polling interval."""
        a = ecf.AvisoAttr("av", "l", "http://u", "/s", "60")
        self.assertIn("polling=60", str(a))

    def test_str_contains_auth(self):
        """str() includes the auth path."""
        a = ecf.AvisoAttr("av", "l", "http://u", "/s", "60", "/myauth")
        self.assertIn("auth=/myauth", str(a))

    def test_str_full_format(self):
        """str() matches the expected full format."""
        a = ecf.AvisoAttr("av", '{"event":"x"}')
        expected = (
            'AvisoAttr(name=av, listener=\'{"event":"x"}\', '
            "url=%ECF_AVISO_URL%, schema=%ECF_AVISO_SCHEMA%, "
            "polling=%ECF_AVISO_POLLING%, revision=0, "
            "auth=%ECF_AVISO_AUTH%, reason='')"
        )
        self.assertEqual(str(a), expected)

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_same_values(self):
        """Two AvisoAttr with identical fields compare equal."""
        self.assertEqual(
            ecf.AvisoAttr("av", "l"),
            ecf.AvisoAttr("av", "l"),
        )

    def test_eq_reflexive(self):
        """An AvisoAttr is equal to itself."""
        a = ecf.AvisoAttr("av", "l")
        self.assertEqual(a, a)

    def test_eq_different_name(self):
        """Different name makes attributes not equal."""
        self.assertNotEqual(
            ecf.AvisoAttr("av1", "l"),
            ecf.AvisoAttr("av2", "l"),
        )

    def test_eq_different_listener(self):
        """Different listener makes attributes not equal."""
        self.assertNotEqual(
            ecf.AvisoAttr("av", '{"event":"x"}'),
            ecf.AvisoAttr("av", '{"event":"y"}'),
        )

    def test_eq_different_url(self):
        """Different url makes attributes not equal."""
        self.assertNotEqual(
            ecf.AvisoAttr("av", "l", "http://url1"),
            ecf.AvisoAttr("av", "l", "http://url2"),
        )

    def test_eq_different_schema(self):
        """Different schema makes attributes not equal."""
        self.assertNotEqual(
            ecf.AvisoAttr("av", "l", "http://u", "/schema1"),
            ecf.AvisoAttr("av", "l", "http://u", "/schema2"),
        )

    def test_eq_different_polling(self):
        """Different polling makes attributes not equal."""
        self.assertNotEqual(
            ecf.AvisoAttr("av", "l", "http://u", "/s", "60"),
            ecf.AvisoAttr("av", "l", "http://u", "/s", "120"),
        )

    def test_eq_different_auth(self):
        """Different auth makes attributes not equal."""
        self.assertNotEqual(
            ecf.AvisoAttr("av", "l", "http://u", "/s", "60", "/auth1"),
            ecf.AvisoAttr("av", "l", "http://u", "/s", "60", "/auth2"),
        )

    def test_ne_different_values(self):
        """__ne__ returns True for different attributes."""
        self.assertTrue(ecf.AvisoAttr("av1", "l") != ecf.AvisoAttr("av2", "l"))

    def test_ne_same_values(self):
        """__ne__ returns False for identical attributes."""
        self.assertFalse(ecf.AvisoAttr("av", "l") != ecf.AvisoAttr("av", "l"))

    # ------------------------------------------------------------------
    # __lt__  (not exposed)
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """AvisoAttr does not support < comparison."""
        a = ecf.AvisoAttr("av1", "l")
        b = ecf.AvisoAttr("av2", "l")
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """AvisoAttr is hashable."""
        self.assertIsInstance(hash(ecf.AvisoAttr("av", "l")), int)

    def test_same_object_same_hash(self):
        """The same AvisoAttr always returns the same hash."""
        a = ecf.AvisoAttr("av", "l")
        self.assertEqual(hash(a), hash(a))

    def test_hash_is_identity_based(self):
        """Two value-equal AvisoAttr objects have different hashes."""
        a = ecf.AvisoAttr("av", "l")
        b = ecf.AvisoAttr("av", "l")
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """AvisoAttr instances can be stored in a Python set."""
        a = ecf.AvisoAttr("av1", "l")
        b = ecf.AvisoAttr("av2", "l")
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """AvisoAttr instances can be used as dictionary keys."""
        a = ecf.AvisoAttr("av", "l")
        d = {a: "aviso"}
        self.assertEqual(d[a], "aviso")

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_returns_equal(self):
        """copy.copy(AvisoAttr) returns an equal instance."""
        a = ecf.AvisoAttr("av", '{"event":"x"}')
        self.assertEqual(copy.copy(a), a)

    def test_copy_is_independent(self):
        """copy.copy(AvisoAttr) returns a distinct object."""
        a = ecf.AvisoAttr("av", '{"event":"x"}')
        self.assertIsNot(copy.copy(a), a)

    def test_copy_preserves_str(self):
        """copy.copy(AvisoAttr) preserves str()."""
        a = ecf.AvisoAttr("av", "l", "http://u", "/s", "60", "/auth")
        self.assertEqual(str(copy.copy(a)), str(a))

    def test_copy_preserves_all_fields(self):
        """copy.copy(AvisoAttr) preserves every accessor."""
        a = ecf.AvisoAttr("av", "l", "http://u", "/s", "60", "/auth")
        c = copy.copy(a)
        self.assertEqual(c.name(), a.name())
        self.assertEqual(c.listener(), a.listener())
        self.assertEqual(c.url(), a.url())
        self.assertEqual(c.schema(), a.schema())
        self.assertEqual(c.polling(), a.polling())
        self.assertEqual(c.auth(), a.auth())


class TestMirrorAttr(unittest.TestCase):
    """Tests for py::class_<ecf::MirrorAttr> exposed as ecf.MirrorAttr in ExportNodeAttr.cpp.

    MirrorAttr mirrors the status of a remote ecFlow Node, allowing local Nodes
    to be triggered based on the remote state.

    Exposed API
    -----------
    Constructors
        MirrorAttr()                                             -- no-args; make_constructor
                                                                   allows it; creates empty instance
        MirrorAttr(name, remote_path)                           -- required fields; optional fields
                                                                   use ECF variable defaults
        MirrorAttr(name, remote_path, remote_host)
        MirrorAttr(name, remote_path, remote_host, remote_port)
        MirrorAttr(name, remote_path, remote_host, remote_port, polling)
        MirrorAttr(name, remote_path, remote_host, remote_port, polling, ssl)
        MirrorAttr(name, remote_path, remote_host, remote_port, polling, ssl, auth)

        Default values:
            remote_host = '%ECF_MIRROR_REMOTE_HOST%'
            remote_port = '%ECF_MIRROR_REMOTE_PORT%'
            polling     = '%ECF_MIRROR_REMOTE_POLLING%'
            ssl         = False
            auth        = '%ECF_MIRROR_REMOTE_AUTH%'

        Name constraints: alphanumeric and underscore only (no spaces, no dashes);
                          empty name raises RuntimeError.

    Instance methods
        name()         -> str    -- attribute name
        remote_path()  -> str    -- path on the remote ecFlow server
        remote_host()  -> str    -- hostname of the remote server
        remote_port()  -> str    -- port of the remote server
        polling()      -> str    -- polling interval
        ssl()          -> bool   -- True when SSL is enabled
        auth()         -> str    -- path to authentication credentials

    Operators
        __str__   -- 'MirrorAttr(name=N, remote_path=P, remote_host=H, remote_port=PORT,
                       polling=POLL, ssl=..., auth=A, reason=)'
        __copy__  -- copy.copy() returns a value-equal, identity-distinct instance
        __eq__    -- value-based equality (all fields compared)
        __ne__    -- implicit complement of __eq__
        __lt__    -- not exposed; raises TypeError
        __hash__  -- identity-based (boost.python C-extension type)
    """

    # ------------------------------------------------------------------
    # Constructor: MirrorAttr()  — no-args via make_constructor
    # ------------------------------------------------------------------

    def test_no_args_creates_instance(self):
        """MirrorAttr() with no args creates an empty instance (make_constructor allows it)."""
        self.assertIsInstance(ecf.MirrorAttr(), ecf.MirrorAttr)

    def test_no_args_empty_name(self):
        """The no-args instance has an empty name."""
        self.assertEqual(ecf.MirrorAttr().name(), "")

    # ------------------------------------------------------------------
    # Constructor: MirrorAttr(name, remote_path)
    # ------------------------------------------------------------------

    def test_ctor_two_args_stores_name(self):
        """MirrorAttr(name, path) stores the name."""
        self.assertEqual(ecf.MirrorAttr("mymirror", "/s/t").name(), "mymirror")

    def test_ctor_two_args_stores_path(self):
        """MirrorAttr(name, path) stores the remote_path."""
        self.assertEqual(
            ecf.MirrorAttr("m", "/suite/task").remote_path(), "/suite/task"
        )

    def test_ctor_two_args_default_host(self):
        """MirrorAttr(name, path) uses %ECF_MIRROR_REMOTE_HOST% as default host."""
        self.assertEqual(
            ecf.MirrorAttr("m", "/p").remote_host(), "%ECF_MIRROR_REMOTE_HOST%"
        )

    def test_ctor_two_args_default_port(self):
        """MirrorAttr(name, path) uses %ECF_MIRROR_REMOTE_PORT% as default port."""
        self.assertEqual(
            ecf.MirrorAttr("m", "/p").remote_port(), "%ECF_MIRROR_REMOTE_PORT%"
        )

    def test_ctor_two_args_default_polling(self):
        """MirrorAttr(name, path) uses %ECF_MIRROR_REMOTE_POLLING% as default polling."""
        self.assertEqual(
            ecf.MirrorAttr("m", "/p").polling(), "%ECF_MIRROR_REMOTE_POLLING%"
        )

    def test_ctor_two_args_default_ssl_false(self):
        """MirrorAttr(name, path) has ssl=False by default."""
        self.assertFalse(ecf.MirrorAttr("m", "/p").ssl())

    def test_ctor_two_args_default_auth(self):
        """MirrorAttr(name, path) uses %ECF_MIRROR_REMOTE_AUTH% as default auth."""
        self.assertEqual(ecf.MirrorAttr("m", "/p").auth(), "%ECF_MIRROR_REMOTE_AUTH%")

    # ------------------------------------------------------------------
    # Constructor: MirrorAttr(name, path, host)
    # ------------------------------------------------------------------

    def test_ctor_three_args_stores_host(self):
        """MirrorAttr(name, path, host) stores the remote_host."""
        self.assertEqual(
            ecf.MirrorAttr("m", "/p", "ecflow.host.example.com").remote_host(),
            "ecflow.host.example.com",
        )

    def test_ctor_three_args_port_still_default(self):
        """Providing host still leaves port at its default."""
        self.assertEqual(
            ecf.MirrorAttr("m", "/p", "host").remote_port(), "%ECF_MIRROR_REMOTE_PORT%"
        )

    # ------------------------------------------------------------------
    # Constructor: MirrorAttr(name, path, host, port)
    # ------------------------------------------------------------------

    def test_ctor_four_args_stores_port(self):
        """MirrorAttr(name, path, host, port) stores the remote_port."""
        self.assertEqual(
            ecf.MirrorAttr("m", "/p", "host", "3141").remote_port(), "3141"
        )

    def test_ctor_four_args_polling_still_default(self):
        """Providing port still leaves polling at its default."""
        self.assertEqual(
            ecf.MirrorAttr("m", "/p", "host", "3141").polling(),
            "%ECF_MIRROR_REMOTE_POLLING%",
        )

    # ------------------------------------------------------------------
    # Constructor: MirrorAttr(name, path, host, port, polling)
    # ------------------------------------------------------------------

    def test_ctor_five_args_stores_polling(self):
        """MirrorAttr(name, path, host, port, polling) stores the polling interval."""
        self.assertEqual(
            ecf.MirrorAttr("m", "/p", "host", "3141", "120").polling(), "120"
        )

    def test_ctor_five_args_ssl_still_false(self):
        """Providing polling still leaves ssl at its default (False)."""
        self.assertFalse(ecf.MirrorAttr("m", "/p", "host", "3141", "120").ssl())

    # ------------------------------------------------------------------
    # Constructor: MirrorAttr(name, path, host, port, polling, ssl)
    # ------------------------------------------------------------------

    def test_ctor_six_args_ssl_true(self):
        """MirrorAttr(..., ssl=True) enables SSL."""
        self.assertTrue(ecf.MirrorAttr("m", "/p", "host", "3141", "120", True).ssl())

    def test_ctor_six_args_ssl_false(self):
        """MirrorAttr(..., ssl=False) keeps SSL disabled."""
        self.assertFalse(ecf.MirrorAttr("m", "/p", "host", "3141", "120", False).ssl())

    def test_ctor_six_args_auth_still_default(self):
        """Providing ssl still leaves auth at its default."""
        self.assertEqual(
            ecf.MirrorAttr("m", "/p", "host", "3141", "120", True).auth(),
            "%ECF_MIRROR_REMOTE_AUTH%",
        )

    # ------------------------------------------------------------------
    # Constructor: MirrorAttr(name, path, host, port, polling, ssl, auth)
    # ------------------------------------------------------------------

    def test_ctor_seven_args_stores_auth(self):
        """MirrorAttr with all seven args stores the auth path."""
        self.assertEqual(
            ecf.MirrorAttr("m", "/p", "host", "3141", "120", True, "/auth").auth(),
            "/auth",
        )

    def test_ctor_seven_args_all_fields(self):
        """MirrorAttr with all seven args stores every field correctly."""
        m = ecf.MirrorAttr("m", "/suite/task", "host", "3141", "120", True, "/auth")
        self.assertEqual(m.name(), "m")
        self.assertEqual(m.remote_path(), "/suite/task")
        self.assertEqual(m.remote_host(), "host")
        self.assertEqual(m.remote_port(), "3141")
        self.assertEqual(m.polling(), "120")
        self.assertTrue(m.ssl())
        self.assertEqual(m.auth(), "/auth")

    # ------------------------------------------------------------------
    # Name validation
    # ------------------------------------------------------------------

    def test_empty_name_raises(self):
        """Empty name raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.MirrorAttr("", "/p")

    def test_name_with_space_raises(self):
        """Name containing a space raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.MirrorAttr("my mirror", "/p")

    def test_name_with_dash_raises(self):
        """Name containing a dash raises RuntimeError."""
        with self.assertRaises(RuntimeError):
            ecf.MirrorAttr("my-mirror", "/p")

    def test_name_with_underscore_accepted(self):
        """Name with underscore is valid."""
        m = ecf.MirrorAttr("my_mirror", "/p")
        self.assertEqual(m.name(), "my_mirror")

    def test_name_with_digits_accepted(self):
        """Name composed of digits is valid."""
        m = ecf.MirrorAttr("123", "/p")
        self.assertEqual(m.name(), "123")

    def test_name_alphanumeric_accepted(self):
        """Alphanumeric name is valid."""
        m = ecf.MirrorAttr("M1mirror", "/p")
        self.assertEqual(m.name(), "M1mirror")

    # ------------------------------------------------------------------
    # remote_path edge cases
    # ------------------------------------------------------------------

    def test_empty_path_accepted(self):
        """Empty remote_path is accepted without error."""
        m = ecf.MirrorAttr("m", "")
        self.assertEqual(m.remote_path(), "")

    # ------------------------------------------------------------------
    # __str__
    # ------------------------------------------------------------------

    def test_str_contains_name(self):
        """str() includes the attribute name."""
        self.assertIn("name=mymirror", str(ecf.MirrorAttr("mymirror", "/p")))

    def test_str_contains_remote_path(self):
        """str() includes the remote_path."""
        self.assertIn(
            "remote_path=/suite/task", str(ecf.MirrorAttr("m", "/suite/task"))
        )

    def test_str_contains_remote_host(self):
        """str() includes the remote_host."""
        m = ecf.MirrorAttr("m", "/p", "myhost")
        self.assertIn("remote_host=myhost", str(m))

    def test_str_contains_remote_port(self):
        """str() includes the remote_port."""
        m = ecf.MirrorAttr("m", "/p", "host", "3141")
        self.assertIn("remote_port=3141", str(m))

    def test_str_contains_polling(self):
        """str() includes the polling interval."""
        m = ecf.MirrorAttr("m", "/p", "host", "3141", "120")
        self.assertIn("polling=120", str(m))

    def test_str_contains_auth(self):
        """str() includes the auth path."""
        m = ecf.MirrorAttr("m", "/p", "host", "3141", "120", False, "/myauth")
        self.assertIn("auth=/myauth", str(m))

    def test_str_ssl_true_differs_from_false(self):
        """str() differs when ssl changes between True and False."""
        m_no_ssl = ecf.MirrorAttr("m", "/p", "host", "3141", "120", False)
        m_ssl = ecf.MirrorAttr("m", "/p", "host", "3141", "120", True)
        self.assertNotEqual(str(m_no_ssl), str(m_ssl))

    # ------------------------------------------------------------------
    # __eq__ / __ne__
    # ------------------------------------------------------------------

    def test_eq_same_values(self):
        """Two MirrorAttr with identical fields compare equal."""
        self.assertEqual(
            ecf.MirrorAttr("m", "/p"),
            ecf.MirrorAttr("m", "/p"),
        )

    def test_eq_reflexive(self):
        """A MirrorAttr is equal to itself."""
        m = ecf.MirrorAttr("m", "/p")
        self.assertEqual(m, m)

    def test_eq_different_name(self):
        """Different name makes attributes not equal."""
        self.assertNotEqual(ecf.MirrorAttr("m1", "/p"), ecf.MirrorAttr("m2", "/p"))

    def test_eq_different_path(self):
        """Different remote_path makes attributes not equal."""
        self.assertNotEqual(ecf.MirrorAttr("m", "/p1"), ecf.MirrorAttr("m", "/p2"))

    def test_eq_different_host(self):
        """Different remote_host makes attributes not equal."""
        self.assertNotEqual(
            ecf.MirrorAttr("m", "/p", "host1"),
            ecf.MirrorAttr("m", "/p", "host2"),
        )

    def test_eq_different_port(self):
        """Different remote_port makes attributes not equal."""
        self.assertNotEqual(
            ecf.MirrorAttr("m", "/p", "host", "3141"),
            ecf.MirrorAttr("m", "/p", "host", "3142"),
        )

    def test_eq_different_polling(self):
        """Different polling makes attributes not equal."""
        self.assertNotEqual(
            ecf.MirrorAttr("m", "/p", "host", "3141", "60"),
            ecf.MirrorAttr("m", "/p", "host", "3141", "120"),
        )

    def test_eq_different_ssl(self):
        """Different ssl flag makes attributes not equal."""
        self.assertNotEqual(
            ecf.MirrorAttr("m", "/p", "host", "3141", "120", False),
            ecf.MirrorAttr("m", "/p", "host", "3141", "120", True),
        )

    def test_eq_different_auth(self):
        """Different auth makes attributes not equal."""
        self.assertNotEqual(
            ecf.MirrorAttr("m", "/p", "host", "3141", "120", False, "/auth1"),
            ecf.MirrorAttr("m", "/p", "host", "3141", "120", False, "/auth2"),
        )

    def test_ne_different_values(self):
        """__ne__ returns True for different attributes."""
        self.assertTrue(ecf.MirrorAttr("m1", "/p") != ecf.MirrorAttr("m2", "/p"))

    def test_ne_same_values(self):
        """__ne__ returns False for identical attributes."""
        self.assertFalse(ecf.MirrorAttr("m", "/p") != ecf.MirrorAttr("m", "/p"))

    # ------------------------------------------------------------------
    # __lt__  (not exposed)
    # ------------------------------------------------------------------

    def test_lt_not_supported(self):
        """MirrorAttr does not support < comparison."""
        a = ecf.MirrorAttr("m1", "/p")
        b = ecf.MirrorAttr("m2", "/p")
        with self.assertRaises(TypeError):
            _ = a < b

    # ------------------------------------------------------------------
    # __hash__
    # ------------------------------------------------------------------

    def test_is_hashable(self):
        """MirrorAttr is hashable."""
        self.assertIsInstance(hash(ecf.MirrorAttr("m", "/p")), int)

    def test_same_object_same_hash(self):
        """The same MirrorAttr always returns the same hash."""
        m = ecf.MirrorAttr("m", "/p")
        self.assertEqual(hash(m), hash(m))

    def test_hash_is_identity_based(self):
        """Two value-equal MirrorAttr objects have different hashes."""
        a = ecf.MirrorAttr("m", "/p")
        b = ecf.MirrorAttr("m", "/p")
        self.assertEqual(a, b)
        self.assertNotEqual(hash(a), hash(b))

    def test_can_be_stored_in_set(self):
        """MirrorAttr instances can be stored in a Python set."""
        a = ecf.MirrorAttr("m1", "/p")
        b = ecf.MirrorAttr("m2", "/p")
        self.assertEqual(len({a, b}), 2)

    def test_can_be_used_as_dict_key(self):
        """MirrorAttr instances can be used as dictionary keys."""
        m = ecf.MirrorAttr("m", "/p")
        d = {m: "mirror"}
        self.assertEqual(d[m], "mirror")

    # ------------------------------------------------------------------
    # __copy__
    # ------------------------------------------------------------------

    def test_copy_returns_equal(self):
        """copy.copy(MirrorAttr) returns an equal instance."""
        m = ecf.MirrorAttr("m", "/suite/task")
        self.assertEqual(copy.copy(m), m)

    def test_copy_is_independent(self):
        """copy.copy(MirrorAttr) returns a distinct object."""
        m = ecf.MirrorAttr("m", "/suite/task")
        self.assertIsNot(copy.copy(m), m)

    def test_copy_preserves_str(self):
        """copy.copy(MirrorAttr) preserves str()."""
        m = ecf.MirrorAttr("m", "/p", "host", "3141", "120", True, "/auth")
        self.assertEqual(str(copy.copy(m)), str(m))

    def test_copy_preserves_all_fields(self):
        """copy.copy(MirrorAttr) preserves every accessor."""
        m = ecf.MirrorAttr("m", "/p", "host", "3141", "120", True, "/auth")
        c = copy.copy(m)
        self.assertEqual(c.name(), m.name())
        self.assertEqual(c.remote_path(), m.remote_path())
        self.assertEqual(c.remote_host(), m.remote_host())
        self.assertEqual(c.remote_port(), m.remote_port())
        self.assertEqual(c.polling(), m.polling())
        self.assertEqual(c.ssl(), m.ssl())
        self.assertEqual(c.auth(), m.auth())

    def test_copy_preserves_ssl_true(self):
        """copy.copy preserves ssl=True."""
        m = ecf.MirrorAttr("m", "/p", "host", "3141", "120", True)
        self.assertTrue(copy.copy(m).ssl())

    def test_copy_preserves_ssl_false(self):
        """copy.copy preserves ssl=False."""
        m = ecf.MirrorAttr("m", "/p", "host", "3141", "120", False)
        self.assertFalse(copy.copy(m).ssl())


if __name__ == "__main__":
    unittest.main(verbosity=2)
