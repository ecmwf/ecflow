# SPDX-FileCopyrightText: 2009- European Centre for Medium-Range Weather Forecasts (ECMWF)
# SPDX-License-Identifier: Apache-2.0

import pytest

from ecflow import Defs

# A suite whose statements are all on a single, ';' joined, line.
SINGLE_LINE_SUITE = (
    "suite s2; clock real +01:00; endclock +02:00;"
    " task ta; family f1; task fa; endfamily; task tb; endsuite"
)

DEFINITION = "suite s1\n task t1\nendsuite\n" + SINGLE_LINE_SUITE


def _write(path, content):
    """Writes the content verbatim, in particular without appending a final newline."""
    with open(path, "w", newline="") as file:
        file.write(content)
    return str(path)


def _check_single_line_suite(defs):
    """Checks that the ';' joined suite was parsed in full, and not truncated."""
    suite = defs.find_abs_node("/s2")
    assert suite is not None, "expected to find suite /s2"
    assert suite.get_clock() is not None, "expected a clock on /s2"

    for path in ["/s2/ta", "/s2/f1", "/s2/f1/fa", "/s2/tb"]:
        assert defs.find_abs_node(path) is not None, "expected to find node " + path


@pytest.mark.parametrize("ending", ["", "\n"])
def test_parsing_last_line_with_multiple_statements(tmp_path, ending):
    path = _write(tmp_path / "multi_statement.def", DEFINITION + ending)

    defs = Defs(path)

    _check_single_line_suite(defs)
    assert len(list(defs.suites)) == 2, "expected two suites"


def test_parsing_definition_made_of_a_single_line(tmp_path):
    path = _write(tmp_path / "single_line.def", SINGLE_LINE_SUITE)

    defs = Defs(path)

    _check_single_line_suite(defs)
    assert len(list(defs.suites)) == 1, "expected a single suite"


def test_parsing_invalid_statement_in_last_line_fails(tmp_path):
    # The end clock precedes the start clock, which is only detected once the whole line is parsed.
    path = _write(
        tmp_path / "invalid.def",
        "suite s1; clock real +02:00; endclock +01:00; task t1; endsuite",
    )

    with pytest.raises(RuntimeError):
        Defs(path)
