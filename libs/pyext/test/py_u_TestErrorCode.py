# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

import copy
import pickle
import unittest

import ecflow


class TestErrorCode(unittest.TestCase):
    def test_coded_failure_preserves_runtime_error_compatibility(self):
        error = ecflow.CodedError("formatted", ecflow.UNCLASSIFIED, "raw")
        self.assertIsInstance(error, RuntimeError)
        self.assertEqual(error.code(), 0)
        self.assertEqual(error.message(), "raw")
        self.assertEqual(str(error), "formatted")
        self.assertEqual(error.args, ("formatted",))

    def test_file_errors_have_distinct_codes(self):
        unavailable = ecflow.FileHandlingError("not found", ecflow.FILE_UNAVAILABLE, "raw")
        empty = ecflow.EmptyContentError("empty", "raw")
        self.assertEqual(unavailable.code(), ecflow.FILE_UNAVAILABLE)
        self.assertIsInstance(empty, ecflow.FileHandlingError)
        self.assertIsInstance(empty, RuntimeError)
        self.assertEqual(empty.code(), ecflow.EMPTY_CONTENT)
        self.assertEqual(empty.message(), "raw")

    def test_group_error_retains_order_and_aggregate_message(self):
        entries = [(ecflow.FILE_UNAVAILABLE, "first"), (0, "second")]
        error = ecflow.GroupError("combined", entries)
        self.assertIsInstance(error, RuntimeError)
        self.assertEqual(error.entries(), entries)
        self.assertEqual(str(error), "combined")
        self.assertEqual(error.args, ("combined",))

    def test_exception_defaults_and_round_trips(self):
        coded = ecflow.CodedError("coded", 9001)
        file_error = ecflow.FileHandlingError("missing")
        empty = ecflow.EmptyContentError("empty", "raw")
        group = ecflow.GroupError("group", [(9001, "first"), (0, "second")])
        self.assertEqual(coded.message(), "")
        self.assertEqual(file_error.code(), ecflow.FILE_UNAVAILABLE)
        self.assertEqual(empty.code(), ecflow.EMPTY_CONTENT)

        for error in (coded, file_error, empty, group):
            for restored in (pickle.loads(pickle.dumps(error)), copy.copy(error), copy.deepcopy(error)):
                self.assertIsInstance(restored, type(error))
                self.assertEqual(str(restored), str(error))
                if isinstance(error, ecflow.GroupError):
                    self.assertEqual(restored.entries(), error.entries())
                else:
                    self.assertEqual(restored.code(), error.code())
                    self.assertEqual(restored.message(), error.message())

    def test_python_subclass_initialises_and_round_trips(self):
        class CustomError(ecflow.CodedError):
            pass

        error = CustomError("custom", 9001, "raw")
        self.assertEqual(error.code(), 9001)
        self.assertEqual(error.message(), "raw")
        self.assertIsInstance(copy.copy(error), CustomError)


if __name__ == "__main__":
    unittest.main()
