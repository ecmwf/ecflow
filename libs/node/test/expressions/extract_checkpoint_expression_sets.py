#!/usr/bin/env python3
#
# Copyright 2009- ECMWF.
#
# This software is licensed under the terms of the Apache Licence version 2.0
# which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
# In applying this licence, ECMWF does not waive the privileges and immunities
# granted to it by virtue of its status as an intergovernmental organisation
# nor does it submit to any jurisdiction.
#
"""Extract raw trigger/complete expression parts from ecFlow checkpoints.

Each checkpoint produces one JSON file containing its attribute parts in source
order, including duplicate expressions.  The output deliberately does not fold
``trigger -a`` / ``trigger -o`` continuations: ExprParser parses each part
independently during definition loading, and its ExprDuplicate cache is keyed by
that individual expression text.  Keeping the raw parts therefore measures the
same duplicate distribution as production parsing.

Example:
    extract_checkpoint_expression_sets.py \
        --out-dir .scratch/checkpoint-expression-data \
        --input-dir .scratch/checkpoints/mlx \
        --input-dir .scratch/checkpoints/od_20260713T074025 \
        --input-dir .scratch/checkpoints/rd_20260722
"""

import argparse
import json
import pathlib
import re
import sys


ATTRIBUTE_RE = re.compile(r"^\s*(trigger|complete)\b(.*)$")
MODIFIER_RE = re.compile(r"^\s*(-[ao])\b\s*(.*)$")


def strip_comment(expression: str) -> str:
    """Return expression without a definition-file trailing comment."""
    marker = re.search(r"(?:^|\s)#", expression)
    if marker:
        expression = expression[: marker.start()]
    return expression.strip()


def extract_checkpoint(path: pathlib.Path) -> list[dict]:
    """Return ordered raw trigger/complete expression parts from one checkpoint."""
    expressions = []
    with path.open(encoding="latin-1") as checkpoint:
        for line_number, raw_line in enumerate(checkpoint, start=1):
            match = ATTRIBUTE_RE.match(raw_line.rstrip("\n"))
            if not match:
                continue

            kind, remainder = match.groups()
            if remainder and not remainder[0].isspace():
                continue

            modifier = None
            modifier_match = MODIFIER_RE.match(remainder)
            if modifier_match:
                modifier = modifier_match.group(1)
                remainder = modifier_match.group(2)

            expression = strip_comment(remainder)
            if expression:
                expressions.append(
                    {
                        "kind": kind,
                        "modifier": modifier,
                        "expression": expression,
                        "line": line_number,
                    }
                )
    return expressions


def output_path(input_dir: pathlib.Path, checkpoint: pathlib.Path, out_dir: pathlib.Path) -> pathlib.Path:
    """Return the JSON output path that mirrors checkpoint's path below input_dir."""
    relative = checkpoint.relative_to(input_dir)
    return out_dir / input_dir.name / relative.with_suffix(".expressions.json")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out-dir", required=True, type=pathlib.Path, help="Root output directory for JSON datasets")
    parser.add_argument(
        "--input-dir",
        action="append",
        required=True,
        type=pathlib.Path,
        help="Directory recursively containing .check files; may be supplied more than once",
    )
    arguments = parser.parse_args()

    total_checkpoints = 0
    total_expressions = 0
    for input_dir in arguments.input_dir:
        if not input_dir.is_dir():
            parser.error(f"input directory does not exist: {input_dir}")

        for checkpoint in sorted(input_dir.rglob("*.check")):
            expressions = extract_checkpoint(checkpoint)
            output = output_path(input_dir, checkpoint, arguments.out_dir)
            output.parent.mkdir(parents=True, exist_ok=True)
            document = {
                "schema_version": 1,
                "checkpoint": str(checkpoint),
                "expression_count": len(expressions),
                "expressions": expressions,
            }
            output.write_text(json.dumps(document, indent=2) + "\n", encoding="utf-8")
            print(f"{checkpoint}: {len(expressions)} expression part(s) -> {output}")
            total_checkpoints += 1
            total_expressions += len(expressions)

    print(f"Extracted {total_expressions} expression part(s) from {total_checkpoints} checkpoint(s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
