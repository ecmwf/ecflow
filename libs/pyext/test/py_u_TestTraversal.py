#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#

# This test ensures that traversal routines work correctly.
#
# The test loads a defs file from the disk, traverses the defs file, and creates another defs file.
# The original and the copy are compared and should be the same when traversal is correct.

import os
import fnmatch
import sys
import pathlib

import pytest

import ecflow


class _Indentor:
    """Manages indentation, for use with context manager."""

    _index = 0

    def __init__(self):
        _Indentor._index += 1

    def __del__(self):
        _Indentor._index -= 1

    @classmethod
    def indent(cls, the_file):
        for _ in range(cls._index):
            the_file.write(" ")


class DefsTraverser:
    """Traverse the ecflow.Defs definition and write to file."""

    def __init__(self, defs, file_name):
        self._defs = defs
        self._file = open(file_name, "w")

    def write_to_file(self):
        for extern in self._defs.externs:
            self._writeln("extern " + extern)
        for suite in self._defs:
            self._write("suite ")
            self._print_node(suite)
            clock = suite.get_clock()
            if clock:
                indent = _Indentor()
                self._writeln(str(clock))
                del indent
            self._print_nc(suite)
            self._writeln("endsuite")
        self._file.close()

    def _print_nc(self, node_container):
        indent = _Indentor()
        for node in node_container:
            if isinstance(node, ecflow.Task):
                self._write("task ")
                self._print_node(node)
                self._print_alias(node)
            else:
                self._write("family ")
                self._print_node(node)
                self._print_nc(node)
                self._writeln("endfamily")
        del indent

    def _print_alias(self, task):
        indent = _Indentor()
        for alias in task:
            self._write("alias ")
            self._print_node(alias)
            self._writeln("endalias")
        del indent

    def _print_node(self, node):
        self._file.write(node.name() + " # state:" + str(node.get_state()) + "\n")

        indent = _Indentor()
        def_status = node.get_defstatus()
        if def_status != ecflow.DState.queued:
            self._writeln("defstatus " + str(def_status))

        autocancel = node.get_autocancel()
        if autocancel:
            self._writeln(str(autocancel))

        autoarchive = node.get_autoarchive()
        if autoarchive:
            self._writeln(str(autoarchive))

        autorestore = node.get_autorestore()
        if autorestore:
            self._writeln(str(autorestore))

        repeat = node.get_repeat()
        if not repeat.empty():
            self._writeln(str(repeat) + " # value: " + str(repeat.value()))

        late = node.get_late()
        if late:
            self._writeln(str(late) + " # is_late: " + str(late.is_late()))

        complete_expr = node.get_complete()
        if complete_expr:
            for part_expr in complete_expr.parts:
                trig = "complete "
                if part_expr.and_expr():
                    trig = trig + "-a "
                if part_expr.or_expr():
                    trig = trig + "-o "
                self._write(trig)
                self._file.write(part_expr.get_expression() + "\n")
        trigger_expr = node.get_trigger()
        if trigger_expr:
            for part_expr in trigger_expr.parts:
                trig = "trigger "
                if part_expr.and_expr():
                    trig = trig + "-a "
                if part_expr.or_expr():
                    trig = trig + "-o "
                self._write(trig)
                self._file.write(part_expr.get_expression() + "\n")

        for var in node.variables:
            self._writeln("edit " + var.name() + " '" + var.value() + "'")
        for meter in node.meters:
            self._writeln(str(meter) + " # value: " + str(meter.value()))
        for event in node.events:
            self._writeln(str(event) + " # value: " + str(event.value()))
        for label in node.labels:
            self._writeln(str(label) + " # value: " + label.new_value())
        for limit in node.limits:
            self._writeln(str(limit) + " # value: " + str(limit.value()))
        for inlimit in node.inlimits:
            self._writeln(str(inlimit))
        for the_time in node.times:
            self._writeln(str(the_time))
        for today in node.todays:
            self._writeln(str(today))
        for date in node.dates:
            self._writeln(str(date))
        for day in node.days:
            self._writeln(str(day))
        for cron in node.crons:
            self._writeln(str(cron))
        for verify in node.verifies:
            self._writeln(str(verify))
        for zombie in node.zombies:
            self._writeln(str(zombie))
        for queue in node.queues:
            self._writeln(str(queue))
        for generic in node.generics:
            self._writeln(str(generic))

        del indent

    def _write(self, the_string):
        _Indentor.indent(self._file)
        self._file.write(the_string)

    def _writeln(self, the_string):
        _Indentor.indent(self._file)
        self._file.write(the_string + "\n")


def _check_traversal(path_to_def):
    reference_def = ecflow.Defs(path_to_def)

    file_name = "copy.def"
    traverser = DefsTraverser(reference_def, file_name)
    traverser.write_to_file()

    try:
        traversed_def = ecflow.Defs(file_name)
    except RuntimeError as e:
        pytest.fail("Could not parse file " + file_name + "\n" + str(e))

    ecflow.Ecf.set_debug_equality(True)
    try:
        defs_equal = traversed_def == reference_def
        if not defs_equal:
            with open(file_name) as f:
                traversed_content = f.read()
            pytest.fail(
                str(path_to_def) + " FAILED\n"
                "The traversed defs=========\n" + str(traversed_def) + "\n"
                "not the same as reference def============\n"
                + str(reference_def)
                + "\n"
                "===================== "
                + file_name
                + " ====================================\n"
                + traversed_content
            )
    finally:
        ecflow.Ecf.set_debug_equality(False)

    if os.path.exists(file_name):
        os.remove(file_name)


def _all_files(root, patterns="*", single_level=False, yield_folders=False):
    patterns = patterns.split(";")
    for path, _subdirs, files in os.walk(root):
        if yield_folders:
            files.extend(_subdirs)
        files.sort()
        for name in files:
            for pattern in patterns:
                if fnmatch.fnmatch(name, pattern):
                    yield os.path.join(path, name)
                    break
        if single_level:
            break


@pytest.fixture
def parser_good_defs_dir():
    source_dir = ecflow.File.source_dir()
    path = (
        pathlib.Path(source_dir)
        / "libs"
        / "node"
        / "test"
        / "parser"
        / "data"
        / "good_defs"
    )
    assert path.exists(), f"Parser good_defs directory not found: {path}"
    return path


def test_traversal_of_all_good_defs(parser_good_defs_dir):
    for path_to_defs_file in _all_files(str(parser_good_defs_dir), "*.def;*.txt"):
        _check_traversal(path_to_defs_file)
